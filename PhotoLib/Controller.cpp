//=============================================================================
// Controller.cpp
//=============================================================================
#include "pch.h"

#include <iostream>
#include <stdlib.h>		// _gcvt()
#include <fstream>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <exception>
#include <stdint.h>
#include <omp.h>
#include <unordered_map>

#include "NIDAQmx.h"
#include "Controller.h"
#include "Channel.h"
#include "Camera.h"
#include "DataArray.h"
#include "Definitions.h"

// #pragma comment(lib,".\\lib\\NIDAQmx.lib")  Chun suggested it but turns out not to make any difference
using namespace std;

/* hacky way of synchronizing things, but it seems to work and nothing better
 * was found
 */
#define CAM_INPUT_OFFSET 10
#define NUM_BNC_CHANNELS 4
#define DAQmxErrChk(functionCall)  if( DAQmxFailed(error=(functionCall)) ) NiErrorDump(); else

 //=============================================================================
Controller::Controller()
{
	error = 0;
	char errBuff[2048] = { '\0' };
	reset = new Channel(0, 100);
	shutter = new Channel(0, 1210);
	sti1 = new Channel(300, 1);
	sti2 = new Channel(300, 1);

	// Acquisition
	acquiOnset = float(50);

	// Number of points per trace
	program = 7;
	numPts = 2000;
	intPts = 1000.0 / (double)Camera::FREQ[program];

	// Default RLI settings
	darkPts = 200;
	lightPts = 280;

	// Flags
	stopFlag = 0;
	scheduleFlag = 0;
	scheduleRliFlag = 0;

	// Ch1
	numPulses1 = 1;
	intPulses1 = 10;

	numBursts1 = 1;
	intBursts1 = 200;

	// Ch2
	numPulses2 = 1;
	intPulses2 = 10;

	numBursts2 = 1;
	intBursts2 = 200;

	// Set Duration
	setDuration();
}


//=============================================================================
Controller::~Controller()
{
	delete reset;
	delete shutter;
	delete sti1;
	delete sti2;
	NI_clearTasks();
}

void Controller::NiErrorDump() {
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	cout << errBuff;
}

//=============================================================================
// Acquisition
//=============================================================================
int Controller::takeRli(unsigned short* memory) {

	Camera cam;

	cam.setCamProgram(getCameraProgram());
	cam.init_cam();

	int array_diodes = cam.width() * cam.height() / 2;
	int rliPts = darkPts + lightPts;

	unsigned char* image;
	int width = cam.width();
	int height = cam.height();
	int quadrantSize = width * height;

	int superframe_factor = cam.get_superframe_factor();

	omp_set_num_threads(NUM_PDV_CHANNELS);
	// acquire dark frames with LED off	
	#pragma omp parallel for	
	for (int ipdv = 0; ipdv < NUM_PDV_CHANNELS; ipdv++) {

		int loops = darkPts / superframe_factor; // superframing 

		// Start all images
		cam.start_images(ipdv, loops);

		unsigned short* privateMem = memory + (ipdv * quadrantSize * rliPts); // pointer to this thread's section of MEMORY	
		for (int i = 0; i < loops; i++)
		{
			// acquire data for this image from the IPDVth channel	
			image = cam.wait_image(ipdv);

			// Save the image(s) to process later	
			memcpy(privateMem, image, quadrantSize * sizeof(short) * superframe_factor);
			privateMem += quadrantSize * superframe_factor; // stride to the next destination for this channel's memory	
		}
	}

	// parallel section pauses, threads sync and close	
	NI_openShutter(1);
	Sleep(100);
	omp_set_num_threads(NUM_PDV_CHANNELS);
	// parallel acquisition resumes now that light is on	
	#pragma omp parallel for	
	for (int ipdv = 0; ipdv < NUM_PDV_CHANNELS; ipdv++) {

		int loops = lightPts / superframe_factor; // superframing 

		cam.start_images(ipdv, loops);

		unsigned short* privateMem = memory + (ipdv * quadrantSize * rliPts) // pointer to this thread's section of MEMORY	
			+ (quadrantSize * darkPts); // offset of where we left off	

		for (int i = 0; i < loops; i++) 		// acquire rest of frames with LED on	
		{
			// acquire data for this image from the IPDVth channel	
			image = cam.wait_image(ipdv);

			// Save the image(s) to process later	
			memcpy(privateMem, image, quadrantSize * sizeof(short) * superframe_factor);
			privateMem += quadrantSize * superframe_factor; // stride to the next destination for this channel's memory	

		}
		cam.end_images(ipdv);
	}
	Sleep(100);
	NI_openShutter(0); // light off	

	//=============================================================================	
	// Image reassembly	
	cam.reassembleImages(memory, rliPts); // deinterleaves, CDS subtracts, and arranges data

	// Debug: print reassembled images out
	/*
	unsigned short* img = (unsigned short*)(memory);
	img += 355 * quadrantSize * NUM_PDV_CHANNELS / 2; // stride to the full image (now 1/2 size due to CDS subtract)


	std::string filename = "full-out355.txt";
	cam.printFinishedImage(img, filename.c_str(), true);
	cout << "\t This full image was located in MEMORY at offset " <<
		(img - (unsigned short*)memory) / quadrantSize << " quadrant-sizes\n";
	*/

	return 0;
}

