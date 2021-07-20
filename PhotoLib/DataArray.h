#pragma once
//=============================================================================
// DataArray.h
//=============================================================================
#ifndef DataArray_H
#define DataArray_H
//=============================================================================
//#include "ArrayWindow.h"

#define Max_Trials 100

class Data;
//=============================================================================
class DataArray
{
private:
	//-----------------------------------------------------
	// indexed as raw_diode_data[diode_no][trial_no][trace_pt]
	// the fp diodes are the last 8 for this and the rli
	short ***raw_diode_data;
	Data *array_data;
	Data *fp_data;
	Data *array_data_ROI[50];			// adding index enabled TraceWindow to retrieve each AveROI independently

	int numTrials;
	int numPts;
	int numAveRec;

	short *rli_low;
	short *rli_high;
	short *rli_max;

	double maxRli;

	// Raw data size
	int m_raw_width, m_raw_height, m_raw_depth;

	// binning
	int digital_binning;

	// Compare
	char compareFlag;
	char nor2ArrayMaxFlag;
	char increaseFlag;
	char absFlag;
	char averageFlag;
	char normalizationFlag;

	int record1No;
	int record2No;

	double* dataFeature;

	// Latency
	double latencyStart;
	double latencyWindow;

	//new
	double rliScalar;
	void resetDataFeature();
	void get_binned_diode_trace(int, int, int, double, double*);
	void get_binned_rli(int, short&, short&, short&);


	void release_mem();
	void alloc_raw_mem(int, int);
	void release_raw_mem();
	void alloc_trial_mem(int, int);
	void release_trial_mem();
	void alloc_binned_mem();
	void release_binned_mem();

public:
	//-----------------------------------------------------
	DataArray(int numPts);
	~DataArray();

	// Memory Manipulation
	void changeMemSize(int, int);
	void changeNumTrials(int);
	void changeNumPts(int);
	void changeRawDataSize(int, int);				//new

	void aveROIData(int region);		// new for averaging ROI
	// array information							all new
	int raw_width();
	int binned_width();
	int raw_height();
	int binned_height();
	int depth();
	int binning();
	void binning(int);
	int num_raw_diodes();
	int num_raw_array_diodes();
	int num_binned_diodes();
	int numRegions = 0;
	int num_diodes_fp();

	// Data Manipulation
	const short* getTrialMem(int trial, int diode);		//new
	void assignTrialData(short *trial_data, int len, int trial, int diode);			//new
	void loadTrialData(int trialNo);
	void arrangeData(int trialNo, unsigned short * input);
	void average();
	void resetData();

	// RLI
	void calRli();
	void setMaxRli();

	void setRliLow(short* dataBlock);
	void setRliHigh(short* dataBlock);
	void setRliMax(short* dataBlock);

	void setRliLow(int diodeNo, short rli);
	void setRliHigh(int diodeNo, short rli);
	void setRliMax(int diodeNo, short rli);

	// Signal Processing
	void process();
	void setCorrectionValue(int dataIndex, double value);
	double getCorrectionValue(int dataIndex);
	void makeCorrection();
	void measureAmp();
	void setNormalizationFlag(char);
	void setAverageFlag(char);
	void setNumAveRec(int);
	char getAverageFlag();
	int getNumAveRec();
	void ampCorrecting();

	// Ignorance
	void doSelection(int);
	void clearIgnoredFlag();
	char getIgnoredFlag(int dataNo);
	void setIgnoredFlag(int dataNo, char flag);
	void ignore(int dataNo);

	//
	void dataInversing();
	void rliDividing();
	void temporalFiltering();

	// Trace 2
	void saveTraces2();

	class Data* getData(int, int region = -1);
	class Data* getROIAve(int region);

	double *getProDataMem(int diodeNo);
	double *getSavDataMem(int diodeNo);
	double *getSlopeMem(int diodeNo);

	double* getAveData();

	double* aveData;
	//	double* getRawDataMem();

		// RLI Processing
	short getRliLow(int diodeNo);
	short getRliHigh(int diodeNo);
	short getRliMax(int diodeNo);
	double getRli(int diodeNo);
	double getRliRatio(int diodeNo);

	// Get Diode Properties
	char getFpFlag(int dataNo);
	double getAmp(int diodeNo, int ptNo);
	double getDataSD(int diodeNo);
	double getSignalToNoise(int value);
	double getMaxSlope(int value);
	double getMaxSlopeLatency(int value);
	double getDataMaxAmpLatency(int dataNo);
	double getDataHalfAmpLatency(int dataNo);
	int getDataMaxAmpLatencyPt(int dataNo);

	double getDataMaxAmp(int diodeNo, int recordNoChoice = 0);
	//	double getDiodeMaxAmpSD(int diodeIndex,int recordNoChoice=0);
	double getDiodeMaxAmpCha(int diodeIndex, int choice = 0);
	double getDiodeMaxAmpChaSD(int diodeIndex);
	double getDiodeMaxAmpPerCha(int diodeIndex);
	double getDiodeMaxAmpPerChaSD(int diodeIndex);

	// Compare
	void setRecordXNo(int recordX, int recordXNo);
	int getRecordXNo(int recordX);
	void setCompareFlag(char);
	char getCompareFlag();
	void setNor2ArrayMaxFlag(char);
	void setIncreaseFlag(char);
	char getIncreaseFlag();
	void setAbsFlag(char);
	char getAbsFlag();

	void setRli2DataFeature();
	void setMaxAmp2DataFeature();
	void setSpikeAmp2DataFeature();
	void setMaxAmpLatency2DataFeature();
	void setHalfAmpLatency2DataFeature();
	void setHalfAmpLatencyRatio2DataFeature();
	void setEPSPLatency2DataFeature();
	void setSignalToNoise2DataFeature();

	double getDataFeature(int);

	// Latency
	void setLatencyStart(double);
	double getLatencyStart();
	void setLatencyWindow(double);

	void setRliScalar(double);					//new
};

//=============================================================================
#endif
//=============================================================================
