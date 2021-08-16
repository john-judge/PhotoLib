import os
import struct
import numpy as np

from pyPhoto21.database.file import File
from pyPhoto21.database.metadata import Metadata


# Support for legacy data file (ZDA)
class LegacyData(File):

    def __init__(self):
        super().__init__(None)

    def load_zda(self, filename, db, meta):

        raw_data, metadata_dict, rli, fp_data = self.read_zda_to_variables(filename)
        self.populate_meta(meta, metadata_dict)
        self.populate_meta_rli(meta, rli)
        self.populate_meta_fp(meta, fp_data)
        db.meta = meta
        self.create_npy_file(db, raw_data)

    def create_npy_file(self, db, raw_data):
        db.clear_or_resize_mmap_file()  # loads correct dimensions since we already set meta
        arr = db.load_data_raw()
        arr[:, :, :, :] = raw_data[:, :, :, :]

    def populate_meta(self, meta_obj, metadata_dict):
        meta_obj.version = metadata_dict['version']
        meta_obj.num_fp = metadata_dict['num_fp']

        meta_obj.current_slice = metadata_dict['slice_number']
        meta_obj.current_location = metadata_dict['location_number']
        meta_obj.current_record = metadata_dict['record_number']

        # Hardware / DAQ settings
        meta_obj.num_trials = metadata_dict['number_of_trials']
        meta_obj.int_trials = metadata_dict['interval_between_trials']
        meta_obj.camera_program = metadata_dict['camera_program']
        meta_obj.height = metadata_dict['raw_height']
        meta_obj.width = metadata_dict['raw_width']
        meta_obj.acqui_onset = metadata_dict['acquisition_onset']
        meta_obj.stim_onset = [metadata_dict['stimulation1_onset'], metadata_dict['stimulation2_onset']]
        meta_obj.stim_duration = [metadata_dict['stimulation1_duration'], metadata_dict['stimulation2_duration']]
        meta_obj.num_pts = metadata_dict['points_per_trace']
        meta_obj.int_pts = metadata_dict['interval_between_samples']

    def populate_meta_fp(self, meta_obj, fp_data):
        sh = fp_data.shape
        if len(sh) < 3:
            t, n = sh
            fp_data = fp_data.reshape(1, t, n)
        meta_obj.fp_data = fp_data

    def populate_meta_rli(self, meta_obj, rli):
        meta_obj.rli_low = rli['rli_low']
        meta_obj.rli_high = rli['rli_high']
        meta_obj.rli_max = rli['rli_max']

    def read_zda_to_variables(self, zda_file):
        """ Reads ZDA file to dataframe, and returns
        metadata as a dict.
        ZDA files are a custom PhotoZ binary format that must be interpreted byte-
        by-byte """
        print("Reading legacy ZDA file. This can take several seconds." +
              "\n\t Important Note: Legacy ZDA file format (2006) will be"
              "\n\t automatically converted to the new .npy and metadata" +
              "\n\t formats (compressed pickle .pbz2).")
        file = open(zda_file, 'rb')
        # data type sizes in bytes
        ch_size = 1
        sh_size = 2
        n_size = 4
        t_size = 8
        f_size = 4

        metadata = {}
        metadata['version'] = (file.read(ch_size))
        metadata['slice_number'] = (file.read(sh_size))
        metadata['location_number'] = (file.read(sh_size))
        metadata['record_number'] = (file.read(sh_size))
        metadata['camera_program'] = (file.read(n_size))

        metadata['number_of_trials'] = (file.read(ch_size))
        metadata['interval_between_trials'] = (file.read(ch_size))
        metadata['acquisition_gain'] = (file.read(sh_size))
        metadata['points_per_trace'] = (file.read(n_size))
        metadata['time_RecControl'] = (file.read(t_size))

        metadata['reset_onset'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['reset_duration'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['shutter_onset'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['shutter_duration'] = struct.unpack('f', (file.read(f_size)))[0]

        metadata['stimulation1_onset'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['stimulation1_duration'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['stimulation2_onset'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['stimulation2_duration'] = struct.unpack('f', (file.read(f_size)))[0]

        metadata['acquisition_onset'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['interval_between_samples'] = struct.unpack('f', (file.read(f_size)))[0]
        metadata['raw_width'] = (file.read(n_size))
        metadata['raw_height'] = (file.read(n_size))

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
                    _ = int.from_bytes(file.read(sh_size), "little")
            for jh in range(metadata['raw_height']):
                for jw in range(metadata['raw_width']):
                    rli[key][jh, jw] = int.from_bytes(file.read(sh_size), "little")

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
                        pt = self.read_data(file, sh_size, num_read)
                        if pt is None:
                            return raw_data, metadata, rli, fp_data
                        fp_data[i, k, x] = int.from_bytes(pt, "little")
                        num_read += 1
            for jh in range(metadata['raw_height']):
                for jw in range(metadata['raw_width']):
                    for k in range(metadata['points_per_trace']):
                        pt = self.read_data(file, sh_size, num_read)
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
