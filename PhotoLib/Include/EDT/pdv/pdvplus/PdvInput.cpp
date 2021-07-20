// camera.cpp : implementation file
//
//


#include "edtimage/EdtImage.h"

// #include "edtdisplayscale.h"

#include "ctype.h"

extern "C" {
#include "edtinc.h"

#ifdef _WIN32

#include "bayer_filter_asm.h"

#endif

}

#ifdef _WIN32
#include <direct.h>

#pragma warning(disable:4800)

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#endif

#include "PdvInput.h"

#ifdef _WIN32

#define ShowMessage(message) \
    ::MessageBox(NULL, message, NULL, MB_OK|MB_ICONEXCLAMATION)

#define snprintf(...) _snprintf(__VA_ARGS__) 

#else

#define ShowMessage(message) \
    edt_msg(EDT_MSG_WARNING, message)


#endif


#ifdef _WIN32
#define SLASH "\\"
#else
#define SLASH "/"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define    CAMSETUPBATFILE	_T("camsetup.bat")
#define    CAMSETUPOUTFILE	_T("camsetup.out")

/////////////////////////////////////////////////////////////////////////////
// CCPdvInput class implementation


/////////////////////////////////////////////////////////////////////////////
// PdvInput construction and destruction


PdvInput::PdvInput(const char *devname, const int nUnit, const int nChannel)
{

    memset(m_Cameras, 0, sizeof(m_Cameras));
    m_nCameras = 1;
    m_pCurCamera = NULL;
    SetCameraIndex(0);

    m_pCurCamera->SetDevUnitChannel(devname, nUnit, nChannel);

    InitVariables();

    Initialize();
}


PdvInput::PdvInput(PdvDev *pdv_p)
{

    memset(m_Cameras, 0, sizeof(m_Cameras));
    m_nCameras = 1;
    m_pCurCamera = NULL;
    SetCameraIndex(0);

    InitVariables();

    if (GetDevice())
        UpdateFromCamera();

}

PdvInput::~PdvInput()
{
    if (pingpong)
        ClearPingPong();

    for (int i = 0; i < m_nCameras; i++)
    {
        SetCameraIndex(i);
        m_pCurCamera->Close();
    }

    m_bBuffering = false;
    // Destroy Camera List

    // Update Profile

    m_nStatus = 0;

    m_dLastTime = 0.0;

}


//////////////////////////////////////////////////////////////////////////////
// PdvInput class public functions 
//

void
PdvInput::InitInstance()

{
    // Change the registry key under which our settings are stored.
    // You should modify this string to be something appropriate
    // such as the name of your company or organization.
    // Determine exe full path name without the file name
}

int PdvInput::GetImageSize()
{    

    if (GetDevice())
    {
        AllocatedSize(pdv_get_allocated_size(GetDevice()));
    }

    return AllocatedSize();

}

#ifdef OLD

int PdvInput::GetImageSize()
{    

    if (GetDevice())
    {
        m_nImageSize = pdv_get_allocated_size(GetDevice());
    }

    return m_nImageSize;

}
#endif

int PdvInput::GetWidth()
{    
    if (GetDevice())
    {
        m_nWidth = pdv_get_width(GetDevice());
    }

    return m_nWidth;

}

int PdvInput::GetFrameHeight()

{
    if (GetDevice())
    {
        return  pdv_get_frame_height(GetDevice());
    }

    return 0;
}

int PdvInput::GetHeight()
{    
    if (GetDevice())
    {
        if (m_bUseFrameHeight)
        {
            m_nHeight = pdv_get_frame_height(GetDevice());
            if (m_nHeight == 0)
                m_nHeight = pdv_get_height(GetDevice());
        }
        else
        {
            m_nHeight = pdv_get_height(GetDevice());
        }
    }

    return m_nHeight;
}

int PdvInput::GetDepth()
{    
    if (GetDevice())
    {
        m_nDepth = pdv_get_depth(GetDevice());
    }

    return m_nDepth;
}

int PdvInput::GetNumTaps()
{    
    if (GetDevice() && !m_nTaps)
    {
        m_nTaps = ((edt_reg_read(GetDevice(), PDV_CL_DATA_PATH) & 0xf0) >> 4) + 1;
    }

    return m_nTaps;
}

int PdvInput::GetTimeouts()
{    
    if (!GetDevice())
        return 0;
    return pdv_timeouts(GetDevice());
}

char* PdvInput::GetCameraType()
{    
    if (!GetDevice())
        return 0;
    return pdv_get_cameratype(GetDevice());
}

char* PdvInput::GetCameraClass()
{    
    if (!GetDevice())
        return 0;

    return pdv_get_camera_class(GetDevice());
}

char* PdvInput::GetCameraModel()
{    
    if (!GetDevice())
        return 0;

    return pdv_get_camera_model(GetDevice());
}
bool PdvInput::IsSetup()
{    
    return m_bSetup;
}

void PdvInput::PreSetup()
{    
    m_bSetup = true;
}

int PdvInput::GetDbg()
{    
    return m_pCurCamera->m_Dbg;
}

int PdvInput::GetBinX()

{
    if (!GetDevice())
        return 0;

    return GetDevice()->dd_p->binx;

}

int PdvInput::GetBinY()

{
    if (!GetDevice())
        return 0;

    return GetDevice()->dd_p->biny;

}

void PdvInput::SetUnitNumber(int nUnitNumber)
{    
    m_pCurCamera->m_nUnitNumber = nUnitNumber;
}

//
// Send a command string to the camera
//

int PdvInput::ReadResponse(char *outbuf, int ishex, int expected_chars)

{
    int ret;

    char buf[2048];

    pdv_serial_wait(GetDevice(), GetDevice()->dd_p->serial_timeout, expected_chars) ;

    buf[0] = 0;

    ret = pdv_serial_read(GetDevice(), buf,2048) ;

    if (ishex)
    {
        int i;

        outbuf[0] = 0;
        for (i=0; i<ret; i++)
            sprintf(&outbuf[strlen(outbuf)], "%02x ", buf[i]);
        if ((strlen(outbuf) > 0) && (outbuf[strlen(outbuf)-1] == ' '))
            outbuf[strlen(outbuf)-1] = '\0';
    }
    else strcpy(outbuf, buf);

    return ret;

}

bool PdvInput::SendCommand(const char* szCommand, int ishex, char *outbuf)
{

    outbuf[0] = 0;

    if (!m_bSetup)
    {    // Send response to output control

        strcpy(outbuf, "Camera not setup");

        return false;
    }

    if (!HasDevice())
    {
        strcpy(outbuf, "Camera not initialized");
        return false;
    }


    // Send command to camera
    // edt_flush_resp(GetDevice()) ;
    (void)pdv_serial_read(GetDevice(), outbuf, 1024) ;

    // binary serial camera -- massage the data to make sure the string
    // is broken up into bytes, (and check that it's really hex data)
    // then send as binary
    if (ishex)
    {
        int nb = 0;
        unsigned char hexbuf[256];

        char *tmpbuf = (char *)szCommand;
        char *tok;

        do 
        {
            hexbuf[nb] = (u_char) strtoul(tmpbuf,&tok,0);

            if (tok != tmpbuf)
            {
                nb++;

                tmpbuf = tok;

            }
            else
            {
                tok = (char *)"";
            }

        } while (*tok);

        pdv_serial_binary_command(GetDevice(), (char *)hexbuf, nb);

    }
    else pdv_serial_command(GetDevice(),szCommand) ;

    return true;
}