int Controller::acqui(unsigned short *memory, float64 *fp_memory)
{
	Camera cam;
	cam.setCamProgram(getCameraProgram());
	cam.init_cam();

	//-------------------------------------------
	// Initialize NI tasks
	float64 samplingRate = 1000.0 / getIntPts(); // NI sampling rate is a function of cam frequency by default, but can be set.
	NI_createChannels(samplingRate);
	NI_setUpStimulationOutput(samplingRate);

	//-------------------------------------------
	// Initialize variables for camera data management
	unsigned char *image;
	int width = cam.width();
	int height = cam.height();
	int quadrantSize = width * height;

	int superframe_factor = cam.get_superframe_factor();

	//-------------------------------------------
	// Configure NI outputs and trigger
				// config clock channel M series don't have internal clock for output.
	// clk frequency calculation: see SM's BNC_ratio, BNC_R_list, output_rate, and frame_interval
	//			output_rate = BNC_ratio*1000.0 / frame_interval;

	DAQmxErrChk(DAQmxCreateTask("", &taskHandle_clk));
	DAQmxErrChk(DAQmxCreateCOPulseChanTime(taskHandle_clk, "Dev1/ctr0", "", 
						DAQmx_Val_Seconds, DAQmx_Val_Low, 0.00, 0.50 / getIntPts(), 0.50 / getIntPts()));
	DAQmxErrChk(DAQmxCfgImplicitTiming(taskHandle_clk, DAQmx_Val_ContSamps, get_digital_output_size()));

	// Clock for synchronizing tasks
	DAQmxErrChk(DAQmxCreateTask("", &taskHandle_out));
	DAQmxErrChk(DAQmxCreateDOChan(taskHandle_out, "Dev1/port0/line0:2", "", DAQmx_Val_ChanForAllLines));
	DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle_out, "/Dev1/PFI12", getIntPts(),
						DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, get_digital_output_size()));		//P
	
	// External trigger:
	DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(taskHandle_clk, "/Dev1/PFI1", DAQmx_Val_Rising));

	//-------------------------------------------
	// Configure NI inputs and trigger
	DAQmxErrChk(DAQmxCreateTask("", &taskHandle_in));
	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskHandle_in, "Dev1/ai0:3", "", DAQmx_Val_RSE, -10.0, 10.0, DAQmx_Val_Volts, NULL));
	DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle_in, "/Dev1/PFI0", 1005.0 / getIntPts(), 
				DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, get_digital_output_size() - getAcquiOnset()));	//frame-by-frame clock trigger


	//	int32 DAQmxWriteDigitalLines (TaskHandle taskHandle, int32 numSampsPerChan, bool32 autoStart, 
	//				float64 timeout, bool32 dataLayout, uInt8 writeArray[], int32 *sampsPerChanWritten, bool32 *reserved);
	//http://zone.ni.com/reference/en-XX/help/370471AM-01/daqmxcfunc/daqmxwritedigitallines/
	/*
	int32 defaultSuccess = -1;
	int32* successfulSamples = &defaultSuccess;
	int32 defaultReadSuccess = -1;
	int32* successfulSamplesIn = &defaultReadSuccess;

	DAQmxErrChk(DAQmxStartTask(taskHandle_in));

	DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle_out, get_digital_output_size(), false, 0, 
						DAQmx_Val_GroupByChannel, outputs, successfulSamples, NULL));
	int start_offset = (int)((double)(CAM_INPUT_OFFSET + acquiOnset) / intPts);
	//int32 DAQmxReadBinaryI16 (TaskHandle taskHandle, int32 numSampsPerChan, float64 timeout, bool32 fillMode, int16 readArray[], uInt32 arraySizeInSamps, int32 *sampsPerChanRead, bool32 *reserved);
	DAQmxErrChk(DAQmxReadBinaryI16(taskHandle_in, (numPts + 7 + start_offset), 0, 
						DAQmx_Val_GroupByScanNumber, fp_memory, 4 * numPts, successfulSamplesIn, NULL));
	DAQmxErrChk(DAQmxStartTask(taskHandle_out));
	*/
	//-------------------------------------------
	// Start NI tasks
	long total_written = 0, total_read = 0;
	float64 *NI_ptr = fp_memory;
	int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);
	DAQmxErrChk(DAQmxRegisterDoneEvent(taskHandle_clk, 0, DoneCallback, NULL));
	DAQmxErrChk(DAQmxWriteDigitalU32(taskHandle_out, get_digital_output_size(), 0, 10.0, 
					DAQmx_Val_GroupByChannel, outputs, &total_written, NULL));

	DAQmxErrChk(DAQmxStartTask(taskHandle_in));

	DAQmxErrChk(DAQmxStartTask(taskHandle_out));
	DAQmxErrChk(DAQmxStartTask(taskHandle_clk));
	cout << "Total written: " << total_written << "\n\t Number output: " << get_digital_output_size() << "\n";

	//-------------------------------------------
	// Camera Acquisition loops
	omp_set_num_threads(NUM_PDV_CHANNELS);
	#pragma omp parallel for	
	for (int ipdv = 0; ipdv < NUM_PDV_CHANNELS; ipdv++) {

		int loops = getNumPts() / superframe_factor; // superframing 

		// Start all images
		cam.start_images(ipdv, loops);

		unsigned short* privateMem = memory + (ipdv * quadrantSize *  getNumPts()); // pointer to this thread's section of MEMORY	
		for (int i = 0; i < loops; i++)
		{
			// acquire data for this image from the IPDVth channel	
			image = cam.wait_image(ipdv);

			// Save the image(s) to process later	
			memcpy(privateMem, image, quadrantSize * sizeof(short) * superframe_factor);
			privateMem += quadrantSize * superframe_factor; // stride to the next destination for this channel's memory	

			
			if (ipdv == 0) {
				long read;
				if (i == loops - 1)
					DAQmxReadAnalogF64(taskHandle_in, superframe_factor*(i + 1) - total_read, 5.0,
						DAQmx_Val_GroupByScanNumber, NI_ptr, (superframe_factor*(i + 1) - total_read + 1)*NUM_BNC_CHANNELS, &read, NULL);
				else
					DAQmxReadAnalogF64(taskHandle_in, superframe_factor*(i + 1) - total_read, 0.0,
						DAQmx_Val_GroupByScanNumber, NI_ptr, (superframe_factor*(i + 1) - total_read)*NUM_BNC_CHANNELS, &read, NULL);
				NI_ptr += read * NUM_BNC_CHANNELS;
				total_read += read;
			}
		}

	}

	//-------------------------------------------
	// NI Acquisition
	// DAQmxReadAnalogF64 on taskHandle input

	//=============================================================================	
	// Image reassembly	
	cam.reassembleImages(memory, numPts);

	free(outputs);
	return 0;
}

