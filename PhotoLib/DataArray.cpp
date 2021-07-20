//=============================================================================
// DataArray.cpp
//=============================================================================
#include "pch.h"
#include <stdlib.h>		// _gcvt()
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>

#include "Definitions.h"
#include "DataArray.h"
#include "Controller.h"
#include "Data.h"

//#include "Diode.h"

#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>
#include "psapi.h"   //add these to the beginning of file

// Diode definitions
#define NUM_FP_DIODES 8
#define DEFAULT_ARRAY_WIDTH 256
#define DEFAULT_ARRAY_HEIGHT 40
#define NUM_BINNED_DIODES 10240

using namespace std;
//=============================================================================
// Constructor and Destructor
//=============================================================================
DataArray::DataArray(int input)
{
	compareFlag = 0;
	nor2ArrayMaxFlag = 1;
	increaseFlag = 1;
	absFlag = 1;
	normalizationFlag = 0;
	averageFlag = 0;
	numAveRec = 5;

	m_raw_depth = 14;
	digital_binning = 1; // max(1, DEFAULT_ARRAY_WIDTH * DEFAULT_ARRAY_HEIGHT / DEFAULT_BINNING_FACTOR);

	record1No = 0;
	record2No = 0;

	numPts = input;
	numTrials = 1;
	maxRli = 0;

	// Latency
	latencyStart = 100;
	latencyWindow = 5;

	rliScalar = 3200.0;					//new

	raw_diode_data = NULL;				//new 
	array_data = NULL;
	/*for (int i = 0; i < 50; i++) {
		array_data_ROI[i] = new Data;
	}*/
	fp_data = NULL;
	dataFeature = NULL;

	alloc_raw_mem(DEFAULT_ARRAY_WIDTH, DEFAULT_ARRAY_HEIGHT);
	alloc_trial_mem(numTrials, numPts);
	alloc_binned_mem();


}

//=============================================================================
DataArray::~DataArray()
{
	/*for (int i = 0; i < 50; i++) {
		delete array_data_ROI[i];
	}*/
	release_mem();
}

//=============================================================================
void DataArray::release_mem()
{
	release_binned_mem();
	release_trial_mem();
	release_raw_mem();
}

//=============================================================================
void DataArray::alloc_raw_mem(int w, int h)
{
	int num_raw = w * h + num_diodes_fp();

	raw_diode_data = new short**[num_raw];
	rli_low = new short[num_raw];
	rli_high = new short[num_raw];
	rli_max = new short[num_raw];

	memset(raw_diode_data, 0, sizeof(short**) * num_raw);
	memset(rli_low, 0, sizeof(short) * num_raw);
	memset(rli_high, 0, sizeof(short) * num_raw);
	memset(rli_max, 0, sizeof(short) * num_raw);

	m_raw_width = w;
	m_raw_height = h;
	cout << " da line 106 - num_raw " << num_raw << "  " << m_raw_width << "  " << m_raw_height << endl;

	digital_binning = 1;// max(1, w * h / DEFAULT_BINNING_FACTOR); // JMJ 12/13/2020 - default binning limits number of bins to ~2000
//	cout << "line 102 " << sizeof(short**)*num_raw << endl;
}

//=============================================================================
void DataArray::release_raw_mem()
{
	delete[] raw_diode_data;
	delete[] rli_low;
	delete[] rli_high;
	delete[] rli_max;

	raw_diode_data = NULL;
	rli_low = rli_high = rli_max = NULL;
	m_raw_width = m_raw_height = 0;
}

//=============================================================================
void DataArray::alloc_trial_mem(int trials, int pts)
{
	int i, j;

	for (i = 0; i < num_raw_diodes(); i++)
	{
		raw_diode_data[i] = new short*[trials];

		for (j = 0; j < trials; j++)
		{
			raw_diode_data[i][j] = new short[pts];
			memset(raw_diode_data[i][j], 0, sizeof(short) * pts);
		}
	}
	numTrials = trials;
	numPts = pts;
	maxRli = 0.0;
}