//
// Increment shutter speed by one unit
//
#define SHUTTER_INC    2
#define SHUTTER_MAX GetDevice()->dd_p->shutter_speed_max
#define SHUTTER_MIN    GetDevice()->dd_p->shutter_speed_min

int PdvInput::IncShutterSpeed()
{
    // Get the current value
    int nShutterSpeed;
    if (!GetShutterSpeed(nShutterSpeed))

    {
        return -1;
    }

    // Validate the shutter speed value
    nShutterSpeed = nShutterSpeed + SHUTTER_INC;

    if (nShutterSpeed > SHUTTER_MAX)
    {    
        nShutterSpeed = SHUTTER_MAX;
    }

    return SetShutterSpeed(nShutterSpeed);
}

//
// Decrement shutter speed by one unit
//
int PdvInput::DecShutterSpeed()
{
    // Get the current value
    int nShutterSpeed;
    if (!GetShutterSpeed(nShutterSpeed))
    {    
        return -1;
    }

    // Validate the shutter speed value
    nShutterSpeed = nShutterSpeed - SHUTTER_INC;
    if (nShutterSpeed < SHUTTER_MIN)
    {    nShutterSpeed = SHUTTER_MIN;
    }

    return SetShutterSpeed(nShutterSpeed);
}

//
// Set shutter speed
//
bool PdvInput::SetShutterSpeed(int nShutterSpeed)
{
    if (!HasDevice())
    {
        return false;
    }

    // Validate the shutter speed value
    if (nShutterSpeed < SHUTTER_MIN || 
        nShutterSpeed > SHUTTER_MAX)
    {    
        char szBuf[256];
        sprintf(szBuf, "Shutter speed value (%d) is outside the valid range.", nShutterSpeed);

#ifdef _WIN32
        ::MessageBox(NULL, szBuf, NULL, MB_OK | MB_ICONEXCLAMATION);
#else
        edt_msg(EDT_MSG_WARNING, szBuf);

#endif

        return false;
    }

    // Set actual camera shutter speed by interacting
    // with the hardware. Return an error if this fails.
    //char speedstr[80] ;    
    //sprintf(speedstr,"EXE %d\r",nShutterSpeed) ;
    //serial_write(GetDevice(),speedstr,strlen(speedstr)) ;
    pdv_set_exposure(GetDevice(),nShutterSpeed);

    return true;
}

//
// Get shutter speed
//
bool PdvInput::GetShutterSpeed(int& nShutterSpeed)
{
    // Get actual camera shutter speed 
    nShutterSpeed = pdv_get_exposure(GetDevice()) ;
    return true;
}

//
// Set gain
//
bool PdvInput::SetGain(int nGain)
{
    if (!HasDevice())
    {
        return false;
    }

    // Validate the gain value
    if (pdv_set_gain(GetDevice(), nGain))
    {
        char szBuf[256];
        sprintf(szBuf, "Gain value (%d) is outside the valid range.", nGain);

#ifdef _WIN32
        ::MessageBox(NULL, szBuf, NULL, MB_OK | MB_ICONEXCLAMATION);
#else
        edt_msg(EDT_MSG_WARNING, szBuf);

#endif
        return false;
    }

    return true;
}

//
// Get gain
//
bool PdvInput::GetGain(int& nGain)
{
    // Get actual camera gain 
    nGain = pdv_get_gain(GetDevice());
    return true;
}

//
// Set level
//
bool PdvInput::SetLevel(int nLevel)
{
    if (!HasDevice())
    {
        return false;
    }

    // Validate the level value
    if (pdv_set_blacklevel(GetDevice(), nLevel))
    {    
        char szBuf[256];
        sprintf(szBuf, "Level value (%d) is outside the valid range.", nLevel);

#ifdef _WIN32
        ::MessageBox(NULL, szBuf, NULL, MB_OK | MB_ICONEXCLAMATION);
#else
        edt_msg(EDT_MSG_WARNING, szBuf);

#endif
        return false;
    }

    return true;
}

//
// Get level
//
bool PdvInput::GetLevel(int& nLevel)
{
    // Get actual camera level
    nLevel = pdv_get_blacklevel(GetDevice()) ;
    return true;
}


//
//
// Test image continuous capture setting
//
bool PdvInput::IsCapturingImages()
{    
    return m_bActiveFlag && (m_bFreerun || GetStarted() > GetDone());
}

//
// Toggle the camera's continuous capture mode
//
bool PdvInput::ToggleContinuousCapture()
{
    SetContinuousCapture(!m_bContinuous);
    return m_bContinuous;
}

//
// Turn on or off the camera's continuous capture mode
//
bool PdvInput::SetContinuousCapture(bool bContinuous)
{
    m_bContinuous = bContinuous;

    return m_bContinuous;
}

//
// Retrieve the camera's continuous capture mode setting
//
bool PdvInput::GetContinuousCapture(bool& bContinous)
{
    // Might be a good idea to talk to the actual hardware
    // here to get this setting

    bContinous = m_bContinuous;

    return true;
}


//
// Fill buffer with current camera image (blocking read)
//
bool PdvInput::FillBuffer(BYTE* pRawBits, ULONG ulSize)
{

    if (!m_bSetup || !pRawBits || !GetDevice() || !ulSize)
    {
        return false;
    }

    pdv_flush_fifo(GetDevice()) ;

    if (pdv_read(GetDevice(), pRawBits, ulSize) != (int) ulSize)
    {    
        //return false;
    }

    m_dwImageCount++;

    return true;
}

bool PdvInput::StartBuffering()
{
    PrepareBuffers();

    StartAcquire();

    return true;
}

void PdvInput::StopBuffering()
{
    m_bBuffering = false;

    if (GetDevice())
    {    
        pdv_stop_continuous(GetDevice());
        edt_disable_ring_buffers(GetDevice());
    }

    DestroyBufferImages();

    EndFrameCount();
}

//
// Capture image from the camera
//

bool PdvInput::CaptureImage(EdtImage * pImage)
{
    if (!m_bSetup || !GetDevice() || !pImage || GetWidth() == 0)
    {
        return false;
    }

    //    pdv_flush_fifo(GetDevice()) ;

    if (pingpong)
    {
        ResetPingPong();

        SetCameraIndex(0);
    }

    if (!IsBuffering())
        AddBufferImages();

    pdv_setup_continuous(GetDevice());

    m_pCurCamera->ResetCounts();

    bool bFreerun = GetFreeRun();

    SetFreeRun(false);

    edt_msleep(0);

    StartAcquire(1);


    EdtImage *pWorkImage = GetNextBufferImage();

    if (pWorkImage)
    {
        pImage->Copy(pWorkImage);
        pImage->SetTimeStamp(pWorkImage->GetTimeStamp());
    }

    SetFreeRun(bFreerun);
    SetActiveFlag(false);

    m_dwImageCount++;

    StopAcquiring();

    return true;
}


