
from sklearn.cluster import KMeans
from sklearn.metrics import (
    silhouette_score,
    davies_bouldin_score,
    adjusted_rand_score
)
import numpy as np
import pandas as pd
import plotly.express as px

rng = np.random.default_rng(SEED)

# Fixed sample for comparable K evaluations
idx = rng.choice(
    len(Xw),
    size=min(15000, len(Xw)),
    replace=False
)
X_sample = Xw[idx]

results = []

for k in range(3, 11):
    model1 = KMeans(
        n_clusters=k, n_init=10,
        random_state=42
    ).fit(X_sample)

    model2 = KMeans(
        n_clusters=k, n_init=10,
        random_state=123
    ).fit(X_sample)

    labels = model1.labels_

    results.append({
        "K": k,
        "Silhouette": silhouette_score(
            X_sample, labels,
            sample_size=min(3000, len(X_sample)),
            random_state=42
        ),
        "Davies-Bouldin": davies_bouldin_score(
            X_sample, labels
        ),
        "Stability_ARI": adjusted_rand_score(
            model1.labels_, model2.labels_
        ),
        "Smallest_Cluster_%":
            pd.Series(labels).value_counts(
                normalize=True
            ).min() * 100
    })

evaluation = pd.DataFrame(results)

print("CLUSTER VALIDATION")
display(evaluation.round(3))

fig = px.line(
    evaluation,
    x="K",
    y=["Silhouette", "Stability_ARI"],
    markers=True,
    title="Cluster Quality and Initialization Stability"
)
fig.update_layout(template="plotly_white")
fig.show()

