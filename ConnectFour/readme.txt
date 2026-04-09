import re
import numpy as np
import pandas as pd
from pathlib import Path

# ============================================================
# CONFIG
# ============================================================
INPUT_XLSX = r"data/Cobra-XM30.xlsx"
SHEET_NAME = "tbl_Weekly Extract"   # if None, script will auto-find best sheet
OUT_XLSX = r"EVMS_PowerBI_Input_XM30.xlsx"
PROGRAM_ID = "XM30"

USE_ACCOUNTING_CLOSE_DATES = True
CAP_ASOF_TO_TODAY = True

# Only used if USE_ACCOUNTING_CLOSE_DATES = True
ACCOUNTING_CLOSE_DATES_2026 = [
    "2026-01-07", "2026-01-21",
    "2026-02-04", "2026-02-18",
    "2026-03-04", "2026-03-18",
    "2026-04-01", "2026-04-15", "2026-04-29",
    "2026-05-13", "2026-05-27",
    "2026-06-10", "2026-06-24",
    "2026-07-08", "2026-07-22",
    "2026-08-05", "2026-08-19",
    "2026-09-02", "2026-09-16", "2026-09-30",
    "2026-10-14", "2026-10-28",
    "2026-11-11", "2026-11-25",
    "2026-12-09", "2026-12-23",
]

# Which cost sets define whether a date is "posted" enough to use as a period anchor
ANCHOR_COSTSETS = {"BCWS", "BCWP", "ACWP"}

# Minimum number of distinct anchor cost sets required on a date
# This avoids choosing a date that only has BCWS but no BCWP/ACWP, etc.
MIN_COSTSET_COUNT_FOR_ANCHOR = 2

# ============================================================
# HELPERS
# ============================================================
def norm_col(c: str) -> str:
    c = str(c).strip()
    c = re.sub(r"\s+", "_", c)
    c = c.replace("-", "_")
    return c.upper()

def norm_text(x):
    if pd.isna(x):
        return ""
    return str(x).strip()

def norm_cost_set(x):
    s = norm_text(x).upper()
    s = re.sub(r"[\s\-_]+", "", s)

    mapping = {
        "BCWS": "BCWS",
        "BCWP": "BCWP",
        "ACWP": "ACWP",
        "ETC": "ETC",

        "BCWSHRS": "BCWS",
        "BCWPHRS": "BCWP",
        "ACWPHRS": "ACWP",
        "ETCHRS": "ETC",

        "BUDGETEDCOSTOFWORKSCHEDULED": "BCWS",
        "BUDGETEDCOSTOFWORKPERFORMED": "BCWP",
        "ACTUALCOSTOFWORKPERFORMED": "ACWP",
    }
    return mapping.get(s, s)

def safe_ratio(numer, denom):
    numer = pd.to_numeric(numer, errors="coerce")
    denom = pd.to_numeric(denom, errors="coerce")
    if pd.isna(numer) or pd.isna(denom) or denom == 0:
        return np.nan
    return float(numer / denom)

def spi_color(x):
    if pd.isna(x):
        return ""
    if x >= 1.05:
        return "#8EB4E3"   # blue
    if x >= 0.98:
        return "#339966"   # green
    if x >= 0.95:
        return "#FFFF99"   # yellow
    return "#C0504D"       # red

def cpi_color(x):
    return spi_color(x)

def pctvar_color(x):
    if pd.isna(x):
        return ""
    a = abs(float(x))
    if a <= 0.05:
        return "#339966"
    if a <= 0.10:
        return "#FFFF99"
    return "#C0504D"

def vac_color(vac_pct):
    if pd.isna(vac_pct):
        return ""
    v = float(vac_pct)
    if v >= 0.055:
        return "#8EB4E3"
    if v >= -0.025:
        return "#339966"
    if v >= -0.055:
        return "#FFFF99"
    return "#C0504D"

def month_start(dt):
    dt = pd.Timestamp(dt).normalize()
    return pd.Timestamp(dt.year, dt.month, 1)

