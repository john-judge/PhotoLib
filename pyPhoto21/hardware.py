import ctypes
import os
import time

import numpy as np

# https://medium.com/@stephenscotttucker/interfacing-python-with-c-using-ctypes-classes-and-arrays-42534d562ce7


class Hardware:
    
    def __init__(self, dll_path='./x64/Release/'):
        self.lib = None
        self.hardware_enabled = True
        try:
            self.load_dll(dll_path=dll_path)
            self.define_c_types()
            self.controller = self.lib.createController()
        except Exception as e:
            print(dll_path, "not found or otherwise unable to load. Photo21 will continue",
                  "in analysis mode without access to hardware for data acquisition.")
            print(e)
            self.hardware_enabled = False
        self.stop_flag = False
        self.livefeed_flags = None  # used to synchronize live feed

    def __del__(self):
        if self.hardware_enabled:
            try:
                self.lib.destroyController(self.controller)
            except AttributeError:
                pass

    def record(self, **kwargs):
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return
        imgs_orig_shape = kwargs['images'].shape
        t, fp_n = kwargs['fp_data'].shape
        imgs = np.zeros(imgs_orig_shape, dtype=np.uint16).reshape(-1)
        fp_data = kwargs['fp_data'].reshape(-1).astype(np.int16)
        try:
            self.lib.acqui(self.controller, imgs, fp_data)
        except Exception as e:
            print("Could not record. Are the camera and NI-USB connected?")
            print(e)
        imgs = imgs.reshape(imgs_orig_shape)
        kwargs['images'][:, :, :, :] = imgs[:, :, :, :]

        # FP shape (num_pts, num_fp)
        fp_data = fp_data.reshape(t, fp_n)
        fp_data_min = np.min(fp_data)
        if fp_data_min < 0:
            fp_data -= fp_data_min
        kwargs['fp_data'][:, :] = fp_data[:, :]

    def take_rli(self, **kwargs):
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return
        orig_shape = kwargs['images'].shape
        imgs = kwargs['images'].reshape(-1)
        try:
            self.lib.takeRli(self.controller, imgs)
        except Exception as e:
            print("Could not take RLI. Are the camera and NI-USB connected?")
            print(e)
        imgs = imgs.reshape(orig_shape)

    # choose programs 0-7 (inclusive)
    def set_camera_program(self, **kwargs):
        if not self.hardware_enabled:
            return
        if 'program' not in kwargs:
            return
        p = kwargs['program']
        if type(p) != int or p < 0 or p > 7:
            return
        self.lib.setCameraProgram(self.controller, kwargs['program'])

    def get_camera_program(self):
        if not self.hardware_enabled:
            return None
        return self.lib.getCameraProgram(self.controller)

    # set the number of points acquired during recording
    def set_num_pts(self, **kwargs):
        if not self.hardware_enabled:
            return
        self.lib.setNumPts(self.controller, kwargs['value'])

    def get_num_pts(self):
        if not self.hardware_enabled:
            return None
        return self.lib.getNumPts(self.controller)

    # set the interval between points acquired during recording
    def set_int_pts(self, **kwargs):
        if not self.hardware_enabled:
            return
        self.lib.setIntPts(self.controller, kwargs['value'])

    def get_int_pts(self):
        if not self.hardware_enabled:
            return None
        return self.lib.getIntPts(self.controller)

    # set the number of pulses during acquisition
    def set_num_pulses(self, **kwargs):
        if not self.hardware_enabled:
            return
        self.lib.setNumPulses(self.controller, kwargs['channel'], kwargs['value'])

    def get_num_pulses(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getNumPulses(self.controller, kwargs['channel'])

    # set the interval between pulses during acquisition, ms->frames
    def set_int_pulses(self, **kwargs):
        if not self.hardware_enabled:
            return
        num_frames = int(float(kwargs['value'] // self.get_int_pts()))
        self.lib.setIntPulses(self.controller, kwargs['channel'], num_frames)

    def get_int_pulses(self, **kwargs):
        if self.hardware_enabled:
            return self.lib.getIntPulses(self.controller, kwargs['channel']) * self.get_int_pts()

    # set the number of bursts during acquisition
    def set_num_bursts(self, **kwargs):
        if self.hardware_enabled:
            self.lib.setNumBursts(self.controller, kwargs['channel'], kwargs['value'])

    def get_num_bursts(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getNumBursts(self.controller, kwargs['channel'])

    # set the interval between bursts during acquisition
    def set_int_bursts(self, **kwargs):
        if self.hardware_enabled:
            num_frames = int(float(kwargs['value'] // self.get_int_pts()))
            self.lib.setIntBursts(self.controller, kwargs['channel'], num_frames)

    def get_int_bursts(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getIntBursts(self.controller, kwargs['channel']) * self.get_int_pts()

    # set acqui onset
    def set_acqui_onset(self, **kwargs):
        if self.hardware_enabled:
            self.lib.setAcquiOnset(self.controller, int(kwargs['acqui_onset']))

    def get_acqui_onset(self):
        if not self.hardware_enabled:
            return None
        return self.lib.getAcquiOnset(self.controller) * self.get_int_pts()

    #  get total acquisition duration
    def get_acqui_duration(self):
        if not self.hardware_enabled:
            return 0
        return self.lib.getAcquiDuration(self.controller)

    # set num dark RLI frames
    def set_num_dark_rli(self, **kwargs):
        if self.hardware_enabled:
            self.lib.setNumDarkRLI(self.controller, kwargs['dark_rli'])

    def get_num_dark_rli(self):
        if not self.hardware_enabled:
            return 200
        return self.lib.getNumDarkRLI(self.controller)

    # set num light RLI frames
    def set_num_light_rli(self, **kwargs):
        if self.hardware_enabled:
            self.lib.setNumLightRLI(self.controller, kwargs['light_rli'])

    def get_num_light_rli(self):
        if not self.hardware_enabled:
            return 280
        return self.lib.getNumLightRLI(self.controller)

    def get_display_width(self):
        if not self.hardware_enabled:
            return 1024
        return self.lib.getDisplayWidth(self.controller)

    def get_display_height(self):
        if not self.hardware_enabled:
            return 40
        return self.lib.getDisplayHeight(self.controller)

    def get_stim_onset(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getStimOnset(self.controller, kwargs['channel'])

    def get_shutter_onset(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getShutterOnset(self.controller)

    def set_shutter_onset(self, **kwargs):
        if self.hardware_enabled:
            v = kwargs['value']
            self.lib.setShutterOnset(self.controller, float(v))

    def get_stim_duration(self, **kwargs):
        if not self.hardware_enabled:
            return 0
        return self.lib.getStimDuration(self.controller, kwargs['channel'])

    def set_stim_onset(self, **kwargs):
        if self.hardware_enabled:
            v = kwargs['value']
            self.lib.setStimOnset(self.controller, kwargs['channel'], float(v))

    def set_stim_duration(self, **kwargs):
        if self.hardware_enabled:
            v = kwargs['value']
            self.lib.setStimDuration(self.controller, kwargs['channel'], float(v))

    def start_livefeed(self, lf_frame):
        # live feed flags are [produced_image, stop_livefeed]
        # scheme is producer-consumer relationship between python threads
        self.livefeed_flags = np.zeros(2, dtype=np.bool)
        self.clear_livefeed_produced_image_flag()
        self.set_livefeed_stop_loop_flag(False)
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return False
        orig_shape = lf_frame.shape
        lf_frame = lf_frame.reshape(-1)
        self.lib.startLiveFeed(self.controller, lf_frame, self.livefeed_flags)
        lf_frame = lf_frame.reshape(orig_shape)
        return True

    def continue_livefeed(self):
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return
        self.lib.continueLiveFeed(self.controller)

    def stop_livefeed(self):
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return
        self.set_livefeed_stop_loop_flag()
        timeout = 5.0
        while self.get_livefeed_stop_loop_flag() and timeout > 0:
            time.sleep(1)
            timeout -= 1
            print("waiting for acqui daemon to read stop-loop flag...", timeout)
        self.livefeed_flags = None

    def define_c_types(self):
        if not self.hardware_enabled:
            print("Hardware not enabled (analysis-only mode).")
            return
        controller_handle = ctypes.POINTER(ctypes.c_char)
        c_uint_array = np.ctypeslib.ndpointer(dtype=np.uint16, ndim=1, flags='C_CONTIGUOUS')
        # c_float_array = np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags='C_CONTIGUOUS')
        c_int_array = np.ctypeslib.ndpointer(dtype=np.int16, ndim=1, flags='C_CONTIGUOUS')
        c_bool_array = np.ctypeslib.ndpointer(dtype=np.bool, ndim=1, flags='C_CONTIGUOUS')

        self.lib.createController.argtypes = []  # argument types
        self.lib.createController.restype = controller_handle  # return type
        
        self.lib.destroyController.argtypes = [controller_handle]
        
        self.lib.takeRli.argtypes = [controller_handle, c_uint_array]
        
        self.lib.acqui.argtypes = [controller_handle, c_uint_array, c_int_array]

        self.lib.startLiveFeed.argtypes = [controller_handle, c_uint_array, c_bool_array]
        self.lib.continueLiveFeed.argtypes = [controller_handle]

        self.lib.resetCamera.argtypes = [controller_handle]
        
        self.lib.setCameraProgram.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getCameraProgram.argtypes = [controller_handle]
        self.lib.getCameraProgram.restype = ctypes.c_int
        
        self.lib.setNumPts.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumPts.argtypes = [controller_handle]
        self.lib.getNumPts.restype = ctypes.c_int
        
        self.lib.setIntPts.argtypes = [controller_handle, ctypes.c_double]
        
        self.lib.getIntPts.argtypes = [controller_handle]
        self.lib.getIntPts.restype = ctypes.c_float
        
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

        self.lib.getDuration.argtypes = [controller_handle]
        self.lib.getDuration.restype = ctypes.c_int
        
        self.lib.setAcquiOnset.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getAcquiOnset.argtypes = [controller_handle]
        self.lib.getAcquiOnset.restype = ctypes.c_int
        
        self.lib.getAcquiDuration.argtypes = [controller_handle]
        self.lib.getAcquiDuration.restype = ctypes.c_float

        self.lib.setNumDarkRLI.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumDarkRLI.argtypes = [controller_handle]
        self.lib.getNumDarkRLI.restype = ctypes.c_int
        
        self.lib.setNumLightRLI.argtypes = [controller_handle, ctypes.c_int]
        
        self.lib.getNumLightRLI.argtypes = [controller_handle]
        self.lib.getNumLightRLI.restype = ctypes.c_int

        self.lib.getDisplayWidth.restype = ctypes.c_int
        self.lib.getDisplayHeight.restype = ctypes.c_int

        self.lib.getStimOnset.restype = ctypes.c_float
        self.lib.getStimOnset.argtypes = [controller_handle, ctypes.c_int]

        self.lib.getStimDuration.restype = ctypes.c_float
        self.lib.getStimDuration.argtypes = [controller_handle, ctypes.c_int]

        self.lib.setShutterOnset.argtypes = [controller_handle, ctypes.c_float]

        self.lib.getShutterOnset.restype = ctypes.c_float
        self.lib.getShutterOnset.argtypes = [controller_handle]

        self.lib.setStimOnset.argtypes = [controller_handle, ctypes.c_int, ctypes.c_float]
        self.lib.setStimDuration.argtypes = [controller_handle, ctypes.c_int, ctypes.c_float]

    def load_dll(self, dll_path='./x64/Release/', verbose=False):
        dll_path = os.path.abspath(dll_path)
        if hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(os.getcwd())
            os.add_dll_directory(os.path.dirname(dll_path + os.path.sep + "PhotoLib.dll"))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib/Include/EDT')))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib/Include')))
            os.add_dll_directory(os.path.dirname(os.path.abspath('./PhotoLib')))
            env_paths = os.environ['PATH'].split(';')
            for path in env_paths:
                try:
                    os.add_dll_directory(path)
                    if verbose:
                        print('added DLL dependency path:', path)
                except:
                    if verbose:
                        print('Failed to add DLL dependency path:', path)
            self.lib = ctypes.cdll.LoadLibrary(dll_path + os.path.sep  + 'PhotoLib.dll')
        else:
            os.environ['PATH'] = os.path.dirname(dll_path + os.path.sep + "PhotoLib.dll") + ';'\
                                 + os.path.dirname(os.path.abspath('./PhotoLib/Include/EDT')) + ';' \
                                 + os.path.dirname(os.path.abspath('./PhotoLib/Include')) + ';' \
                                 + os.path.dirname(os.path.abspath('./PhotoLib')) + ';' \
                                 + os.environ['PATH']
            self.lib = ctypes.cdll.LoadLibrary(dll_path + 'PhotoLib.dll')

    def set_stop_flag(self, v=True):
        if v:
            print("Sending stop signal to daemon...")
        self.stop_flag = v

    def get_stop_flag(self):
        return self.stop_flag

    def reset_camera(self):
        self.lib.resetCamera(self.controller)

    def get_livefeed_produced_image_flag(self):
        return self.livefeed_flags[0]

    def clear_livefeed_produced_image_flag(self):
        self.livefeed_flags[0] = False

    def get_livefeed_stop_loop_flag(self):
        return self.livefeed_flags[1]

    def set_livefeed_stop_loop_flag(self, v=True):
        self.livefeed_flags[1] = v
