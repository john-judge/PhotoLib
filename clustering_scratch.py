import matplotlib.pyplot as plt
   from kneed import KneeLocator
   from sklearn.datasets import make_blobs
   from sklearn.cluster import KMeans
   from sklearn.metrics import silhouette_score
   from sklearn.preprocessing import StandardScaler

#make test data set
features, true_labels = make_blobs(
   n_samples=200,
   centers=3,
   cluster_std=2.75,
   random_state=42
   )

print(test)