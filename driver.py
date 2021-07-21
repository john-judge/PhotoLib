from gui import GUI
from hardware import Hardware
from data import Data
from file import File

file = File()
hardware = Hardware()
data = Data(hardware)
gui = GUI(data, hardware, file)