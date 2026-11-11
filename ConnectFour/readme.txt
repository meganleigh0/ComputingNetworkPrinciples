"""Gardner White | Customer Intelligence. Run: python3 -m streamlit run app.py"""
from __future__ import annotations
import io
from pathlib import Path
import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import streamlit as st

st.set_page_config(page_title="Customer Intelligence | Gardner White", page_icon="◈", layout="wide")
NAVY="#12345B"; BLUE="#2464A4"; LIGHT="#EAF2F9"
GROUPS={4:"Sleep System Investors",6:"Sleep System Investors",
        1:"Living Room Upgraders",3:"Multi-Room Furnishers",
        2:"Value Seekers",7:"Value Seekers",5:"Online Shoppers"}
ORDER=["Sleep System Investors","Living Room Upgraders","Multi-Room Furnishers","Value Seekers","Online Shoppers"]
PALETTE=dict(zip(ORDER,["#163F78","#4D83BA","#79A6C4","#CF9856","#7B8DA5"]))
BLOCKS={
    "spend":["log_avg_project_sales"],
    "breadth":["log_projects_per_year","avg_rooms_per_project","piece_types_per_room"],
    "room_mix":["living_share","dining_share","bedroom_share","mattress_share"],
    "price":["tier_score","premium_share","good_share"],
    "value":["asis_share","promo_rate"],
    "channel":["web_share"],
    "mattress":["paid_adj_base_rate","protector_rate"]
}
FEATURES=[c for cols in BLOCKS.values() for c in cols]
PROFILE=["lifetime_net_sales","avg_project_sales","n_projects","projects_per_year","tenure_years",
         "avg_rooms_per_project","piece_types_per_room","living_share","dining_share",
         "bedroom_share","mattress_share","tier_score","premium_share","good_share",
         "asis_share","promo_rate","web_share","paid_adj_base_rate","protector_rate",
         "pickup_rate","haul_away_rate","special_order_rate","return_rate"]
st.markdown("""<style>
.block-container{max-width:1500px;padding-top:1.5rem}
[data-testid="stSidebar"]{background:#f0f4f9}
h1,h2,h3{color:#12345B}
[data-testid="stMetric"]{background:#f4f8fc;border:1px solid #e0e9f2;padding:14px;border-radius:12px}
.hero{background:linear-gradient(110deg,#102e54,#245e93);color:white;border-radius:16px;padding:23px 28px;margin-bottom:20px}
.hero h1{color:white;margin:0}.hero p{margin:7px 0 0;color:#dcebf9}
</style>""",unsafe_allow_html=True)
st.markdown('<div class="hero"><h1>Customer Intelligence</h1><p>Behavioral segmentation · Store performance · Commercial opportunities</p></div>',unsafe_allow_html=True)

@st.cache_data(show_spinner=False)
def read_csv(raw:bytes):
    return pd.read_csv(io.BytesIO(raw),low_memory=False,dtype={"customer_id":"string"},na_values=["NULL","null"])

def get_source():
    uploaded=st.sidebar.file_uploader("Customer feature files (CSV)",type="csv",accept_multiple_files=True,
                                      help="Upload persona_features_part1.csv, part2.csv, part3.csv, or a combined customer-level file.")
    local=sorted(Path("data").glob("persona_features_part*.csv"))
    if not uploaded and not local:
        return None,""
    if uploaded:
        frames=[read_csv(f.getvalue()) for f in uploaded]
        label=", ".join(f.name for f in uploaded)
    else:
        frames=[read_csv(f.read_bytes()) for f in local]
        label=", ".join(f.name for f in local)
    df=pd.concat(frames,ignore_index=True)
    df.columns=df.columns.str.strip()
    for col in ["persona5","price_band","family","assigned_group","assign_method"]:
        if col in df:
            df[col]=df[col].astype("string").str.strip().replace("",pd.NA)
    return df,label

df,source=get_source()
if df is None:
    st.info("Load the customer feature exports to explore the segmentation. Place `persona_features_part1.csv`, `persona_features_part2.csv`, and `persona_features_part3.csv` in a `data/` folder beside this file, or upload them from the sidebar.")
    st.markdown("#### Data model")
    st.write("Customer-level feature exports drive the segmentation. The model uses seven equally weighted behavior blocks: spend, breadth, room mix, price, value, channel, and mattress attachment. Demographic and store attributes are used only for profiling.")
    st.code("python3 -m pip install streamlit pandas numpy plotly scikit-learn\npython3 -m streamlit run app.py",language="bash")
    st.stop()