int
pdv_cl_pixelsperline(PdvDev *pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;
    int w = edt_reg_read(pdv_p, PDV_CL_PIXELSPERLINE);

    /* vert must be even if dual chan, and regs set to value/2  */
    if (pdv_is_cameralink(pdv_p))
    {

        /* negative htaps is used for 24-bit (ex) coming in
        as a single 8-bit stream */

        if (dd_p->htaps > 0)
        {
            w *= dd_p->htaps;   
        }
        else if (dd_p->htaps < 0)
        {
            w /=  -dd_p->htaps;
        }

    }

    return w;

}

int
pdv_cl_linesperframe(PdvDev *pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;
    int h = edt_reg_read(pdv_p, PDV_CL_LINESPERFRAME);
    /* vert must be even if dual chan, and regs set to value/2  */
    if (pdv_is_cameralink(pdv_p))
    {

        h *= dd_p->vtaps;
    }


    return h;

}

int
pdv_set_roi_registers(PdvDev * pdv_p, int hskip, int hactv, int vskip, int vactv)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     cam_w, cam_h;	/* camera actual w/h */


    cam_w = pdv_get_cam_width(pdv_p);
    cam_h = pdv_get_cam_height(pdv_p);

    /* check width/height for out of range, unless camera link or FOI */
    if (!pdv_is_cameralink(pdv_p))
    {
        if ((cam_w && ((hskip + hactv) > cam_w)) || ((hskip + hactv) <= 0))
        {
            return -1;
        }
        if ((cam_h && ((vskip + vactv) > cam_h)) || ((vskip + vactv) <= 0))
        {
            return -1;
        }
    }

    /* vert must be even if dual chan, and regs set to value/2  */
    if (pdv_is_cameralink(pdv_p))
    {

        /* negative htaps is used for 24-bit (ex) coming in
        as a single 8-bit stream */

        if (dd_p->htaps > 0)
        {
            hactv = (hactv / dd_p->htaps) * dd_p->htaps;
            hskip = (hskip / dd_p->htaps) * dd_p->htaps;
        }

        vactv = (vactv / dd_p->vtaps) * dd_p->vtaps;
        vskip = (vskip / dd_p->vtaps) * dd_p->vtaps;


        if (dd_p->htaps > 0)
        {

            edt_reg_write(pdv_p, PDV_HSKIP, (hskip / dd_p->htaps));
            edt_reg_write(pdv_p, PDV_HACTV, (hactv / dd_p->htaps) - 1);
        }
        else
        {
            edt_reg_write(pdv_p, PDV_HSKIP, (hskip * -dd_p->htaps));
            edt_reg_write(pdv_p, PDV_HACTV, (hactv * -dd_p->htaps) - 1);
        }

        edt_reg_write(pdv_p, PDV_VSKIP, (vskip / dd_p->vtaps));
        edt_reg_write(pdv_p, PDV_VACTV, (vactv / dd_p->vtaps) - 1);
    }
    else if (dd_p->dual_channel)
    {
        vactv = (vactv / 2) * 2;
        vskip = (vskip / 2) * 2;
        edt_reg_write(pdv_p, PDV_VSKIP, (vskip / 2));
        edt_reg_write(pdv_p, PDV_HSKIP, hskip);
        edt_reg_write(pdv_p, PDV_VACTV, (vactv / 2) - 1);
        edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
    }
    else
    {
        edt_reg_write(pdv_p, PDV_HSKIP, hskip);
        edt_reg_write(pdv_p, PDV_VSKIP, vskip);
        edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
        edt_reg_write(pdv_p, PDV_VACTV, vactv - 1);
    }

    return 0;			/* ALERT: need to return error from above if
                        * any */
}


bool PdvInput::GetNextBuffer(BYTE*& pRawBits)
{

    bool stopping = false;

    if (GetDone() == 0)
        TRACE("First GetNextBuffer %d > %d %f\n", 
        GetStarted(), GetDone(), edt_timestamp());

    // Wait for the last io operation to complete
    if (GetActiveFlag())
    {
        int nSkipped = 0;
        unsigned int timevec[2];
        int t0 = pdv_timeouts(GetDevice());

        if (checksize)
        {

        }

        if (m_bKeepUp)
        {

            if (m_bWaitNext)
            {
                pRawBits = pdv_wait_next_image_raw(GetDevice(), &nSkipped, true);
            }
            else
            {
                pRawBits = pdv_wait_last_image_raw(GetDevice(), &nSkipped,true);
            }

            edt_get_timestamp(GetDevice(), timevec, GetDevice()->donecount - 1);
        }
        else
        {
            pRawBits = pdv_wait_image_timed_raw(GetDevice(), timevec,true);

        }

        m_acquired_bufnum = (GetDevice()->donecount - 1) % GetDevice()->ring_buffer_numbufs;

        if (checksize)
        {


#if 1
            int w = pdv_get_width_xferred(GetDevice());
            int h = pdv_get_lines_xferred(GetDevice());
#else
            int w = pdv_get_width(GetDevice());
            int h = pdv_get_height(GetDevice());
#endif
            TRACE("Getting roi reg as %d x %d\n", w, h);
            if (w <= 0 || h <= 0)
            {
                TRACE("size neg.\n");
            }


            SetAcquiredSize(w,h);



        }

        m_dLastTime = timevec[0] + (double) timevec[1] * 0.000000001;    

        if (m_nLastInput == GetDevice()->donecount)
        {
            TRACE("Caught up...\n");
        }

        m_nLastInput = GetDevice()->donecount;


#if 0
        TRACE("done %d driver %d camera %d started %d skipped %d\n", 
            GetDevice()->donecount,
            edt_done_count(GetDevice()),
            GetDone(),
            GetStarted(), nSkipped);
#endif

        SetDone(GetDone() + nSkipped + 1);

        SetSkipped(GetSkipped() + nSkipped);

        m_nAhead = GetStarted() - GetDone();

        // TRACE("Waited\n");
        int t1 = pdv_timeouts(GetDevice());

        int overrun;

        if (m_bCheckOverrun)
            overrun = edt_reg_read(GetDevice(), PDV_STAT) & 1;
        else 
            overrun = 0;

        if (overrun || (t1 != t0))
        {
            TRACE("Timeouts %d Old %d Overrun %d\n", pdv_timeouts(GetDevice()), t0,
                overrun);

            pdv_timeout_restart(GetDevice(), true);

            m_bTimeoutRecovery = true;

        }
        else if (m_bTimeoutRecovery)
        {
            // We have to go another time, because the image just started may be dirty
            //
            TRACE("Timeout Reset\n");

            pdv_timeout_restart(GetDevice(), true);

            m_bTimeoutRecovery = false;		

        }    
    }
    else
    {
        TRACE("Getting All Started ...\n");
        edt_stop_buffers(GetDevice()) ;


        TRACE("StopAcquire Bufs %d Donecount %d pdv->donecount %d\n",
            m_nBufs,
            edt_done_count(GetDevice()), 
            GetDevice()->donecount);

        pRawBits = GetAllStartedImages();

        stopping = true;

        edt_set_buffer(GetDevice(),0);

        TRACE("Timeouts %d\n", pdv_timeouts(GetDevice()));

    }

    if (!pRawBits)
    {    
        m_bBuffering = false;
        return false;
    }

    m_dwImageCount++;

    // Start the next io operation if we weren't asked to stop
    if (!m_bContinuous || stopping)
    {

        // TRACE("Not starting...\n");
    }
    else 
    {    
        if (!m_bWaitForScreenUpdate 
            && GetDevice()->dd_p->swinterlace != PDV_BYTE_TEST1)
        {

            int ntostart = 1;

            if (m_bKeepUp)
            {
                // always start enough to keep ahead

                u_int curdone ;
                u_int curtodo ;

                curdone = edt_done_count(GetDevice()) ;
                curtodo = edt_get_todo(GetDevice()) ;

                ntostart = m_nBufs - (curtodo - curdone) - 2;

                if (ntostart <= 0)
                    ntostart = 1;

            }

            StartImages( ntostart );

        }
    }

    return true;
}

