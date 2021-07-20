
#include "pch.h"
#include "Camera.h"
#include <algorithm>
#include <iostream>

/*
extern "C"
{

	__declspec(dllexport) int add(int a, int b)
	{
		return a + b;
	}

};*/



Sorter::Sorter(int array_size)
{
	std::cout << "Sorter constructed." << std::endl;
	size = array_size;
	array = new int[size];
}

Sorter::~Sorter()
{
	std::cout << "Sorter destructed." << std::endl;
	delete array;
}

void Sorter::setArray(int* new_array)
{
	memcpy(array, new_array, size * sizeof(int));
}

void Sorter::getArray(int* array_out)
{
	memcpy(array_out, array, size * sizeof(int));
}

void Sorter::sortArray()
{
	std::cout << "Array sorted!" << std::endl;
	std::sort(array, array + size);
}