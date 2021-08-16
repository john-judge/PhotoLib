import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore
from pyPhoto21.viewers.trace import Trace
from pyPhoto21.database.database import Database
from pyPhoto21.database.file import File
from pyPhoto21.database.legacy import LegacyData


# This class will supersede Data...
class Data(File):

    # Parsed events from GUI are handed to Data for backend effects.
    def __init__(self, hardware):
        super().__init__()

        # interaction with other modules. It is the Data class' responsibility to sync all these.
        self.hardware = hardware
        self.db = Database()
        self.core = AnalysisCore(self.db.meta)

        # Internal (non-user facing) settings and flags
        self.is_loaded_from_file = False
        self.is_live_feed_enabled = False
        self.current_trial_index = 0

        # Memory not written to file
        # raw RLI frames are not written to file or even shown.
        # We will calculate averages before writing in metadata.
        self.rli_images = None
        self.livefeed_frame = None

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

        # Init actions
        self.sync_hardware_from_metadata()
        self.sync_analysis_from_metadata()
        self.allocate_image_memory()

    def sync_analysis_from_metadata(self):
        pass

    # numpy autosaves the image arrays; only need to save meta actively
    def save_metadata_to_compressed_file(self):
        pass

    def sync_hardware_from_metadata(self):
        if self.get_is_loaded_from_file():
            print("Settings loaded from data file will be overwritten with currnet hardware settings.")
            self.set_is_loaded_from_file(False)

        # Settings that don't matter to File
        self.set_camera_program(self.db.meta.camera_program, prevent_resize=True)
        self.set_num_pts(self.db.meta.num_pts, prevent_resize=True)
        self.set_num_dark_rli(280, prevent_resize=True)  # currently not user-configurable
        self.set_num_light_rli(200, prevent_resize=True)  # currently not user-configurable

        self.hardware.set_num_pulses(value=self.db.meta.num_pulses[0],
                                     channel=1)
        self.hardware.set_num_pulses(value=self.db.meta.num_pulses[1],
                                     channel=2)

        self.hardware.set_int_pulses(value=self.db.meta.int_pulses[0],
                                     channel=1)
        self.hardware.set_int_pulses(value=self.db.meta.int_pulses[1],
                                     channel=2)

        self.hardware.set_num_bursts(value=self.db.meta.num_bursts[0],
                                     channel=1)
        self.hardware.set_num_bursts(value=self.db.meta.num_bursts[1],
                                     channel=2)

        self.hardware.set_int_bursts(value=self.db.meta.int_bursts[0],
                                     channel=1)
        self.hardware.set_int_bursts(value=self.db.meta.int_bursts[1],
                                     channel=2)

        self.hardware.set_acqui_onset(acqui_onset=self.db.meta.acqui_onset)

        self.hardware.set_stim_onset(value=self.db.meta.stim_onset[0],
                                     channel=1)
        self.hardware.set_stim_onset(value=self.db.meta.stim_onset[1],
                                     channel=2)
        self.hardware.set_stim_duration(value=self.db.meta.stim_duration[0],
                                        channel=1)
        self.hardware.set_stim_duration(value=self.db.meta.stim_duration[1],
                                        channel=2)

    def set_meta(self, meta):
        """ Given a Metadata class instance, replace in current settings
            The GUI will update itself after the call to here returns """
        self.db.meta = meta
        self.core.meta = meta
        self.sync_hardware_from_metadata()
        self.sync_analysis_from_metadata()

    def find_unused_filenames(self, extensions=('.npy', '.pbz2')):
        # gets filenames to save to, avoiding overwrites, recognizing all extensions
        filenames = [self.get_filename(self.db.meta.current_slice,
                                       self.db.meta.current_location,
                                       self.db.meta.current_record,
                                       ext,
                                       path=None)  # no path, as file_exists doesn't want it
                     for ext in extensions]

        while any([self.file_exists(fn) for fn in filenames]):
            self.set_override_filename(None)
            self.increment_record()
            filenames = [self.get_filename(self.db.meta.current_slice,
                                           self.db.meta.current_location,
                                           self.db.meta.current_record,
                                           ext,
                                           path=None)
                         for ext in extensions]
        return filenames

    def load_from_file(self, file):
        file_prefix, extension = file.split('.')
        meta_file = file_prefix + ".pbz2"
        data_file = file_prefix + '.npy'
        if extension == "zda":
            self.set_override_filename(data_file)
            meta_obj = LegacyData().load_zda(file, self.db)  # side-effect is to create and populate .npy file
            self.set_meta(meta_obj)
        elif extension in ['npy', 'pbz2']:
            if not self.file_exists(meta_file.split("\\")[0]):
                print("Corresponding metadata file", meta_file, "not found.")
                return
            if not self.file_exists(data_file.split("\\")[0]):
                print("Corresponding data file", data_file, "not found.")
                return
            self.set_override_filename(data_file)
            meta_obj = self.load_metadata_from_file(meta_file)
            self.db.load_mmap_file()
            self.set_meta(meta_obj)

    def save_metadata_to_file(self, filename):
        """ Pickle the instance of Metadata class """
        self.dump_python_object_to_pickle(filename, self.db.meta)

    def load_metadata_from_file(self, filename):
        """ Read instance of Metadata class and load into usage"""
        return self.retrieve_python_object_from_pickle(filename)

    def get_record_array_shape(self):
        return (self.get_num_trials(),
                2,
                self.get_num_pts(),
                self.get_display_height(),
                self.get_display_width())

    def get_slice_num(self):
        return self.db.meta.current_slice

    def get_location_num(self):
        return self.db.meta.current_location

    def get_record_num(self):
        return self.db.meta.current_record

    def increment_slice(self, num=1):
        self.db.meta.current_slice += num
        self.db.meta.current_location = 0
        self.db.meta.current_record = 0
        self.set_current_trial_index(0)

    def increment_location(self, num=1):
        self.db.meta.current_location += num
        self.db.meta.current_record = 0
        self.set_current_trial_index(0)

    def increment_record(self, num=1):
        self.db.meta.current_record += num
        self.set_current_trial_index(0)

    def decrement_slice(self, num=1):
        self.db.meta.current_slice -= num
        if self.db.meta.current_slice >= 0:
            self.db.meta.current_location = 0
            self.db.meta.current_record = 0
            self.set_current_trial_index(0)
        else:
            self.db.meta.current_slice = 0

    def decrement_location(self, num=1):
        self.db.meta.current_location -= num
        if self.db.meta.current_location >= 0:
            self.db.meta.current_record = 0
            self.set_current_trial_index(0)
        else:
            self.db.meta.current_location = 0

    def decrement_record(self, num=1):
        self.db.meta.current_record -= 1

    def set_slice(self, v):
        if v > self.db.meta.current_slice:
            self.db.meta.increment_slice(v - self.db.meta.current_slice)
        elif v < self.db.meta.current_slice:
            self.db.meta.decrement_slice(self.db.meta.current_slice - v)

    def set_record(self, v):
        if v > self.db.meta.current_record:
            self.db.meta.increment_record(v - self.db.meta.current_record)
        elif v < self.db.meta.current_record:
            self.db.meta.decrement_record(self.db.meta.current_record - v)

    def set_location(self, v):
        if v > self.db.meta.current_location:
            self.db.meta.increment_location(v - self.db.meta.current_location)
        if v < self.db.meta.current_location:
            self.db.meta.decrement_location(self.db.meta.current_location - v)

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this width
    def get_display_width(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.width
        return self.display_widths[self.get_camera_program()]

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this height
    def get_display_height(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.height
        return self.display_heights[self.get_camera_program()]

    ''' Attributes controlled at Data level '''

    def get_num_fp(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_fp
        return 4  # Little Dave: Fixed at 4 field potential measurements with NI-USB

    def set_num_fp(self, value):
        self.db.meta.num_fp = value

    def set_camera_program(self, program, force_resize=False, prevent_resize=False):
        curr_program = self.hardware.get_camera_program()
        self.hardware.set_camera_program(program=program)
        if (force_resize or curr_program != program) and not prevent_resize:
            self.db.meta.camera_program = program
            self.db.meta.width = self.get_display_width()
            self.db.meta.height = self.get_display_height()
            self.db.clear_or_resize_mmap_file()

    def get_camera_program(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.camera_program
        return self.hardware.get_camera_program()

    def set_crop_window(self, v, index=None):
        if index is None:
            self.db.meta.crop_window = v
        elif index == 0 or index == 1:
            self.db.meta.crop_window[index] = v

    def get_crop_window(self, index=None):
        if index is None:
            return self.db.meta.crop_window
        return self.db.meta.crop_window[index]

    def get_num_pts(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_pts
        return self.hardware.get_num_pts()

    def get_num_pulses(self, ch):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_pulses[ch - 1]
        return self.hardware.get_num_pulses(channel=ch)

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

    # We allocate twice the memory since C++ needs room for CDS subtraction
    def allocate_image_memory(self):
        w = self.get_display_width()
        h = self.get_display_height()

        # raw RLI frames are not written to file or even shown.
        # We will calculate averages before writing in metadata.
        self.rli_images = np.zeros((self.get_num_trials(),
                                    2,
                                    self.get_num_rli_pts(),
                                    h,
                                    w,),
                                   dtype=np.uint16)
        self.db.meta.fp_data = np.zeros((self.get_num_trials(),
                                         self.get_num_pts(),
                                         self.get_num_fp()),
                                        dtype=np.int16)

        self.db.clear_or_resize_mmap_file()

    # Move to GUI? or FV?
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
        x, y = np.meshgrid(np.arange(h), np.arange(w))  # make a canvas with coordinates
        x, y = x.flatten(), y.flatten()
        points = np.vstack((x, y)).T

        p = Path(index, closed=False)
        mask = p.contains_points(points).reshape(h, w)  # mask of filled in path

        mask_where = np.where(mask)
        if np.size(mask_where) < 1:
            print("frame mask: filled shape is empty:", index, mask_where)
            return mask
        return mask

    # Move to GUI? or TV?
    # returns a Trace object representing trace
    def get_display_trace(self, index=None, fp_index=None):
        trial = self.get_current_trial_index()
        if fp_index is not None:
            return self.get_display_fp_trace(fp_index)

        images = self.get_acqui_images()
        if images is None:
            print("get_display_trace: No images to display.")
            return None

        print(index, index.shape, type(index))
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

    def get_fp_data(self):
        trial = self.get_current_trial_index()
        if self.db.meta.fp_data is None:
            self.db.meta.fp_data = np.zeros((self.get_num_trials(),
                                             self.get_num_pts(),
                                             self.get_num_fp()),
                                            dtype=np.int16)
        if trial is None:
            return self.db.meta.fp_data
        return self.db.meta.fp_data[trial, :, :]

    def get_acqui_images(self):
        trial = self.get_current_trial_index()
        if trial is None:
            return self.db.load_data_raw()
        else:
            return self.db.load_trial_data_raw(trial)

    def get_rli_images(self):
        trial = self.get_current_trial_index()
        if trial is None:
            return self.rli_images[:, 0, :, :, :]
        else:
            return self.rli_images[trial, 0, :, :, :]

    def set_num_pts(self, value=1, force_resize=False, prevent_resize=False):
        if type(value) != int or value < 1:
            return
        tmp = self.get_num_pts()
        if (force_resize or tmp != value) and not prevent_resize:
            self.db.clear_or_resize_mmap_file()
            self.db.meta.fp_data = np.resize(self.db.meta.fp_data,
                                     (self.get_num_trials(),
                                      self.get_num_fp(),
                                      self.get_num_pts()))
        self.hardware.set_num_pts(value=value)

    # Populate meta.rli_high from RLI raw frames
    def calculate_light_rli_frame(self, margins=40):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        if self.get_is_loaded_from_file() or self.db.meta.rli_high is not None:
            return self.db.meta.rli_high
        n = self.get_num_rli_pts()
        rli_light_frames = self.get_rli_images()[d + margins + 1:n - 1 - margins, :, :]
        if rli_light_frames is None:
            return None
        self.db.meta.rli_high = np.average(rli_light_frames, axis=0)
        return self.db.meta.rli_high

    def calculate_dark_rli_frame(self, margins=40):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        if self.get_is_loaded_from_file() or self.db.meta.rli_low is not None:
            return self.db.meta.rli_low
        rli_dark_frames = self.get_rli_images()[margins + 1:d - margins - 1, :, :]
        if rli_dark_frames is None:
            return None
        self.db.meta.rli_low = np.average(rli_dark_frames, axis=0)
        return self.db.meta.rli_low

    def calculate_max_rli_frame(self):
        if self.get_is_loaded_from_file()  or self.db.meta.rli_max is not None:
            return self.db.meta.rli_max
        rli_frames = self.get_rli_images()
        self.db.meta.rli_max = np.max(rli_frames, axis=0)
        return self.db.meta.rli_max

    def calculate_rli(self):
        light = self.calculate_light_rli_frame()
        dark = self.calculate_dark_rli_frame()
        if light is None or dark is None:
            return None
        diff = (light - dark).astype(np.float32)
        diff[diff == 0] = 0.0001  # avoid div by 0
        return diff

    def set_num_dark_rli(self, dark_rli, force_resize=False, prevent_resize=False):
        tmp = self.hardware.get_num_dark_rli()
        if (force_resize or tmp != dark_rli) and not prevent_resize:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.get_num_trials(),
                                        2,
                                        self.get_num_rli_pts(),
                                        w,
                                        h))
        self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_num_light_rli(self, light_rli, force_resize=False, prevent_resize=False):
        tmp = self.hardware.get_num_light_rli()
        if (force_resize or tmp != light_rli) and not prevent_resize:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.get_num_trials(),
                                        2,  # extra mem for C++ reassembly
                                        self.get_num_rli_pts(),
                                        w,
                                        h))
        self.hardware.set_num_light_rli(light_rli=light_rli)

    def get_num_trials(self):
        return self.db.meta.num_trials

    def set_num_trials(self, value):
        self.db.meta.num_trials = value

    def get_int_trials(self):
        return self.db.meta.int_trials

    def set_int_trials(self, value):
        self.db.meta.int_trials = value

    def get_num_records(self):
        return self.db.meta.num_records

    def set_num_records(self, value):
        self.db.meta.num_records = value

    def get_int_records(self):
        return self.db.meta.int_records

    def set_int_records(self, value):
        self.db.meta.int_records = value

    def get_is_loaded_from_file(self):
        return self.is_loaded_from_file

    def set_is_loaded_from_file(self, value):
        self.is_loaded_from_file = value

    def get_is_auto_save_enabled(self):
        return self.db.meta.auto_save_enabled

    def set_is_auto_save_enabled(self, value):
        self.db.meta.auto_save_enabled = value

    def get_is_schedule_rli_enabled(self):
        return self.db.meta.schedule_rli_enabled

    def set_is_schedule_rli_enabled(self, value):
        self.db.meta.schedule_rli_enabled = value

    ''' Attributes controlled at Hardware level '''

    def get_int_pts(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.int_pts
        return self.hardware.get_int_pts()

    def get_acqui_duration(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_pts * self.db.meta.int_pts
        return self.hardware.get_acqui_duration()

    def get_num_rli_pts(self):
        return self.hardware.get_num_dark_rli() + self.hardware.get_num_light_rli()

    def get_acqui_onset(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.acqui_onset
        return self.hardware.get_acqui_onset()

    def get_stim_onset(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.db.meta.stim_onset[ch-1]
            return self.hardware.get_stim_onset(channel=ch)

    def get_stim_duration(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.db.meta.stim_duration[ch-1]
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
        return self.db.meta.is_rli_division_enabled

    def set_is_rli_division_enabled(self, v):
        self.db.meta.is_rli_division_enabled = v

    def get_is_data_inverse_enabled(self):
        return self.db.meta.is_data_inverse_enabled

    def set_is_data_inverse_enabled(self, v):
        self.db.meta.is_data_inverse_enabled = v

    def get_is_livefeed_enabled(self):
        return self.is_live_feed_enabled

    def set_is_livefeed_enabled(self, v):
        self.is_live_feed_enabled = v

    def get_display_value_option_index(self):
        return self.db.meta.display_value_option_index

    def set_display_value_option_index(self, v):
        self.db.meta.display_value_option_index = v

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
