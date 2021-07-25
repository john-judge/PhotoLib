from pyPhoto21.gui import GUI
from pyPhoto21.hardware import Hardware
from pyPhoto21.data import Data
from pyPhoto21.file import File


hardware = Hardware()
data = Data(hardware)
file = File(data)
gui = GUI(data, hardware, file)
