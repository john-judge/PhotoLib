import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore


class Data:

    def __init__(self, hardware):
        self.hardware = hardware
        self.num_trials = 5
        self.int_trials = 10  # seconds
        self.light_rli = 200
        self.dark_rli = 280
        self.num_pts = 2000
        self.interval_pts = 0.5
        self.num_pulses = 5
        self.interval_pulses = 15
        self.num_bursts = 5
        self.interval_bursts = 15
        self.duration = 200
        self.acqui_onset = 50
        self.acqui_duration = 267
        self.program = 7
        self.num_fp_pts = 4
        self.light_on_onset = 0
        self.light_on_duration = 1200
        self.stimulator_onset = {
            1: 300,
            2: 300,
        }
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

        self.schedule_rli_flag = False  # TO DO: include take RLI and division
        self.auto_save_data = False

        # Analysis
        self.core = AnalysisCore()

        # Memory
        self.rli_images = None
        self.acqui_images = None
        self.fp_data = None
        self.is_loaded_from_file = False

        # synchronize defaults into hardware
        self.set_camera_program(self.program,
                                force_resize=True)
        self.set_num_pts(num_pts=self.num_pts,
                         force_resize=True)
        self.set_num_dark_rli(dark_rli=self.dark_rli,
                              force_resize=True)
        self.set_num_light_rli(light_rli=self.light_rli,
                               force_resize=True)

        self.hardware.set_num_pulses(value=self.num_pulses,
                                     channel=1)
        self.hardware.set_num_pulses(value=self.num_pulses,
                                     channel=2)

        self.hardware.set_int_pulses(value=self.interval_pulses,
                                     channel=1)
        self.hardware.set_int_pulses(value=self.interval_pulses,
                                     channel=2)

        self.hardware.set_num_bursts(value=self.num_bursts,
                                     channel=1)
        self.hardware.set_num_bursts(value=self.num_bursts,
                                     channel=2)

        self.hardware.set_int_bursts(value=self.interval_bursts,
                                     channel=1)
        self.hardware.set_int_bursts(value=self.interval_bursts,
                                     channel=2)

        self.hardware.set_schedule_rli_flag(schedule_rli_flag=self.schedule_rli_flag)
        self.hardware.set_acqui_onset(acqui_onset=self.acqui_onset)

    # We allocate twice the memory since C++ needs room for CDS subtraction
    def allocate_image_memory(self):
        w = self.get_display_width()
        h = self.get_display_height()
        self.rli_images = np.zeros((2,
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
                                dtype=np.float64)

    def set_camera_program(self, program, force_resize=False):
        if force_resize or self.program != program:
            self.program = program
            self.hardware.set_camera_program(program=program)
            self.resize_image_memory()

    def get_camera_program(self):
        return self.program

    def resize_image_memory(self):
        w = self.get_display_width()
        h = self.get_display_height()
        if self.rli_images is not None and self.acqui_images is not None:

            self.rli_images = np.resize(self.rli_images, (2,
                                        (self.light_rli + self.dark_rli + 1),
                                        h,
                                        w))
            self.acqui_images = np.resize(self.acqui_images, (2,
                                          self.get_num_trials(),
                                          (self.num_pts + 1),
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
    def get_display_frame(self, index=None, trial=None, get_rli=False, show_processed=False, binning=1):
        if show_processed:
            return self.core.get_processed_display_frame()
        if get_rli:
            images = self.get_rli_images()
        else:
            images = self.get_acqui_images(trial=trial)
        if images is None:
            return None
        print("get display frame image shape:", images.shape)

        ret_frame = None
        if type(index) == int and (index < images.shape[0]) and index >= 0:
            ret_frame = images[index, :, :]
        elif type(index) == list and len(index) == 2:
            ret_frame = np.average(images[index[0]:index[1], :, :], axis=0)
        else:
            ret_frame = np.average(images, axis=0)

        # digital binning
        ret_frame = self.core.create_binned_data(ret_frame, binning_factor=binning)

        # crop out 1px borders
        ret_frame = ret_frame[1:-2, 1:-2]
        return ret_frame

    def get_display_trace(self, index=None, trial=None):
        images = self.get_acqui_images()
        if images is None:
            print("get_display_trace: No images to display.")
            return None
        if trial is None:
            images = np.average(images, axis=0)
        else:
            images = images[trial, :, :]

        ret_trace = None
        if index is None:
            return ret_trace
        elif type(index) == np.ndarray:
            if index.shape[0] == 1:
                ret_trace = images[:, index[0, 1], index[0, 0]]
            elif np.size(index) > 0:
                _, h, w = images.shape
                x, y = np.meshgrid(np.arange(w), np.arange(h))  # make a canvas with coordinates
                x, y = x.flatten(), y.flatten()
                points = np.vstack((x, y)).T

                p = Path(index, closed=False)
                mask = p.contains_points(points).reshape(h, w)  # mask of filled in path
                index = np.where(mask)
                if np.size(index) < 1:
                    print("get_display_trace: filled shape is empty:", index)
                    return None
                ret_trace = images[:, mask]
                ret_trace = np.average(ret_trace, axis=1)
            else:
                print("get_display_trace: drawn shape is empty:", index)
        return ret_trace

    # Returns the full (x2) memory for hardware to use
    def get_acqui_memory(self, trial=None):
        if trial is None:
            return self.acqui_images
        return self.acqui_images[trial, :, :, :, :]

    # Returns the full (x2) memory for hardware to use
    def get_rli_memory(self, trial=None):
        if trial is None:
            return self.rli_images
        return self.rli_images[trial, :, :, :, :]

    def get_fp_data(self, trial=None):
        if self.fp_data is None:
            self.fp_data = np.zeros((self.get_num_trials(),
                                     self.get_num_pts(),
                                     self.get_num_fp()),
                                    dtype=np.float64)
        if trial is None:
            return self.fp_data
        return self.fp_data[trial, :, :]

    def get_acqui_images(self, trial=None):
        if self.get_is_loaded_from_file():
            if trial is None:
                return self.acqui_images[:, :, :, :]
            else:
                return self.acqui_images[trial, :, :, :]

        if trial is None:
            return self.acqui_images[:, 0, :, :, :]
        else:
            return self.acqui_images[trial, 0, :, :, :]

    def get_rli_images(self):
        if self.get_is_loaded_from_file():
            return self.rli_images[:, :, :]
        return self.rli_images[0, :, :, :]

    def clear_data_memory(self):
        # deleting the refs may trigger garbage collection (ideally)
        del self.acqui_images
        del self.rli_images
        del self.fp_data
        self.rli_images = None
        self.acqui_images = None
        self.fp_data = None

    # trial is ignored if data is from file
    def set_acqui_images(self, data, trial=None, from_file=False):
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
        self.set_is_loaded_from_file(from_file)
        if from_file:
            self.rli_images = data
        else:
            self.rli_images[0, :, :, :] = data[:, :, :]

    def set_fp_data(self, data):
        self.fp_data = data

    def set_num_pts(self, num_pts, force_resize=False):
        tmp = self.num_pts
        self.num_pts = num_pts
        if force_resize or tmp != num_pts:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.acqui_images, (self.get_num_trials(),
                                          2, # extra mem for C++ reassembly
                                          self.get_num_pts(),
                                          w,
                                          h))
            self.fp_data = np.resize(self.fp_data,
                                     (self.get_num_trials(),
                                      self.get_num_fp(),
                                      self.get_num_pts()))
            self.hardware.set_num_pts(value=num_pts)

    def set_num_dark_rli(self, dark_rli, force_resize=False):
        tmp = self.dark_rli
        self.dark_rli = dark_rli
        if force_resize or tmp != dark_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.light_rli + self.dark_rli,
                                        2,  # extra mem for C++ reassembly
                                        w,
                                        h))
            self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_num_light_rli(self, light_rli, force_resize=False):
        tmp = self.light_rli
        self.light_rli = light_rli
        if force_resize or tmp != light_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (self.light_rli + self.dark_rli,
                                        2,  # extra mem for C++ reassembly
                                        w,
                                        h))
            self.hardware.set_num_light_rli(light_rli=light_rli)

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this width
    def get_display_width(self):
        return self.display_widths[self.get_camera_program()]

    # This is the allocated memory size, not necessarily the current camera state
    # However, the Hardware class should be prepared to init camera to this height
    def get_display_height(self):
        return self.display_heights[self.get_camera_program()]

    def get_stim_onset(self, channel):
        if channel == 1 or channel == 2:
            return self.stimulator_onset[channel]

    def set_stim_onset(self, channel, value):
        if channel == 1 or channel == 2:
            self.stimulator_onset[channel] = value

    def get_duration(self):
        return self.duration

    def get_acqui_duration(self):
        return self.acqui_duration

    def get_num_pts(self):
        return self.num_pts

    def get_num_rli_pts(self):
        return self.dark_rli + self.light_rli

    def get_int_pts(self):
        return self.interval_pts

    def get_num_fp(self):
        if self.get_is_loaded_from_file():
            return self.num_fp_pts
        return 4  # Little Dave: Fixed at 4 field potential measurements with NI-USB

    def get_num_pulses(self, ch):
        return self.num_pulses

    def set_num_fp(self, value):
        self.num_fp_pts = value

    def get_num_trials(self):
        return self.num_trials

    def set_num_trials(self, num_trials):
        self.num_trials = num_trials

    def get_int_trials(self):
        return self.int_trials

    def set_int_trials(self, int_trials):
        self.int_trials = int_trials

    def get_is_loaded_from_file(self):
        return self.is_loaded_from_file

    def set_is_loaded_from_file(self, value):
        self.is_loaded_from_file = value

    def set_light_on_onset(self, v):
        self.light_on_onset = v

    def set_light_on_duration(self, v):
        self.light_on_duration = v

    def set_acqui_onset(self, v):
        self.acqui_onset = v

    def set_acqui_duration(self, v):
        self.acqui_duration = v

    def get_acqui_onset(self):
        return self.acqui_onset

