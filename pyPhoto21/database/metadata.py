import json


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
        self.stim_onset = [20, 20]
        self.stim_duration = [1, 1]
        self.version = 6  # Little Dave version

        self.num_fp = 4

        self.num_pts = 600
        self.int_pts = 1000 / 7500

        # Management and Automation / Workflow Settings
        self.schedule_rli_flag = True
        self.display_value_option_index = 0
        self.is_analysis_only_mode_enabled = True
        self.is_schedule_rli_enabled = False
        self.is_rli_division_enabled = True
        self.is_data_inverse_enabled = True
        self.is_trial_averaging_enabled = False
        self.notepad_text = 'Notes for this recording...'

        # TraceViewer settings
        self.measure_window = [94, 134]  # Time Window cropping applied to the temporal axis.
        # Default is to crop out frames for artifact exclusion
        self.camera_artifact_exclusion_window = [30, self.num_pts-10]

        # FrameViewer settings
        self.show_rli = False
        self.color_map_option = 0  # default jet
        self.binning = 1
        self.contrast_scaling = 1.0  # the original max intensity is kept as the saturation point, and
        #                               all values are scaled by this factor.

        # Analysis Settings
        self.baseline_correction_type_index = 1  # RLI default
        self.baseline_skip_window = [94, 134]
        self.is_temporal_filer_enabled = True
        self.temporal_filter_radius = 25.0
        self.temporal_filter_type_index = 3
        self.is_spatial_filer_enabled = True
        self.spatial_filter_sigma = 1.0
        self.background_option_index = 0

        # ROI Identification
        self.is_roi_enabled = False