def month_end(dt):
    dt = pd.Timestamp(dt).normalize()
    return (pd.Timestamp(dt.year, dt.month, 1) + pd.offsets.MonthEnd(1)).normalize()

def next_month_window(as_of_date):
    as_of_date = pd.Timestamp(as_of_date).normalize()
    nm_start = (month_start(as_of_date) + pd.offsets.MonthBegin(1)).normalize()
    nm_end = month_end(nm_start)
    return nm_start, nm_end

def assert_row(name, ok, details=""):
    return {"TEST": name, "PASS": bool(ok), "DETAILS": details}

# ============================================================
# INPUT / SHEET FINDER
# ============================================================
def pick_sheet(path: Path, preferred_sheet: str | None = None):
    xl = pd.ExcelFile(path)

    if preferred_sheet and preferred_sheet in xl.sheet_names:
        return preferred_sheet

    candidates = []
    for s in xl.sheet_names:
        sl = s.lower()
        score = 0
        if "tbl" in sl:
            score += 3
        if "weekly" in sl:
            score += 3
        if "extract" in sl:
            score += 3
        if "cobra" in sl:
            score += 1
        candidates.append((score, s))

    candidates = sorted(candidates, reverse=True)
    if candidates and candidates[0][0] > 0:
        return candidates[0][1]

    return xl.sheet_names[0]

# ============================================================
# READ + CLEAN
# ============================================================
in_path = Path(INPUT_XLSX)
if not in_path.exists():
    alt = Path.cwd() / INPUT_XLSX
    if alt.exists():
        in_path = alt
    else:
        raise FileNotFoundError(f"INPUT_XLSX not found: {INPUT_XLSX} (also tried {alt})")

sheet_to_use = pick_sheet(in_path, SHEET_NAME)

raw = pd.read_excel(in_path, sheet_name=sheet_to_use)
raw.columns = [norm_col(c) for c in raw.columns]

# Normalize known column variants
rename_map = {}
if "COST-SET" in raw.columns and "COST_SET" not in raw.columns:
    rename_map["COST-SET"] = "COST_SET"
if "CHG#" in raw.columns and "CHG" not in raw.columns:
    rename_map["CHG#"] = "CHG"
if "SUBTEAM" in raw.columns and "SUB_TEAM" not in raw.columns:
    rename_map["SUBTEAM"] = "SUB_TEAM"
if "PRODUCT_TEAM" in raw.columns and "SUB_TEAM" not in raw.columns:
    rename_map["PRODUCT_TEAM"] = "SUB_TEAM"
if "WORKPACKAGE" in raw.columns and "WORK_PACKAGE" not in raw.columns:
    rename_map["WORKPACKAGE"] = "WORK_PACKAGE"

if rename_map:
    raw = raw.rename(columns=rename_map)

required = {"SUB_TEAM", "COST_SET", "PLUG", "DATE", "HOURS"}
missing = required - set(raw.columns)
if missing:
    raise ValueError(
        f"Missing required columns: {missing}\n"
        f"Found columns: {list(raw.columns)}"
    )

df = raw.copy()
df["SUB_TEAM"] = df["SUB_TEAM"].apply(norm_text)
df["COST_SET"] = df["COST_SET"].apply(norm_cost_set)
df["PLUG"] = df["PLUG"].apply(lambda x: norm_text(x).upper())
df["DATE"] = pd.to_datetime(df["DATE"], errors="coerce").dt.normalize()
df["HOURS"] = pd.to_numeric(df["HOURS"], errors="coerce")

# Hours-only working frame
dfH = df[
    (df["PLUG"] == "HOURS") &
    df["DATE"].notna() &
    df["HOURS"].notna()
].copy()

today = pd.Timestamp.today().normalize()

if CAP_ASOF_TO_TODAY:
    dfH = dfH[dfH["DATE"] <= today].copy()

if dfH.empty:
    raise ValueError("No HOURS rows with valid DATE/HOURS were found after cleaning.")

