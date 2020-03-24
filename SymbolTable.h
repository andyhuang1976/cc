/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SymbolTable.h,v 1.3 2014/04/12 09:22:08 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include ".\scaner.h"
#include "strsafe.h"
#include <vector>
#include ".\global.h"

const int HASH_TABLE_SIZE = 211;  // HASH_TABLE_SIZE is the size of the GetHashKey table
const int HASH_TABLE_SHIFT = 4;   // HASH_TABLE_SHIFT is the power of two used as multiplier in GetHashKey function

typedef std::vector<UINT>         LINEINDEX_ARRAY;

class CSyntaxTreeNode;

// The record in the bucket lists for each variable, including name, 
// assigned memory m_nLocation, and the list of line numbers in which
// it appears in the source code
class CSymbol
{
public:
	CSymbol(CSyntaxTreeNode* pSyntaxTreeNode = NULL) 
		: m_pSyntaxTreeNode(pSyntaxTreeNode)
	    , m_nTokenType(TT_INT)
		, m_nMemoryLocation( 0 )
		, m_pNext( NULL ) 
		, m_nArraySize(0)
		, m_nOffset(0)
		, m_bVariableFlag(0)
	{
		
	}	

	~CSymbol()
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
	void SetScope(char* pScope)                    { memcpy(m_szScope,pScope,strlen(pScope));}
	char* GetScope()                               { return m_szScope;}
	void SetMemoryLocation(int nMemoryLocation)    { m_nMemoryLocation = nMemoryLocation;}
	int GetMemoryLocation()                        { return m_nMemoryLocation;}

	void SetNext(CSymbol* pNext)                   { m_pNext = pNext;}
	CSymbol* GetNext()                             { return m_pNext;}
	/*void SetLineIndex(CVariableLineList* pLineIndex) { m_pLineIndex = pLineIndex;}
	CVariableLineList* GetLineIndex()              { return m_pLineIndex;}*/
	void SetArraySize(int nArraySize)              { m_nArraySize = nArraySize;}
	int GetArraySize()                             { return m_nArraySize;}
	void  SetOffset(int nOffset)                   { m_nOffset = nOffset;}
	int GetOffset()                                { return m_nOffset;}
	void SetVariableFlag(bool bVariableFlag)       { m_bVariableFlag = bVariableFlag;}
	bool GetVariableFlag()                         { return m_bVariableFlag;}
public:
	void AppendLineIndex(int nLineIndex)
	{
		m_arrLinesIndex.push_back(nLineIndex);
	}	
	UINT GetLineIndexCount()                { return m_arrLinesIndex.size();}
	UINT GetLineIndex(UINT nIndex)
	{
		if(nIndex < m_arrLinesIndex.size())
		{
			return m_arrLinesIndex[nIndex];
		}
		return 0;
	}

private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSyntaxTreeNode,SyntaxTreeNode);  //²Î¿¼Ã¶¾ÙACM_MODE_X86,ACM_MODE_X64
	AXC_SYMBOL_TYPE	        m_nTokenType;
	char			        m_szName[SYMBOL_MAX_LENGTH];
	char			        m_szScope[SYMBOL_MAX_LENGTH];		    // node function scope
	int					    m_nMemoryLocation;	                    // memory m_nLocation for variable
	int                     m_nArraySize;                           // Array's size
	int                     m_nOffset;                              // Offset of function or variable,  
	bool                    m_bVariableFlag;                        // It indicate the symbol is a variable
	LINEINDEX_ARRAY	        m_arrLinesIndex;
	CSymbol*		        m_pNext;
};

class CSymbolTable
{
public:
	CSymbolTable();
	~CSymbolTable();

// Operations
public:
	CSymbol*	InsertSymbol(CSyntaxTreeNode* pSyntaxTreeNode);
	//CSymbol*	InsertSymbol(CSyntaxTreeNode* pSyntaxTreeNode);

	CSymbol*    LookupSymbol(char* pName, char* pScope );
	int			LookupMemoryLocation( char* name, char* scope );
	AXC_SYMBOL_TYPE LookupType( char* pName, char* pScope );
	int         AccountGlobalDataSize(); 
	void		PrintSymbolTable(CScaner* pScaner);
	void		DestroyHashTable();
private:
	inline int	GetHashKey(char* key );
	void		InitHashTable();
	void		PrintSymbolItem(char* format, ... );

private:
	CSymbol*	m_arrHashTable[HASH_TABLE_SIZE];
};

