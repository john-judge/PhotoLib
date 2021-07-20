import ctypes

lib = ctypes.CDLL("x64/Debug/PhotoLib.dll")

lib.add.argtypes = (ctypes.c_int, ctypes.c_int)
lib.add.restype = ctypes.c_int

print("2 + 2 =", lib.add(2, 2))


