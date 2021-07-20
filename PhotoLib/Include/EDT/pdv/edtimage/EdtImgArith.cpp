
#include "edtinc.h"

#include "edtimage/EdtImage.h"

#include "EdtImgArith.h"


#define FUNC_BYTE_BYTE_BYTE FUNC_DYAD(EDT_TYPE_BYTE,EDT_TYPE_BYTE,EDT_TYPE_BYTE)
#define FUNC_USHORT_USHORT_USHORT FUNC_DYAD(EDT_TYPE_USHORT,EDT_TYPE_USHORT,EDT_TYPE_USHORT)

template <class T1, class T2, class T3> 
void AddBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] + Src2[row][i];
}

template <class T1, class T2, class T3> 
void SubBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] - Src2[row][i];
}

template <class T1, class T2, class T3> 
void MultBlockTyped(T1 **Src1, 
				   T2 **Src2,				   				   
				   T3 **Target, 
				   double scale,
				   int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] * Src2[row][i];
}

template <class T1, class T2, class T3> 
void DivideBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, 
				   double scale, 
				   int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = (T3) (scale * (Src1[row][i] - Src2[row][i]));
}

template <class T1, class T2, class T3> 
void AndBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] & Src2[row][i];
}

template <class T1, class T2, class T3> 
void OrBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] | Src2[row][i];
}

template <class T1, class T2, class T3> 
void XorBlockTyped(T1 **Src1, 
				   T2 **Src2,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] ^ Src2[row][i];
}

template <class T1, class T3> 
void NotBlockTyped(T1 **Src1, 
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = ~Src1[row][i];
}

//////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////

template <class T1, class T2, class T3> 
void AddKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] + k;
}

template <class T1, class T2, class T3> 
void SubKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] - k;
}
template <class T1, class T2, class T3> 
void MultKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = (T3)(Src1[row][i] * k);
}

template <class T1, class T2, class T3> 
void DivideKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = (Src1[row][i] / k);
}

template <class T1, class T2, class T3> 
void AndKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] & k;
}

template <class T1, class T2, class T3> 
void OrKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] | k;
}

template <class T1, class T2, class T3> 
void XorKTyped(T1 **Src1, 
				   T2 k,
				   T3 **Target, int nSize, int nRows)

{
	for (int row = 0; row < nRows; row++)
		for (int i=0;i<nSize;i++)
			Target[row][i] = Src1[row][i] ^ k;
}

static void resolve_sizes_3(EdtImage *pSrc1, 
				   EdtImage *pSrc2, 
				   EdtImage *pDest,
				   int &newwidth,
				   int &newheight)

{
	newwidth = pSrc1->GetWidth();

	if (pSrc2->GetWidth() < newwidth)
		newwidth = pSrc2->GetWidth();

	if (pDest->GetWidth() < newwidth)
		newwidth = pDest->GetWidth();

	newheight = pSrc1->GetHeight();

	if (pSrc2->GetHeight() < newheight)
		newheight = pSrc2->GetHeight();

	if (pDest->GetHeight() < newheight)
		newheight = pDest->GetHeight();
	
}

static void resolve_sizes_2(EdtImage *pSrc1, 
				   EdtImage *pDest,
				   int &newwidth,
				   int &newheight)

{
	newwidth = pSrc1->GetWidth();

	if (pDest->GetWidth() < newwidth)
		newwidth = pDest->GetWidth();

	newheight = pSrc1->GetHeight();

	if (pDest->GetHeight() < newheight)
		newheight = pDest->GetHeight();
	
}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_add_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest)

{
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		AddBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		AddBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_subtract_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest)

{
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		SubBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		SubBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_mult_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest, double scale)

{
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		MultBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						scale,
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		MultBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						scale,
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_divide_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest, double scale)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		DivideBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						scale,
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		DivideBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						scale,
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_and_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		AndBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		AndBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_or_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		OrBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		OrBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_xor_image(EdtImage *pSrc1, EdtImage *pSrc2, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_3(pSrc1, pSrc2, pDest, width, height);

	int ftype = FUNC_DYAD(pSrc1->GetType(), pSrc2->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE_BYTE:
		
		XorBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pSrc2->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT_USHORT:
		
		XorBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pSrc2->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

///////////////////////////////////////////////
//
///////////////////////////////////////////////

int edt_add_image_k(EdtImage *pSrc1, int k, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		AddKTyped(( byte ** )pSrc1->GetPixelRows(),
						k,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		AddKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						k,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

int edt_subtract_image_k(EdtImage *pSrc1, int k, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		SubKTyped(( byte ** )pSrc1->GetPixelRows(),
						k,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		SubKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						k,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}


int edt_mult_image_k(EdtImage *pSrc1, double scale, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		MultKTyped(( byte ** )pSrc1->GetPixelRows(),
						scale,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		MultKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						scale,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}



int edt_and_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		AndKTyped(( byte ** )pSrc1->GetPixelRows(),
						k,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		AndKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						k,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}


int edt_or_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		OrKTyped(( byte ** )pSrc1->GetPixelRows(),
						k,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		OrKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						k,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}

int edt_xor_image_k(EdtImage *pSrc1, unsigned int k, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		XorKTyped(( byte ** )pSrc1->GetPixelRows(),
						k,
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		XorKTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						k,
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}



int edt_not_image(EdtImage *pSrc1, EdtImage *pDest)

{
	
	int width, height;

	int error = 0;

	resolve_sizes_2(pSrc1, pDest, width, height);

	int ftype = FUNC_MONAD(pSrc1->GetType(), pDest->GetType());

	switch (ftype) {

	case FUNC_BYTE_BYTE:
		
		NotBlockTyped(( byte ** )pSrc1->GetPixelRows(),
						( byte ** )pDest->GetPixelRows(),
						width, height);

		break;

	case FUNC_USHORT_USHORT:
		
		NotBlockTyped(( ushort_t ** )pSrc1->GetPixelRows(),
						( ushort_t ** )pDest->GetPixelRows(),
						width, height);

		break;
	

	default:

		error = -1; // unsupported types

	}

	return error;

}
