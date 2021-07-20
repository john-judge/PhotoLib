// PdvInput.h : header file
//
// PdvInput class
//

#ifndef __CAMERA__
#define __CAMERA__

extern "C" {
#include "edtinc.h"
} 
// Defaults


#include "edtdefines.h"

#include "edtimage/EdtImage.h"

#define MAX_CAMERAS 8

class ChannelDescription {
    public:

        PdvDev *m_hPdv;

        char m_sCameraSelect[256];
        char m_sCamFile[256];
        char m_sCamConfigPath[256];
        char m_sDevName[32];
        char m_sDevIdString[32];
        char m_sDriverVer[32];
        char m_sLibraryVer[32];
        char m_sPromDesc[32];
        char m_sBusDesc[32];
        char m_sFpgaDesc[32];
        char m_sFpgaMfg[32];
        char m_sSerialNumStr[32];
        char m_sPartNumStr[32];
        int m_nDevId;
        int m_nUnitNumber;
        int m_nChannelNumber;
        bool m_bOwnPDV;

        bool m_Dbg;

        int m_nStarted;
        int m_nTodo;
        int m_nDone;
        int m_nSkipped;

        int m_nAcquiredWidth;
        int m_nAcquiredHeight;

        int nextwidth;
        int nextheight;

        ChannelDescription()
        {
            m_hPdv = NULL;
            m_nUnitNumber=0;
            m_nChannelNumber = 0;
            m_nDevId = -1;
            m_Dbg = false;
            m_bOwnPDV = false;
            m_sCameraSelect[0] = 0;
            m_sCamFile[0] = 0;
            m_sCamConfigPath[0] = 0;
            m_sBusDesc[0] = 0;
            m_sPromDesc[0] = 0;
            m_sFpgaDesc[0] = 0;
            m_sFpgaMfg[0] = 0;
            m_sSerialNumStr[0] = 0;
            m_sPartNumStr[0] = 0;

            strcpy(m_sDevName,EDT_INTERFACE);
            strcpy(m_sDevIdString,EDT_INTERFACE);
            strcpy(m_sDriverVer, "unknown");
            strcpy(m_sLibraryVer, "unknown");
        }

        void SetDevUnitChannel(const char *devname, const int unit, const int channel)
        {

            m_nUnitNumber = unit;
            m_nChannelNumber = channel;

            if (devname) 
            {
                if (!strcmp(devname,"dmy"))
                    m_Dbg = true;
                else
                    m_Dbg = false;

                strncpy(m_sDevName, devname, sizeof(m_sDevName)-1);
            }	


        }

        void Close()
        {
            if (m_bOwnPDV && m_hPdv)
            {
                pdv_close(m_hPdv);
            }
            m_hPdv = NULL;
        }

        bool Open()

        {
            if (m_hPdv)
            {    
                return true;
            }

            // Open a handle to the device   
            m_hPdv = pdv_open_channel(m_sDevName, m_nUnitNumber, m_nChannelNumber);
            if (m_hPdv == NULL)
            {
                return false;
            }

            u_short dmy;
            int promcode =  edt_flash_prom_detect(m_hPdv, &dmy);
            Edt_prominfo *pi = edt_get_prominfo(promcode);
            EdtPromData pdata;
            Edt_embinfo ei;


            edt_read_prom_data(m_hPdv, promcode, pi->defaultseg, &pdata);
            edt_parse_devinfo(pdata.esn, &ei);

            strcpy(m_sDevIdString, edt_idstring(m_hPdv->devid, promcode));

            if (edt_is_simulator(m_hPdv))
            {
                pdv_close(m_hPdv);
                m_hPdv = NULL;
                return false;
            }

            m_bOwnPDV = true ;

            m_nDevId = m_hPdv->devid;

            edt_get_driver_version(m_hPdv, m_sDriverVer, sizeof(m_sDriverVer));
            edt_get_library_version(m_hPdv, m_sLibraryVer, sizeof(m_sLibraryVer));

            strncpy(m_sFpgaDesc, pi->fpga, 31);
            strncpy(m_sBusDesc, pi->busdesc, 31);
            strncpy(m_sPromDesc, pi->promdesc, 31);
            strncpy(m_sFpgaMfg, edt_get_fpga_mfg(m_hPdv), 31);
            strncpy(m_sSerialNumStr, ei.sn, 31);
            edt_fmt_pn(ei.pn, m_sPartNumStr);

            return true;
        }

