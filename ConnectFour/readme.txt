
import numpy as np
import pandas as pd
import plotly.express as px

from sklearn.cluster import (
    KMeans, Birch, MiniBatchKMeans
)
from sklearn.mixture import GaussianMixture
from sklearn.metrics import (
    silhouette_score,
    davies_bouldin_score
)

rng = np.random.default_rng(42)
idx = rng.choice(
    len(Xw),
    size=min(15000, len(Xw)),
    replace=False
)
X_test = Xw[idx]

results = []

models = {
    "K-Means (7)": KMeans(
        n_clusters=7, n_init=10, random_state=42
    ),
    "K-Means (9)": KMeans(
        n_clusters=9, n_init=10, random_state=42
    ),
    "MiniBatch K-Means (9)": MiniBatchKMeans(
        n_clusters=9, n_init=10, random_state=42
    ),
    "BIRCH (9)": Birch(
        n_clusters=9
    ),
    "Gaussian Mixture (9)": GaussianMixture(
        n_components=9,
        covariance_type="diag",
        reg_covar=1e-5,
        random_state=42
    )
}

for name, model in models.items():
    try:
        labels = model.fit_predict(X_test)
        n_clusters = len(np.unique(labels))

        if n_clusters < 2:
            continue

        results.append({
            "Model": name,
            "Clusters": n_clusters,
            "Silhouette": silhouette_score(
                X_test, labels,
                sample_size=min(3000, len(X_test)),
                random_state=42
            ),
            "Davies-Bouldin":
                davies_bouldin_score(X_test, labels),
            "Smallest Cluster %":
                pd.Series(labels).value_counts(
                    normalize=True
                ).min() * 100
        })
    except Exception as e:
        print(f"{name} failed: {e}")

comparison = pd.DataFrame(results)
comparison = comparison.sort_values(
    "Silhouette", ascending=False
)

print("CUSTOMER SEGMENTATION: MODEL COMPARISON")
display(comparison.round(3))

fig = px.scatter(
    comparison,
    x="Davies-Bouldin",
    y="Silhouette",
    color="Model",
    size="Smallest Cluster %",
    hover_data=["Clusters"],
    title="Clustering Algorithm Comparison",
    template="plotly_white"
)
fig.show()
