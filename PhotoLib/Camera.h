#pragma once

class Sorter
{
protected:
	int *array;

public:
	int size;
	Sorter(int array_size);
	~Sorter();
	void setArray(int* new_array);
	void getArray(int* array_out);
	void sortArray();
};