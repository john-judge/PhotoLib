import numpy as np
from matplotlib.path import Path

from pyPhoto21.analysis.core import AnalysisCore
from pyPhoto21.viewers.trace import Trace
from pyPhoto21.database.file import File

import os


class Database(File):

    def __init__(self):
        super().__init__()
        self.override_filename = None
        self.current_slice = 0
        self.current_location = 0
        self.current_record = 0

