#include "pch.h"
#include "Camera.h"

extern "C"
{

	__declspec(dllexport) Sorter* createSorter(int array_size)
	{
		return new Sorter(array_size);
	}

	__declspec(dllexport) void destroySorter(Sorter* sorter)
	{
		delete sorter;
	}

	__declspec(dllexport) void setSorterArray(Sorter* sorter, int* new_array)
	{
		sorter->setArray(new_array);
	}

	__declspec(dllexport) void getSorterArray(Sorter* sorter, int* array_out)
	{
		sorter->getArray(array_out);
	}

	__declspec(dllexport) void sortArray(Sorter* sorter)
	{
		sorter->sortArray();
	}

};