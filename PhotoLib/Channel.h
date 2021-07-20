#pragma once
//=============================================================================
// Channel.h
//=============================================================================
#ifndef _Channel_h
#define _Channel_h
//=============================================================================

class Channel
{
private:
	float onset;
	float duration;

public:
	Channel(float, float);

	void setOnset(float);
	float getOnset();
	char *getOnsetTxt();

	void setDuration(float);
	float getDuration();
	char *getDurationTxt();
};

//=============================================================================
#endif
//=============================================================================
