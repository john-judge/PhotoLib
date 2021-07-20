// BufferTimerData.h: interface for the BufferTimerData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUFFERTIMERDATA_H__77FF4793_24EC_11D4_B864_003048100300__INCLUDED_)
#define AFX_BUFFERTIMERDATA_H__77FF4793_24EC_11D4_B864_003048100300__INCLUDED_

class BufferTimerData  
{

	struct FrameStamp {
		int frame;
		double timestamp;
	};

	FrameStamp *pStampVector;

	
public:

	BufferTimerData();

	virtual ~BufferTimerData();

	double m_dStartTime;

	double *m_pTimeVec;
	int *m_pBufVec; 

	int m_nLastBuffer;
	int m_nCurBuffer;

	int m_nAllocMask;
	int m_nStartBuffer;


	void AllocateTimes(const int nMaxCount);

	void Start()
	{
		m_nLastBuffer = -1;
		m_nStartBuffer = -1;

		m_nCurBuffer = 0;

		AllocateTimes(1);
	}

	void SetBufferTime(int nBuffer, const double dtime)
	{
		if (!m_pTimeVec)
			AllocateTimes(1);
		
		if (m_nLastBuffer == -1)
			m_nStartBuffer = nBuffer;

		int tbuf = m_nCurBuffer & m_nAllocMask;

		//printf("Setting %d to %d %f\n",m_nCurBuffer,nBuffer,dtime);

		m_pTimeVec[tbuf] = dtime;
		m_pBufVec[tbuf] = nBuffer;

		m_nCurBuffer ++ ;

		m_nLastBuffer = nBuffer;

	}

	double GetBufferTime(int nBuffer)
	{
		if (m_pTimeVec)
			return m_pTimeVec[nBuffer & m_nAllocMask];
		
		return -1.0;
	}
	
	double GetFrameRate();

	void StartTimer()
	{
		m_dStartTime = edt_timestamp();
	}

	double ElapsedTime()
	{
		return edt_timestamp() - m_dStartTime;
	}

};

#endif // !defined(AFX_BUFFERTIMERDATA_H__77FF4793_24EC_11D4_B864_003048100300__INCLUDED_)
