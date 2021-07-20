// EdtImgop.h: interface for the CEdtImgop class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_EDTIMGOP_H_INCLUDED_)
#define _EDTIMGOP_H_INCLUDED_


class EdtImgOp 
{

protected:

	EdtImage *m_pSrc;
	EdtImage *m_pDest;

public:



	EdtImgOp();

	EdtImgOp(EdtImage *pSrc, EdtImage *pDest);

	
	virtual int Setup(EdtImage *pSrc, EdtImage *pDest);
	virtual int Go();

	virtual int DoOp(EdtImage *pSrc, EdtImage *pDest);


	virtual ~EdtImgOp();

	virtual EdtImage *GetSrc() { return m_pSrc; }
	virtual EdtImage *GetDest() { return m_pDest; }

	virtual void SetSrc(EdtImage *pSrc) { m_pSrc = pSrc; }
	virtual void SetDest(EdtImage *pDest) { m_pDest = pDest; }

	virtual bool Ok();
};

#endif // !defined(_EDTIMGOP_H_INCLUDED_)
