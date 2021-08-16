import time

import numpy as np


# Exports processed data. Uses core for background processing,
# checking that data is finished before exporting.
class Exporter:

    def __init__(self, core):
        self.core = core

    # blocks until processor daemon has stopped
    def acquire_processing_lock(self):
        while not self.core.full_data_processor.get_is_data_up_to_date():
            time.sleep(.1)
        # don't need atomic operations -- this will be called from main thread,
        # which is the only thread that should be queuing processing tasks.
        self.core.full_data_processor.pause_processor()

    def drop_processing_lock(self):
        self.core.full_data_processor.unpause_processor()


