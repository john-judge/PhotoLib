class EventMapping:

    def __init__(self, gui):
        self.event_mapping = {
            'Record': {
                'function': gui.record,
                'args': {}
            },
            'Take RLI': {
                'function': gui.take_rli,
                'args': {}
            },
            'Save': {
                'function': gui.file.save_to_compressed_file,
                'args': {}
            },
            'Auto Save': {
                'function': gui.toggle_auto_save,
                'args': {}
            },
            'Auto RLI': {
                'function': gui.toggle_auto_rli,
                'args': {}
            },
            'Launch Hyperslicer': {
                'function': gui.launch_hyperslicer,
                'args': {},
            },
            "-CAMERA PROGRAM-": {
                'function': gui.set_camera_program,
                'args': {},
            },
            "Select Background": {
                'function': gui.set_background_option_index,
                'args': {},
            },
            "Show RLI": {
                'function': gui.toggle_show_rli,
                'args': {},
            },
            "Open": {
                'function': gui.load_data_file,
                'args': {},
            },
            '-github-': {
                'function': gui.launch_github_page,
                'args': {},
            },
            'Digital Binning': {
                'function': gui.set_digital_binning,
                'args': {},
            },
            "Choose Save Directory": {
                'function': gui.choose_save_dir,
                'args': {},
            },
            'Acquisition Onset': {
                'function': gui.set_acqui_onset,
                'args': {},
            },
            'Acquisition Duration': {
                'function': gui.set_acqui_duration,
                'args': {},
            },
            'Stimulator #1 Onset': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_stim_onset},
            },
            'Stimulator #2 Onset': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_stim_onset},
            },
            'Stimulator #1 Duration': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_stim_duration},
            },
            'Stimulator #2 Duration': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_stim_duration},
            },
            'num_pulses Stim #1': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_num_pulses},
            },
            'num_pulses Stim #2': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_num_pulses},
            },
            'int_pulses Stim #1': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_int_pulses},
            },
            'int_pulses Stim #2': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_int_pulses},
            },
            'num_bursts Stim #1': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_num_bursts},
            },
            'num_bursts Stim #2': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_num_bursts},
            },
            'int_bursts Stim #1': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 1, 'call': gui.data.hardware.set_int_bursts},
            },
            'int_bursts Stim #2': {
                'function': gui.validate_and_pass_channel,
                'args': {'channel': 2, 'call': gui.data.hardware.set_int_bursts},
            },
            "ROI Identifier Config": {
                'function': gui.launch_roi_settings,
                'args': {},
            },
            "Identify ROI": {
                'function': gui.enable_roi_identification,
                'args': {}
            },
            'Pixel-wise SNR cutoff Raw': {
                'function': gui.set_cutoff,
                'args': {'form': 'raw',
                         'kind': 'pixel'}
            },
            'Pixel-wise SNR cutoff Percentile': {
                'function': gui.set_cutoff,
                'args': {'form': 'percentile',
                         'kind': 'pixel'}
            },
            'Cluster-wise SNR cutoff Raw': {
                'function': gui.set_cutoff,
                'args': {'form': 'raw',
                         'kind': 'cluster'}
            },
            'Cluster-wise SNR cutoff Percentile': {
                'function': gui.set_cutoff,
                'args': {'form': 'percentile',
                         'kind': 'cluster'}
            },
            'ROI-wise SNR cutoff Raw': {
                'function': gui.set_cutoff,
                'args': {'form': 'raw',
                         'kind': 'roi_snr'}
            },
            'ROI-wise SNR cutoff Percentile': {
                'function': gui.set_cutoff,
                'args': {'form': 'percentile',
                         'kind': 'roi_snr'}
            },
            'ROI-wise Amplitude cutoff Raw': {
                'function': gui.set_cutoff,
                'args': {'form': 'raw',
                         'kind': 'roi_amplitude'}
            },
            'ROI-wise Amplitude cutoff Percentile': {
                'function': gui.set_cutoff,
                'args': {'form': 'percentile',
                         'kind': 'roi_amplitude'}
            },
            "Time Window Start frames pre_stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 0,
                         'kind': 'pre_stim',
                         'form': 'frames'}
            },
            "Time Window Start (ms) pre_stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 0,
                         'kind': 'pre_stim',
                         'form': 'ms'}
            },
            "Time Window End frames pre_stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 1,
                         'kind': 'pre_stim',
                         'form': 'frames'}
            },
            "Time Window End (ms) pre_stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 1,
                         'kind': 'pre_stim',
                         'form': 'ms'}
            },
            "Time Window Start frames Stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 0,
                         'kind': 'stim',
                         'form': 'frames'}
            },
            "Time Window Start (ms) stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 0,
                         'kind': 'stim',
                         'form': 'ms'}
            },
            "Time Window End frames stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 1,
                         'kind': 'stim',
                         'form': 'frames'}
            },
            "Time Window End (ms) stim": {
                'function': gui.set_roi_time_window,
                'args': {'index': 1,
                         'kind': 'stim',
                         'form': 'ms'}
            },
            "roi.k_clusters": {
                'function': gui.set_roi_k_clusters,
                'args': {}
            },
            "View Silhouette Plot": {
                'function': gui.view_roi_plot,
                'args': {'type': 'silhouette'}
            },
            "View Elbow Plot": {
                'function': gui.view_roi_plot,
                'args': {'type': 'elbow'}
            },
            "Load ROI Data from File": {
                'function': gui.load_roi_file,
                'args': {'type': 'elbow'}
            },
            "Save ROI Data to File": {
                'function': gui.save_roi_file,
                'args': {'type': 'elbow'}
            },
            'num_trials': {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.set_num_trials, 'max_val': 20},
            },
            'int_trials': {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.set_int_trials},
            },
            'Number of Points': {
                'function': gui.set_num_pts,
                'args': {}
            },
            'num_records': {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.set_num_records, 'max_val': 20},
            },
            'int_records': {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.set_int_records},
            },
            'Unload File': {
                'function': gui.unload_file,
                'args': {}
            },
            'File Name': {
                'function': gui.change_save_filename,
                'args': {}
            },
            "Increment Trial": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.data.increment_current_trial_index,
                         'call2': gui.update_tracking_num_fields}
            },
            "Decrement Trial": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.data.decrement_current_trial_index,
                         'call2': gui.update_tracking_num_fields}
            },
            "Increment Record": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.increment_record,
                         'call2': gui.update_tracking_num_fields}
            },
            "Decrement Record": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.decrement_record,
                         'call2': gui.update_tracking_num_fields}
            },
            "Increment Location": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.increment_location,
                         'call2': gui.update_tracking_num_fields}
            },
            "Decrement Location": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.decrement_location,
                         'call2': gui.update_tracking_num_fields}
            },
            "Increment Slice": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.increment_slice,
                         'call2': gui.update_tracking_num_fields}
            },
            "Decrement Slice": {
                'function': gui.pass_no_arg_calls,
                'args': {'call': gui.file.decrement_slice,
                         'call2': gui.update_tracking_num_fields}
            },
            "Trial Number": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.set_current_trial_index,
                         'call2': gui.update_tracking_num_fields}
            },
            "Location Number": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.set_location,
                         'call2': gui.update_tracking_num_fields}
            },
            "Record Number": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.set_record,
                         'call2': gui.update_tracking_num_fields}
            },
            "Slice Number": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.set_slice,
                         'call2': gui.update_tracking_num_fields}
            },
            "STOP!": {
                'function': gui.hardware.set_stop_flag,
                'args': {}
            },
            "Temporal Filter Radius": {
                'function': gui.set_t_filter_radius,
                'args': {}
            },
            "Spatial Filter Sigma": {
                'function': gui.set_s_filter_sigma,
                'args': {}
            },
            "Select Temporal Filter": {
                'function': gui.set_temporal_filter_index,
                'args': {},
            },
            'T-Filter': {
                'function': gui.set_is_t_filter_enabled,
                'args': {}
            },
            'S-Filter': {
                'function': gui.set_is_s_filter_enabled,
                'args': {}
            },
            "Select Baseline Correction": {
                'function': gui.set_baseline_correction,
                'args': {}
            },
            "Baseline Skip Window Size": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.core.set_skip_window_start}
            },
            "Baseline Skip Window Start": {
                'function': gui.validate_and_pass_int,
                'args': {'call': gui.data.core.set_skip_window_size}
            },
            "Reset Cam": {
                'function': gui.hardware.reset_camera,
                'args': {}
            },
            'RLI Division': {
                'function': gui.set_rli_division,
                'args': {}
            },
            'Data Inverse': {
                'function': gui.set_data_inverse,
                'args': {}
            },
            'Live Feed': {
                'function': gui.start_livefeed,
                'args': {}
            },
            "Select Display Value": {
                'function': gui.set_display_value_option_index,
                'args': {},
            }
        }

    def get_event_mapping(self):
        return self.event_mapping
