import numpy as np

from skimage.transform import downscale_local_mean
from scipy.ndimage import gaussian_filter
import cv2 as cv


class SignalProcessor:

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
