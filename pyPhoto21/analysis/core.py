import numpy as np
from yellowbrick.cluster import KElbowVisualizer
from sklearn.metrics import silhouette_samples, silhouette_score
from sklearn.cluster import AgglomerativeClustering
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt

from skimage.transform import downscale_local_mean
from scipy.ndimage import gaussian_filter
import cv2 as cv


# Analysis functionality shared by many features
class AnalysisCore:

    def __init__(self):
        self.snr = None
        self.k_clusters = None
        self.snr_cutoff = None
        self.clustered = None
        self.cluster_indices_by_snr = None

        self.time_window = [0, -1]
        self.current_processed_frame = None

    def get_time_window(self):
        return self.time_window

    def set_time_window_start(self, v):
        self.time_window[0] = v

    def set_time_window_end(self, v):
        self.time_window[1] = v

    def get_processed_display_frame(self):
        return self.current_processed_frame

    def set_processed_display_frame(self, image):
        if type(image) != np.ndarray or len(image.shape) != 2:
            print("Not a valid processed display frame (core.py)!")
        self.current_processed_frame = image

    def get_snr(self, plot=False):
        """ Given a single trial, compute the SNR image for this trial """
        self.snr = np.mean(self.data, axis=2) / np.std(self.data, axis=2)

        if plot:
            plt.imshow(self.snr, cmap='jet', interpolation='nearest')
            plt.show()

        return self.snr

    def cluster_on_snr(self, k_clusters=3, snr_cutoff=0.7, plot=False):
        """ Perform 1-D clustering on SNR after masking out the pixels
        whose snr is below snr_cutoff (a percentile in range [0,1]) """

        self.k_clusters = k_clusters
        self.snr_cutoff = np.percentile(self.snr, snr_cutoff * 100)

        if self.snr is None:
            raise ValueError("No SNR data found.")

        mask = (self.snr >= self.snr_cutoff).astype(np.float)
        if plot:
            # masked image: reasonability check
            plt.imshow(self.snr * mask, cmap='jet', interpolation='nearest')
            plt.show()

        # +1 for the masked 0's
        snr_copy = self.snr
        snr_orig_shape = self.snr.shape
        km = KMeans(n_clusters=k_clusters + 1).fit(self.snr.reshape(-1, 1))

        self.clustered = np.array(km.labels_).reshape(self.snr.shape) + 1
        self.clustered = self.clustered.astype(np.int)

        self.snr.reshape(snr_orig_shape)

        if plot:
            plt.imshow(self.clustered * mask, cmap='viridis', interpolation='nearest')
            plt.show()

        return self.clustered

    def get_average_snr_by_cluster(self):
        """ Returns a list of average SNR values by cluster, where
        the float at index i is the average SNR for cluster i+1 """
        if self.k_clusters is None:
            raise ValueError("must call method cluster_on_snr() before getting average SNRs for clusters")
        return [np.average(self.snr[np.where(self.clustered == i)[0]])
                for i in range(1, self.k_clusters + 2)]

    def get_kth_cluster(self, k, plot=False):
        """
        Returns iterable of indexes of pixels in the kth cluster
        (k=0,...,k_clusters)
        """
        if self.k_clusters is None:
            raise ValueError("must call method cluster_on_snr() before getting kth cluster")
        if k > self.k_clusters:
            raise ValueError("k is greater than number of clusters")

        # sort clusters by SNR (which can differ from cluster label)
        if self.cluster_indices_by_snr is None:
            # SNR by cluster
            avg_snr_by_cluster = self.get_average_snr_by_cluster()
            self.cluster_indices_by_snr = np.argsort(np.array(avg_snr_by_cluster)) + 1

        k_selection = self.cluster_indices_by_snr[-1 - k]

        mask = (self.snr >= self.snr_cutoff).astype(np.float)
        # Select the pixels in this SNR cluster, above SNR cutoff

        arg_selection = np.stack(np.where(self.clustered * mask == k_selection))

        if plot:
            for i in range(arg_selection.shape[1]):
                x_max = arg_selection[0][i]
                y_max = arg_selection[1][i]

                mask[x_max, y_max] *= 3  # highlight
            plt.imshow(self.clustered * mask, cmap='jet', interpolation='nearest')
            plt.show()

        return arg_selection

    def get_silhouette_score(self, plot_elbow=True):
        """ Return silhouette score and plot Elbow plot for this K-means clustering """
        raise NotImplementedError
        print("Silhouette score:", silhouette_score(features, label))

        # Instantiate a scikit-learn K-Means model
        model = KMeans(random_state=0)

        # Instantiate the KElbowVisualizer with the number of clusters and the metric
        visualizer = KElbowVisualizer(model, k=(2, 6), metric='silhouette', timings=False)

        # Fit the data and visualize
        visualizer.fit(features)
        visualizer.poof()

    @staticmethod
    def create_binned_data(data, binning_factor=2):
        """ binning utility function """
        if binning_factor < 2:
            return data
        return downscale_local_mean(data, (binning_factor, binning_factor))

    @staticmethod
    def filter_temporal(self, meta, raw_data, sigma_t=1.0):
        """ Temporal filtering: 1-d binomial 8 filter (approx. Gaussian) """
        filtered_data = np.zeros(raw_data.shape)
        for i in range(raw_data.shape[0]):
            for jh in range(raw_data.shape[2]):
                for jw in range(raw_data.shape[3]):
                    filtered_data[i, :, jh, jw] = gaussian_filter(raw_data[i, :, jh, jw],
                                                                  sigma=sigma_t)
        return filtered_data

    @staticmethod
    def filter_spatial(self, raw_data, sigma_s=1.0):
        """ Spatial filtering: Gaussian """
        filtered_data = np.zeros(raw_data.shape)
        raw_data = raw_data
        filter_size = int(sigma_s * 3.5)
        for i in range(raw_data.shape[0]):
            for t in range(raw_data.shape[1]):
                filtered_data[i, t, :, :] = cv.GaussianBlur(raw_data[i, t, :, :].astype(np.float32),
                                                            (filter_size,filter_size),
                                                            sigma_s)
        return filtered_data
