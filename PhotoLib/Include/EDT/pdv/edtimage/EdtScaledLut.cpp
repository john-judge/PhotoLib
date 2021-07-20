// EdtScaledLut.cpp: implementation of the EdtScaledLut class.
//
//////////////////////////////////////////////////////////////////////

#include "EdtScaledLut.h"

#include <stdio.h>

#if 0
#define thisprintf printf("%p : ", this); printf
#else
#define thisprintf
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtScaledLut::EdtScaledLut()
{
    m_pZoom = new ZoomData;
    m_bOwnZoom = true;
}

EdtScaledLut::~EdtScaledLut()
{
    if (m_bOwnZoom && m_pZoom)
	delete m_pZoom;
}



#define RGBtoRGB16(r,g,b) ((((r)>>3) << 11) | (((g)>>3) << 6) | ((b) >> 3))


int EdtScaledLut::Transform(EdtImage *pSrc, EdtImage *pTarget, int nColor)

{
    // Not implemented yet
    if (!(pSrc && pTarget))
	return -1;

    if (m_pZoom->IsIdentity())
	return EdtLut::Transform(pSrc,pTarget,nColor);

    printf("Transform pSrc = %p pTarget = %p luts[0] = %p type %d => %d colors %d\n",
	pSrc, pTarget, 
	m_pLuts[0], pSrc->GetType(), pTarget->GetType(),
	m_nColors);

    if (m_pZoom->IsInteger())
    {
	return TransformIntegerZoom(pSrc,pTarget,0,0,
	    pSrc->GetWidth(),pSrc->GetHeight(),0,0,nColor);
    }
    else
    {
	return TransformDoubleZoom(pSrc,pTarget,0,0,
	    pSrc->GetWidth(),pSrc->GetHeight(),0,0,nColor);
    }
}

int EdtScaledLut::Transform(EdtImage *pSrc, EdtImage *pTarget, 
			    int fromx,int fromy,
			    int width,int height,
			    int tox, int toy,
			    int nColor)

{
    // Not implemented yet
    if (!(pSrc && pTarget))
	return -1;

    if (m_pZoom->IsIdentity())
	return EdtLut::Transform(pSrc,pTarget,fromx,fromy,width,height,tox,toy,nColor);


    if (m_pZoom->IsInteger())
    {
	return TransformIntegerZoom(pSrc,pTarget,fromx,fromy,
	    width,height,tox,toy,nColor);
    }
    else
    {
	return TransformDoubleZoom(pSrc,pTarget,fromx,fromy,
	    width,height,tox,toy,nColor);
    }




}

int EdtScaledLut::TransformDoubleZoom(EdtImage *pSrc, EdtImage *pTarget,
				      int fromx, int fromy, int width, int height,
				      int tox, int toy, 
				      int nColor)

