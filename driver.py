from pyPhoto21.gui import GUI
from pyPhoto21.hardware import Hardware
from pyPhoto21.data import Data
from pyPhoto21.file import File

file = File()
hardware = Hardware()
data = Data(hardware)
gui = GUI(data, hardware, file)
