/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/FunctionArgsCheck.h,v 1.3 2014/04/12 09:22:08 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include ".\scaner.h"

class CSyntaxTreeNode;

class CParameters
{
public:
	CParameters() 
		: m_nTokenType(TT_NULL_TYPE)
		, m_pNext( NULL )
	{
	}
	

	~CParameters()
	{ 
		if( m_pNext )
		{
			delete m_pNext;
		}
	}
public:
	void SetTokenType(AXC_SYMBOL_TYPE nTokenType)   { m_nTokenType = nTokenType;}
	AXC_SYMBOL_TYPE GetTokenType()                  { return m_nTokenType;}
	void SetNext(CParameters* pNext)               { m_pNext = pNext;}
	CParameters* GetNext()                         { return m_pNext;}
private:
	AXC_SYMBOL_TYPE	              m_nTokenType;
	CParameters*	              m_pNext;
};

class CFunctionDeclarations
{
public:
	CFunctionDeclarations() 
		: m_nTokenType(TT_NULL_TYPE)
		, m_nCount( 0 )
		, m_pParameters(NULL)
		, m_pNext( NULL ) 
	{
		memset(m_szName,0,SYMBOL_MAX_LENGTH);
	}
	CFunctionDeclarations( char* pName, AXC_SYMBOL_TYPE nToenType ) 
		: m_nTokenType(nToenType)
		, m_nCount( 0 )
		, m_pParameters(NULL)
		, m_pNext( NULL ) 
	{
		memcpy(m_szName,pName,strlen(pName));
	}
	~CFunctionDeclarations()
	{ 
		if(NULL != m_pNext )
		{
			delete m_pNext;
		}
	}
public:
	void SetTokenType(AXC_SYMBOL_TYPE nTokenType)   { m_nTokenType = nTokenType;}
	AXC_SYMBOL_TYPE GetTokenType()                  { return m_nTokenType;}
	void SetName(char* pName)                      { memcpy(m_szName,pName,strlen(pName));}
	char* GetName()                                { return m_szName;}
	void SetCount(int nCount)                      { m_nCount =  nCount;}
	int  GetCount()                                { return m_nCount;}
	void SetNext(CFunctionDeclarations* pNext)     { m_pNext = pNext;}
	CFunctionDeclarations* GetNext()               { return m_pNext;}
	CParameters* GetParameters()                   { return m_pParameters;}
public:
	CParameters* AddParameter(CParameters* pLasParameter,CParameters* pNewParameter)
	{
		assert(NULL != pNewParameter);

		if(NULL == pLasParameter)
		{
			m_pParameters = pNewParameter;
		}
		else
		{
			pLasParameter->SetNext(pNewParameter);
		}
        m_nCount++;

		return pNewParameter;
	}
private:
	char			        m_szName[SYMBOL_MAX_LENGTH];
	AXC_SYMBOL_TYPE	        m_nTokenType;
	int				        m_nCount;
	CParameters*	        m_pParameters;
	CFunctionDeclarations*  m_pNext;
};

class CFunctionArgsCheck
{
public:
	CFunctionArgsCheck();
	~CFunctionArgsCheck();

public:
	void	DestroyList();
	void	Insert( CSyntaxTreeNode* pNode );
	int		Check( CSyntaxTreeNode* pNode );	
private:
	CFunctionDeclarations	*m_pFirst;
	CFunctionDeclarations	*m_pLast;
};