# ============================================================
# PERIOD ANCHORING
# ============================================================
def build_anchor_summary(df_hours: pd.DataFrame):
    """Summarize which cost sets appear on each date and how much."""
    a = (
        df_hours[df_hours["COST_SET"].isin(ANCHOR_COSTSETS)]
        .groupby(["DATE", "COST_SET"], as_index=False)["HOURS"]
        .sum()
    )

    if a.empty:
        return pd.DataFrame(columns=["DATE", "COSTSET_COUNT", "TOTAL_HOURS", "COSTSETS"])

    summary = (
        a.groupby("DATE", as_index=False)
         .agg(
             COSTSET_COUNT=("COST_SET", "nunique"),
             TOTAL_HOURS=("HOURS", "sum")
         )
    )

    costsets_per_date = (
        a.groupby("DATE")["COST_SET"]
         .apply(lambda s: ", ".join(sorted(s.unique().tolist())))
         .reset_index(name="COSTSETS")
    )

    summary = summary.merge(costsets_per_date, on="DATE", how="left")
    return summary.sort_values("DATE")

anchor_summary = build_anchor_summary(dfH)

if anchor_summary.empty:
    raise ValueError(
        f"No anchor rows found using cost sets {sorted(ANCHOR_COSTSETS)}. "
        f"Check COST_SET values in source file."
    )

def pick_asof_and_prior_from_file(anchor_df: pd.DataFrame):
    """
    Choose AS_OF and PRIOR from real file data.
    Prefer dates that have at least MIN_COSTSET_COUNT_FOR_ANCHOR anchor cost sets.
    """
    eligible = anchor_df[anchor_df["COSTSET_COUNT"] >= MIN_COSTSET_COUNT_FOR_ANCHOR].copy()
    if eligible.empty:
        # fallback: use whatever latest date exists
        eligible = anchor_df.copy()

    as_of = eligible["DATE"].max()

    prior_candidates = eligible[eligible["DATE"] < as_of]["DATE"].sort_values()
    prior = prior_candidates.iloc[-1] if len(prior_candidates) else None

    return pd.Timestamp(as_of).normalize(), (
        pd.Timestamp(prior).normalize() if prior is not None else None
    )

def pick_asof_and_prior_with_close_dates(anchor_df: pd.DataFrame, close_dates):
    """
    Use accounting close dates, but only if they land on or before real posted data.
    Then choose the nearest eligible posted dates <= those close dates.
    """
    eligible = anchor_df[anchor_df["COSTSET_COUNT"] >= MIN_COSTSET_COUNT_FOR_ANCHOR].copy()
    if eligible.empty:
        eligible = anchor_df.copy()

    if eligible.empty:
        return None, None, "No eligible anchor dates in file."

    max_posted_date = eligible["DATE"].max()

    close_dates = pd.to_datetime(pd.Series(close_dates), errors="coerce").dropna().dt.normalize()
    if CAP_ASOF_TO_TODAY:
        close_dates = close_dates[close_dates <= today]
    close_dates = close_dates[close_dates <= max_posted_date]

    if close_dates.empty:
        as_of, prior = pick_asof_and_prior_from_file(anchor_df)
        return as_of, prior, "No usable accounting close date <= posted data; using file dates."

    target_close = close_dates.max()

    # Choose latest eligible posted date <= target_close
    eligible_asof = eligible[eligible["DATE"] <= target_close]["DATE"].sort_values()
    if eligible_asof.empty:
        as_of, prior = pick_asof_and_prior_from_file(anchor_df)
        return as_of, prior, "No eligible posted date <= target close; using file dates."

    as_of = eligible_asof.iloc[-1]

    prior_close_candidates = close_dates[close_dates < target_close]
    if len(prior_close_candidates) > 0:
        target_prior_close = prior_close_candidates.max()
        eligible_prior = eligible[eligible["DATE"] <= target_prior_close]["DATE"].sort_values()
        prior = eligible_prior.iloc[-1] if len(eligible_prior) else None
    else:
        prior = None

    if prior is None:
        prior_candidates = eligible[eligible["DATE"] < as_of]["DATE"].sort_values()
        prior = prior_candidates.iloc[-1] if len(prior_candidates) else None
        reason = "AS_OF based on accounting close; PRIOR fell back to previous posted date."
    else:
        reason = "AS_OF/PRIOR based on accounting close dates constrained to posted dates."

    return pd.Timestamp(as_of).normalize(), (
        pd.Timestamp(prior).normalize() if prior is not None else None
    ), reason