if "customer_id" in df:
    df["customer_id"]=df["customer_id"].astype("string")
    duplicates=int(df["customer_id"].duplicated().sum())
else:
    duplicates=0
for col in PROFILE+["avg_project_sales","projects_per_year","n_projects"]:
    if col in df:
        df[col]=pd.to_numeric(df[col],errors="coerce")
for col in ["web_share","return_rate"]:
    if col in df: df[col]=df[col].clip(0,1)
if "protection_rate" in df:
    df=df.loc[df["protection_rate"].notna()].copy()
if "customer_id" in df and duplicates:
    st.warning(f"{duplicates:,} duplicate customer IDs detected. Confirm whether these are multiple records per customer before interpreting customer-level metrics.")
st.sidebar.caption(f"Source: {source}")
st.sidebar.caption(f"{len(df):,} customer records · {len(df.columns)} fields")

def prepare_features(frame):
    feat=pd.DataFrame(index=frame.index)
    feat["log_avg_project_sales"]=np.log1p(pd.to_numeric(frame["avg_project_sales"],errors="coerce").clip(lower=0))
    feat["log_projects_per_year"]=np.log1p(pd.to_numeric(frame["projects_per_year"],errors="coerce").clip(lower=0))
    for c in FEATURES:
        if c not in feat:
            feat[c]=pd.to_numeric(frame[c],errors="coerce")
    for c in ["premium_share","good_share","asis_share","promo_rate","paid_adj_base_rate","protector_rate","web_share"]:
        feat[c]=feat[c].fillna(0)
    for c in ["piece_types_per_room","tier_score"]:
        feat[c]=feat[c].fillna(feat[c].median())
    for c in FEATURES:
        feat[c]=feat[c].replace([np.inf,-np.inf],np.nan).fillna(feat[c].median())
        if feat[c].isna().any(): feat[c]=feat[c].fillna(0)
    for c in ["log_avg_project_sales","log_projects_per_year","avg_rooms_per_project","piece_types_per_room"]:
        lo,hi=feat[c].quantile([.01,.99])
        feat[c]=feat[c].clip(lo,hi)
    from sklearn.preprocessing import StandardScaler
    scaler=StandardScaler()
    z=np.clip(scaler.fit_transform(feat[FEATURES]),-4,4)
    weights=np.array([1/np.sqrt(len(cols)) for cols in BLOCKS.values() for _ in cols])
    return z*weights,scaler,weights

@st.cache_data(show_spinner="Fitting customer segments…")
def fit_model(frame,k,seed):
    from sklearn.cluster import KMeans
    x,scaler,weights=prepare_features(frame)
    model=KMeans(n_clusters=k,n_init=20,random_state=seed)
    raw=model.fit_predict(x)
    order=pd.Series(raw).value_counts().index
    remap={int(old):i+1 for i,old in enumerate(order)}
    labels=pd.Series(raw,index=frame.index).map(remap)
    return labels,model,x

st.sidebar.divider()
st.sidebar.subheader("Segmentation")
has_cluster="cluster" in df
has_group="group5" in df
mode=st.sidebar.radio("Assignments",["Use existing assignments","Refit K-Means"] if (has_cluster or has_group) else ["Refit K-Means"])
if mode=="Refit K-Means":
    missing=[c for c in FEATURES if c not in df and c not in ("log_avg_project_sales","log_projects_per_year")]
    missing += [c for c in ["avg_project_sales","projects_per_year"] if c not in df]
    if missing:
        st.error("Cannot refit: missing required feature columns: "+", ".join(sorted(set(missing))))
        st.stop()
    k=st.sidebar.slider("Number of clusters",3,12,7)
    labels,model,x=fit_model(df,k,42)
    df["cluster"]=labels
    if k==7:
        st.sidebar.info("Refitted cluster numbers are size-ranked, not semantically aligned to the original seven groups. Do not apply the original five-group mapping without matching cluster profiles.")
        df["segment"]=df["cluster"].map(lambda n:f"Cluster {n}")
    else:
        df["segment"]=df["cluster"].map(lambda n:f"Cluster {n}")
