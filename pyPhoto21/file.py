import os
import struct
import numpy as np

import bz2
import _pickle as cPickle


class File:

    def __init__(self, data):
        self.data = data
        self.save_dir = os.getcwd()
        self.override_filename = None
        self.current_slice = 0
        self.current_location = 0
        self.current_record = 0

    # gets a filename to save to, avoiding overwrites
    def get_filename_to_write(self, extension='.pbz2'):
        fn = self.get_filename()
        while self.file_exists(fn):
            self.set_override_filename(None)
            self.increment_record()
            fn = self.get_filename(extension=extension)
        return fn

    def file_exists(self, filename):
        return filename in self.get_filenames_in_folder()

    def get_filenames_in_folder(self):
        return os.listdir(self.get_save_dir())

    def set_override_filename(self, fn):
        self.override_filename = fn

    def get_slice_num(self):
        return self.current_slice

    def get_location_num(self):
        return self.current_location

    def get_record_num(self):
        return self.current_record

    def increment_slice(self, num=1):
        self.current_slice += num
        self.current_location = 0
        self.current_record = 0
        self.data.set_current_trial_index(0)

    def increment_location(self, num=1):
        self.current_location += num
        self.current_record = 0
        self.data.set_current_trial_index(0)

    def increment_record(self, num=1):
        self.current_record += num
        self.data.set_current_trial_index(0)

    def decrement_slice(self, num=1):
        self.current_slice -= num
        if self.current_slice >= 0:
            self.current_location = 0
            self.current_record = 0
            self.data.set_current_trial_index(0)
        else:
            self.current_slice = 0

    def decrement_location(self, num=1):
        self.current_location -= num
        if self.current_location >= 0:
            self.current_record = 0
            self.data.set_current_trial_index(0)
        else:
            self.current_location = 0

    def decrement_record(self, num=1):
        self.current_record -= 1

    def set_slice(self, v):
        if v > self.current_slice:
            self.increment_slice(v - self.current_slice)
        elif v < self.current_slice:
            self.decrement_slice(self.current_slice - v)

    def set_record(self, v):
        if v > self.current_record:
            self.increment_record(v - self.current_record)
        elif v < self.current_record:
            self.decrement_record(self.current_record - v)

    def set_location(self, v):
        if v > self.current_location:
            self.increment_location(v - self.current_location)
        if v < self.current_location:
            self.decrement_location(self.current_location - v)

    @staticmethod
    def pad_zero(i, dst_len=2):
        s = str(i)
        if len(s) < dst_len:
            return '0' + s
        return s

    def get_filename(self, extension='.pbz2', no_path=False):
        fn = ''
        if self.override_filename is not None:
            fn = self.override_filename
            if '.' not in fn:
                fn += extension
        else:
            fn = self.pad_zero(self.get_slice_num()) + '-' + \
                 self.pad_zero(self.get_location_num()) + '-' + \
                 self.pad_zero(self.get_record_num()) + extension
        if no_path:
            return fn
        return self.get_save_dir() + '/' + fn

    def dump_python_object_to_pickle(self, filename, obj, extension='.pbz2'):
        if len(extension) > 0 and extension[0] != '.':
            extension = '.' + extension
        if filename is None:
            filename = self.get_filename_to_write(extension=extension)
        with bz2.BZ2File(filename, 'w') as f:
            cPickle.dump(obj, f)

    @staticmethod
    def retrieve_python_object_from_pickle(filename):
        try:
            data = bz2.BZ2File(filename, 'rb')
            return cPickle.load(data)
        except Exception as e:
            print("could not load file:", filename)
            print(e)

    def save_to_compressed_file(self):
        acqui_images = self.data.get_acqui_images()
        rli_images = self.data.get_rli_images()
        fp_data = self.data.get_fp_data()
        fn = self.get_filename_to_write()

        print("Saving to file " + fn + "...")
        metadata = {
            'version': 6,  # Little Dave version
            'num_fp': self.data.get_num_fp(),
            'slice_no': self.current_slice,
            'location_no': self.current_location,
            'run_no': self.current_record,
            'num_trials': self.data.get_num_trials(),
            'num_pts': self.data.get_num_pts(),
            'int_pts': self.data.get_int_pts(),
            'acquisition_onset': self.data.get_acqui_onset(),
            'stimulation1_onset': self.data.get_stim_onset(1),
            'stimulation2_onset': self.data.get_stim_onset(2),
            'stimulation1_duration': self.data.get_stim_duration(1),
            'stimulation2_duration': self.data.get_stim_duration(2),
            'rli_pts_dark': self.data.hardware.get_num_dark_rli(),
            'rli_pts_light': self.data.hardware.get_num_light_rli(),
        }
        d = {
            'acqui': acqui_images,
            'rli': rli_images,
            'fp': fp_data,
            'meta': metadata
        }
        self.dump_python_object_to_pickle(fn, d)
        print("File saved.")

    def load_from_compressed_file(self, filename):
        data = self.retrieve_python_object_from_pickle(filename)
        self.data.file_metadata = data['meta']

        # Recording load
        self.data.set_acqui_images(data['acqui'], from_file=True)

        # RLI load
        self.data.set_rli_images(data['rli'], from_file=True)

        # FP data load
        self.data.set_fp_data(data['fp'])

    def load_from_file(self, filename):
        file_ext = filename.split('.')[-1]
        if file_ext == 'zda':
            self.load_from_legacy_file(filename)
        elif file_ext == 'pbz2' or file_ext == 'roi':
            self.load_from_compressed_file(filename)

    def load_from_legacy_file(self, filename):
        ds = Dataset(filename)
        # Set meta data
        meta = ds.get_meta()
        self.data.file_metadata = meta

        # Recording load
        images = ds.get_data()
        self.data.set_acqui_images(images, from_file=True)

        # RLI load
        rli_dict = ds.get_rli()
        rli_shape = rli_dict['rli_low'].shape
        rli_images = np.zeros((3, rli_shape[0], rli_shape[1]))
        rli_images[0, :, :] = rli_dict['rli_low']
        rli_images[1, :, :] = rli_dict['rli_high']
        rli_images[2, :, :] = rli_dict['rli_max']
        self.data.set_rli_images(rli_images, from_file=True)

        # FP data load
        fp_data = ds.get_fp_data()
        self.data.set_fp_data(fp_data)
        self.data.set_num_fp(meta['num_fp'])

        if meta is not None:
            self.current_slice = ds.slice_number
            self.current_location = ds.location_number
            self.current_record = ds.record_number
            self.data.set_num_trials(ds.number_of_trials)
            new_num_pts = ds.points_per_trace
            if new_num_pts < 1 or new_num_pts is None:
                new_num_pts = images.shape[1]
            print("Number of pts: ", new_num_pts)
            self.data.set_num_pts(new_num_pts)
            self.data.set_num_trials(ds.number_of_trials)
            # There are only 3 (averaged) RLI frames in ZDA files:
            self.data.set_num_dark_rli(1)
            self.data.set_num_light_rli(2)

        return ds.get_meta()

    def set_save_dir(self, directory):
        self.save_dir = directory

    def get_save_dir(self):
        if self.save_dir is None:
            return os.getcwd()
        return self.save_dir