        void SetConfigName(char *CamFile)

        {
            strcpy(m_sCamFile , CamFile);
            strcpy(m_sCamConfigPath,m_sCamFile);

            size_t cp = strlen(m_sCamConfigPath);

            while (cp > 0 && m_sCamConfigPath[cp] != '/' && m_sCamConfigPath[cp] != '\\')
                cp--;

            m_sCamConfigPath[cp] = '\0';

        }

        void ResetCounts()
        {
            m_nStarted = 0;
            m_nDone = 0;
            m_nSkipped = 0;
            if (m_hPdv)
            {
                nextwidth = m_nAcquiredWidth = pdv_get_width(m_hPdv);
                nextheight = m_nAcquiredHeight = pdv_get_height(m_hPdv);
            }

        }


};

class PdvInput : public EdtImageData

{

    private:

        int m_nCameras;
        int m_nCurrentCamera;    
        bool m_bCycleCamera;
        std::string m_sLastError;

        ChannelDescription m_Cameras[MAX_CAMERAS];
        ChannelDescription *m_pCurCamera;

        // Attributes
    protected:
        byte * m_pLiveLut;

        EdtImage **m_pBufImages;
        EdtImage **m_pRawImages;

        bool m_bInputMethod;
        bool m_bDoBufs ;
        int m_nBufs ;
        int m_nBufs_userSet ;
        bool m_bSetup;
        bool m_bContinuous;
        bool m_bInfraredMap;

        bool m_bBuffering;
        bool m_bNoReset;

        // true if in a timeout recovery
        bool m_bTimeoutRecovery; 

        long m_dwImageCount;
        double m_dwStartTime;
        double m_dwEndTime;
        char m_szBatchFile[_MAX_PATH];

        double m_dLastTime;

        void ExecuteSetup();


        bool    m_bHeaderDma;

        int	m_nStartDelay;

        byte ** m_pBufPointers;


        EdtImage *m_pLastBufImage;
        EdtImage *m_pLastRawImage;

        int m_nBufImages;
        u_int m_nLastInput;
        int m_nAhead;

        bool m_bWaitNext;
        bool m_bCheckOverrun;
        bool m_bUseFrameHeight;



        bool m_bTimedOut;
        bool m_bWaitForScreenUpdate;

        int m_nStatus; // What's happening

        EdtDataType m_nRawType;

        bool m_bActiveFlag;

        bool checksize;
        bool pingpong;

        bool fval_done;

        int m_acquired_bufnum;

    public:
        // Construction / Destruction

        PdvInput(const char *devname = NULL, 
                const int nUnit = 0,
                const int nChannel = 0);

        PdvInput(PdvDev *pdv_p);

        virtual ~PdvInput();



        // Implementation

        byte * GetAllStartedImages();
        void StartImages(const int n);

        void AttachBufferImage(int nIndex, byte *pData);
        void AttachRawImage(int nIndex, byte *pData);
        char * GetLoadedConfig();
        int GetFrameCount()
        {	
            return m_dwImageCount;
        }
        bool HasDevice();

        double GetFrameRate();
        void SetLiveLut(byte *pLut= NULL);
        char* GetCameraType();

        bool CanCaptureImage();
        void DestroyBufferImages();
        bool AddBufferImages();

        void UpdateFromCamera();

        int GetTimeouts();

        bool IsSetup();
        void PreSetup();

        bool Setup(const char* szCamCfgPath,
                const char* szCamCfgFile);

        bool Initialize(bool bQuiet = false); // bQuiet is obsoloete

        bool Initialize(
                const char *szInterface, 
                const int nUnitNumber = 0,
                const int nChannelNumber = 0);

        bool SendCommand(const char* command, int ishex, char *outbuf);
        int ReadResponse(char *outbuf, int ishex, int expected_chars = 10);

        int  IncShutterSpeed();
        int  DecShutterSpeed();


        int  GetUnitNumber()
        {
            return m_pCurCamera->m_nUnitNumber;
        }

        int  GetChannelNumber()
        {
            return m_pCurCamera->m_nChannelNumber;
        }

        char *GetDevName()
        {
            return m_pCurCamera->m_sDevName;
        }

        char *GetDevIdStr()
        {
            return m_pCurCamera->m_sDevIdString;
        }

