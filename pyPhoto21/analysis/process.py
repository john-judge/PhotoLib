import time

import numpy as np
from queue import Queue


class Processor:
    """ Processes data in background """
    def __init__(self, db):
        self.db = db
        self.dirty = False  # True if processing settings have been updated
        self.is_active = False
        self.sleep_interval = 2.0
        self.stop_worker_flag = False
        self.pause = False

    def stop_processor(self):
        self.stop_worker_flag = True
        while self.stop_worker_flag:
            time.sleep(1)

    def pause_processor(self):
        self.pause = True

    def unpause_processor(self):
        self.pause = False

    # call this to signal processor to start its workflow
    # from the top
    def update_full_processed_data(self):
        self.dirty = True

    # Launch this looped worker as separate thread
    def process_continually(self):
        while not self.stop_worker_flag:
            if not self.dirty:
                self.is_active = False
                time.sleep(self.sleep_interval * 3)
            else:
                self.is_active = True
                self.process()
                self.is_active = False
                self.dirty = False

            time.sleep(self.sleep_interval)

    def get_is_data_up_to_date(self):
        return not self.is_active and not self.dirty

    def process(self):
        raw = self.db.load_data_raw()
        process = self.db.load_data_processed()

        # RLI division

        # data inversing

        # baseline correction

        # binning

        # spatial filter

        # temporal filter

