import os
import struct
import numpy as np

import bz2
import _pickle as cPickle


class File:

    def __init__(self):
        self.save_dir = os.getcwd()
        self.override_filename = None

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
        if self.override_filename is not None:
            return self.override_filename
        fn = self.pad_zero(slice_num) + '-'  \
            + self.pad_zero(location_num) + '-' \
            + self.pad_zero(record_num) + extension
        if path is None:
            return fn
        return path + '/' + fn

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
    def dump_python_object_to_pickle(filename, obj, extension='.pbz2'):
        if len(extension) > 0 and extension[0] != '.':
            extension = '.' + extension
        with bz2.BZ2File(filename, 'w') as f:
            cPickle.dump(obj, f)

    def set_override_filename(self, fn):
        self.override_filename = fn
