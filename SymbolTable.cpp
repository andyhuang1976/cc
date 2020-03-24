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
|  ��������  : CSymbolTable::LookupSymbol
|  ��    ��  : Look up symbol in hash table by name and scope
|  ��    ��  : char* pName����
|              char* pScope����
|  �� �� ֵ  : If find,return a pointer to the symbol.
|  �޸ļ�¼  : 2007-4-25 14:30:33   -huangdy-   ����
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
|  ��������  : CSymbolTable::InsertSymbol
|  ��    ��  : Process local variable 
|  ��    ��  : char* pName����
|              char* pScope����
|              AXC_SYMBOL_TYPE nTokenType����
|              int nLineIndex����
|              int nOffset����
|              bool  bVariableFlag = TRUE����
|              bool bArray = FALSE����
|              int nArraySize = 0����
|  �� �� ֵ  : 
|  �޸ļ�¼  : 2007-5-1 20:07:29   -huangdy-   ����
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
|  ��������  : CSymbolTable::InsertSymbol
|  ��    ��  : 
|  ��    ��  :  char* pName����
|              char* pScope����
|              AXC_SYMBOL_TYPE nTokenType����
|              int nLineIndex����
|              int nMemoryLocation����
|              BOOL bArray����
|  �� �� ֵ  : 
|  �޸ļ�¼  : 2007-4-25 11:59:55   -huangdy-   ����
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
|  ��������  : CSymbolTable::LookupMemoryLocation
|  ��    ��  : lookup a node with specified pName and pScope from the GetHashKey table,
|  ��    ��  : char* pName����
|              char* pScope����
|  �� �� ֵ  : return the nMemoryLocation
|  �޸ļ�¼  : 2007-4-25 14:21:23   -huangdy-   ����
-----------------------------------------------------------------*/
int CSymbolTable::LookupMemoryLocation(char* pName, char* pScope )
{
	CSymbol* pSymbol = LookupSymbol(pName,pScope);

	return (NULL == pSymbol) ? -1 : pSymbol->GetMemoryLocation();
}


/*-----------------------------------------------------------------
|  ��������  : CSymbolTable::QuerySymbolExtraType
|  ��    ��  : check if it is array
|  ��    ��  :  char* pName����
|              char* pScope����
|  �� �� ֵ  : TRUE----
|              FALSE---
|  �޸ļ�¼  : 2007-4-25 14:22:59   -huangdy-   ����
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
|  ��������  : CSymbolTable::LookupType
|  ��    ��  : return the nTokenType of the specified table node
|  ��    ��  : char* pName����
|              char* pScope����
|  �� �� ֵ  : 
|  �޸ļ�¼  : 2007-4-25 14:23:18   -huangdy-   ����
-----------------------------------------------------------------*/
AXC_SYMBOL_TYPE CSymbolTable::LookupType(  char* pName, char* pScope )
{
	CSymbol* pSymbol = LookupSymbol(pName,pScope);

	return (pSymbol == NULL ) ? TT_ERROR_TYPE : pSymbol->GetTokenType();
}

/*-----------------------------------------------------------------
|  ��������  : CSymbolTable::PrintSymbolItem
|  ��    ��  : PrintSymbolItem a string to buffer
|  ��    ��  : char* pMsgBuffer����
|              int nLength����
|              char* format����
|  �� �� ֵ  : 
|  �޸ļ�¼  : 2007-4-25 14:38:09   -huangdy-   ����
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
|  ��������  : CSymbolTable::PrintSymbolTable
|  ��    ��  : Fill a formatted listing of the symbol table contents to buffer
|  ��    ��  : char* pMsgBuffer����
|              int nLength����
|  �� �� ֵ  : 
|  �޸ļ�¼  : 2007-4-25 14:38:26   -huangdy-   ����
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
|  ��������  : CSymbolTable::AccountGlobalDataSize
|  ��    ��  : Account global data size(include local variable in main function)
|              and Set variable's offset 
|  �� �� ֵ  : return global data size
|  �޸ļ�¼  : 2007-4-29 10:53:09   -huangdy-   ����
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