{
    // Not implemented yet
    if (!(pSrc && pTarget))
	return -1;

    printf("TransformDoubleZoom\n");

    double zx =  m_pZoom->GetZoomX();
    double zy =  m_pZoom->GetZoomY();

    int srcwidth = edt_min(pSrc->GetWidth(),width);
    int srcheight = edt_min(pSrc->GetHeight(),height);

    int destwidth = pTarget->GetWidth();
    int destheight = pTarget->GetHeight();


    if (fromx < 0)
    {
	srcwidth += fromx;
	tox -= (int) (fromx*zx);

	fromx = 0;
    } 
    else if (srcwidth + fromx > pSrc->GetWidth())
    {
	srcwidth = pSrc->GetWidth() - fromx;
    }


    if (tox < 0)
    {
	destwidth += (int) tox;
	fromx -= (int) (tox/zx);

	tox = 0;
    }
    else if (destwidth + tox > pTarget->GetWidth())
    {
	destwidth = pTarget->GetWidth() - tox;
    }

    if (fromy < 0)
    {
	srcheight +=  fromy;
	toy -= (int)(fromy*zx);
	fromy = 0;

    }
    else if (srcheight + fromy > pSrc->GetHeight())
    {
	srcheight = pSrc->GetHeight() - fromy;
    }

    if (toy < 0)
    {
	destheight += toy;
	fromy -= (int)(toy/zx);
	toy = 0;
    }
    else if (destheight + toy > pSrc->GetHeight())
    {
	destheight = pSrc->GetHeight() - toy;
    }


    // dimensions are in target coordinates

    int nWidth = edt_min((int)(srcwidth * zx), destwidth);
    int nHeight = edt_min((int)(srcheight * zy), destheight);

    if (m_pLuts[0])
    {

	if (m_MapType == AutoLinear)
	{

	    UpdateMapAutoLinear();
	}

	int nOpTypes = FUNC_MONAD(pSrc->GetType(), pTarget->GetType());

	byte **brows1, ** brows2;
	ushort_t **srows1, **srows2;
	RGBPixel **rgbrows2;

	int startcolor = 0;
	int endcolor = m_nColors;

	int srcx,srcy,destx,desty;


	double dx = 1.0/zx;
	double dy = 1.0/zy;

	// source addresses
	double row_z;
	double col_z;

	// nColor == -1 means all bands

	if (nColor != -1)
	{
	    startcolor = nColor;
	    endcolor = nColor + 1;
	    if (nColor >= m_nColors)
	    {
		return -1;
	    }
	}

	switch (nOpTypes)
	{

	case FUNC_BYTE_BYTE:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();

		byte *pLut = (byte *) m_pLuts[0];

		row_z = fromy;


		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    for (destx = tox, col_z= fromx; 
			destx < nWidth+tox; 
			destx ++)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v = pLut[brows1[srcy][srcx]];

			brows2[desty][destx] = v;

		    }

		}		
	    }
	    break;

	case FUNC_USHORT_BYTE:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();

		byte *pLut = (byte *) m_pLuts[0];

		row_z = fromy;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    for (destx = tox, col_z= fromx; 
			destx < nWidth+tox; 
			destx ++)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v = pLut[srows1[srcy][srcx]];

			brows2[desty][destx] = v;


		    }
		}		
	    }
	    break;

	case FUNC_USHORT_USHORT:

	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows();
		srows2 = (ushort_t **) pTarget->GetPixelRows();

		ushort_t *pLut = (ushort_t *) m_pLuts[0];

		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    for (destx = tox, col_z= fromx; 
			destx < nWidth+tox; 
			destx ++)
		    {

			srcx = (int) col_z;
			col_z += dx;
			ushort_t v = pLut[srows1[srcy][srcx]];

			srows2[desty][destx] = v;

		    }

		}		
	    }		
	    break;


	case FUNC_BGR_BGR:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc, *pdest;

		nWidth *= 3;
		fromx *= 3;
		tox *= 3;

		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = brows1[srcy];

		    pdest = brows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx += 3)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v0 = pLut0[psrc[srcx]];
			byte v1 = pLut1[psrc[srcx+1]];
			byte v2 = pLut2[psrc[srcx+2]];

			pdest[destx] = v0;
			pdest[destx+1] = v1;
			pdest[destx+2] = v2;
		    }		

		}		

	    }
	    break;

	case FUNC_BYTE_BGR:
	    {
		brows1 = (byte **) pSrc->GetPixelRows() ;
		rgbrows2 = (RGBPixel **) pTarget->GetPixelRows();


		RGBPixel * pLut = (RGBPixel *)
		    m_pSingleInputLut;

		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc;
		RGBPixel *pdest;

		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = brows1[srcy];

		    pdest = rgbrows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx ++)
		    {

			srcx = (int) col_z;
			col_z += dx;

			pdest[destx] = pLut[psrc[srcx]];
		    }		
		}		
	    }
	    break;

	case FUNC_USHORT_BGR:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows() ;
		brows2 = (byte **) pTarget->GetPixelRows() ;


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		ushort_t *psrc;
		byte *pdest;
		nWidth *= 3;
		tox *= 3;
		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = srows1[srcy];

		    pdest = brows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx += 3)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v0 = pLut0[psrc[srcx]];

			pdest[destx] = v0;
			pdest[destx+1] = v0;
			pdest[destx+2] = v0;
		    }		

		}		

	    }
	    break;

	case FUNC_BGR_BGRA:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows() ;


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc, *pdest;

		nWidth *= 4;
		fromx *= 3;
		tox *= 4;

		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = brows1[srcy];

		    pdest = brows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx += 4)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v0 = pLut0[psrc[srcx]];
			byte v1 = pLut1[psrc[srcx+1]];
			byte v2 = pLut2[psrc[srcx+2]];

			pdest[destx] = v0;
			pdest[destx+1] = v1;
			pdest[destx+2] = v2;
		    }		

		}		

	    }
	case FUNC_BYTE_BGRA:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc, *pdest;
		nWidth *= 4;
		tox *= 4;
		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = brows1[srcy];

		    pdest = brows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx += 4)
		    {

			srcx = (int) col_z;
			col_z += dx;

			byte v0 = pLut0[psrc[srcx]];

			pdest[destx] = v0;
			pdest[destx+1] = v0;
			pdest[destx+2] = v0;

		    }		

		}		

	    }
	    break;

	case FUNC_USHORT_BGRA:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		ushort_t *psrc;
		byte *pdest;
		nWidth *= 4;
		tox *= 4;

		row_z = fromy;

		for (desty = toy;desty<nHeight+toy;desty ++)
		{
		    srcy = (int) row_z;
		    row_z += dy;
		    col_z = fromx;

		    psrc = srows1[srcy];

		    pdest = brows2[desty];

		    for (destx = tox; 
			destx < nWidth+tox; 
			destx += 3)
		    {

			srcx = (int) col_z;
			col_z += dx;
			byte v0 = pLut0[psrc[srcx]];

			pdest[destx] = v0;
			pdest[destx+1] = v0;
			pdest[destx+2] = v0;

		    }		

		}		

	    }
	    break;

	default:
	    // ERROR - not implemented yet
	    return -1;

	}
    }

    return 0;
}


