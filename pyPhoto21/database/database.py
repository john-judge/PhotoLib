import numpy as np

from pyPhoto21.database.file import File
from pyPhoto21.database.metadata import Metadata


class Database(File):

    def __init__(self):
        super().__init__(Metadata())
        self.extension = '.npy'
        self.memmap_file = None
        self.open_filename = None
        self.rli_images = None

    def get_current_filename(self, no_path=False, extension=None):
        if self.open_filename is not None:
            fn = self.open_filename
            if no_path:
                fn = self.strip_path(fn)
            fn, _ = fn.split('.')
            if extension is not None:
                fn += extension
            return fn
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
    def load_mmap_file(self, meta=None, filename=None, mode='r+', reshape_rli_buffer=True):  # w+ allows create/overwrite. mode=None for auto-choose
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
        if meta is None:
            meta = self.meta

        arr_shape = (meta.num_trials,
                     2,
                     meta.num_pts + 3,  # the extra 3 frames on the first trial are used for RLI
                     meta.height + 1,  # the extra row on each image stores the FP data for all FPs
                     meta.width)
        if mode == "w+":
            print("Creating file:", filename,
                  " with shape", arr_shape,
                  "(Size: ", np.prod(arr_shape) // 1024 * 16, "KB)")
        try:
            self.memmap_file = np.memmap(filename,
                                         dtype=np.uint16,
                                         mode=mode,
                                         shape=arr_shape)
        except OSError as e:
            print("Error while allocating disk space:", str(e))
            if "Disk" in str(e) or "space" in str(e):
                print("\n Please free up disk space to continue.")
            else:
                print("\nLikely, invalid assumptions were made about this file's existence")

        if reshape_rli_buffer:
            num_rli = self.rli_images.shape[1]
            rli_mem_shape = (2, num_rli, meta.height, meta.width)
            self.rli_images = np.zeros(rli_mem_shape,
                                       dtype=np.uint16)

    def clear_or_resize_mmap_file(self):
        self.open_filename = None
        # should avoid overwriting data by renaming current file
        if self.file_exists(self.get_current_filename(no_path=True, extension=self.extension)) and \
                not self.is_current_data_file_empty():
            print("File exists and contains nonzero data. Warning: data may be overwritten.")
        self.load_mmap_file(mode=None)

    def load_trial_data_raw(self, trial):
        return self.memmap_file[trial, 0, :-3, :-1, :]

    def load_trial_data_processed(self, trial):
        return self.memmap_file[trial, 1, :-3, :-1, :]

    def load_data_raw(self):
        return self.memmap_file[:, 0, :-3, :-1, :]

    def load_data_processed(self):
        return self.memmap_file[:, 1, :-3, :-1, :]

    def load_rli_data(self):
        return self.memmap_file[0, 0, -3:, :-1, :]  # shape (3, height, width)

    def get_rli_low(self):
        return self.memmap_file[0, 0, -3, :-1, :]  # shape (height, width)

    def get_rli_high(self):
        return self.memmap_file[0, 0, -2, :-1, :]  # shape (height, width)

    def get_rli_max(self):
        return self.memmap_file[0, 0, -1, :-1, :]  # shape (height, width)

    def load_fp_data(self):
        return self.memmap_file[:, 0, :-3, -1, :self.meta.num_fp]  # shape (num_trials, num_pts, num_fp)

    def load_trial_fp_data(self, trial):
        return self.memmap_file[trial, 0, :-3, -1, :self.meta.num_fp]  # shape (num_pts, num_fp)

    # Returns the full (x2) IMAGE memory for hardware to use
    def load_trial_all_data(self, trial):
        return self.memmap_file[trial, :, :-3, :-1, :]

    def is_current_data_file_empty(self):
        self.load_mmap_file(filename=self.get_current_filename(extension=self.extension), mode=None)
        return np.all(self.memmap_file == 0)  # all zeros