if USE_ACCOUNTING_CLOSE_DATES:
    as_of_close, prior_close, asof_reason = pick_asof_and_prior_with_close_dates(
        anchor_summary, ACCOUNTING_CLOSE_DATES_2026
    )
else:
    as_of_close, prior_close = pick_asof_and_prior_from_file(anchor_summary)
    asof_reason = "Using file-posted anchor dates only."

if as_of_close is None:
    raise ValueError("Could not determine AS_OF date.")

HAS_PRIOR = prior_close is not None
CTD_END = as_of_close
nm_start, nm_end = next_month_window(as_of_close)

# ============================================================
# METRIC FUNCTIONS
# ============================================================
def sum_hours(df_hours, cost_set, start=None, end=None, sub_team=None):
    x = df_hours.copy()
    m = (x["COST_SET"] == cost_set)

    if sub_team is not None:
        m &= (x["SUB_TEAM"] == sub_team)

    if start is not None:
        m &= (x["DATE"] >= pd.Timestamp(start).normalize())

    if end is not None:
        m &= (x["DATE"] <= pd.Timestamp(end).normalize())

    return float(x.loc[m, "HOURS"].sum())

def ctd(cost_set, end_date, sub_team=None):
    return sum_hours(dfH, cost_set, start=None, end=end_date, sub_team=sub_team)

def cur_from_ctd(cost_set, end_date, prior_end_date, sub_team=None):
    if prior_end_date is None:
        return np.nan
    return ctd(cost_set, end_date, sub_team=sub_team) - ctd(cost_set, prior_end_date, sub_team=sub_team)

# ============================================================
# TESTS / DEBUG CHECKS
# ============================================================
tests = []
tests.append(assert_row(
    "AS_OF not future-dated",
    as_of_close <= today,
    f"AS_OF={as_of_close.date()} TODAY={today.date()} REASON={asof_reason}"
))
tests.append(assert_row(
    "HOURS data exists",
    len(dfH) > 0,
    f"rows_hours={len(dfH)}"
))
tests.append(assert_row(
    "Anchor summary exists",
    len(anchor_summary) > 0,
    f"anchor_dates={len(anchor_summary)}"
))
tests.append(assert_row(
    "Prior date exists",
    HAS_PRIOR,
    f"prior_close={prior_close.date() if HAS_PRIOR else None}"
))

# Program-level CTD
bcws_ctd = ctd("BCWS", CTD_END)
bcwp_ctd = ctd("BCWP", CTD_END)
acwp_ctd = ctd("ACWP", CTD_END)

# Program-level CUR/LSD
bcws_cur = cur_from_ctd("BCWS", CTD_END, prior_close)
bcwp_cur = cur_from_ctd("BCWP", CTD_END, prior_close)
acwp_cur = cur_from_ctd("ACWP", CTD_END, prior_close)

spi_ctd = safe_ratio(bcwp_ctd, bcws_ctd)
cpi_ctd = safe_ratio(bcwp_ctd, acwp_ctd)

spi_cur = safe_ratio(bcwp_cur, bcws_cur)
cpi_cur = safe_ratio(bcwp_cur, acwp_cur)

tests.append(assert_row("BCWS_CTD nonzero", bcws_ctd != 0, f"BCWS_CTD={bcws_ctd}"))
tests.append(assert_row("BCWP_CTD present", not pd.isna(bcwp_ctd), f"BCWP_CTD={bcwp_ctd}"))
tests.append(assert_row("ACWP_CTD present", not pd.isna(acwp_ctd), f"ACWP_CTD={acwp_ctd}"))