class Dataset:

    def __init__(self, filename, x_range=[0, -1], y_range=[0, -1], t_range=[0, -1]):
        self.x_range = x_range
        self.y_range = y_range
        self.t_range = t_range
        self.data, metadata, self.rli, self.fp_data = self.read_zda_to_df(filename)
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

    def get_fp_data(self, trial=None):
        if trial is None:
            return self.fp_data
        return self.fp_data[trial, :, :]

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

    def read_zda_to_df(self, zda_file):
        """ Reads ZDA file to dataframe, and returns
        metadata as a dict.
        ZDA files are a custom PhotoZ binary format that must be interpreted byte-
        by-byte """
        print("Reading legacy ZDA file. This can take several seconds." +
              "\n\t Important Note: Legacy ZDA file format (2006) is slow, and I" +
              "\n\t have no plans to speed it up (possible but not worth doing)." +
              "\n\t Please use PhotoZ to save your ZDA, once loaded, in the new" +
              "\n\t format (compressed pickle .pbz2) format.")
        file = open(zda_file, 'rb')
        # data type sizes in bytes
        chSize = 1
        shSize = 2
        nSize = 4
        tSize = 8
        fSize = 4

        metadata = {}
        metadata['version'] = (file.read(chSize))
        metadata['slice_number'] = (file.read(shSize))
        metadata['location_number'] = (file.read(shSize))
        metadata['record_number'] = (file.read(shSize))
        metadata['camera_program'] = (file.read(nSize))

        metadata['number_of_trials'] = (file.read(chSize))
        metadata['interval_between_trials'] = (file.read(chSize))
        metadata['acquisition_gain'] = (file.read(shSize))
        metadata['points_per_trace'] = (file.read(nSize))
        metadata['time_RecControl'] = (file.read(tSize))

        metadata['reset_onset'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['reset_duration'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['shutter_onset'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['shutter_duration'] = struct.unpack('f', (file.read(fSize)))[0]

        metadata['stimulation1_onset'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['stimulation1_duration'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['stimulation2_onset'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['stimulation2_duration'] = struct.unpack('f', (file.read(fSize)))[0]

        metadata['acquisition_onset'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['interval_between_samples'] = struct.unpack('f', (file.read(fSize)))[0]
        metadata['raw_width'] = (file.read(nSize))
        metadata['raw_height'] = (file.read(nSize))

        # Bytes to Python data type
        for k in metadata:
            if k in ['interval_between_samples', ] or 'onset' in k or 'duration' in k:
                pass  # any additional float processing can go here
            elif k == 'time_RecControl':
                pass  # TO DO: timestamp processing
            else:
                metadata[k] = int.from_bytes(metadata[k], "little")  # endianness

        metadata['num_fp'] = 4  # Little Dave
        if metadata['version'] <= 5:
            metadata['num_fp'] = 8  # Little Joe and legacy versions

        print("ZDA Version:", metadata['version'], "=> inferring", metadata['num_fp'], "FPs.")

        file.seek(1024, 0)
        # RLI
        rli_shape = (metadata['raw_height'], metadata['raw_width'])
        rli = {}
        for key in ['rli_low', 'rli_high', 'rli_max']:
            rli[key] = np.zeros(rli_shape)
            if key != 'rli_low':  # rli_low appears to not have FP data?
                for k in range(metadata['num_fp']):
                    # discard FP data associated with RLI
                    _ = int.from_bytes(file.read(shSize), "little")
            for jh in range(metadata['raw_height']):
                for jw in range(metadata['raw_width']):
                    rli[key][jh, jw] = int.from_bytes(file.read(shSize), "little")

        raw_data = np.zeros((metadata['number_of_trials'],
                             metadata['points_per_trace'],
                             metadata['raw_height'],
                             metadata['raw_width'])).astype(int)
        fp_data = np.zeros((metadata['number_of_trials'],
                            metadata['points_per_trace'],
                            metadata['num_fp'])).astype(int)
        num_read = 0
        for i in range(metadata['number_of_trials']):
            for x in range(metadata['num_fp']):
                for k in range(metadata['points_per_trace']):
                    if k != 0 and i != 0:  # skip the first num_fp
                        pt = self.read_data(file, shSize, num_read)
                        if pt is None:
                            return raw_data, metadata, rli, fp_data
                        fp_data[i, k, x] = int.from_bytes(pt, "little")
                        num_read += 1
            for jh in range(metadata['raw_height']):
                for jw in range(metadata['raw_width']):
                    for k in range(metadata['points_per_trace']):
                        pt = self.read_data(file, shSize, num_read)
                        if pt is None:
                            return raw_data, metadata, rli, fp_data
                        raw_data[i, k, jh, jw] = int.from_bytes(pt, "little")
                        num_read += 1

        file.close()
        return raw_data, metadata, rli, fp_data

    @staticmethod
    def read_data(file, sz, num_read):
        pt = file.read(sz)
        if not pt:
            print("Ran out of points. Read:", num_read)
            file.close()
            return None
        return pt


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