EdtImage *
PdvInput::GetNextBufferImage()

{
    if (!m_pBufImages)
        return NULL;

    BYTE *pRawBits;
    BYTE *pDisplayBits;
    int i;

    EdtImage *pImage = NULL;
    EdtImage *pRawImage = NULL;


    if (m_bDoBufs)
    {

        if (GetNextBuffer(pRawBits))
        {

            if (GetDevice()->pInterleaver)
            {

                pDisplayBits = pdv_get_interleave_data(GetDevice(), pRawBits, m_acquired_bufnum);

                pdv_deinterlace(GetDevice(), GetDevice()->dd_p, pRawBits, pDisplayBits);

            }
            else
            {

                pDisplayBits = pRawBits;

            }


            for (i = 0; i< m_nBufImages; i++)
            {
                if ((byte *) m_pBufImages[i]->GetBaseData() == pDisplayBits)
                {
                    pImage = m_pBufImages[i];
                    break;
                }
            }
            for (i = 0; i< m_nBufImages; i++)
            {
                if ((byte *) m_pRawImages[i]->GetBaseData() == pRawBits)
                {
                    pRawImage = m_pRawImages[i];
                    break;
                }
            }

            if (!pImage && pRawBits && (m_nBufImages < m_nBufs * 2))
            {

                // Add temp buffer to image list
                AttachBufferImage(m_nBufImages,pDisplayBits);
                pImage = m_pBufImages[m_nBufImages];

                AttachRawImage(m_nBufImages,pRawBits);
                pRawImage = m_pRawImages[m_nBufImages];

                m_nBufImages ++;

            }

        }

        if (pImage)
        {
            pImage->SetTimeStamp(m_dLastTime);
            pImage->SetSequenceNumber(GetDevice()->donecount-1);
            pImage->SetDataChanged();

            if (checksize)
            {
                pImage->SetSubRegion(GetAcquiredWidth(), GetAcquiredHeight());
            }
            else if (fval_done)
            {
                // clear the non-DMA region
                int lines = pdv_get_lines_xferred(GetDevice());
                pImage->FillRegion(128, 0, lines, pImage->GetWidth(), pImage->GetHeight()-lines);

            }

        }

    } 

    else

    {
        CaptureImage(m_pBufImages[0]);
        pImage = m_pBufImages[0];
    }

    if (pImage)
        m_pLastBufImage = pImage;

    if (!pRawImage)
        pRawImage = pImage;

    if (pRawImage)
        m_pLastRawImage = pRawImage;

    if (m_nCameras > 1 && m_bCycleCamera)
    {
        StepCameraIndex();
    }
    //TRACE("Drawing Image %d:%x\n",    i,pImage);

    return pImage;


}


bool PdvInput::IsBuffering()
{

    bool bBuffering = m_bBuffering;
    return bBuffering;
}

bool PdvInput::DoBufs()
{
    return m_bDoBufs ;
}

void PdvInput::SetDoBufs(bool val)
{
    m_bDoBufs = val ;
}


void PdvInput::StartNext()
{
    StartImages(1) ;
}

int PdvInput::SetPixelTypeFromDepth()
{
    int n = GetDepth();
    int d = n;


    if (n <= 8)
        n =  TypeBYTE;
    else if (n >8 && n <= 16)
        n =  TypeUSHORT;
    else if (n > 16 && n <= 24)
        n =  TypeBGR;
    else
        n =  TypeBGRA;

    SetType((EdtDataType) n);
    SetDepth(d);

    return n;
}    

int PdvInput::SetRawTypeFromDepth()
{
    int n = pdv_get_extdepth(GetDevice());
    EdtDataType t = TypeBYTE;

    if (n <= 8)
        t =  TypeBYTE;
    else if (n >8 && n <= 16)
        t =  TypeUSHORT;
    else if (n > 16 && n <= 24)
        t =  TypeBGR;
    else
        t =  TypeBGRA;

    SetRawType(t);

    return n;
}    

bool PdvInput::AddBufferImages()

{
    if (!GetDevice())
        return false;

    // if not user set, set up a reasonable # of bufs based on
    // image size, or if force_single specified just use one buffer

    TRACE("GetDevice() = %x GetDevice()->dd_p = %x\n", GetDevice(), GetDevice()->dd_p);

    if (!m_nBufs_userSet) 
    {
        int imagesize = GetImageSize();

       m_nBufs = 0x3000000 / imagesize ; 

        if (m_nBufs > 16)
            m_nBufs = 16;
        else if (m_nBufs < 3)
            m_nBufs = 3;
    }

    TRACE("Calling multibuf for %d bufs\n", m_nBufs);

    for (int cam = 0; cam < m_nCameras; cam++)
    {

        pdv_multibuf(GetDevice(), m_nBufs);

        pdv_setup_continuous(GetDevice());
    }

    m_pBufImages = new EdtImage *[2 * m_nCameras * m_nBufs];
    m_pRawImages = new EdtImage *[2 * m_nCameras * m_nBufs];

    m_nBufImages = 0;

    m_bBuffering = true;

    return true;

}

void PdvInput::DestroyBufferImages()
{

    if (m_pBufImages)
    {
        int i;

        for (i=0;i< m_nBufImages;i++)
            if (m_pBufImages[i])
                m_pBufImages[i]->Destroy();

        delete[] m_pBufImages;

        for (i=0;i< m_nBufImages;i++)
            if (m_pRawImages[i])
                m_pRawImages[i]->Destroy();

        delete[] m_pRawImages;

        m_pRawImages = NULL;
        m_pBufImages = NULL;
        m_nBufImages = 0;
    }
}



void PdvInput::SetLiveLut(byte * pLut)
{
    m_pLiveLut = pLut;
}

double PdvInput::GetFrameRate()
{

#ifdef _WIN32
    m_dwEndTime = GetTickCount();    

    if (m_dwStartTime /* && edt_get_debug(GetDevice()) */)
    {    
        double dwElapsedTime = m_dwEndTime - m_dwStartTime;
        if (dwElapsedTime)

            return (double)m_dwImageCount/ ((double)dwElapsedTime/1000.0);

    }
#endif

    return 0.0;
}