//=============================================================================
void DataArray::release_trial_mem()
{
	int i, j;
	int num_raw = m_raw_width * m_raw_height + NUM_FP_DIODES;

	for (i = 0; i < num_raw; i++)
	{
		for (j = 0; j < numTrials; j++)
			delete[] raw_diode_data[i][j];

		delete[] raw_diode_data[i];
		raw_diode_data[i] = NULL;
	}
	numTrials = numPts = 0;
	maxRli = 0.0;
}

//=============================================================================
void DataArray::alloc_binned_mem()
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
	DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;

	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;

	/*	cout << "Total Virtual Mem\t\t"<<totalVirtualMem << " bytes\n memUsed\t\t\t" << virtualMemUsed << " bytes \n";
		cout << "memory Used by program\t\t" << virtualMemUsedByMe << " bytes \n";
		cout << "size of allocation\t\t" << num_binned_diodes() * sizeof(Data) << "\n";
		cout << "size of data\t\t\t" << sizeof(Data) << "\nNum Diodes\t\t\t" << num_binned_diodes()<<"\n";*/

	array_data = new Data[num_binned_diodes()];
	fp_data = new Data[num_diodes_fp()];
	dataFeature = new double[num_binned_diodes()];
	for (int i = 0; i < num_binned_diodes(); i++)
	{
		dataFeature[i] = 0.0;
	}
	for (int i = 0; i < 50; i++) {
		array_data_ROI[i] = new Data;
	}
}

//=============================================================================
void DataArray::release_binned_mem()
{
	delete[] array_data;
	delete[] fp_data;
	delete[] dataFeature;

	array_data = fp_data = NULL;
	dataFeature = NULL;
	for (int i = 0; i < 50; i++) {
		array_data_ROI[i]->~Data();
		delete array_data_ROI[i];
	}
}

//=============================================================================
void DataArray::get_binned_diode_trace(int bin_diode, int trial_start, int trial_end, double scale, double *out)
{
	int i, j, k, m;
	j = k = m = 0;
	for (i = 0; i < numPts; i++)		out[i] = 0.0;
	int count = 0;
	double rwh = m_raw_height;				//these 3 lines round row_length so that binning works for n = 3 and 6 
	double binning = digital_binning;
	int row_length = max(1, 0.7 + (rwh / binning));		//	int row_length = m_raw_height / digital_binning;  //	previous version before fix
	int raw_diode = (bin_diode / row_length)*digital_binning*m_raw_width + (bin_diode%row_length)*digital_binning;		//	int raw_diode = bin_diode * digital_binning; this version is incorrect (MJ) 
	int x = raw_diode % m_raw_width;
	int y = raw_diode / m_raw_height;

	for (i = trial_start; i < trial_end; i++)							//index for trials
	{
		for (j = 0; j < digital_binning; j++)							//index for diodes in the bin
		{
			if (y + j >= m_raw_height)
				continue;
			for (k = 0; k < digital_binning; k++)
			{
				if (x + k >= m_raw_width)
					break;
				count++;
				for (m = 0; m < numPts; m++)								//index for time points
				{
					//					int diodeNo = raw_diode + k + (m_raw_width*j);
					int diodeNo = raw_diode + k + (m_raw_height*j);
					out[m] += raw_diode_data[diodeNo][i][m];				//key output of binned trace
				}
			}
		}
	}
	if (count != 0)
	{
		scale /= count;														//key division by number of diodes
		for (i = 0; i < numPts; i++)
		{
			out[i] *= scale;
		}
	}
	else return;
}
//=============================================================================
void DataArray::get_binned_rli(int bin_diode, short &low, short &high, short &max)
{
	int i, j, l, h, m;
	int count = 0;
	l = h = m = 0;
	double rwh = m_raw_height;				//these 3 lines round  row_length so that binning works for n = 3 and 6 
	double binning = digital_binning;
	int row_length = max(1, 0.7 + (rwh / binning));
	int raw_diode = (bin_diode / row_length)*digital_binning*m_raw_width + (bin_diode%row_length)*digital_binning;// key fix of bug from undergrad version
	int x = raw_diode % m_raw_width;
	int y = raw_diode / m_raw_height;
	for (i = 0; i < digital_binning; i++)
	{
		if (y + i >= m_raw_height)
			continue;
		for (j = 0; j < digital_binning; j++)
		{
			if (x + j >= m_raw_width)
				break;
			int index = raw_diode + j + (m_raw_height * i);
			l += rli_low[index];
			h += rli_high[index];
			m = max(rli_max[index], m);
			count++;
		}
	}
	if (count == 0)
		count = 1;
	l = l / count;
	h = h / count;
	low = l;
	high = h;
	max = m;
}

