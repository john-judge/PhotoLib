import time
import threading

import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore
from pyPhoto21.viewers.trace import Trace
from pyPhoto21.database.database import Database
from pyPhoto21.database.file import File
from pyPhoto21.database.legacy import LegacyData
from pyPhoto21.database.metadata import Metadata
from pyPhoto21.analysis.process import Processor


# This class will supersede Data...
class Data(File):

    # Parsed events from GUI are handed to Data for backend effects.
    def __init__(self, hardware):

        # interaction with other modules. It is the Data class' responsibility to sync all these.
        self.hardware = hardware
        self.db = Database()
        self.core = AnalysisCore(self.db.meta)
        self.gui = None

        self.full_data_processor = Processor(self)
        super().__init__(self.db.meta)

        # Internal (non-user facing) settings and flags
        self.is_loaded_from_file = False
        self.is_live_feed_enabled = False
        self.current_trial_index = 0
        self.metadata_extension = '.json'
        self.is_metadata_dirty = False
        self.meta_daemon_stop_flag = False

        # Memory not written to file
        # raw RLI frames are not written to file or even shown.
        # We will calculate averages before writing in metadata.
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
        self.sync_from_metadata()

        # If enabled, start data processor listening for jobs in background
        if self.full_data_processor.enabled:
            threading.Thread(target=self.full_data_processor.process_continually,
                             args=(),
                             daemon=True).start()

    def sync_from_metadata(self, suppress_allocate=False):
        if self.get_is_trial_averaging_enabled():
            self.set_current_trial_index(None)
        self.sync_hardware_from_metadata()
        self.sync_analysis_from_metadata()
        if not suppress_allocate:
            self.allocate_image_memory()

    def is_processed_data_up_to_date(self):
        return not self.full_data_processor.get_is_data_up_to_date()

    def stop_background_processing(self):
        self.full_data_processor.stop_processor()

    # blocks until processor daemon has stopped
    def acquire_processing_lock(self):
        if not self.full_data_processor.enabled:
            return
        # don't need atomic operations -- this will be called from main thread,
        # which is the only thread that should be queuing processing tasks.
        self.full_data_processor.pause_processor()
        while self.full_data_processor.get_is_active():
            time.sleep(0.5)

    def drop_processing_lock(self):
        if not self.full_data_processor.enabled:
            return
        self.full_data_processor.unpause_processor()

    def sync_analysis_from_metadata(self):
        if not self.full_data_processor.enabled:
            return
        self.full_data_processor.update_full_processed_data()

    # numpy autosaves the image arrays; only need to save meta actively
    def save_metadata_to_json(self):
        fn = self.db.get_current_filename(extension=self.metadata_extension)
        print("Saving metadata to:", fn)
        self.save_metadata_to_file(fn)

    def sync_hardware_from_metadata(self):
        # if self.get_is_loaded_from_file():
        #     self.set_is_loaded_from_file(False)

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

        self.set_acqui_onset(self.db.meta.acqui_onset)

        self.hardware.set_stim_onset(value=self.db.meta.stim_onset[0],
                                     channel=1)
        self.hardware.set_stim_onset(value=self.db.meta.stim_onset[1],
                                     channel=2)
        self.hardware.set_stim_duration(value=self.db.meta.stim_duration[0],
                                        channel=1)
        self.hardware.set_stim_duration(value=self.db.meta.stim_duration[1],
                                        channel=2)

    def set_meta(self, meta, suppress_resize=False):
        """ Given a Metadata class instance, replace in current settings
            The GUI will update itself after the call to here returns """
        self.db.meta = meta
        self.core.meta = meta
        self.meta = meta
        self.sync_from_metadata(suppress_allocate=suppress_resize)
        if not suppress_resize:
            self.allocate_image_memory()

    def load_current_metadata_file(self):
        meta_file = self.db.get_current_filename(extension=self.metadata_extension)
        meta_no_path = self.strip_path(meta_file)
        if not self.file_exists(meta_no_path):
            # Need to create a metadata file
            # instead of blanking out and going to clean defaults,
            # persist current settings but clear data
            self.meta.num_fp = 4
            self.meta.version = 6
            self.set_camera_program(7)
            self.save_metadata_to_file(meta_file)
            print("No metadata file found: \n", meta_file,
                  "\nA metadata file has been created. Please make any corrections to the "
                  "file manually by opening it with a text editor.")
            # other defaults are more handy to persist.
        else:
            self.set_is_loaded_from_file(True)
            meta_obj = self.load_metadata_from_file(meta_file)
            if meta_obj is not None:
                self.set_meta(meta_obj, suppress_resize=True)

    # assumes file has already been validated to exist
    def load_preference_file(self, file):
        file_prefix, extension = file.split('.')
        if extension != 'json':
            print("Failed to load", file, "\n\t Only .json files are supported.")
            return
        meta_obj = self.load_metadata_from_file(file)
        if meta_obj is not None:
            self.set_meta(meta_obj, suppress_resize=True)

    def save_preference_file(self, meta_file):
        self.save_metadata_to_file(meta_file)
        print("Saved your preferences to:\n", meta_file)

    def load_from_file(self, file):
        self.save_metadata_to_json()
        self.set_is_loaded_from_file(True)
        print("Loading from:", file)
        orig_path_prefix = file.split(".")[0]
        file = self.strip_path(file)
        file_prefix, extension = file.split('.')

        if extension == "zda":
            # We will auto-create some files, so find names:
            self.increment_record_until_filename_free()

            new_meta = Metadata()
            self.set_meta(new_meta, suppress_resize=True)
            ld = LegacyData(self.get_save_dir())
            ld.load_zda(orig_path_prefix + '.zda',
                        self.db,
                        new_meta)  # side-effect is to create and populate .npy file
            self.save_metadata_to_json()
        elif extension in ['npy', 'json']:
            meta_no_path = file_prefix + self.metadata_extension
            meta_file = orig_path_prefix + self.metadata_extension
            data_no_path = file_prefix + self.db.extension
            data_file = orig_path_prefix + self.db.extension
            if not self.file_exists(meta_no_path):
                print("Corresponding metadata file", meta_no_path, "not found.")
                return
            if not self.file_exists(data_no_path):
                print("Corresponding data file", data_no_path, "not found. Loading as preference-only file.")
            meta_obj = self.load_metadata_from_file(meta_file)
            if meta_obj is not None:
                self.set_meta(meta_obj, suppress_resize=True)
                self.db.load_mmap_file(filename=data_file, mode="r+")

    def save_metadata_to_file(self, filename):
        """ Pickle the instance of Metadata class """
        self.dump_python_object_to_json(filename, self.db.meta)

    def load_metadata_from_file(self, filename):
        """ Read instance of Metadata class and load into usage"""
        meta_dict = self.retrieve_python_object_from_json(filename)
        if meta_dict is not None and type(meta_dict) == dict:
            meta = self.handle_missing_meta_attributes(meta_dict)
            return meta
        else:
            print("Failed to load metadata object from", filename)
            print(type(meta_dict), meta_dict)

    # load metadata defaults into file metadata for backwards compatibility
    @staticmethod
    def handle_missing_meta_attributes(loaded_meta_dict):
        new_meta = Metadata()
        loaded_ct = 0
        for attr in loaded_meta_dict:
            if not attr.startswith("_"):
                setattr(new_meta, attr, loaded_meta_dict[attr])
                loaded_ct += 1
        print("Loaded", loaded_ct, "attributes from the metadata file.")
        return new_meta  # defaults kept if not in loaded dict

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

    def get_is_trial_averaging_enabled(self):
        return self.meta.is_trial_averaging_enabled

    def set_is_trial_averaging_enabled(self, v):
        self.meta.is_trial_averaging_enabled = v

    def increment_slice(self, num=1):
        self.save_metadata_to_json()
        self.db.meta.current_slice += num
        self.db.meta.current_location = 0
        self.db.meta.current_record = 0
        self.set_current_trial_index(0)
        self.db.open_filename = None
        self.load_current_metadata_file()
        self.db.load_mmap_file(mode=None)
        self.full_data_processor.update_full_processed_data()

    def increment_location(self, num=1):
        self.save_metadata_to_json()
        self.db.meta.current_location += num
        self.db.meta.current_record = 0
        self.set_current_trial_index(0)
        self.db.open_filename = None
        self.load_current_metadata_file()
        self.db.load_mmap_file(mode=None)
        self.full_data_processor.update_full_processed_data()

    def increment_record(self, num=1, suppress_file_create=False):
        self.save_metadata_to_json()
        self.db.meta.current_record += num
        self.set_current_trial_index(0)
        self.db.open_filename = None
        if not suppress_file_create:
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()

    # includes paths
    def find_existing_file_pair(self, direction=1):
        files = self.get_filenames_in_folder()
        paired_files = []
        # turn files list into a map
        file_map = {}
        unindexed_map = {}
        for file in files:
            parts = self.decompose_filename(file)
            if len(parts) == 4:
                slic, loc, rec, ext = parts
                if slic not in file_map:
                    file_map[slic] = {}
                if loc not in file_map[slic]:
                    file_map[slic][loc] = {}
                if rec not in file_map[slic][loc]:
                    file_map[slic][loc][rec] = []
                if ext not in file_map[slic][loc][rec] and ext in ['json', 'npy']:
                    file_map[slic][loc][rec].append(file)
                    if len(file_map[slic][loc][rec]) == 2:
                        file, _ = file.split('.')
                        paired_files.append(file)
            # unconventionally named files: name is not indexed
            elif len(parts) == 2:
                file_prefix, _ = parts
                if file_prefix not in unindexed_map:
                    unindexed_map[file_prefix] = []
                unindexed_map[file_prefix].append(file)
                if len(unindexed_map[file_prefix]) == 2:
                    file, _ = file.split('.')
                    paired_files.append(file)

        curr_filename = self.db.get_current_filename(extension='', no_path=True)
        if curr_filename not in paired_files:
            paired_files.append(curr_filename)
        paired_files.sort()

        ind = (paired_files.index(curr_filename) + direction) % len(paired_files)
        file_prefix = self.get_save_dir() + "\\" + paired_files[ind]
        return file_prefix + self.metadata_extension, file_prefix + self.db.extension

    def auto_change_file(self, direction=1):
        self.save_metadata_to_json()
        files = self.find_existing_file_pair(direction=direction)  # includes paths
        if files is None:
            return
        self.set_is_loaded_from_file(True)
        meta_file, data_file = files
        meta_obj = self.load_metadata_from_file(meta_file)
        if meta_obj is not None:
            self.set_meta(meta_obj, suppress_resize=True)
            self.db.load_mmap_file(mode='r+', filename=data_file)
            self.full_data_processor.update_full_processed_data()

    def increment_file(self):
        self.auto_change_file(direction=1)

    def decrement_file(self):
        self.auto_change_file(direction=-1)

    def decrement_slice(self, num=1):
        self.save_metadata_to_json()
        self.db.meta.current_slice -= num
        if self.db.meta.current_slice >= 0:
            self.set_current_trial_index(0)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        else:
            self.db.meta.current_slice = 0
            if num > 1:
                self.db.meta.current_location = 0
                self.db.meta.current_record = 0
                self.db.open_filename = None
                self.load_current_metadata_file()
                self.db.load_mmap_file(mode=None)
                self.full_data_processor.update_full_processed_data()

    def decrement_location(self, num=1):
        self.save_metadata_to_json()
        self.db.meta.current_location -= num
        if self.db.meta.current_location >= 0:
            self.db.meta.current_record = 0
            self.set_current_trial_index(0)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        else:
            self.db.meta.current_location = 0
            if num > 1:
                self.db.open_filename = None
                self.load_current_metadata_file()
                self.db.load_mmap_file(mode=None)
                self.full_data_processor.update_full_processed_data()

    def decrement_record(self, num=1):
        self.save_metadata_to_json()
        self.db.meta.current_record -= num
        if self.db.meta.current_record >= 0:
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        else:
            self.db.meta.current_record = 0
            if num > 1:
                self.db.open_filename = None
                self.load_current_metadata_file()
                self.db.load_mmap_file(mode=None)
                self.full_data_processor.update_full_processed_data()

    def set_slice(self, v):
        self.save_metadata_to_json()
        if v > self.db.meta.current_slice:
            self.increment_slice(v - self.db.meta.current_slice)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        elif v < self.db.meta.current_slice:
            self.decrement_slice(self.db.meta.current_slice - v)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()

    def set_record(self, v):
        self.save_metadata_to_json()
        if v > self.db.meta.current_record:
            self.increment_record(v - self.db.meta.current_record)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        elif v < self.db.meta.current_record:
            self.decrement_record(self.db.meta.current_record - v)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()

    def set_location(self, v):
        self.save_metadata_to_json()
        if v > self.db.meta.current_location:
            self.increment_location(v - self.db.meta.current_location)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()
        if v < self.db.meta.current_location:
            self.decrement_location(self.db.meta.current_location - v)
            self.db.open_filename = None
            self.load_current_metadata_file()
            self.db.load_mmap_file(mode=None)
            self.full_data_processor.update_full_processed_data()

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

    def set_camera_program(self, program, force_resize=False, prevent_resize=False, suppress_processing=False):
        curr_program = self.hardware.get_camera_program()
        if curr_program is not None:
            self.hardware.set_camera_program(program=program)
        if (force_resize or curr_program != program) and not prevent_resize:
            self.db.meta.camera_program = program
            self.db.meta.width = self.get_display_width()
            self.db.meta.height = self.get_display_height()
            self.meta.int_pts = self.get_int_pts()  # syncs from hardware
            self.increment_record_until_filename_free()
        if not suppress_processing:
            self.full_data_processor.update_full_processed_data()

    def get_camera_program(self):
        cam_prog = self.hardware.get_camera_program()
        if self.get_is_loaded_from_file() or cam_prog is None:
            return self.db.meta.camera_program
        return cam_prog

    def set_measure_window(self, kind, index, value, suppress_processing=False):
        if index is None:
            self.db.meta.measure_window = value
        elif index == 0 or index == 1:
            self.db.meta.measure_window[index] = value
        if not suppress_processing:
            self.full_data_processor.update_full_processed_data()

    def get_measure_window(self, index=None):
        if index is None:
            return self.db.meta.measure_window
        return self.db.meta.measure_window[index]

    def get_num_pts(self):
        num_pts = self.hardware.get_num_pts()
        if self.get_is_loaded_from_file() or num_pts is None:
            return self.db.meta.num_pts
        return num_pts

    def get_num_pulses(self, ch):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_pulses[ch - 1]
        return self.hardware.get_num_pulses(channel=ch)

    # The purpose of this linspace is to
    # allow us to remove trace points without
    # losing track of the absolute frame number w.r.t
    # the stim time, etc
    def get_cropped_linspace(self, start_frames=None, end_frames=None):
        if start_frames is None or end_frames is None:
            start_frames, end_frames = self.get_artifact_exclusion_window()
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
        self.db.rli_images = np.zeros((2,
                                       self.get_num_rli_pts(),
                                       h,
                                       w),
                                      dtype=np.uint16)
        self.increment_record_until_filename_free()

    def increment_record_until_filename_free(self):
        if self.get_is_loaded_from_file():  # more interested in looking at existing files.
            return
        i = 0
        while i < 100 and (
                self.file_exists(self.db.get_current_filename(no_path=True,
                                                              extension=self.db.extension))
                or self.file_exists(self.db.get_current_filename(no_path=True,
                                                                 extension=self.metadata_extension))):
            if self.db.is_current_data_file_empty():  # will load file
                print(self.db.get_current_filename(no_path=True,
                                                   extension=self.db.extension),
                      "exists but is all zeros. Planning to overwrite.")
                return
            else:
                self.db.open_filename = None  # don't consider it fully loaded

            if self.db.open_filename is not None:
                self.db.open_filename = None
            else:
                self.increment_record(suppress_file_create=True)  # can create new files
                i += 1

        if i >= 100:
            print("data.py: searched 100 filenames for free name. Likely an issue.")
        self.db.load_mmap_file(filename=self.db.get_current_filename(extension=self.db.extension), mode=None)

    def get_background_option_index(self):
        return self.db.meta.background_option_index

    @staticmethod
    def get_background_options():
        return ['Frame Selector Amp', 'Max Amp', 'MaxAmp/SD', 'Mean Amp', 'MeanAmp/SD']

    @staticmethod
    def bg_uses_frame_selector(bg_name):
        return "Frame Selector" in bg_name

    def apply_temporal_aggregration_frame(self, images):
        ret_frame = None
        if images.size < 1:
            return None
        agg_type = self.get_background_options()[self.get_background_option_index()]

        if agg_type == 'Amp at Frame Selector':
            return images
        elif agg_type == 'Max Amp':
            ret_frame = np.max(images, axis=0)
        elif agg_type == 'MaxAmp/SD':
            std = np.std(images, axis=0)
            std[std == 0] = 0.0000001
            ret_frame = np.max(images, axis=0) / std
        elif agg_type == 'Mean Amp':
            ret_frame = np.average(images, axis=0)
        elif agg_type == 'MeanAmp/SD':
            std = np.std(images, axis=0)
            std[std == 0] = 0.0000001
            ret_frame = np.average(images, axis=0) / std
        return ret_frame

    # Based on system state, create/get the frame that should be displayed.
    # index can be an integer or a list of [start:end] over which to average
    def get_display_frame(self, index=None, get_rli=False, binning=1):
        if self.get_is_livefeed_enabled():
            return self.get_livefeed_frame()[0, :, :]
        if self.core.get_show_processed_data():
            return self.core.get_processed_display_frame()
        images = None
        using_preprocessed = False
        if get_rli:
            images = self.calculate_rli()
            return images
        else:
            # fetching data
            if self.full_data_processor.enabled and self.full_data_processor.get_is_data_up_to_date():
                using_preprocessed = True
                self.acquire_processing_lock()
                images = self.get_processed_images()
            else:
                images = self.get_acqui_images()
        if images is None:
            return None
        if len(images.shape) != 3:
            print("Issue in data.py: get display frame image shape:", images.shape)
            return None

        # Temporal selection index: a single time index or a time window
        ret_frame = None
        # Apply measure window if applicable
        bg_name = self.get_background_options()[self.get_background_option_index()]
        if index is None or not self.bg_uses_frame_selector(bg_name):
            index = self.get_measure_window()
        if type(index) == int and (index < images.shape[0]) and index >= 0:
            ret_frame = images[index, :, :]
        elif type(index) == list and len(index) == 2:
            ret_frame = self.apply_temporal_aggregration_frame(images[index[0]:index[1], :, :])
        else:
            ret_frame = self.apply_temporal_aggregration_frame(images)

        if self.full_data_processor.enabled and using_preprocessed:  # can skip the minimal processing.
            self.drop_processing_lock()
            print("Showing frame from fully processed data.")
            return ret_frame

        if ret_frame is None or ret_frame.size < 1:
            return None

        # RLI division
        if self.get_is_rli_division_enabled():
            rli = self.calculate_rli()
            if rli is not None and rli.shape == ret_frame.shape:
                ret_frame = ret_frame.astype(np.float32) / rli
                ret_frame = np.nan_to_num(ret_frame)
            elif rli.shape != ret_frame.shape:
                print("RLI and data shapes don't match:",
                      rli.shape,
                      ret_frame.shape,
                      "Skipping RLI division.")

        # digital binning
        ret_frame = self.core.create_binned_data(ret_frame, binning_factor=binning)

        # spatial filtering
        ret_frame = self.core.filter_spatial(ret_frame)

        if ret_frame is None or ret_frame.size < 1:
            return None

        # crop out 1px borders
        ret_frame = ret_frame
        return ret_frame

    def get_display_fp_trace(self, fp_index):
        trial = self.get_current_trial_index()
        traces = self.get_fp_data()
        if trial is None:
            traces = np.average(traces, axis=0)
        start, end = self.get_artifact_exclusion_window()
        return Trace(traces[:, fp_index],
                     self.get_int_pts(),
                     fp_index=fp_index,
                     is_fp_trace=True,
                     start_frame=start,
                     end_frame=end)

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
            return mask
        return mask

    # returns a Trace object representing trace
    # when mask is not None, it overrides the index argument.
    # Index is a pixel specifier argument.
    # fp_index is the integer corresponding to the FP trace index
    def get_display_trace(self, index=None, fp_index=None, zoom_factor=1.0, masks=None):
        if fp_index is not None:
            tr = self.get_display_fp_trace(fp_index)
            tr.normalize(zoom_factor=zoom_factor)
            return tr

        images = self.get_acqui_images()
        if images is None:
            print("get_display_trace: No images to display.")
            return None

        ret_trace = None
        master_mask = None
        mask = None
        if masks is not None and len(masks) > 0:
            master_mask = masks[0]
            for m in masks[1:]:
                if m.shape != master_mask.shape:
                    return None  # image must have changed, region no longer valid.
                master_mask = np.logical_or(master_mask, m)
            try:
                ret_trace = images[:, master_mask]
            except IndexError:
                return None
            if ret_trace.size < 1:
                return None
            ret_trace = np.average(ret_trace, axis=1)
        elif index is None:
            return ret_trace
        elif type(index) == np.ndarray:
            if index.shape[0] == 1:
                if index[0, 1] >= images.shape[1]:
                    return None  # image must have changed, pixel no longer valid.
                if index[0, 0] >= images.shape[2]:
                    return None  # image must have changed, pixel no longer valid.
                ret_trace = images[:, index[0, 1], index[0, 0]]
            elif np.size(index) > 0:
                _, h, w = images.shape
                mask = self.get_frame_mask(h, w, index=index)
                try:
                    ret_trace = images[:, mask]
                except IndexError as e:
                    return None
                if ret_trace.size < 1:
                    return None
                ret_trace = np.average(ret_trace, axis=1)
            else:
                print("get_display_trace: drawn shape is empty:", index)

        if ret_trace is None or ret_trace.size < 1:
            return None

        start, end = self.get_artifact_exclusion_window()
        if masks is None and mask is not None:
            masks = [mask]
            master_mask = mask
        ret_trace = Trace(ret_trace,
                          self.get_int_pts(),
                          start_frame=start,
                          end_frame=end,
                          pixel_indices=index,
                          fp_index=fp_index,
                          masks=masks,
                          master_mask=master_mask)

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
            if self.validate_filter_size(filter_type, sigma_t):
                ret_trace.filter_temporal(filter_type, sigma_t)  # applies time cropping if needed

        # normalize
        ret_trace.normalize(zoom_factor=zoom_factor)

        return ret_trace

    # Return true if filter size is not too big
    def validate_filter_size(self, filter_type, sigma_t):
        n = self.get_num_pts()
        m = None
        if filter_type == 'Low Pass':  # i.e. Moving Average
            m = int(sigma_t)
        elif filter_type.startswith('Binomial-'):
            m = int(filter_type[-1]) + 1
        else:
            return True
        return n - m > m

    def get_fp_data(self):
        trial = self.get_current_trial_index()
        if trial is None:
            return self.db.load_fp_data()
        return self.db.load_trial_fp_data(trial)

    def get_acqui_images(self):
        trial = self.get_current_trial_index()
        if trial is None:
            images = self.db.load_data_raw()
            if self.get_is_trial_averaging_enabled():
                images = np.average(images, axis=0)
            return images
        else:
            return self.db.load_trial_data_raw(trial)

    def get_acqui_memory(self):
        trial = self.get_current_trial_index()
        if trial is None:
            print("data.py: get_acqui_memory: Trial index not found. Getting trial 0.")
            trial = 0
        return self.db.load_trial_all_data(trial)

    # Assumes caller is responsible for locking the processed memmapped file
    def get_processed_images(self):
        trial = self.get_current_trial_index()
        if trial is None:
            images = self.db.load_data_processed()
            if self.get_is_trial_averaging_enabled():
                images = np.average(images, axis=0)
            return images
        else:
            return self.db.load_trial_data_processed(trial)

    # not the memmapped files, but the memory used for RLI acquisition and calc
    def get_rli_images(self):
        return self.db.rli_images[0, :, :, :]

    def get_rli_memory(self):
        return self.db.rli_images[:, :, :, :]

    def set_num_pts(self, value=1, force_resize=False, prevent_resize=False, suppress_processing=True):
        if type(value) != int or value < 1:
            return
        tmp = self.get_num_pts()
        if (force_resize or tmp != value) and not prevent_resize:
            self.increment_record_until_filename_free()
        self.hardware.set_num_pts(value=value)
        self.meta.num_pts = value
        if not suppress_processing:
            self.full_data_processor.update_full_processed_data()

    # Populate meta.rli_high from RLI raw frames
    def calculate_light_rli_frame(self, margins=40, force_recalculate=False):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        rli_high = self.db.get_rli_high()
        if np.any(rli_high != 0):
            return rli_high
        n = self.get_num_rli_pts()
        rli_light_frames = self.get_rli_images()[d + margins + 1:n - 1 - margins, :, :]
        if rli_light_frames is None or rli_light_frames.shape[0] == 0:
            return None
        try:
            rli_high[:, :] = np.average(rli_light_frames, axis=0)[:, :]
        except ValueError:
            print("We're unable to calculate RLI due to invalid array size assumptions.")
        return rli_high

    def calculate_dark_rli_frame(self, margins=40, force_recalculate=False):
        d = self.hardware.get_num_dark_rli()
        while margins * 2 >= d:
            margins //= 2
        rli_low = self.db.get_rli_low()
        if np.any(rli_low != 0) and not force_recalculate:
            return rli_low
        rli_dark_frames = self.get_rli_images()[margins + 1:d - margins - 1, :, :]
        if rli_dark_frames is None or rli_dark_frames.shape[0] == 0:
            return None
        try:
            rli_low[:, :] = np.average(rli_dark_frames, axis=0)[:, :]
        except ValueError:
            print("We're unable to calculate RLI due to invalid array size assumptions.")
        return rli_low

    def calculate_max_rli_frame(self, force_recalculate=False):
        rli_max = self.db.get_rli_max()
        if np.any(rli_max != 0) and not force_recalculate:
            return rli_max
        rli_frames = self.get_rli_images()
        rli_max[:, :] = np.max(rli_frames, axis=0)[:, :]
        return rli_max

    def calculate_rli(self, force_recalculate=False):
        light = self.calculate_light_rli_frame(force_recalculate=force_recalculate)
        dark = self.calculate_dark_rli_frame(force_recalculate=force_recalculate)
        if light is None or dark is None:
            return np.zeros((self.get_display_height(),
                             self.get_display_width()),
                            dtype=np.uint16)
        diff = np.abs(light.astype(np.float32) - dark.astype(np.float32))
        diff[diff == 0] = 0.000001  # avoid div by 0
        return diff

    def set_num_dark_rli(self, dark_rli, force_resize=False, prevent_resize=False):
        tmp = self.hardware.get_num_dark_rli()
        if (force_resize or tmp != dark_rli) and not prevent_resize:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.db.rli_images, (2,
                                           self.get_num_rli_pts(),
                                           w,
                                           h))
        self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_num_light_rli(self, light_rli, force_resize=False, prevent_resize=False):
        tmp = self.hardware.get_num_light_rli()
        if (force_resize or tmp != light_rli) and not prevent_resize:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.db.rli_images, (2,  # extra mem for C++ reassembly
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
        if value:
            self.gui.freeze_hardware_settings(freeze_file_flip=False,
                                              include_buttons=False)
        else:
            self.gui.unfreeze_hardware_settings()
        self.is_loaded_from_file = value

    def get_is_analysis_only_mode_enabled(self):
        return self.db.meta.is_analysis_only_mode_enabled

    def set_is_analysis_only_mode_enabled(self, value):
        self.db.meta.is_analysis_only_mode_enabled = value

    def get_is_schedule_rli_enabled(self):
        return self.db.meta.is_schedule_rli_enabled

    def set_is_schedule_rli_enabled(self, value):
        self.db.meta.is_schedule_rli_enabled = value

    ''' Attributes controlled at Hardware level '''

    def get_int_pts(self):
        int_pts = self.hardware.get_int_pts()
        if self.get_is_loaded_from_file() or int_pts is None:
            return self.db.meta.int_pts
        return int_pts

    def get_acqui_duration(self):
        if self.get_is_loaded_from_file():
            return self.db.meta.num_pts * self.db.meta.int_pts
        return self.hardware.get_acqui_duration()

    def get_num_rli_pts(self):
        return self.hardware.get_num_dark_rli() + self.hardware.get_num_light_rli()

    def get_acqui_onset(self):
        onset = self.hardware.get_acqui_onset()
        if self.get_is_loaded_from_file() or onset is None:
            return self.db.meta.acqui_onset
        return onset

    def set_acqui_onset(self, v):
        self.meta.acqui_onset = v
        self.hardware.set_acqui_onset(acqui_onset=v)

    def get_stim_onset(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.db.meta.stim_onset[ch - 1]
            return self.hardware.get_stim_onset(channel=ch)

    def set_stim_onset(self, **kwargs):
        if kwargs['value'] is None or type(kwargs['value']) != int:
            kwargs['value'] = 0
        self.hardware.set_stim_onset(kwargs)
        self.meta.stim_onset[kwargs['channel'] - 1] = float(kwargs['value'])

    def get_stim_duration(self, ch):
        if ch == 1 or ch == 2:
            if self.get_is_loaded_from_file():
                return self.db.meta.stim_duration[ch - 1]
            return self.hardware.get_stim_duration(channel=ch)

    def set_stim_duration(self, **kwargs):
        if kwargs['value'] is None or type(kwargs['value']) != int:
            kwargs['value'] = 0
        self.hardware.set_stim_duration(kwargs)
        self.meta.stim_duration[kwargs['channel'] - 1] = float(kwargs['value'])

    def get_current_trial_index(self):
        return self.current_trial_index

    def set_current_trial_index(self, v):
        self.current_trial_index = v

    def increment_current_trial_index(self):
        if self.current_trial_index is None:
            self.current_trial_index = 0
        self.current_trial_index = min(self.current_trial_index + 1,
                                       self.get_num_trials() - 1)

    def decrement_current_trial_index(self):
        if self.current_trial_index is None:
            self.current_trial_index = 0
        self.current_trial_index = max(self.current_trial_index - 1, 0)

    def get_is_rli_division_enabled(self):
        return self.db.meta.is_rli_division_enabled

    def set_is_rli_division_enabled(self, v, suppress_processing=False):
        self.db.meta.is_rli_division_enabled = v
        if not suppress_processing:
            self.full_data_processor.update_full_processed_data()

    def get_is_data_inverse_enabled(self):
        return self.db.meta.is_data_inverse_enabled

    def set_is_data_inverse_enabled(self, v, suppress_processing=False):
        self.db.meta.is_data_inverse_enabled = v
        if not suppress_processing:
            self.full_data_processor.update_full_processed_data()

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
                self.livefeed_frame = np.zeros((4, h, w), dtype=np.uint16)
                return self.livefeed_frame

    def clear_livefeed_frame(self):
        self.livefeed_frame = None

    def set_notepad_text(self, **kwargs):
        v = kwargs['values']
        self.meta.notepad_text = v

    def get_contrast_scaling(self):
        return self.meta.contrast_scaling

    def set_contrast_scaling(self, v):
        self.meta.contrast_scaling = v

    def get_artifact_exclusion_window(self):
        return self.meta.camera_artifact_exclusion_window

    def set_artifact_exclusion_window(self, kind, ind, v):
        if ind is None:
            self.meta.camera_artifact_exclusion_window = v
        if ind in [0, 1]:
            self.meta.camera_artifact_exclusion_window[ind] = v