//=============================================================================
int Controller::NI_openShutter(uInt8 on)
{
	int32       error = 0;
	TaskHandle  taskHandle = 0;
	uInt8       data[4] = { 0,on,0,0 };
	char        errBuff[2048] = { '\0' };

	DAQmxErrChk(DAQmxCreateTask("", &taskHandle));
	DAQmxErrChk(DAQmxCreateDOChan(taskHandle, "Dev1/port0/line0:1", "", DAQmx_Val_ChanForAllLines));
	DAQmxErrChk(DAQmxStartTask(taskHandle));
	DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL));

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	if (taskHandle != 0) {
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}
	if (DAQmxFailed(error))
		printf("DAQmx Error: %s\n", errBuff);
	return 0;
}

//=============================================================================
void Controller::NI_stopTasks()
{
	// just ensure stopped
	DAQmxErrChk(DAQmxStopTask(taskHandle_in));
	DAQmxErrChk(DAQmxStopTask(taskHandle_out));
}


//=============================================================================
int Controller::stop()
{
	stopFlag = 1;
	NI_stopTasks();
	return  0;
}

//=============================================================================
void Controller::NI_setUpStimulationOutput(int freq)//set outputs samples array here//configure tasks here
{
	fillPDOut(outputs, 1);

	//Set timing for inputs
	  //http://zone.ni.com/reference/en-XX/help/370471AM-01/daqmxcfunc/daqmxcfgsampclktiming/
	  // "" should drive both from Onboard Clock (internal)
	//DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle_in, "", freq, DAQmx_Val_RisingSlope, DAQmx_Val_FiniteSamps, get_digital_output_size());
	//DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle_out, "", freq, DAQmx_Val_RisingSlope, DAQmx_Val_FiniteSamps, get_digital_output_size());
}