//=============================================================================
void DataArray::changeMemSize(int trials, int pts)
{
	int i;

	if (numTrials == trials && numPts == pts)
		return;

	release_trial_mem();
	alloc_trial_mem(trials, pts);

	for (i = 0; i < num_binned_diodes(); i++)
	{
		dataFeature[i] = 0.0;
		array_data[i].changeNumPts();
	}
	for (i = 0; i < num_diodes_fp(); i++)
	{
		fp_data[i].changeNumPts();
	}

	numTrials = trials;
	numPts = pts;
	maxRli = 0.0;
}

//=============================================================================
void DataArray::changeNumTrials(int trials)
{
	changeMemSize(trials, numPts);
}

//=============================================================================
void DataArray::changeNumPts(int pts)
{
	changeMemSize(numTrials, pts);
}

//=============================================================================
void DataArray::changeRawDataSize(int w, int h)
{
	if (m_raw_width == w && m_raw_height == h)
		return;

	// need to save these numbers
	int trials = numTrials;
	int pts = numPts;

	// release all memory
	release_mem();
	alloc_raw_mem(w, h);
	alloc_trial_mem(trials, pts);
	alloc_binned_mem();
}

//=============================================================================
int DataArray::raw_width()
{
	return m_raw_width;
}

//=============================================================================
int DataArray::binned_width()
{
	// divide and round up
	return (m_raw_width + (digital_binning - 1)) / digital_binning;
}

//=============================================================================
int DataArray::raw_height()
{
	return m_raw_height;
}

//=============================================================================
int DataArray::binned_height()
{
	// divide and round up
	return (m_raw_height + (digital_binning - 1)) / digital_binning;
}

//=============================================================================
int DataArray::depth()
{
	return m_raw_depth;
}

//=============================================================================
int DataArray::binning()
{
	return digital_binning;
}

//=============================================================================
void DataArray::binning(int binning)
{
	if (binning < 1 || binning == digital_binning)
		return;

	digital_binning = binning;
	release_binned_mem();
	alloc_binned_mem();
}

//=============================================================================
int DataArray::num_raw_array_diodes()
{
	return raw_width() * raw_height();		//divide by 4 because each pdv channel has 1/4 of the diodes. Update: Don't do that.
}

//=============================================================================
int DataArray::num_raw_diodes()
{
	return raw_width() * raw_height() + num_diodes_fp();
}

//=============================================================================
int DataArray::num_binned_diodes()
{
	//	cout << "da line 384 "<< binned_width() * binned_height()<< "\n";
	return binned_width() * binned_height();
}

//=============================================================================
int DataArray::num_diodes_fp()
{
	return NUM_FP_DIODES;
}

//=============================================================================
Data *DataArray::getData(int index, int region)
{
	/*	if (region != -1) {
			memcpy((array_data->getRawDataMem()), aveData, sizeof(double) * dc->getNumPts());
			return array_data; // +region + index;
		}*/
	if (index < 0)
		return fp_data + index + NUM_FP_DIODES;
	return array_data + index;
}



//=============================================================================
// RLI Processing
//=============================================================================
void DataArray::setRliLow(short *dataBlock)
{
	memcpy(rli_low, dataBlock, num_raw_array_diodes() * sizeof(short));
}

//=============================================================================
void DataArray::setRliHigh(short *dataBlock)
{
	memcpy(rli_high, dataBlock, num_raw_array_diodes() * sizeof(short));
}

//=============================================================================
void DataArray::setRliMax(short *dataBlock)
{
	memcpy(rli_max, dataBlock, num_raw_array_diodes() * sizeof(short));
}

