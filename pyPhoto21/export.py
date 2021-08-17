import numpy as np

from pyPhoto21.database.file import File


class Exporter(File):

    def __init__(self, data):
        super().__init__(data.meta)