        char *GetLibraryVersion()
        {
            return m_pCurCamera->m_sLibraryVer;
        }

        char *GetDriverVersion()
        {
            return m_pCurCamera->m_sDriverVer;
        }

        bool SetShutterSpeed(int);
        bool GetShutterSpeed(int&);

        bool SetGain(int);
        bool GetGain(int&);

        bool SetLevel(int);
        bool GetLevel(int&);

        bool SetInfraredMap(bool);
        bool GetInfraredMap(bool&);

        bool IsCapturingImages();
        bool ToggleContinuousCapture();

        bool SetContinuousCapture(bool);
        bool GetContinuousCapture(bool&);

        void SetCheckOverrun(const bool newstate)
        {
            m_bCheckOverrun = newstate;
        }
        bool GetCheckOverrun() const
        {
            return m_bCheckOverrun;
        }

        int GetSkipped() const
        {
            return m_pCurCamera->m_nSkipped;
        }
        void SetSkipped(const int n)
        {
            m_pCurCamera->m_nSkipped = n;
        }

        int GetDone() const
        {
            return m_pCurCamera->m_nDone;
        }

        void SetTodo(const int n)
        {
            m_pCurCamera->m_nTodo = n;
        }
        int GetTodo() const
        {
            return m_pCurCamera->m_nTodo;
        }

        void SetDone(const int n)
        {
            m_pCurCamera->m_nDone = n;
        }

        void SetAcquiredSize(const int w, const int h)
        {
            m_pCurCamera->m_nAcquiredWidth = w;
            m_pCurCamera->m_nAcquiredHeight = h;
        }

        int GetAcquiredWidth() const
        {
            return m_pCurCamera->m_nAcquiredWidth;
        }
        int GetAcquiredHeight() const
        {
            return m_pCurCamera->m_nAcquiredHeight;
        }

        void SetStarted(const int n)
        {
            m_pCurCamera->m_nStarted = n;
        }

        void IncStarted(const int n)
        {
            m_pCurCamera->m_nStarted += n;
        }

        int GetStarted() const
        {
            return m_pCurCamera->m_nStarted;
        }


        void SetUseFrameHeight(const bool newstate)
        {
            m_bUseFrameHeight = newstate;
        }
        bool GetUseFrameHeight() const
        {
            return m_bUseFrameHeight;
        }


        bool CaptureImage(EdtImage *pImage);

        bool FillBuffer(BYTE*, unsigned long);
        bool NextFileIndex(int&);
        bool StartBuffering();
        void StopBuffering();
        bool GetNextBuffer(BYTE*&);
        bool IsBuffering();
        bool DoBufs();
        void StartNext();
        void SetNBufs(int bufs) ;

        int  GetNBufs() const
        {
            return m_nBufs;
        }

        void SetDoBufs(bool doit);
        void StartFrameCount();
        void ShowFrameCount();
        void EndFrameCount();

        void IncrFrameCount() {m_dwImageCount++;}

        EdtImage *GetNextBufferImage();

        void GetBinning(int &xval, int &yval);
        void SetBinning(int xval, int yval);
        bool m_bKeepUp;
        bool runSetupBat(char *szOutputFile);
        char * SerialTerminator();
        bool IsBinarySerialCamera();
        bool m_bNeedsExtraFrame;
        void DisableROI();
        void GetCamSize(int &nWidth, int &nHeight);

        bool HasROI();
        bool HasBinning();

        void SetROI(const int hskip, const int vskip, const int hactv, const int vactv);
        void GetROI(int &hskip, int &vskip, int &hactv, int &vactv);
        void Resume();
        bool Pause();

        bool GetFreeRun();
        void SetFreeRun(const bool bFreerun = false);
        bool SelectConfigFile(bool bAllowNew);
        bool BuffersPending();
        bool GetConfigFile();

        void InitiateStop();
        void StopAcquiring();

        EdtImage * GetLastImage();
        EdtImage * GetLastRawImage();

        void AllocateBuffers();
        void StartAcquire(int nTodo = 0);
        void PrepareBuffers();

        bool IsClink();
        bool IsFox();
        void SetStartAction(int action);

        bool m_bFreerun;

