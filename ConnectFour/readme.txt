
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from IPython.display import display

# ==========================================================
# NEW CUSTOMER FIELD ANALYSIS
# ==========================================================

# Use the DataFrame already loaded in your notebook
data = df.copy()

DATE_COL = "SDate"
NEW_COL = "NewCustomer"

# Clean dates and customer flags
data[DATE_COL] = pd.to_datetime(
    data[DATE_COL], errors="coerce"
)

data["NewCustomer_Clean"] = (
    data[NEW_COL]
    .astype("string")
    .str.strip()
    .str.lower()
    .map({
        "true": True,
        "false": False,
        "1": True,
        "0": False
    })
)

data = data.dropna(subset=[DATE_COL])

if data.empty:
    raise ValueError("No valid SDate values found.")

# Use the latest available date in the dataset
end_date = data[DATE_COL].max().normalize()

print("=" * 55)
print("NEW CUSTOMER FIELD ANALYSIS")
print("Latest Date in Dataset:", end_date.date())
print("=" * 55)

# ==========================================================
# 1. TIME PERIOD SUMMARY
# ==========================================================

periods = {
    "Last 7 Days": end_date - pd.Timedelta(days=6),
    "Last 1 Month": end_date - pd.DateOffset(months=1),
    "Last 3 Months": end_date - pd.DateOffset(months=3),
    "Last 6 Months": end_date - pd.DateOffset(months=6),
    "Last 12 Months": end_date - pd.DateOffset(months=12),
    "Last 2 Years": end_date - pd.DateOffset(years=2)
}

results = []

for period, start_date in periods.items():

    subset = data[
        (data[DATE_COL] >= start_date) &
        (data[DATE_COL] < end_date + pd.Timedelta(days=1))
    ]

    valid = subset["NewCustomer_Clean"].dropna()

    true_count = int((valid == True).sum())
    false_count = int((valid == False).sum())
    total = len(valid)

    results.append({
        "Period": period,
        "Total Records": len(subset),
        "True Count": true_count,
        "False Count": false_count,
        "Missing/Invalid": len(subset) - total,
        "True %": round(true_count / total * 100, 2)
                  if total else np.nan,
        "False %": round(false_count / total * 100, 2)
                   if total else np.nan
    })

summary = pd.DataFrame(results)

display(summary)

# ==========================================================
# 2. PLOTLY STACKED BAR CHART
# ==========================================================

fig = go.Figure()

fig.add_trace(go.Bar(
    name="True",
    x=summary["Period"],
    y=summary["True %"],
    marker_color="#2E86AB",
    text=summary["True %"].map(
        lambda x: f"{x:.1f}%" if pd.notna(x) else ""
    ),
    textposition="inside",
    hovertemplate="%{x}<br>True: %{y:.2f}%<extra></extra>"
))

fig.add_trace(go.Bar(
    name="False",
    x=summary["Period"],
    y=summary["False %"],
    marker_color="#E07A5F",
    text=summary["False %"].map(
        lambda x: f"{x:.1f}%" if pd.notna(x) else ""
    ),
    textposition="inside",
    hovertemplate="%{x}<br>False: %{y:.2f}%<extra></extra>"
))

fig.update_layout(
    title="New Customer Flag | True vs False",
    barmode="stack",
    yaxis_title="Percentage of Records (%)",
    xaxis_title="",
    yaxis=dict(range=[0, 100]),
    template="plotly_white",
    height=500,
    legend_title="NewCustomer",
    hovermode="x unified"
)

fig.show()

# ==========================================================
# 3. MONTHLY TREND - LAST 24 MONTHS
# ==========================================================

monthly = data[
    (data[DATE_COL] >= periods["Last 2 Years"]) &
    (data[DATE_COL] < end_date + pd.Timedelta(days=1))
].copy()

monthly["Month"] = (
    monthly[DATE_COL].dt.to_period("M").dt.to_timestamp()
)

monthly_summary = (
    monthly.groupby("Month")["NewCustomer_Clean"]
    .agg(
        Total=lambda x: x.notna().sum(),
        True_Count=lambda x: (x == True).sum(),
        False_Count=lambda x: (x == False).sum()
    )
    .reset_index()
)

monthly_summary["True %"] = np.where(
    monthly_summary["Total"] > 0,
    monthly_summary["True_Count"] /
    monthly_summary["Total"] * 100,
    np.nan
)

monthly_summary["False %"] = np.where(
    monthly_summary["Total"] > 0,
    monthly_summary["False_Count"] /
    monthly_summary["Total"] * 100,
    np.nan
)

print("\nMONTHLY NEW CUSTOMER TREND")
display(monthly_summary.round(2))

fig2 = go.Figure()

fig2.add_trace(go.Scatter(
    x=monthly_summary["Month"],
    y=monthly_summary["True %"],
    mode="lines+markers",
    name="NewCustomer = True",
    line=dict(color="#2E86AB", width=3),
    marker=dict(size=7),
    customdata=monthly_summary[["True_Count", "Total"]],
    hovertemplate=(
        "%{x|%B %Y}<br>"
        "True: %{y:.2f}%<br>"
        "True Records: %{customdata[0]:,}<br>"
        "Valid Records: %{customdata[1]:,}"
        "<extra></extra>"
    )
))

fig2.update_layout(
    title="Monthly Trend | New Customers (Last 2 Years)",
    xaxis_title="Month",
    yaxis_title="NewCustomer = True (%)",
    yaxis=dict(range=[0, 100]),
    template="plotly_white",
    height=500,
    hovermode="x unified"
)

fig2.show()
