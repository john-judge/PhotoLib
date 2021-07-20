// EdtImgDyadOp.h: interface for the CEdtImgDyadOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(EDTIMGDYADOP_H_INCLUDED_)
#define EDTIMGDYADOP_H_INCLUDED_

//  Base Class - 2 input, 1 output image operators

class CEdtImgDyadOp 
{

protected:

	EdtImage *m_pSrc1;
	EdtImage *m_pSrc2;
	EdtImage *m_pDest;

public:



	CEdtImgDyadOp();
	CEdtImgDyadOp(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);

	virtual int Setup(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);
	virtual int Go();

	virtual int DoOp(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);


	virtual ~CEdtImgDyadOp();

	virtual EdtImage *GetSrc1() { return m_pSrc1; }
	virtual EdtImage *GetSrc2() { return m_pSrc2; }
	virtual EdtImage *GetDest() { return m_pDest; }

	virtual void SetSrc1(EdtImage *pSrc1) { m_pSrc1 = pSrc1; }
	virtual void SetSrc2(EdtImage *pSrc2) { m_pSrc2 = pSrc2; }
	virtual void SetDest(EdtImage *pDest) { m_pDest = pDest; }

};

#endif // !defined(EDTIMGDYADOP_H_INCLUDED_)
