// BufferTimerData.cpp: implementation of the BufferTimerData class.
//
//////////////////////////////////////////////////////////////////////

extern "C" {
#include "edtinc.h"
}

#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "BufferTimerData.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define no_printf

BufferTimerData::BufferTimerData()
{
	m_pTimeVec = NULL;
	m_pBufVec = NULL;

	m_nLastBuffer = 0;
	m_nStartBuffer = 0;
	m_dStartTime = 0.0;
	m_nCurBuffer = 0;

	m_nAllocMask = 0x1ff;

}

BufferTimerData::~BufferTimerData()
{
	if (m_pTimeVec)
	{
		free(m_pTimeVec);
		m_pTimeVec = NULL;
	}
	if (m_pBufVec)
	{
		free(m_pBufVec);
		m_pBufVec = NULL;
	}
}

void BufferTimerData::AllocateTimes(const int nMaxCount)

{
	int n = nMaxCount;

	if (!m_pTimeVec)
		m_pTimeVec = (double *) calloc(1, (m_nAllocMask + 1) * sizeof(double));
	if (!m_pBufVec)
		m_pBufVec = (int *) calloc(1, (m_nAllocMask + 1) * sizeof(int));


}

double BufferTimerData::GetFrameRate()
{
	
	double dtime;
	
	if (m_nLastBuffer >= 0 && m_pTimeVec)
	{	

		int n = 100;
		int samplebuf = m_nCurBuffer - 1;

		int startn = samplebuf - n;

		if (startn < 0)
		  {
			startn = 0;
		  }

		if (samplebuf == 0)
		  return 0.0;

		
		int nWhich = samplebuf & m_nAllocMask;
		startn &= m_nAllocMask;
				
		dtime = m_pTimeVec[nWhich] - m_pTimeVec[startn];
		n = m_pBufVec[nWhich] - m_pBufVec[startn];

		if (dtime)
		{
		  no_printf("buffertimerdata: (%d -%d) n/dtime = %d/%f = %f\n",
				 nWhich, startn, n,dtime,n/dtime);
		  return (n) / dtime;
		}
	}
	else
	{
		int n = m_nLastBuffer - m_nStartBuffer;

		dtime = edt_timestamp() - m_dStartTime;

		if (dtime)
			return (n) / dtime;

	}


	return 0.0;
}
