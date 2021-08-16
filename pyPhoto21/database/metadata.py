import os
import struct
import numpy as np


class Metadata:
    """ A simple class that stores hardware settings
        and analysis metadata, intended to be easily pickleable and
        easily swapped into the program
        Do not place methods here, because they will be needlessly
        pickled to file """
    def __init__(self):
        self.current_slice = 0
        self.current_location = 0
        self.current_record = 0

        # Hardware / DAQ settings
        self.num_trials = 5
        self.int_trials = 10  # ms
        self.num_records = 1
        self.int_records = 15  # seconds
        self.camera_program = 7
        self.height = 40
        self.width = 1024
        self.num_pulses = [1, 1]
        self.int_pulses = [15, 15]
        self.num_bursts = [1, 1]
        self.int_bursts = [15, 15]
        self.acqui_onset = 0
        self.stim_onset = [0, 0]
        self.stim_duration = [1, 1]
        self.version = 6  # Little Dave version

        # Larger data
        self.rli_low = None
        self.rli_high = None
        self.rli_max = None
        self.fp_data = None
        self.num_fp = None  # will default to 4 unless loaded from file

        self.num_pts = 2000
        self.int_pts = 0.5

        # Management and Automation / Workflow Settings
        self.schedule_rli_flag = False
        self.display_value_option_index = 0
        self.auto_save_enabled = True
        self.schedule_rli_enabled = False
        self.is_rli_division_enabled = True
        self.is_data_inverse_enabled = True

        # Time Window cropping
        self.crop_window = [0, -1]

        # TraceViewer settings

        # FrameViewer settings

        # Analysis Settings