//=============================================================================
int Controller::NI_createChannels(float64 SamplingRate)
{
	DAQmxErrChk(DAQmxCreateTask("  ", &taskHandle_in));
	DAQmxErrChk(DAQmxCreateTask("  ", &taskHandle_out));
	//int32 DAQmxCreateDOChan (TaskHandle taskHandle, const char lines[], const char nameToAssignToLines[], int32 lineGrouping);
		//http://zone.ni.com/reference/en-XX/help/370471AM-01/daqmxcfunc/daqmxcreatedochan/
		//Channel names: http://zone.ni.com/reference/en-XX/help/370466AH-01/mxcncpts/physchannames/
//	DAQmxErrChk(DAQmxCreateDOChan(taskHandle_out, "Dev1/port0/line0:1", "", DAQmx_Val_ChanForAllLines));	//			this one did not work and was changed to the line below
	DAQmxErrChk(DAQmxCreateDOChan(taskHandle_out, "Dev1/port0/line1", "ledOutP0L0", DAQmx_Val_ChanForAllLines));
	//Set timing.
	//int32 DAQmxCfgSampClkTiming (TaskHandle taskHandle, const char source[], float64 rate, int32 activeEdge, int32 sampleMode, uInt64 sampsPerChanToAcquire);
		//http://zone.ni.com/reference/en-XX/help/370471AM-01/daqmxcfunc/daqmxcfgsampclktiming/
	DAQmxErrChk(DAQmxCfgSampClkTiming(taskHandle_out, NULL, SamplingRate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 348));
	return 0;
}

//=============================================================================
void Controller::NI_clearTasks()
{
	DAQmxClearTask(taskHandle_in);
	DAQmxClearTask(taskHandle_out);
}

size_t Controller::get_digital_output_size() {
	return (size_t)duration + 10;
}

//=============================================================================
void Controller::fillPDOut(uInt32 *outputs, char realFlag)
{
	int i, j, k;
	float start, end;
	outputs = new uInt32[get_digital_output_size()];
	const uInt32 shutter_mask = (1);		// digital out 0 based on virtual channel
	const uInt32 sti1_mask = (1 << 1);			// digital out 2
	const uInt32 sti2_mask = (1 << 2);			// digital out 3
	//--------------------------------------------------------------
	// Reset the array
	memset(outputs, 0, sizeof(uInt32) * get_digital_output_size());
	//--------------------------------------------------------------
	// Shutter
	if (realFlag) {
		start = shutter->getOnset();
		end = (start + shutter->getDuration());
		for (i = (int)start; i < end; i++)
			outputs[i] |= shutter_mask;
	}
	//--------------------------------------------------------------
	// Stimulator #1
	for (k = 0; k < numBursts1; k++)
	{
		for (j = 0; j < numPulses1; j++)
		{
			start = sti1->getOnset() + j * intPulses1 + k * intBursts1;
			end = (start + sti1->getDuration());
			for (i = (int)start; i < end; i++)
				outputs[i] |= sti1_mask;
		}
	}
	//--------------------------------------------------------------
	// Stimulator #2
	for (k = 0; k < numBursts2; k++)
	{
		for (j = 0; j < numPulses2; j++)
		{
			start = sti2->getOnset() + j * intPulses2 + k * intBursts2;
			end = (start + sti2->getDuration());
			for (i = (int)start; i < end; i++)
				outputs[i] |= sti2_mask;
		}
	}

	// Dear future hackers/developers: Add new stimulators or stimulation features and patterns here

}

void Controller::resetCamera()
{
	int	sure = 1; // fl_ask("Are you sure you want to reset camera?");
	Camera cam;
	if (sure == 1) {
		for (int ipdv = 0; ipdv < 4; ipdv++) {
			cam.end_images(ipdv);
		}
		char command1[80];
		sprintf(command1, "c:\\EDT\\pdv\\initcam -u pdv0_0 -f c:\\EDT\\pdv\\camera_config\\DM2K_1024x20.cfg");	//	command sequence from Chun B 4/22/2020
		system(command1);
		sprintf(command1, "c:\\EDT\\pdv\\initcam -u pdv1_0 -f c:\\EDT\\pdv\\camera_config\\DM2K_1024x20.cfg");
		system(command1);
		sprintf(command1, "c:\\EDT\\pdv\\initcam -u pdv0_1 -f c:\\EDT\\pdv\\camera_config\\DM2K_1024x20.cfg");
		system(command1);
		sprintf(command1, "c:\\EDT\\pdv\\initcam -u pdv1_1 -f c:\\EDT\\pdv\\camera_config\\DM2K_1024x20.cfg");
		system(command1);
		cout << " DapC resetCamera reset camera " << endl;
	}
	for (int ipdv = 0; ipdv < 4; ipdv++) {
		try {
			if (cam.open_channel(ipdv)) {
				cout << "DapC resetCamera Failed to open the channel!\n";
			}
		}
		catch (exception& e) {
			cout << e.what() << '\n';
		}
	}
}

