from gui import GUI
from hardware import Hardware
from data import Data

hardware = Hardware()
data = Data(hardware)
gui = GUI(data, hardware)