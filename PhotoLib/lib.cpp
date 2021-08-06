#include "pch.h"
#include "Camera.h"
#include "Controller.h"



extern "C"
{

	__declspec(dllexport) Controller* createController()
	{
		return new Controller();
	}

	__declspec(dllexport) void destroyController(Controller* controller)
	{
		delete controller;
	}

	__declspec(dllexport) void takeRli(Controller* controller, unsigned short* images)
	{
		controller->takeRli(images);
	}

	__declspec(dllexport) void setCameraProgram(Controller* controller, int program)
	{
		controller->setCameraProgram(program);
	}

	__declspec(dllexport) int getCameraProgram(Controller* controller)
	{
		return controller->getCameraProgram();
	}


	__declspec(dllexport) void acqui(Controller* controller, unsigned short* images, float64* fp_data)
	{
		controller->acqui(images, fp_data);
	}

	__declspec(dllexport) void setNumPts(Controller* controller,  int numPts)
	{
		controller->setNumPts(numPts);
	}

	__declspec(dllexport) int getNumPts(Controller* controller)
	{
		return controller->getNumPts();
	}

	__declspec(dllexport) void setIntPts(Controller* controller, int interv)
	{
		controller->setIntPts(interv);
	}

	__declspec(dllexport) int getIntPts(Controller* controller)
	{
		return controller->getIntPts();
	}

	__declspec(dllexport) void setNumPulses(Controller* controller, int channel, int pulses)
	{
		controller->setNumPulses(channel, pulses);
	}

	__declspec(dllexport) int getNumPulses(Controller* controller, int channel)
	{
		return controller->getNumPulses(channel);
	}

	__declspec(dllexport) void setNumBursts(Controller* controller, int channel, int bursts)
	{
		controller->setNumBursts(channel, bursts);
	}

	__declspec(dllexport) int getNumBursts(Controller* controller, int channel)
	{
		return controller->getNumBursts(channel);
	}

	__declspec(dllexport) void setIntPulses(Controller* controller, int channel, int interv)
	{
		controller->setIntPulses(channel, interv);
	}

	__declspec(dllexport) int getIntPulses(Controller* controller, int channel)
	{
		return controller->getIntPulses(channel);
	}

	__declspec(dllexport) void setIntBursts(Controller* controller, int channel, int interv)
	{
		controller->setIntBursts(channel, interv);
	}

	__declspec(dllexport) int getIntBursts(Controller* controller, int channel)
	{
		return controller->getIntBursts(channel);
	}

	__declspec(dllexport) int getDuration(Controller* controller)
	{
		return controller->getDuration();
	}

	__declspec(dllexport) void setAcquiOnset(Controller* controller, float onset)
	{
		controller->setAcquiOnset(onset);
	}

	__declspec(dllexport) int getAcquiOnset(Controller* controller)
	{
		return controller->getAcquiOnset();
	}

	__declspec(dllexport) int getAcquiDuration(Controller* controller)
	{
		return controller->getAcquiDuration();
	}

	__declspec(dllexport) void setNumDarkRLI(Controller* controller, int dark)
	{
		controller->setNumDarkRLI(dark);
	}

	__declspec(dllexport) int getNumDarkRLI(Controller* controller)
	{
		return controller->getNumDarkRLI();
	}

	__declspec(dllexport) void setNumLightRLI(Controller* controller, int light)
	{
		controller->setNumLightRLI(light);
	}

	__declspec(dllexport) int getNumLightRLI(Controller* controller)
	{
		return controller->getNumLightRLI();
	}

	__declspec(dllexport) int getDisplayWidth(Controller* controller)
	{
		return controller->getDisplayWidth();
	}

	__declspec(dllexport) int getDisplayHeight(Controller* controller)
	{
		return controller->getDisplayHeight();
	}

	__declspec(dllexport) void setStimOnset(Controller* controller, int ch, float v)
	{
		controller->setStimOnset(ch, v);
	}

	__declspec(dllexport) void setStimDuration(Controller* controller, int ch, float v)
	{
		controller->setStimDuration(ch, v);
	}

	__declspec(dllexport) float getStimOnset(Controller* controller, int ch)
	{
		return controller->getStimOnset(ch);
	}

	__declspec(dllexport) float getStimDuration(Controller* controller, int ch)
	{
		return controller->getStimDuration(ch);
	}
};