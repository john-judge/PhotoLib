//=============================================================================
// Data.cpp
//=============================================================================
#include "pch.h"
#include <math.h>

#include "Controller.h"
#include "Data.h"
//#include "RecControl.h"
//#include "SignalProcessor.h"
//#include "ArrayWindow.h"
//#include "TraceWindow.h"
#include "DataArray.h"
#include "Definitions.h"
#include <iostream>

using namespace std;
//=============================================================================
double Data::perAmp = 0.5;

void Data::setPerAmp(double input)
{
	perAmp = input;
}

//=============================================================================
Data::Data()
{
	ignoredFlag = 0;
	correctionValue = 1;
	maxAmpLatency = 0;

	allocMem();

	fittingVar[0] = 160;

	for (int i = 1; i < 5; i++)
	{
		fittingVar[i] = 0;
	}

	spikeStart = spikeEnd = 0;

	autoDetectSpike = 1;
	maxSlope = 0;
	spikeAmp = 0;
}

//=============================================================================
Data::~Data()
{
	releaseMem();
}


//=============================================================================
// Memory Manipulation
//
void Data::allocMem()
{
	int numPts = getNumPts();

	rawData = new double[numPts];
}

int Data::getNumPts() {
	return numPts;
}

void Data::setNumPts(int p) {
	numPts = p;
}

//=============================================================================
void Data::releaseMem()
{
	delete[] rawData;
	rawData = NULL;
}

//=============================================================================
void Data::changeNumPts()
{
	releaseMem();
	allocMem();
}

//=============================================================================
double *Data::getRawDataMem()
{
	return rawData;
}

//=============================================================================
void Data::setRli(double p)
{
	rli = p;
}

//=============================================================================

// added to do subtraction as in photoz 5.3
void Data::setRliLow(short p)
{
	rliLow = p;
}

//=============================================================================
short Data::getRliLow()
{
	return rliLow;
}

//=============================================================================
void Data::setRliHigh(short p)
{
	rliHigh = p;
}

//=============================================================================
short Data::getRliHigh()
{
	return rliHigh;
}
void Data::setRliMax(short p)
{
	rliMax = p;
}

//=============================================================================
short Data::getRliMax()
{
	return rliMax;
}


double Data::getRli()
{
	return rli;
}


//=============================================================================
double* Data::getRliArray()
{
	return rliArray;
}

//=============================================================================
// Reset Data and Properties
//
void Data::reset()
{
	int numPts = getNumPts();

	for (int i = 0; i < numPts; i++)
	{
		rawData[i] = 0;
	}

	// Reset RLI and Properties
	rliLow = 0;
	rliHigh = 0;
	rliMax = 0;
	rli = 1.0e-10;

	maxAmp = 0;
	maxAmpLatency = 0;
	halfAmpLatency = 0;
}
