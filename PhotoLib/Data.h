#pragma once
//=============================================================================
// Data.h
//=============================================================================
#ifndef Data_H
#define Data_H

#define Max_Num_Files 50		// has big impact on memory demand and increase with changes to camera cfgs with more traces

//=============================================================================
class Data
{
private:
	static double perAmp;

	//=====================================================
	// Flag
	//
	char ignoredFlag;

	//=====================================================
	// Memory
	//
	int numPts;
	double *rawData;		// Raw Data

	//=====================================================
	short rliLow;
	short rliHigh;
	short rliMax;

	// Properties
	//
	double rli;				// RLI Value
	double maxAmp;			// Maximal Amplitude
	double maxAmpLatency;	// The Latency of the Maximal Amplitude Point
	double halfAmpLatency;
	double maxSlopeLatency;
	int maxAmpLatencyPt;

	double correctionValue;

	//=====================================================
	// Time Course
	//
	double rliArray[Max_Num_Files];			// RLI Value
	double ampArray[Max_Num_Files];			// Amplitude
	double maxAmpArray[Max_Num_Files];		// Maximal Amplitude
	double maxAmpLatencyArray[Max_Num_Files];
	double halfAmpLatencyArray[Max_Num_Files];

	//=====================================================
	// Fitting
	//
	double fittingVar[5];
	int spikeStart, spikeEnd;

	bool autoDetectSpike;

	double maxSlope;
	double spikeAmp;

private:
	void allocMem();
	void releaseMem();

public:
	//=====================================================
	// Constructor and Destructor
	//
	Data();
	~Data();

	void static setPerAmp(double);

	//=====================================================
	// Memory Manipulation
	//
	void changeNumPts();
	int getNumPts();
	void setNumPts(int p);

	double *getRawDataMem();

	//=====================================================
	// RLI
	//
	//  void setRli(double);
	void setRliLow(short rliLow);
	void setRliHigh(short rliHigh);
	void setRliMax(short rliMax);
	void setRli(double);

	short getRliLow();
	short getRliHigh();
	short getRliMax();
	double getRli();

	double* getRliArray();

	void reset();

};

//=============================================================================
#endif
//=============================================================================
