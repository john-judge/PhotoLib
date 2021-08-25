import os

from pyPhoto21.gui import GUI
from pyPhoto21.hardware import Hardware
from pyPhoto21.data import Data
from pyPhoto21.database.metadata import Metadata

# Change directory
dir = os.getcwd()
os.chdir(dir + "\\..\\..")

PRODUCTION_MODE = False
print("\tLaunching PhotoZ Version 6.0 (Little Dave) \n\twith pyPhoto21 interface \n\tIn mode:")
if PRODUCTION_MODE:
    print("\t\tProduction")
else:
    print("\t\tDebug")

# We want data and hardware to sync up before we hook up the GUI display
hardware = Hardware()
data = Data(hardware)

# Now GUI will show the consistent settings from Data
gui = GUI(data, production_mode=PRODUCTION_MODE)