void PdvInput::AttachBufferImage(int nIndex, byte * pData)
{
    m_pBufImages[nIndex] = new EdtImage;

    m_pBufImages[nIndex]->HeaderSize(pdv_get_header_size(GetDevice()) - PDV_TIMESTAMP_SIZE);

    m_pBufImages[nIndex]->HeaderPosition(pdv_get_header_position(GetDevice()));

    m_pBufImages[nIndex]->HeaderOffset(pdv_get_header_offset(GetDevice()));

    m_pBufImages[nIndex]->AttachToData(pData, GetType(), GetWidth(), 
        pdv_get_height(GetDevice()),0);

    m_pBufImages[nIndex]->BufferSize(edt_allocated_size(GetDevice(), 0));

    if (GetDepth() > 8 && GetDepth() <= 16) {

        m_pBufImages[nIndex]->SetMinMax(0,(1 << GetDepth()) - 1);

    }

}

void PdvInput::AttachRawImage(int nIndex, byte * pData)
{
    m_pRawImages[nIndex] = new EdtImage;

    m_pRawImages[nIndex]->HeaderSize(pdv_get_header_size(GetDevice()) - PDV_TIMESTAMP_SIZE);

    m_pRawImages[nIndex]->HeaderPosition(pdv_get_header_position(GetDevice()));

    m_pRawImages[nIndex]->HeaderOffset(pdv_get_header_offset(GetDevice()));

    m_pRawImages[nIndex]->AttachToData(pData, GetRawType(), GetWidth(), 
        pdv_get_height(GetDevice()),0);

    m_pRawImages[nIndex]->BufferSize(pdv_get_dmasize(GetDevice()));

    if (GetDepth() > 8 && GetDepth() <= 16) {

        m_pRawImages[nIndex]->SetMinMax(0,(1 << GetDepth()) - 1);

    }

}


// Create the setup batch file

bool PdvInput::Setup(const char *cfgPathName,
                     const char* cfgFileName)
{

    if (IsBuffering())
        StopBuffering();

    m_bSetup = false;

    char szCurrentPath[_MAX_PATH+1];
    bool needchd = false; /* need to change directory */

    if (!getcwd(szCurrentPath,_MAX_PATH))
    {
        setLastError("Unable to get Current Directory.");
        return false;
    }

    char szPdvRoot[_MAX_PATH+1];
    strcpy(szPdvRoot,cfgPathName);

    char *sub;

    if (sub = strstr(szPdvRoot,"camera_config"))
    {
        size_t len = sub - szPdvRoot;

        szPdvRoot[len] = '\0';

    }

    if (strlen(szPdvRoot) && strcmp(szPdvRoot, szCurrentPath))
    {
        if (chdir(szPdvRoot))
        {
            setLastError( "Unable to Change To Directory.");
            return false;
        } 

        needchd = true;
    }

#ifdef _WIN32
    // Determine the path to the camera setup batch file
    char szCommand[_MAX_PATH+25];
    sprintf(szCommand,".%sinitcam -u %d -c %d -f \"%s\"\n", 
        SLASH, GetUnitNumber(), GetChannelNumber(), 
        cfgFileName);
#else
    //create path for config file
    // called by camsetup*_*.sh
    char szPath[_MAX_PATH+25];
    strcpy(szPath, cfgPathName);
    strcat(szPath, "/");
    strcat(szPath, cfgFileName);

    char szCommand[_MAX_PATH+1];
    sprintf(szCommand,".%sinitcam -u %d -c %d -f \"%s\"\n",
        SLASH, GetUnitNumber(), GetChannelNumber(),
        szPath);
#endif

#ifdef _WIN32
#define EXTENSION ".bat"
#else 
#define EXTENSION ".sh"
#endif

    sprintf(m_szBatchFile, 
        ".%s%s%d_%d%s", 
        SLASH, "camsetup",
        GetUnitNumber(), GetChannelNumber(),
        EXTENSION); 

    remove(m_szBatchFile);

    // Determine the path to the camera setup output file
    char szOutputFile[_MAX_PATH];
    sprintf(szOutputFile, "%s%d_%d.out", "camsetup",GetUnitNumber(),GetChannelNumber()); 
    remove(szOutputFile);

    // Build the contents for the setup batch file

    // Create the camera setup batch file
    FILE * fSetup;

    if ( (fSetup = fopen(m_szBatchFile, "w")) == NULL )
    {   
        char error_message[_MAX_PATH];

        snprintf(error_message, _MAX_PATH-1,
                 "Camera setup failed. Couldn't open file %s in %s for writing. Ensure that you have write permissions in %s and try again.", 
                 m_szBatchFile, szPdvRoot, szPdvRoot);
        setLastError(error_message);
        return false;
    }

    // Update the camera setup batch file with the contents 
#ifdef _WIN32
    fprintf(fSetup,"echo on\n");
#endif

    fprintf(fSetup,szCommand);

#ifdef _WIN32
    fprintf(fSetup,"Echo %%errorlevel%% >> %s\n", szOutputFile);
#else 
    fprintf(fSetup, "echo $? > %s", szOutputFile); /* initcam exit status */
#endif

    fclose(fSetup);

#ifndef _WIN32
    int rc = chmod(m_szBatchFile, 0755);
    if (rc == -1) 
    {
        char error_message[_MAX_PATH];

        snprintf(error_message, _MAX_PATH-1,
                 "Camera setup failed: chmod on %s failed\n",
                 m_szBatchFile);
        setLastError(error_message);

        return false;
    }
#endif

    int ret =  runSetupBat(szOutputFile);

    if (needchd)
    {
        chdir(szCurrentPath);
    }

    return (bool) ret;
}



void PdvInput::ExecuteSetup()
{

    system(m_szBatchFile) ;


}



void PdvInput::UpdateFromCamera()
{
    SetPixelTypeFromDepth();
    SetRawTypeFromDepth();

    pdv_setup_postproc(GetDevice(), GetDevice()->dd_p, NULL);

    m_bSetup = true;

    if (GetLoadedConfig())
    {
        m_pCurCamera->SetConfigName(GetLoadedConfig());
    }
    // get the camera_config path

    HeaderSize(pdv_get_header_size(GetDevice()));
    HeaderPosition(pdv_get_header_position(GetDevice()));
    HeaderOffset(pdv_get_header_offset(GetDevice()));
    m_bHeaderDma = (bool) (pdv_get_header_dma(GetDevice()) != 0);

}

bool PdvInput::Initialize(const char *devname,
                          int nUnitNumber,
                          int nChannelNumber)
{
    if (GetDevice())
    {    
        return true;
    }

    m_pCurCamera->SetDevUnitChannel(devname, nUnitNumber, nChannelNumber);

    return Initialize();
}


//note: bQuiet is obsolete; up to the caller now to check return value and throw message if appropriate
bool PdvInput::Initialize(bool bQuiet)
{
    bool rc;

    rc = m_pCurCamera->Open();

    if (!rc)
        return false;

#if 0
    /* Not sure why this is here,
    but it breaks unconfigured camera */

    if (!pdv_get_width(GetDevice()))
    {
        m_pCurCamera->Close();
        return false;
    }
#endif

    UpdateFromCamera();

    return true;
}



