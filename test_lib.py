from hardware import Hardware
import numpy as np

# Demo

photo = Hardware()

new_arr = np.empty((2048*1024), dtype=np.uint16)

my_Controller_instance = photo.lib.createController()

photo.lib.setCameraProgram(my_Controller_instance, 7)

#print("Controller handle:", my_Controller_instance)

#photo.lib.takeRli(my_Controller_instance, new_arr)
#print('OK so far...')
#print("Array after calling sortArray():", new_arr)

# ... TO DO: use PhotoZ_Image ...

photo.lib.destroyController(my_Controller_instance)

print('done!')

