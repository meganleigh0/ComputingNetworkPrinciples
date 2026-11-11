Gardner White customer segmentation | Streamlit POC.

Run: streamlit run app.py
Demo uses approximate presentation figures and annotated illustrative behavior values.
Upload customer-level CSV(s) to calculate figures from actual data.
"""
from __future__ import annotations

import io
import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import streamlit as st

st.set_page_config(page_title="Customer Intelligence | POC", page_icon="🛋️", layout="wide", initial_sidebar_state="expanded")

NAVY = "#143B71"
INK = "#162B4C"
PALE = "#EAF1F8"
BLUE = "#337DAE"
GOLD = "#D6A14A"
SEGMENTS = ["Sleep System Investors", "Living Room Upgraders", "Multi-Room Furnishers", "Value Seekers", "Online Shoppers"]
COLORS = dict(zip(SEGMENTS, ["#174B84", "#5699CA", "#7D9FAF", "#C9944E", "#89B1C8"]))
SHORT_TO_SEGMENT = {"A": SEGMENTS[0], "B": SEGMENTS[1], "C": SEGMENTS[2], "D": SEGMENTS[3], "E": SEGMENTS[4]}
STORES = ["Fourteen Mile", "Hall Road", "Canton", "Novi", "Taylor", "Auburn Hills", "Southfield", "Saginaw", "Waterford", "Ann Arbor", "Howell", "Rochester", "Telegraph Taylor"]

st.markdown("""<style>
.block-container {padding-top: 1.5rem; padding-bottom: 3rem; max-width: 1450px;}
[data-testid="stSidebar"] {background:#f3f6fa;}
h1,h2,h3 {color:#17395e; letter-spacing:-.025em;}
.hero {padding: 21px 26px; border-radius: 17px; color:white; background:linear-gradient(115deg,#102e58,#215889); margin-bottom: 17px;}
.hero h1 {color:white; font-size:2rem; margin:0 0 5px 0;}
.hero p {color:#dcebf7; margin:0;}
.mini {font-size:.83rem;color:#54708b;}
.notice {padding:10px 15px;border:1px solid #cbd8e6;border-radius:10px;background:#f6f9fc;}
[data-testid="stMetric"] {background:#f6f9fc; border:1px solid #e4eaf1; border-radius:13px; padding:14px 17px;}
</style>""", unsafe_allow_html=True)

@st.cache_data
def demo_groups():
    return pd.DataFrame({"Segment": SEGMENTS, "Customer %": [17, 33, 17, 23, 9],
        "Sales %": [29, 33, 26, 8, 5], "Spend per trip": [3660, 2300, 3025, 780, 1175]})

@st.cache_data
def demo_behaviors():
    # Values from user's prior output screenshot; labels standardized to deck language.
    items = {
        "Spend per project": [166,104,137,35,53],
        "Rooms per project": [103,89,141,87,90],
        "Premium purchasing": [345,58,79,22,23],
        "Mattress share": [246,15,130,92,92],
        "Online share": [12,6,9,7,991],
        "Promotional activity": [511,8,22,6,34],
        "Protection attachment": [84,153,88,59,63],
        "Projects / year": [108,98,102,96,98],
    }
    return pd.DataFrame(items, index=SEGMENTS)

@st.cache_data
def demo_stores():
    # Slide 12 store counts and indexes: indices are chain-normalized (100=expected).
    rows = [
        ("Fourteen Mile",83857,87,91,113,104),
        ("Hall Road",76121,110,106,96,86),
        ("Canton",65969,104,109,100,87),
        ("Novi",48545,114,105,102,84),
        ("Taylor",48262,107,99,100,90),
        ("Auburn Hills",38360,92,100,94,115),
        ("Southfield",35098,86,75,117,121),
        ("Saginaw",34173,89,107,80,123),
        ("Waterford",28720,83,92,104,131),
        ("Ann Arbor",27570,100,100,96,95),
        ("Howell",27414,105,124,82,74),
        ("Rochester",24087,127,101,103,86),
        ("Telegraph Taylor",14433,78,81,101,180),
    ]
    return pd.DataFrame(rows, columns=["Store","Customer records",*SEGMENTS[:4]])

def read_uploaded_csv(files):
    if not files:
        return None
    frames = []
    for file in files:
        raw = pd.read_csv(file, low_memory=False, dtype={"customer_id": "string"})
        frames.append(raw)
    return pd.concat(frames, ignore_index=True)

def segment_label(x):
    if pd.isna(x):
        return np.nan
    s = str(x).strip()
    if s in SEGMENTS:
        return s
    if s in SHORT_TO_SEGMENT:
        return SHORT_TO_SEGMENT[s]
    for prefix, name in SHORT_TO_SEGMENT.items():
        if s.startswith(prefix + " "):
            return name
    # Existing group5 codes may appear as strings; groups are explicitly tagged in notebook.
    return s

def first_available(df, candidates):
    return next((c for c in candidates if c in df.columns), None)

METRICS = {
    "Spend per project": ["avg_project_sales"],
    "Rooms per project": ["avg_rooms_per_project"],
    "Premium purchasing": ["premium_share"],
    "Mattress share": ["mattress_share"],
    "Online share": ["web_share"],
    "Promotional activity": ["promo_rate"],
    "Protection attachment": ["protection_rate"],
    "Projects / year": ["projects_per_year"],
}

def real_metrics(df):
    group_col = first_available(df, ["group5", "group", "segment", "Segment"])
    if not group_col:
        return None, None, None, "Missing group5 (or group / segment) field. Upload output with assignments and feature columns."
    data = df.copy()
    data["Segment"] = data[group_col].map(segment_label)
    data = data.loc[data["Segment"].notna()].copy()
    if data.empty:
        return None, None, None, "No segment assignments were found."
    revenue_col = first_available(data, ["lifetime_net_sales", "net_sales", "sales"])
    spend_col = first_available(data, ["avg_project_sales"])
    if revenue_col:
        data[revenue_col] = pd.to_numeric(data[revenue_col], errors="coerce")
    if spend_col:
        data[spend_col] = pd.to_numeric(data[spend_col], errors="coerce")
    grp = data.groupby("Segment", dropna=False)
    n = grp.size()
    summary = pd.DataFrame({"Segment": n.index, "Customer %": n / len(data) * 100})
    summary["Sales %"] = np.nan
    if revenue_col and data[revenue_col].sum() > 0:
        rev = grp[revenue_col].sum()
        summary["Sales %"] = summary["Segment"].map((rev / data[revenue_col].sum() * 100).to_dict())
    summary["Spend per trip"] = np.nan
    if spend_col:
        summary["Spend per trip"] = summary["Segment"].map(grp[spend_col].mean().to_dict())
    summary["Customers"] = summary["Segment"].map(n.to_dict()).astype(int)
    idx = {}
    for title, choices in METRICS.items():
        col = first_available(data, choices)
        if col:
            v = pd.to_numeric(data[col], errors="coerce")
            avg = v.mean()
            if pd.notna(avg) and avg != 0:
                idx[title] = v.groupby(data["Segment"]).mean() / avg * 100
    behavior = pd.DataFrame(idx).reindex(summary["Segment"] if len(idx) else [])
    store_col = first_available(data, ["assigned_group", "store", "store_name", "Store"])
    stores = None
    if store_col:
        valid = data.loc[data[store_col].notna()].copy()
        if len(valid):
            table = pd.crosstab(valid[store_col].astype(str), valid["Segment"])
            expected = valid["Segment"].value_counts(normalize=True)
            norm = table.div(table.sum(axis=1), axis=0)
            indexes = norm.div(expected, axis=1).mul(100).replace([np.inf,-np.inf],np.nan)
            indexes.insert(0, "Customer records", table.sum(axis=1))
            stores = indexes.reset_index().rename(columns={store_col: "Store"})
    return summary, behavior, stores, None

with st.sidebar:
    st.markdown("### 🛋️ Customer Intelligence")
    st.caption("Gardner White · segmentation proof of concept")
    st.markdown("---")
    mode = st.radio("Data source", ["Presentation demo", "Upload customer CSV(s)"], label_visibility="visible")
    uploaded = None
    if mode == "Upload customer CSV(s)":
        uploaded = st.file_uploader("Upload feature CSVs", type="csv", accept_multiple_files=True,
            help="One or more customer-level feature CSVs containing group5 and purchase metrics. Can concatenate part1/part2/part3.")
        st.caption("For full metrics, upload the original customer feature exports AFTER group5 assignments have been attached. The assignment-only export does not contain purchase metrics.")
    st.markdown("---")
    st.caption("POC only · demo includes rounded presentation percentages and a photographed store table. Neither is live operational data.")

df_customer = None
if mode == "Upload customer CSV(s)" and uploaded:
    try:
        df_customer = read_uploaded_csv(uploaded)
        groups, behavior, stores, error = real_metrics(df_customer)
        if error:
            st.error(error)
            st.stop()
        is_demo = False
    except Exception as exc:
        st.error(f"Could not read uploaded CSVs: {exc}")
        st.stop()
else:
    groups, behavior, stores = demo_groups(), demo_behaviors(), demo_stores()
    is_demo = True

st.markdown("""<div class="hero"><h1>Customer Segmentation Intelligence</h1>
<p>Understand who shops, where customer behaviors differ, and which initiatives are worth testing.</p></div>""", unsafe_allow_html=True)
if is_demo:
    st.warning("DEMO MODE: Customer shares, spend figures, and store indices are transcribed from presentation screenshots. Behavior indices come from a separate notebook screenshot. Some values are rounded; no live customer data or validated financial forecast is loaded.", icon="⚠️")
else:
    st.success(f"LIVE CSV MODE: {len(df_customer):,} uploaded rows. Figures below are calculated from the uploaded data; check row grain, coverage and definitions before sharing externally.")

left, mid, right, last = st.columns(4)
with left:
    st.metric("Customer segments", f"{len(groups):,}")
with mid:
    if is_demo:
        st.metric("Customers in original notebook", "561K", help="Approximate customer rows from notebook screenshot; not validated unique customers.")
    else:
        st.metric("Customer rows", f"{len(df_customer):,}")
with right:
    if groups["Sales %"].notna().any():
        top = groups.sort_values("Sales %", ascending=False).iloc[0]
        st.metric("Largest sales contributor", top["Segment"], f"{top['Sales %']:.1f}% of sales", delta_color="off")
    else:
        st.metric("Revenue contribution", "Not loaded")
with last:
    if groups["Spend per trip"].notna().any():
        top_spend = groups.sort_values("Spend per trip", ascending=False).iloc[0]
        st.metric("Highest average project spend", f"${top_spend['Spend per trip']:,.0f}", top_spend["Segment"], delta_color="off")
    else:
        st.metric("Average project spend", "Not loaded")

st.markdown("")
t1,t2,t3,t4 = st.tabs(["📊 Executive overview", "🧬 Customer behaviors", "🏬 Store explorer", "🎯 Action & impact lab"])

with t1:
    st.subheader("Customer share vs. sales contribution")
    a,b = st.columns([1.7,1], gap="large")
    with a:
        melted = groups.melt(id_vars="Segment", value_vars=["Customer %", "Sales %"], var_name="Measure", value_name="Percent").dropna()
        fig = px.bar(melted, x="Segment", y="Percent", color="Measure", barmode="group", text_auto=".1f",
            color_discrete_map={"Customer %":NAVY,"Sales %":BLUE})
        fig.update_layout(template="plotly_white", height=430, margin=dict(l=10,r=10,t=25,b=75),
            xaxis_title="", yaxis_title="Share (%)", legend_title="", legend=dict(orientation="h", y=1.13))
        fig.update_xaxes(tickangle=-18)
        st.plotly_chart(fig, use_container_width=True)
    with b:
        st.markdown("#### Segment snapshot")
        opt = st.selectbox("Choose a group", groups["Segment"].tolist(), key="overview_segment")
        rec = groups.loc[groups["Segment"]==opt].iloc[0]
        st.metric("Customer share", f"{rec['Customer %']:.1f}%")
        if pd.notna(rec.get("Sales %")):
            st.metric("Sales contribution", f"{rec['Sales %']:.1f}%")
        if pd.notna(rec.get("Spend per trip")):
            st.metric("Average project spend", f"${rec['Spend per trip']:,.0f}")
        st.caption("Historical descriptive metrics, not incremental lift or predicted customer lifetime value.")
    st.markdown("**What to discuss:** Do the segments with the highest historical sales share represent the best opportunity for retention, expansion, or acquisition? That requires prospective testing, not just the chart above.")

with t2:
    st.subheader("Which behaviors define each segment?")
    if behavior is None or behavior.empty:
        st.info("No usable behavior features found in uploaded CSV. Include avg_project_sales, mattress_share, promo_rate, web_share, etc.")
    else:
        use_cols = st.multiselect("Behavior metrics", behavior.columns.tolist(), default=behavior.columns.tolist(), key="behavior_metrics")
        if use_cols:
            show = behavior[use_cols]
            cap = st.slider("Cap color intensity at index", 150, 600, 200, step=25,
                help="The printed number stays unchanged. Only the displayed color is capped.")
            z = show.clip(lower=0, upper=cap)
            fig = go.Figure(go.Heatmap(z=z.values, x=show.columns, y=show.index,
                colorscale=[[0,"#C48C5F"],[0.5,"#F6F8FB"],[1,"#287FB4"]], zmin=0, zmax=cap,
                text=np.round(show.values,0),texttemplate="%{text:.0f}",
                hovertemplate="Segment: %{y}<br>Behavior: %{x}<br>Index: %{text:.0f}<extra></extra>",
                colorbar=dict(title="Index")))
            fig.update_layout(template="plotly_white", height=450, margin=dict(l=10,r=10,t=20,b=70),
                xaxis=dict(tickangle=-25), yaxis=dict(autorange="reversed"))
            st.plotly_chart(fig, use_container_width=True)
            st.caption("100 = overall customer mean. Values like 991 can reflect a very low overall baseline; inspect raw numerator/denominator before interpreting an index as an opportunity.")
            with st.expander("Show underlying index table"):
                st.dataframe(show.round(1), use_container_width=True)
    st.info("Model caveat: These are primarily the same behaviors used to form the segments. Clear differences here describe the groups; they do not independently validate predictive value.")

with t3:
    st.subheader("Store-level mix & opportunity exploration")
    if stores is None or stores.empty:
        st.info("Upload customer-level records containing assigned_group or store to show store profiles. The demo uses slide indices.")
    else:
        available_segments = [s for s in SEGMENTS if s in stores.columns]
        if not available_segments:
            available_segments = [c for c in stores.columns if c not in ["Store", "Customer records"] and pd.api.types.is_numeric_dtype(stores[c])]
        a,b = st.columns([1,1])
        with a:
            chosen = st.selectbox("Select customer segment", available_segments)
        with b:
            minimum = st.slider("Minimum customer rows", min_value=0, max_value=100000, value=2000, step=1000)
        plotted = stores.loc[stores["Customer records"] >= minimum].copy()
        plotted = plotted.sort_values(chosen)
        if not plotted.empty:
            fig = px.bar(plotted, x=chosen, y="Store", orientation="h", color=chosen,
                color_continuous_scale=["#D8E5EE","#2C6FAD","#123C75"],
                hover_data=["Customer records"], text=chosen,
                title=f"{chosen} · segment representation by store")
            fig.add_vline(x=100, line_dash="dash", line_color="#D5A04D",
                annotation_text="Expected = 100", annotation_position="top right")
            fig.update_layout(template="plotly_white", height=max(400, 36*len(plotted)+80),
                coloraxis_showscale=False, xaxis_title="Representation index (100 = chain baseline)", yaxis_title="", margin=dict(l=10,r=10,t=55,b=15))
            st.plotly_chart(fig, use_container_width=True)
            sel_store = st.selectbox("Inspect store", plotted["Store"].tolist())
            store_row = plotted.loc[plotted["Store"]==sel_store].iloc[0]
            st.markdown(f"**{sel_store}** · {int(store_row['Customer records']):,} customer rows in the available store view")
            detail = pd.DataFrame({"Segment":available_segments,"Index":[store_row[c] for c in available_segments]})
            st.dataframe(detail.style.format({"Index":"{:.1f}"}), hide_index=True, use_container_width=True)
        else:
            st.info("No stores meet your minimum sample-size threshold.")
        st.caption("Index >100 means overrepresentation versus baseline, NOT higher sales conversion or proof of a merchandising gap. In demo mode, online shopper index is not in the photographed slide and is deliberately omitted.")

with t4:
    st.subheader("Prioritize a pilot, not just a segment")
    st.markdown("**Illustrative scenario calculator** — use real eligible volume, observed conversion, order margin and costs before making a budget decision.")
    x,y,z = st.columns(3)
    with x:
        eligible = st.number_input("Eligible shopping opportunities / year", min_value=0, value=10000, step=500)
        baseline = st.number_input("Current conversion (%)", min_value=0.0, max_value=100.0, value=10.0, step=0.5)
    with y:
        lift = st.number_input("Assumed absolute conversion lift (percentage points)", min_value=0.0, max_value=100.0, value=1.0, step=0.25)
        order_value = st.number_input("Incremental net sales per conversion ($)", min_value=0.0, value=2000.0, step=100.0)
    with z:
        margin = st.number_input("Incremental gross margin (%)", min_value=0.0, max_value=100.0, value=35.0, step=1.0)
        cost = st.number_input("Annual incremental pilot cost ($)", min_value=0.0, value=25000.0, step=2500.0)
    increment_orders = eligible * min(lift / 100.0, max(0,1-baseline/100))
    sales = increment_orders * order_value
    profit = sales * margin / 100 - cost
    k1,k2,k3 = st.columns(3)
    k1.metric("Extra conversions (assumed)", f"{increment_orders:,.0f}")
    k2.metric("Incremental sales (assumed)", f"${sales:,.0f}")
    k3.metric("Gross profit less pilot cost", f"${profit:,.0f}")
    st.caption("Scenario only; NOT an estimate derived from clustering. Assumes opportunities are independent, lift is causal and incremental, and gross margin applies to new sales. Does not model cannibalization, returns, labor or fixed-cost impacts.")
    st.markdown("#### Suggested pilot scorecard")
    scorecard = pd.DataFrame([
        ["Premium sleep assortment + coaching", "Selected matched stores", "Premium sleep conversion; gross profit / eligible customer", "Matched-store or randomized pilot"],
        ["Living-room cross-sell follow-up", "Recent sofa purchasers", "Second-room purchase within 90/180 days", "Customer-level holdout"],
        ["Online-to-store experience", "Online-first shoppers", "Incremental conversion and contribution margin", "A/B test"],
    ],columns=["Pilot","Population","Primary KPI","Evaluation design"])
    st.dataframe(scorecard, hide_index=True, use_container_width=True)
    st.markdown("#### Decisions and validation questions")
    st.markdown("- Confirm why seven K-Means clusters were chosen, then manually merged into five business groups.\n- Test whether merging obscures important distinctions in promotion response and buying behavior.\n- Verify feature missingness, store assignment methods and whether customer records are unique.\n- Assess stability on fresh cohorts and holdout future purchases rather than only the training features.\n- Use matched stores or randomized campaign holdouts before claiming an initiative caused increased sales.")

st.divider()
st.caption("Internal proof of concept · prepared from photographed presentation/notebook evidence. Demo data is approximate and not intended for production decision-making.")
