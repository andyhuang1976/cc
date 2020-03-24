/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SymbolTable.cpp,v 1.4 2014/04/16 12:09:22 administrator Exp $
* 
*******************************************************************************
* 
* Description:implementation file
******************************************************************************/
static const char _CppVersion[] = "@(#)  $Header: /cvsdata/vc/SaxCCompile/SymbolTable.cpp,v 1.4 2014/04/16 12:09:22 administrator Exp $";
#include "stdafx.h"
// $Nokeywords: $
#include "SymbolTable.h"
#pragma warning(disable : 4995)
#include ".\global.h"
#include "parser.h"


CSymbolTable::CSymbolTable()
{
	InitHashTable();
}

CSymbolTable::~CSymbolTable()
{
	for( int i = 0; i < HASH_TABLE_SIZE; i++ )
	{
		if( m_arrHashTable[i] ) 
		{
			delete m_arrHashTable[i];
		}
	}
}
void CSymbolTable::InitHashTable()
{
	memset(m_arrHashTable,0,sizeof(m_arrHashTable));	
}


// GetHashKey function
int CSymbolTable::GetHashKey(char* pString)
{
	int nResult = 0, i;

	for( nResult = 0, i = 0; pString[i] != '\0'; i++ )
	{
		nResult = ((nResult << HASH_TABLE_SHIFT) + pString[i]) % HASH_TABLE_SIZE;
	}

	return nResult;
}



void CSymbolTable::DestroyHashTable()
{
	for( int i = 0; i < HASH_TABLE_SIZE; i++ ) 
	{
		if(NULL != m_arrHashTable[i] )
		{
			delete m_arrHashTable[i];
			m_arrHashTable[i] = NULL;
		}
	}
}





/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::LookupSymbol
|  描    述  : Look up symbol in hash table by name and scope
|  参    数  : char* pName——
|              char* pScope——
|  返 回 值  : If find,return a pointer to the symbol.
|  修改记录  : 2007-4-25 14:30:33   -huangdy-   创建
-----------------------------------------------------------------*/
CSymbol* CSymbolTable::LookupSymbol(char* pName, char* pScope )
{
	int nHashKey = GetHashKey( pName );
	CSymbol* pSymbol = m_arrHashTable[nHashKey];

	while( pSymbol && (strcmp(pSymbol->GetName(), pName) != 0 
		|| strcmp(pSymbol->GetScope(),pScope) != 0) ) 
	{
		pSymbol = pSymbol->GetNext();
	}

	return pSymbol;
}


// insert a node into the GetHashKey table,
// nMemoryLocation is inserted only the m_pFirst time, otherwise ignored


/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::InsertSymbol
|  描    述  : Process local variable 
|  参    数  : char* pName——
|              char* pScope——
|              AXC_SYMBOL_TYPE nTokenType——
|              int nLineIndex——
|              int nOffset——
|              bool  bVariableFlag = TRUE——
|              bool bArray = FALSE——
|              int nArraySize = 0——
|  返 回 值  : 
|  修改记录  : 2007-5-1 20:07:29   -huangdy-   创建
-----------------------------------------------------------------*/
//CSymbol*	CSymbolTable::InsertSymbol(char* pName, char* pScope, 
//									   int nOffset,int nArraySize)
//{
//	int nHashKey = GetHashKey( pName );
//	CSymbol* pSymbol = m_arrHashTable[nHashKey];
//
//	while( pSymbol && (strcmp(pSymbol->GetName(), pName) != 0 
//		|| strcmp(pSymbol->GetScope(),pScope) != 0) ) 
//	{
//		pSymbol = pSymbol->GetNext();
//	}
//
//	if(NULL == pSymbol)
//	{
//		pSymbol = new CSymbol(nTokenType,pName,pScope,nArraySize,nOffset);		
//		pSymbol->SetNext(m_arrHashTable[nHashKey]);
//		m_arrHashTable[nHashKey] = pSymbol;
//	} 
//
//	return pSymbol;
//}

/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::InsertSymbol
|  描    述  : 
|  参    数  :  char* pName——
|              char* pScope——
|              AXC_SYMBOL_TYPE nTokenType——
|              int nLineIndex——
|              int nMemoryLocation——
|              BOOL bArray——
|  返 回 值  : 
|  修改记录  : 2007-4-25 11:59:55   -huangdy-   创建
-----------------------------------------------------------------*/
CSymbol* CSymbolTable::InsertSymbol( CSyntaxTreeNode* pSyntaxTreeNode)
{
	int nHashKey = GetHashKey( pName );
	CSymbol* pSymbol = m_arrHashTable[nHashKey];

	while( pSymbol && (strcmp(pSymbol->GetName(), pName) != 0 
		|| strcmp(pSymbol->GetScope(),pScope) != 0) ) 
	{
		pSymbol = pSymbol->GetNext();
	}

	if(NULL == pSymbol)
	{
		pSymbol = new CSymbol(pSyntaxTreeNode);		
		pSymbol->SetNext(m_arrHashTable[nHashKey]);
		m_arrHashTable[nHashKey] = pSymbol;
	} 

	pSymbol->AppendLineIndex(nLineIndex);

	return pSymbol;
}


/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::LookupMemoryLocation
|  描    述  : lookup a node with specified pName and pScope from the GetHashKey table,
|  参    数  : char* pName——
|              char* pScope——
|  返 回 值  : return the nMemoryLocation
|  修改记录  : 2007-4-25 14:21:23   -huangdy-   创建
-----------------------------------------------------------------*/
int CSymbolTable::LookupMemoryLocation(char* pName, char* pScope )
{
	CSymbol* pSymbol = LookupSymbol(pName,pScope);

	return (NULL == pSymbol) ? -1 : pSymbol->GetMemoryLocation();
}


