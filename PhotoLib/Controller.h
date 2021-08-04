#pragma once
//=============================================================================
// Controller.h
//=============================================================================
#ifndef _Controller_h
#define _Controller_h
//=============================================================================
#include "edtinc.h"
#include <fstream>
#include "NIDAQmx.h"

class Channel;
class Camera;

//=============================================================================
class Controller
{
private:

	TaskHandle taskHandle_out; // Digital Output
	TaskHandle taskHandle_in; // Analog Input
	TaskHandle taskHandle_clk; // Chun: "M series don't have internal clock for output." -- X series though?
	TaskHandle taskHandle_led;

	int numPts;

	int32_t error = 0;
	char errBuff[2048];

	float acquiOnset;
	float intPts;
	float duration;
	int program;

	// RLI
	int darkPts;
	int lightPts;

	// Ch1
	int numPulses1;
	int intPulses1;

	int numBursts1;
	int intBursts1;


	// Ch1
	int numPulses2;
	int intPulses2;

	int numBursts2;
	int intBursts2;

	// Flags
	char stopFlag;
	char ltpIndFlag;
	char scheduleFlag;
	char scheduleRliFlag;

public:
	// Constructors
	Controller();
	~Controller();
	Channel* reset;
	Channel* shutter;
	Channel* sti1;
	Channel* sti2;

	// NI-DAQmx
	int NI_openShutter(uInt8);

	void NI_stopTasks();
	void NI_clearTasks();

	// Flags
	void setScheduleRliFlag(char);
	char getScheduleRliFlag();

	// Buffers for digital output
	uInt8 *outputs;

	// RLI
	int takeRli(unsigned short*);
	void setNumDarkRLI(int);
	int getNumDarkRLI();
	void setNumLightRLI(int);
	int getNumLightRLI();

	int getDisplayWidth();
	int getDisplayHeight();

	void setStimOnset(int ch, float v);
	void setStimDuration(int ch, float v);
	float getStimOnset(int ch);
	float getStimDuration(int ch);

	// NI Digital Output: create stimulation patterns
	void NI_fillOutputs();

	// Acquisition Control
	int acqui(unsigned short*, float64*);
	int stop();
	void resetCamera();

	// Acquisition Duration
	void setAcquiOnset(float);
	float getAcquiOnset();
	float getAcquiDuration();
	size_t get_digital_output_size();

	void setNumPts(int);
	int getNumPts();

	int getCameraProgram();
	void setCameraProgram(int p);

	void setIntPts(float);
	float getIntPts();

	// Duration of the whole Process
	void setDuration();
	float getDuration();

	// Stimulator
	void setNumPulses(int ch, int num);
	int getNumPulses(int ch);
	void setIntPulses(int ch, int num);
	int getIntPulses(int ch);

	void setNumBursts(int ch, int num);
	int getNumBursts(int ch);
	void setIntBursts(int ch, int num);
	int getIntBursts(int ch);
};

//=============================================================================
#endif
//=============================================================================