else:
    if has_group:
        labels=df["group5"].astype("string")
        named={"A":"Sleep System Investors","B":"Living Room Upgraders","C":"Multi-Room Furnishers","D":"Value Seekers","E":"Online Shoppers"}
        df["segment"]=labels.map(lambda x: named.get(str(x).split(" ")[0],str(x)) if pd.notna(x) else pd.NA)
    elif has_cluster:
        df["segment"]=pd.to_numeric(df["cluster"],errors="coerce").map(GROUPS)
    else:
        st.error("No segment assignments found.")
        st.stop()
    if df["segment"].isna().any():
        st.warning("Some records have missing or unmapped group assignments.")

segments=[s for s in ORDER if s in df["segment"].dropna().unique()]+[s for s in df["segment"].dropna().unique() if s not in ORDER]
st.sidebar.divider()
selected=st.sidebar.multiselect("Customer segments",segments,default=segments)
if not selected: st.stop()
store_col=next((c for c in ["assigned_group","store","store_name","location"] if c in df),None)
if store_col:
    stores=sorted(df[store_col].dropna().astype(str).unique())
    selected_stores=st.sidebar.multiselect("Store / assignment",stores,default=stores)
else: selected_stores=[]
base=df[df["segment"].isin(selected)].copy()
if store_col and selected_stores: base=base[base[store_col].isin(selected_stores)]
if base.empty: st.warning("No records match the current filters.");st.stop()
st.caption(f"Customer records: {len(base):,} · Source: {source} · Segments reflect the selected assignment method.")
if mode=="Refit K-Means": st.warning("Refitted cluster labels are exploratory. Profile and validate the clusters before using business-facing names.")

def seg_summary(frame):
    g=frame.groupby("segment",dropna=False).agg(customers=("segment","size"))
    g["customer_share"]=g["customers"]/len(frame)*100
    if "lifetime_net_sales" in frame:
        g["sales"]=frame.groupby("segment")["lifetime_net_sales"].sum()
        denom=g["sales"].sum()
        g["sales_share"]=g["sales"]/denom*100 if denom else np.nan
        g["sales_per_customer"]=g["sales"]/g["customers"]
    if "avg_project_sales" in frame: g["avg_project_sales"]=frame.groupby("segment")["avg_project_sales"].mean()
    return g.reset_index()

summary=seg_summary(base)
tabs=st.tabs(["Executive overview","Segment DNA","Store intelligence","Customer journeys","Model diagnostics","Opportunity planner"])
with tabs[0]:
    a,b,c,d=st.columns(4)
    a.metric("Customers",f"{len(base):,}")
    b.metric("Segments",len(summary))
    if "lifetime_net_sales" in base:
        c.metric("Lifetime net sales",f"${base['lifetime_net_sales'].sum()/1e6:,.1f}M")
    if "avg_project_sales" in base:
        d.metric("Avg project sales",f"${base['avg_project_sales'].mean():,.0f}")
    st.subheader("Who buys vs. who drives revenue")
    if "sales_share" in summary:
        long=summary.melt(id_vars="segment",value_vars=["customer_share","sales_share"],var_name="Metric",value_name="Percent")
        long["Metric"]=long["Metric"].map({"customer_share":"Customer share","sales_share":"Lifetime sales share"})
        fig=px.bar(long,x="segment",y="Percent",color="Metric",barmode="group",text_auto=".1f",
                   color_discrete_map={"Customer share":NAVY,"Lifetime sales share":"#77A9D3"})
        fig.update_layout(xaxis_title="",yaxis_title="Share (%)",legend_title="",height=450)
        st.plotly_chart(fig,use_container_width=True)
        st.caption("Sales share uses the customer-level `lifetime_net_sales` field, not a current-period revenue measure.")
    else:
        st.dataframe(summary,use_container_width=True,hide_index=True)
    c1,c2=st.columns(2)
    with c1:
        fig=px.bar(summary.sort_values("customers"),x="customers",y="segment",orientation="h",text="customers",title="Segment size")
        fig.update_layout(yaxis_title="",xaxis_title="Customer records",height=370)
        st.plotly_chart(fig,use_container_width=True)
    with c2:
        if "sales_per_customer" in summary:
            fig=px.bar(summary.sort_values("sales_per_customer"),x="sales_per_customer",y="segment",orientation="h",
                       title="Lifetime sales per customer")
            fig.update_layout(yaxis_title="",xaxis_title="Lifetime net sales ($)",height=370)
            st.plotly_chart(fig,use_container_width=True)

