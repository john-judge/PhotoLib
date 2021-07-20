import ctypes
import numpy as np

## https://medium.com/@stephenscotttucker/interfacing-python-with-c-using-ctypes-classes-and-arrays-42534d562ce7


class PhotoLibDriver:
    
    def __init__(self):
        self.lib = ctypes.CDLL("x64/Debug/Photolib.dll")
        
        controllerHandle = ctypes.POINTER(ctypes.c_char)
        c_uint_array = np.ctypesself.lib.ndpointer(dtype=np.uint16, ndim=1, flags='C_CONTIGUOUS')
        
        self.lib.createController.argtypes = [ctypes.c_int]  # argument types
        self.lib.createController.restype = controllerHandle  # return type
        
        self.lib.destroyController.argtypes = [controllerHandle]
        
        self.lib.takeRli.argtypes = [controllerHandle, c_uint_array, ctypes.c_int]
        
        self.lib.acqui.argtypes = [controllerHandle, c_uint_array]
        
        self.lib.setCameraProgram.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getCameraProgram.argtypes = [controllerHandle]
        self.lib.getCameraProgram.restype = ctypes.c_int
        
        self.lib.setNumPts.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getNumPts.argtypes = [controllerHandle]
        self.lib.getNumPts.restype = ctypes.c_int
        
        self.lib.setIntPts.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getIntPts.argtypes = [controllerHandle]
        self.lib.getIntPts.restype = ctypes.c_int
        
        self.lib.setNumPulses.argtypes = [controllerHandle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getNumPulses.argtypes = [controllerHandle, ctypes.c_int]
        self.lib.getNumPulses.restype = ctypes.c_int
        
        self.lib.setIntPulses.argtypes = [controllerHandle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getIntPulses.argtypes = [controllerHandle, ctypes.c_int]
        self.lib.getIntPulses.restype = ctypes.c_int
        
        self.lib.setNumBursts.argtypes = [controllerHandle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getNumBursts.argtypes = [controllerHandle, ctypes.c_int]
        self.lib.getNumBursts.restype = ctypes.c_int
        
        self.lib.setIntBursts.argtypes = [controllerHandle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getIntBursts.argtypes = [controllerHandle, ctypes.c_int]
        self.lib.getIntBursts.restype = ctypes.c_int
        
        self.lib.setScheduleRliFlag.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getScheduleRliFlag.argtypes = [controllerHandle]
        self.lib.getScheduleRliFlag.restype = ctypes.c_int
        
        self.lib.setDuration.argtypes = [controllerHandle]
        
        self.lib.getDuration.argtypes = [controllerHandle]
        self.lib.getDuration.restype = ctypes.c_int
        
        self.lib.setAcquiOnset.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getAcquiOnset.argtypes = [controllerHandle]
        self.lib.getAcquiOnset.restype = ctypes.c_int
        
        self.lib.getAcquiDuration.argtypes = [controllerHandle]
        self.lib.getAcquiDuration.restype = ctypes.c_int
        
        self.lib.NiErrorDump.argtypes = [controllerHandle]
        
        self.lib.setNumDarkRLI.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getNumDarkRLI.argtypes = [controllerHandle]
        self.lib.getNumDarkRLI.restype = ctypes.c_int
        
        self.lib.setNumLightRLI.argtypes = [controllerHandle, ctypes.c_int]
        
        self.lib.getNumLightRLI.argtypes = [controllerHandle]
        self.lib.getNumLightRLI.restype = ctypes.c_int


# Demo

photo = PhotoLibDriver()

unsorted = np.array([5,1,3,2,4], dtype=np.int32)
new_arr = np.empty(5, dtype=np.int32)

my_Controller_instance = photo.lib.createController(5)
photo.lib.setControllerArray(my_Controller_instance, unsorted)

print("Controller handle:", my_Controller_instance)

photo.lib.takeRli(my_Controller_instance)
print("Array after calling sortArray():", new_arr)

# ... TO DO: use PhotoZ_Image ...

photo.lib.destroyController(my_Controller_instance)