EdtImage * PdvInput::GetCameraImage(EdtImage *pImage)

{

    if (!pImage)
    {
        pImage = new EdtImage;
    }

    pImage->Create(GetType(), GetWidth(), GetHeight());

    if (GetDepth() > 8 &&
        GetDepth() < 24) {
            pImage->SetMinMax(0,(1 << GetDepth()) - 1);
    }

    return pImage;

}

void PdvInput::SetHardwareInvert(bool bInvert)
{
    pdv_invert(GetDevice(), bInvert);
}

bool PdvInput::GetHardwareInvert()
{
    return (bool)pdv_get_invert(GetDevice());
}

void PdvInput::SetFirstPixelCounter(bool bCounter)
{
    pdv_set_firstpixel_counter(GetDevice(), (int)bCounter);
}

bool PdvInput::GetFirstPixelCounter()
{
    return (bool)pdv_get_firstpixel_counter(GetDevice());
}

bool PdvInput::GetMinMaxShutter(int & low, int & high)
{
    low = pdv_get_min_shutter(GetDevice());
    high = pdv_get_max_shutter(GetDevice());
    if (low == high)
        return false;
    return true;
}

bool PdvInput::GetMinMaxGain(int & low, int & high)
{
    low = pdv_get_min_gain(GetDevice());
    high = pdv_get_max_gain(GetDevice());
    if (low == high)
        return false;
    return true;
}

bool PdvInput::GetMinMaxLevel(int & low, int & high)
{
    low = pdv_get_min_offset(GetDevice());
    high = pdv_get_max_offset(GetDevice());
    if (low == high)
        return false;
    return true;
}

void PdvInput::SetWaitForScreenUpdate(bool bState)
{
    m_bWaitForScreenUpdate = bState;

}

void PdvInput::SetStartAction(int action)
{

    if (GetDevice())
    {
        edt_startdma_action(GetDevice(),action);
    }

}

bool PdvInput::IsClink()
{
    if (GetDevice())
    {
        return (pdv_is_cameralink(GetDevice()));
    }
    return false;
}

bool PdvInput::IsFox()
{
    if (GetDevice())
    {
        return (edt_is_dvfox(GetDevice()));
    }
    return false;
}

void PdvInput::StartFrameCount()
{    m_dwImageCount = 0;
m_dwStartTime = edt_timestamp();
m_dwEndTime = 0;
}

void PdvInput::EndFrameCount()
{
    m_dwEndTime = edt_timestamp();    

    m_dwImageCount = 0;
    m_dwStartTime = 0;
    m_dwEndTime = 0;
}

void PdvInput::SetNBufs(int val)
{
    m_nBufs = val ;
    m_nBufs_userSet = 1 ;
}

void PdvInput::PrepareBuffers()
{

    StartFrameCount();

    //    if (m_nStarted > 0)
    //    {
    //	return;
    //    }

#if 0
    edt_set_debug(GetDevice(),1);
#endif

    if (!m_bSetup || !GetDevice())
    {    
        return;
    }

    if (!IsBuffering())
    {
        // Build list of images 
        AddBufferImages();
    }

    // Setup double buffering ring buffers


    // flush fifo and setup for mulitple acquires

    //    edt_msleep(50);

    m_bBuffering = true;

    SetContinuousCapture(true);

    return ;
}

void PdvInput::StartAcquire(int nTodo)
{

    int nToStart;
    int nTodoPerCamera = nTodo;

    // Start the io

    if (!m_bDoBufs)
    {

    }
    else if (m_nBufs > 3 && (m_nStartDelay == 0))
        nToStart = (m_nBufs - 2) ;
    else
        nToStart = (1) ;


    if (nTodo && nToStart > nTodo)
        nToStart = nTodo;

    if (pingpong)
        InitPingPong();

    fval_done = GetDevice()->dd_p->fval_done;


    SetActiveFlag(true);
    SetContinuousCapture(true);


    m_nAhead = 0;

#if 1
    TRACE("StartAcquire %d Bufs %d Donecount %d pdv->donecount %d %f\n",
        nTodo,
        m_nBufs,
        edt_done_count(GetDevice()), 
        GetDevice()->donecount, edt_timestamp());
#endif

    for (int cam = 0; cam < m_nCameras ; cam++)
    {
        SetCameraIndex(cam);
        m_pCurCamera->ResetCounts();
        SetTodo(nTodo);
        edt_reset_counts(GetDevice());
        edt_set_buffer(GetDevice(),0);

        pdv_setup_continuous(GetDevice());
        GetDevice()->donecount = edt_done_count(GetDevice());
        StartImages(nToStart);

    }

    SetCameraIndex(0);

}



EdtImage * PdvInput::GetLastImage()
{
    return m_pLastBufImage;
}

EdtImage * PdvInput::GetLastRawImage()
{
    return m_pLastRawImage;
}

void PdvInput::InitiateStop()

{
    if (GetDevice())
    {    
        SetActiveFlag(false);
    }
}

void PdvInput::StopAcquiring()
{

    // return if already stoppped


    if (GetDevice())
    {    


        SetActiveFlag(false);

        if (m_bFreerun)
        {
            pdv_start_image(GetDevice());


            pdv_stop_continuous(GetDevice());

        }

        PdvDependent *dd_p;

        dd_p = GetDevice()->dd_p;

        if (dd_p->fv_once)
        {
            int done;
            int todo;

            done = edt_done_count(GetDevice());
            todo = edt_get_todo(GetDevice());

            TRACE("done = %d todo = %d\n", done, todo);

            pdv_stop_hardware_continuous(GetDevice());
        }

    }

    EndFrameCount();

}

void PdvInput::Abort()
{

    // return if already stoppped

    if (!GetActiveFlag())
        return;

    if (GetDevice())
    {    


        SetActiveFlag(false);

        pdv_stop_continuous(GetDevice());

        edt_abort_current_dma(GetDevice());

    }

}

void PdvInput::StartImages(const int n)
{
    int actual_n = n;

    if (GetDevice())
    {

        if (GetTodo() && GetStarted() +  actual_n > GetTodo())
            actual_n = GetTodo() - GetStarted();

        if (m_nStartDelay)
            edt_msleep(m_nStartDelay);

        if (actual_n > 0)
        {
            if (m_bFreerun)
            {
                pdv_start_images(GetDevice(), 0);
            }
            else if (GetDevice()->dd_p->force_single)
            {
                pdv_start_images(GetDevice(), 1);
                IncStarted(1);
            }
            else
            {
                pdv_start_images(GetDevice(), actual_n);
                IncStarted(actual_n);
            }

#if 0
            printf("%x: Start %d Started %d Todo %d Done %d %f\n",
                this, actual_n, GetStarted(), GetTodo(), Done(),
                edt_timestamp());
#endif

        }
        else if (n == 0)
        {
            pdv_start_images(GetDevice(), 0);
        }

    }
}