with tabs[1]:
    st.subheader("How each customer segment shops")
    candidates=[c for c in PROFILE if c in base and c not in ["lifetime_net_sales","n_projects"]]
    default=[c for c in ["avg_project_sales","avg_rooms_per_project","premium_share","mattress_share","web_share","promo_rate","protector_rate","projects_per_year"] if c in candidates]
    metrics=st.multiselect("Behavioral measures",candidates,default=default)
    if metrics:
        means=base.groupby("segment")[metrics].mean()
        overall=base[metrics].mean().replace(0,np.nan)
        index=means.div(overall,axis=1)*100
        fig=px.imshow(index,text_auto=".0f",aspect="auto",color_continuous_scale="RdBu",
                      color_continuous_midpoint=100,title="Behavior index · 100 = selected population average")
        fig.update_layout(height=max(390,65*len(index)),xaxis_title="",yaxis_title="")
        st.plotly_chart(fig,use_container_width=True)
        st.caption("Indexes above 100 indicate above-average behavior. Extreme indexes can arise when the overall baseline is very small.")
        st.markdown("#### Underlying segment averages")
        st.dataframe(means.round(3),use_container_width=True)
    if "persona5" in base:
        st.subheader("Rule-based personas vs. behavior-based segments")
        cross=pd.crosstab(base["segment"],base["persona5"],normalize="index")*100
        st.plotly_chart(px.imshow(cross,text_auto=".0f",aspect="auto",color_continuous_scale="Blues",
                                  title="Composition of each behavior segment (%)"),use_container_width=True)

with tabs[2]:
    st.subheader("Store-level customer mix")
    if not store_col:
        st.info("No store assignment field was found. Expected `assigned_group`, `store`, or `store_name`.")
    else:
        usable=base.dropna(subset=[store_col,"segment"]).copy()
        counts=pd.crosstab(usable[store_col],usable["segment"])
        chain=usable["segment"].value_counts(normalize=True).reindex(counts.columns)
        store_share=counts.div(counts.sum(axis=1),axis=0)
        idx=store_share.div(chain,axis=1)*100
        min_n=st.slider("Minimum store customer records",100,5000,500,100)
        idx=idx.loc[counts.sum(axis=1)>=min_n]
        if len(idx):
            fig=px.imshow(idx.round(0),text_auto=True,aspect="auto",color_continuous_scale="RdBu",
                          color_continuous_midpoint=100,title="Store representation index · 100 = chain average")
            fig.update_layout(height=max(420,len(idx)*28+130),xaxis_title="Segment",yaxis_title="Store")
            st.plotly_chart(fig,use_container_width=True)
            st.caption("This is a descriptive customer-mix index, not a causal measure of store execution or performance.")
            st.download_button("Download store index",idx.round(1).to_csv().encode(),"store_segment_index.csv","text/csv")
        else: st.info("Lower the minimum customer threshold to see stores.")
        if "lifetime_net_sales" in usable:
            rev=usable.pivot_table(index=store_col,columns="segment",values="lifetime_net_sales",aggfunc="sum",fill_value=0)
            st.subheader("Lifetime sales by store and segment")
            st.plotly_chart(px.bar(rev.reset_index().melt(id_vars=store_col,var_name="Segment",value_name="Sales"),
                                   x=store_col,y="Sales",color="Segment",title="Segment contribution to customer lifetime sales"),
                            use_container_width=True)

with tabs[3]:
    st.subheader("Repeat purchase and customer maturity")
    if "n_projects" in base:
        repeat=base.assign(repeat=np.where(base["n_projects"].fillna(0)>=2,"2+ projects","1 project"))
        repeat=repeat[repeat["n_projects"].fillna(0)>=1]
        cross=pd.crosstab(repeat["segment"],repeat["repeat"],normalize="index")*100
        st.plotly_chart(px.bar(cross.reset_index().melt(id_vars="segment",var_name="Purchase history",value_name="Percent"),
                               x="segment",y="Percent",color="Purchase history",barmode="stack",
                               title="Repeat purchasing by segment"),use_container_width=True)
    if "tenure_years" in base:
        fig=px.box(base.dropna(subset=["tenure_years"]).sample(n=min(20000,base["tenure_years"].notna().sum()),random_state=42),
                   x="segment",y="tenure_years",points=False,title="Customer tenure distribution")
        st.plotly_chart(fig,use_container_width=True)
    st.caption("Repeat projects and tenure are historical descriptions, not retention rates or forecasts. Cohort tracking requires dated transactions.")

