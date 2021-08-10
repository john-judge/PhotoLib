import numpy as np

from skimage.transform import downscale_local_mean
from scipy.ndimage import gaussian_filter
import cv2 as cv
from sklearn.linear_model import LinearRegression
from numpy.polynomial import polynomial


# Analysis functionality shared by many features
class AnalysisCore:

    def __init__(self):

        # Filtering settings
        self.is_temporal_filer_enabled = False
        self.temporal_filter_radius = 25.0
        self.temporal_filter_type_index = 0
        self.temporal_filter_options = [
            'None',
            'Gaussian',
            'Low Pass',
            'Binomial-8',
            'Binomial-6',
            'Binomial-4',
        ]
        self.is_spatial_filer_enabled = False
        self.spatial_filter_sigma = 1.0

        # Baseline Correction settings
        self.baseline_correction_options = [
            'None',
            'Linear',
            # 'Exponential',
            'Polynomial-8th',
            'Quadratic',
            'Cubic'
        ]
        self.baseline_correction_type_index = 0
        self.skip_window_size = 100
        self.skip_window_start = 200

        # Time window selection
        self.time_window = [0, -1]

        # Display frame cached by analysis submodules
        self.current_processed_frame = None
        self.show_processed_data = False

    def set_is_temporal_filter_enabled(self, v):
        self.is_temporal_filer_enabled = v

    def get_is_temporal_filter_enabled(self):
        return self.is_temporal_filer_enabled

    def get_temporal_filter_radius(self):
        return self.temporal_filter_radius

    def set_temporal_filter_radius(self, v):
        self.temporal_filter_radius = v

    def get_temporal_filter_options(self):
        return self.temporal_filter_options

    def get_temporal_filter_index(self):
        return self.temporal_filter_type_index

    def set_temporal_filter_index(self, v):
        self.temporal_filter_type_index = v

    def set_is_spatial_filter_enabled(self, v):
        self.is_spatial_filer_enabled = v

    def get_is_spatial_filter_enabled(self):
        return self.is_spatial_filer_enabled

    def set_spatial_filter_sigma(self, v):
        self.spatial_filter_sigma = v

    def get_spatial_filter_sigma(self):
        return self.spatial_filter_sigma

    def get_baseline_correction_options(self):
        return self.baseline_correction_options

    def get_baseline_correction_type_index(self):
        return self.baseline_correction_type_index

    def set_baseline_correction_type_index(self, v):
        self.baseline_correction_type_index = v

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

    def get_skip_window_start(self):
        return self.skip_window_start

    def set_skip_window_start(self, value=0):
        self.skip_window_start = value

    def set_show_processed_data(self, v):
        self.show_processed_data = v

    def get_show_processed_data(self):
        return self.show_processed_data

    def get_skip_window_size(self):
        return self.skip_window_size

    def set_skip_window_size(self, value=0):
        self.skip_window_size = value

    @staticmethod
    def get_snr(data):
        """ Given a single trial, compute the SNR image for this trial
            The temporal axis is assumed to be the third from the end. """
        return np.mean(data, axis=-3) / np.std(data, axis=-3)

    @staticmethod
    def create_binned_data(data, binning_factor=2):
        """ binning utility function """
        if binning_factor < 2:
            return data
        return downscale_local_mean(data, (binning_factor, binning_factor))

    def filter_temporal(self, trace):
        """ Temporal filtering: 1-d binomial 8 filter (approx. Gaussian) """
        if self.get_is_temporal_filter_enabled():
            filter_type = self.get_temporal_filter_options()[self.get_temporal_filter_index()]
            sigma_t = self.get_temporal_filter_radius()
            trace = np.array(trace)

            if filter_type == 'Gaussian':
                trace = gaussian_filter(trace,
                                        sigma=sigma_t)
            elif filter_type == 'Low Pass':  # i.e. Moving Average
                trace = np.convolve(trace,
                                    np.ones(int(sigma_t)),
                                    'valid') / int(sigma_t)
            elif filter_type.startswith('Binomial-'):
                n = int(filter_type[-1])
                binom_coeffs = (np.poly1d([0.5, 0.5]) ** n).coeffs  # normalized binomial filter
                trace = np.convolve(trace,
                                    binom_coeffs,
                                    'valid')
            elif filter_type != 'None':
                print("T-Filter", filter_type, "not implemented.")
        return trace

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

    def baseline_correct_noise(self, trace, int_pts):
        """ subtract background drift off of single trace """
        n = len(trace)
        t = np.linspace(0, n * int_pts, num=n)

        fit_type = self.get_baseline_correction_options()[self.get_baseline_correction_type_index()]

        poly_powers = {
            'Quadratic': 2,
            'Cubic': 3,
            "Polynomial-8th": 8
        }
        if fit_type in ["Exponential", "Linear"]:
            t = t.reshape(-1, 1)
            min_val = None
            if fit_type == "Exponential":
                min_val = np.min(trace)
                if min_val <= 0:
                    trace += (-1 * min_val + 0.01)  # make all positive
                trace = np.log(trace)
            trace = trace.reshape(-1, 1)
            reg = LinearRegression().fit(t, trace).predict(t)
            if fit_type == "Exponential":
                reg = np.exp(reg)
                if min_val <= 0:
                    reg -= (-1 * min_val + 0.01)
        elif fit_type in poly_powers:
            power = poly_powers[fit_type]
            coeffs, stats = polynomial.polyfit(t, trace, power, full=True)
            reg = np.array(polynomial.polyval(t, coeffs))
        else:
            return trace

        trace = (trace.reshape(-1, 1) - reg.reshape(-1, 1)).reshape(-1)
        return trace

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
