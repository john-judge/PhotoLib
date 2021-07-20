// EdtImgDyadOp.h: interface for the CEdtImgDyadOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(EDTIMGDYADOP_H_INCLUDED_)
#define EDTIMGDYADOP_H_INCLUDED_


int edt_add_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);

int edt_subtract_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);

int edt_mult_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest, double scale);

int edt_divide_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest, double scale);


int edt_and_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);

int edt_or_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);

int edt_xor_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest);


int edt_add_image_k(EdtImage *pSrc1, int k, EdtImage *pDest);

int edt_subtract_image_k(EdtImage *pSrc1, int k, EdtImage *pDest);

int edt_mult_image_k(EdtImage *pSrc1, double scale, EdtImage *pDest);


int edt_and_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest);

int edt_or_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest);

int edt_xor_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest);


int edt_not_image(EdtImage *pSrc1, EdtImage *pDest);


#endif // !defined(EDTIMGDYADOP_H_INCLUDED_)
