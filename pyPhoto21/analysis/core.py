import numpy as np
from skimage.transform import downscale_local_mean
import cv2 as cv

from pyPhoto21.analysis.process import Processor


# Analysis functionality shared by many features
class AnalysisCore:

    def __init__(self, meta):

        self.meta = meta  # Metadata object.

        # Filtering settings
        self.temporal_filter_options = [
            'None',         # 0
            'Gaussian',     # 1
            'Low Pass',     # 2
            'Binomial-8',   # 3
            'Binomial-6',   # 4
            'Binomial-4',   # 5
        ]

        # Baseline Correction settings
        self.baseline_correction_options = [
            'None',
            'Linear',
            # 'Exponential',
            'Polynomial-8th',
            'Quadratic',
            'Cubic'
        ]

        # Display frame cached by analysis submodules
        self.current_processed_frame = None
        self.show_processed_data = False

    def set_is_temporal_filter_enabled(self, v):
        self.meta.is_temporal_filer_enabled = v

    def get_is_temporal_filter_enabled(self):
        return self.meta.is_temporal_filer_enabled

    def get_temporal_filter_radius(self):
        return self.meta.temporal_filter_radius

    def set_temporal_filter_radius(self, v):
        self.meta.temporal_filter_radius = v

    def get_temporal_filter_options(self):
        return self.temporal_filter_options

    def get_temporal_filter_index(self):
        return self.meta.temporal_filter_type_index

    def set_temporal_filter_index(self, v, suppress_processing=False):
        self.meta.temporal_filter_type_index = v

    def set_is_spatial_filter_enabled(self, v):
        self.meta.is_spatial_filer_enabled = v

    def get_is_spatial_filter_enabled(self):
        return self.meta.is_spatial_filer_enabled

    def set_spatial_filter_sigma(self, v):
        self.meta.spatial_filter_sigma = v

    def get_spatial_filter_sigma(self):
        return self.meta.spatial_filter_sigma

    def get_baseline_correction_options(self):
        return self.baseline_correction_options

    def get_baseline_correction_type_index(self):
        return self.meta.baseline_correction_type_index

    def set_baseline_correction_type_index(self, v):
        self.meta.baseline_correction_type_index = v

    def get_processed_display_frame(self):
        return self.current_processed_frame

    def set_processed_display_frame(self, image):
        if type(image) != np.ndarray or len(image.shape) != 2:
            print("Not a valid processed display frame (core.py)!")
        self.current_processed_frame = image

    def set_show_processed_data(self, v):
        self.show_processed_data = v

    def get_show_processed_data(self):
        return self.show_processed_data

    @staticmethod
    def get_snr(data):
        """ Given a single trial, compute the SNR image for this trial
            The temporal axis is assumed to be the third from the end. """
        return np.mean(data, axis=-3) / np.std(data, axis=-3)

    def create_binned_data(self, data, binning_factor=2):
        """ binning utility function """
        if binning_factor < 2:
            return data
        return self.create_binned_data_ndim(data, (binning_factor, binning_factor))

    @staticmethod
    def create_binned_data_ndim(data, bin_shape):
        """ binning utility function: for N-D data"""
        return downscale_local_mean(data, bin_shape)

    def filter_spatial(self, frame, filter_type='Gaussian'):
        """ Spatial filtering: Gaussian """
        if self.get_is_spatial_filter_enabled():
            sigma_s = self.get_spatial_filter_sigma()
            if filter_type == 'Gaussian':
                filter_size = int(sigma_s * 3)
                if filter_size % 2 == 0:
                    filter_size += 1

                frame = cv.GaussianBlur(frame.astype(np.float32),
                                        (filter_size, filter_size),
                                        sigma_s)
        return frame

    def filter_spatial_4dim(self, frames, filter_type='Gaussian'):
        """ Spatial filtering: Gaussian, for 4-d array of 2-d images """
        if self.get_is_spatial_filter_enabled():
            sigma_s = self.get_spatial_filter_sigma()
            if filter_type == 'Gaussian':
                filter_size = int(sigma_s * 3)
                if filter_size % 2 == 0:
                    filter_size += 1
                for tr in range(frames.shape[0]):
                    for t in range(frames.shape[1]):
                        frames = cv.GaussianBlur(frames[tr, t, :, :].astype(np.float32),
                                                 (filter_size, filter_size),
                                                 sigma_s)
        return frames

    def set_baseline_skip_window(self, kind, index, value):
        if index == 0 or index == 1:
            self.meta.baseline_skip_window[index] = value

    def get_baseline_skip_window(self):
        return self.meta.baseline_skip_window

    @staticmethod
    def get_half_width(location, trace):
        """ Return TRACE's zeros on either side, if any, of location
            Includes linear interpolation """
        hw = trace[location] / 2
        if len(trace.shape) < 1 or trace.shape[0] < 2:
            return 0

        i_left = location
        while trace[i_left] > hw:
            i_left -= 1
            if i_left < 0:
                return None
        # linearly interpolate
        i_left -= trace[i_left + 1] / (trace[i_left + 1] + trace[i_left])

        i_right = location
        while trace[i_right] > hw:
            i_right += 1
            if i_right >= trace.shape[0]:
                return None
        # linearly interpolate
        i_right += trace[i_right - 1] / (trace[i_right - 1] + trace[i_right])

        if i_right - i_left <= 0:
            return None
        return i_right - i_left
