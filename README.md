# ============================================================
# NEW CUSTOMER DATA QUALITY INVESTIGATION
# Gardner White | Sales Summary
# ============================================================

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from IPython.display import display

# ------------------------------------------------------------
# 1. PREPARE DATA
# ------------------------------------------------------------

df = DATA.copy()
df.columns = df.columns.str.strip()

print("=" * 65)
print("NEW CUSTOMER DATA QUALITY INVESTIGATION")
print("=" * 65)

print(f"\nTotal records: {len(df):,}")
print(f"Total columns: {len(df.columns)}")

print("\nAVAILABLE COLUMNS:")
print(df.columns.tolist())

# Identify relevant columns
date_col = "SDate"
customer_col = "CustomerID"
invoice_col = "InvoiceNumber"
flag_col = "NewCustomer"

# Validate required fields
required = [date_col, customer_col, invoice_col, flag_col]

missing = [c for c in required if c not in df.columns]

if missing:
    raise ValueError(f"Missing required columns: {missing}")

# Convert dates
df[date_col] = pd.to_datetime(
    df[date_col],
    errors="coerce"
)

# Normalize new customer flag
def normalize_flag(value):
    if pd.isna(value):
        return np.nan

    if isinstance(value, (bool, np.bool_)):
        return bool(value)

    value = str(value).strip().lower()

    if value in ["true", "1", "yes", "y"]:
        return True

    if value in ["false", "0", "no", "n"]:
        return False

    return np.nan

df["NewCustomer_Clean"] = (
    df[flag_col].apply(normalize_flag)
)

# ------------------------------------------------------------
# 2. BASIC DATA QUALITY CHECKS
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("1. BASIC DATA QUALITY")
print("=" * 65)

print("\nOriginal flag distribution:")
print(df[flag_col].value_counts(dropna=False))

print("\nNormalized flag distribution:")
print(
    df["NewCustomer_Clean"]
    .value_counts(dropna=False)
)

print("\nMissing values:")
print(
    df[
        [date_col, customer_col, invoice_col, flag_col]
    ].isna().sum()
)

print("\nDate range:")
print("Earliest:", df[date_col].min())
print("Latest:  ", df[date_col].max())

print("\nUnique customers:",
      df[customer_col].nunique())

print("Unique invoices:",
      df[invoice_col].nunique())


# ------------------------------------------------------------
# 3. DAILY NEW CUSTOMER TREND
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("2. DAILY NEW CUSTOMER TREND")
print("=" * 65)

df["SalesDate"] = df[date_col].dt.date

daily = (
    df.groupby("SalesDate")
    .agg(
        total_rows=(flag_col, "size"),

        new_customer_true=(
            "NewCustomer_Clean",
            lambda x: (x == True).sum()
        ),

        new_customer_false=(
            "NewCustomer_Clean",
            lambda x: (x == False).sum()
        ),

        missing_flag=(
            "NewCustomer_Clean",
            lambda x: x.isna().sum()
        )
    )
    .reset_index()
)

daily["pct_new"] = (
    daily["new_customer_true"]
    / daily["total_rows"] * 100
)

print("\nMost recent 30 days:")
display(daily.tail(30))

plt.figure(figsize=(14, 5))

plt.plot(
    daily["SalesDate"],
    daily["pct_new"],
    linewidth=2
)

plt.title("Daily Percentage Flagged as New Customer")
plt.xlabel("Sales Date")
plt.ylabel("New Customer (%)")
plt.xticks(rotation=45)
plt.grid(alpha=0.3)
plt.tight_layout()
plt.show()


# ------------------------------------------------------------
# 4. MONTHLY TREND
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("3. MONTHLY NEW CUSTOMER TREND")
print("=" * 65)

df["SalesMonth"] = (
    df[date_col]
    .dt.to_period("M")
)

monthly = (
    df.groupby("SalesMonth")
    .agg(
        total_rows=(flag_col, "size"),

        new_customer_true=(
            "NewCustomer_Clean",
            lambda x: (x == True).sum()
        ),

        new_customer_false=(
            "NewCustomer_Clean",
            lambda x: (x == False).sum()
        )
    )
    .reset_index()
)

