import ctypes, os
import numpy as np

# https://medium.com/@stephenscotttucker/interfacing-python-with-c-using-ctypes-classes-and-arrays-42534d562ce7


class PhotoLibDriver:
    
    def __init__(self, dll_path='./x64/Debug/'):
        dll_path = os.path.abspath(dll_path)

        self.lib = None
        if hasattr(os, 'add_dll_directory'):
            print(type(dll_path))
            os.add_dll_directory(os.path.dirname(dll_path + os.path.sep + "PhotoLib.dll"))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib/Include/EDT')))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib/Include')))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib')))
            env_paths = os.environ['PATH'].split(';')
            for path in env_paths:
                try:
                    os.add_dll_directory(path)
                    print('added DLL dependency path:', path)
                except:
                    print('Failed to add DLL dependency path:', path)

            print(os.path.dirname(os.path.abspath('./PhotoLib')))
            self.lib = ctypes.CDLL(dll_path + os.path.sep + "PhotoLib.dll")
        else:
            self.lib = ctypes.CDLL('./x64/Debug/PhotoLib.dll')

        controller_handle = ctypes.POINTER(ctypes.c_char)
        c_uint_array = np.ctypesself.lib.ndpointer(dtype=np.uint16, ndim=1, flags='C_CONTIGUOUS')
        
        self.lib.createController.argtypes = [ctypes.c_int]  # argument types
        self.lib.createController.restype = controller_handle  # return type
        
        self.lib.destroyController.argtypes = [controller_handle]
        
        self.lib.takeRli.argtypes = [controller_handle, c_uint_array, ctypes.c_int]
        
        self.lib.acqui.argtypes = [controller_handle, c_uint_array]
        
        self.lib.setCameraProgram.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getCameraProgram.argtypes = [controller_handle]
        self.lib.getCameraProgram.restype = ctypes.c_int
        
        self.lib.setNumPts.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumPts.argtypes = [controller_handle]
        self.lib.getNumPts.restype = ctypes.c_int
        
        self.lib.setIntPts.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getIntPts.argtypes = [controller_handle]
        self.lib.getIntPts.restype = ctypes.c_int
        
        self.lib.setNumPulses.argtypes = [controller_handle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getNumPulses.argtypes = [controller_handle, ctypes.c_int]
        self.lib.getNumPulses.restype = ctypes.c_int
        
        self.lib.setIntPulses.argtypes = [controller_handle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getIntPulses.argtypes = [controller_handle, ctypes.c_int]
        self.lib.getIntPulses.restype = ctypes.c_int
        
        self.lib.setNumBursts.argtypes = [controller_handle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getNumBursts.argtypes = [controller_handle, ctypes.c_int]
        self.lib.getNumBursts.restype = ctypes.c_int
        
        self.lib.setIntBursts.argtypes = [controller_handle, ctypes.c_int, ctypes.c_int]
        
        self.lib.getIntBursts.argtypes = [controller_handle, ctypes.c_int]
        self.lib.getIntBursts.restype = ctypes.c_int
        
        self.lib.setScheduleRliFlag.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getScheduleRliFlag.argtypes = [controller_handle]
        self.lib.getScheduleRliFlag.restype = ctypes.c_int
        
        self.lib.setDuration.argtypes = [controller_handle]
        
        self.lib.getDuration.argtypes = [controller_handle]
        self.lib.getDuration.restype = ctypes.c_int
        
        self.lib.setAcquiOnset.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getAcquiOnset.argtypes = [controller_handle]
        self.lib.getAcquiOnset.restype = ctypes.c_int
        
        self.lib.getAcquiDuration.argtypes = [controller_handle]
        self.lib.getAcquiDuration.restype = ctypes.c_int
        
        self.lib.NiErrorDump.argtypes = [controller_handle]
        
        self.lib.setNumDarkRLI.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumDarkRLI.argtypes = [controller_handle]
        self.lib.getNumDarkRLI.restype = ctypes.c_int
        
        self.lib.setNumLightRLI.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumLightRLI.argtypes = [controller_handle]
        self.lib.getNumLightRLI.restype = ctypes.c_int


