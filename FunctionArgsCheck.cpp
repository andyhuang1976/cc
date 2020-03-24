/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/FunctionArgsCheck.cpp,v 1.3 2014/04/16 12:09:22 administrator Exp $
* 
*******************************************************************************
* 
* Description:implementation file
******************************************************************************/

static const char _CppVersion[] = "@(#)  $Header: /cvsdata/vc/SaxCCompile/FunctionArgsCheck.cpp,v 1.3 2014/04/16 12:09:22 administrator Exp $";
#include "stdafx.h"
// $Nokeywords: $
#include "FunctionArgsCheck.h"
#include "parser.h"

CFunctionArgsCheck::CFunctionArgsCheck()
: m_pFirst(NULL)
, m_pLast(NULL)
{
}

CFunctionArgsCheck::~CFunctionArgsCheck()
{
	if(NULL != m_pFirst ) 
	{
		delete m_pFirst;
	}
}



void CFunctionArgsCheck::DestroyList()
{
	if(NULL != m_pFirst ) 
	{ 
		delete m_pFirst; 
		m_pFirst = NULL;
		m_pLast  = NULL; 
	}
}


/*-----------------------------------------------------------------
|  函数名称  : CFunctionArgsCheck::Insert
|  描    述  : insert a TNK_FUNCTION_DECL node into the list, plus its parameters
|  参    数  : CSyntaxTreeNode* pNode——
|  修改记录  : 2007-4-25 11:06:07   -huangdy-   创建
-----------------------------------------------------------------*/
void CFunctionArgsCheck::Insert( CSyntaxTreeNode* pNode )
{
	assert( pNode->GetNodeType() == TNK_FUNCTION_DECL );

	CFunctionDeclarations* pNewFunctionDec = new CFunctionDeclarations( 
		pNode->GetName(), pNode->GetTokenType());

	CSyntaxTreeNode* pChildNode = pNode->GetChildNode(0);
	if(NULL != pChildNode ) 
	{
		CParameters* pNewParameter = new CParameters( pChildNode->GetTokenType(), 
			pChildNode->GetExtraDataType());
		pNewFunctionDec->AddParameter(NULL,pNewParameter);

		while( pChildNode->GetChain() != NULL) 
		{
			pChildNode = pChildNode->GetChain();
			pNewParameter = pNewFunctionDec->AddParameter(pNewParameter,
				new CParameters( pChildNode->GetTokenType(),pChildNode->GetExtraDataType()));
		}
	}

	if( NULL == m_pFirst )
	{
		m_pFirst = m_pLast = pNewFunctionDec;
	}
	else 
	{ 
		m_pLast->SetNext(pNewFunctionDec);
		m_pLast = m_pLast->GetNext();
	}
}

/*-----------------------------------------------------------------
|  函数名称  : CFunctionArgsCheck::Check
|  描    述  : check if a function call's arguments match its declaration parameters
|  参    数  : CSyntaxTreeNode* pNode——
|  返 回 值  :  -1:	not found
| -2:  type not match
| -3:  match 
| else count not match, return the declaration parameter count
|  修改记录  : 2007-4-25 11:29:11   -huangdy-   创建
-----------------------------------------------------------------*/
int CFunctionArgsCheck::Check( CSyntaxTreeNode* pNode )
{
	assert( pNode->GetNodeType() == TNK_STATEMENT 
		&& pNode->GetStatementKink() == TNK_CALL_EXPR );

	CFunctionDeclarations* pFunctionDec = m_pFirst;
	while(NULL != pFunctionDec && strcmp(pFunctionDec->GetName() , pNode->GetName()) != 0 )
	{
		pFunctionDec = pFunctionDec->GetNext();
	}

	if(NULL == pFunctionDec) 
	{
		return -1;
	}

	CParameters* pParameter = pFunctionDec->GetParameters();
	CSyntaxTreeNode* pChildNode = pNode->GetChildNode(0);

	while( pParameter && pChildNode ) 
	{
		if( (pParameter->GetTokenType()        == pChildNode->GetTokenType()
			&& pParameter->GetExtraDataType() == EDT_ARRAY      == pChildNode->GetExtraDataType() == EDT_ARRAY)
			||
			(pChildNode->GetNodeType()         == TNK_EXPRESSION 
			&& pChildNode->GetExpressionKind() == EK_CONST 
			&& (pChildNode->GetTokenType()     == TT_INTEGER_TYPE 
			|| pChildNode->GetTokenType()      == TT_CHAR_TYPE
			|| pChildNode->GetTokenType()      == TT_REAL_TYPE))) 
		{
			pParameter = pParameter->GetNext();
			pChildNode = pChildNode->GetChain();
		} 
		else                        /* type not match */ 
		{
			return -2;
		}
	}

	if( pParameter || pChildNode )                    // count not match
	{
		return pFunctionDec->GetCount();
	}

	return -3;
}