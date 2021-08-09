import numpy as np
from yellowbrick.cluster import KElbowVisualizer
from sklearn.metrics import silhouette_samples, silhouette_score
from sklearn.cluster import AgglomerativeClustering
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt


class ROI:

    def __init__(self, data):
        self.data = data  # AnalysisCore: core analysis code

        # initialize any more variables you need for cached data, default settings, etc
        self.cutoff = {
            'pixel': {
                'raw': None,
                'percentile': 1.0,
            },
            'cluster': {
                'raw': None,
                'percentile': 1.0,
            },
            'roi_snr': {
                'raw': None,
                'percentile': 1.0,
            },
            'roi_amplitude': {
                'raw': None,
                'percentile': 1.0,
            },
        }
        self.time_window = {
            'pre_stim': [0, -1],
            'stim': [0, -1],
        }
        self.k_clusters = 3
        self.clustering_type = None

        # Possible attributes (just suggestions) that may be handy?
        self.snr = None
        self.snr_cutoff = None
        self.clustered = None
        self.cluster_indices_by_snr = None

        self.current_display_frame = None  # this should be a numpy array of (h, w) dimensions before you activate cluster mode

    def enable_roi_identification(self, trial=None):

        data = self.data.get_acqui_images(trial=trial)  # get the acquired image data

        if data is None:
            return

        print(data.shape, "shape of data (trial, time, height, width)")
        print("enable_roi_identification not implemented")
        # compute the frame to display and store it to self.current_display_frame
        # then call:
        #   self.data.core.set_show_processed_data(True)
        #   self.data.core.set_processed_display_frame(self.current_display_frame)
        #   (the order doesn't matter)

        # Use the methods of FrameViewer (in frame.py, called only from the GUI class)
        # to manipulate display settings
        # and update the matplotlib plot by calling either:
        #  - update() if the data is the same size as previously, and the slider and
        #                   shaded regions don't need to be redrawn
        #  - update_new_image() otherwise to fully redraw
        # E.g. you will want to update the slider values this way with self.fv.update_num_frames
        # Feel free to enhance or add new methods as needed.
        # Add shaded regions with self.fv.add_shape:
        #   The points are a Nx2 numpy array of x,y
        #   coordinates representing a polygon path

    def disable_roi_identification(self):
        #   To turn off showing processed data:
        #      self.data.core.set_show_processed_data(True)
        pass  # do any other cleanup needed here

    def launch_cluster_score_plot(self, plot_type):
        if plot_type == 'silhouette':
            print("silhouette plot not implemented")
        elif plot_type == 'elbow':
            print("elbow plot not implemented")

        # we will need extra code in gui to launch this in a new window
        # with a matplotlib fig and a button to pull in the best k_clusters
        # We can do this later.

    def get_cutoff(self, kind, form):
        if kind in self.cutoff:
            if form in self.cutoff[kind]:
                return self.cutoff[kind][form]
        return None

    def set_cutoff(self, kind, form, value):
        print(self.cutoff, kind, form, value)
        if self.cutoff[kind][form] != value:
            pass
            # Set the 'partner' form as well.
            # e.g. if 'Raw' is updated, calculate
            # the 'Percentile' if possible and set that.
            # The GUI will then update both fields.
            # Maybe trigger an update event immediately?
            # Or if that's too performance-intensive, we can update only when
            # user is done filling out the form.
        print("Setting cutoff of", kind, form, "to:", value)
        self.cutoff[kind][form] = value

    # Validation and setting the partner field is handled by the
    # GUI, because there we have access to interval between points
    def get_time_window(self, kind):
        if kind in self.time_window:
            return self.time_window[kind]
        return None

    # Validation and setting the partner field is handled by the
    # GUI, because there we have access to interval between points
    def set_time_window(self, kind, index, value):
        if index == 0 or index == 1:
            self.time_window[kind][index] = value

    def get_k_clusters(self):
        return self.k_clusters

    def set_k_clusters(self, k):
        self.k_clusters = k

    #  put all data into an object (e.g. dict) and return it.
    #  it will be written to a compressed file (pickle)
    def dump_roi_data(self):
        print("dump_roi_data not implemented")
        filename = None
        data = {}
        return data, filename

    #  restore data from file to this object
    #  and any other ancillary tasks to put the user right
    #  back where they left off.
    #  The file_data is returned to you exactly as you stored it
    #  from dump_roi_data
    def load_roi_data(self, data_obj):
        print("load_roi_data not implemented")

    # get and set clustering_type (hierarchical, k-means)?

    # Code from my previous scripts (will likely need to be adapted)

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
