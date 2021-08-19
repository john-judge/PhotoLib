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
        super().__init__(Metadata())
        self.extension = '.npy'
        self.memmap_file = None
        self.open_filename = None

    def get_current_filename(self, no_path=False, extension=None):
        if extension is None:
            extension = self.extension
        path = ''
        if not no_path:
            path = self.get_save_dir()

        return self.get_filename(self.meta.current_slice,
                                 self.meta.current_location,
                                 self.meta.current_record,
                                 extension=extension,
                                 path=path)

    # https://numpy.org/doc/stable/reference/generated/numpy.memmap.html#numpy.memmap
    def load_mmap_file(self, filename=None, mode='r+'):  # w+ allows create/overwrite. mode=None for auto-choose
        if filename is None:
            if self.open_filename is not None:
                filename = self.open_filename
            filename = self.get_current_filename(no_path=False, extension=self.extension)
        else:
            self.open_filename = filename
        if mode is None:
            file_no_path = self.strip_path(filename)
            if self.file_exists(file_no_path):
                mode = "r+"
            else:
                mode = "w+"
        if mode == "w+":
            print("Creating or overwriting file:", filename)
        arr_shape = (self.meta.num_trials,
                     2,
                     self.meta.num_pts,
                     self.meta.height,
                     self.meta.width)
        self.memmap_file = np.memmap(filename,
                                     dtype=np.uint16,
                                     mode=mode,
                                     shape=arr_shape)

    def clear_or_resize_mmap_file(self):
        mode = "w+"
        self.open_filename = None
        # should avoid overwriting data by renaming current file
        if self.file_exists(self.get_current_filename(no_path=True, extension=self.extension)):
            print("File exists. Warning: data may be overwritten.")
            mode = "r+"
        self.load_mmap_file(mode=mode)

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
