// EdtImgop.cpp: implementation of the CEdtImgop class.
//
//////////////////////////////////////////////////////////////////////

#include "edtinc.h"
#include "edtimage/EdtImage.h"

#include "EdtImgOp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtImgOp::EdtImgOp()
{
	SetSrc(NULL);
	SetDest(NULL);

}

EdtImgOp::EdtImgOp(EdtImage *pSrc, EdtImage *pDest)
{
	SetSrc(pSrc);
	SetDest(pDest);
	
}

EdtImgOp::~EdtImgOp()
{

}


int EdtImgOp::DoOp(EdtImage *pSrc, EdtImage *pDest)

{
	Setup(pSrc,pDest);
	return Go();
}

int EdtImgOp::Setup(EdtImage *pSrc, EdtImage *pDest)

{
	SetSrc(pSrc);
	SetDest(pDest);

	return 0;
}

bool EdtImgOp::Ok()

{
	return (m_pSrc != NULL && m_pDest != NULL);
}

int  EdtImgOp::Go()

{
	return 0; // no op
}
