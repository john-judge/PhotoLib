import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore
from pyPhoto21.viewers.trace import Trace


class Data:

    def __init__(self, hardware):
        self.hardware = hardware
        self.num_trials = 5
        self.int_trials = 10  # ms
        self.num_records = 1
        self.int_records = 15  # seconds

        self.current_trial_index = 0

        self.num_fp_pts = None  # will default to 4 unless loaded from file

        self.file_metadata = {}

        # Little Dave reference data
        self.display_widths = [2048, 2048, 1024, 1024, 1024, 1024, 1024, 1024]
        self.display_heights = [1024, 100, 320, 160, 160, 80, 60, 40]
        self.display_camera_programs = ["200 Hz   2048x1024",
                                        "2000 Hz  2048x100",
                                        "1000 Hz  1024x320",
                                        "2000 Hz  1024x160",
                                        "2000 Hz  1024x160",
                                        "4000 Hz  1024x80",
                                        "5000 Hz  1024x60",
                                        "7500 Hz  1024x40"]

        # Management and Automation
        self.schedule_rli_flag = False  # TO DO: include take RLI and division
        self.auto_save_data = False

        # Analysis
        self.core = AnalysisCore()

        # Memory
        self.rli_images = None
        self.acqui_images = None
        self.fp_data = None
        self.livefeed_frame = None

        # Settings
        self.is_loaded_from_file = False
        self.is_live_feed_enabled = False
        self.display_value_option_index = 0

        self.auto_save_enabled = True
        self.schedule_rli_enabled = False
        self.is_rli_division_enabled = True
        self.is_data_inverse_enabled = True

        self.sync_defaults_into_hardware()

        # Time Window cropping
        self.crop_window = [0, -1]

    def set_crop_window(self, v, index=None):
        if index is None:
            self.crop_window = v
        elif index == 0 or index == 1:
            self.crop_window[index] = v

    def get_crop_window(self, index=None):
        if index is None:
            return self.crop_window
        return self.crop_window[index]

    # The purpose of this linspace is to
    # allow us to remove trace points without
    # losing track of the absolute frame number w.r.t
    # the stim time, etc
    def get_cropped_linspace(self):
        start_frames, end_frames = self.get_crop_window()
        if end_frames < 0:
            end_frames = self.get_num_pts()
        int_pts = self.get_int_pts()
        return np.linspace(start_frames * int_pts,
                           end_frames * int_pts,
                           end_frames - start_frames)

    def sync_defaults_into_hardware(self):
        if self.get_is_loaded_from_file():
            print("Settings loaded from data file will be overwritten with currnet hardware settings.")
            self.set_is_loaded_from_file(False)

        # Settings that don't matter to File
        self.set_camera_program(7, force_resize=True)
        self.set_num_dark_rli(280, force_resize=True)
        self.set_num_light_rli(200, force_resize=True)

        self.hardware.set_num_pulses(value=1,
                                     channel=1)
        self.hardware.set_num_pulses(value=1,
                                     channel=2)

        self.hardware.set_int_pulses(value=15,
                                     channel=1)
        self.hardware.set_int_pulses(value=15,
                                     channel=2)

        self.hardware.set_num_bursts(value=1,
                                     channel=1)
        self.hardware.set_num_bursts(value=1,
                                     channel=2)

        self.hardware.set_int_bursts(value=15,
                                     channel=1)
        self.hardware.set_int_bursts(value=15,
                                     channel=2)

        self.hardware.set_acqui_onset(acqui_onset=0)

        self.hardware.set_stim_onset(value=0,
                                     channel=1)
        self.hardware.set_stim_onset(value=0,
                                     channel=2)
        self.hardware.set_stim_duration(value=1,
                                        channel=1)
        self.hardware.set_stim_duration(value=1,
                                        channel=2)

    # We allocate twice the memory since C++ needs room for CDS subtraction
    def allocate_image_memory(self):
        w = self.get_display_width()
        h = self.get_display_height()
        self.rli_images = np.zeros((self.get_num_trials(),
                                    2,
                                    self.get_num_rli_pts(),
                                    h,
                                    w,),
                                   dtype=np.uint16)
        self.acqui_images = np.zeros((self.get_num_trials(),
                                      2,  # extra mem for C++ reassembly
                                      self.get_num_pts(),
                                      h,
                                      w),
                                     dtype=np.uint16)
        self.fp_data = np.zeros((self.get_num_trials(),
                                 self.get_num_pts(),
                                 self.get_num_fp()),
                                dtype=np.int16)

    def set_camera_program(self, program, force_resize=False):
        curr_program = self.hardware.get_camera_program()
        if force_resize or curr_program != program:
            self.hardware.set_camera_program(program=program)
            self.resize_image_memory()

    def get_camera_program(self):
        if self.get_is_loaded_from_file():
            return self.file_metadata['camera_program']
        return self.hardware.get_camera_program()

    def resize_image_memory(self):
        w = self.get_display_width()
        h = self.get_display_height()
        if self.rli_images is not None and self.acqui_images is not None:

            self.rli_images = np.resize(self.rli_images, (self.get_num_trials(),
                                                          2,
                                                          self.get_num_rli_pts(),
                                                          h,
                                                          w))
            self.acqui_images = np.resize(self.acqui_images, (self.get_num_trials(),
                                                              2,
                                                              self.get_num_pts(),
                                                              h,
                                                              w))
            self.fp_data = np.resize(self.fp_data,
                                     (self.get_num_trials(),
                                      self.get_num_fp(),
                                      self.get_num_pts()))

        else:
            self.allocate_image_memory()

    # Based on system state, create/get the frame that should be displayed.
    # index can be an integer or a list of [start:end] over which to average
    def get_display_frame(self, index=None, get_rli=False, binning=1, raw_override=False):
        if self.get_is_livefeed_enabled() and not raw_override:
            return self.get_livefeed_frame()[0, :, :]
        if self.core.get_show_processed_data() and not raw_override:
            return self.core.get_processed_display_frame()
        images = None
        if get_rli:
            images = self.get_rli_images()
        else:
            images = self.get_acqui_images()
        if images is None:
            return None
        if len(images.shape) != 3:
            print("Issue in data.py: get display frame image shape:", images.shape)
            return None

        ret_frame = None
        if type(index) == int and (index < images.shape[0]) and index >= 0:
            ret_frame = images[index, :, :]
        elif type(index) == list and len(index) == 2:
            ret_frame = np.average(images[index[0]:index[1], :, :], axis=0)
        else:
            ret_frame = np.average(images, axis=0)

        # RLI division
        if self.get_is_rli_division_enabled() and not get_rli:
            rli = self.calculate_rli()
            if rli is not None and rli.shape == ret_frame.shape:
                ret_frame = ret_frame.astype(np.float32) / rli
                ret_frame = ret_frame.astype(np.int32)
                ret_frame = np.nan_to_num(ret_frame)
                min_val = np.min(ret_frame)
                if min_val < 0:
                    ret_frame -= min_val  # make everything positive
            elif rli.shape != ret_frame.shape:
                print("RLI and data shapes don't match:",
                      rli.shape,
                      ret_frame.shape,
                      "Skipping RLI division.\n",
                      "You may be using a legacy ZDA data file (or a converted legacy data file)?")

        # TO DO: technically we should apply data inversing, baseline correction, and t-filter to
        # data when computing the display frame...
        # Would probably need to cache processed data to make it fast enough, so forego for now.

        # digital binning
        ret_frame = self.core.create_binned_data(ret_frame, binning_factor=binning)

        # spatial filtering
        ret_frame = self.core.filter_spatial(ret_frame)

        # crop out 1px borders
        ret_frame = ret_frame[1:-2, 1:-2]
        return ret_frame

    def get_display_fp_trace(self, fp_index):
        trial = self.get_current_trial_index()
        traces = self.get_fp_data()
        if trial is None:
            traces = np.average(traces, axis=0)
        return Trace(traces[:, fp_index], self.get_int_pts(), is_fp_trace=True)

    @staticmethod
    def get_frame_mask(h, w, index=None):
        """ Return a frame mask given a polygon """
        if index is None or type(index) != np.ndarray or index.shape[0] == 1:
            return None
        x, y = np.meshgrid(np.arange(w), np.arange(h))  # make a canvas with coordinates
        x, y = x.flatten(), y.flatten()
        points = np.vstack((x, y)).T

        p = Path(index, closed=False)
        mask = p.contains_points(points).reshape(h, w)  # mask of filled in path

        mask_where = np.where(mask)
        if np.size(mask_where) < 1:
            print("frame mask: filled shape is empty:", index, mask_where)
            return None
        return mask

    # returns a Trace object representing trace
    def get_display_trace(self, index=None, fp_index=None):
        trial = self.get_current_trial_index()
        if fp_index is not None:
            return self.get_display_fp_trace(fp_index)

        images = self.get_acqui_images()
        if images is None:
            print("get_display_trace: No images to display.")
            return None

        ret_trace = None
        if index is None:
            return ret_trace
        elif type(index) == np.ndarray:
            if index.shape[0] == 1:
                ret_trace = images[:, index[0, 1], index[0, 0]]
            elif np.size(index) > 0:
                _, h, w = images.shape
                mask = self.get_frame_mask(h, w, index=index)
                ret_trace = images[:, mask]
                ret_trace = np.average(ret_trace, axis=1)
            else:
                print("get_display_trace: drawn shape is empty:", index)

        if ret_trace is None:
            return None

        ret_trace = Trace(ret_trace, self.get_int_pts())

        # data inversing (BEFORE baseline correction)
        if self.get_is_data_inverse_enabled():
            ret_trace.apply_inverse()

        # baseline correction
        fit_type = self.core.get_baseline_correction_options()[self.core.get_baseline_correction_type_index()]
        skip_window = self.core.get_baseline_skip_window()
        ret_trace.baseline_correct_noise(fit_type, skip_window)

        # temporal filtering
        if self.core.get_is_temporal_filter_enabled():
            filter_type = self.core.get_temporal_filter_options()[self.core.get_temporal_filter_index()]
            sigma_t = self.core.get_temporal_filter_radius()
            ret_trace.filter_temporal(filter_type, sigma_t)  # applies time cropping if needed

        return ret_trace

    # Returns the full (x2) memory for hardware to use
    def get_acqui_memory(self):
        trial = self.get_current_trial_index()
        if self.acqui_images is None:
            self.allocate_image_memory()
        if trial is None:
            return self.acqui_images
        return self.acqui_images[trial, :, :, :, :]

    # Returns the full (x2) memory for hardware to use
    def get_rli_memory(self):
        trial = self.get_current_trial_index()
        if self.rli_images is None:
            self.allocate_image_memory()
        if trial is None:
            return self.rli_images
        return self.rli_images[trial, :, :, :, :]

    def get_fp_data(self):
        trial = self.get_current_trial_index()
        if self.fp_data is None:
            self.allocate_image_memory()
        if self.fp_data is None:
            self.fp_data = np.zeros((self.get_num_trials(),
                                     self.get_num_pts(),
                                     self.get_num_fp()),
                                    dtype=np.int16)
        if trial is None or self.get_is_loaded_from_file() or len(self.fp_data.shape) < 3:
            return self.fp_data
        return self.fp_data[trial, :, :]

    def get_acqui_images(self):
        trial = self.get_current_trial_index()
        if self.acqui_images is None:
            return None
        if self.get_is_loaded_from_file():
            if trial is None or len(self.acqui_images.shape) < 4:
                return self.acqui_images
            else:
                return self.acqui_images[trial]

        if trial is None:
            return self.acqui_images[:, 0, :, :, :]
        else:
            return self.acqui_images[trial, 0, :, :, :]

    def get_rli_images(self):
        trial = self.get_current_trial_index()
        if self.rli_images is None:
            return None
        if self.get_is_loaded_from_file():
            if type(self.rli_images) == dict and 'rli_low' in self.rli_images:
                h, w = self.rli_images['rli_low'].shape
                return np.concatenate((self.rli_images['rli_low'].reshape(1, h, w),
                                       self.rli_images['rli_high'].reshape(1, h, w),
                                       self.rli_images['rli_max'].reshape(1, h, w)),
                                      axis=0)
            return self.rli_images
        if trial is None:
            return self.rli_images[:, 0, :, :, :]
        else:
            return self.rli_images[trial, 0, :, :, :]

    def clear_data_memory(self):
        # deleting the refs may trigger garbage collection (ideally)
        del self.acqui_images
        del self.rli_images
        del self.fp_data
        self.rli_images = None
        self.acqui_images = None
        self.fp_data = None

    # trial is ignored if data is from file
    def set_acqui_images(self, data, from_file=False):
        trial = self.get_current_trial_index()
        self.set_is_loaded_from_file(from_file)
        if from_file:
            self.acqui_images = data
            return
        if trial is None:
            self.acqui_images[:, 0, :, :, :] = data[:, :, :, :]
        else:
            if len(data.shape) > 3:
                self.acqui_images[trial, 0, :, :, :] = data[trial, :, :, :]
            else:
                self.acqui_images[trial, 0, :, :, :] = data[:, :, :]

    # trial is ignored if data is from file
    def set_rli_images(self, data, from_file=False):
        trial = self.get_current_trial_index()
        self.set_is_loaded_from_file(from_file)
        if from_file:
            self.rli_images = data
        elif trial is not None:
            self.rli_images[trial, 0, :, :, :] = data[:, :, :]

    def set_fp_data(self, data):
        self.fp_data = data

    def set_num_pts(self, value=1, force_resize=False):
        if type(value) != int or value < 1:
            return
        tmp = self.get_num_pts()
        if force_resize or tmp != value:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.acqui_images, (self.get_num_trials(),
                                          2,  # extra mem for C++ reassembly
                                          self.get_num_pts(),
                                          w,
                                          h))
            self.fp_data = np.resize(self.fp_data,
                                     (self.get_num_trials(),
                                      self.get_num_fp(),
                                      self.get_num_pts()))
            self.hardware.set_num_pts(value=value)

    def calculate_light_rli_frame(self, margins=40):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        if self.get_is_loaded_from_file():
            return self.get_rli_images()[1, :, :]
        n = self.get_num_rli_pts()
        rli_light_frames = self.get_rli_images()[d+margins+1:n-1-margins, :, :]
        if rli_light_frames is None:
            return None
        return np.average(rli_light_frames, axis=0)

    def calculate_dark_rli_frame(self, margins=40):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        if self.get_is_loaded_from_file():
            return self.get_rli_images()[0, :, :]
        rli_dark_frames = self.get_rli_images()[margins+1:d-margins-1, :, :]
        if rli_dark_frames is None:
            return None
        return np.average(rli_dark_frames, axis=0)

    def calculate_max_rli_frame(self):
        if self.get_is_loaded_from_file():
            return self.get_rli_images()[2, :, :]
        rli_frames = self.get_rli_images()
        return np.max(rli_frames, axis=0)

    def calculate_rli(self):
        light = self.calculate_light_rli_frame()
        dark = self.calculate_dark_rli_frame()
        if light is None or dark is None:
            return None
        diff = (light - dark).astype(np.float32)
        diff[diff == 0] = 0.0001  # avoid div by 0
        return diff

    def set_num_dark_rli(self, dark_rli, force_resize=False):
        tmp = self.hardware.get_num_dark_rli()
        if force_resize or tmp != dark_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.get_num_trials(),
                                        2,
                                        self.get_num_rli_pts(),
                                        w,
                                        h))
            self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_num_light_rli(self, light_rli, force_resize=False):
        tmp = self.hardware.get_num_light_rli()
        if force_resize or tmp != light_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.get_num_trials(),
                                        2,  # extra mem for C++ reassembly
                                        self.get_num_rli_pts(),
                                        w,
                                        h))
            self.hardware.set_num_light_rli(light_rli=light_rli)

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this width
    def get_display_width(self):
        if self.get_is_loaded_from_file():
            w = self.file_metadata['raw_width']
            if w < 1:
                w = self.get_acqui_images().shape[-2]
            return w
        return self.display_widths[self.get_camera_program()]

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this height
    def get_display_height(self):
        if self.get_is_loaded_from_file():
            h = self.file_metadata['raw_height']
            if h < 1:
                h = self.get_acqui_images().shape[-1]
            return h
        return self.display_heights[self.get_camera_program()]

    ''' Attributes controlled at Data level '''

    def get_num_fp(self):
        if self.get_is_loaded_from_file():
            if 'num_fp' in self.file_metadata:
                return self.file_metadata['num_fp']
        return 4  # Little Dave: Fixed at 4 field potential measurements with NI-USB

    def set_num_fp(self, value):
        self.num_fp_pts = value

    def get_num_trials(self):
        if self.get_is_loaded_from_file() and 'num_trials' in self.file_metadata:
            return self.file_metadata['num_trials']
        return self.num_trials

    def set_num_trials(self, value):
        self.num_trials = value

    def get_int_trials(self):
        if self.get_is_loaded_from_file() and 'int_trials' in self.file_metadata:
            return self.file_metadata['int_trials']
        return self.int_trials

    def set_int_trials(self, value):
        self.int_trials = value

    def get_num_records(self):
        return self.num_records

    def set_num_records(self, value):
        self.num_records = value

    def get_int_records(self):
        return self.int_records

    def set_int_records(self, value):
        self.int_records = value

    def get_is_loaded_from_file(self):
        return self.is_loaded_from_file

    def set_is_loaded_from_file(self, value):
        self.is_loaded_from_file = value

    def get_is_auto_save_enabled(self):
        return self.auto_save_enabled

    def set_is_auto_save_enabled(self, value):
        self.auto_save_enabled = value

    def get_is_schedule_rli_enabled(self):
        return self.schedule_rli_enabled

    def set_is_schedule_rli_enabled(self, value):
        self.schedule_rli_enabled = value

    ''' Attributes controlled at Hardware level '''

    def get_int_pts(self):
        if self.get_is_loaded_from_file() and 'int_pts' in self.file_metadata:
            int_pts = self.file_metadata['int_pts']
            if int_pts > 0:
                return int_pts
        return self.hardware.get_int_pts()

    # Is this even needed?
    def get_duration(self):
        return self.hardware.get_duration()

    def get_acqui_duration(self):
        return self.hardware.get_acqui_duration()

    def get_num_pts(self):
        if self.get_is_loaded_from_file():
            num_pts = 0
            if 'points_per_trace' in self.file_metadata:
                num_pts = self.file_metadata['points_per_trace']
            elif 'num_pts' in self.file_metadata:
                num_pts = self.file_metadata['num_pts']
            if num_pts > 0:
                return num_pts
            else:
                return self.get_acqui_images().shape[-3]
        return self.hardware.get_num_pts()

    def get_num_rli_pts(self):
        if self.get_is_loaded_from_file():
            if 'rli_pts_dark' in self.file_metadata and 'rli_pts_light' in self.file_metadata:
                return self.file_metadata['rli_pts_dark'] + self.file_metadata['rli_pts_light']
            else:
                return 3  # legacy ZDA format
        return self.hardware.get_num_dark_rli() + self.hardware.get_num_light_rli()

    def get_num_pulses(self, ch):
        return self.hardware.get_num_pulses(channel=ch)

    def get_acqui_onset(self):
        if self.get_is_loaded_from_file():
            return self.file_metadata['acquisition_onset']
        return self.hardware.get_acqui_onset()

    def get_stim_onset(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.file_metadata['stimulation' + str(ch) + '_onset']
            return self.hardware.get_stim_onset(channel=ch)

    def get_stim_duration(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.file_metadata['stimulation' + str(ch) + '_duration']
            return self.hardware.get_stim_duration(channel=ch)

    def get_current_trial_index(self):
        return self.current_trial_index

    def set_current_trial_index(self, v):
        self.current_trial_index = v

    def increment_current_trial_index(self):
        self.current_trial_index = min(self.current_trial_index + 1,
                                       self.get_num_trials())

    def decrement_current_trial_index(self):
        self.current_trial_index = max(self.current_trial_index - 1, 0)

    def get_is_rli_division_enabled(self):
        return self.is_rli_division_enabled

    def set_is_rli_division_enabled(self, v):
        self.is_rli_division_enabled = v

    def get_is_data_inverse_enabled(self):
        return self.is_data_inverse_enabled

    def set_is_data_inverse_enabled(self, v):
        self.is_data_inverse_enabled = v

    def get_is_livefeed_enabled(self):
        return self.is_live_feed_enabled

    def set_is_livefeed_enabled(self, v):
        self.is_live_feed_enabled = v

    def get_display_value_option_index(self):
        return self.display_value_option_index

    def set_display_value_option_index(self, v):
        self.display_value_option_index = v

    def get_livefeed_frame(self):
        if self.get_is_livefeed_enabled():
            if self.livefeed_frame is not None:
                return self.livefeed_frame
            else:
                w = self.get_display_width()
                h = self.get_display_height()
                self.livefeed_frame = np.zeros((2, h, w), dtype=np.uint16)
                return self.livefeed_frame

    def clear_livefeed_frame(self):
        self.livefeed_frame = None