        void SetWaitForScreenUpdate(bool bState);
        bool GetMinMaxLevel(int &low, int &high);
        bool GetMinMaxGain(int &low, int &high);
        bool GetMinMaxShutter(int &low, int &high);
        void SetHardwareInvert(bool bInvert);
        bool GetHardwareInvert();
        void SetFirstPixelCounter(bool bInvert);
        bool GetFirstPixelCounter();

        EdtImage * GetCameraImage(EdtImage *pImage = NULL);

        double GetLastTime() 
        {
            return m_dLastTime;
        }

        void InitVariables();

        void SetCameraStrings(const char *Select, const char * FileName, const char *ConfigPath)
        {
            strcpy(m_Cameras[m_nCurrentCamera].m_sCameraSelect,Select);
            strcpy(m_Cameras[m_nCurrentCamera].m_sCamFile,FileName);
            strcpy(m_Cameras[m_nCurrentCamera].m_sCamConfigPath,ConfigPath);
        }

        void SetCamConfigPath(const char *s)
        {
            strcpy(m_Cameras[m_nCurrentCamera].m_sCamConfigPath,s);
        }

        const char * GetCameraSelect() const {return m_Cameras[m_nCurrentCamera].m_sCameraSelect;}
        const char * GetCamFile() const {return m_Cameras[m_nCurrentCamera].m_sCamFile;}
        const char * GetCamConfigPath() const {return m_Cameras[m_nCurrentCamera].m_sCamConfigPath;}

        bool IsMultiFrame();

        void InitInstance();

        int GetDbg();

        char* GetCameraClass();

        char* GetCameraModel();

        void SetUnitNumber(int);
        void ResetLastFileType();
        int GetLastFileType();

        void ResetLastSaveType();
        void SetLastSaveType();

        bool GetLastSaveType();

        int GetUserCamera(bool bUseCurrent);
        void SetTimedOut(bool nTimedOut) 
        {
            m_bTimedOut = nTimedOut;
        }

        bool GetTimedOut() const 
        {
            bool rc = m_bTimedOut;
            return rc;
        }

        void SetActiveFlag(bool nActiveFlag) 
        {
            m_bActiveFlag = nActiveFlag;
        }

        bool GetActiveFlag() const 
        {
            bool rc = m_bActiveFlag;
            return rc;
        }

        virtual int GetHeaderSize(); 
        virtual int GetHeaderOffset();
        virtual int GetHeaderPosition();
        virtual int GetHeaderDma();

        virtual void SetHeaderSize(const int n);
        virtual void SetHeaderOffset(const int n);
        virtual void SetHeaderPosition(const int n);
        virtual void SetHeaderDma(const int n);

        virtual int GetWidth(); 
        virtual int GetHeight(); 
        virtual int GetFrameHeight(); // frame_height for multi-frame images
        virtual int GetDepth(); 
        virtual int GetNumTaps(); 
        virtual int GetImageSize();

        int SetPixelTypeFromDepth();
        int SetRawTypeFromDepth();

        int GetBinX();
        int GetBinY();

        void DalsaSetLSExposure(int hskip, int hactv);

        void DalsaGetLSExposure(int &hskip, int &hactv);

        void UpdateValuesFromCamera();

        void DVCSetMode(char *modestr);

        char *DVCGetMode(char *modestr = NULL);

        void SetBayerOrder(bool bRedRowFirst, bool bGreenPixelFirst);

        void SetBayerParameters(int nSourceDepth,
                double scale[3],
                double gamma,
                int nOffset,
                int bRedRowFirst,
                int bGreenPixelFirst,
                int quality = 0,
                int bias = 0,
                int gradientcolor = 0);

        void GetBayerOrder(bool &bRedRowFirst, bool &bGreenPixelFirst);

        void SetBayerQuality(int nQuality, bool bGradientColor);

        int GetDevID();

        int GetInterlaceType();

        int GetExtendedDepth();

        char *GetWhiteBalanceFile();
        char *GetCCMFile();


        int GetDoneCount()
        {
            if (GetDevice())
                return GetDevice()->donecount;
            else
                return 0;
        }

        int GetShutterTiming()
        {
            if (GetDevice() && GetDevice()->dd_p)
                return GetDevice()->dd_p->camera_shutter_timing;
            else
                return 0;
        }

        int SerialWait(int msecs, int maxchars)
        {
            return pdv_serial_wait(GetDevice(), msecs, maxchars);
        }