//=============================================================================
void Controller::setDuration()
{
	float time;

	time = acquiOnset + int(getAcquiDuration()) + 1;
	duration = time;

	time = reset->getOnset() + reset->getDuration();
	if (time > duration)
		duration = time;

	time = shutter->getOnset() + shutter->getDuration();
	if (time > duration)
		duration = time;

	time = sti1->getOnset() + sti1->getDuration() + (numPulses1 - 1) * intPulses1 + (numBursts1 - 1) * intBursts1;
	if (time > duration)
		duration = time;

	time = sti2->getOnset() + sti2->getDuration() + (numPulses2 - 1) * intPulses2 + (numBursts2 - 1) * intBursts2;
	if (time > duration)
		duration = time;

	duration++;

	if (duration > 60000)
	{
		cout << "DC setDuration The total duration of the acquisition can not exceed 1 min! Please adjust DAP settings.\n";
		return;
	}
}

float Controller::getDuration() {
	setDuration();
	return duration;
}

//=============================================================================
void Controller::setNumPulses(int ch, int p) {
	if (ch == 1) numPulses1 = p;
	else numPulses2 = p;
}

//=============================================================================
void Controller::setNumBursts(int ch, int num) {
	if (ch == 1) numBursts1 = num;
	else numBursts2 = num;
}

//=============================================================================
int Controller::getNumBursts(int ch) {
	if (ch == 1) return numBursts1;
	return numBursts2;
}

//=============================================================================
int Controller::getNumPulses(int ch) {
	if (ch == 1) return numPulses1;
	return numPulses2;
}

//=============================================================================
void Controller::setIntBursts(int ch, int p) {
	if (ch == 1) intBursts1 = p;
	else intBursts2 = p;
}

//=============================================================================
void Controller::setIntPulses(int ch, int p) {
	if (ch == 1) intPulses1 = p;
	else intPulses2 = p;
}

//=============================================================================
int Controller::getIntBursts(int ch) {
	if (ch == 1) return intBursts1;
	return intBursts2;
}

//=============================================================================
int Controller::getIntPulses(int ch) {
	if (ch == 1) return intPulses1;
	return intPulses2;
}

//=============================================================================
void Controller::setScheduleRliFlag(char p) {
	scheduleRliFlag = p;
}

//=============================================================================
char Controller::getScheduleRliFlag() {
	return scheduleRliFlag;
}

//=============================================================================
// Number of Points per Trace
//=============================================================================
void Controller::setNumPts(int p)
{
	numPts = p;
}

//=============================================================================
int Controller::getNumPts()
{
	return numPts;
}

//=============================================================================
// Acquisition Onset
//=============================================================================
void Controller::setAcquiOnset(float p)
{
	acquiOnset = p;
}

//=============================================================================
float Controller::getAcquiOnset()
{
	return acquiOnset;
}

//=============================================================================
// Acquisition Duration
//=============================================================================
float Controller::getAcquiDuration()
{
	return (float)(numPts * intPts);
}

//=============================================================================
void Controller::setCameraProgram(int p)
{
	program = p;
}

//=============================================================================
int Controller::getCameraProgram()
{
	return program;
}

//=============================================================================
// Interval between Samples
//=============================================================================
void Controller::setIntPts(double p)
{
	intPts = p;
}

//=============================================================================
double Controller::getIntPts()
{
	return intPts;
}

void Controller::setNumDarkRLI(int dark) {
	darkPts = dark;
}

int Controller::getNumDarkRLI() {
	return darkPts;
}

void Controller::setNumLightRLI(int light) {
	lightPts = light;
}

int Controller::getNumLightRLI() {
	return lightPts;
}

int Controller::getDisplayWidth() {
	return Camera::DISPLAY_WIDTH[getCameraProgram()];
}

int Controller::getDisplayHeight() {
	return Camera::DISPLAY_HEIGHT[getCameraProgram()];
}


//=============================================================================

// Notes:
//Defining functions in files (like .dap files) which can send the signals to NI
//Dap820Put is used to send system commands. Figure out port equivalent to SYSin
//(or check if it's even needed as tasks can define and what needs to be done and
//  when executed will automatically send signals for niboards ports to the LED and STIMULATOR)

//Burst mode usage