//=============================================================================
void DataArray::setRliLow(int diodeNo, short rli)
{
	rli_low[diodeNo] = rli;
}

//=============================================================================
void DataArray::setRliHigh(int diodeNo, short rli)
{
	rli_high[diodeNo] = rli;
}

//=============================================================================
void DataArray::setRliMax(int diodeNo, short rli)
{
	rli_max[diodeNo] = rli;
}

//=============================================================================
short DataArray::getRliLow(int index)
{
	return rli_low[index];
}

//=============================================================================
short DataArray::getRliHigh(int index)
{
	return rli_high[index];
}

//=============================================================================
short DataArray::getRliMax(int index)
{
	return rli_max[index];
}

//=============================================================================

void DataArray::calRli()
{
	double rli;
	short low, high, max;
	// bin the rli
	for (int i = 0; i < num_binned_diodes(); i++)
	{
		get_binned_rli(i, low, high, max);
		double diff = (double)(high - low);
		//		rli = diff/ 3276.8;
		rli = double(high) / 3278;
		/*		if (rli <= 0) // No RLI
				{
					rli = -1;  //commented out to test rli acquisition
				}*/
		array_data[i].setRli(rli);
	}
}

//=============================================================================
void DataArray::setMaxRli()
{
	maxRli = 0;

	for (int i = 0; i < num_binned_diodes(); i++)
	{
		maxRli = max(array_data[i].getRli(), maxRli);
	}

	// don't check fp_data - all the rlis are 0
}

//=============================================================================
double DataArray::getRli(int diodeNo)
{
	return getData(diodeNo)->getRli();
}

//=============================================================================
// Output the ratio of RLI to maximum RLI
//=============================================================================
double DataArray::getRliRatio(int p)
{
	return getData(p)->getRli() / maxRli;
}


//=============================================================================
/*double DataArray::getDiodeMaxAmpSD(int diodeIndex,int recordNoChoice)
{
	// Determine which record is going to processed.
	int recordIndex;

	if(recordNoChoice==0)
	{
		recordIndex=recControl->getRecordNo()-1;
	}
	else if(recordNoChoice==1)
	{
		recordIndex=record1No;
	}
	else if(recordNoChoice==2)
	{
		recordIndex=record2No;
	}
	else if(recordNoChoice==3)
	{
		recordIndex=record2No;
	}

	// Calculation
	double sd=0;

	if(recordIndex>=(numAveRec-1))
	{
		int i;
		double sum=0;

		// Calculate the mean
		for(i=0;i<numAveRec;i++)
		{
			sum+=getData(diodeIndex)->getMaxAmp(recordIndex-i);
		}

		double mean=sum/numAveRec;

		// Calculate the sum of squares
		for(i=0,sum=0;i<numAveRec;i++)
		{
			sum+=pow(getData(diodeIndex)->getMaxAmp(recordIndex-i)-mean,2);
		}

		// Calculate SD (Standard Deviation)
		sd=sqrt(sum/numAveRec);
	}

	return sd;
}*/


//=============================================================================
char DataArray::getFpFlag(int diodeNo)
{
	return diodeNo < 0;
}

//=============================================================================
void DataArray::average()
{
	int i, j, k;

	//	short Gain=recControl->getAcquiGain();
	double Gain = 1.0;
	double scale = 1.0 / (Gain*3.2768);		// this line originally had numTrials in the denominator which scaled twice because numTrials was divided in get_binned ~ line 210

	// bin the traces
	for (i = 0; i < num_binned_diodes(); i++)
	{
		double *binned_data = array_data[i].getRawDataMem();
		get_binned_diode_trace(i, 0, numTrials, scale, binned_data);
	}

	// bin the fp traces
	for (i = 0; i < num_diodes_fp(); i++)
	{
		double *fp_trace = fp_data[i].getRawDataMem();

		for (j = 0; j < numPts; j++)		// removal appears to have no impact
		{
			fp_trace[j] = 0.0;
		}

		for (k = 0; k < numTrials; k++)
		{
			for (j = 0; j < numPts; j++)
			{
				fp_trace[j] += raw_diode_data[num_raw_array_diodes() + i][k][j];
			}
		}

		for (j = 0; j < numPts; j++)
		{
			fp_trace[j] *= scale;
		}
	}
}

