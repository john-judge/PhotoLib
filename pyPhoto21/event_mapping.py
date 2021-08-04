

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
            "Show RLI": {
                'function': gui.toggle_show_rli,
                'args': {},
            },
            "Open": {
                'function': gui.load_zda_file,
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
            'Light On Onset': {
                'function': gui.set_light_on_onset,
                'args': {},
            },
            'Light On Duration': {
                'function': gui.set_light_on_duration,
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
                'function': gui.set_stimulator_onset,
                'args': {'channel': 1},
            },
            'Stimulator #2 Onset': {
                'function': gui.set_stimulator_onset,
                'args': {'channel': 2},
            },
            'num_pulses Stim #1': {
                'function': gui.validate_and_pass,
                'args': {'channel': 1, 'call': gui.data.hardware.set_num_pulses},
            },
            'num_pulses Stim #2': {
                'function': gui.validate_and_pass,
                'args': {'channel': 2, 'call': gui.data.hardware.set_num_pulses},
            },
            'int_pulses Stim #1': {
                'function': gui.validate_and_pass,
                'args': {'channel': 1, 'call': gui.data.hardware.set_int_pulses},
            },
            'int_pulses Stim #2': {
                'function': gui.validate_and_pass,
                'args': {'channel': 2, 'call': gui.data.hardware.set_int_pulses},
            },
            'num_bursts Stim #1': {
                'function': gui.validate_and_pass,
                'args': {'channel': 1, 'call': gui.data.hardware.set_num_bursts},
            },
            'num_bursts Stim #2': {
                'function': gui.validate_and_pass,
                'args': {'channel': 2, 'call': gui.data.hardware.set_num_bursts},
            },
            'int_bursts Stim #1': {
                'function': gui.validate_and_pass,
                'args': {'channel': 1, 'call': gui.data.hardware.set_int_bursts},
            },
            'int_bursts Stim #2': {
                'function': gui.validate_and_pass,
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
        }

    def get_event_mapping(self):
        return self.event_mapping