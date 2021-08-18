import time

import numpy as np
from queue import Queue

from pyPhoto21.viewers.trace import Trace


# Reasons to make disabled: disrupts acqui threads, sync flags aren't good enough probably due
# to file linearization, and benefit is minimal anyway.

# However, we will give user a button to process fully.
class Processor:
    """ Processes data in background """
    def __init__(self, data):
        self.data = data
        self.dirty = False  # True if processing settings have been updated
        self.is_active = False
        self.sleep_interval = 2.0
        self.stop_worker_flag = False
        self.pause = False
        self.t_linspace = None

        self.enabled = False  # set to False to disable background processing permanently

    def stop_processor(self):
        self.stop_worker_flag = True
        while self.stop_worker_flag:
            time.sleep(1)

    def pause_processor(self):
        if not self.enabled:
            return
        self.pause = True

    def unpause_processor(self):
        self.pause = False

    # call this to signal processor to start its workflow
    # from the top
    def update_full_processed_data(self):
        if not self.enabled:
            return
        self.dirty = True

    # Launch this looped worker as separate thread
    def process_continually(self):
        if not self.enabled:
            return
        while not self.stop_worker_flag:
            if not self.dirty or self.pause:
                if self.pause:
                    time.sleep(self.sleep_interval * 3)
            else:
                self.is_active = True
                start = time.time()
                self.process()
                end = time.time()
                self.dirty = False
                print("Processor daemon spent ", end - start, "seconds in a processing run.")

            self.is_active = False
            time.sleep(self.sleep_interval)

        self.stop_worker_flag = False
        self.dirty = False
        self.is_active = False
        self.pause = False
        print("Processor daemon has exited.")

    def get_is_data_up_to_date(self):
        return not self.is_active and not self.dirty and self.enabled

    def get_is_active(self):
        return self.is_active and self.enabled

    # check for pause, stop, or dirty flags. Returns true if should abort processing.
    def check_flags_while_active(self):
        return self.dirty or self.stop_worker_flag or self.pause  # True -> abort job and start over

    def process(self):
        raw = self.data.db.load_data_raw()
        process = self.data.db.load_data_processed()
        process[:, :, :, :] = raw[:, :, :, :]
        if self.enabled and self.check_flags_while_active():
            return

        if not self.enabled:
            print("RLI division...")
        # RLI division
        if self.data.get_is_rli_division_enabled():
            rli_frame = self.data.calculate_rli()
            if rli_frame is not None and rli_frame.shape[-2] == process.shape[-2] \
                and rli_frame.shape[-1] == process.shape[-1]:
                h, w = rli_frame.shape
                rli_frame = rli_frame.reshape(1, 1, h, w)
                process = process.astype(np.float32) / rli_frame
                process = np.nan_to_num(process.astype(np.int32))
                min_val = np.min(process)
                if min_val < 0:
                    process -= min_val  # make everything positive
            elif rli_frame is not None:
                print("Processor daemon: RLI and data shapes don't match. Skipping"
                      " RLI division.")
        if self.enabled and self.check_flags_while_active():
            return

        if not self.enabled:
            print("binning...")
        # binning
        if self.data.meta.binning > 1:
            bin_shape = (1, 1, self.data.meta.binning, self.data.meta.binning)
            process = self.data.core.create_binned_data_ndim(process, bin_shape)
        if self.enabled and self.check_flags_while_active():
            return

        if not self.enabled:
            print("Inversing...")
        # data inversing
        if self.data.get_is_data_inverse_enabled():
            process = 0 - process
        if self.enabled and self.check_flags_while_active():
            return

        # spatial filter
        process = self.data.core.filter_spatial_4dim(process)

        """ This section takes way too long to run for all traces. """
        # if not self.enabled:
        #     print("baseline correction...")
        # # baseline correction AND temporal filter AND any t-window cropping if it supersedes
        # start = 0
        # end = process.shape[1]
        # fit_type = self.data.core.get_baseline_correction_options()[self.data.core.get_baseline_correction_type_index()]
        # skip_window = self.data.core.get_baseline_skip_window()
        # for tr in range(process.shape[0]):
        #     for h in range(process.shape[2]):
        #         for w in range(process.shape[3]):
        #             trace = Trace(process[tr, :, h, w],
        #                           self.data.get_int_pts())
        #             trace.baseline_correct_noise(fit_type, skip_window)  # baseline correction
        #             if self.data.core.get_is_temporal_filter_enabled():  # temporal filtering
        #                 filter_type = self.data.core.get_temporal_filter_options()[self.data.core.get_temporal_filter_index()]
        #                 sigma_t = self.data.core.get_temporal_filter_radius()
        #                 trace.filter_temporal(filter_type, sigma_t)  # applies time cropping if needed
        #             trace.clip_time_window(self.data.meta.crop_window)  # apply additional cropping while we're here
        #             process[tr, :, h, w] = trace.get_data_unclipped()
        #             start = max(start, trace.get_start_point())
        #             end = min(end, trace.get_end_point())
        # if self.enabled and self.check_flags_while_active():
        #     return

        # We track final time cropping this way:
        # self.data.meta.crop_window = [start, end]
        # self.t_linspace = self.get_linspace(start, end)


    def get_linspace(self, start, end):
        int_pts = self.data.get_int_pts()
        t = np.linspace(start * end,
                        end * int_pts,
                        end - start)
        return t