int EdtScaledLut::TransformIntegerZoom(EdtImage *pSrc, EdtImage *pTarget,
				       int fromx, int fromy, int width, int height,
				       int tox, int toy, 
				       int nColor)

{
    if (!(pSrc && pTarget))
	return -1;

    ASSERT(m_pZoom && m_pZoom->IsInteger());

    int zx = (int) m_pZoom->GetZoomX();
    int zy = (int) m_pZoom->GetZoomY();

    ASSERT(zx >= 1 && zy >= 1);

    int srcwidth = edt_min(pSrc->GetWidth(),width);
    int srcheight = edt_min(pSrc->GetHeight(),height);

    int destwidth = pTarget->GetWidth();
    int destheight = pTarget->GetHeight();

    thisprintf("TransformIntegerZoom\n");

    if (fromx < 0)
    {
	srcwidth += fromx;
	tox -= fromx*zx;

	fromx = 0;
    } 
    else if (srcwidth + fromx > pSrc->GetWidth())
    {
	srcwidth = pSrc->GetWidth() - fromx;
    }


    if (tox < 0)
    {
	destwidth += tox/zx;
	fromx -= tox/zx;

	tox = 0;
    }
    else if (destwidth + tox > pTarget->GetWidth())
    {
	destwidth = pTarget->GetWidth() - tox;
    }

    if (fromy < 0)
    {
	srcheight += fromy;
	toy -= fromy*zx;
	fromy = 0;

    }
    else if (srcheight + fromy > pSrc->GetHeight())
    {
	srcheight = pSrc->GetHeight() - fromy;
    }

    if (toy < 0)
    {
	destheight += toy/zx;
	fromy -= toy/zx;
	toy = 0;
    }
    else if (destheight + toy > pSrc->GetHeight())
    {
	destheight = pSrc->GetHeight() - toy;
    }


    // dimensions are in target coordinates

    int nWidth = edt_min(srcwidth * zx, destwidth) - zx + 1;
    int nHeight = edt_min(srcheight * zy, destheight) - zy + 1;


    if (m_pLuts[0])
    {

	if (m_MapType == AutoLinear)
	{

	    UpdateMapAutoLinear();
	}

	int nOpTypes = FUNC_MONAD(pSrc->GetType(), pTarget->GetType());

	byte **brows1, ** brows2;
	ushort_t **srows1, **srows2;
	RGBPixel **rgbrows2;

	int startcolor = 0;
	int endcolor = m_nColors;

	int srcx,srcy,destx,desty,row;
	int i;
	// nColor == -1 means all bands

	if (nColor != -1)
	{
	    startcolor = nColor;
	    endcolor = nColor + 1;
	    if (nColor >= m_nColors)
	    {
		return -1;
	    }
	}

	switch (nOpTypes)
	{

	case FUNC_BYTE_BYTE:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();

		byte *pLut = (byte *) m_pLuts[0];

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    for (row=desty;row < desty+zy;row++)
		    {
			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    destx += zx, srcx ++)
			{
			    byte v = pLut[brows1[srcy][srcx]];

			    memset(brows2[row] + destx,v, zx);

			}
		    }
		}		
	    }
	    break;

	case FUNC_USHORT_BYTE:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();

		byte *pLut = (byte *) m_pLuts[0];

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    for (row=desty;row < desty+zy;row++)
		    {
			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    destx += zx, srcx ++)
			{
			    byte v = pLut[srows1[srcy][srcx]];

			    memset(brows2[row] + destx,v, zx);

			}
		    }
		}		
	    }
	    break;

	case FUNC_USHORT_USHORT:

	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows();
		srows2 = (ushort_t **) pTarget->GetPixelRows();

		ushort_t *pLut = (ushort_t *) m_pLuts[0];

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    for (row=desty;row < desty+zy;row++)
		    {
			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    destx += zx, srcx ++)
			{
			    ushort_t v = pLut[srows1[srcy][srcx]];

			    memset(srows2[row] + destx,v, 2*zx);

			}
		    }
		}		
	    }		
	    break;



	case FUNC_BGR_BGR:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows();


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc, *pdest;

		nWidth *= 3;
		fromx *= 3;
		tox *= 3;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = brows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx +=3)
			{
			    byte v0 = pLut0[psrc[srcx]];
			    byte v1 = pLut1[psrc[srcx+1]];
			    byte v2 = pLut2[psrc[srcx+2]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v0;
				pdest[destx+1] = v1;
				pdest[destx+2] = v2;
				destx += 3;
			    }
			}		
		    }	
		}		

	    }
	    break;

	case FUNC_BYTE_BGR:
	    {
		brows1 = (byte **) pSrc->GetPixelRows() ;
		rgbrows2 = (RGBPixel **) pTarget->GetPixelRows();

		RGBPixel *pLut = (RGBPixel *) 
		    m_pSingleInputLut;

		byte *psrc;
		RGBPixel *pdest;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = rgbrows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{
			    RGBPixel v = pLut[psrc[srcx]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		
	    }
	    break;

	case FUNC_USHORT_BGR:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows() ;
		rgbrows2 = (RGBPixel **) pTarget->GetPixelRows();

		RGBPixel *pLut = (RGBPixel *) 
		    m_pSingleInputLut;

		ushort_t *psrc;
		RGBPixel *pdest;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = srows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = rgbrows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{
			    RGBPixel v = pLut[psrc[srcx]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		
	    }
	    break;

	case FUNC_BGR_RGB16:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		srows2 = (ushort_t **) pTarget->GetPixelRows();


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc;
		ushort_t *pdest;

		fromx *= 3;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = srows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx +=3)
			{
			    byte v0 = pLut0[psrc[srcx]];
			    byte v1 = pLut1[psrc[srcx+1]];
			    byte v2 = pLut2[psrc[srcx+2]];

			    ushort_t v = RGBtoRGB16(v0,v1,v2);

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
			    }
			}		
		    }	
		}		

	    }
	    break;

	case FUNC_BYTE_RGB16:
	    {
		brows1 = (byte **) pSrc->GetPixelRows() ;
		ushort_t **srows2 = (ushort_t **) pTarget->GetPixelRows();

		ushort_t *pLut = (ushort_t *) 
		    m_pSingleInputLut;
		ushort_t *pdest;

		byte *psrc;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = srows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{
			    ushort_t v = pLut[psrc[srcx]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		

	    }
	    break;

	case FUNC_USHORT_RGB16:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows() ;
		ushort_t **srows2 = (ushort_t **) pTarget->GetPixelRows();

		ushort_t *pLut = (ushort_t *) 
		    m_pSingleInputLut;

		ushort_t *psrc;
		ushort_t *pdest;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = srows1[srcy];

		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = srows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{

			    ushort_t v = pLut[psrc[srcx]];

			    if (row == 100 && srcx == 100)
			    {
				thisprintf("(100,100) = %x -> %x\n",
				    psrc[srcx], v);
			    }
			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		

	    }
	    break;
	case FUNC_BGR_BGRA:
	    {
		brows1 = (byte **) pSrc->GetPixelRows();
		brows2 = (byte **) pTarget->GetPixelRows() ;


		byte *pLut0 = (byte *) m_pLuts[0];
		byte *pLut1 = (byte *) m_pLuts[1];
		byte *pLut2 = (byte *) m_pLuts[2];

		byte *psrc, *pdest;

		nWidth *= 4;
		fromx *= 3;
		tox *= 4;


		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = brows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx +=3)
			{
			    byte v0 = pLut0[psrc[srcx]];
			    byte v1 = pLut1[psrc[srcx+1]];
			    byte v2 = pLut2[psrc[srcx+2]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v0;
				pdest[destx+1] = v1;
				pdest[destx+2] = v2;

				destx += 4;
			    }
			}		
		    }	
		}		

	    }
	    break;
	case FUNC_BYTE_BGRA:
	    {
		brows1 = (byte **) pSrc->GetPixelRows() ;
		int **irows2 = (int **) pTarget->GetPixelRows();

		int *pLut = (int *) 
		    m_pSingleInputLut;

		byte *psrc;
		int *pdest;

		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = brows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = irows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{
			    int v = pLut[psrc[srcx]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		
	    }
	    break;

	case FUNC_USHORT_BGRA:
	    {
		srows1 = (ushort_t **) pSrc->GetPixelRows() ;
		uint **lrows2 = (uint **) pTarget->GetPixelRows();

		uint *pLut = (uint *)  m_pSingleInputLut;

		ushort_t *psrc;
		uint *pdest;


		for (srcy = fromy,desty = toy;desty<nHeight+toy;desty += zy,srcy++)
		{
		    psrc = srows1[srcy];
		    for (row=desty;row < desty+zy;row++)
		    {
			pdest = lrows2[row];

			for (destx = tox, srcx= fromx; 
			    destx < nWidth+tox; 
			    srcx ++)
			{
			    uint v = pLut[psrc[srcx]];

			    for (i=0;i<zx;i++)
			    {
				pdest[destx] = v;
				destx ++;
			    }
			}		
		    }	
		}		
	    }
	    break;

	default:
	    // ERROR - not implemented yet
	    return -1;

	}
    }

    else
	pTarget->Copy(pSrc);

    return 0;
}


