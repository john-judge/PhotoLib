import time

import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore
from pyPhoto21.viewers.trace import Trace
from pyPhoto21.database.file import File
from pyPhoto21.database.metadata import Metadata

import os


class Database(File):

    def __init__(self):
        super().__init__()
        self.meta = Metadata()
        self.extension = '.npy'
        self.memmap_file = None
        self.open_filename = None

    def get_current_filename(self, no_path=False):
        fn = ''
        if self.override_filename is not None:
            return fn + self.override_filename

        path = None
        if not no_path:
            path = self.get_save_dir()
        return self.get_filename(self.meta.current_slice,
                                 self.meta.current_location,
                                 self.meta.current_record,
                                 extension=self.extension,
                                 path=path)

    # https://numpy.org/doc/stable/reference/generated/numpy.memmap.html#numpy.memmap
    def load_mmap_file(self, mode='w+'):  # w+ allows create/overwrite. Set mode=None to auto-determine
        fn = self.get_current_filename(no_path=True)
        if mode is None:
            mode = 'w+'
            if self.file_exists(fn):
                print("r+")
                mode = 'r+'
        arr_shape = (self.meta.num_trials,
                     2,
                     self.meta.num_pts,
                     self.meta.width,
                     self.meta.height)
        print("loading array of shape:", arr_shape)
        self.memmap_file = np.memmap(fn,
                                     dtype=np.uint16,
                                     mode=mode,
                                     shape=arr_shape)
        self.open_filename = fn

    def clear_or_resize_mmap_file(self):
        print("Called clear or resize memmapped file. (Please do this only as often as needed -- slow,"
              " fills up disk space.)")
        clear_filename_failed = False
        # should avoid overwriting data by renaming current file
        if self.file_exists(self.get_current_filename(no_path=True)):
            if self.meta.auto_save_enabled:
                print("Archiving file: ", self.get_current_filename())
                i = 0
                new_name = self.get_current_filename(no_path=True) + ".archive_" + self.pad_zero(i, dst_len=2)
                while self.file_exists(new_name) or i > 100:
                    i += 1
                    new_name = self.get_current_filename(no_path=True) + ".archive_" + self.pad_zero(i, dst_len=2)
                new_name = self.get_current_filename() + ".archive_" + self.pad_zero(i, dst_len=2)
                file_locked = True
                attempts_left = 10
                while file_locked and attempts_left > 0:
                    try:
                        os.rename(self.get_current_filename(), new_name)
                        file_locked = False
                    except Exception as e:
                        if attempts_left == 10:
                            print("\n\n\nCould not archive current data file: ",
                                  self.get_current_filename(),
                                  "\n\n\nActual exception:")
                            print(e)
                            print("We will try to acquire the lock for", attempts_left * 2.5, "seconds")
                        file_locked = True
                        time.sleep(2.5)
                        attempts_left -= 1
                if file_locked:
                    clear_filename_failed = True
            else:  # auto-save not enabled, delete file
                os.remove(self.get_current_filename())
                if self.file_exists(self.get_current_filename(no_path=True)):
                    print("Failed to remove file:", self.get_current_filename(no_path=True))
                    clear_filename_failed = True
        if clear_filename_failed:
            print("Failed to clear name for file. Auto-choosing free filename...")
            i = 1
            fn_base = self.get_current_filename(no_path=True)
            fn_new = fn_base + "(" + self.pad_zero(i) + ")"
            self.set_override_filename(fn_new)
            while self.file_exists(fn_new):
                i += 1
                fn_new = fn_base + "(" + self.pad_zero(i) + ")"
                self.set_override_filename(fn_new)
            print("Chose filename:", self.get_current_filename(no_path=True))
        self.load_mmap_file()

    def load_trial_data_raw(self, trial):
        return self.memmap_file[trial, 0, :, :, :]

    def load_trial_data_processed(self, trial):
        return self.memmap_file[trial, 1, :, :, :]

    def load_data_raw(self):
        return self.memmap_file[:, 0, :, :, :]

    def load_data_processed(self):
        return self.memmap_file[:, 1, :, :, :]

    # Returns the full (x2) memory for hardware to use
    def load_trial_all_data(self, trial):
        return self.memmap_file[trial, :, :, :, :]
