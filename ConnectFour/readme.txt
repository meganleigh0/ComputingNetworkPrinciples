
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from IPython.display import display

# Use your existing DataFrame
data = df.copy()

# Exact column names from your dataset
DATE_COL = "SDate"
NEW_COL = "NewCustomer"

# Clean data
data[DATE_COL] = pd.to_datetime(data[DATE_COL], errors="coerce")

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

# Use latest date available in your data
end_date = data[DATE_COL].max().normalize()

periods = {
    "Last 7 Days": end_date - pd.Timedelta(days=7),
    "Last 1 Month": end_date - pd.DateOffset(months=1),
    "Last 3 Months": end_date - pd.DateOffset(months=3),
    "Last 6 Months": end_date - pd.DateOffset(months=6),
    "Last 12 Months": end_date - pd.DateOffset(months=12)
}

# Calculate percentages
results = []

for name, start_date in periods.items():

    subset = data[
        (data[DATE_COL] > start_date) &
        (data[DATE_COL] < end_date + pd.Timedelta(days=1))
    ]

    valid = subset["NewCustomer_Clean"].dropna()

    true_count = (valid == True).sum()
    false_count = (valid == False).sum()
    total = len(valid)

    results.append({
        "Period": name,
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

print("NEW CUSTOMER FIELD ANALYSIS")
print("Latest date in dataset:", end_date.date())
display(summary)

# Visualization 1: True vs False by time period
ax = (
    summary.set_index("Period")[["True %", "False %"]]
    .plot(
        kind="bar",
        stacked=True,
        figsize=(12, 6),
        color=["steelblue", "coral"]
    )
)

plt.title("NewCustomer: True vs False by Time Period")
plt.ylabel("Percentage of Records (%)")
plt.xlabel("")
plt.ylim(0, 100)
plt.xticks(rotation=0)
plt.legend(title="NewCustomer")
plt.grid(axis="y", alpha=0.2)
plt.tight_layout()
plt.show()

# Visualization 2: Monthly trend
monthly = data.dropna(subset=["NewCustomer_Clean"]).copy()
monthly["Month"] = monthly[DATE_COL].dt.to_period("M")

monthly_summary = (
    monthly.groupby("Month")["NewCustomer_Clean"]
    .agg(
        Total="count",
        True_Count=lambda x: (x == True).sum(),
        False_Count=lambda x: (x == False).sum()
    )
)

monthly_summary["True %"] = (
    monthly_summary["True_Count"]
    / monthly_summary["Total"] * 100
)

monthly_summary["False %"] = (
    monthly_summary["False_Count"]
    / monthly_summary["Total"] * 100
)

print("\nMONTHLY NEW CUSTOMER TREND")
display(monthly_summary.tail(18))

monthly_summary["True %"].tail(18).plot(
    figsize=(12, 5),
    marker="o",
    linewidth=2
)

plt.title("Monthly Percentage of NewCustomer = True")
plt.xlabel("Month")
plt.ylabel("New Customer (%)")
plt.ylim(0, 100)
plt.grid(alpha=0.3)
plt.tight_layout()
plt.show()
