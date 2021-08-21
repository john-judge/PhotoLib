import os
import struct
import numpy as np

import bz2
import _pickle as cPickle


class File:

    def __init__(self, meta):
        self.save_dir = os.getcwd()
        self.meta = meta

    def set_save_dir(self, directory):
        self.save_dir = directory

    def get_save_dir(self):
        if self.save_dir is None:
            return os.getcwd()
        return self.save_dir

    def file_exists(self, filename):
        return filename in self.get_filenames_in_folder()

    def get_filenames_in_folder(self):
        return os.listdir(self.get_save_dir())

    def get_filename(self, slice_num, location_num, record_num, extension, path=None):

        fn = self.pad_zero(slice_num) + '-'  \
            + self.pad_zero(location_num) + '-' \
            + self.pad_zero(record_num) + extension
        if not fn.endswith(extension):
            fn = fn.split(".")[0] + extension
        if path is None or len(path) == 0:
            return fn
        return path + '\\' + fn

    @staticmethod
    def pad_zero(i, dst_len=2):
        s = str(i)
        if len(s) < dst_len:
            return '0' + s
        return s

    @staticmethod
    def retrieve_python_object_from_pickle(filename):
        try:
            data = bz2.BZ2File(filename, 'rb')
            return cPickle.load(data)
        except Exception as e:
            print("could not load file:", filename)
            print(e)

    @staticmethod
    def dump_python_object_to_pickle(filename, obj):
        with bz2.BZ2File(filename, 'w') as f:
            cPickle.dump(obj, f)

    @staticmethod
    def strip_path(filename):
        return filename.split("/")[-1].split("\\")[-1]

    @staticmethod
    def decompose_filename(filename):
        parts = filename.split('.')
        if len(parts) != 2:
            return []
        filename, ext = parts
        if ext not in ['npy', 'pbz2']:
            return []
        parts = filename.split('-')
        if len(parts) != 3:
            return [filename, ext]
        if all([i.isnumeric() for i in parts]):
            return [int(i) for i in parts] + [ext]
        return [filename, ext]