monthly["pct_new"] = (
    monthly["new_customer_true"]
    / monthly["total_rows"] * 100
)

display(monthly.tail(18))

plt.figure(figsize=(14, 5))

plt.plot(
    monthly["SalesMonth"].astype(str),
    monthly["pct_new"],
    marker="o",
    linewidth=2
)

plt.title("Monthly New Customer Flag Trend")
plt.xlabel("Month")
plt.ylabel("New Customer (%)")
plt.xticks(rotation=60)
plt.grid(alpha=0.3)
plt.tight_layout()
plt.show()


# ------------------------------------------------------------
# 5. INVOICE CONSISTENCY
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("4. INVOICE CONSISTENCY CHECK")
print("=" * 65)

invoice_check = (
    df.dropna(subset=[invoice_col])
    .groupby(invoice_col)
    .agg(
        row_count=(flag_col, "size"),

        unique_flags=(
            "NewCustomer_Clean",
            lambda x: x.dropna().nunique()
        ),

        unique_customers=(
            customer_col,
            "nunique"
        )
    )
    .reset_index()
)

conflicting_invoices = invoice_check[
    invoice_check["unique_flags"] > 1
]

print(
    "\nInvoices with conflicting NewCustomer values:",
    len(conflicting_invoices)
)

print(
    "Invoices associated with multiple CustomerIDs:",
    (invoice_check["unique_customers"] > 1).sum()
)

display(conflicting_invoices.head(20))


# ------------------------------------------------------------
# 6. CUSTOMER CONSISTENCY
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("5. CUSTOMER CONSISTENCY CHECK")
print("=" * 65)

customer_check = (
    df.dropna(subset=[customer_col])
    .groupby(customer_col)
    .agg(
        total_rows=(flag_col, "size"),

        unique_invoices=(
            invoice_col,
            "nunique"
        ),

        unique_flags=(
            "NewCustomer_Clean",
            lambda x: x.dropna().nunique()
        ),

        first_date=(date_col, "min"),
        last_date=(date_col, "max")
    )
    .reset_index()
)

mixed_flag_customers = customer_check[
    customer_check["unique_flags"] > 1
]

print("\nCustomers with both True and False flags:")
print(len(mixed_flag_customers))

print(
    "\nNote: A customer having both values is not "
    "necessarily an error. A customer may be new "
    "on their first purchase and returning later."
)

display(mixed_flag_customers.head(20))


# ------------------------------------------------------------
# 7. FIRST OBSERVED PURCHASE VALIDATION
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("6. FIRST OBSERVED PURCHASE VALIDATION")
print("=" * 65)

# Use observed customer history from exported data.
# This is not necessarily complete purchase history.

valid = df.dropna(
    subset=[customer_col, invoice_col, date_col]
).copy()

# Build a customer/invoice-level table
invoice_level = (
    valid.groupby(
        [customer_col, invoice_col],
        dropna=False
    )
    .agg(
        invoice_date=(date_col, "min"),

        flag_true=(
            "NewCustomer_Clean",
            lambda x: (x == True).any()
        ),

        flag_false=(
            "NewCustomer_Clean",
            lambda x: (x == False).any()
        ),

        flag_missing=(
            "NewCustomer_Clean",
            lambda x: x.isna().all()
        ),

        line_count=(flag_col, "size")
    )
    .reset_index()
)

invoice_level = invoice_level.sort_values(
    [customer_col, "invoice_date", invoice_col]
)

# Identify earliest observed transaction date per customer
first_dates = (
    invoice_level.groupby(customer_col)["invoice_date"]
    .transform("min")
)

invoice_level["FirstObservedPurchase"] = (
    invoice_level["invoice_date"] == first_dates
)

# Only evaluate invoices with one unambiguous flag
invoice_level["ConsistentFlag"] = np.select(
    [
        invoice_level["flag_true"] &
        ~invoice_level["flag_false"],

        invoice_level["flag_false"] &
        ~invoice_level["flag_true"]
    ],
    [True, False],
    default=None
)