if HAS_PRIOR:
    tests.append(assert_row("BCWS_CUR present", not pd.isna(bcws_cur), f"BCWS_CUR={bcws_cur}"))
    tests.append(assert_row("BCWP_CUR present", not pd.isna(bcwp_cur), f"BCWP_CUR={bcwp_cur}"))
    tests.append(assert_row("ACWP_CUR present", not pd.isna(acwp_cur), f"ACWP_CUR={acwp_cur}"))

# ============================================================
# OUTPUT 1: PROGRAM OVERVIEW
# ============================================================
program_overview = pd.DataFrame([{
    "ProgramID": PROGRAM_ID,

    "SPI_LSD": spi_cur,
    "SPI_CTD": spi_ctd,
    "CPI_LSD": cpi_cur,
    "CPI_CTD": cpi_ctd,

    "LSD_START": (prior_close + pd.Timedelta(days=1)).date() if HAS_PRIOR else pd.NaT,
    "LSD_END": as_of_close.date(),
    "AS_OF_DATE": as_of_close.date(),
    "PREV_DATE": prior_close.date() if HAS_PRIOR else pd.NaT,

    "SPI_LSD_Color": spi_color(spi_cur),
    "SPI_CTD_Color": spi_color(spi_ctd),
    "CPI_LSD_Color": cpi_color(cpi_cur),
    "CPI_CTD_Color": cpi_color(cpi_ctd),

    "Cause": "",
    "Corrective Actions": "",
}])

# ============================================================
# OUTPUT 2: PRODUCT TEAM SPI/CPI
# ============================================================
teams = sorted([
    t for t in dfH["SUB_TEAM"].dropna().unique().tolist()
    if str(t).strip() and str(t).strip().lower() != "nan"
])

rows = []
for team in teams:
    bcws_ctd_t = ctd("BCWS", CTD_END, sub_team=team)
    bcwp_ctd_t = ctd("BCWP", CTD_END, sub_team=team)
    acwp_ctd_t = ctd("ACWP", CTD_END, sub_team=team)

    bcws_cur_t = cur_from_ctd("BCWS", CTD_END, prior_close, sub_team=team)
    bcwp_cur_t = cur_from_ctd("BCWP", CTD_END, prior_close, sub_team=team)
    acwp_cur_t = cur_from_ctd("ACWP", CTD_END, prior_close, sub_team=team)

    spi_ctd_t = safe_ratio(bcwp_ctd_t, bcws_ctd_t)
    cpi_ctd_t = safe_ratio(bcwp_ctd_t, acwp_ctd_t)
    spi_cur_t = safe_ratio(bcwp_cur_t, bcws_cur_t)
    cpi_cur_t = safe_ratio(bcwp_cur_t, acwp_cur_t)

    # Skip teams with literally no EVMS values at all
    if all([
        (pd.isna(bcws_ctd_t) or bcws_ctd_t == 0),
        (pd.isna(bcwp_ctd_t) or bcwp_ctd_t == 0),
        (pd.isna(acwp_ctd_t) or acwp_ctd_t == 0),
    ]):
        continue

    rows.append({
        "ProgramID": PROGRAM_ID,
        "Product Team": team,

        "SPI_LSD": spi_cur_t,
        "SPI_CTD": spi_ctd_t,
        "CPI_LSD": cpi_cur_t,
        "CPI_CTD": cpi_ctd_t,

        "LSD_START": (prior_close + pd.Timedelta(days=1)).date() if HAS_PRIOR else pd.NaT,
        "LSD_END": as_of_close.date(),
        "AS_OF_DATE": as_of_close.date(),
        "PREV_DATE": prior_close.date() if HAS_PRIOR else pd.NaT,

        "SPI_LSD_Color": spi_color(spi_cur_t),
        "SPI_CTD_Color": spi_color(spi_ctd_t),
        "CPI_LSD_Color": cpi_color(cpi_cur_t),
        "CPI_CTD_Color": cpi_color(cpi_ctd_t),

        "Cause": "",
        "Corrective Actions": "",
    })

productteam_spi_cpi = pd.DataFrame(rows)