byte * PdvInput::GetAllStartedImages()
{
    if (GetDevice())
    {
        // catch up


        //int delta = GetDevice()->donecount - edt_done_count(GetDevice());

        //edt_abort_current_dma(GetDevice());
        int curdone = edt_done_count(GetDevice()) ;
        int curtodo = edt_get_todo(GetDevice()) ;

        if (m_bNeedsExtraFrame)
        {
            curtodo --;

            if (curtodo < curdone)
                curtodo = curdone;

        }

        TRACE("Waiting for %d\n",
            curtodo - curdone);

        byte *data = pdv_wait_images_raw(GetDevice(), curtodo - curdone);

        SetDone(curtodo);
        SetStarted(curtodo);

        return data;

    }

    else

        return NULL;
}



bool PdvInput::BuffersPending()
{
    //    TRACE("BuffersPending %d > %d\n",
    //	m_nStarted, GetDone());

    return ((GetStarted() > GetDone()));
}

char *PdvInput::GetLoadedConfig()

{
    if (GetDevice())
        return GetDevice()->dd_p->cfgname;
    else
        return NULL;
}



void PdvInput::SetFreeRun(const bool bFreerun)
{
    m_bFreerun = bFreerun;

}


bool PdvInput::GetFreeRun()
{
    return m_bFreerun;

}

// HeaDer Data Functions


int PdvInput::GetHeaderSize()
{
    return (GetDevice())? pdv_get_header_size(GetDevice()) : 0;
}

int PdvInput::GetHeaderPosition()
{
    return (GetDevice())? pdv_get_header_position(GetDevice()) : 0;
}

int PdvInput::GetHeaderOffset()
{
    return (GetDevice())? pdv_get_header_offset(GetDevice()) : 0;
}

int PdvInput::GetHeaderDma()
{
    return (GetDevice())? pdv_get_header_dma(GetDevice()) : 0;
}

void PdvInput::SetHeaderSize(const int nSize)
{
    if (GetDevice()) 
        pdv_set_header_size(GetDevice(), nSize);
}

void PdvInput::SetHeaderPosition(const int nPosition)
{
    if (GetDevice()) 
        pdv_set_header_position(GetDevice(), (HdrPosition) nPosition);
}

void PdvInput::SetHeaderOffset(const int nOffset)
{
    if (GetDevice()) 
        pdv_set_header_offset(GetDevice(), nOffset);
}

void PdvInput::SetHeaderDma(const int nDma)
{
    if (GetDevice()) 
        pdv_set_header_dma(GetDevice(), nDma);
}

bool PdvInput::Pause()
{
    return false;
}

void PdvInput::Resume()
{

}

void PdvInput::GetROI(int & hskip, int & vskip, int & hactv, int & vactv)
{
    if (GetDevice())
    {
        hskip = GetDevice()->dd_p->hskip;
        vskip = GetDevice()->dd_p->vskip;
        hactv = GetDevice()->dd_p->hactv;
        vactv = GetDevice()->dd_p->vactv;
    }

}

void PdvInput::SetROI(int hskip, int vskip, int hactv, int vactv)
{

    if (!GetDevice())
        return;

    if (hactv == GetDevice()->dd_p->hactv &&
        vactv == GetDevice()->dd_p->vactv)
    {
        pdv_set_roi(GetDevice(),hskip,  hactv, vskip, vactv);

    }
    else
    {
        StopBuffering();

        pdv_set_roi(GetDevice(),hskip,  hactv, vskip, vactv);

        pdv_enable_roi(GetDevice(),true);

        PrepareBuffers();
    }

}

void PdvInput::DisableROI()
{

    StopBuffering();

    pdv_enable_roi(GetDevice(),false);

    PrepareBuffers();

}

bool PdvInput::HasROI()
{
    if (GetDevice())
    {
        if (GetDevice()->dd_p->xilinx_rev >= 2 && GetDevice()->dd_p->xilinx_rev < 32)
            return true;
    }

    return false;
}

bool PdvInput::HasBinning()
{
    if (GetDevice())
    {
        if (GetDevice()->dd_p->serial_binning[0])
            return true;
    }

    return false;
}

void PdvInput::GetCamSize(int & nWidth, int & nHeight)
{

    nWidth = pdv_get_cam_width(GetDevice());
    nHeight = pdv_get_cam_height(GetDevice());

}



bool PdvInput::IsBinarySerialCamera()
{
    //infer whether this camera uses a binary serial type of communication
    // method. Kind of kludgey but whatever...
    if ((GetDevice()->dd_p->camera_shutter_timing == SPECINST_SERIAL)
        || (GetDevice()->dd_p->camera_shutter_timing == SMD_SERIAL)
        || (GetDevice()->dd_p->serial_format == SERIAL_BINARY))
        return 1;
    return 0;

}

char * PdvInput::SerialTerminator()
{
    return (GetDevice()->dd_p->serial_term);
}

bool PdvInput::runSetupBat(char *szOutputFile)
{

    system(m_szBatchFile);

    // Check the results in the output file
    FILE * fOutput;

    if (!(fOutput = fopen(szOutputFile,"r")))
    {    
        ShowMessage("Camera setup failed open. See camsetup.out for error info");
        return false;
    }


    char c = '1';

    fscanf(fOutput,"%c", &c);


    m_bSetup = (c == '0');

    fclose(fOutput);

    if (!m_bSetup)
    {
        ShowMessage("Camera setup failed. See camsetup.out for error info.");
    }

    if (m_bSetup)
    {
        m_pCurCamera->Close();

        edt_msleep(500);


        Initialize();

    }

    return m_bSetup;

}


void PdvInput::SetBinning(int xval, int yval)
{
    if (!GetDevice())
        return;

    StopBuffering();

    int roictl = edt_reg_read(GetDevice(), PDV_ROICTL);
    TRACE("roictl = %x\n", roictl);
    if (pdv_set_binning(GetDevice(),xval, yval) == 0)
    {



    }

    roictl = edt_reg_read(GetDevice(), PDV_ROICTL);
    TRACE("roictl = %x\n", roictl);

    if (HasROI())
        pdv_enable_roi(GetDevice(),true);

    PrepareBuffers();

}


void PdvInput::GetBinning(int &xval, int &yval)
{

}

void PdvInput::InitVariables()
{
    m_bSetup = false;
    m_bContinuous = false;
    m_bInfraredMap = false;

    m_bDoBufs = true;
    m_nBufs = 4 /* PDV_BUFS */;
    m_nBufs_userSet = 0 ;

    m_bBuffering = false;

    m_dwImageCount = 0;

    m_dwStartTime = 0;
    m_dwEndTime = 0;

    m_bFreerun = false;
    m_pLiveLut = NULL;

    m_bWaitForScreenUpdate = false;

    m_nBufImages = 0;

    m_pBufImages = NULL;
    m_pRawImages = NULL;

    m_pLastBufImage = NULL;
    m_pLastRawImage = NULL;

    m_bNeedsExtraFrame = 0;

    m_bKeepUp = true;

    m_bWaitNext = 0;

    m_bNoReset = false;

    m_bTimeoutRecovery = false;


    m_bInputMethod = false;
    m_nLastInput = 0;
    m_bActiveFlag = false;

    m_bCheckOverrun = 0;

    m_bUseFrameHeight = true;

    m_bHeaderDma = 0;

    m_nStartDelay = 0;

    m_nCameras = 1;
    m_nCurrentCamera = 0;

    checksize = false;
    pingpong = false;
    fval_done = false;
    m_acquired_bufnum = 0;
    m_nDepth = 0;
    m_nTaps = 0;
    m_szBatchFile[0] = '\0';
    m_sLastError = "none";

}

