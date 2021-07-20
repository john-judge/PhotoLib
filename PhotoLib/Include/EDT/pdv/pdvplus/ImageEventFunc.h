
#ifndef _IMAGE_EVENT_FUNC_H
#define _IMAGE_EVENT_FUNC_H



typedef int (*ImageEventFunc)(void *target, 
			      void *pNewImage,
			      void *pWindow,
			      int nFrameNumber);

struct EventFuncRec {
    ImageEventFunc pFunc;
    void *target;

    EventFuncRec(ImageEventFunc f, void *pt)
    {
	pFunc = f;
	target = pt;
    }

    EventFuncRec()
    {
	pFunc = NULL;
	target = NULL;
    }



};


#endif
