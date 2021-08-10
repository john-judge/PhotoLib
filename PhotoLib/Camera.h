#pragma once


//=============================================================================
// Camera.h
//=============================================================================
#ifndef CAMERA_H_
#define CAMERA_H_

#include "edtinc.h"
#define NUM_PDV_CHANNELS 4

extern class RecControl* recControl;  // from definitions.h for getting camGain
// compiler camera switch
//#define LILJOE
#define LILDAVE

class Camera {
	char edt_devname[128];
	int m_depth;
	PdvDev* pdv_pt[NUM_PDV_CHANNELS];
	int timeouts[NUM_PDV_CHANNELS], m_num_timeouts;
	int last_timeouts;
	int overruns = 0;
	bool recovering_timeout;
	int m_program;				// which program is loaded currently

public:

#ifdef LILJOE

#define NUM_CAM_PRGS 8
	static const char* LABEL[];
	static const int WIDTH[];
	static const int HEIGHT[];
	static const int FREQ[];

#endif // LILJOE

public:
#ifdef LILDAVE

#define NUM_CAM_PRGS 8
	static const char* LABEL[];
	static const int WIDTH[];
	static const int HEIGHT[];
	static const int DISPLAY_WIDTH[];
	static const int DISPLAY_HEIGHT[];
	static const int WIDTH_DIVISIVE_FACTOR[];
	static const char* PROG[];
	static const int FREQ[];
	static const double sm_lib_rates[];
	static const double sm_cam_lib_rates[];
	static const double sm_integration_increments[];
	static const int ccd_lib_bin[];
	static const int config_load_lib[];
	static const int cds_lib[];
	static const int ndr_lib[];
	static const int stripes_lib[];
	static const int layers_lib[];
	static const int rotate_lib[];
	static const int bad_pix_lib[];
	static const int start_line_lib[];
	static const int super_frame_lib[];
	static const int NDR_start_lib[];
	static const int NDR_inc_lib[];
	static const int NDR_code_lib[];
	static const int NDR_max_lib[];
	static const int reserve1_lib[];
	static const int reserve2_lib[];
	static const int reserve3_lib[];
	static const int channelOrders[16];
	char cameraType[80], cameraStr[256];

#endif // LILDAVE*/

	Camera();
	~Camera();

	int open_channel(int ipdv);

	int close_channels();

	unsigned char* single_image(int ipdv);
	void start_images(int ipdv, int count);
	void end_images(int ipdv);
	void load_cfg();
	void init_cam();

	void setCamProgram(int p);
	unsigned char* wait_image(int ipdv);
	unsigned char** wait_images(int ipdv, int count);

	void serial_write(const char* buf);
	void serial_read(char* buf, int size);
	void serial_command(const char* command);

	int num_timeouts(int ipdv);

	void get_image_info(int ipdv);
	int get_buffer_size(int ipdv);

	int program();
	void program(int p);

	int get_superframe_factor();
	int get_ccd_bin_factor();
	int width();
	int height();
	int get_display_width();
	int get_display_height();
	int depth();
	int freq();

	// Main camera functions
	bool isValidPlannedState(int num_diodes);
	bool isValidPlannedState(int num_diodes, int num_fp_diodes);

	unsigned short* allocateImageMemory(int num_diodes, int numPts);
	bool acquireImages(unsigned short* memory, int numPts, int first, int notLast);
	bool acquireImages(unsigned short* images, int numPts);

	// camera utility functions. Generally drivers should not need to call
	void remapQuadrantsOneImage(unsigned short* srcBuf, unsigned short* dstBuf, int quadHeight, int quadWidth);
	void reassembleImages(unsigned short* images, int nImages);

	void deinterleave(unsigned short* srcBuf, unsigned short* dstBuf, int quadHeight, int quadWidth, const int* channelOrder);
	void subtractCDS(unsigned short* image_data, int nImages, int quad_height, int quad_width);

	// Debugging
	void printFinishedImage(unsigned short* image, const char* filename, bool CDS_done);
	void printQuadrant(unsigned short* image, const char* filename);


	// From TurboSM
	/*
	int SM_initCamera(int load_config_flag, int close_flag, int load_pro_flag);
	int SM_pdv_open();
	void SM_pdv_close();
	int checkCfgLUT();
	*/
};

#endif // CAMERA_H_


/*
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
};*/