#pragma once
//=============================================================================
// Definitions.h
//=============================================================================
#ifndef _Definitions_h
#define _Definitions_h
//=============================================================================
#include "Controller.h"
// Global Objects
extern class Controller *dc;
extern class Controller *dapControl;

extern class Camera *camera;
extern class DataArray *dataArray;


/*
extern class LiveFeed *lf;
extern class SignalProcessor *sp;
extern class ColorWindow *cw;
extern class DiodeArray *diodeArray;
extern class DapWindow *dw;
extern class FileController *fileController;
extern class WindowExporter *we;
extern class Color* colorControl;
*/
char* i2txt(int);
char* d2txt(double);
char* d2txt(double value, int digit);
char* f2txt(float);
//=============================================================================
#endif
//=============================================================================
