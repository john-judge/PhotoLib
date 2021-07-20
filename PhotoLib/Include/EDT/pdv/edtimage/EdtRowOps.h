/** ********************************
 * 
 * @file EdtImage/EdtRowOps.h
 *
 * Copyright (c) Engineering Design Team 2009
 * All Rights Reserved
 */


#ifndef _EDT_ROW_OPS_H_
#define _EDT_ROW_OPS_H_

// Template functions for use with pixel types

template <class T1> void FillRowTyped(T1 *Target, T1 Value, int nSize)
{
    for (int i=0;i<nSize;i++)
	Target[i] = Value;
}

template <class T1> void FillRowTyped(T1 *Target, T1 Value, int nSize,
				      int nStart1, int nSkip1)
{
    int i1;
    if (nSkip1 == 0) nSkip1 = 1;

    int i2 = (nSize * nSkip1) + nStart1;
    for (i1 = nStart1;i1<i2; i1+=nSkip1)
	Target[i1] = Value;
}

template <class T1, class T2> void CopyRowTyped(T1 *Src, T2 *Target, int nSize)

{
    for (int i=0;i<nSize;i++)
	Target[i] = (T2) Src[i];
}
// Version for use with multi-color pixels - 
template <class T1, class T2> void CopyRowTyped(T1 *Src, T2 *Target, int nSize,
						int nStart1, int nStart2,
						int nSkip1, int nSkip2)

{
    int i1, i2;
    if (nSkip1 == 0) nSkip1 = 1;
    if (nSkip2 == 0) nSkip2 = 1;

    for (i1 = nStart1, i2 = nStart2 ;i1<nSize * nSkip1 + nStart1;i1 += nSkip1, i2 += nSkip2 )
	Target[i2] = (T2) Src[i1];
}

template <class T1, class T2> void CopyRowLUTTyped(T1 *Src, T2 *Target, T2 *pLut, int nSize)

{
    for (int i=0;i<nSize;i++)
	Target[i] = pLut[Src[i]];
}

// Version for use with multi-color pixels - 
template <class T1, class T2> void CopyRowLUTTyped(T1 *Src, T2 *Target, T2 *pLut, int nSize,
						   int nStart1, int nStart2,
						   int nSkip1, int nSkip2)

{
    int i1, i2;
    if (nSkip1 == 0) nSkip1 = 1;
    if (nSkip2 == 0) nSkip2 = 1;

    for (i1 = nStart1, i2 = nStart2;i1<nSize * nSkip1;i1 += nSkip1, i2 += nSkip2 )
	Target[i2] = pLut[Src[i1]];
}

// Block Template functions for use with pixel types
// Input/ouput is via an array of row pointers


template <class T1> void FillBlockTyped(T1 **Target, T1 Value, int nSize, int nRows, 
					int yStart = 0, int xStart = 0)

{
    for (int row = yStart; row < nRows + yStart; row++)
	for (int i=xStart;i<nSize+xStart;i++)
	    Target[row][i] = Value;
}

template <class T1, class T2> void CopyBlockTyped(T1 **Src, T2 **Target, int nSize, int nRows)

{
    for (int row = 0; row < nRows; row++)
	for (int i=0;i<nSize;i++)
	    Target[row][i] = (T2) Src[row][i];
}
// Version for use with multi-color pixels - 
template <class T1, class T2> void CopyBlockTyped(T1 **Src, T2 **Target, int nSize, int nRows,
						  int nStart1, int nStart2, 
						  int nSkip1, int nSkip2)

{
    int i1, i2;
    if (nSkip1 == 0) nSkip1 = 1;
    if (nSkip2 == 0) nSkip2 = 1;

    for (int row = 0; row < nRows; row++)
	for (i1 = nStart1, i2 = nStart2;i1<nSize * nSkip1;i1 += nSkip1, i2 += nSkip2 )
	    Target[row][i2] = (T2) Src[row][i1];
}

template <class T1, class T2> void CopyBlockLUTTyped(T1 **Src, T2 **Target, T2 *pLut, int nSize, int nRows)

{
    for (int row = 0; row < nRows; row++)
	for (int i=0;i<nSize;i++)
	    Target[row][i] = pLut[Src[row][i]];
}

// Version for use with multi-color pixels - 
template <class T1, class T2> void CopyBlockLUTTyped(T1 **Src, T2 **Target, T2 *pLut, int nSize, int nRows,
						     int nStart1, int nStart2, int nSkip1, int nSkip2)

{
    int i1, i2;
    if (nSkip1 == 0) nSkip1 = 1;
    if (nSkip2 == 0) nSkip2 = 1;

    for (int row = 0; row < nRows; row++)
	for (i1 = nStart1, i2 = nStart2;i1<nSize * nSkip1;i1 += nSkip1, i2 += nSkip2 )
	    Target[row][i2] = pLut[Src[row][i1]];
}

 
#endif
