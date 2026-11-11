
import numpy as np
import pandas as pd
import plotly.express as px

# Change to "cluster" to analyze the original 7 groups
GROUP = "group5"

METRICS = {
    "Average project spend": "avg_project_sales",
    "Rooms per project": "avg_rooms_per_project",
    "Premium purchases": "premium_share",
    "Mattress purchases": "mattress_share",
    "Online purchases": "web_share",
    "Promotional purchases": "promo_rate",
    "Protection attachment": "protection_rate",
    "Repeat projects/year": "projects_per_year"
}

available = {
    label: col
    for label, col in METRICS.items()
    if col in df.columns
}

cols = list(available.values())

work = df[[GROUP] + cols].copy()
work[cols] = work[cols].apply(
    pd.to_numeric, errors="coerce"
)

# Mean of each behavior across customers
overall = work[cols].mean()

# Mean behavior within each customer segment
segment_means = work.groupby(GROUP)[cols].mean()

# 100 = overall customer average
index = segment_means.div(
    overall.replace(0, np.nan)
).mul(100)

index = index.rename(
    columns={v: k for k, v in available.items()}
)

fig = px.imshow(
    index,
    color_continuous_scale="RdBu",
    color_continuous_midpoint=100,
    aspect="auto",
    text_auto=".0f",
    labels={
        "x": "Customer Behavior",
        "y": "Customer Segment",
        "color": "Index"
    },
    title="Customer Behavior Index | 100 = Average"
)

fig.update_layout(
    template="plotly_white",
    height=550,
    xaxis_tickangle=-30
)

fig.show()