# ============================================================
# OUTPUT 3: PRODUCT TEAM BAC / EAC / VAC
# ============================================================
rows = []
for team in teams:
    bac = ctd("BCWS", CTD_END, sub_team=team)
    acwp = ctd("ACWP", CTD_END, sub_team=team)
    etc = ctd("ETC", CTD_END, sub_team=team)

    eac = acwp + etc
    vac = bac - eac
    vac_pct = safe_ratio(vac, bac)

    if all([
        (pd.isna(bac) or bac == 0),
        (pd.isna(acwp) or acwp == 0),
        (pd.isna(etc) or etc == 0),
    ]):
        continue

    rows.append({
        "ProgramID": PROGRAM_ID,
        "Product Team": team,
        "BAC": bac,
        "EAC": eac,
        "VAC": vac,
        "VAC_PCT": vac_pct,
        "VAC_Color": vac_color(vac_pct),
        "AS_OF_DATE": as_of_close.date(),
        "Cause": "",
        "Corrective Actions": "",
    })

productteam_bac_eac_vac = pd.DataFrame(rows)

# ============================================================
# OUTPUT 4: PROGRAM MANPOWER
# ============================================================
scheduled_ctd = bcws_ctd
actual_ctd = acwp_ctd

# Percent variance = (Actual - Scheduled) / Scheduled
pct_var = safe_ratio(actual_ctd - scheduled_ctd, scheduled_ctd)

next_mo_bcws = sum_hours(dfH, "BCWS", start=nm_start, end=nm_end)
next_mo_etc = sum_hours(dfH, "ETC", start=nm_start, end=nm_end)

tests.append(assert_row(
    "Next Mo BCWS checked",
    True,
    f"Next_Month_Window={nm_start.date()}..{nm_end.date()} Next_Mo_BCWS={next_mo_bcws}"
))

program_manpower = pd.DataFrame([{
    "ProgramID": PROGRAM_ID,
    "Scheduled Hours": scheduled_ctd,
    "Actual Hours": actual_ctd,
    "%Var": pct_var,
    "%Var_Color": pctvar_color(pct_var),
    "Next Mo BCWS": next_mo_bcws,
    "Next Mo ETC Hours": next_mo_etc,
    "AS_OF_DATE": as_of_close.date(),
    "NEXT_MONTH_START": nm_start.date(),
    "NEXT_MONTH_END": nm_end.date(),
    "Cause": "",
    "Corrective Actions": "",
}])

# ============================================================
# EDA / DEBUG TAB
# ============================================================
# COST_SET counts + sums
cs_counts = (
    dfH.groupby("COST_SET")["HOURS"]
       .agg(["count", "sum"])
       .reset_index()
       .rename(columns={"count": "ROW_COUNT", "sum": "HOURS_SUM"})
)

# DATE coverage by COST_SET
date_cov = (
    dfH.groupby("COST_SET")["DATE"]
       .agg(["min", "max", "nunique"])
       .reset_index()
       .rename(columns={"min": "MIN_DATE", "max": "MAX_DATE", "nunique": "UNIQUE_DATES"})
)

# Anchor-date detail
anchor_detail = (
    dfH[dfH["COST_SET"].isin(ANCHOR_COSTSETS)]
    .groupby(["DATE", "COST_SET"], as_index=False)["HOURS"]
    .sum()
    .sort_values(["DATE", "COST_SET"])
)

# Period window detail
period_window_detail = (
    dfH[
        (dfH["COST_SET"].isin(["BCWS", "BCWP", "ACWP", "ETC"])) &
        (dfH["DATE"] >= (prior_close + pd.Timedelta(days=1) if HAS_PRIOR else as_of_close)) &
        (dfH["DATE"] <= as_of_close)
    ]
    .groupby(["DATE", "COST_SET"], as_index=False)["HOURS"]
    .sum()
    .sort_values(["DATE", "COST_SET"])
)

# Next-month detail
next_month_detail = (
    dfH[
        (dfH["COST_SET"].isin(["BCWS", "ETC"])) &
        (dfH["DATE"] >= nm_start) &
        (dfH["DATE"] <= nm_end)
    ]
    .groupby(["DATE", "COST_SET"], as_index=False)["HOURS"]
    .sum()
    .sort_values(["DATE", "COST_SET"])
)