void PdvInput::SetBayerParameters(int nSourceDepth,
                                  double scale[3],
                                  double gamma,
                                  int nBlackOffset,
                                  int bRedRowFirst,
                                  int bGreenPixelFirst,
                                  int quality,
                                  int bias,
                                  int gradientcolor)

{
    set_bayer_parameters(nSourceDepth,
        scale,
        gamma,
        nBlackOffset,
        bRedRowFirst,
        bGreenPixelFirst);

#ifdef USE_MMX

    // not running on Linux yet

    set_bayer_quality(quality);

    set_bayer_gradient_color(gradientcolor);

    set_bayer_bias_value(bias);

#endif


    if (GetDevice())
    {
        EdtPostProc *pCtrl;

        GetDevice()->dd_p->kbs_red_row_first = bRedRowFirst;
        GetDevice()->dd_p->kbs_green_pixel_first = bGreenPixelFirst;

        pCtrl = (EdtPostProc *) GetDevice()->pInterleaver;
        if (pCtrl)
            pCtrl->order =  (bRedRowFirst<<1) | bGreenPixelFirst;
    }
}

void PdvInput::GetBayerOrder(bool &bRedRowFirst, bool &bGreenPixelFirst)

{
    if (GetDevice())
    {
        bRedRowFirst = (bool) GetDevice()->dd_p->kbs_red_row_first; 
        bGreenPixelFirst = (bool) GetDevice()->dd_p->kbs_green_pixel_first;
    }

}

void PdvInput::SetBayerQuality(int nQuality, bool bGradientColor)

{
#ifdef USE_MMX
    set_bayer_quality(nQuality);

    set_bayer_gradient_color(bGradientColor);
#endif
}


int PdvInput::GetDevID()
{
    if (GetDevice())
    {
        return GetDevice()->devid;
    }
    else
        return 0;
}

int PdvInput::GetInterlaceType()
{
    if (GetDevice())
    {
        return GetDevice()->dd_p->swinterlace;
    }

    return 0;

}

int PdvInput::GetExtendedDepth()
{
    if (GetDevice())
    {
        return GetDevice()->dd_p->extdepth;
    }

    return 0;
}

void PdvInput::DalsaSetLSExposure(int hskip, int hactv)
{
    if (GetDevice())
        pdv_dalsa_ls_set_expose(GetDevice(), hskip, hactv);

}

void PdvInput::DalsaGetLSExposure(int &hskip, int &hactv)
{
    if (GetDevice())
    {
        hactv = GetDevice()->dd_p->hactv;
        hskip = GetDevice()->dd_p->hskip;
    }
}

void PdvInput::UpdateValuesFromCamera()
{
    pdv_update_values_from_camera(GetDevice());
}

void PdvInput::DVCSetMode(char *modestr)
{
    pdv_set_mode_dvc(GetDevice(), modestr);
}

void PdvInput::SetBayerOrder(bool bRedRowFirst, bool bGreenPixelFirst)
{
    GetDevice()->dd_p->kbs_red_row_first = bRedRowFirst;
    GetDevice()->dd_p->kbs_green_pixel_first = bGreenPixelFirst;
}



char * PdvInput::DVCGetMode(char *s)
{
    if (!s)
        s = (char *) malloc(4);
    if (GetDevice())
    {
        if (pdv_is_dvc(GetDevice()))
        {
            char error_message[80];
            char out_buf[80];
            int tries = 0;

            do 
            {

                SendCommand("MDE", false, error_message);

                ReadResponse(out_buf,false,12);

                // Response should be MDE\rMDE XXX\r"

                if (strlen(out_buf) == 12)
                {
                    strncpy(s,out_buf + 8,3);

                    TRACE("Success\n");
                    break;

                }
                else
                {
                    TRACE("Failure : ");
                    TRACE(out_buf);
                    TRACE("\n");

                }

                tries++;


            } while (tries < 3);

        }
    }

    return s;
}


bool PdvInput::HasDevice()
{
    if (!GetDevice())
    {
        if (!Initialize())
        {
            return false;
        }

    }

    return (GetDevice() != NULL);
}


bool PdvInput::IsMultiFrame()

{
    if (GetDevice())
        return (pdv_get_frame_height(GetDevice()) && 
        pdv_get_frame_height(GetDevice()) != pdv_get_height(GetDevice()));

    return false;

}


void PdvInput::ResetPingPong()
{
    u_int util3;

    PdvDev *pdv_pp_0;
    pdv_pp_0 = GetDevice(0);

    /* reset pingpong to channel 0 */
    util3 = edt_reg_read(pdv_pp_0, PDV_UTIL3);
    edt_reg_write(pdv_pp_0, PDV_UTIL3, util3 | PDV_PPRST);
    edt_reg_write(pdv_pp_0, PDV_UTIL3, util3 & ~PDV_PPRST);

}

void PdvInput::ClearPingPong()
{
    u_int util3;

    PdvDev *pdv_pp_0;
    pdv_pp_0 = GetDevice(0);

    if (!pdv_pp_0)
        return;

    util3 = edt_reg_read(pdv_pp_0, PDV_UTIL3);
    edt_reg_write(pdv_pp_0, PDV_UTIL3, util3 & ~PDV_PPENB);

}

void PdvInput::InitPingPong()

{
    PdvDev *pdv_pp_0;
    PdvDev *pdv_pp_1;
    u_int util3;


    if (m_nCameras != 2)
        return;

    pdv_pp_0 = GetDevice(0);
    pdv_pp_1 = GetDevice(1);

    /*
    * we want region of interest disabled in this case; do so here
    * in case it wasn't done by the config file
    */
    pdv_enable_roi(pdv_pp_0, 0);
    pdv_enable_roi(pdv_pp_1, 0);

    /* 
    * set frame valid done flag and enable ping pong bit (0x02) in
    * utility 3 register (special FW required)
    */
    pdv_set_fval_done(pdv_pp_0, 0);

    util3 = edt_reg_read(pdv_pp_0, PDV_UTIL3);
    edt_reg_write(pdv_pp_0, PDV_UTIL3, util3 & ~PDV_PPENB);
    edt_reg_write(pdv_pp_0, PDV_UTIL3, util3 | PDV_PPENB);

    pdv_set_fval_done(pdv_pp_1, 0);

    util3 = edt_reg_read(pdv_pp_1, PDV_UTIL3);
    edt_reg_write(pdv_pp_1, PDV_UTIL3, util3 & ~PDV_PPENB);
    edt_reg_write(pdv_pp_1, PDV_UTIL3, util3 | PDV_PPENB);

    ResetPingPong();

    pdv_set_fval_done(pdv_pp_0, 1);
    pdv_set_fval_done(pdv_pp_1, 1); 
}