        int SerialWaitNext(int msecs, int maxchars)
        {
            return pdv_serial_wait_next(GetDevice(), msecs, maxchars);
        }

        int SerialBinaryCommand(char *cmd, int len)
        {
            return pdv_serial_binary_command(GetDevice(), cmd, len);
        }

        int SerialCommand(char *cmd)
        {
            return pdv_serial_command(GetDevice(), cmd);
        }

        int SerialCommandHex(char *cmd, int len)
        {
            return pdv_serial_command_hex(GetDevice(), cmd, len);
        }

        int SerialReadResponse(char *buf)
        {
            return pdv_read_response(GetDevice(), buf);
        }

        int  GetDevId() const
        {
            return m_Cameras[m_nCurrentCamera].m_nDevId;
        }

        char *GetPromDesc()
        {
            return m_Cameras[m_nCurrentCamera].m_sPromDesc;
        }

        char *GetBusDesc()
        {
            return m_Cameras[m_nCurrentCamera].m_sBusDesc;
        }

        char *GetFpgaDesc()
        {
            return m_Cameras[m_nCurrentCamera].m_sFpgaDesc;
        }

        char *GetFpgaMfg()
        {
            return m_Cameras[m_nCurrentCamera].m_sFpgaMfg;
        }

        char *GetSerialNumStr()
        {
            return m_Cameras[m_nCurrentCamera].m_sSerialNumStr;
        }

        char *GetPartNumStr()
        {
            return m_Cameras[m_nCurrentCamera].m_sPartNumStr;
        }

        int GetNAhead() const
        {
            return m_nAhead;
        }


        void Abort();

        void SetRawType(EdtDataType newtype)
        {
            m_nRawType = newtype;
        }

        EdtDataType GetRawType() const
        {
            return m_nRawType;
        }

        int GetStartDelay() const
        {
            return m_nStartDelay;
        }

        void SetStartDelay(const int newstart)
        {
            m_nStartDelay = newstart;
        }

        void SetCameraIndex(const int newindex)
        {

            if (newindex == m_nCurrentCamera && m_pCurCamera)
                return;

            if (newindex >= 0 && newindex < m_nCameras)
            {
                m_nCurrentCamera = newindex;
                m_pCurCamera = &m_Cameras[m_nCurrentCamera];
            }
            else
            {
                // ERROR
            }
        }

        void StepCameraIndex()
        {
            if (m_nCameras > 1)
            {
                int next = (m_nCurrentCamera + 1) % m_nCameras;
                SetCameraIndex(next);
            }
        }

        void SetCycleCamera(const bool newstate)
        {
            m_bCycleCamera = newstate;
        }

        bool GetCycleCamera() const
        {
            return m_bCycleCamera;
        }

        int GetCameraIndex() const
        {
            return m_nCurrentCamera;
        }

        void AddCamera(char *name, const int unit, const int channel);

        void AddCamera(PdvDev *pdv);

        PdvDev *GetDevice()
        {
            return (m_pCurCamera)?(m_pCurCamera->m_hPdv):NULL;
        }

        PdvDev *GetDevice(const int which)
        {

            if (which >= 0 && which < m_nCameras)
            {
                return  m_Cameras[which].m_hPdv;
            }
            else
            {
                return NULL;
                // ERROR
            }	
        }

        void SetNCameras(const int n)
        {
            m_nCameras = n;
        }
        int GetNCameras() const
        {
            return m_nCameras;
        }

        void ResetPingPong();

        void InitPingPong();

        void ClearPingPong();

        bool PingPong() const
        {
            return pingpong;
        }

        void SetPingPong(const bool state)
        {
            pingpong = state;
            if (pingpong)
                SetCheckSize(true);
            else
                ClearPingPong();
        }

        // check xferred size - works w/ camera link only

        void SetCheckSize(const bool state)
        {
            checksize = state;
        }

        bool CheckSize() const
        {
            return checksize;
        }

        void SetImageToCamera(EdtImage *pImage)

        {
            if (pImage)
            {
                pImage->Create(GetType(), GetWidth(), GetHeight());

                pImage->SetMinMax(GetMinValue(),GetMaxValue());
            }
        }

        void setLastError(const std::string &str)
        {
            m_sLastError = str;
        }

        void setLastError(const char *str)
        {
            m_sLastError = str;
        }

        std::string lastError()
        {
            return m_sLastError;
        }

};



#endif // __CAMERA__
