// EdtList.h: interface for the CEdtList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDTLIST_H__1B7C7A72_258E_11D3_8B95_00104B357776__INCLUDED_)
#define AFX_EDTLIST_H__1B7C7A72_258E_11D3_8B95_00104B357776__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

template <class Type> class EdtListHead 
{

class CEdtListMember {

public:

	class CEdtListMember *pPrev;
	class CEdtListMember *pNext;

	Type *pData;

	CEdtListMember(Type *pObj = NULL)
	{
		pPrev = pNext = this;
		pData = pObj;
	}

	void Cutout()
	{
		pPrev->pNext = pNext;
		pNext->pPrev = pPrev;

		pPrev = pNext = this;
	}

	void InsertAfter(CEdtListMember *pNew)
	{
		pNew->pNext = pNext;
		pNext->pPrev = pNew;

		pNew->pPrev = this;
		pNext = pNew;

	}

	void InsertBefore(CEdtListMember *pNew)
	{
		pNew->pPrev = pPrev;
		pPrev->pNext = pNew;

		pNew->pNext = this;
		pPrev = pNew;
	}


};

public:
	void InsertAfterCurrent(Type *pObj);
	void Prepend(Type *pObj);
	void InsertBeforeCurrent(Type *pObj);

	CEdtListMember Head;
	CEdtListMember *pCurrent;

	bool IsInList(Type *pObj);
	bool EndOfList();

	int Length();

	Type * GetPrevious();
	Type * GetTail();
	Type * GetNext();
	Type * GetFirst();
	void Append(Type *pObj);
	
	void Clear();
	EdtListHead();
	virtual ~EdtListHead();

	Type * DeleteCurrent();

	void Delete(Type *pObj);

};

// EdtList.cpp: implementation of the EdtListHead<Type> class.
//
//////////////////////////////////////////////////////////////////////
/////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template <class Type>
EdtListHead<Type>::EdtListHead()
{

	pCurrent = NULL;
}


template <class Type>
EdtListHead<Type>::~EdtListHead()
{
	Clear();
}


template <class Type>
void EdtListHead<Type>::Clear()
{
	// delete all but Head first
	pCurrent = Head.pNext;

	while (pCurrent != &Head)
	{
		CEdtListMember *pLast;
		pLast = pCurrent;
		
		pCurrent = pCurrent->pNext;

		pLast->Cutout();
		delete pLast;

	}

	pCurrent = NULL;
	
}


template <class Type>
bool EdtListHead<Type>::EndOfList()
{
	if (pCurrent)
		return false;
	else
		return true;
}


template <class Type>
Type * EdtListHead<Type>::GetFirst()
{
	if (Head.pNext != &Head)
	{
		pCurrent = Head.pNext;
		return pCurrent->pData;
	}
	else
	{
		// list is empty
		pCurrent = NULL;
		return NULL;
	}
}


template <class Type>
Type * EdtListHead<Type>::GetNext()
{

	if (!pCurrent)
		return NULL;

	pCurrent = pCurrent->pNext;

	if (pCurrent != &Head)
		return pCurrent->pData;
	else
		pCurrent = NULL;

	return NULL;

}


template <class Type>
Type * EdtListHead<Type>::GetTail()
{
	if (Head.pPrev != &Head)
	{
		pCurrent = Head.pPrev;
		return pCurrent->pData;
	}
	else
	{
		// list is empty
		pCurrent =NULL;
		return NULL;
	}
}


template <class Type>
Type * EdtListHead<Type>::GetPrevious()
{
	if (!pCurrent)
		return NULL;

	pCurrent = pCurrent->pPrev;

	if (pCurrent != &Head)
		return pCurrent->pData;
	else
		pCurrent = NULL;

	return NULL;

}

template <class Type>
bool EdtListHead<Type>::IsInList(Type * pObj)
{
	Type *pTest;

	pTest = GetFirst();

	while (!EndOfList())
	{
		if (pTest == pObj)
			return true;

		pTest = GetNext();
	}

	return false;

}

template <class Type>
int EdtListHead<Type>::Length()
{
	int count = 0;

	GetFirst();

	while (!EndOfList())
	{
		count++;
		GetNext();
	}

	return count;

}
template <class Type>
void EdtListHead<Type>::Append(Type * pObj)
{
	Head.InsertAfter(new CEdtListMember(pObj));
}

template <class Type>
void EdtListHead<Type>::InsertBeforeCurrent(Type * pObj)
{
	if (pCurrent)
		pCurrent->InsertBefore(new CEdtListMember(pObj));

}


template <class Type>
void EdtListHead<Type>::Prepend(Type * pObj)
{
	Head.InsertAfter(new CEdtListMember(pObj));

}	


template <class Type>
void EdtListHead<Type>::InsertAfterCurrent(Type * pObj)
{
	ASSERT(pCurrent);

	if (pCurrent)
		pCurrent->InsertAfter(new CEdtListMember(pObj));

}

// Calls GetNext and deletes the current obj
// Returns GetNext return value
//
template <class Type>
Type * EdtListHead<Type>::DeleteCurrent()

{
	CEdtListMember *pLastOne = pCurrent;

	Type * pNew = NULL;

	if (pCurrent)
	{

		pNew = GetNext();

		pLastOne->Cutout();

	}

	return pNew;

}


template <class Type>
void EdtListHead<Type>::Delete(Type *pObj)

{
	if (IsInList(pObj))
	{

		DeleteCurrent();
	
	}

}

#endif // !defined(AFX_EDTLIST_H__1B7C7A72_258E_11D3_8B95_00104B357776__INCLUDED_)