with tabs[4]:
    st.subheader("Segmentation quality and reproducibility")
    st.write("The baseline model applies log transforms to spend and frequency, imputes missing values, winsorizes selected features, standardizes and caps z-scores, then balances seven behavioral blocks before K-Means.")
    st.dataframe(pd.DataFrame([{"Block":name,"Features":", ".join(cols),"Feature count":len(cols),
                                "Weight per feature":round(1/np.sqrt(len(cols)),3)} for name,cols in BLOCKS.items()]),
                 hide_index=True,use_container_width=True)
    if "cluster" in base:
        st.plotly_chart(px.bar(base["cluster"].value_counts().sort_index().rename_axis("Cluster").reset_index(name="Customers"),
                               x="Cluster",y="Customers",title="Cluster sizes"),use_container_width=True)
    if mode=="Refit K-Means":
        from sklearn.metrics import silhouette_score,davies_bouldin_score
        st.caption("Diagnostics use a reproducible sample to avoid quadratic silhouette computation on the full dataset.")
        if st.button("Calculate current-fit diagnostics"):
            sample_n=min(5000,len(x))
            sample_idx=np.random.default_rng(42).choice(len(x),size=sample_n,replace=False)
            if len(np.unique(labels.iloc[sample_idx]))>1:
                c1,c2=st.columns(2)
                c1.metric("Silhouette",f"{silhouette_score(x[sample_idx],labels.iloc[sample_idx]):.3f}")
                c2.metric("Davies–Bouldin",f"{davies_bouldin_score(x[sample_idx],labels.iloc[sample_idx]):.3f}")
    st.markdown("**Validation priorities:** compare alternative k values, repeat fits on resamples, check segment stability over time, inspect sensitivity to feature weighting, and test whether the five-group merge preserves meaningful differences.")
    if "cluster" in df and "group5" in df:
        st.write("Original cluster → business group mapping")
        st.dataframe(pd.crosstab(df["cluster"],df["group5"]),use_container_width=True)

with tabs[5]:
    st.subheader("Commercial opportunity scenarios")
    st.caption("Planning scenarios are assumptions, not predictions. Use controlled pilots to estimate incremental lift.")
    choice=st.selectbox("Target segment",segments)
    subset=base[base["segment"]==choice]
    a,b,c=st.columns(3)
    reach=a.number_input("Customers targeted",min_value=0,max_value=int(len(subset)),value=min(1000,len(subset)),step=100)
    lift=b.slider("Incremental conversion (percentage points)",0.0,20.0,2.0,0.5)
    order_value=float(subset["avg_project_sales"].mean()) if "avg_project_sales" in subset and subset["avg_project_sales"].notna().any() else 1000.0
    ticket=c.number_input("Incremental order value ($)",min_value=0.0,value=round(order_value,0),step=50.0)
    a,b=st.columns(2)
    margin=a.slider("Contribution margin (%)",0,80,30)
    campaign=b.number_input("Pilot cost ($)",min_value=0.0,value=5000.0,step=500.0)
    incremental_orders=reach*lift/100
    revenue=incremental_orders*ticket
    profit=revenue*margin/100-campaign
    m1,m2,m3=st.columns(3)
    m1.metric("Incremental orders",f"{incremental_orders:,.1f}")
    m2.metric("Incremental revenue",f"${revenue:,.0f}")
    m3.metric("Net contribution",f"${profit:,.0f}")
    st.markdown("#### Pilot measurement plan")
    st.write("Randomly assign eligible customers to treatment and holdout groups. Track incremental conversion, net revenue, gross/contribution margin, and opt-outs over a defined attribution window. Stratify or adjust for store, customer tenure, and historical spend.")
    st.download_button("Download filtered customer summary",summary.to_csv(index=False).encode(),
                       "customer_segment_summary.csv","text/csv")

st.divider()
st.caption("Internal analytics · Customer-level records should remain within approved company systems. Avoid publishing identifiable customer exports.")
