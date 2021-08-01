

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

        self.current_display_frame = None  # this should be a numpy array of (h, w) dimensions before you activate cluster mode

    def enable_roi_identification(self, trial=None):

        data = self.data.get_acqui_images(trial=trial)  # get the acquired image data

        if data is None:
            return

        print(data.shape, "shape of data (trial, time, height, width)")
        print("enable_roi_identification not implemented")
        # compute the frame to display and store it to self.current_display_frame
        # then call self.data.core.set_processed_display_frame(self.current_display_frame)

        # Use the methods of FrameViewer (from frame.py) to manipulate display settings
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
        pass  # do any cleanup needed here

    def launch_silhouette_plot(self, fig):
        print("launch_silhouette_plot not implemented")
        # TO DO: The caller of this method
        # in gui.py will create that window and pass a matplotlib as `fig` for
        # this method to populate.

    def get_cutoff(self, kind, form):
        if kind in self.cutoff:
            if form in self.cutoff[kind]:
                return self.cutoff[kind][form]
        return None

    def set_cutoff(self, kind, form, value):
        if self.cutoff[kind][form] != value:
            pass
            # Maybe trigger an update event immediately?
            # Or if that's too performance-intensive, we can update only when
            # user is done filling out the form.
        print("Setting cutoff of", kind, form, "to:", value)
        self.cutoff[kind][form] = value

    def get_time_window(self, kind):
        if kind in self.time_window:
            return self.time_window[kind]
        return None

    def set_time_window(self, kind, value):
        self.time_window[kind] = value

    def get_k_clusters(self):
        return self.k_clusters

    def set_k_clusters(self, k):
        self.k_clusters = k

    #  put all data into an object (e.g. dict) and return it.
    #  it will be written to a compressed file (pickle)
    def dump_roi_data(self):
        print("dump_roi_data not implemented")
        data = {}
        return data

    #  restore data from file to this object
    #  and any other ancillary tasks to put the user right
    #  back where they left off.
    #  The file_data is returned to you exactly as you stored it
    #  from dump_roi_data
    def load_roi_data(self, file_data):
        print("load_roi_data not implemented")

    # get and set clustering_type (hierarchical, k-means)?

