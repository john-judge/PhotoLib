import os
import bz2
import _pickle as cPickle
import json
from json import JSONEncoder

import numpy as np


class FileEncoder(JSONEncoder):
    def default(self, o):
        # if type(o) == np.ndarray:
        #     return o.tolist()
        return o.__dict__


class File:

    def __init__(self, meta):
        self.save_dir = os.getcwd()
        self.meta = meta
        self.save_dir_was_changed = False

    def is_save_dir_default(self):
        return not self.save_dir_was_changed

    def set_save_dir(self, directory):
        self.save_dir = directory
        self.save_dir_was_changed = True

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
    def retrieve_python_object_from_json(filename):
        try:
            f = open(filename)
            obj = json.load(f)
            if type(obj) == str:
                obj = json.loads(obj)
            f.close()
            return obj
        except Exception as e:
            print("could not load file:", filename)
            print(e)

    @staticmethod
    def dump_python_object_to_json(filename, obj):
        print("Saving metadata ", filename)
        with open(filename, 'w') as f:
            json.dump(obj, f, cls=FileEncoder)

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
        if ext not in ['npy', 'json']:
            return []
        parts = filename.split('-')
        if len(parts) != 3:
            return [filename, ext]
        if all([i.isnumeric() for i in parts]):
            return [int(i) for i in parts] + [ext]
        return [filename, ext]
