from wrapper import PhotoLibDriver


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