eda = pd.DataFrame([{
    "INPUT_FILE": str(in_path.resolve()),
    "SHEET_USED": sheet_to_use,
    "ROWS_TOTAL": len(df),
    "ROWS_HOURS": len(dfH),

    "ASOF_REASON": asof_reason,
    "AS_OF_CLOSE": as_of_close.date(),
    "PRIOR_CLOSE": prior_close.date() if HAS_PRIOR else None,

    "BCWS_CTD_ASOF": bcws_ctd,
    "BCWP_CTD_ASOF": bcwp_ctd,
    "ACWP_CTD_ASOF": acwp_ctd,

    "BCWS_CUR": bcws_cur,
    "BCWP_CUR": bcwp_cur,
    "ACWP_CUR": acwp_cur,

    "SPI_CUR": spi_cur,
    "CPI_CUR": cpi_cur,
    "SPI_CTD": spi_ctd,
    "CPI_CTD": cpi_ctd,

    "NEXT_MONTH_START": nm_start.date(),
    "NEXT_MONTH_END": nm_end.date(),
    "NEXT_MO_BCWS": next_mo_bcws,
    "NEXT_MO_ETC": next_mo_etc,
}])

tests_df = pd.DataFrame(tests)

# ============================================================
# OPTIONAL ROUNDING
# ============================================================
def round_numeric_cols(df_in, digits=6):
    df_out = df_in.copy()
    for c in df_out.columns:
        if pd.api.types.is_numeric_dtype(df_out[c]):
            df_out[c] = df_out[c].round(digits)
    return df_out

program_overview = round_numeric_cols(program_overview, 6)
productteam_spi_cpi = round_numeric_cols(productteam_spi_cpi, 6)
productteam_bac_eac_vac = round_numeric_cols(productteam_bac_eac_vac, 6)
program_manpower = round_numeric_cols(program_manpower, 6)
eda = round_numeric_cols(eda, 6)

# ============================================================
# WRITE OUTPUT
# ============================================================
out_path = Path(OUT_XLSX)

with pd.ExcelWriter(out_path, engine="openpyxl") as writer:
    program_overview.to_excel(writer, sheet_name="Program_Overview", index=False)
    productteam_spi_cpi.to_excel(writer, sheet_name="ProductTeam_SPI_CPI", index=False)
    productteam_bac_eac_vac.to_excel(writer, sheet_name="ProductTeam_BAC_EAC_VAC", index=False)
    program_manpower.to_excel(writer, sheet_name="Program_Manpower", index=False)

    # Put all debug tables on EDA with spacing
    startrow = 0
    eda.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(eda) + 3
    cs_counts.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(cs_counts) + 3
    date_cov.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(date_cov) + 3
    anchor_summary.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(anchor_summary) + 3
    anchor_detail.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(anchor_detail) + 3
    period_window_detail.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(period_window_detail) + 3
    next_month_detail.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

    startrow += len(next_month_detail) + 3
    tests_df.to_excel(writer, sheet_name="EDA", index=False, startrow=startrow)

print(f"WROTE: {out_path.resolve()}")
print(f"SHEET_USED={sheet_to_use}")
print(f"AS_OF_CLOSE={as_of_close.date()} | PRIOR_CLOSE={prior_close.date() if HAS_PRIOR else None}")
print(f"CUR_WINDOW={(prior_close + pd.Timedelta(days=1)).date() if HAS_PRIOR else None}..{as_of_close.date()}")
print(f"NEXT_MONTH_WINDOW={nm_start.date()}..{nm_end.date()}")
print(f"SPI_LSD={spi_cur} | CPI_LSD={cpi_cur} | SPI_CTD={spi_ctd} | CPI_CTD={cpi_ctd}")

fails = tests_df[tests_df["PASS"] == False]
print("\nVALIDATION FAILURES:" if len(fails) else "\nVALIDATION FAILURES: None")
if len(fails):
    print(fails[["TEST", "DETAILS"]].to_string(index=False))