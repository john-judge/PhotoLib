import numpy as np


class Data:

    def __init__(self, hardware):
        self.hardware = hardware
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
        self.program = 7
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

        self.schedule_rli_flag = True

        # Memory
        self.rli_images = None
        self.acqui_images = None
        self.fp_data = None

        # synchronize defaults into hardware
        self.set_camera_program(self.program,
                                force_resize=True)
        self.set_num_pts(num_pts=self.num_pts,
                         force_resize=True)
        self.set_num_dark_rli(dark_rli=self.dark_rli,
                              force_resize=True)
        self.set_num_light_rli(light_rli=self.light_rli,
                               force_resize=True)

        self.hardware.set_int_pts(interval=self.interval_pts)

        self.hardware.set_num_pulses(num_pulses=self.num_pulses,
                                     channel=1)
        self.hardware.set_num_pulses(num_pulses=self.num_pulses,
                                     channel=2)

        self.hardware.set_int_pulses(interval_pulses=self.interval_pulses,
                                     channel=1)
        self.hardware.set_int_pulses(interval_pulses=self.interval_pulses,
                                     channel=2)

        self.hardware.set_num_bursts(num_bursts=self.num_bursts,
                                     channel=1)
        self.hardware.set_num_bursts(num_bursts=self.num_bursts,
                                     channel=2)

        self.hardware.set_int_bursts(interval_bursts=self.interval_bursts,
                                     channel=1)
        self.hardware.set_int_bursts(interval_bursts=self.interval_bursts,
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
        self.acqui_images = np.zeros((2,
                                      self.get_num_pts(),
                                      h,
                                      w),
                                     dtype=np.uint16)
        self.fp_data = np.zeros((self.get_num_pts(), self.get_num_fp()),
                                 dtype=np.int16)

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
                                          (self.num_pts + 1),
                                          h,
                                          w))
            self.fp_data = np.resize(self.fp_data,
                                     (self.get_num_pts(), self.get_num_fp()))

        else:
            self.allocate_image_memory()

    # Based on system state, create/get the frame that should be displayed.
    # index can be an integer or a list of [start:end] over which to average
    def get_display_frame(self, index=None, get_rli=False):
        images = self.get_acqui_images()
        if get_rli:
            images = self.get_rli_images()

        if type(index) == int and (index < images.shape[0]) and index >= 0:
            return images[index, :, :]
        elif type(index) == list and len(index) == 2:
            return np.average(images[index[0]:index[1], :, :], axis=0)
        else:
            return np.average(images, axis=0)

    # Returns the full (x2) memory for hardware to use
    def get_acqui_memory(self):
        return self.acqui_images

    # Returns the full (x2) memory for hardware to use
    def get_rli_memory(self):
        return self.rli_images

    def get_fp_data(self):
        return self.fp_data

    # This pointer will not change even when we resize array
    def get_acqui_images(self):
        return self.acqui_images[0, :, :, :]

    # This pointer will not change even when we resize array
    def get_rli_images(self):
        return self.rli_images[0, :, :, :]

    def set_num_pts(self, num_pts, force_resize=False):
        tmp = self.num_pts
        self.num_pts = num_pts
        if force_resize or tmp != num_pts:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.acqui_images, (2,
                                          self.num_pts,
                                          w,
                                          h))
            self.fp_data = np.resize(self.fp_data,
                                     (self.get_num_pts(), self.get_num_fp()))
            self.hardware.set_num_pts(num_pts=num_pts)

    def set_num_dark_rli(self, dark_rli, force_resize=False):
        tmp = self.dark_rli
        self.dark_rli = dark_rli
        if force_resize or tmp != dark_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (2,
                                        self.light_rli + self.dark_rli,
                                        w,
                                        h))
            self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_num_light_rli(self, light_rli, force_resize=False):
        tmp = self.light_rli
        self.light_rli = light_rli
        if force_resize or tmp != light_rli:
            w = self.get_display_width()
            h = self.get_display_height()
            np.resize(self.rli_images, (2,
                                        self.light_rli + self.dark_rli,
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

    def get_duration(self):
        return self.hardware.get_duration()

    def get_acqui_duration(self):
        return self.hardware.get_acqui_duration()

    def get_num_pts(self):
        return self.hardware.get_num_pts()

    def get_num_rli_pts(self):
        return self.hardware.get_num_dark_rli() + self.hardware.get_num_light_rli()

    # Fixed at 4 field potential measurements with NI-USB
    @staticmethod
    def get_num_fp():
        return 4

