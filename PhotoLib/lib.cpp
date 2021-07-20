#include "pch.h"
#include "Camera.h"



extern "C"
{
	__declspec(dllexport) Camera* createCamera(int array_size)
	{
		return new Camera();
	}

	__declspec(dllexport) void destroyCamera(Camera* camera)
	{
		delete camera;
	}


};