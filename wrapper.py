import ctypes
import numpy as np

## https://medium.com/@stephenscotttucker/interfacing-python-with-c-using-ctypes-classes-and-arrays-42534d562ce7

lib = ctypes.CDLL("x64/Debug/PhotoLib.dll")


SorterHandle = ctypes.POINTER(ctypes.c_char)

c_int_array = np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags='C_CONTIGUOUS')

lib.createSorter.argtypes = [ctypes.c_int]
lib.createSorter.restype = SorterHandle

lib.destroySorter.argtypes = [SorterHandle]

lib.setSorterArray.argtypes = [SorterHandle, c_int_array]

lib.getSorterArray.argtypes = [SorterHandle, c_int_array]

lib.sortArray.argtypes = [SorterHandle]


### Demo

unsorted = np.array([5,1,3,2,4], dtype=np.int32)
new_arr = np.empty(5, dtype=np.int32)

my_sorter_instance = lib.createSorter(5)
lib.setSorterArray(my_sorter_instance, unsorted)

print("Sorter handle:",my_sorter_instance)

lib.getSorterArray(my_sorter_instance, new_arr)
print("Array before calling sortArray():", new_arr)

lib.sortArray(my_sorter_instance)
lib.getSorterArray(my_sorter_instance, new_arr)
print("Array after calling sortArray():", new_arr)

lib.destroySorter(my_sorter_instance)


## Write a Python class wrapper as interface to lib

class Sorter:

    def __init__(self, array_size):
        self.size = array_size
        self.instance = lib.createSorter(self.size)

    def __del__(self):
        lib.destroySorter(self.instance)

    def getArray(self):
        arr = np.empty(self.size, dtype=np.int32)
        lib.getSorterArray(self.instance, arr)
        return arr

    def setArray(self, new_array):
        lib.setSorterArray(self.instance, new_array)

    def sort(self):
        lib.sortArray(self.instance)