/**
 *  EdtImgStats.h: interface for the EdtImgStats class.
 *
*/

#if !defined(EDTIMGSTATS_H)
#define EDTIMGSTATS_H

// basic one-d statisticsof integer values

class EdtImgStats  
{

public:

	int 	m_nN,
			m_nMin,
			m_nMax,
			m_nMode,
			m_nMedian,
            m_nModeFreq;

	double m_dMean,
			m_dSum,
			m_dVariance,
			m_dSSQ,
			m_dSD;

	EdtImgStats()
	{
		Clear();
	}

	void Clear()
	{

		m_dMean	= 0.0;
		m_dSum	= 0.0;
		m_dVariance	= 0.0;
		m_dSSQ = 0.0;
		m_nN = 0;
		m_nMin = 0;
		m_nMax = 0;
		m_nMode = 0;
        m_nModeFreq = 0;
		m_nMedian = 0;
		

	}

};

#endif // 
