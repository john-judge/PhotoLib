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

        self.schedule_rli_flag = True
        self.width = None
        self.height = None
        self.rli_images = None
        self.acqui_images = None

        self.set_camera_program(self.program,
                                force_resize=True)

        # synchronize defaults into hardware
        self.hardware.set_num_pts(num_pts=self.num_pts)
        self.hardware.set_camera_program(program=self.program)
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
        self.hardware.set_num_dark_rli(dark_rli=self.dark_rli)
        self.hardware.set_num_light_rli(light_rli=self.light_rli)

    def allocate_image_memory(self):
        self.rli_images = np.zeros((self.light_rli + self.dark_rli,
                                    self.width,
                                    self.height),
                                   dtype=np.uint16)
        self.acqui_images = np.zeros((self.num_pts,
                                      self.width,
                                      self.height),
                                     dtype=np.uint16)

    def set_camera_program(self, program, force_resize=False):
        if force_resize or self.program != program:
            self.hardware.set_camera_program(program=program)
            self.program = self.hardware.get_camera_program()
            self.resize_image_memory()

    def resize_image_memory(self):
        self.width = self.get_display_width()
        self.height = self.get_display_height()
        if self.rli_images is not None and self.acqui_images is not None:
            np.resize(self.rli_images, (self.light_rli + self.dark_rli,
                                        self.width,
                                        self.height))
            np.resize(self.acqui_images, (self.num_pts,
                                          self.width,
                                          self.height))
        else:
            self.allocate_image_memory()

    # This pointer will not change even when we resize array
    def get_acqui_images(self):
        return self.acqui_images

    # This pointer will not change even when we resize array
    def get_rli_images(self):
        return self.rli_images

    def set_num_pts(self, num_pts):
        tmp = self.num_pts
        self.num_pts = num_pts
        if tmp != num_pts:
            np.resize(self.acqui_images, (self.num_pts,
                                          self.width,
                                          self.height))
            self.hardware.set_num_pts(num_pts=num_pts)

    def set_dark_rli(self, dark_rli):
        tmp = self.dark_rli
        self.dark_rli = dark_rli
        if tmp != dark_rli:
            np.resize(self.rli_images, (self.light_rli + self.dark_rli,
                                        self.width,
                                        self.height))
            self.hardware.set_num_dark_rli(dark_rli=dark_rli)

    def set_light_rli(self, light_rli):
        tmp = self.light_rli
        self.light_rli = light_rli
        if tmp != light_rli:
            np.resize(self.rli_images, (self.light_rli + self.dark_rli,
                                        self.width,
                                        self.height))
            self.hardware.set_num_light_rli(light_rli=light_rli)

    def get_display_width(self):
        return self.hardware.get_display_width()

    def get_display_height(self):
        return self.hardware.get_display_height()

    def get_duration(self):
        return self.hardware.get_duration()

    def get_acqui_duration(self):
        return  self.hardware.get_acqui_duration()