comparison = invoice_level[
    invoice_level["ConsistentFlag"].notna()
].copy()

comparison["FlagMismatch"] = (
    comparison["ConsistentFlag"]
    != comparison["FirstObservedPurchase"]
)

print("\nInvoices evaluated:", len(comparison))

print(
    "Disagreements with first observed purchase:",
    comparison["FlagMismatch"].sum()
)

print(
    "\nIMPORTANT: These are investigation candidates, "
    "not proven errors. The export may not contain "
    "complete customer purchase history, and the "
    "business definition of NewCustomer may differ."
)

display(
    comparison[
        comparison["FlagMismatch"]
    ].head(25)
)


# ------------------------------------------------------------
# 8. STORE / CHANNEL INVESTIGATION
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("7. STORE / CHANNEL INVESTIGATION")
print("=" * 65)

# Find likely store/channel fields
store_cols = [
    c for c in df.columns
    if any(
        term in c.lower()
        for term in ["store", "channel", "web", "online"]
    )
]

print("\nPotential store/channel columns:")
print(store_cols)

for col in store_cols:

    if df[col].nunique(dropna=False) <= 100:

        print(f"\nBreakdown for: {col}")

        channel_summary = (
            df.groupby(col, dropna=False)
            .agg(
                total_rows=(flag_col, "size"),

                new_true=(
                    "NewCustomer_Clean",
                    lambda x: (x == True).sum()
                ),

                new_false=(
                    "NewCustomer_Clean",
                    lambda x: (x == False).sum()
                )
            )
            .reset_index()
        )

        channel_summary["pct_new"] = (
            channel_summary["new_true"]
            / channel_summary["total_rows"] * 100
        )

        display(
            channel_summary.sort_values(
                "total_rows",
                ascending=False
            ).head(30)
        )


# ------------------------------------------------------------
# 9. RECENT PERIOD COMPARISON
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("8. RECENT PERIOD COMPARISON")
print("=" * 65)

latest_date = df[date_col].max()

if pd.notna(latest_date):

    periods = {
        "Last 7 days": 7,
        "Last 30 days": 30,
        "Last 90 days": 90
    }

    results = []

    for label, days in periods:

        subset = df[
            (df[date_col] >=
             latest_date - pd.Timedelta(days=days))
            &
            (df[date_col] <= latest_date)
        ]

        results.append({
            "Period": label,
            "Rows": len(subset),
            "True": (
                subset["NewCustomer_Clean"] == True
            ).sum(),
            "False": (
                subset["NewCustomer_Clean"] == False
            ).sum(),
            "Pct True": (
                (subset["NewCustomer_Clean"] == True).mean()
                * 100
                if len(subset) else np.nan
            )
        })

    display(pd.DataFrame(results))


# ------------------------------------------------------------
# 10. FINAL INVESTIGATION SUMMARY
# ------------------------------------------------------------

print("\n" + "=" * 65)
print("INVESTIGATION SUMMARY")
print("=" * 65)

print(f"Total sales rows: {len(df):,}")

print(
    "NewCustomer = True:",
    (df["NewCustomer_Clean"] == True).sum()
)

print(
    "NewCustomer = False:",
    (df["NewCustomer_Clean"] == False).sum()
)

print(
    "Missing/unrecognized flags:",
    df["NewCustomer_Clean"].isna().sum()
)

print(
    "Invoices with conflicting flags:",
    len(conflicting_invoices)
)

print(
    "Customers with both flags:",
    len(mixed_flag_customers)
)

print(
    "First-observed-purchase disagreements:",
    comparison["FlagMismatch"].sum()
)

print("\nNEXT STEPS:")
print("1. Confirm the definition of NewCustomer.")
print("2. Identify the e-commerce store/channel.")
print("3. Compare affected online invoices with source records.")
print("4. Review the SQL/DAX logic generating the flag.")
print("5. Determine whether the issue is source data or reporting.")

print("\nInvestigation complete.")
