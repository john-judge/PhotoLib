import pandas as pd
import os, glob
import struct
import numpy as np


class File:

    def __init__(self):
        self.current_slice = 0
        self.current_location = 0
        self.current_run = 0

    def increment_slice(self):
        self.current_slice += 1
        self.current_location = 0
        self.current_run = 0

    def increment_location(self):
        self.current_location += 1
        self.current_run = 0

    def increment_run(self):
        self.current_run += 1

    @staticmethod
    def pad_zero(i, dst_len=2):
        s = str(i)
        if len(s) < dst_len:
            return '0' + s
        return s

    def get_filename(self, extension='.zda'):
        return self.pad_zero(self.current_slice) + '-' + \
               self.pad_zero(self.current_location) + '-' + \
               self.pad_zero(self.current_run) + extension

    def save_to_file(self, acqui_images, rli_images):
        fn = self.get_filename()
        print("Saving to file " + fn + "...")
        # TO DO: use ZDA format.

    def load_from_file(self, filename, acqui_images_dst):
        ds = Dataset(filename)
        acqui_images = ds.get_data()
        return ds.get_rli(), ds.get_meta()


class Dataset:

    def __init__(self, filename, x_range=[0, -1], y_range=[0, -1], t_range=[0, -1]):
        self.x_range = x_range
        self.y_range = y_range
        self.t_range = t_range
        self.data, metadata, self.rli = self.read_zda_to_df(filename)
        self.filename = filename
        self.meta = metadata
        if self.meta is not None:
            self.version = metadata['version']
            self.slice_number = metadata['slice_number']
            self.location_number = metadata['location_number']
            self.record_number = metadata['record_number']
            self.camera_program = metadata['camera_program']
            self.number_of_trials = metadata['number_of_trials']
            self.interval_between_trials = metadata['interval_between_trials']
            self.acquisition_gain = metadata['acquisition_gain']
            self.points_per_trace = metadata['points_per_trace']
            self.time_RecControl = metadata['time_RecControl']
            self.reset_onset = metadata['reset_onset']
            self.reset_duration = metadata['reset_duration']
            self.shutter_onset = metadata['shutter_onset']
            self.shutter_duration = metadata['shutter_duration']
            self.stimulation1_onset = metadata['stimulation1_onset']
            self.stimulation1_duration = metadata['stimulation1_duration']
            self.stimulation2_onset = metadata['stimulation2_onset']
            self.stimulation2_duration = metadata['stimulation2_duration']
            self.acquisition_onset = metadata['acquisition_onset']
            self.interval_between_samples = metadata['interval_between_samples']
            self.raw_width = metadata['raw_width']
            self.raw_height = metadata['raw_height']

            # original dimensions
            self.original = {'raw_width': self.raw_width,
                             'raw_height': self.raw_height,
                             'points_per_trace': self.points_per_trace}

    def clip_data(self, x_range=[0, -1], y_range=[0, -1], t_range=[0, -1]):
        """ Imposes a range restriction on frames and/or traces """
        self.x_range = x_range
        self.y_range = y_range
        self.t_range = t_range

    def get_unclipped_data(self, trial=None):
        """ Returns unclipped data """

        # Reset to unclipped values
        self.meta['points_per_trace'] = self.original['points_per_trace']
        self.meta['raw_width'] = self.original['raw_width']
        self.meta['raw_height'] = self.original['raw_height']

        self.raw_width = self.meta['raw_width']
        self.raw_height = self.meta['raw_height']
        self.points_per_trace = self.meta['points_per_trace']

        if trial is not None:
            return self.data[trial, :, :, :]
        return self.data

    def get_data(self, trial=None):
        """ Returns clipped data """

        # Set to clipped values
        self.meta['points_per_trace'] = self.t_range[1] - self.t_range[0]
        self.meta['raw_width'] = self.x_range[1] - self.x_range[0]
        self.meta['raw_height'] = self.y_range[1] - self.y_range[0]

        self.raw_width = self.meta['raw_width']
        self.raw_height = self.meta['raw_height']
        self.points_per_trace = self.meta['points_per_trace']

        if trial is not None:
            return self.data[trial,
                   self.x_range[0]:self.x_range[1],
                   self.y_range[0]:self.y_range[1],
                   self.t_range[0]:self.t_range[1]]
        else:
            return self.data[:,
                   self.x_range[0]:self.x_range[1],
                   self.y_range[0]:self.y_range[1],
                   self.t_range[0]:self.t_range[1]]

    def get_meta(self):
        """ Returns metadata dictionary. Mostly for legacy behavior. """
        return self.meta

    def get_rli(self):
        """ Returns RLI data """
        return self.rli

    def get_trial_data(self, trial_no):
        """ Returns array slice for trial number """
        raise NotImplementedError
        return

    @staticmethod
    def read_zda_to_df(zda_file):
        """ Reads ZDA file to dataframe, and returns
        metadata as a dict.
        ZDA files are a custom PhotoZ binary format that must be interpreted byte-
        by-byte """
        file = open(zda_file, 'rb')
        print(zda_file)
        # data type sizes in bytes
        ch_size = 1
        sh_size = 2
        n_size = 4
        t_size = 8
        f_size = 4

        metadata = {'version': (file.read(ch_size)), 'slice_number': (file.read(sh_size)),
                    'location_number': (file.read(sh_size)), 'record_number': (file.read(sh_size)),
                    'camera_program': (file.read(n_size)), 'number_of_trials': (file.read(ch_size)),
                    'interval_between_trials': (file.read(ch_size)), 'acquisition_gain': (file.read(sh_size)),
                    'points_per_trace': (file.read(n_size)), 'time_RecControl': (file.read(t_size)),
                    'reset_onset': struct.unpack('f', (file.read(f_size)))[0],
                    'reset_duration': struct.unpack('f', (file.read(f_size)))[0],
                    'shutter_onset': struct.unpack('f', (file.read(f_size)))[0],
                    'shutter_duration': struct.unpack('f', (file.read(f_size)))[0],
                    'stimulation1_onset': struct.unpack('f', (file.read(f_size)))[0],
                    'stimulation1_duration': struct.unpack('f', (file.read(f_size)))[0],
                    'stimulation2_onset': struct.unpack('f', (file.read(f_size)))[0],
                    'stimulation2_duration': struct.unpack('f', (file.read(f_size)))[0],
                    'acquisition_onset': struct.unpack('f', (file.read(f_size)))[0],
                    'interval_between_samples': struct.unpack('f', (file.read(f_size)))[0],
                    'raw_width': (file.read(n_size)), 'raw_height': (file.read(n_size))}

        # Bytes to Python data type
        for k in metadata:
            if k in ['interval_between_samples', ] or 'onset' in k or 'duration' in k:
                pass  # any additional float processing can go here
            elif k == 'time_RecControl':
                pass  # TO DO: timestamp processing
            else:
                metadata[k] = int.from_bytes(metadata[k], "little")  # endianness

        num_diodes = metadata['raw_width'] * metadata['raw_height']

        file.seek(1024, 0)
        # RLI
        rli = {'rli_low': [int.from_bytes(file.read(sh_size), "little") for _ in range(num_diodes)],
               'rli_high': [int.from_bytes(file.read(sh_size), "little") for _ in range(num_diodes)],
               'rli_max': [int.from_bytes(file.read(sh_size), "little") for _ in range(num_diodes)]}

        raw_data = np.zeros((metadata['number_of_trials'],
                             metadata['raw_width'],
                             metadata['raw_height'],
                             metadata['points_per_trace'])).astype(int)

        for i in range(metadata['number_of_trials']):
            for jw in range(metadata['raw_width']):
                for jh in range(metadata['raw_height']):
                    for k in range(metadata['points_per_trace']):
                        pt = file.read(sh_size)
                        if not pt:
                            print("Ran out of points.", len(raw_data))
                            file.close()
                            return metadata
                        raw_data[i, jw, jh, k] = int.from_bytes(pt, "little")

        file.close()
        return raw_data, metadata, rli


class DataLoader:

    def __init__(self):
        self.all_data = {}  # maps file names to Dataset objects

    def select_data_by_keyword(self, keyword):
        """ Returns the data for the first file matching the keyword """
        for file in self.all_data:
            if keyword in file:
                return self.all_data[file]

    def load_all_zda(self, data_dir='.'):
        """ Loads all ZDA data in data_dir
            into a dictionary of dataframes and metadata """
        n_files_loaded = 0
        for dirName, subdirList, fileList in os.walk(data_dir, topdown=True):
            for file in fileList:
                file = str(dirName + "/" + file)
                if '.zda' == file[-4:]:
                    self.all_data[file] = Dataset(file)
                    n_files_loaded += 1

        print('Number of files loaded:', n_files_loaded)
        return self.all_data

    def get_dataset(self, filename):
        return self.all_data[filename]
