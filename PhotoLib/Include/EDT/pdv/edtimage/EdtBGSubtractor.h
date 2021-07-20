// EdtBGSubtractor.h: interface for the CEdtBGSubtractor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDTBGSUBTRACTOR_H__7233553C_657D_491E_B16E_D1DDE9DFE74C__INCLUDED_)
#define AFX_EDTBGSUBTRACTOR_H__7233553C_657D_491E_B16E_D1DDE9DFE74C__INCLUDED_

#include "EdtImgOp.h"

class CEdtBGSubtractor : public EdtImgOp  
{

protected:

	EdtImage *m_pBGImage;

public:

	CEdtBGSubtractor();
	CEdtBGSubtractor(EdtImage *pSrc, EdtImage *pDest, EdtImage *pBG);

	void SetBGImage(EdtImage *pBG) {m_pBGImage = pBG;}
	EdtImage *GetBGImage() { return m_pBGImage; }

	virtual int Go();
	virtual bool Ok();

	virtual ~CEdtBGSubtractor();

};

#endif // !defined(AFX_EDTBGSUBTRACTOR_H__7233553C_657D_491E_B16E_D1DDE9DFE74C__INCLUDED_)
