// EdtBGSubtractor.cpp: implementation of the CEdtBGSubtractor class.
//
//////////////////////////////////////////////////////////////////////

#include "edtinc.h"
#include "edtimage/EdtImage.h"
#include "EdtImgOp.h"

#include "EdtImgArith.h"

#include "EdtBGSubtractor.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEdtBGSubtractor::CEdtBGSubtractor() : EdtImgOp(NULL,NULL)
{
	m_pBGImage = NULL;
}

CEdtBGSubtractor::CEdtBGSubtractor(EdtImage *pSrc, EdtImage *pDest, EdtImage *pBG) :
		EdtImgOp(pSrc, pDest)

{
	m_pBGImage = pBG;
}

CEdtBGSubtractor::~CEdtBGSubtractor()
{

}


int CEdtBGSubtractor::Go()

{
	if (m_pSrc && m_pDest && m_pBGImage)
	{
		
		return edt_subtract_image(m_pSrc, m_pBGImage, m_pDest);

	}

	// Null Args Error

	return -1;

}

bool CEdtBGSubtractor::Ok()

{
	return (EdtImgOp::Ok() && m_pBGImage != NULL);
}