/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::QuerySymbolExtraType
|  描    述  : check if it is array
|  参    数  :  char* pName——
|              char* pScope——
|  返 回 值  : TRUE----
|              FALSE---
|  修改记录  : 2007-4-25 14:22:59   -huangdy-   创建
-----------------------------------------------------------------*/
//EXTRA_DATA_TYPE CSymbolTable::QuerySymbolExtraType( char* pName, char* pScope )
//{
//	CSymbol* pSymbol = LookupSymbol(pName,pScope);
//	if(NULL != pSymbol)
//	{
//		return pSymbol->GetExtraDataType();
//	}
//
//	return EDT_NONE;
//}


/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::LookupType
|  描    述  : return the nTokenType of the specified table node
|  参    数  : char* pName——
|              char* pScope——
|  返 回 值  : 
|  修改记录  : 2007-4-25 14:23:18   -huangdy-   创建
-----------------------------------------------------------------*/
AXC_SYMBOL_TYPE CSymbolTable::LookupType(  char* pName, char* pScope )
{
	CSymbol* pSymbol = LookupSymbol(pName,pScope);

	return (pSymbol == NULL ) ? TT_ERROR_TYPE : pSymbol->GetTokenType();
}

/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::PrintSymbolItem
|  描    述  : PrintSymbolItem a string to buffer
|  参    数  : char* pMsgBuffer——
|              int nLength——
|              char* format——
|  返 回 值  : 
|  修改记录  : 2007-4-25 14:38:09   -huangdy-   创建
-----------------------------------------------------------------*/
void CSymbolTable::PrintSymbolItem(char* format, ... )
{
	const UINT ERR_BUFFER_LEN           = 2048;
BYTE szCommonBuffer[ERR_BUFFER_LEN] = {0};

	va_list params;

	va_start( params, format );
	_vsnprintf_s((char*)szCommonBuffer,ERR_BUFFER_LEN,ERR_BUFFER_LEN, format, params );
	va_end( params );	

	ATLTRACE("%s",(char*)szCommonBuffer);
}

// print a formatted listing of the symbol table contents to lpszPathName

/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::PrintSymbolTable
|  描    述  : Fill a formatted listing of the symbol table contents to buffer
|  参    数  : char* pMsgBuffer——
|              int nLength——
|  返 回 值  : 
|  修改记录  : 2007-4-25 14:38:26   -huangdy-   创建
-----------------------------------------------------------------*/
void CSymbolTable::PrintSymbolTable(CScaner* pScaner)
{
	PrintSymbolItem( "           Scope        AnalyseVariable Name     Type     Location    Line Numbers\r\n" );
	PrintSymbolItem( "       -------------    -------------    ------    --------    ------------\r\n" );
	for( int i = 0; i < HASH_TABLE_SIZE; i++ )
	{
		if( m_arrHashTable[i] != NULL )
		{
			CSymbol* pSymbol = m_arrHashTable[i];
			while( pSymbol ) 
			{
				PrintSymbolItem( "%18s", pSymbol->GetScope());
				PrintSymbolItem( "%17s", pSymbol->GetName() );
				if( pSymbol->GetTokenType() == TT_LABEL_TYPE )
				{
					PrintSymbolItem( "%11s", "m_pParser" );
				}
				else
				{
					PrintSymbolItem( "%9s%s", 
						pScaner->GetKeywordDescription(pSymbol->GetTokenType()) 
						,(pSymbol->GetExtraDataType() == EDT_ARRAY ? "[]" : "") );
				}
				PrintSymbolItem( "%12d     ", pSymbol->GetMemoryLocation());

				for(UINT i = 0;i < pSymbol->GetLineIndexCount();i++)
				{
					PrintSymbolItem( "%3d ", pSymbol->GetLineIndex(i));
				}
				PrintSymbolItem( "\r\n" );
				pSymbol = pSymbol->GetNext();
			}
		}
	}
}


/*-----------------------------------------------------------------
|  函数名称  : CSymbolTable::AccountGlobalDataSize
|  描    述  : Account global data size(include local variable in main function)
|              and Set variable's offset 
|  返 回 值  : return global data size
|  修改记录  : 2007-4-29 10:53:09   -huangdy-   创建
-----------------------------------------------------------------*/
int  CSymbolTable::AccountGlobalDataSize()
{
	int nGlobalDataSize = 0;
	int nTmp    = 0;

	for( int i = 0; i < HASH_TABLE_SIZE; i++ )   //must scan whole array.
	{
		if( m_arrHashTable[i] != NULL )
		{
			CSymbol* pSymbol = m_arrHashTable[i];
			while(NULL != pSymbol ) 
			{
				if(pSymbol->GetVariableFlag() && (strcmp(pSymbol->GetScope(),"global") == 0 
					|| strcmp(pSymbol->GetScope(),"main") == 0))
				{
					pSymbol->SetOffset(nGlobalDataSize);  
					if(pSymbol->GetExtraDataType() == EDT_ARRAY)
					{
						nTmp = pSymbol->GetArraySize() * GetDataTypeSize(pSymbol->GetTokenType());
					}
					else
					{
						nTmp = GetDataTypeSize(pSymbol->GetTokenType());
					}                   
					
					nGlobalDataSize += nTmp;
				}
				
				pSymbol = pSymbol->GetNext();
			}
		}
	}

	return nGlobalDataSize;
}
