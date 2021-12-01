import os
import struct
import numpy as np
from astropy.utils.data import get_pkg_data_filename
from astropy.io import fits

from pyPhoto21.database.file import File
from pyPhoto21.database.metadata import Metadata


# load from .fit format? or tsm/tbn format?
# does fit = fits?  https://fits.gsfc.nasa.gov/
# RedshirtImaging website says it works with ImageJ, which supports:
#   https://imagej.nih.gov/ij/docs/guide/146-7.html#sub:Native-Formats
class TSM_Reader(File):

    def __init__(self, save_dir):
        super().__init__(None)
        self.save_dir = save_dir

    def load_tsm(self, filename, db, meta):
        raw_data, metadata_dict, rli, fp_data = self.read_tsm_to_variables(filename)
        self.populate_meta(meta, metadata_dict)
        db.meta = meta
        self.create_npy_file(db, raw_data, rli, fp_data)

    def load_fits(self, filename, db, meta):
        image_file = get_pkg_data_filename(filename)
        image_data = fits.getdata(image_file, ext=0)
        print("tsm_reader image shape:", image_data.shape)

        # set metadata in preparation for file creation
        meta.height, meta.width, meta.num_pts = image_data.shape
        meta.num_trials = 1

        # create npy file from image data
        db.clear_or_resize_mmap_file()  # loads correct dimensions since we already set meta
        arr = db.load_data_raw()
        arr[0, :, :, :] = image_data[:, :, :]  # only 1 trial per FITS file

    def read_tsm_to_variables(self, filename):
        pass

    # read NI data from .tbn file
    def read_tbn_to_variables(self, filename):

    def populate_meta(self, meta, metadata_dict):
        pass

    def create_npy_file(self, db, raw_data, rli, fp_data):
        pass

