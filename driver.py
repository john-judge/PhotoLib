from pyPhoto21.gui import GUI
from pyPhoto21.hardware import Hardware
from pyPhoto21.data import Data
from pyPhoto21.file import File

PRODUCTION_MODE = False
hardware = Hardware()
data = Data(hardware)
file = File(data)
gui = GUI(data, hardware, file, production_mode=PRODUCTION_MODE)