//=============================================================================
void DataArray::resetData()
{
	int i, j;
	for (i = 0; i < num_raw_diodes(); i++)
	{
		for (j = 0; j < numTrials; j++)
			memset(raw_diode_data[i][j], 0, sizeof(short) * numPts);
	}

	for (i = 0; i < num_binned_diodes(); i++)
	{
		array_data[i].reset();
	}

	for (i = 0; i < num_diodes_fp(); i++)
	{
		fp_data[i].reset();
	}

	memset(rli_low, 0, sizeof(short) * num_raw_diodes());
	memset(rli_high, 0, sizeof(short) * num_raw_diodes());
	memset(rli_max, 0, sizeof(short) * num_raw_diodes());

	maxRli = 0.0;
}

//=============================================================================
const short* DataArray::getTrialMem(int trial, int diode)
{
	return raw_diode_data[diode][trial];
}

//=============================================================================
void DataArray::assignTrialData(short *trial_data, int len, int trial, int diode)
{
	memcpy(raw_diode_data[diode][trial], trial_data, sizeof(short)*len);
}

//=============================================================================
// The specification for the format of INPUT is determined in Camera.cpp
// They are read out in image-major order, i.e. quadrants of img0, quadrants of img1, ... 
void DataArray::arrangeData(int trialNo, unsigned short* input)		// used in DapControllerAcqui with input from camera (memory)
{
	int i, j;

	for (i = 0; i < num_raw_diodes(); i++)
	{
		for (j = 0; j < numPts; j++)
		{
			raw_diode_data[i][trialNo][j] = input[i + j * num_raw_diodes()];
		}
	}
}

//=============================================================================
void DataArray::setRecordXNo(int recordX, int recordXNo)
{
	if (recordX == 1)
	{
		record1No = recordXNo;
	}
	else if (recordX == 2)
	{
		record2No = recordXNo;
	}
}

//=============================================================================
int DataArray::getRecordXNo(int recordX)
{
	if (recordX == 1)
	{
		return record1No;
	}
	else if (recordX == 2)
	{
		return record2No;
	}

	return 1;
}

//=============================================================================
void DataArray::setCompareFlag(char input)
{
	compareFlag = input;
}

//=============================================================================
char DataArray::getCompareFlag()
{
	return compareFlag;
}

//=============================================================================
void DataArray::setNor2ArrayMaxFlag(char input)
{
	nor2ArrayMaxFlag = input;
}

//=============================================================================
void DataArray::setIncreaseFlag(char input)
{
	increaseFlag = input;
}

//=============================================================================
char DataArray::getIncreaseFlag()
{
	return increaseFlag;
}

//=============================================================================
void DataArray::setAbsFlag(char input)
{
	absFlag = input;
}

//=============================================================================
char DataArray::getAbsFlag()
{
	return absFlag;
}



//=============================================================================
void DataArray::setAverageFlag(char input)
{
	averageFlag = input;
}

//=============================================================================
char DataArray::getAverageFlag()
{
	return averageFlag;
}

//=============================================================================
double DataArray::getDataFeature(int input)
{
	return dataFeature[input];
}

//=============================================================================
void DataArray::resetDataFeature()
{
	for (int i = 0; i < num_binned_diodes(); i++)
	{
		dataFeature[i] = 0.0;
	}
}


//=============================================================================
void DataArray::setNormalizationFlag(char input)
{
	normalizationFlag = input;
}

//=============================================================================
void DataArray::setNumAveRec(int num)
{
	numAveRec = num;
}

//=============================================================================
int DataArray::getNumAveRec()
{
	return numAveRec;
}

//=============================================================================
double DataArray::getLatencyStart()
{
	return latencyStart;
}

void DataArray::setRliScalar(double input)
{
	if (input <= 0.001)
		input = 0.1;
	rliScalar = input;
}
