/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/parser.cpp,v 1.22 2015/08/17 01:05:56 administrator Exp $
* 
*******************************************************************************
* 
* Description:1.检查操作数的类型是否匹配
2.完善运算操作
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/parser.cpp,v 1.22 2015/08/17 01:05:56 administrator Exp $";
#include "stdafx.h"
// $Nokeywords: $
#include "parser.h"
#include <strsafe.h>
#include ".\saxccompileimpl.h"
#include <algorithm> 

#pragma warning(disable : 4995)

TCHAR const  CBINDINGLEVEL_MEMORY_POOL_ID[] = _T("CBindingLevel");
INIT_MEMORY_POOL(CBindingLevel)

/*----------------------------CSyntaxTreeNode--------------------------------------*/

LPCSTR CSyntaxTreeNode::GetSymbol()     
{
	CIdentifierTreeNode* pID = GetName();
	if(NULL != pID)
	{
		return pID->GetTitle();
	}

	return NULL;
}
BOOL CSyntaxTreeNode::IsCmpExpression()
{
	switch(m_nNodeType)
	{
	case TNK_LT_EXPR:
	case TNK_NLT_EXPR:
	case TNK_GT_EXPR:
	case TNK_NGT_EXPR:
	case TNK_EQ_EXPR:
	case TNK_NEQ_EXPR:
		return TRUE;
	}

	return FALSE;

}

BOOL CSyntaxTreeNode::IsArithmeticExpression()
{
	switch(m_nNodeType)
	{
	case TNK_MODIFY_EXPR:
	case TNK_PLUS_EXPR:
	case TNK_MINUS_EXPR:
	case TNK_MULT_EXPR:
	case TNK_TRUNC_DIV_EXP:
	case TNK_TRUNC_MOD_EXPR:
	case TNK_BIT_AND_EXPR:
	case TNK_BIT_IOR_EXPR:
	case TNK_BIT_XOR_EXPR:
	case TNK_PLUS_ASSIGN:
	case TNK_MINUS_ASSIGN:
	case TNK_MULT_ASSIGN:
	case TNK_DIV_ASSIGN:
	case TNK_AND_ASSIGN:
	case TNK_XOR_ASSIGN:
	case TNK_OR_ASSIGN:
	case TNK_TRUTH_NOT_EXPR:
	case TNK_BIT_NOT_EXPR:
	case TNK_PREINCREMENT_EXPR:
	case TNK_PREDECREMENT_EXPR:
	case TNK_NEGATE_EXPR:
	case TNK_POSTDECREMENT_EXPR:
	case TNK_POSTINCREMENT_EXPR:
		return TRUE;
	}

	return FALSE;
}

BOOL CSyntaxTreeNode::IsExpression()
{
	switch(m_nNodeType)
	{
	case TNK_MODIFY_EXPR:
	case TNK_PLUS_EXPR:
	case TNK_MINUS_EXPR:
	case TNK_MULT_EXPR:
	case TNK_TRUNC_DIV_EXP:
	case TNK_TRUNC_MOD_EXPR:
	case TNK_BIT_AND_EXPR:
	case TNK_BIT_IOR_EXPR:
	case TNK_BIT_XOR_EXPR:
	case TNK_LT_EXPR:
	case TNK_GT_EXPR:
	case TNK_EQ_EXPR:
	case TNK_NEQ_EXPR:
	case TNK_PLUS_ASSIGN:
	case TNK_MINUS_ASSIGN:
	case TNK_MULT_ASSIGN:
	case TNK_DIV_ASSIGN:
	case TNK_AND_ASSIGN:
	case TNK_XOR_ASSIGN:
	case TNK_OR_ASSIGN:
	case TNK_NGT_EXPR:
	case TNK_NLT_EXPR:
	case TNK_TRUTH_AND_EXPR:
	case TNK_TRUTH_OR_EXPR:
	case TNK_TRUTH_NOT_EXPR:
	case TNK_BIT_NOT_EXPR:
	case TNK_PREINCREMENT_EXPR:
	case TNK_PREDECREMENT_EXPR:
	case TNK_NEGATE_EXPR:
	case TNK_POSTDECREMENT_EXPR:
	case TNK_POSTINCREMENT_EXPR:
		return TRUE;
	}

	return FALSE;
}

BOOL CSyntaxTreeNode::IsInterger()
{
	switch(m_nNodeType)
	{
	case TNK_BYTE_TYPE:
	case TNK_CHAR_TYPE:
	case TNK_BOOLEAN_TYPE:
	case TNK_WORD_TYPE:
	case TNK_SHORT_TYPE:
	case TNK_DWORD_TYPE:
	case TNK_LONG_TYPE:   
	case TNK_VOID_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONGLONG_TYPE:
		return TRUE;
	}

	return FALSE;
}

BOOL CSyntaxTreeNode::IsFloat()
{
	switch(m_nNodeType)
	{
	case TNK_FLOAT_TYPE:
	case TNK_DOUBLE_TYPE:
		return TRUE;
	}

	return FALSE;
}

BOOL CSyntaxTreeNode::IsNumeric()
{
	switch(m_nNodeType)
	{
	case TNK_BYTE_TYPE:
	case TNK_CHAR_TYPE:
	case TNK_BOOLEAN_TYPE:
	case TNK_WORD_TYPE:
	case TNK_SHORT_TYPE:
	case TNK_DWORD_TYPE:
	case TNK_LONG_TYPE:   
	case TNK_VOID_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONGLONG_TYPE:
	case TNK_FLOAT_TYPE:
	case TNK_DOUBLE_TYPE:
		return TRUE;
	}

	return FALSE;
}

BOOL CSyntaxTreeNode::Ismodifiable()
{
	switch(m_nNodeType)
	{
	case TNK_VAR_REF:
	case TNK_ARRAY_REF:
	case TNK_INDIRECT_REF:   
		return TRUE;
	}

	return FALSE;
}

CSyntaxTreeNode* CSyntaxTreeNode::GetLastChainNode()     // get pLastNode sibling
{
	CSyntaxTreeNode* pLastNode = this;
	CSyntaxTreeNode* pNode   = this;

	while( pNode ) 
	{
		pLastNode = pNode;
		pNode = pNode->m_pChain;
	}

	return pLastNode;
}   
/*---------------------------------------------------------------------------------
| Name   : CSyntaxTreeNode::AddRef
| Desc   : 增加节点的参考引用记录
| Parameter : CSyntaxTreeNode* pSyntaxTreeNode——>节点指针
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CSyntaxTreeNode::AddRef(CSyntaxTreeNode* pSyntaxTreeNode)
{
	ATLASSERT(pSyntaxTreeNode);

	CSyntaxTreeNode* pRefNode = new CRefListTreeNode(pSyntaxTreeNode);

	if(NULL != pRefNode)
	{
		if(NULL == m_pReferences)
		{
			m_pReferences = pRefNode;
		}
		else
		{
			pRefNode->SetChain(m_pReferences);
			m_pReferences = pRefNode;
		}
	}
	else
	{
		throw CParserException("Fail to create the CRefListTreeNode object!");
	}

	return pRefNode;
}



/*----------------------------CTxtConstantsTreeNode--------------------------------------*/

/*---------------------------------------------------------------------------------
| Name   : CTxtConstantsTreeNode::AppendConstantNode
| Desc   : 把指定的文字常量增加到列表
| Return  : 常量节点指针
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
void CTxtConstantsTreeNode::AppendConstantNode(CConstTreeNode* pConstTreeNode)
{
	ATLASSERT(pConstTreeNode->GetNodeType() == TNK_INTEGER_CST
		|| pConstTreeNode->GetNodeType() == TNK_REAL_CST
		|| pConstTreeNode->GetNodeType() == TNK_STRING_CST);

	m_arrConstants.push_back(pConstTreeNode);   
}

/*---------------------------------------------------------------------------------
| Name   : CTxtConstantsTreeNode::RmoveConstantNode
| Desc   : 把指定的常量节点从列表中移除
| Return  : 常量节点指针
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
void CTxtConstantsTreeNode::RmoveConstantNode(CSyntaxTreeNode* pConstTreeNode)
{
	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrConstants.begin();

	while(itr != m_arrConstants.end())
	{
		if(*itr == pConstTreeNode)
		{
			m_arrConstants.erase(itr);
			break;
		}
		itr++;
	}
}

CConstTreeNode* CTxtConstantsTreeNode::GetConstantNode(UINT nIndex)
{
	if(nIndex < m_arrConstants.size())
	{
		return (CConstTreeNode*)m_arrConstants[nIndex];
	}

	return NULL;
}

/*----------------------------CVectorTreeNode--------------------------------------*/
UINT CVectorTreeNode::GetSize()
{ 
	return m_nCount * m_pDataType->GetSize();
} 

/*----------------------------CDeclarationTreeNode--------------------------------------*/
UINT CDeclarationTreeNode::GetSize()
{ 
	UINT nResult = 0;
	if(m_pDataType)
	{
		nResult = m_pDataType->GetSize();
	}

	return nResult;
}

BOOL CDeclarationTreeNode::IsUnsigned()   
{ 
	if(m_nDeclAttribute & NDA_UNSIGNED_FLAG)
	{
		return TRUE;
	}

	CDataTypeTreeNode* pDataType = GetDataType();
	if(pDataType)
	{
		if(pDataType->GetNodeType() == TNK_ARRAY_TYPE)
		{
			pDataType = pDataType->GetDataType();
		}
		switch(pDataType->GetNodeType())
		{
		case TNK_BYTE_TYPE:
		case TNK_WORD_TYPE:
		case TNK_DWORD_TYPE:
		case TNK_POINTER_TYPE:
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CExpressionTreeNode::IsUnsigned()
{
	switch(GetNodeType())
	{
	case TNK_QUESTION_MARK_EXPR:   //问号表达式
		return (m_arrOperands[1]->IsUnsigned() || m_arrOperands[2]->IsUnsigned());
		break;

	case TNK_COMMA_EXPR:         //逗号表达式,判断最后一个操作数
		for(int i = MAX_CHILDREN_NUMBER - 1;i >= 0;i--)
		{
			if(NULL != m_arrOperands[i])
			{
				if(m_arrOperands[i]->IsUnsigned())
				{
					return TRUE;
				}
				break;
			}			
		}
		break;

	default:
		for(UINT i = 0;i < MAX_CHILDREN_NUMBER;i++)
		{
			if(NULL == m_arrOperands[i])
			{
				break;
			}
			if(m_arrOperands[i]->IsUnsigned())
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*----------------------------CParserContext--------------------------------------*/

CParserContext::~CParserContext()
{
	LABLEREF_ARRAY_ITR itr = m_arrLableRefs.begin();
	while(m_arrLableRefs.end() != itr)
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, (*itr)->GetGotoSMTM()->GetLineIndex(),0,
			_T("label '%s' was undefined")
			,CString((*itr)->GetGotoSMTM()->GetSymbol()));

		delete *itr;
		itr++;
	}
}

CLableRef* CParserContext::AppenLableRef(CSyntaxTreeNode* pGotoSMTM,char* pLableName)
{
	CLableRef* pNewLableRef = new CLableRef(pGotoSMTM,pLableName);
	if(pNewLableRef)
	{
		m_arrLableRefs.push_back(pNewLableRef);
	}

	return pNewLableRef;
}

void CParserContext::RelatedLableRef(CSyntaxTreeNode* pLableDeclNode)
{
	ATLASSERT(NULL != pLableDeclNode);

	LABLEREF_ARRAY_ITR itr = m_arrLableRefs.begin();
	while(m_arrLableRefs.end() != itr)
	{
		if((*itr)->GetName() == pLableDeclNode->GetSymbol())
		{
			((CExpressionTreeNode*)((*itr)->GetGotoSMTM()))->SetChildNode(0,pLableDeclNode);
			itr = m_arrLableRefs.erase(itr);
		}
		else
		{
			itr++;
		}
	}
}


/*----------------------------CBindingLevel--------------------------------------*/

CBindingLevel::CBindingLevel(CParserContext* pContext)
	: m_pContext(pContext)
{

}

CBindingLevel::~CBindingLevel()
{
	m_arrDeclNodes.clear();
	m_arrTags.clear();
}

/*---------------------------------------------------------------------------------
| Name   : CBindingLevel::AppendDeclNode
| Desc   : 增加申明节点到列表
| Parameter : CSyntaxTreeNode* pDeclNode——>
| Return  : void 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
void CBindingLevel::AppendDeclNode(CSyntaxTreeNode* pDeclNode)
{
	ATLASSERT(pDeclNode);

	m_arrDeclNodes.push_back(pDeclNode);
}

/*---------------------------------------------------------------------------------
| Name   : CBindingLevel::LookupDeclNode
| Desc   : 根据名字查找指定的节点,lable声明可以和其他的变量声明重名,所以要区分开
| Parameter : char* pName——>
| Parameter : BOOL bLable——>TRUE:表示查找标签声明,否则是变量声明
| Return  : CSyntaxTreeNode* 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CBindingLevel::LookupDeclNode(char* pName,BOOL bLable)
{
	CSyntaxTreeNode*    pResult   = NULL;
	CIdentifierTreeNode*  pIdentifier = NULL;
	SYNTAX_TREE_NODE_ARRAY_ITR itr2 = m_arrDeclNodes.begin();
	while(m_arrDeclNodes.end() != itr2)
	{
		TREE_NODE_TYPE nNodeType = (*itr2)->GetNodeType();

		if(bLable && TNK_LABEL_DECL != nNodeType
			|| !bLable && TNK_LABEL_DECL == nNodeType)
		{
			itr2++;
			continue;
		}

		pIdentifier = (*itr2)->GetName();
		if(pIdentifier)
		{
			if(pIdentifier->GetTitle() == pName)
			{
				pResult = (*itr2);
				break;
			}
		}
		itr2++;
	}

	return pResult;
}

/*---------------------------------------------------------------------------------
| Name   : CBindingLevel::AppendTagNode
| Desc   : 增加申明接点到列表
| Parameter : CSyntaxTreeNode* pDeclNode——>
| Return  : void 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
void CBindingLevel::AppendTagNode(CSyntaxTreeNode* pDeclNode)
{
	ATLASSERT(pDeclNode);

	m_arrTags.push_back(pDeclNode);
}

/*---------------------------------------------------------------------------------
| Name   : CBindingLevel::LookupTagNode
| Desc   : 根据名字查找指定的节点
| Parameter : char* pName——>
| Return  : CSyntaxTreeNode* 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CBindingLevel::LookupTagNode(char* pName)
{
	CSyntaxTreeNode*    pResult   = NULL;
	CIdentifierTreeNode*  pIdentifier = NULL;

	SYNTAX_TREE_NODE_ARRAY_ITR itr2 = m_arrTags.begin();
	while(m_arrTags.end() != itr2)
	{
		pIdentifier = (*itr2)->GetName();
		if(pIdentifier)
		{
			if(pIdentifier->GetTitle() == pName)
			{
				pResult = (*itr2);
				break;
			}
		}
		itr2++;
	}

	return pResult;
}


/*----------------------------CParser--------------------------------------*/
CParser::CParser(CSaxCCompileimpl* pSaxCCompileimpl,UINT nMode)
	: m_pScaner(NULL)
	, m_pSaxCCompileimpl(pSaxCCompileimpl)
	, m_nMode(nMode)
	, m_nMaxID(1)
	, m_pCurrentBindingLevel(NULL)
	, m_pTxtConstantsList(NULL)
{
	memset(m_szScope,SYMBOL_MAX_LENGTH,0);   
}

CParser::~CParser()
{
	Reset();
}

void CParser::Reset()
{
	m_nMaxID     = 1;
	m_strIndentSpace.Empty();

	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrEntries.begin();
	while(m_arrEntries.end() != itr)
	{
		delete *itr;
		itr++;
	}
	m_arrEntries.clear();   


	m_arrDataTypes.clear();   
	m_arrIdentifiers.clear();


	BINDING_LEVEL_ARRAY_ITR itr2 = m_arrBindingLevels.begin();
	while(m_arrBindingLevels.end() != itr2)
	{
		delete (*itr2);
		itr2++;
	}
	m_arrBindingLevels.clear();

	m_arrFilesList.clear();

	delete m_pTxtConstantsList;
	m_pTxtConstantsList = NULL;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::CreateNewBindingLevel
| Desc   : 创建一个新的“CBindingLevel”
| Return  : CBindingLevel* 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
CBindingLevel* CParser::CreateNewBindingLevel(CParserContext* pContext/* = NULL*/)
{
	CBindingLevel* pBindingLevel = new CBindingLevel(pContext);
	if(pBindingLevel)
	{
		m_pCurrentBindingLevel = pBindingLevel;
		m_arrBindingLevels.push_back(pBindingLevel);
	}
	else 
	{
		throw CParserException("Fail to create the CBindingLevel object!");
	}

	return pBindingLevel;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::PopBindingLevel
| Desc   : 删除m_arrBindingLevels数组最后的节点
| Return  : void 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
void CParser::PopBindingLevel()
{
	if(m_arrBindingLevels.size() > 0)
	{
		delete m_pCurrentBindingLevel;
		m_arrBindingLevels.pop_back();

		if(m_arrBindingLevels.size() > 0)
		{
			m_pCurrentBindingLevel = *(m_arrBindingLevels.rbegin()); //设置最后一个节点为当前节点
		}
	}
}

/*---------------------------------------------------------------------------------
| Name   : CParser::LookupDeclNode
| Desc   : 通过名字查找定义的变量
| Parameter : char* pName——>
| Return  : CSyntaxTreeNode* 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CParser::LookupDeclNode(char* pName,BOOL bLable)
{
	CSyntaxTreeNode* pResult = NULL;

	BINDING_LEVEL_ARRAY_RTR rit = m_arrBindingLevels.rbegin();
	while(m_arrBindingLevels.rend() != rit)
	{
		pResult = (*rit)->LookupDeclNode(pName,bLable);
		if(NULL != pResult)
		{
			break;
		}
		rit++;
	}

	return pResult;
}


/*---------------------------------------------------------------------------------
| Name   : CParser::LookupTagNode
| Desc   : 通过名字查找自定义的数据类型(结构、联合、枚举)
| Parameter : char* pName——>
| Return  : CSyntaxTreeNode* 
| Author  : Andy.h
| Date   : 星期三, 四月 16 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CParser::LookupTagNode(char* pName)
{
	CSyntaxTreeNode* pResult = NULL;

	BINDING_LEVEL_ARRAY_RTR rit = m_arrBindingLevels.rbegin();
	while(m_arrBindingLevels.rend() != rit)
	{
		pResult = (*rit)->LookupTagNode(pName);
		if(NULL != pResult)
		{
			break;
		}
		rit++;
	}

	return pResult;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::ListDeclaration
| 描  述 : Analyse whole program,整个程序就是由数据类型和函数两
|       种类型的定义构成的.
| 返 回 值 : 
| 修改记录 : 2007-4-23 20:23:19  -huangdy-  创建
-----------------------------------------------------------------*/

CSyntaxTreeNode* CParser::AnalyseProgram()
{
	CSyntaxTreeNode *pFirstNode = NULL;
	CSyntaxTreeNode *pLastNode = NULL;
	CSyntaxTreeNode *pNewNode = NULL;

	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	UINT nPrefixAttr = 0;

	m_pTxtConstantsList = new CTxtConstantsTreeNode;
	if(NULL == m_pTxtConstantsList)
	{
		throw CParserException("Fail to create the CTxtConstantsTreeNode object!");
	}

	CreateNewBindingLevel();
	CreateInternalConst();


	pFirstNode = m_pTxtConstantsList;
	pLastNode = m_pTxtConstantsList;

	do
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
		if(m_pCurrentToken->GetType() != TT_EOF_TYPE && m_pCurrentToken->GetType() != TT_ERROR_TYPE)
		{
			nPrefixAttr = AnalysePrefixAtti();       

			m_pCurrentToken = m_pScaner->GetNextToken();
			if(m_pCurrentToken->GetType() != TT_EOF_TYPE && m_pCurrentToken->GetType() != TT_ERROR_TYPE)
			{
				pDataTypeTreeNode = AnalyseDataType(nPrefixAttr); 
				if(NULL != pDataTypeTreeNode)
				{
					if( (pNewNode = AnalyseDeclaration(pDataTypeTreeNode,nPrefixAttr)) != NULL )        // link all declarations together
					{   
						if( !pFirstNode )
						{ 
							pFirstNode = pNewNode;
							pLastNode = pNewNode;
						}
						else
						{ 
							pLastNode->SetChain(pNewNode); 
							pLastNode = pNewNode; 
						}
					}        
				}
				else
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_pCurrentToken->GetLineIndex(),0
						,_T("'%s' missing type specifier")
						,CString(m_pCurrentToken->GetSymbolTitle()));
				}
			}
		}     

	}while(m_pCurrentToken->GetType() != TT_EOF_TYPE && m_pCurrentToken->GetType() != TT_ERROR_TYPE);

	return pFirstNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::BuildSyntaxTree
| 描  述 : build the Syntax parse tree
| 参  数 : char* pSourceCode——A pointer to source code string to be parsed
|       int nSourceCodeLen——Length of the source code string.
|       char* pErrorBuffer——A pointer to buffer to contains error messages.
|       int nBufferLen——Length of pErrorBuffer
| 返 回 值 : If the function is successful,return a pointer to root of
|       syntax tree,Otherwise null.  
| 修改记录 : 2007-4-23 13:22:59  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::BuildSyntaxTree( char* const pSourceCode
	,int nSourceCodeLen)
{
	CSyntaxTreeNode* pProgramSyntaxTreeRoot = NULL;
	if(NULL != m_pScaner)
	{
		m_pScaner->SetSourceCode(pSourceCode,nSourceCodeLen);   

		pProgramSyntaxTreeRoot = AnalyseProgram();

		/*   if(pSyntaxTreeRoot)
		{
		pSyntaxTreeRoot->SetChain(pProgramSyntaxTreeRoot);
		}
		else
		{
		pSyntaxTreeRoot = pProgramSyntaxTreeRoot;
		}*/
	}

	return pProgramSyntaxTreeRoot;
}


CDeclarationTreeNode* CParser::CreateIntConstDeclNode(TREE_NODE_TYPE /*nKind*/, char* pName,int nValue)
{
	CDeclarationTreeNode* pNewNode = new CDeclarationTreeNode(TNK_CONST_DECL);
	if(pNewNode)
	{
		pNewNode->SetName(GetIdentifier(pName));

		CConstIntTreeNode* pConstIntTreeNode = CreateConstIntTreeNode(nValue);
		pNewNode->SetInitial(pConstIntTreeNode);
		pNewNode->SetDataType(pConstIntTreeNode->GetDataType());

		pNewNode->SetUID(m_nMaxID++);   

		ATLASSERT(NULL != m_pCurrentBindingLevel);
		if(NULL != m_pCurrentBindingLevel)
		{
			if(m_pCurrentBindingLevel->LookupDeclNode(pName))
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE,0,0,_T(" redefinition of formal parameter '%s'"), pName);
			}
			else
			{
				m_pCurrentBindingLevel->AppendDeclNode(pNewNode);
			}
		}

		m_arrEntries.push_back(pNewNode);
	}
	else
	{
		throw CParserException("Fail to create the CDeclarationTreeNode object!");
	}



	return pNewNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::CreateInternalConst
| Desc   : 创建内建的常量，比如:true(1),false(0)
| Return  : CExpressionTreeNode* 
| Author  : Andy.h
| Date   : 星期二, 四月 15 2014
---------------------------------------------------------------------------------*/
CExpressionTreeNode* CParser::CreateInternalConst()
{
	struct tagInternalConstItem
	{
		TREE_NODE_TYPE nNodeType;
		char*     pName;
		UINT      nValue;
	}arrInternalConstItems[3] =
	{
		{TNK_INT_TYPE,"true",1}
		,{TNK_INT_TYPE,"false",0}
		,{TNK_INT_TYPE,"null",0}
	};

	CDeclarationTreeNode* pHeadNode = NULL;
	CDeclarationTreeNode* pTaileNode = NULL;
	CDeclarationTreeNode* pNewNode = NULL;
	for(UINT i = 0;i < 3;i++)
	{
		pNewNode = CreateIntConstDeclNode(arrInternalConstItems[i].nNodeType
			,arrInternalConstItems[i].pName,arrInternalConstItems[i].nValue);
		if(NULL == pHeadNode)
		{
			pHeadNode = pTaileNode = pNewNode;
		}
		else
		{
			pTaileNode->SetChain(pNewNode);
			pTaileNode = pNewNode;
		}
	}

	CExpressionTreeNode* pExpressionTreeNode = NULL;

	if(NULL != pHeadNode)
	{
		pExpressionTreeNode = CreateExpressionNode(TNK_DECL_STMT);
		pExpressionTreeNode->SetChildNode(0,pHeadNode);
	}

	return pExpressionTreeNode;
}


CIdentifierTreeNode* CParser::GetIdentifier(char* pName)
{
	ATLASSERT(NULL != pName);

	CIdentifierTreeNode* pIdentifier = LookupIdentifier(pName);
	if(NULL == pIdentifier)
	{
		static UINT nNextID = 0;
		pIdentifier = new CIdentifierTreeNode(++nNextID,pName);
		if(NULL != pIdentifier)
		{
			m_arrIdentifiers.push_back(pIdentifier);
			m_arrEntries.push_back(pIdentifier);
		}
		else
		{
			throw CParserException("Fail to create the CIdentifierTreeNode object!");
		}
	}

	return pIdentifier;
}




/*---------------------------------------------------------------------------------
| Name   : CParser::LookupIdentifier
| Desc   : 
| Parameter : char* pName——>
| Return  : CIdentifierTreeNode* 
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CIdentifierTreeNode* CParser::LookupIdentifier(char* pName)
{
	ATLASSERT(NULL != pName);

	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrIdentifiers.begin();
	while(m_arrIdentifiers.end() != itr)
	{
		if(((CIdentifierTreeNode*)(*itr))->GetTitle() == CStringA(pName))
		{
			return (CIdentifierTreeNode*)(*itr);
		}
		itr++;
	}

	return NULL;
}




/*---------------------------------------------------------------------------------
| Name   : ConventToNodeKind
| Desc   : 
| Parameter : AXC_SYMBOL_TYPE nTokenType——>
| Return  : TREE_NODE_TYPE 
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
TREE_NODE_TYPE ConventToNodeKind(AXC_SYMBOL_TYPE nTokenType)
{
	TREE_NODE_TYPE nNodeKind = TNK_UNKNOWN;

	switch(nTokenType)
	{
	case TT_DOUBLE:
		nNodeKind = TNK_DOUBLE_TYPE;
		break;

	case TT_INT:
		nNodeKind = TNK_INT_TYPE;
		break;

	case TT_CHAR:
		nNodeKind = TNK_CHAR_TYPE;
		break;

	case TT_FLOAT:
		nNodeKind = TNK_FLOAT_TYPE;
		break;

	case TT_BYTE:
		nNodeKind = TNK_BYTE_TYPE;
		break;

	case TT_SHORT:
		nNodeKind = TNK_SHORT_TYPE;
		break;

	case TT_BOOL:
		nNodeKind = TNK_BOOLEAN_TYPE;
		break;

	case TT_VOID:
		nNodeKind = TNK_VOID_TYPE;
		break;

	case TT_LONG:
		nNodeKind = TNK_LONG_TYPE;
		break;

	case TT_WORD:
		nNodeKind = TNK_WORD_TYPE;
		break;

	case TT_DWORD:
		nNodeKind = TNK_DWORD_TYPE;
		break;

	case TT_LONGLONG:
		nNodeKind = TNK_LONGLONG_TYPE;
		break;

	default:
		ATLASSERT(false);
	}

	return nNodeKind;
}

CDataTypeTreeNode* CParser::LookupInternalDataType(TREE_NODE_TYPE nTokenType)
{
	CDataTypeTreeNode* pResult = NULL;

	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrDataTypes.begin();

	while(m_arrDataTypes.end() != itr)
	{     
		if((*itr)->GetNodeType() == nTokenType)
		{
			pResult = (CDataTypeTreeNode*)(*itr);
		}
		itr++;
	}

	return pResult;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::GetInternalDataType
| Desc   : 返回简单类型节点，如果不存在就创建一个
| Parameter : AXC_SYMBOL_TYPE nTokenType——>
|       char* pName——>
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::GetInternalDataType(AXC_SYMBOL_TYPE nTokenType,char* pName)
{
	CDataTypeTreeNode* pResult = NULL;

	ATLASSERT(nTokenType >= TT_INT && nTokenType < TT_MAX_DATA_TYPE);
	if(nTokenType >= TT_INT && nTokenType < TT_MAX_DATA_TYPE)
	{
		const TREE_NODE_TYPE nNodeKind = ConventToNodeKind(nTokenType);

		pResult = LookupInternalDataType(nNodeKind);
		if(NULL == pResult)
		{
			UINT nSize = 0;
			switch(nNodeKind)
			{
			case TNK_BYTE_TYPE:
			case TNK_CHAR_TYPE:   /*属TYPE结点类，表示PASCAL中的CHAR类型，不在C中使用。不需使用特殊域。 */
			case TNK_BOOLEAN_TYPE:
				nSize = 1;
				break;

			case TNK_WORD_TYPE:
			case TNK_SHORT_TYPE:
				nSize = 2;
				break;

			case TNK_DWORD_TYPE: 
			case TNK_LONG_TYPE:
			case TNK_FLOAT_TYPE:
			case TNK_VOID_TYPE:    /*属于TYPE结点类，表示C语言中的void 类型。*/ 
			case TNK_INT_TYPE:     /*属于TYPE结点类，表示所有语言中的整型，包括C中的字符型，也用于其它离散类型的子范围类型。包含TYPE_MIN_VALUE、TYPE_MAX_VALUE和TYPE_PRECISION三个域。 对PASCAL中的子类型，TREE_DATA_TYPE将指向其超类型（另一个INTEGER_TYPE、ENUMERAL_TYPE或BOOLEAN_TYPE）。否则TREE_DATA_TYPE为空。*/
				nSize = 4;
				break;

			case TNK_LONGLONG_TYPE:
			case TNK_DOUBLE_TYPE:  /*属于TYPE结点类，表示C语言中的浮点和双精度类型，不同的浮点类型由机器模式、TYPE_SIZE和TYPE_PRECISION区分。*/
				nSize = 8;
				break;
			default:
				ATLASSERT(false);
			}

			pResult = new CDataTypeTreeNode(nNodeKind,nSize);
			if(pResult)
			{
				pResult->SetName(GetIdentifier(pName));          
				m_arrDataTypes.push_back(pResult);

				m_arrEntries.push_back(pResult);
			}
			else 
			{
				throw CParserException(("Fail to create the CDataTypeTreeNode object!"));
			}
		}
	}   

	return pResult;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::CreateDeclNode
| 描  述 : construct a new node
| 参  数 : TREE_NODE_TYPE nKind——
|       AXC_SYMBOL_TYPE nTokenType——
|       char* pID——
| 返 回 值 : If the function is successful,return a pointer to the 
|       new node to be created.Otherwise return NULL.
| 修改记录 : 2007-4-23 13:40:58  -huangdy-  创建
-----------------------------------------------------------------*/
CDeclarationTreeNode* CParser::CreateDeclNode( TREE_NODE_TYPE nKind,CSymbolInfo* pCurrentToken)
{
	ATLASSERT(pCurrentToken);
	ATLASSERT(NULL != m_pCurrentBindingLevel);

	CDeclarationTreeNode* pNewNode = NULL;

	if(NULL != m_pCurrentBindingLevel)
	{
		if(TNK_LABEL_DECL == nKind)
		{  //lable声明在整个函数作用域里面是唯一,并且可以和其它变量重名
			pNewNode = (CDeclarationTreeNode*)LookupDeclNode(pCurrentToken->GetSymbolTitle(),TRUE);
		}
		else
		{
			pNewNode = (CDeclarationTreeNode*)m_pCurrentBindingLevel->LookupDeclNode(pCurrentToken->GetSymbolTitle());
		}

		if(NULL == pNewNode)
		{
			pNewNode = new CDeclarationTreeNode(nKind);

			if(NULL != pNewNode)
			{
				pNewNode->SetLineIndex(pCurrentToken->GetLineIndex());     
				pNewNode->SetTokenType(pCurrentToken->GetType());
				pNewNode->SetName(GetIdentifier(pCurrentToken->GetSymbolTitle()));
				pNewNode->SetUID(m_nMaxID++);

				m_pCurrentBindingLevel->AppendDeclNode(pNewNode);
				m_arrEntries.push_back(pNewNode);
			}
			else 
			{
				throw CParserException(("Fail to create the CDeclarationTreeNode object!"));
			}
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_pCurrentToken->GetLineIndex(),0,_T("'%s' redefinition")
				,CString(m_pCurrentToken->GetSymbolTitle()));
		}
	}   

	return pNewNode;
}

CDataTypeTreeNode* CParser::CreateTypeNode( TREE_NODE_TYPE nKind,CSymbolInfo* pCurrentToken)
{
	ATLASSERT(pCurrentToken);
	ATLASSERT(TNK_ENUMERAL_TYPE == nKind
		|| TNK_RECORD_TYPE == nKind
		|| TNK_UNION_TYPE == nKind);

	CDataTypeTreeNode* pNewNode = NULL;

	if(NULL != m_pCurrentBindingLevel)
	{
		pNewNode = (CDataTypeTreeNode*)m_pCurrentBindingLevel->LookupTagNode(pCurrentToken->GetSymbolTitle());
		if(NULL == pNewNode)
		{
			pNewNode = new CDataTypeTreeNode(nKind,0);

			if(NULL != pNewNode)
			{
				pNewNode->SetLineIndex(pCurrentToken->GetLineIndex());     
				pNewNode->SetTokenType(pCurrentToken->GetType());
				pNewNode->SetName(GetIdentifier(pCurrentToken->GetSymbolTitle()));

				m_pCurrentBindingLevel->AppendTagNode(pNewNode);
				m_arrEntries.push_back(pNewNode);
			}   
			else 
			{
				throw CParserException(("Fail to create a CDataTypeTreeNode object!"));
			}
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_pCurrentToken->GetLineIndex(),0,_T("'%s' redefinition")
				,CString(m_pCurrentToken->GetSymbolTitle()));
		}   
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::CreateStatementNode
| 描  述 : construct a new statment node
| 参  数 : STATEMENT_KIND nKind——
|       char* pID——
| 返 回 值 : If the function is successful,return a pointer to the 
|       new node to be created.Otherwise return NULL.
| 修改记录 : 2007-4-23 13:43:27  -huangdy-  创建
-----------------------------------------------------------------*/
CExpressionTreeNode* CParser::CreateStatementNode( TREE_NODE_TYPE nKind, CSymbolInfo* pCurrentToken)
{
	CExpressionTreeNode* pNewNode = new CExpressionTreeNode(nKind);

	if(NULL != pNewNode)
	{
		if(pCurrentToken)
		{
			pNewNode->SetLineIndex(pCurrentToken->GetLineIndex());
			pNewNode->SetTokenType(pCurrentToken->GetType());
			//pNewNode->SetName(pCurrentToken->GetSymbolTitle());
		}

		m_arrEntries.push_back(pNewNode);
	}
	else 
	{
		throw CParserException(("Fail to create a CExpressionTreeNode object!"));
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::CreateExpressionNode
| 描  述 : construct a new AnalyseFourteenthLevelOp node
| 参  数 : EXPRESSION_KIND nKind——
|       AXC_SYMBOL_TYPE nTokenType——
|       char* pID——
| 返 回 值 : If the function is successful,return a pointer to the 
|       new node to be created.Otherwise return NULL.
| 修改记录 : 2007-4-23 13:48:21  -huangdy-  创建
-----------------------------------------------------------------*/
CExpressionTreeNode* CParser::CreateExpressionNode( TREE_NODE_TYPE nKind
	,UINT nLineIndex,AXC_SYMBOL_TYPE nSymbolType)
{
	ATLASSERT(TNK_UNKNOWN != nKind);
	CExpressionTreeNode* pNewNode = new CExpressionTreeNode(nKind);
	if(NULL != pNewNode)
	{
		pNewNode->SetLineIndex(nLineIndex);
		pNewNode->SetTokenType(nSymbolType);

		m_arrEntries.push_back(pNewNode);
	}
	else 
	{
		throw CParserException(("Fail to create a CExpressionTreeNode object!"));
	}
	return pNewNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::CreateExpressionNode
| 描  述 : construct a new AnalyseFourteenthLevelOp node
| 参  数 : EXPRESSION_KIND nKind——
|       AXC_SYMBOL_TYPE nTokenType——
|       char* pID——
| 返 回 值 : If the function is successful,return a pointer to the 
|       new node to be created.Otherwise return NULL.
| 修改记录 : 2007-4-23 13:48:21  -huangdy-  创建
-----------------------------------------------------------------*/
CExpressionTreeNode* CParser::CreateExpressionNode(CSymbolInfo* pCurrentToken)
{
	ATLASSERT(pCurrentToken);

	return CreateExpressionNode((TREE_NODE_TYPE)pCurrentToken->GetNodeType()
		,pCurrentToken->GetLineIndex(),pCurrentToken->GetType());   
}

CExpressionTreeNode* CParser::CreateExpressionNode(
	TREE_NODE_TYPE nKind
	,UINT nLineIndex
	,AXC_SYMBOL_TYPE nSymbolType
	,CDataTypeTreeNode* pDataType
	,CSyntaxTreeNode *pExprNode1
	,CSyntaxTreeNode *pExprNode2/* = NULL*/)
{
	CExpressionTreeNode* pResult = CreateExpressionNode(nKind,nLineIndex,nSymbolType);

	if(NULL != pResult)
	{
		pResult->SetDataType(pDataType);		 
		pResult->SetChildNode(0,pExprNode1);
		pResult->SetChildNode(1,pExprNode2);

		AppendRef(pExprNode1,pResult);
		if(NULL != pExprNode2)
		{
			AppendRef(pExprNode2,pResult);
		}
	}

	return pResult;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::CreateExpressionNode
| 描  述 : construct a new AnalyseFourteenthLevelOp node
| 参  数 : EXPRESSION_KIND nKind——
|       AXC_SYMBOL_TYPE nTokenType——
|       char* pID——
| 返 回 值 : If the function is successful,return a pointer to the 
|       new node to be created.Otherwise return NULL.
| 修改记录 : 2007-4-23 13:48:21  -huangdy-  创建
-----------------------------------------------------------------*/
CExpressionTreeNode* CParser::CreateExpressionNode(
	CSymbolInfo* pCurrentToken
	,CDataTypeTreeNode* pDataType
	,CSyntaxTreeNode *pExprNode1
	,CSyntaxTreeNode *pExprNode2/* = NULL*/)
{
	ATLASSERT(pCurrentToken);

	return CreateExpressionNode((TREE_NODE_TYPE)pCurrentToken->GetNodeType()
		, pCurrentToken->GetLineIndex()
		, pCurrentToken->GetType()
		, pDataType
		, pExprNode1
		, pExprNode2);
}

CExpressionTreeNode* CParser::CreateExpressionNode(
	CSymbolInfo* pCurrentToken
	,AXC_SYMBOL_TYPE nDataType
	,char* lpName
	,CSyntaxTreeNode *pExprNode1
	,CSyntaxTreeNode *pExprNode2)
{
	ATLASSERT(pCurrentToken);

	return CreateExpressionNode(pCurrentToken,GetInternalDataType(nDataType,lpName),pExprNode1,pExprNode2);
}

CExpressionTreeNode* CParser::CreateExpressionNode(
	TREE_NODE_TYPE nKind
	,UINT nLineIndex
	,AXC_SYMBOL_TYPE nSymbolType
	,CSyntaxTreeNode *pExprNode1
	,CSyntaxTreeNode *pExprNode2)
{
	CDataTypeTreeNode* pExprDataType   = pExprNode1->GetDataType();   

	if(nKind != TNK_LSHIFT_EXPR   && nKind != TNK_RSHIFT_EXPR)
	{
		CDataTypeTreeNode* pLeftNodeDataType = pExprDataType;   
		CDataTypeTreeNode* pRightNodeDataType = pExprNode2->GetDataType();

		if(pLeftNodeDataType->GetNodeType() == TNK_FLOAT_TYPE
			|| pLeftNodeDataType->GetNodeType() == TNK_DOUBLE_TYPE)
		{
			pExprDataType = pLeftNodeDataType;
		}
		else if(pRightNodeDataType->GetNodeType() == TNK_FLOAT_TYPE
			|| pRightNodeDataType->GetNodeType() == TNK_DOUBLE_TYPE)
		{
			pExprDataType = pRightNodeDataType;
		}
		else if(pLeftNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
		{
			pExprDataType = pLeftNodeDataType;
		}
		else if(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
		{
			pExprDataType = pRightNodeDataType;
		}
		else if(pLeftNodeDataType->GetNodeType() == TNK_POINTER_TYPE)
		{
			pExprDataType = pLeftNodeDataType;
		}
		else if(pRightNodeDataType->GetNodeType() == TNK_POINTER_TYPE)
		{
			pExprDataType = pRightNodeDataType;
		}
		else
		{
			if(pLeftNodeDataType->GetSize() < pRightNodeDataType->GetSize())
			{
				pExprDataType = pRightNodeDataType;
			}     
		}
	}

	return CreateExpressionNode(nKind,nLineIndex,nSymbolType,pExprDataType,pExprNode1,pExprNode2);
}


CExpressionTreeNode* CParser::CreateExpressionNode(
	CSymbolInfo* pCurrentToken
	,CSyntaxTreeNode *pExprNode1
	,CSyntaxTreeNode *pExprNode2)
{
	ATLASSERT(pCurrentToken);

	return CreateExpressionNode((TREE_NODE_TYPE)pCurrentToken->GetNodeType()
		, pCurrentToken->GetLineIndex()
		, pCurrentToken->GetType()
		, pExprNode1
		, pExprNode2);
}



void CParserBase::AppendRef(CSyntaxTreeNode* pNodeReferenced,CSyntaxTreeNode* pSyntaxTreeNode)
{
	ATLASSERT(pNodeReferenced && pSyntaxTreeNode);
	if(NULL != pNodeReferenced && NULL != pSyntaxTreeNode)
	{
		CSyntaxTreeNode* pRefNode = pNodeReferenced->AddRef(pSyntaxTreeNode);
		m_arrEntries.push_back(pRefNode);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::Destroy
| 描  述 : 删除指定的节点,如果是文字常量节点,还是从常量列表中移出
| 参  数 : CSyntaxTreeNode* pNode——被删除的节点指针
| 修改记录 : 2007-4-23 13:50:28  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::Destroy(CSyntaxTreeNode* pNode)
{
	ATLASSERT(pNode);

	SYNTAX_TREE_NODE_ARRAY_ITR itr = std::find(m_arrEntries.begin(),m_arrEntries.end(),pNode);
	if(m_arrEntries.end() != itr)
	{
		m_arrEntries.erase(itr);
	}

	if(pNode->GetNodeType() == TNK_INTEGER_CST
		|| pNode->GetNodeType() == TNK_REAL_CST
		|| pNode->GetNodeType() == TNK_STRING_CST)
	{
		((CConstTreeNode*)pNode)->Release();
		if(((CConstTreeNode*)pNode)->GetRef() <= 0)
		{
			m_pTxtConstantsList->RmoveConstantNode(pNode);
			delete pNode;
		}
	}
	else
	{
		delete pNode;
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::Match
| 描  述 : get the next token, check if its ASM_TOKEN_TYPE is expected
| 参  数 : AXC_SYMBOL_TYPE nTokenType——
| 返 回 值 : TRUE----
|       FALSE---
| 修改记录 : 2007-4-23 13:50:28  -huangdy-  创建
-----------------------------------------------------------------*/
BOOL CParser::Match( AXC_SYMBOL_TYPE nTokenType )
{
	m_pCurrentToken = m_pScaner->GetNextToken();
	return (m_pCurrentToken->GetType() == nTokenType);
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::ConsumeUntil
| 描  述 : for error recovery
| 参  数 : AXC_SYMBOL_TYPE nTokenType——
| 返 回 值 : 
| 修改记录 : 2007-4-23 14:02:16  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::ConsumeUntil( AXC_SYMBOL_TYPE nTokenType )
{
	while( m_pCurrentToken->GetType() != nTokenType && m_pCurrentToken->GetType() != TT_EOF_TYPE )
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}
}

void CParser::ConsumeUntil( AXC_SYMBOL_TYPE nTokenType1, AXC_SYMBOL_TYPE nTokenType2 )
{
	while( m_pCurrentToken->GetType() != nTokenType1 
		&& m_pCurrentToken->GetType() != nTokenType2 
		&& m_pCurrentToken->GetType() != TT_EOF_TYPE )
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}
}
/*---------------------------------------------------------------------------------
| Name   : CParser::ConvertConstRealToInt
| Desc   : 把浮点数类型的常量转换为整形常量
| Return  : 常量节点
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CConstIntTreeNode* CParser::ConvertConstRealToInt(CConstTreeNode* pSrc)
{
	CConstIntTreeNode* pConstIntTreeNode = NULL;
	ATLASSERT(NULL != pSrc);
	if(NULL != pSrc)
	{
		ATLASSERT(pSrc->IsNumericConstants());

		if(pSrc->GetNodeType() == TNK_INTEGER_CST
			|| pSrc->GetNodeType() != TNK_REAL_CST)
		{
			return (CConstIntTreeNode*)pSrc;
		}

		pConstIntTreeNode = CreateConstIntTreeNode((int)pSrc->GetValue2());
		if(NULL == pConstIntTreeNode)
		{
			pConstIntTreeNode = (CConstIntTreeNode*)pSrc;
		}
		else
		{
			Destroy(pSrc);
		}
	}

	return pConstIntTreeNode;
}

CConstIntTreeNode*   CParser::LookupConstIntTreeNode(LONGLONG nValue)
{
	CConstTreeNode* pConstTreeNode = NULL;
	for(UINT i = 0;i < m_pTxtConstantsList->GetConstantsCount();i++)
	{
		pConstTreeNode = m_pTxtConstantsList->GetConstantNode(i);
		if(pConstTreeNode->GetValue1() == nValue)
		{
			return (CConstIntTreeNode*)pConstTreeNode;
		}
	}

	return NULL;
}

CConstRealTreeNode*   CParser::LookupConstRealTreeNode(double nValue)
{
	CConstTreeNode* pConstTreeNode = NULL;
	for(UINT i = 0;i < m_pTxtConstantsList->GetConstantsCount();i++)
	{
		pConstTreeNode = m_pTxtConstantsList->GetConstantNode(i);
		if(pConstTreeNode->GetValue2() == nValue)
		{
			return (CConstRealTreeNode*)pConstTreeNode;
		}
	}

	return NULL;
}

CConstStringTreeNode*   CParser::LookupConstStringTreeNode(char* pValue)
{
	CConstTreeNode* pConstTreeNode = NULL;
	for(UINT i = 0;i < m_pTxtConstantsList->GetConstantsCount();i++)
	{
		pConstTreeNode = m_pTxtConstantsList->GetConstantNode(i);
		if(pConstTreeNode->GetValue3() == pValue)
		{
			return (CConstStringTreeNode*)pConstTreeNode;
		}
	}

	return NULL;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::CreateConstIntTreeNode
| Desc   : 创建整型常量节点并设置其类型为整形
| Return  : 被创建的常量节点
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CConstIntTreeNode*   CParser::CreateConstIntTreeNode(LONGLONG nValue)
{   
	CConstIntTreeNode* pConstIntTreeNode = NULL;/*LookupConstIntTreeNode(nValue);*/
	//if(NULL == pConstIntTreeNode)
	{
		pConstIntTreeNode= new CConstIntTreeNode(nValue);   
		if(pConstIntTreeNode)
		{
			if(nValue <= 0xffffffff)
			{
				pConstIntTreeNode->SetDataType(GetInternalDataType(TT_INT,"int"));
			}
			else
			{
				pConstIntTreeNode->SetDataType(GetInternalDataType(TT_LONGLONG,"LONGLONG"));
			}
			m_pTxtConstantsList->AppendConstantNode(pConstIntTreeNode);
			m_arrEntries.push_back(pConstIntTreeNode);
		}
		else 
		{
			throw CParserException(("Fail to create a CConstIntTreeNode object!"));
		}
	}
	pConstIntTreeNode->AddRef();

	return pConstIntTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::CreateConstRealTreeNode
| Desc   : 创建浮点数常量节点并设置其类型为整形
| Return  : 被创建的常量节点
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CConstRealTreeNode*   CParser::CreateConstRealTreeNode(double nValue)
{
	CConstRealTreeNode* pConstRealTreeNode = NULL;//LookupConstRealTreeNode(nValue);

	//if(NULL == pConstRealTreeNode)
	{
		pConstRealTreeNode = new CConstRealTreeNode(nValue);
		if(NULL != pConstRealTreeNode)
		{
			pConstRealTreeNode->SetDataType(GetInternalDataType(TT_DOUBLE,"double"));   

			m_pTxtConstantsList->AppendConstantNode(pConstRealTreeNode);
			m_arrEntries.push_back(pConstRealTreeNode);
		}
		else 
		{
			throw CParserException(("Fail to create a CConstRealTreeNode object!"));
		}
	}

	pConstRealTreeNode->AddRef();

	return pConstRealTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::CreateConstStringTreeNode
| Desc   : 创建字符串常量节点并设置其类型为字符型指针
| Return  : 被创建的常量节点
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/
CConstStringTreeNode*   CParser::CreateConstStringTreeNode(char* pValue)
{
	CConstStringTreeNode* pConstStringTreeNode = NULL;//LookupConstStringTreeNode(pValue);

	//if(NULL == pConstStringTreeNode)
	{
		pConstStringTreeNode = new CConstStringTreeNode(pValue);
		if(NULL != pConstStringTreeNode) 
		{
			CDataTypeTreeNode* pBaseDataTypeTreeNode = GetInternalDataType(TT_CHAR,"char");
			pConstStringTreeNode->SetDataType(GetPointerDataType(pBaseDataTypeTreeNode,1));   

			m_pTxtConstantsList->AppendConstantNode(pConstStringTreeNode);  //把字符串常量数据加入列表
			m_arrEntries.push_back(pConstStringTreeNode);
		}
		else 
		{
			throw CParserException(("Fail to create a CConstStringTreeNode object!"));
		}
	}

	pConstStringTreeNode->AddRef();

	return pConstStringTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::AnalysePrefixAtti
| Desc   : 分析变量的修饰属性，包括:"static","register","exter","const"和"unsigned",
|       并且要求前面四个不能同时使用,"unsigned"需要特殊处理
| Return  : UINT ——>修饰属性
| Author  : Andy.h
| Date   : 星期五, 四月 11 2014
---------------------------------------------------------------------------------*/

UINT CParser::AnalysePrefixAtti()
{
	UINT nResult = 0;

	switch(m_pCurrentToken->GetType())
	{
	case TT_REGISTER:
		nResult |= NDA_REGDECL_FLAG;    //有"register"修饰符号    
		break;
	case TT_EXTERN:
		nResult |= NDA_EXTERNAL_FLAG;   //有"extern"修饰符号    
		break;
	case TT_STATIC:
		nResult |= NDA_STATIC_FLAG;    //有"static"修饰符号    
		break;

	case TT_CONST:
		nResult |= NDA_CONST_FLAG;     //有"const"修饰符号    
		break;
	}

	if(0 != nResult)
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}

	if(TT_UNSIGNED == m_pCurrentToken->GetType())
	{
		nResult |= NDA_UNSIGNED_FLAG;    //有"const"修饰符号 
	}
	else
	{
		m_pScaner->PushBack();
	}

	return nResult;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::AnalyseDataType
| Desc   : 分析数据类型，如果成功返回类型节点
| Parameter : UINT nPrefixAttr——>变量的修饰属性,因为c语言的"unsigned"
|       等同于"unsigned int"
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::AnalyseDataType(UINT nPrefixAttr)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	AXC_SYMBOL_TYPE nTokenType = m_pCurrentToken->GetType();

	if(TT_IDENTIFY_TYPE == nTokenType)  //有可能是自定义类型
	{
		pDataTypeTreeNode = (CDataTypeTreeNode*)LookupTagNode(m_pCurrentToken->GetSymbolTitle());
		//if(NULL == pDataTypeTreeNode)
		//{
		//   if(NULL == LookupDeclNode(m_pCurrentToken->GetSymbolTitle()) ) //也不是定义的变量 
		//   {
		//     m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("invalid type '%s'")
		//        ,CString(m_pCurrentToken->GetSymbolTitle()));
		//   }
		//}
	}
	else
	{
		if( nTokenType >= TT_INT && nTokenType < TT_MAX_DATA_TYPE) 
		{     
			pDataTypeTreeNode = GetInternalDataType(nTokenType,m_pCurrentToken->GetSymbolTitle());   
		} 
		else 
		{
			if(NDA_UNSIGNED_FLAG & nPrefixAttr)
			{
				if(TT_IDENTIFY_TYPE == nTokenType || TT_MULT == nTokenType)  //说明没有具体的类型， 默认为"int"类型
				{
					pDataTypeTreeNode = GetInternalDataType(TT_INT,"int");   
				}
				else
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("invalid type '%s'")
						,CString(m_pCurrentToken->GetSymbolTitle()));

					ConsumeUntil( TT_SEMI/* ';' */);   // error recovery
				}
			}     
		}     
	}

	return pDataTypeTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::LookupPointerDataType
| Desc   : 查找指针类型
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型指针
|       UINT nPointerDepth——>指针深度
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::LookupPointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPointerDepth)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;

	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrDataTypes.begin();

	while(m_arrDataTypes.end() != itr)
	{
		pDataTypeTreeNode = (CDataTypeTreeNode*)(*itr);
		if(pDataTypeTreeNode->GetNodeType() == TNK_POINTER_TYPE)
		{
			if(pDataTypeTreeNode->GetDataType() == pBaseDataTypeTreeNode
				&& pDataTypeTreeNode->GetPointerDepth() == nPointerDepth)
			{
				return pDataTypeTreeNode;
			}
		}
		itr++;
	}

	return NULL;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::LookupRefrenceDataType
| Desc   : 查询指定类型的参考类型
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型节点
| Return  :( CDataTypeTreeNode* )成功返回类型节点指针
| Author  : Andy.h
| Date   : 星期日, 四月 13 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::LookupRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;

	SYNTAX_TREE_NODE_ARRAY_ITR itr = m_arrDataTypes.begin();

	while(m_arrDataTypes.end() != itr)
	{
		pDataTypeTreeNode = (CDataTypeTreeNode*)(*itr);
		if(pDataTypeTreeNode->GetNodeType() == TNK_REFERENCE_TYPE
			&& pDataTypeTreeNode->GetDataType() == pBaseDataTypeTreeNode)
		{
			return pDataTypeTreeNode;
		}
		itr++;
	}

	return NULL;
}

/*---------------------------------------------------------------------------------
| Name   : GetPointerDataType
| Desc   : 得到指定的指针类型，如果不存在几创建一个
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型指针
|       UINT nPointerDepth——>指针深度
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::GetPointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPointerDepth)
{
	ATLASSERT(NULL != pBaseDataTypeTreeNode);
	//ATLASSERT(pBaseDataTypeTreeNode->GetPointerDepth() == 0);

	CDataTypeTreeNode* pDataTypeTreeNode = LookupPointerDataType(pBaseDataTypeTreeNode,nPointerDepth);
	if(NULL == pDataTypeTreeNode)
	{
		pDataTypeTreeNode = new CDataTypeTreeNode(TNK_POINTER_TYPE,(GetMode() == ACM_MODE_X86)? 4:8);
		if(pDataTypeTreeNode)
		{
			pDataTypeTreeNode->SetDataType(pBaseDataTypeTreeNode);
			pDataTypeTreeNode->SetPointerDepth((BYTE)nPointerDepth);
			pDataTypeTreeNode->SetName(GetIdentifier("Pointer"));

			m_arrDataTypes.push_back(pDataTypeTreeNode);
			m_arrEntries.push_back(pDataTypeTreeNode);
		}
		else 
		{
			throw CParserException("Fail to create the pointer data type!");
		}
	}


	return pDataTypeTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::GetArrayDataType
| Desc   : 创建指定的数组类型
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型指针
|       UINT nElements——>元素个数
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::GetArrayDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nElements)
{
	ATLASSERT(NULL != pBaseDataTypeTreeNode);
	ATLASSERT(pBaseDataTypeTreeNode->GetPointerDepth() == 0);


	CDataTypeTreeNode* pDataTypeTreeNode = new CDataTypeTreeNode(TNK_ARRAY_TYPE,nElements * pBaseDataTypeTreeNode->GetSize());
	if(pDataTypeTreeNode)
	{
		pDataTypeTreeNode->SetDataType(pBaseDataTypeTreeNode);   
		pDataTypeTreeNode->SetName(GetIdentifier("Array"));
		pDataTypeTreeNode->SetMinval(nElements);

		m_arrDataTypes.push_back(pDataTypeTreeNode);
		m_arrEntries.push_back(pDataTypeTreeNode);
	}
	else 
	{
		throw CParserException(("Fail to create the array data type!"));
	}


	return pDataTypeTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::GetRefrenceDataType
| Desc   : 得到指定类型的参考类型节点指针
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型节点指针
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期日, 四月 13 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::GetRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode)
{
	ATLASSERT(NULL != pBaseDataTypeTreeNode);

	CDataTypeTreeNode* pDataTypeTreeNode = LookupRefrenceDataType(pBaseDataTypeTreeNode);
	if(NULL == pDataTypeTreeNode)
	{
		pDataTypeTreeNode = new CDataTypeTreeNode(TNK_REFERENCE_TYPE,(GetMode() == ACM_MODE_X86)? 4:8);
		if(pDataTypeTreeNode)
		{
			pDataTypeTreeNode->SetDataType(pBaseDataTypeTreeNode);
			pDataTypeTreeNode->SetName(GetIdentifier("Refrence"));

			m_arrDataTypes.push_back(pDataTypeTreeNode);
			m_arrEntries.push_back(pDataTypeTreeNode);
		}
		else 
		{
			throw CParserException(("Fail to create the pointer data type!"));
		}
	}


	return pDataTypeTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::AnalysePointerDataType
| Desc   : 分析是否是指针数据类型，如果不是直接返回基本类型节点指针
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型节点指针
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::AnalysePointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	UINT nPointerDepth = 0;

	m_pCurrentToken = m_pScaner->GetNextToken();   
	while(TT_MULT == m_pCurrentToken->GetType())
	{
		nPointerDepth++;

		m_pCurrentToken = m_pScaner->GetNextToken();
	}

	if(nPointerDepth > 0)
	{
		pDataTypeTreeNode = GetPointerDataType(pBaseDataTypeTreeNode,nPointerDepth);     
	}
	else
	{
		pDataTypeTreeNode = pBaseDataTypeTreeNode;
	}

	m_pScaner->PushBack();  //说明是指针类型

	return pDataTypeTreeNode;
}

/*---------------------------------------------------------------------------------
| Name   : CParser::AnalysePointerDataType
| Desc   : 分析是否是指针数据类型，如果不是直接返回基本类型节点指针
| Parameter : CDataTypeTreeNode* pBaseDataTypeTreeNode——>基本类型节点指针
| Return  : CDataTypeTreeNode* 
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CDataTypeTreeNode* CParser::AnalyseRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;

	m_pCurrentToken = m_pScaner->GetNextToken();
	if(m_pCurrentToken->GetType() == TT_BITWISE_AND)
	{
		pDataTypeTreeNode = GetRefrenceDataType(pBaseDataTypeTreeNode);
	}
	else
	{
		pDataTypeTreeNode = pBaseDataTypeTreeNode;
		m_pScaner->PushBack();  //说明是指针类型
	}   

	return pDataTypeTreeNode;
}




/*-----------------------------------------------------------------
| 函数名称 : CParser::Declaration
| 描  述 : Analyse variable or function declaration. Grammar 3:
|       Declaration->VariableDeclaration | FunctionDeclaration
| 返 回 值 : If the function is successful,return a pointer to 
|       the syntax tree node to be created,Otherwise null.
| 修改记录 : 2007-4-23 14:13:43  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseDeclaration(CDataTypeTreeNode* pDataTypeTreeNode,UINT nPrefixAttr)
{   
	CSyntaxTreeNode* pNewNode = NULL;

	m_pScaner->SaveStatusPoint();

	do
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}while(TT_MULT == m_pCurrentToken->GetType());  //跳过“*"号

	if(m_pCurrentToken->GetType() == TT_BITWISE_AND) //跳过参考符号
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}

	//StringCchPrintfA(m_szScope,SYMBOL_MAX_LENGTH,"global");      // global function or variable Declaration
	//m_objTypeToken = *m_pCurrentToken;
	//m_pCurrentToken = m_pScaner->GetNextToken();
	//m_objIDToken  = *(m_pScaner->GetNextToken());

	if( m_pCurrentToken->GetType() == TT_IDENTIFY_TYPE ) 
	{
		m_pCurrentToken = m_pScaner->GetNextToken(); 
		switch(m_pCurrentToken->GetType())
		{
		case TT_LPARAN:
			m_pScaner->RecoverStatusPoint();
			pNewNode = AnalyseFunctionsDefine(pDataTypeTreeNode,nPrefixAttr);
			break;

		case TT_SEMI:
		case TT_LSQUARE:
		case TT_COMMA:
		case TT_ASSIGN:
			m_pScaner->RecoverStatusPoint();
			pNewNode = AnalyseVariablesDeclaration(pDataTypeTreeNode,nPrefixAttr | NDA_STATIC_FLAG); //申明在函数外面的变量应该静态的,不管是否明确定义
			break;     

		default:   //Error
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ';' after identifier \"%s\".")
				,CString(m_pCurrentToken->GetSymbolTitle()));
			ConsumeUntil( TT_SEMI, TT_RBRACE );
		}
	} 
	else 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("\"%s\" is a reserved token.")
			,CString(m_pCurrentToken->GetSymbolTitle()));

		ConsumeUntil( TT_SEMI, TT_RBRACE );
	}

	return pNewNode;
}
CString GetPointerDepthDesc(UINT nDepth)
{
	CString strResult(_T("*"));
	if(nDepth > 1)
	{
		for(UINT i = 1;i < nDepth;i++)
		{
			strResult += _T("*");
		}
	}

	return strResult;
}

void CParser::TypeCheckingforArrayInit(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode)
{
	CDataTypeTreeNode* pLeftDataTypeNode = pLeftNode->GetDataType();
	CDataTypeTreeNode* pRightDataTypeNode = pRightNode->GetDataType();

	CDataTypeTreeNode* pLeftBaseDataTypeTreeNode = pLeftDataTypeNode->GetDataType();
	if(NULL == pLeftBaseDataTypeTreeNode || NULL == pRightDataTypeNode)
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
			,_T("illegal indirection"));
		return;
	}
	if(!pRightNode->IsNumericConstants())
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
			,_T("Expect a constant"));
		return;
	}


	CDataTypeTreeNode* pBaseDataTypeTreeNode;
	switch(pLeftBaseDataTypeTreeNode->GetNodeType())
	{
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:          
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE: 
	case TNK_LONGLONG_TYPE:  
	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		switch(pRightDataTypeNode->GetNodeType())
		{
		case TNK_POINTER_TYPE:
		case TNK_REFERENCE_TYPE:
			pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
				,_T("cannot convert from '%s *' to '%s'")
				,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
				,CString(pLeftBaseDataTypeTreeNode->GetSymbol()));
			break;

		case TNK_ARRAY_TYPE:
			pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
				,_T("cannot convert from '%s []' to '%s'")
				,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
				,CString(pLeftBaseDataTypeTreeNode->GetSymbol()));
			break;

		case TNK_CHAR_TYPE:
		case TNK_BYTE_TYPE:
		case TNK_BOOLEAN_TYPE:          
		case TNK_SHORT_TYPE:
		case TNK_WORD_TYPE:
		case TNK_INT_TYPE:
		case TNK_LONG_TYPE:
		case TNK_DWORD_TYPE: 
		case TNK_LONGLONG_TYPE:
		case TNK_FLOAT_TYPE:       
		case TNK_DOUBLE_TYPE:
			if(!pRightNode->IsNumericConstants())
			{
				if(pLeftBaseDataTypeTreeNode->GetSize() < pRightDataTypeNode->GetSize()
					|| pLeftBaseDataTypeTreeNode != pRightDataTypeNode)
				{
					m_pSaxCCompileimpl->OutputErrMsg(FALSE, pLeftNode->GetLineIndex(),0
						,_T("conversion from '%s' to '%s', possible loss of data")
						,CString(pRightDataTypeNode->GetSymbol()),CString(pLeftBaseDataTypeTreeNode->GetSymbol()));
				}   
			}
			break;

		default:
			if(NULL == pRightDataTypeNode)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("illegal indirection"));
			}
		}     
		break;

	case TNK_POINTER_TYPE:   
		switch(pRightDataTypeNode->GetNodeType())
		{
		case TNK_ARRAY_TYPE:
			if(pLeftBaseDataTypeTreeNode->GetPointerDepth() != 1
				|| pRightDataTypeNode->GetDataType() != pLeftBaseDataTypeTreeNode->GetDataType())
			{
				pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();

				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s*' to '%s%s'")
					,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
					,CString(pLeftBaseDataTypeTreeNode->GetSymbol())
					,GetPointerDepthDesc(pLeftBaseDataTypeTreeNode->GetPointerDepth()));
			}
			break;

		case TNK_POINTER_TYPE:
			if(pRightDataTypeNode->GetDataType() != pLeftBaseDataTypeTreeNode->GetDataType()
				|| pRightDataTypeNode->GetPointerDepth() != pLeftBaseDataTypeTreeNode->GetPointerDepth())
			{
				pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s%s' to '%s%s'")
					,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
					,GetPointerDepthDesc(pRightDataTypeNode->GetPointerDepth())
					,CString(pLeftBaseDataTypeTreeNode->GetSymbol())
					,GetPointerDepthDesc(pLeftDataTypeNode->GetPointerDepth()));
			}
			break;

		default:
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s' to '%s%s'")
					,CString(pRightDataTypeNode->GetSymbol())
					,CString(pLeftBaseDataTypeTreeNode->GetSymbol())
					,GetPointerDepthDesc(pLeftBaseDataTypeTreeNode->GetPointerDepth()));
			}
		}
		break;

	default:
		ATLASSERT(FALSE);
	}   
}

/*---------------------------------------------------------------------------------
| Name   : CParser::TypeCompatibleCheckingofPointer
| Desc   : 指针兼容性检查
| Parameter :CDataTypeTreeNode* pLeftDataTypeNode——>
|      CDataTypeTreeNode* pRightDataTypeNode——>
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
void CParser::TypeCompatibleCheckingofPointer(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode)
{
	CDataTypeTreeNode* pLeftDataTypeNode = pLeftNode->GetDataType();
	CDataTypeTreeNode* pRightDataTypeNode = pRightNode->GetDataType();

	ATLASSERT(pLeftDataTypeNode && pRightDataTypeNode);
	int nDepth1 = 0;
	int nDepth2 = 0;
	pLeftDataTypeNode = pLeftDataTypeNode->GetPointerBaseType(nDepth1);   
	pRightDataTypeNode = pRightDataTypeNode->GetPointerBaseType(nDepth2);   

	if(pRightDataTypeNode->GetDataType() != pLeftDataTypeNode->GetDataType()
		|| nDepth1 != nDepth2)
	{
		CString strDataType1 = (NULL != pRightDataTypeNode)? CString(pRightDataTypeNode->GetSymbol()) : _T("");
		CString strDataType2 = (NULL != pLeftDataTypeNode)? CString(pLeftDataTypeNode->GetSymbol()) : _T("");

		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
			,_T("cannot convert from '%s%s' to '%s%s'")
			,strDataType1
			,GetPointerDepthDesc(nDepth2)
			,strDataType2
			,GetPointerDepthDesc(nDepth1));
	}
}


/*---------------------------------------------------------------------------------
| Name   : CParser::TypeCompatibleChecking
| Desc   : 检查赋值操作两边的数据类型是否匹配
| Parameter :CSyntaxTreeNode* pLeftNode——>被赋值的变量
|      CSyntaxTreeNode* pRightNode——>右边的表达式
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
void CParser::TypeCompatibleChecking(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode,BOOL bInitCheck)
{
	CDataTypeTreeNode* pLeftDataTypeNode = pLeftNode->GetDataType();
	CDataTypeTreeNode* pRightDataTypeNode = pRightNode->GetDataType();

	if(NULL == pLeftDataTypeNode || NULL == pRightDataTypeNode)
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
			,_T("illegal indirection"));
		return;
	}
	if(pRightDataTypeNode->GetNodeType() == TNK_VOID_TYPE)
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
			,_T("cannot convert from '%s' to '%s'")
			,CString(pRightDataTypeNode->GetSymbol())
			,CString(pLeftDataTypeNode->GetSymbol()));
		return;
	}

	if(!bInitCheck && pLeftDataTypeNode->GetNodeType() == TNK_REFERENCE_TYPE)
	{
		pLeftDataTypeNode = pLeftDataTypeNode->GetDataType();
	}


	CDataTypeTreeNode* pBaseDataTypeTreeNode = NULL;
	switch(pLeftDataTypeNode->GetNodeType())
	{
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:          
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE: 
	case TNK_LONGLONG_TYPE:  
	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		switch(pRightDataTypeNode->GetNodeType())
		{
		case TNK_POINTER_TYPE:
		case TNK_REFERENCE_TYPE:
			pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
				,_T("cannot convert from '%s *' to '%s'")
				,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
				,CString(pLeftDataTypeNode->GetSymbol()));
			break;

		case TNK_ARRAY_TYPE:
			pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
				,_T("cannot convert from '%s []' to '%s'")
				,(NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("")
				,CString(pLeftDataTypeNode->GetSymbol()));
			break;

		case TNK_CHAR_TYPE:
		case TNK_BYTE_TYPE:
		case TNK_BOOLEAN_TYPE:          
		case TNK_SHORT_TYPE:
		case TNK_WORD_TYPE:
		case TNK_INT_TYPE:
		case TNK_LONG_TYPE:
		case TNK_DWORD_TYPE: 
		case TNK_LONGLONG_TYPE:
		case TNK_FLOAT_TYPE:       
		case TNK_DOUBLE_TYPE:
			if(!pRightNode->IsNumericConstants())
			{
				if(pLeftDataTypeNode->GetSize() < pRightDataTypeNode->GetSize()
					|| pLeftDataTypeNode != pRightDataTypeNode)
				{
					m_pSaxCCompileimpl->OutputErrMsg(FALSE, pLeftNode->GetLineIndex(),0
						,_T("conversion from '%s' to '%s', possible loss of data")
						,CString(pRightDataTypeNode->GetSymbol()),CString(pLeftDataTypeNode->GetSymbol()));
				}   
			}
			break;

		default:
			if(NULL == pRightDataTypeNode)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("illegal indirection"));
			}
		}     
		break;

	case TNK_POINTER_TYPE:   
		switch(pRightDataTypeNode->GetNodeType())
		{
		case TNK_ARRAY_TYPE:
			if(pLeftDataTypeNode->GetPointerDepth() != 1
				|| pRightDataTypeNode->GetDataType() != pLeftDataTypeNode->GetDataType())
			{
				pBaseDataTypeTreeNode = pRightDataTypeNode->GetDataType();
				CString strDataType1 = (NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("");

				pBaseDataTypeTreeNode = pLeftDataTypeNode->GetDataType();
				CString strDataType2 = (NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("");

				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s*' to '%s%s'")
					,strDataType1
					,strDataType2
					,GetPointerDepthDesc(pLeftDataTypeNode->GetPointerDepth()));
			}
			break;

		case TNK_POINTER_TYPE:
			TypeCompatibleCheckingofPointer(pLeftNode,pRightNode);
			break;

		default:
			{
				if(!(pRightNode->GetNodeType() == TNK_INTEGER_CST
					&& pRightNode->GetValue1() == 0)) //排出"null"常量
				{
					pBaseDataTypeTreeNode = pLeftDataTypeNode->GetDataType();
					CString strDataType2 = (NULL != pBaseDataTypeTreeNode)? CString(pBaseDataTypeTreeNode->GetSymbol()) : _T("");

					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
						,_T("cannot convert from '%s' to '%s%s'")
						,CString(pRightDataTypeNode->GetSymbol()),strDataType2
						,GetPointerDepthDesc(pLeftDataTypeNode->GetPointerDepth()));
				}
			}
		}
		break;

	case TNK_REFERENCE_TYPE:
		switch(pRightNode->GetNodeType())
		{
		case TNK_VAR_REF:
		//case TNK_PARM_DECL:
			if(pLeftDataTypeNode->GetDataType() != pRightDataTypeNode)
			{
				pBaseDataTypeTreeNode= pLeftDataTypeNode->GetDataType();
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s' to '%s &'")
					,CString(pRightDataTypeNode->GetSymbol())
					,CString(pBaseDataTypeTreeNode->GetSymbol()));
			}
			break;

		case TNK_CONST_DECL:
			if(((CDeclarationTreeNode*)pRightNode)->GetDeclAttribute() & NDA_CONST_FLAG)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("error in line %d:cannot convert from 'const %s' to '%s &'")
					,CString(pRightDataTypeNode->GetSymbol())
					,CString(pRightDataTypeNode->GetSymbol()));
			}
			break;
		case TNK_INDIRECT_REF:
			if(pLeftDataTypeNode->GetDataType() != pRightDataTypeNode)
			{
				pBaseDataTypeTreeNode= pLeftDataTypeNode->GetDataType();
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0
					,_T("cannot convert from '%s' to '%s &'")
					,CString(pRightDataTypeNode->GetSymbol())
					,CString(pLeftDataTypeNode->GetSymbol()));
			}
			break;

		default:
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0,
				_T("Initial value of reference to non-const must be an lvalue"));
		}     
		break;

	default:
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftNode->GetLineIndex(),0,
				_T("Syntax Error"));
	}   
}
/*---------------------------------------------------------------------------------
| Name   : CParser::AnalyseArrayInitialization
| Desc   : 分析数组初始化
| Return  : 初始化链表
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CVectorTreeNode*    CParser::AnalyseArrayInitialization(CDeclarationTreeNode* pArrayNode
	,CParserContext* pContext)
{
	ATLASSERT(NULL != pArrayNode);

	CDataTypeTreeNode* pLeftDataTypeNode   = pArrayNode->GetDataType();
	CDataTypeTreeNode* pElementDataTypeNode = pLeftDataTypeNode->GetDataType();

	UINT nElement = 0;
	CSyntaxTreeNode *pFirstNode = NULL;
	CSyntaxTreeNode *pLastNode = NULL;
	CSyntaxTreeNode *pNewNode  = NULL;
	if(m_pCurrentToken->GetType() == TT_STRING_TYPE)
	{
		if(pElementDataTypeNode->GetNodeType() == TNK_CHAR_TYPE)
		{
			CStringA strValue(m_pCurrentToken->GetSymbolTitle());
			const UINT LEN = strValue.GetLength();
			for(UINT i = 0;i <= LEN;i++)
			{
				pNewNode = CreateConstIntTreeNode((i < LEN)? strValue.GetAt(i): 0);

				if(NULL == pFirstNode)
				{
					pFirstNode = pLastNode = pNewNode;
				}
				else
				{
					pLastNode->SetChain(pNewNode);
					pLastNode = pNewNode;
				}   
				TypeCheckingforArrayInit(pArrayNode,pNewNode);
			}

			nElement = LEN + 1;
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("cannot convert from 'const char[]' to '%s []'"),
				CString(pElementDataTypeNode->GetSymbol()));

		}
	}
	else if(m_pCurrentToken->GetType() == TT_LBRACE)
	{
		do
		{
			m_pCurrentToken = m_pScaner->GetNextToken();
			if(m_pCurrentToken->GetType() != TT_RBRACE)
			{
				pNewNode = AnalyseThirteenthLevelOp(pContext);
				if(NULL == pFirstNode)
				{
					pFirstNode = pLastNode = pNewNode;
				}
				else
				{
					pLastNode->SetChain(pNewNode);
					pLastNode = pNewNode;
				}   
				nElement++;
				TypeCheckingforArrayInit(pArrayNode,pNewNode);

				m_pCurrentToken = m_pScaner->GetNextToken();
				if(m_pCurrentToken->GetType() != TT_RBRACE
					&& m_pCurrentToken->GetType() != TT_COMMA)// `,`
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
						,_T("missing ','"));
				}
			}

		}while(m_pCurrentToken->GetType() != TT_RBRACE);
	}
	else
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("initialization whit '{...}' expected for aggregate object"));
	}

	CVectorTreeNode* pResult = new CVectorTreeNode(nElement,pFirstNode);
	if(NULL != pResult)
	{
		pArrayNode->SetInitial(pResult);
		if(pLeftDataTypeNode->GetMinval() == 0)
		{
			pLeftDataTypeNode->SetSize(nElement * pElementDataTypeNode->GetSize());
			pLeftDataTypeNode->SetMinval(nElement);
		}
		else if(pLeftDataTypeNode->GetMinval() < nElement)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("too many initializers"));
		}

		m_arrEntries.push_back(pResult);
	}
	else 
	{
		throw CParserException("Fail to create the CVectorTreeNode object!");
	}

	return pResult;
}
/*---------------------------------------------------------------------------------
| Name   : CParser::AnalyseVariablesDeclaration
| Desc   : Analyse global variable declaration. Grammar 3:
| 4. VariableDeclaration->type_specifier ID(, ...)`;` | type_specifier ID `[` NUM `]`(, ...)`;`
| 5. type_specifier->`int` | `void` | `char`, actually this step is in ListDeclaration()
| Parameter : CDataTypeTreeNode* pDataTypeTreeNode——>基本类型
|       UINT nPrefixAttr——>变量的修饰属性(auto，register，extern，static,const)
|       CParserContext* pContext——>如果为"NULL"表示全局变量，否则是所属代码快的
|       局部变量
| Return  : CSyntaxTreeNode* If the function is successful,return a pointer to 
|       the syntax tree node to be created,Otherwise null.
| Author  : Andy.h
| Date   : 星期六, 四月 12 2014
---------------------------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseVariablesDeclaration(CDataTypeTreeNode* pBaseDataTypeTreeNode
	,UINT nPrefixAttr,CParserContext* pContext)
{
	ATLASSERT(NULL != pBaseDataTypeTreeNode);

	CSyntaxTreeNode *pFirstNode = NULL;
	CSyntaxTreeNode *pLastNode = NULL;

	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	do
	{
		pDataTypeTreeNode = AnalysePointerDataType(pBaseDataTypeTreeNode);

		if(pDataTypeTreeNode == pBaseDataTypeTreeNode)  //说明不是指针类型分析是否参考类型
		{
			pDataTypeTreeNode = AnalyseRefrenceDataType(pBaseDataTypeTreeNode);     
		}

		CSymbolInfo   objIDToken = *(m_pScaner->GetNextToken());

		if(objIDToken.GetType() != TT_IDENTIFY_TYPE ) 
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0
				,_T("\"%s\" is a reserved token.")
				,CString(objIDToken.GetSymbolTitle()));

			ConsumeUntil( TT_SEMI, TT_RBRACE );
			break;
		} 

		CDeclarationTreeNode* pNewNode = CreateDeclNode((nPrefixAttr & NDA_CONST_FLAG)? TNK_CONST_DECL:TNK_VAR_DECL
			,&objIDToken);
		ATLASSERT(NULL != pNewNode);
		if(NULL == pNewNode)
		{
			break;
		}

		pNewNode->SetDeclAttribute(nPrefixAttr);
		if(NULL != pContext)
		{
			pNewNode->SetContext(pContext->GetFunctionDeclNode());
		}
		else
		{
			pNewNode->SetContext(NULL);
		}

		m_pCurrentToken = m_pScaner->GetNextToken();
		switch(m_pCurrentToken->GetType())
		{
		case TT_LSQUARE:    // '[' 说明是数组变量,目前只支持一维数组
			{
				m_pCurrentToken = m_pScaner->GetNextToken(); 

				UINT nElements = 0;
				if(m_pCurrentToken->GetType() != TT_RSQUARE)
				{
					CSyntaxTreeNode* pElements = AnalyseFourteenthLevelOp(pContext);  //数组的下标支持常量表达式
					if(pElements->GetNodeType() != TNK_INTEGER_CST)
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0
							,_T(" '%s' constant expression is not integral.")
							,CString(objIDToken.GetSymbolTitle()));
					}
					else
					{
						nElements = (UINT)pElements->GetValue1();
					}                  
				}
				else
				{  //当前数组下标为空
					m_pScaner->PushBack();          
				}

				CDataTypeTreeNode* pArryDataTypeTreeNode = GetArrayDataType(pDataTypeTreeNode,nElements);
				ATLASSERT(pArryDataTypeTreeNode);
				if(NULL != pArryDataTypeTreeNode)
				{
					pNewNode->SetDataType(pArryDataTypeTreeNode);
				}        

				if( !Match(TT_RSQUARE) ) // `]`
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ']'.")
						,CString(m_pCurrentToken->GetSymbolTitle()));
					ConsumeUntil( TT_SEMI, TT_RBRACE );           // error recovery
				}

				m_pCurrentToken = m_pScaner->GetNextToken();   // should be ';' , ',' or '='

				if(m_pCurrentToken->GetType() == TT_ASSIGN)
				{
					m_pCurrentToken = m_pScaner->GetNextToken();   
					AnalyseArrayInitialization(pNewNode,pContext);

					m_pCurrentToken = m_pScaner->GetNextToken();   // should be ';' , ','
				}
			}
			break;

		case TT_COMMA:
		case TT_SEMI:
			pNewNode->SetDataType(pDataTypeTreeNode);
			break;

		case TT_ASSIGN: //说明要给变量赋初值
			{
				pNewNode->SetDataType(pDataTypeTreeNode);          
				m_pCurrentToken = m_pScaner->GetNextToken(); 

				CSyntaxTreeNode* pInitial = AnalyseFourteenthLevelOp(pContext); 

				pNewNode->SetInitial(pInitial);   
				if(NULL == pInitial)
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0,_T("expected an expression"));
				}
				else
				{
					TypeCompatibleChecking(pNewNode,pInitial,TRUE);

					AppendRef(pInitial,pNewNode);
					pInitial->SetParent(pNewNode);
				}

				m_pCurrentToken = m_pScaner->GetNextToken();   // should be ';' , ','
			}
			break;     
		default:
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0,_T("syntax error"));
		}

		if(NULL == pNewNode->GetInitial())
		{
			pDataTypeTreeNode = pNewNode->GetDataType();
			if(pDataTypeTreeNode->GetNodeType() == TNK_REFERENCE_TYPE)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
					,_T("'%s' : references must be initialized")
					,CString(pNewNode->GetSymbol()));
			}
			else if(pDataTypeTreeNode->GetNodeType() == TNK_ARRAY_TYPE
				&& pDataTypeTreeNode->GetMinval() == 0)        
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0,_T(" '%s' : unknown size.")
					,CString(pNewNode->GetSymbol()));
			}
		}



		if((nPrefixAttr & NDA_CONST_FLAG))
		{
			CSyntaxTreeNode* pInitial = pNewNode->GetInitial();
			if(NULL == pInitial)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
					,_T("const object '%s' must be initialized if not extern")
					,CString(pNewNode->GetSymbol()));
			}
			else
			{
				if(pInitial->GetNodeType() < TNK_INTEGER_CST 
					|| pInitial->GetNodeType() > TNK_STRING_CST)   //说明不是常量
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
						,_T("const object '%s' must be initialized if not extern")
						,CString(pNewNode->GetSymbol()));
				}
			}
		}


		if(NULL == pFirstNode)
		{
			pFirstNode = pNewNode;
			pLastNode = pNewNode;
		}
		else
		{
			pLastNode->SetChain(pNewNode);
			pLastNode = pNewNode;
		}

		if(m_pCurrentToken->GetType() != TT_COMMA
			&& m_pCurrentToken->GetType() != TT_SEMI)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("bad declaration sequence, missing ';'"));
			ConsumeUntil( TT_SEMI, TT_RBRACE );
			break;
		}   

	}while(m_pCurrentToken->GetType() == TT_COMMA);

	CExpressionTreeNode* pExpressionTreeNode = NULL;

	if(NULL != pFirstNode)
	{
		pExpressionTreeNode = CreateExpressionNode(TNK_DECL_STMT);
		pExpressionTreeNode->SetChildNode(0,pFirstNode);
	}

	return pExpressionTreeNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::FunctionDeclaration
| 描  述 : Analyse function declaration. 
| 6. FunctionDeclaration->type_specifier ID `(` Parameters `)` AnalyseCompoundStatements
| 返 回 值 : If the function is successful,return a pointer to 
|       the syntax tree node to be created,Otherwise null.
| 修改记录 : 2007-4-23 14:58:19  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFunctionsDefine(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPrefixAttr)
{
	CDataTypeTreeNode* pDataTypeTreeNode = AnalysePointerDataType(pBaseDataTypeTreeNode);
	ATLASSERT(NULL != pDataTypeTreeNode);
	if(NULL == pDataTypeTreeNode)
	{
		return NULL;
	}

	m_pCurrentToken = m_pScaner->GetNextToken();
	CDeclarationTreeNode* pFunctionNode = CreateDeclNode( TNK_FUNCTION_DECL, m_pCurrentToken);
	if(NULL == pFunctionNode)
	{
		return NULL;
	}
	pFunctionNode->SetDataType(pDataTypeTreeNode);
	pFunctionNode->SetDeclAttribute(nPrefixAttr);

	//StringCchPrintfA(m_szScope,SYMBOL_MAX_LENGTH,m_objIDToken.GetSymbolTitle());   // update function scope

	// Parameters
	CParserContext objContext(pFunctionNode,m_pSaxCCompileimpl);
	CreateNewBindingLevel(&objContext);

	m_pCurrentToken = m_pScaner->GetNextToken();
	CSyntaxTreeNode* pTmpNode = AnalyseParameters(pFunctionNode);
	pFunctionNode->SetArguments(pTmpNode);

	m_pCurrentToken = m_pScaner->GetNextToken(); 

	
	switch(m_pCurrentToken->GetType())
	{
	case TT_LBRACE:
		pTmpNode = AnalyseCompoundStatements(pFunctionNode,&objContext);          // compound statements
		pFunctionNode->SetSavedTree(pTmpNode);
		break;

	case TT_SEMI:
		break;

	default:
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("unpaired '{'"));
		m_pScaner->PushBack();

		pTmpNode = AnalyseCompoundStatements(pFunctionNode,&objContext);          // compound statements
		pFunctionNode->SetSavedTree(pTmpNode);
	}   

	PopBindingLevel();


	return pFunctionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::Parameters
| 描  述 : Analyse parameters declaration of function. 
| Grammar:
| 7. Parameters->param_list | `void` | empty, `void` is thought as empty
| 8. param_list->param_list `,` param | param
| 9. param->type_specifier ID | type_specifier ID `[` `]`
| 返 回 值 : If the function is successful,return a pointer to 
|       the parameters list to be created,Otherwise null.
| 修改记录 : 2007-4-23 15:02:05  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseParameters(CDeclarationTreeNode* pFunctionNode)
{
	ATLASSERT(pFunctionNode);
	if(NULL == pFunctionNode)
	{
		return NULL;
	}

	CDeclarationTreeNode *pLastNode = NULL;
	CDeclarationTreeNode *pNewNode = NULL;

	if( m_pCurrentToken->GetType() != TT_LPARAN ) // '('
	{
		return NULL;
	}
	m_pCurrentToken = m_pScaner->GetNextToken();
	if( m_pCurrentToken->GetType() == TT_RPARAN ) // ')'  
	{//说明没有参数
		return NULL;
	}
	m_pScaner->PushBack();


	/*在32位模式下第一参数的偏移是8,最初的8个字节保存了返回地址和EBP的值,每增加一个参数偏移增加4或8个字节
	,不管什么类型都占用四个字节或8个字节,通过[EBP+8]这种方式取得第一个参数的值*/

	UINT nOffset = 8;    
	UINT nPrefixAttr = 0;
	CDataTypeTreeNode* pBaseDataTypeTreeNode = NULL;
	CDataTypeTreeNode* pDataTypeTreeNode   = NULL;
	do
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
		nPrefixAttr = AnalysePrefixAtti();       

		m_pCurrentToken = m_pScaner->GetNextToken();
		pBaseDataTypeTreeNode = AnalyseDataType(nPrefixAttr); 
		if(NULL == pBaseDataTypeTreeNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("invalid type '%s'")
				,CString(m_pCurrentToken->GetSymbolTitle()));
			ConsumeUntil( TT_COMMA );
			continue;
		}

		pDataTypeTreeNode = AnalysePointerDataType(pBaseDataTypeTreeNode);
		if(pDataTypeTreeNode == pBaseDataTypeTreeNode)  //说明不是指针类型分析是否参考类型
		{
			pDataTypeTreeNode = AnalyseRefrenceDataType(pBaseDataTypeTreeNode);
		}

		m_pCurrentToken = m_pScaner->GetNextToken(); 
		switch(m_pCurrentToken->GetType())
		{
		case TT_IDENTIFY_TYPE:
			pNewNode = CreateDeclNode( TNK_PARM_DECL, m_pCurrentToken);
			if(pNewNode)
			{
				pNewNode->SetOffset(nOffset);
				switch(pDataTypeTreeNode->GetNodeType())
				{
				case TNK_DOUBLE_TYPE:
				case TNK_LONGLONG_TYPE:
					nOffset += 8;  //"double"和"LONGLONG"类型是8个字节
					break;

				case TNK_POINTER_TYPE:
				case TNK_ARRAY_TYPE:
					nOffset += (GetMode() == ACM_MODE_X86)? 4:8; 
					break;

				default:     
					nOffset += 4; 
				}

				m_pCurrentToken = m_pScaner->GetNextToken(); 
				if(m_pCurrentToken->GetType() == TT_LSQUARE) //说明是数组参数
				{
					m_pCurrentToken = m_pScaner->GetNextToken(); 
					if(m_pCurrentToken->GetType() == TT_RSQUARE)
					{
						CDataTypeTreeNode* pArryDataTypeTreeNode = GetPointerDataType(pDataTypeTreeNode,0);
						ATLASSERT(pArryDataTypeTreeNode);
						if(NULL != pArryDataTypeTreeNode)
						{
							pNewNode->SetDataType(pArryDataTypeTreeNode);
						}   

						m_pCurrentToken = m_pScaner->GetNextToken(); 
					}
					else
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0,_T("bad array parameter, missing ']'"));
					}
				}
				else
				{
					pNewNode->SetDataType(pDataTypeTreeNode);
				}

				pNewNode->SetContext(pFunctionNode);
				if(NULL != pNewNode)          
				{             
					pNewNode->SetChain(pLastNode);
					pLastNode = pNewNode;
				}
			}
			break;

		case TT_RPARAN:
			break;

		default:
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("Unknown type name \"%s\".")
				, CString(m_pCurrentToken->GetSymbolTitle())); 
		}

		if(TT_RPARAN != m_pCurrentToken->GetType()
			&& TT_COMMA != m_pCurrentToken->GetType()) 
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("invalid parameter \"%s\".")
				, CString(m_pCurrentToken->GetSymbolTitle()));
		} 

	}while(m_pCurrentToken->GetType() == TT_COMMA);

	if(m_pCurrentToken->GetType() != TT_RPARAN) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("missing ')' in function \"%s\"(...) Declaration.")
			, CString(m_pCurrentToken->GetSymbolTitle()));

		m_pScaner->PushBack();
	}   
	CExpressionTreeNode * pExpressionTreeNode = NULL;
	if(NULL != pLastNode)
	{
		pExpressionTreeNode = CreateExpressionNode(TNK_PARA_STMT);
		pExpressionTreeNode->SetChildNode(0,pLastNode);
	}

	return pExpressionTreeNode;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::CheckDuplicateCase
| 描  述 : 检查重复case分支
| 返 回 值 : true:有重复,false:无
| 修改记录 : 2007-4-23 15:20:41  -huangdy-  创建
-----------------------------------------------------------------*/
BOOL CParser::CheckDuplicateCase(CSyntaxTreeNode* pSTMTNodes,CSyntaxTreeNode* pCaseNode)
{
	if(NULL == pSTMTNodes)
	{
		return FALSE;
	}
	ATLASSERT(pCaseNode);
	if(pCaseNode->GetNodeType() != TNK_CASE_LABEL)
	{
		return FALSE;
	}
	CSyntaxTreeNode* pValueNode = ((CExpressionTreeNode*)pCaseNode)->GetChildNode(0);
	const LONGLONG nValue = pValueNode->GetValue1();

	BOOL bResult = FALSE;
	while(pSTMTNodes)
	{
		if(pSTMTNodes->GetNodeType() == TNK_CASE_LABEL)
		{
			pValueNode = ((CExpressionTreeNode*)pSTMTNodes)->GetChildNode(0);
			if(nValue == pValueNode->GetValue1())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pCaseNode->GetLineIndex(),0
					,_T("case value '%d' already used"),nValue);
				bResult = TRUE;
				break;
			}
		}
		pSTMTNodes = pSTMTNodes->GetChain();
	}
	return bResult;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::LookParentSTMT
| 描  述 : 查找指定类型的父语句节点
| 返 回 值 : true:找到,false:每找到
| 修改记录 : 2007-4-23 15:20:41  -huangdy-  创建
-----------------------------------------------------------------*/
BOOL CParser::LookParentSTMT(CSyntaxTreeNode* pParent
	,TREE_NODE_TYPE nNodeType1,TREE_NODE_TYPE nNodeType2,TREE_NODE_TYPE nNodeType3)
{
	BOOL bResult = FALSE;

	while(pParent)
	{
		if(pParent->GetNodeType() == nNodeType1
			|| pParent->GetNodeType() == nNodeType2
			|| pParent->GetNodeType() == nNodeType3)
		{
			bResult = TRUE;
			break;
		}
		pParent = pParent->GetParent();
	}

	return bResult;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseCompoundStatements
| 描  述 : Analyse compound statements in function body.
| Grammar:
| 10. AnalyseCompoundStatements->`{` loal_declarations statement_list `}` | AnalyseExpressionStatement
| 返 回 值 : If the function is successful,return a pointer to 
|       the tree to be created,Otherwise null.
| 修改记录 : 2007-4-23 15:20:41  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseCompoundStatements(CSyntaxTreeNode* pParent
	,CParserContext* pContext,BOOL nSingleStatement)
{
	CSyntaxTreeNode *pFirstNode = NULL;
	CSyntaxTreeNode   *pLastNode = NULL;
	CSyntaxTreeNode   *pNewNode  = NULL;

	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	UINT nPrefixAttr = 0;   

	do                            
	{   
		m_pCurrentToken = m_pScaner->GetNextToken();
		if(m_pCurrentToken->GetType() == TT_LBRACE)
		{
			CreateNewBindingLevel(pContext);

			pNewNode = AnalyseCompoundStatements(pParent,pContext);

			PopBindingLevel();
		}
		else
		{
			nPrefixAttr = AnalysePrefixAtti();  //First,analyse local variables declaration 

			m_pCurrentToken = m_pScaner->GetNextToken();
			pDataTypeTreeNode = AnalyseDataType(nPrefixAttr); 
			if(NULL != pDataTypeTreeNode)               //说明是变量定义
			{
				pNewNode = AnalyseVariablesDeclaration(pDataTypeTreeNode,nPrefixAttr,pContext);
			}
			else
			{
				switch( m_pCurrentToken->GetType() ) 
				{
				case TT_SEMI:                 // ';'空语句
				case TT_INTEGER_TYPE:             // eg. 1234;
				case TT_CHAR_TYPE:               // eg. 'a';
					break; //这三中语句没任何作用，忽略

					//case TT_LOGICAL_NOT:
					//case TT_LPARAN:
					//   pNewNode = AnalyseExpressionStatement(pContext);   
					//   break;

				case TT_PLUS_PLUS:
				case TT_MINUS_MINUS:
				case TT_MULT:                    //指针引用,可定时一个表达式
				case TT_IDENTIFY_TYPE:                
					pNewNode = AnalyseExpressionStatement(pContext);
					break;          

				case TT_IF:
					pNewNode = AnalyseIfStatement(pParent,pContext);        
					break;

				case TT_WHILE:
					pNewNode = AnalyseWhileStatement(pParent,pContext);     
					break;

				case TT_DO:
					pNewNode = AnalyseDoStatement(pParent,pContext);   
					break;

				case TT_FOR:
					pNewNode = AnalyseForStatement(pParent,pContext);        
					break;

				case TT_GOTO:
					pNewNode = AnalyseGotoStatement(pParent,pContext);        
					break;

				case TT_BREAK:
					pNewNode = AnalyseBreakStatement(pParent,pContext);   
					if(!LookParentSTMT(pParent,TNK_FOR_STMT,TNK_WHILE_STMT,TNK_DO_STMT)
						&& !LookParentSTMT(pParent,TNK_SWITCH_STMT))
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
							,_T("illegal break"));
					}
					break;

				case TT_CONTINUE:
					pNewNode = AnalyseContinueStatement(pParent,pContext);
					if(!LookParentSTMT(pParent,TNK_FOR_STMT,TNK_WHILE_STMT,TNK_DO_STMT))
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
							,_T("illegal continue"));
					}
					break; 

				case TT_RETURN:
					pNewNode = AnalyseReturnStatement(pParent,pContext);     
					break;

				case TT_SWITCH:
					pNewNode = AnalyseSwitchStatement(pParent,pContext);
					break;

				case TT_CASE:
				case TT_DEFAULT:
					pNewNode = AnalyseCaseStatement(pParent,pContext);
					CheckDuplicateCase(pFirstNode,pNewNode);
					if(!LookParentSTMT(pParent,TNK_SWITCH_STMT))
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, pNewNode->GetLineIndex(),0
							,_T("illegal %s"),(pNewNode->GetTokenType() == TT_CASE)? _T("case"):_T("default"));
					}
					break; 

				case TT_ELSE:
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
						,_T("unpaired 'else' statement."));

					ConsumeUntil( TT_SEMI, TT_RBRACE );
					break;

				default:
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("undefined symbol \"%s\".")
						, CString(m_pCurrentToken->GetSymbolTitle()));
					ConsumeUntil( TT_SEMI, TT_RBRACE );
				}
			}
		}

		if( pNewNode )          
		{   
			pNewNode->SetParent(pParent);

			if( !pFirstNode ) 
			{ 
				pFirstNode = pNewNode; 
				pLastNode = pNewNode; 
			}
			else
			{ 
				pLastNode->SetChain(pNewNode); 
				pLastNode = pNewNode;
			}
		}

		if(nSingleStatement) //仅仅分析一条语句,主要是针对"if","while"等语句
		{
			break;
		}

		m_pCurrentToken = m_pScaner->GetNextToken();
		if( m_pCurrentToken->GetType() == TT_RBRACE )  // '}'  
		{
			break;     
		}
		else 
		{
			m_pScaner->PushBack();
		}

	}while(m_pCurrentToken->GetType() != TT_EOF_TYPE && m_pCurrentToken->GetType() != TT_ERROR_TYPE);

	if(!nSingleStatement)
	{
		if( m_pCurrentToken->GetType() != TT_RBRACE ) // '}'
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("unpaired '}'"));     
		}
	}


	CExpressionTreeNode* pExpressionTreeNode = NULL;

	if(NULL != pFirstNode)
	{
		pExpressionTreeNode = CreateExpressionNode(TNK_COMPOUND_STMT);
		pExpressionTreeNode->SetChildNode(0,pFirstNode);
	}

	return pExpressionTreeNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseExpressionStatement
| 描  述 : 会返回TNK_LABEL_STMT和TNK_EXPR_STMT两种语句
| Grammar:
| 15. AnalyseExpressionStatement->AnalyseFourteenthLevelOp `;` | `:`
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:16:24  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseExpressionStatement(CParserContext* pContext)
{   
	CSyntaxTreeNode* pNewNode = AnalyseFifteenthLevelOp(pContext);

	if(pNewNode)
	{
		if(pNewNode->GetNodeType() != TNK_LABEL_STMT) 
		{//说明是一be表达式语句
			CExpressionTreeNode* pExpression = CreateStatementNode( TNK_EXPR_STMT ,NULL);
			ATLASSERT(pExpression);
			if(NULL != pExpression)
			{
				pExpression->SetChildNode(0,pNewNode);
				pNewNode = pExpression;
			}

			if( !Match(TT_SEMI) ) 
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ';'"));

				m_pScaner->PushBack();       // error recovery
			}
		}
	}
	else
	{
		ConsumeUntil( TT_SEMI);
	}   

	return pNewNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseTwelfthLevelOp
| 描  述 : 分析逗号运算符
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:28:32  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFifteenthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pCommaExprNode    = NULL;
	CSyntaxTreeNode *pLeftExpressionNode = pCommaExprNode= AnalyseFourteenthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken = m_pScaner->GetNextToken();
	if(m_pCurrentToken->GetType() == TT_COMMA)
	{
		pCommaExprNode = CreateExpressionNode(m_pCurrentToken);
		((CExpressionTreeNode*)pCommaExprNode)->SetChildNode(0,pLeftExpressionNode);

		UINT nIndex = 1;
		while( m_pCurrentToken->GetType() == TT_COMMA ) 
		{
			m_pCurrentToken   = m_pScaner->GetNextToken();
			pLeftExpressionNode = AnalyseEleventhLevelOp(pContext);
			AppendRef(pLeftExpressionNode,pCommaExprNode);

			if(NULL != pCommaExprNode)
			{
				((CExpressionTreeNode*)pCommaExprNode)->SetChildNode(nIndex++,pLeftExpressionNode);
				((CExpressionTreeNode*)pCommaExprNode)->SetDataType(pLeftExpressionNode->GetDataType());
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
				break;
			}

			m_pCurrentToken = m_pScaner->GetNextToken();
			if(nIndex >= MAX_CHILDREN_NUMBER)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
				break;
			}
		}
	}
	m_pScaner->PushBack();// put the next token back

	return pCommaExprNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseFourteenthLevelOp
| 描  述 : Analyse an expression.
| Grammar:
| 16. AnalyseFourteenthLevelOp->AnalyseFactor `=` AnalyseFourteenthLevelOp | AnalyseTwelfthLevelOp
| FIRST( AnalyseFourteenthLevelOp ) = { `!`, `(`, ID, NUM, CHARACTER }
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:17:11  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFourteenthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode *pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode *pLeftExpressionNode = AnalyseThirteenthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);
	const AXC_SYMBOL_TYPE SYMBOL_TYPE = objOperator.GetType();

	if( TT_ASSIGN == SYMBOL_TYPE
		|| TT_PLUS_ASSIGN == SYMBOL_TYPE
		|| TT_MINUS_ASSIGN == SYMBOL_TYPE
		|| TT_MULT_ASSIGN == SYMBOL_TYPE
		|| TT_DIV_ASSIGN  == SYMBOL_TYPE
		|| TT_AND_ASSIGN  == SYMBOL_TYPE
		|| TT_XOR_ASSIGN  == SYMBOL_TYPE
		|| TT_OR_ASSIGN  == SYMBOL_TYPE
		|| TT_MOD_ASSIGN  == SYMBOL_TYPE
		|| TT_LSHIFT_ASSIGN == SYMBOL_TYPE
		|| TT_RSHIFT_ASSIGN == SYMBOL_TYPE) 
	{
		if(!pLeftExpressionNode->Ismodifiable())
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
				,0,_T(" '%s' : left operand must be l-value")
				,CString(m_pCurrentToken->GetSymbolTitle()));
		}

		m_pCurrentToken = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseThirteenthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}

		if(TT_ASSIGN != SYMBOL_TYPE)
		{
			TREE_NODE_TYPE nNodeType  = TNK_UNKNOWN;
			AXC_SYMBOL_TYPE nSymbolType = TT_UNKNOWN;
			switch(SYMBOL_TYPE)
			{
			case TT_PLUS_ASSIGN:
				nNodeType  = TNK_PLUS_EXPR;
				nSymbolType = TT_PLUS;
				TypeCheckingFourthLevelOp(nSymbolType,pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_MINUS_ASSIGN:
				nNodeType  = TNK_MINUS_EXPR;
				nSymbolType = TT_MINUS;
				TypeCheckingFourthLevelOp(nSymbolType,pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_MULT_ASSIGN:
				nNodeType  = TNK_MULT_EXPR;
				nSymbolType = TT_MULT;
				TypeCheckingThirdLevelOp(nSymbolType,pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_DIV_ASSIGN:
				nNodeType  = TNK_TRUNC_DIV_EXP;
				nSymbolType = TT_DIV;
				TypeCheckingThirdLevelOp(nSymbolType,pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_MOD_ASSIGN:
				nNodeType  = TNK_TRUNC_MOD_EXPR;
				nSymbolType = TT_MOD;
				TypeCheckingThirdLevelOp(nSymbolType,pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_AND_ASSIGN:
				nNodeType  = TNK_BIT_AND_EXPR;
				nSymbolType = TT_BITWISE_AND;
				TypeCheckingBitOp("&",pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_XOR_ASSIGN:
				nNodeType  = TNK_BIT_XOR_EXPR;
				nSymbolType = TT_BITWISE_XOR;
				TypeCheckingBitOp("^",pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_OR_ASSIGN:
				nNodeType  = TNK_BIT_IOR_EXPR;
				nSymbolType = TT_BITWISE_OR;
				TypeCheckingBitOp("|",pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_LSHIFT_ASSIGN:
				nNodeType  = TNK_LSHIFT_EXPR;
				nSymbolType = TT_LSHIFT;
				TypeCheckingFifthLevelOp("<<",pLeftExpressionNode,pRightExpressionNode);
				break;

			case TT_RSHIFT_ASSIGN:
				nNodeType  = TNK_RSHIFT_EXPR;
				nSymbolType = TT_RSHIFT;
				TypeCheckingFifthLevelOp(">>",pLeftExpressionNode,pRightExpressionNode);
				break;

			default:
				ATLASSERT(FALSE);
			}

			pTmpNode = CreateExpressionNode(nNodeType,objOperator.GetLineIndex(),nSymbolType
				,pLeftExpressionNode,pRightExpressionNode);
			pRightExpressionNode = pTmpNode;
		}

		TypeCompatibleChecking(pLeftExpressionNode,pRightExpressionNode);

		if(pRightExpressionNode->IsNumericConstants())
		{
			CDataTypeTreeNode* pDataTypeNode = pLeftExpressionNode->GetDataType();
			if(pDataTypeNode->GetNodeType() == TNK_BOOLEAN_TYPE)
			{
				if(pRightExpressionNode->GetNodeType() == TNK_REAL_CST)
				{
					pRightExpressionNode = ConvertConstRealToInt((CConstTreeNode*)pRightExpressionNode);
				}
				if(pRightExpressionNode->GetValue1() != 0)
				{
					((CConstIntTreeNode*)pRightExpressionNode)->SetValue(1);
				}
			}
		}
		pTmpNode = CreateExpressionNode(TNK_MODIFY_EXPR,objOperator.GetLineIndex(),TT_ASSIGN
			,pLeftExpressionNode,pRightExpressionNode);

		if(NULL != pTmpNode)
		{
			pLeftExpressionNode = pTmpNode;
		}
		else
		{
			Destroy(pLeftExpressionNode);
			Destroy(pRightExpressionNode); 

			pLeftExpressionNode = NULL;
		}   
	} 
	else
	{
		m_pScaner->PushBack();
	}

	return pLeftExpressionNode;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseThirteenthLevelOp
| 描  述 : 条件运算符号?得分析函数
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:28:32  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseThirteenthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode *pFirstExpressionNode = AnalyseTwelfthLevelOp(pContext);
	if(NULL == pFirstExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken = m_pScaner->GetNextToken();
	if( m_pCurrentToken->GetType() == TT_QUESTION_MARK ) 
	{
		CSymbolInfo objOperator = *(m_pCurrentToken);

		m_pCurrentToken = m_pScaner->GetNextToken();
		CSyntaxTreeNode *pSecondExpressionNode = AnalyseTwelfthLevelOp(pContext);

		if(NULL != pSecondExpressionNode)
		{
			if( Match(TT_COLON) ) 
			{   
				m_pCurrentToken = m_pScaner->GetNextToken();
				CSyntaxTreeNode *pThirdExpressionNode = AnalyseTwelfthLevelOp(pContext);

				if(NULL != pThirdExpressionNode)
				{
					CSyntaxTreeNode* pTmpNode = CreateExpressionNode(&objOperator);
					if(NULL != pTmpNode)
					{
						((CExpressionTreeNode*)pTmpNode)->SetChildNode(0,pFirstExpressionNode);   
						((CExpressionTreeNode*)pTmpNode)->SetChildNode(1,pSecondExpressionNode);   
						((CExpressionTreeNode*)pTmpNode)->SetChildNode(2,pThirdExpressionNode);
						TypeCompatibleChecking(pSecondExpressionNode,pThirdExpressionNode);

						//没有设置表达式类型
						pTmpNode->SetDataType(pSecondExpressionNode->GetDataType());

						AppendRef(pFirstExpressionNode,pTmpNode);
						AppendRef(pSecondExpressionNode,pTmpNode);
						AppendRef(pThirdExpressionNode,pTmpNode);

						TypeCompatibleChecking(pSecondExpressionNode,pThirdExpressionNode);

						pFirstExpressionNode = pTmpNode;
					}               
				}
				else
				{
					Destroy(pFirstExpressionNode);
					Destroy(pSecondExpressionNode);
					pFirstExpressionNode = NULL;

					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
						,_T("syntax error"));
				}
			}
			else
			{
				Destroy(pFirstExpressionNode);
				Destroy(pSecondExpressionNode);
				pFirstExpressionNode = NULL;

				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ':'"));
				m_pScaner->PushBack();       // error recovery
			}        
		}
		else
		{
			Destroy(pFirstExpressionNode);
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			pFirstExpressionNode = NULL;
		}   
	}
	else
	{
		m_pScaner->PushBack();// put the next token back
	}

	return pFirstExpressionNode;
}
/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingBitOp
| 描  述 : 比较操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingLogicalOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1
	,CSyntaxTreeNode* pOperand2)
{
	ATLASSERT(pOperand1 && pOperand2);

	CDataTypeTreeNode* pDataTypeTreeNode = pOperand1->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!(pDataTypeTreeNode->IsNumeric()
		|| pDataTypeTreeNode->GetNodeType() == TNK_POINTER_TYPE
		|| pDataTypeTreeNode->GetNodeType() == TNK_ARRAY_TYPE))
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, left operand has type '%s'"),CString(lpOperator)
			,CString(pDataTypeTreeNode->GetSymbol()));
	}

	pDataTypeTreeNode = pOperand2->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!(pDataTypeTreeNode->IsNumeric()
		|| pDataTypeTreeNode->GetNodeType() == TNK_POINTER_TYPE
		|| pDataTypeTreeNode->GetNodeType() == TNK_ARRAY_TYPE))
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, right operand has type '%s'"),CString(lpOperator)
			,CString(pDataTypeTreeNode->GetSymbol()));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseTwelfthLevelOp
| 描  述 : Analyse an Logic expression.
| Grammar:
| 17. AnalyseTwelfthLevelOp->AnalyseTwelfthLevelOp `||` AnalyseEleventhLevelOp | AnalyseEleventhLevelOp
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:28:32  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseTwelfthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode *pLeftExpressionNode = AnalyseEleventhLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);

	while( m_pCurrentToken->GetType() == TT_LOGICAL_OR ) 
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseEleventhLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}     

		if(pLeftExpressionNode->IsNumericConstants()) 
		{
			if((pLeftExpressionNode->GetValue1() + pLeftExpressionNode->GetValue2()) != 0)
			{
				Destroy(pRightExpressionNode);
				pRightExpressionNode = NULL;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				pLeftExpressionNode = pRightExpressionNode;
			}
		}
		else if(pRightExpressionNode->IsNumericConstants()) 
		{
			if( (pRightExpressionNode->GetValue1() + pRightExpressionNode->GetValue2()) != 0)
			{
				Destroy(pLeftExpressionNode);
				pLeftExpressionNode = pRightExpressionNode;
			}
			else
			{
				Destroy(pRightExpressionNode);
				pRightExpressionNode = NULL;
			}
		}
		else
		{
			TypeCheckingLogicalOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,pRightExpressionNode);

			pTmpNode = CreateExpressionNode(&objOperator,TT_BOOL,"bool"
				,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode); 

				pLeftExpressionNode = NULL;
				break;
			}   
		}

		objOperator = *(m_pScaner->GetNextToken());   
	}
	m_pScaner->PushBack();// put the next token back

	return pLeftExpressionNode;  
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseEleventhLevelOp
| 描  述 : Analyse an Logic expression.
| Grammar:
| 18. AnalyseEleventhLevelOp-> AnalyseEleventhLevelOp `&&` AnalyseSixthLevelOp | AnalyseSixthLevelOp
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:41:07  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseEleventhLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode *pLeftExpressionNode = AnalyseTenthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);

	while( m_pCurrentToken->GetType() == TT_LOGICAL_AND ) 
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseTenthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}     

		if(pLeftExpressionNode->IsNumericConstants()) 
		{
			if((pLeftExpressionNode->GetValue1() + pLeftExpressionNode->GetValue2()) == 0)
			{
				Destroy(pRightExpressionNode);
				pRightExpressionNode = NULL;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				pLeftExpressionNode = pRightExpressionNode;
			}
		}
		else if(pRightExpressionNode->IsNumericConstants()) 
		{
			if( (pRightExpressionNode->GetValue1() + pRightExpressionNode->GetValue2()) == 0)
			{
				Destroy(pLeftExpressionNode);
				pLeftExpressionNode = pRightExpressionNode;
			}
			else
			{
				Destroy(pRightExpressionNode);
				pRightExpressionNode = NULL;
			}
		}
		else
		{
			TypeCheckingLogicalOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,pRightExpressionNode);
			
			pTmpNode = CreateExpressionNode(&objOperator,TT_BOOL,"bool"
					,pLeftExpressionNode,pRightExpressionNode);
		

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;
			}
			else
			{
				Destroy( pLeftExpressionNode);
				Destroy( pRightExpressionNode); 

				pLeftExpressionNode = NULL;
				break;
			}   
		}

		objOperator = *(m_pScaner->GetNextToken());   
	}
	m_pScaner->PushBack();// put the next token back

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingBitOp
| 描  述 : 比较操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingBitOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1
	,CSyntaxTreeNode* pOperand2)
{
	ATLASSERT(pOperand1 && pOperand2);

	CDataTypeTreeNode* pDataTypeTreeNode = pOperand1->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!pDataTypeTreeNode->IsInterger())
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, left operand has type '%s'"),CString(lpOperator)
			,CString(pDataTypeTreeNode->GetSymbol()));
	}

	pDataTypeTreeNode = pOperand2->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!pDataTypeTreeNode->IsInterger())
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, right operand has type '%s'"),CString(lpOperator)
			,CString(pDataTypeTreeNode->GetSymbol()));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseNinthLevelOp
| 描  述 : 处理位或运算符号"|"
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseTenthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseNinthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);   

	while(m_pCurrentToken->GetType() == TT_BITWISE_OR ) 
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseNinthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}

		TypeCheckingBitOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,
			pRightExpressionNode);

		if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
			&& pRightExpressionNode->GetNodeType() == TNK_INTEGER_CST) //说明是常量立即计算出值
		{        
			(*(CConstTreeNode*)pLeftExpressionNode) |= ((CConstTreeNode*)pRightExpressionNode);

			Destroy(pRightExpressionNode); 
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode); 

				pLeftExpressionNode = NULL;
				break;
			}   
		}

		objOperator = *(m_pScaner->GetNextToken());
	} 

	m_pScaner->PushBack();

	return pLeftExpressionNode;   
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseNinthLevelOp
| 描  述 : 处理位异或运算符号"^"
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseNinthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseEighthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);   

	while(m_pCurrentToken->GetType() == TT_BITWISE_XOR ) 
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseEighthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}

		TypeCheckingBitOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,
			pRightExpressionNode);

		if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
			&& pRightExpressionNode->GetNodeType() == TNK_INTEGER_CST) //说明是常量立即计算出值
		{        
			(*(CConstTreeNode*)pLeftExpressionNode) ^= ((CConstTreeNode*)pRightExpressionNode);

			Destroy(pRightExpressionNode); 
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode); 

				pLeftExpressionNode = NULL;
				break;
			}   
		}

		objOperator = *(m_pScaner->GetNextToken());
	} 

	m_pScaner->PushBack();

	return pLeftExpressionNode;   
}



/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseEighthLevelOp
| 描  述 : 处理位与运算符号"&"
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseEighthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseSeventhLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);   

	while(m_pCurrentToken->GetType() == TT_BITWISE_AND ) 
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseSeventhLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}
		TypeCheckingBitOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,pRightExpressionNode);

		if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
			&& pRightExpressionNode->GetNodeType() == TNK_INTEGER_CST) //说明是常量立即计算出值
		{        
			(*(CConstTreeNode*)pLeftExpressionNode) &= ((CConstTreeNode*)pRightExpressionNode);

			Destroy(pRightExpressionNode); 
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode); 

				pLeftExpressionNode = NULL;
				break;
			}   
		}

		objOperator = *(m_pScaner->GetNextToken());
	} 

	m_pScaner->PushBack();

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingCompareOp
| 描  述 : 比较操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingCompareOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1
	,CSyntaxTreeNode* pOperand2)
{
	CDataTypeTreeNode* pDataTypeTreeNode1 = pOperand1->GetDataType();
	CDataTypeTreeNode* pDataTypeTreeNode2 = pOperand2->GetDataType();

	ATLASSERT(pDataTypeTreeNode1 && pDataTypeTreeNode2);

	if(pDataTypeTreeNode1 && pDataTypeTreeNode2)
	{   
		if(pDataTypeTreeNode1->IsNumeric())
		{
			if(!pDataTypeTreeNode2->IsNumeric())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '%s' : no conversion from '%s' to '%s'"),CString(lpOperator)
					,CString(pDataTypeTreeNode2->GetSymbol())
					,CString(pDataTypeTreeNode1->GetSymbol()));
			}
		}
		else if(pDataTypeTreeNode1->GetNodeType() == TNK_POINTER_TYPE
			|| pDataTypeTreeNode1->GetNodeType() == TNK_ARRAY_TYPE)
		{
			if(pDataTypeTreeNode2->GetNodeType() != TNK_POINTER_TYPE
				&& pDataTypeTreeNode2->GetNodeType() != TNK_ARRAY_TYPE)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '%s' : no conversion from '%s' to '%s'"),CString(lpOperator)
					,CString(pDataTypeTreeNode2->GetSymbol())
					,CString(pDataTypeTreeNode1->GetSymbol()));
			}
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
				,_T(" '%s' : illegal, left operand has type '%s'"),CString(lpOperator)
				,CString(pDataTypeTreeNode1->GetSymbol()));
		}   
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseSixthLevelOp
| 描  述 : Analyse a Simple expression.
| Grammar:
| 19. AnalyseSixthLevelOp->AnalyseFourthLevelOp relop AnalyseFourthLevelOp | AnalyseFourthLevelOp
| 20. relop-> `==` | `!=` 
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseSeventhLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseSixthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);

	if(m_pCurrentToken->GetType() == TT_EQ || m_pCurrentToken->GetType() == TT_NEQ ) 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseSixthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}

		TypeCheckingCompareOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,pRightExpressionNode);

		if(pLeftExpressionNode->IsNumericConstants()
			&& pRightExpressionNode->IsNumericConstants() )  //说明两边是常量,立即计算出结果
		{
			switch(objOperator.GetType())
			{
			case TT_EQ:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) == ((CConstTreeNode*)pRightExpressionNode));
				break;

			case TT_NEQ:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) != ((CConstTreeNode*)pRightExpressionNode));
				break;

			default:
				ATLASSERT(FALSE);
			}

			Destroy(pRightExpressionNode);
			pRightExpressionNode = NULL;

			pLeftExpressionNode = ConvertConstRealToInt((CConstTreeNode*)pLeftExpressionNode);        
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,TT_BOOL,"bool"
				,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;        
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode);

				pLeftExpressionNode = NULL;
			}
		}     
	} 
	else
	{
		m_pScaner->PushBack();
	}

	return pLeftExpressionNode;
}



/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseSixthLevelOp
| 描  述 : Analyse a Simple expression.
| Grammar:
| 19. AnalyseSixthLevelOp->AnalyseFourthLevelOp relop AnalyseFourthLevelOp | AnalyseFourthLevelOp
| 20. relop-> `<=` | `<` | `>` | `>=`
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseSixthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseFifthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);

	if( m_pCurrentToken->GetType()  == TT_NGT 
		|| m_pCurrentToken->GetType() == TT_LT 
		|| m_pCurrentToken->GetType() == TT_GT 
		|| m_pCurrentToken->GetType() == TT_NLT) 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseFifthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			Destroy(pLeftExpressionNode);
			return NULL;
		}
		TypeCheckingCompareOp(objOperator.GetSymbolTitle(),pLeftExpressionNode,pRightExpressionNode);

		if(pLeftExpressionNode->IsNumericConstants()
			&& pRightExpressionNode->IsNumericConstants() )  //说明两边是常量,立即计算出结果
		{
			switch(objOperator.GetType())
			{
			case TT_GT:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) > ((CConstTreeNode*)pRightExpressionNode));
				break;

			case TT_NGT:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) >= ((CConstTreeNode*)pRightExpressionNode));
				break;

			case TT_LT:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) < ((CConstTreeNode*)pRightExpressionNode));
				break;

			case TT_NLT:
				pLeftExpressionNode = ((*(CConstTreeNode*)pLeftExpressionNode) <= ((CConstTreeNode*)pRightExpressionNode));
				break;
			default:
				ATLASSERT(FALSE);
			}

			Destroy(pRightExpressionNode);
			pRightExpressionNode = NULL;

			pLeftExpressionNode = ConvertConstRealToInt((CConstTreeNode*)pLeftExpressionNode);        
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,TT_BOOL,"bool"
				,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;        
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode);

				pLeftExpressionNode = NULL;
				pRightExpressionNode = NULL;
			}
		}
	} 
	else
	{
		m_pScaner->PushBack();
	}

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingFifthLevelOp
| 描  述 : 比较操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingFifthLevelOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1
	,CSyntaxTreeNode* pOperand2)
{
	CDataTypeTreeNode* pDataTypeTreeNode = pOperand1->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!pDataTypeTreeNode->IsInterger())
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, left operand has type '%s'")
			, CString(lpOperator)
			, CString(pDataTypeTreeNode->GetSymbol()));
	}
	pDataTypeTreeNode = pOperand2->GetDataType();
	ATLASSERT(pDataTypeTreeNode);
	if(!pDataTypeTreeNode->IsInterger())
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' is illegal, right operand has type '%s'")
			, CString(lpOperator)
			, CString(pDataTypeTreeNode->GetSymbol()));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseFifthLevelOp
| 描  述 : Analyse a Simple expression.
| Grammar:
| 19. AnalyseSixthLevelOp->AnalyseFourthLevelOp relop AnalyseFourthLevelOp | AnalyseFourthLevelOp
| 20. relop-> `<<` | `>>`
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:50:56  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFifthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseFourthLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken     = m_pScaner->GetNextToken();   
	CSymbolInfo objOperator = *(m_pCurrentToken);

	if( m_pCurrentToken->GetType()  == TT_LSHIFT 
		|| m_pCurrentToken->GetType() == TT_RSHIFT) 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseFourthLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("syntax error"));
			return pLeftExpressionNode;
		}

		TypeCheckingFifthLevelOp( (objOperator.GetType() == TT_LSHIFT)? "<<" :">>"
			, pLeftExpressionNode,pRightExpressionNode);


		if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
			&& pRightExpressionNode->GetNodeType() == TNK_INTEGER_CST) //说明是常量立即计算出值
		{
			if(objOperator.GetType() == TT_LSHIFT)
			{
				*(CConstTreeNode*)pLeftExpressionNode <<= ((CConstTreeNode*)pRightExpressionNode);
			}
			else
			{
				*(CConstTreeNode*)pLeftExpressionNode >>= ((CConstTreeNode*)pRightExpressionNode);
			}
			Destroy(pRightExpressionNode); 
		}
		else
		{
			pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);

			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;        
			}
			else
			{
				Destroy(pLeftExpressionNode);
				Destroy(pRightExpressionNode);

				pLeftExpressionNode = NULL;
			}
		}
	} 
	else
	{
		m_pScaner->PushBack();
	}

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingFourthLevelOp
| 描  述 : 操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingFourthLevelOp(AXC_SYMBOL_TYPE NODE_TYPE
	,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2)
{
	CDataTypeTreeNode* pDataTypeTreeNode1 = pOperand1->GetDataType();
	CDataTypeTreeNode* pDataTypeTreeNode2 = pOperand2->GetDataType();


	if(pDataTypeTreeNode1 && pDataTypeTreeNode2)
	{
		switch(NODE_TYPE)
		{
		case TT_PLUS:   
			if(pDataTypeTreeNode1->IsInterger())
			{
				if(!pDataTypeTreeNode2->IsNumeric() 
					&& pDataTypeTreeNode2->GetNodeType() != TNK_POINTER_TYPE
					&& pDataTypeTreeNode2->GetNodeType() != TNK_ARRAY_TYPE)
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
						,_T(" '+' : illegal, right operand has type '%s'")
						,CString(pDataTypeTreeNode2->GetSymbol()));
				}
			}
			else if(pDataTypeTreeNode1->GetNodeType() == TNK_POINTER_TYPE
				|| pDataTypeTreeNode1->GetNodeType() == TNK_ARRAY_TYPE)
			{
				if(!pDataTypeTreeNode2->IsInterger())
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
						,_T(" '+' : illegal, right operand has type '%s'")
						,CString(pDataTypeTreeNode2->GetSymbol()));
				}
			}
			else if(pDataTypeTreeNode1->IsFloat())
			{
				if(!pDataTypeTreeNode2->IsNumeric())
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
						,_T(" '+' : illegal, right operand has type '%s'")
						,CString(pDataTypeTreeNode2->GetSymbol()));
				}
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '+' : illegal, left operand has type '%s'")
					,CString(pDataTypeTreeNode1->GetSymbol()));
			}
			break;

		case TT_MINUS:
			if(pDataTypeTreeNode1->IsNumeric())
			{
				if(!pDataTypeTreeNode2->IsNumeric())
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
						,_T(" '-' : illegal, right operand has type '%s'"),CString(pDataTypeTreeNode2->GetSymbol()));
				}
			}
			else if(pDataTypeTreeNode1->GetNodeType() == TNK_POINTER_TYPE
				|| pDataTypeTreeNode1->GetNodeType() == TNK_ARRAY_TYPE)
			{
				if(!pDataTypeTreeNode2->IsInterger())
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
						,_T(" '-' : illegal, right operand has type '%s'"),CString(pDataTypeTreeNode2->GetSymbol()));
				}
			}        
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '-' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode1->GetSymbol()));
			}
			break;
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseFourthLevelOp
| 描  述 : Analyse an additive expression.
| Grammar:
| 21. AnalyseFourthLevelOp -> AnalyseFourthLevelOp addop AnalyseThirdLevelOp | AnalyseThirdLevelOp
| 22. addop-> `+` | `-`
| 返 回 值 : 
| 修改记录 : 2007-4-23 16:55:04  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFourthLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseThirdLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	CSymbolInfo objOperator = *(m_pScaner->GetNextToken());
	while( m_pCurrentToken->GetType() == TT_PLUS || m_pCurrentToken->GetType() == TT_MINUS )
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseThirdLevelOp(pContext);

		if(NULL != pRightExpressionNode)
		{
			TypeCheckingFourthLevelOp(objOperator.GetType(),
				pLeftExpressionNode,pRightExpressionNode);

			if(pLeftExpressionNode->IsNumericConstants()
				&& pRightExpressionNode->IsNumericConstants() )  //说明两边是常量,立即计算出结果
			{
				if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
					&& pRightExpressionNode->GetNodeType() == TNK_REAL_CST) //结果就为浮点数
				{
					CConstRealTreeNode*   pRealTreeNode =   CreateConstRealTreeNode(double(pLeftExpressionNode->GetValue1()));
					if(pRealTreeNode)
					{
						Destroy(pLeftExpressionNode);
						pLeftExpressionNode = pRealTreeNode;
					}
				}

				switch(objOperator.GetType())
				{
				case TT_PLUS:
					(*(CConstTreeNode*)pLeftExpressionNode) += ((CConstTreeNode*)pRightExpressionNode);
					break;

				case TT_MINUS:
					(*(CConstTreeNode*)pLeftExpressionNode) -= ((CConstTreeNode*)pRightExpressionNode);
					break;
				}

				Destroy(pRightExpressionNode);
			}
			else
			{
				pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);
				if(NULL != pTmpNode)
				{
					pLeftExpressionNode = pTmpNode;
				}
				else
				{
					Destroy(pLeftExpressionNode);
					Destroy(pRightExpressionNode);

					pLeftExpressionNode = NULL;
					pRightExpressionNode = NULL;
				}
			}

			objOperator = *(m_pScaner->GetNextToken());
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("expected an expression"));
			break;
		}   
	}
	m_pScaner->PushBack();  // put the next token back

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingThirdLevelOp
| 描  述 : 操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingThirdLevelOp(AXC_SYMBOL_TYPE NODE_TYPE,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2)
{
	CDataTypeTreeNode* pDataTypeTreeNode1 = pOperand1->GetDataType();
	CDataTypeTreeNode* pDataTypeTreeNode2 = pOperand2->GetDataType();

	if(pDataTypeTreeNode1 && pDataTypeTreeNode2)
	{
		switch(NODE_TYPE)
		{
		case TT_MULT:   
			if(!pDataTypeTreeNode1->IsNumeric())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '*' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode1->GetSymbol()));
			}

			if(!pDataTypeTreeNode2->IsNumeric())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand2->GetLineIndex(),0
					,_T(" '*' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode2->GetSymbol()));
			}
			break;

		case TT_DIV:
			if(!pDataTypeTreeNode1->IsNumeric())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '/' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode1->GetSymbol()));
			}

			if(!pDataTypeTreeNode2->IsNumeric())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand2->GetLineIndex(),0
					,_T(" '/' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode2->GetSymbol()));
			}
			break;

		case TT_MOD:
			if(!pDataTypeTreeNode1->IsInterger())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand1->GetLineIndex(),0
					,_T(" '%%' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode1->GetSymbol()));
			}

			if(!pDataTypeTreeNode2->IsInterger())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand2->GetLineIndex(),0
					,_T(" '%%' : illegal, left operand has type '%s'"),CString(pDataTypeTreeNode2->GetSymbol()));
			}
			break;
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseThirdLevelOp
| 描  述 : Analyse a term expression.
| Grammar:
| 23. AnalyseThirdLevelOp->AnalyseThirdLevelOp mulop AnalyseSecondLevelOp | AnalyseSecondLevelOp
| 24. mulop-> `*` | `/` | `%`
| 返 回 值 : If the function is successful,return a pointer to 
|       a node or child tree to be created,Otherwise null.
| 修改记录 : 2007-4-23 17:01:17  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseThirdLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pTmpNode       = NULL;
	CSyntaxTreeNode* pRightExpressionNode = NULL;
	CSyntaxTreeNode* pLeftExpressionNode = AnalyseSecondLevelOp(pContext);
	if(NULL == pLeftExpressionNode)
	{
		return NULL;
	}

	m_pCurrentToken = m_pScaner->GetNextToken();
	CSymbolInfo objOperator = *m_pCurrentToken;
	while( objOperator.GetType() == TT_MULT 
		|| objOperator.GetType() == TT_DIV 
		|| objOperator.GetType() == TT_MOD ) //处理连续的乘除关系
	{
		m_pCurrentToken   = m_pScaner->GetNextToken();
		pRightExpressionNode = AnalyseSecondLevelOp(pContext);
		if(NULL == pRightExpressionNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("expected an expression"));
			break;
		}

		if(pLeftExpressionNode->IsNumericConstants() && (pLeftExpressionNode->GetValue1()
			+ pLeftExpressionNode->GetValue2()) == 0) //0乘, 除,模以任何数得0
		{
			Destroy(pRightExpressionNode);
		}
		else if(pRightExpressionNode->IsNumericConstants() && (pRightExpressionNode->GetValue1()
			+ pRightExpressionNode->GetValue2()) == 0 && objOperator.GetType() == TT_MULT) //0乘以任何数得0
		{
			Destroy(pLeftExpressionNode);
			pLeftExpressionNode = pRightExpressionNode;
		}   
		else if(pLeftExpressionNode->IsNumericConstants() && pRightExpressionNode->IsNumericConstants())  //说明两边是常量,立即计算出结果
		{
			if(pLeftExpressionNode->GetNodeType() == TNK_INTEGER_CST
				&& pRightExpressionNode->GetNodeType() == TNK_REAL_CST) //在乘法和除法运算中其中一个操作数为浮点数,结果就为浮点数
			{
				CConstRealTreeNode*   pRealTreeNode =   CreateConstRealTreeNode(double(pLeftExpressionNode->GetValue1()));
				if(pRealTreeNode)
				{
					Destroy(pLeftExpressionNode);
					pLeftExpressionNode = pRealTreeNode;
				}
			}          

			try
			{
				switch(objOperator.GetType())
				{
				case TT_MULT:
					(*(CConstTreeNode*)pLeftExpressionNode) *= ((CConstTreeNode*)pRightExpressionNode);
					break;

				case TT_DIV:
					(*(CConstTreeNode*)pLeftExpressionNode) /= ((CConstTreeNode*)pRightExpressionNode);
					break;

				case TT_MOD:
					(*(CConstTreeNode*)pLeftExpressionNode) %= ((CConstTreeNode*)pRightExpressionNode);
					break;
				default:
					ATLASSERT(FALSE);
				}
			}
			catch(CParserException& objParserException) 
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("%s")
					,CString(objParserException.what()));
			}
			Destroy(pRightExpressionNode);
		}     
		else
		{
			TypeCheckingThirdLevelOp(objOperator.GetType(),pLeftExpressionNode,pRightExpressionNode);

			pTmpNode = CreateExpressionNode(&objOperator,pLeftExpressionNode,pRightExpressionNode);
			if(NULL != pTmpNode)
			{
				pLeftExpressionNode = pTmpNode;             
			}
			else
			{
				Destroy( pLeftExpressionNode);
				Destroy( pRightExpressionNode);

				pLeftExpressionNode = NULL;
				pRightExpressionNode = NULL;
			}
		}

		objOperator = *(m_pScaner->GetNextToken());
	}

	m_pScaner->PushBack();// put the next token back

	return pLeftExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::TypeCheckingSecondLevelOp
| 描  述 : 操作数类型检查
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::TypeCheckingSecondLevelOp(AXC_SYMBOL_TYPE NODE_TYPE,CSyntaxTreeNode* pOperand)
{
	CDataTypeTreeNode* pDataTypeTreeNode = NULL;
	pDataTypeTreeNode = pOperand->GetDataType();
	if(pDataTypeTreeNode)
	{
		switch(NODE_TYPE)
		{
		case TT_MINUS_MINUS:
			if(!pDataTypeTreeNode->IsNumeric() && (pDataTypeTreeNode->GetNodeType() != TNK_POINTER_TYPE))
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T(" '~' : illegal on operands of type '%s'"),CString(pDataTypeTreeNode->GetSymbol()));
			}

			if(pDataTypeTreeNode->GetNodeType() == TNK_BOOLEAN_TYPE)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T("'--' : not allowed on operand of type 'bool'"));
			}
			break;

		case TT_BITWISE_NOT:
			if(!pDataTypeTreeNode->IsInterger())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T(" '~' : illegal on operands of type '%s'"),CString(pDataTypeTreeNode->GetSymbol()));
			}
			break;

		case TT_LOGICAL_NOT:
		case TT_PLUS_PLUS:
			if(!pDataTypeTreeNode->IsNumeric() && (pDataTypeTreeNode->GetNodeType() != TNK_POINTER_TYPE))
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T(" '~' : illegal on operands of type '%s'"),CString(pDataTypeTreeNode->GetSymbol()));
			}
			break;

		case TT_MINUS:
			if(!(pDataTypeTreeNode->IsNumeric()))
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T(" '~' : illegal on operands of type '%s'"),CString(pDataTypeTreeNode->GetSymbol()));
			}
			break;

		case TT_MULT:
			if(pDataTypeTreeNode->GetNodeType() != TNK_POINTER_TYPE)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T("illegal indirection"));
			}
			break;

		case TT_BITWISE_AND:
			if(!pOperand->IsVariable() 
				&& pOperand->GetNodeType() != TNK_INDIRECT_REF
				&& pOperand->GetNodeType() != TNK_ARRAY_REF)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, pOperand->GetLineIndex(),0
					,_T("'&' requires l-value"));
			}
			break;
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseSizeof
| 描  述 : 分析sizeof语法,直接计算出数据空间的字节数
| 返 回 值 : 一个整形常量节点
| 修改记录 : 2007-4-23 17:13:15  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseSizeof(CParserContext* /*pContext*/)
{
	UINT nSize = 0;

	if(m_pCurrentToken->GetType() != TT_LPARAN)             // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
			,0,_T("missing '('"));
	}
	m_pCurrentToken  = m_pScaner->GetNextToken();
	switch(m_pCurrentToken->GetType())
	{
	case TT_STRING_TYPE:
		nSize = m_pCurrentToken->GetSymbol().GetLength() + 1;
		break;

	case TT_CHAR_TYPE:
	case TT_CHAR:
	case TT_BYTE:
	case TT_BOOL:
		nSize = 1;
		break;

	case TT_SHORT:
	case TT_WORD:
		nSize = 2;
		break;

	case TT_INTEGER_TYPE:
	case TT_INT:
	case TT_LONG:
	case TT_FLOAT:
	case TT_DWORD:
		nSize = 4;
		break;

	case TT_REAL_TYPE:
	case TT_DOUBLE:
	case TT_LONGLONG:
		nSize = 8;
		break;

	case TT_IDENTIFY_TYPE:
		{
			CSyntaxTreeNode* pDataTypeNode = NULL;
			CSyntaxTreeNode* pVarDeclNode = LookupDeclNode(m_pCurrentToken->GetSymbolTitle());
			if(NULL != pVarDeclNode)
			{
				pDataTypeNode = pVarDeclNode->GetDataType();
			}
			else
			{//自定义类型,比如:结构,联合
				pDataTypeNode = LookupTagNode(m_pCurrentToken->GetSymbolTitle());
			}

			if(NULL != pDataTypeNode)
			{
				nSize = pDataTypeNode->GetSize();
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
					,0,_T("expect a variable or variable type"));
			}
		}
		break;

	case TT_VOID:
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
			,0,_T("'void': illegal sizeof operand"));
		break;

	case TT_MULT:
		{
			int nPointerDeep = 0;
			do
			{
				nPointerDeep++;
				m_pCurrentToken  = m_pScaner->GetNextToken();
			}
			while(m_pCurrentToken->GetType() == TT_MULT);

			if(m_pCurrentToken->GetType() == TT_IDENTIFY_TYPE)
			{
				CDataTypeTreeNode* pDataTypeNode = NULL;
				CSyntaxTreeNode* pVarDeclNode = LookupDeclNode(m_pCurrentToken->GetSymbolTitle());

				if(NULL != pVarDeclNode)
				{
					pDataTypeNode = pVarDeclNode->GetDataType();
					if(pDataTypeNode->GetNodeType() == TNK_POINTER_TYPE)
					{
						if(nPointerDeep == pDataTypeNode->GetPointerDepth())
						{
							nSize = pDataTypeNode->GetDataType()->GetSize();
						}
						else
						{
							nSize = 4;
						}
					}
					else
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
							,0,_T("operand of the '*' must be a pointer"));
					}
				}
				else
				{
					m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
						,0,_T("identifier '%s' is undefined"),CString(m_pCurrentToken->GetSymbolTitle()));
				}
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
					,0,_T("operand of the '*' must be a pointer"));
			}
		}
		break;
	default:   
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
			,0,_T("expect a variable or variable type"));
	}

	if( !Match(TT_RPARAN) )             // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
			,0,_T("missing ')'"));
	}

	return CreateConstIntTreeNode(nSize);
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseSecondLevelOp
| 描  述 : 分析单目运算符
| 返 回 值 : If the function is successful,return a pointer to 
|       a node or child tree to be created,Otherwise null.
| 修改记录 : 2007-4-23 17:06:44  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseSecondLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode *pExpressionNode = NULL;
	CSyntaxTreeNode *pLeftExprNode    = NULL;

	CSymbolInfo objOperator = *m_pCurrentToken;
	const AXC_SYMBOL_TYPE NODE_TYPE = m_pCurrentToken->GetType();
	if( NODE_TYPE == TT_LOGICAL_NOT
		|| NODE_TYPE == TT_BITWISE_NOT
		|| NODE_TYPE == TT_PLUS_PLUS
		|| NODE_TYPE == TT_MINUS_MINUS
		|| NODE_TYPE == TT_MINUS    //负号
		|| NODE_TYPE == TT_MULT     //指针引用,不是乘法运算符号
		|| NODE_TYPE == TT_BITWISE_AND //取地址符号,不是位与
		|| NODE_TYPE == TT_SIZEOF) 
	{   
		m_pCurrentToken = m_pScaner->GetNextToken();
		if(NODE_TYPE == TT_SIZEOF)
		{
			pLeftExprNode = AnalyseSizeof(pContext);
		}
		else
		{
			pLeftExprNode = AnalyseSecondLevelOp(pContext);
		}

		if(NULL == pLeftExprNode)
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
				,_T("'~' : illegal on operands of type"));
			return NULL;
		}
		TypeCheckingSecondLevelOp(NODE_TYPE,pLeftExprNode);


		if(pLeftExprNode->IsNumericConstants())
		{
			switch(NODE_TYPE)
			{
			case TT_LOGICAL_NOT:
				pExpressionNode = CreateConstIntTreeNode(
					((pLeftExprNode->GetValue1() + pLeftExprNode->GetValue2()) == 0)? 1 :0);
				break;

			case TT_BITWISE_NOT:          
				pExpressionNode = CreateConstIntTreeNode(int(~(pLeftExprNode->GetValue1())) );
				break;

			case TT_MINUS:
				if(pLeftExprNode->IsFloat())
				{
					pExpressionNode = CreateConstRealTreeNode(-pLeftExprNode->GetValue2());
				}
				else
				{
					pExpressionNode = CreateConstIntTreeNode(int(-(pLeftExprNode->GetValue1())) );
				}
				break;

			case TT_SIZEOF:
				pExpressionNode = pLeftExprNode;
				break;

			default:
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
					,_T("expression must be a modifiable lvalue"));
			}
			if(NODE_TYPE != TT_SIZEOF)
			{
				Destroy( pLeftExprNode);
			}
		}
		else
		{
			pExpressionNode = CreateExpressionNode(&objOperator);
			if(NULL != pExpressionNode)
			{
				AppendRef(pLeftExprNode,pExpressionNode);

				((CExpressionTreeNode*)pExpressionNode)->SetChildNode(0, pLeftExprNode);
				pExpressionNode->SetDataType(pLeftExprNode->GetDataType());
				switch(NODE_TYPE)
				{
				case TT_LOGICAL_NOT:
					pExpressionNode->SetDataType(GetInternalDataType(TT_BOOL,"bool"));
					break;

				case TT_MINUS:    //需要修改表达式类型
					pExpressionNode->SetNodeType(TNK_NEGATE_EXPR);
					break;

				case TT_MULT:   
					{
						pExpressionNode->SetNodeType(TNK_INDIRECT_REF);

						CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();
						ATLASSERT(pLeftNodeDataType);
						if(pLeftNodeDataType && pLeftNodeDataType->GetNodeType() == TNK_POINTER_TYPE)
						{
							if(pLeftNodeDataType->GetPointerDepth() == 1)
							{
								pExpressionNode->SetDataType(pLeftNodeDataType->GetDataType());
							}
							else
							{
								int nDepth = 0;
								pLeftNodeDataType = pLeftNodeDataType->GetPointerBaseType(nDepth);                    
								pExpressionNode->SetDataType(GetPointerDataType(pLeftNodeDataType,nDepth - 1));
							}
						}
						else
						{
							m_pSaxCCompileimpl->OutputErrMsg(TRUE, pLeftExprNode->GetLineIndex(),0
								,_T("Illegal instruction"));
						}
					}
					break;

				case TT_BITWISE_AND:
					pExpressionNode->SetNodeType(TNK_ADDR_EXPR);
					pExpressionNode->SetDataType(GetPointerDataType(pLeftExprNode->GetDataType(),1)); //实际是指针
					break;

					/*case TT_SIZEOF:
					m_pSaxCCompileimpl->OutputErrMsg(_T("missing ')'")
					, m_pCurrentToken->GetLineIndex());
					break;*/
				}
			}
		}
	} 
	else
	{
		pExpressionNode = AnalyseFirstLevelOp(pContext);
		m_pCurrentToken = m_pScaner->GetNextToken();
		if(NULL != pExpressionNode && (m_pCurrentToken->GetType() == TT_PLUS_PLUS
			|| m_pCurrentToken->GetType() == TT_MINUS_MINUS))
		{
			TypeCheckingSecondLevelOp(m_pCurrentToken->GetType(),pExpressionNode);

			pLeftExprNode = pExpressionNode;
			pExpressionNode = CreateExpressionNode(m_pCurrentToken,pLeftExprNode->GetDataType(),pLeftExprNode);
			if(NULL != pExpressionNode)
			{
				pExpressionNode->SetNodeType((m_pCurrentToken->GetType() == TT_PLUS_PLUS)? TNK_POSTINCREMENT_EXPR : TNK_POSTDECREMENT_EXPR);
			}
		}
		else
		{
			m_pScaner->PushBack();// put the next token back
		}
	}
	if(NULL != pExpressionNode)
	{
		if(pExpressionNode->GetNodeType() == TNK_POSTINCREMENT_EXPR
			|| pExpressionNode->GetNodeType() == TNK_POSTDECREMENT_EXPR
			|| pExpressionNode->GetNodeType() == TNK_PREINCREMENT_EXPR
			|| pExpressionNode->GetNodeType() == TNK_PREDECREMENT_EXPR)
		{
			CDataTypeTreeNode* pDataTypeNode = pExpressionNode->GetDataType();
			if(pDataTypeNode->GetNodeType() == TNK_FLOAT_TYPE
				|| pDataTypeNode->GetNodeType() == TNK_DOUBLE_TYPE)
			{
				//浮点数的++或--,要保存隐性的常量1,
				((CExpressionTreeNode*)pExpressionNode)->SetChildNode(1, CreateConstIntTreeNode(1));
			}
		}
	}

	return pExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseFirstLevelOp
| 描  述 : 分析运算符号"()","[]","->",".",当前处理了"()"
| 返 回 值 : If the function is successful,return a pointer to 
|       a node or child tree to be created,Otherwise null.
| 修改记录 : 2007-4-23 17:09:55  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFirstLevelOp(CParserContext* pContext)
{
	CSyntaxTreeNode* pNewNode = NULL;

	switch( m_pCurrentToken->GetType() ) 
	{
	case TT_LPARAN:
		m_pCurrentToken = m_pScaner->GetNextToken();   // m_pCurrentToken contain the pFirstNode token of AnalyseFourteenthLevelOp
		pNewNode = AnalyseFifteenthLevelOp(pContext);
		if( !Match(TT_RPARAN) )             // Match ')'
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ')'"));

			m_pScaner->PushBack();            // error recovery
		}
		break;

	default:
		pNewNode = AnalyseFactor(pContext);
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseFactor
| 描  述 : Grammar: 27. AnalyseFactor->ID | ID `[` AnalyseFourteenthLevelOp `]`
|        m_objIDToken contains ID
| 返 回 值 : 
| 修改记录 : 2007-4-23 17:13:15  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseFactor(CParserContext* pContext)
{
	CSyntaxTreeNode* pFactorNode = NULL;
	CSyntaxTreeNode* pVarDeclNode = NULL;

	switch(m_pCurrentToken->GetType())
	{
	case TT_STRING_TYPE:
		pFactorNode = CreateConstStringTreeNode(m_pCurrentToken->GetSymbolTitle());
		break;

	case TT_CHAR_TYPE:
		{
			CStringA strSymbol = m_pCurrentToken->GetSymbolTitle();
			if(!strSymbol.IsEmpty())
			{
				pFactorNode = CreateConstIntTreeNode(strSymbol.GetAt(0));
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("empty character constant"));        
			}
		}
		break;

	case TT_INTEGER_TYPE:
		pFactorNode = CreateConstIntTreeNode(_atoi64(m_pCurrentToken->GetSymbolTitle()));
		break;

	case TT_REAL_TYPE:
		pFactorNode = CreateConstRealTreeNode(atof(m_pCurrentToken->GetSymbolTitle()));
		break;

	case TT_IDENTIFY_TYPE:   //说明是变量,标签或函数名称
		{
			CSymbolInfo   objIDToken = *m_pCurrentToken;
			m_pCurrentToken  = m_pScaner->GetNextToken();   
			switch(m_pCurrentToken->GetType())
			{
			case TT_LPARAN:     //说明是函数调用
				pFactorNode = AnalyseCallStatement(&objIDToken,pContext);
				break;

			case TT_COLON:                // 说明是标签语句，如: "exit:"或者问号表达式a? b:c
				pFactorNode = LookupDeclNode(objIDToken.GetSymbolTitle());
				if(NULL == pFactorNode)
				{
					CDeclarationTreeNode* pLableDeclNode = CreateDeclNode(TNK_LABEL_DECL, &objIDToken); //创建声明节点
					pFactorNode = CreateStatementNode( TNK_LABEL_STMT, &objIDToken);//创建标签语句
					if(NULL != pFactorNode)
					{
						((CExpressionTreeNode*)pFactorNode)->SetChildNode(0,pLableDeclNode);
						pContext->RelatedLableRef(pLableDeclNode);
					}
				} 
				else
				{
					m_pScaner->PushBack();   //必须要推回去,分析问号表达式式需要
				}
				break;


			case TT_LSQUARE:    //说明是数组
				{
					pVarDeclNode = LookupDeclNode(objIDToken.GetSymbolTitle());
					if(NULL != pVarDeclNode)
					{
						if(pVarDeclNode->GetDataType()->GetNodeType() == TNK_ARRAY_TYPE
							|| pVarDeclNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
						{
							m_pCurrentToken  = m_pScaner->GetNextToken();   
							CSyntaxTreeNode* nIndex = AnalyseFourteenthLevelOp(pContext); //分析数组索引
							m_pCurrentToken  = m_pScaner->GetNextToken();
							if(m_pCurrentToken->GetType() != TT_RSQUARE)
							{
								m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex()
									,0,_T("missing ']'"));
							}
							if(NULL != nIndex)
							{
								pFactorNode = CreateExpressionNode(TNK_ARRAY_REF);
								if(pFactorNode)
								{
									CDataTypeTreeNode* pDataTypeTreeNode = pVarDeclNode->GetDataType();
									pFactorNode->SetDataType(pDataTypeTreeNode->GetDataType());
									((CExpressionTreeNode*)pFactorNode)->SetChildNode(0,pVarDeclNode);
									((CExpressionTreeNode*)pFactorNode)->SetChildNode(1,nIndex);

									AppendRef(pVarDeclNode,pFactorNode);
									AppendRef(nIndex,pFactorNode);
								}   
							}
							else
							{
								m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
									,_T("expect an index of '%s' array")
									,CString(objIDToken.GetSymbolTitle()));
							}
						}
						else
						{
							m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0
								,_T("subscript requires array or pointer type"));
						}
					}
					else
					{
						m_pSaxCCompileimpl->OutputErrMsg(TRUE, objIDToken.GetLineIndex(),0
							,_T("'%s' undeclared identifier"),CString(objIDToken.GetSymbolTitle()));
					}
				}
				break;

			default: //变量
				m_pScaner->PushBack();
				pVarDeclNode = LookupDeclNode(objIDToken.GetSymbolTitle());

				if(NULL != pVarDeclNode)
				{
					CDataTypeTreeNode* pDataTypeTreeNode = pVarDeclNode->GetDataType();

					if(pDataTypeTreeNode->GetNodeType() == TNK_REFERENCE_TYPE)
					{
						pFactorNode = CreateExpressionNode(TNK_INDIRECT_REF); //参考变量的应用和指针变量的应用是类型
						if(pFactorNode)
						{							
							pFactorNode->SetDataType(pDataTypeTreeNode->GetDataType());
							((CExpressionTreeNode*)pFactorNode)->SetChildNode(0,pVarDeclNode);

							AppendRef(pVarDeclNode,pFactorNode);
						}   
					}
					else
					{
						if(pVarDeclNode->GetNodeType() == TNK_CONST_DECL)  //常量变量直接用初始值
						{
							pFactorNode = ((CDeclarationTreeNode*)pVarDeclNode)->GetInitial();
							switch(pFactorNode->GetNodeType())
							{
							case TNK_INTEGER_CST:
								pFactorNode = CreateConstIntTreeNode(pFactorNode->GetValue1());
								break;

							case TNK_REAL_CST:
								pFactorNode = CreateConstRealTreeNode(pFactorNode->GetValue2());
								break;
							default:
								ATLASSERT(FALSE);
							}
						}
						else
						{
							pFactorNode = CreateExpressionNode(TNK_VAR_REF,
								objIDToken.GetLineIndex(),TT_IDENTIFY_TYPE,pVarDeclNode->GetDataType(),pVarDeclNode);						
						}
					}
				}
			}


			if(NULL == pFactorNode)   
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
					,_T("'%s' undeclared identifier")
					,CString(objIDToken.GetSymbolTitle()));
			}   
		}
		break;

	default:
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0
			,_T("'%s' undeclared identifier")
			,CString(m_pCurrentToken->GetSymbolTitle()));
	}   

	return pFactorNode;
}

// Grammar: 
// 28. Call->ID `(` args `)`
// m_pCurrentToken->GetSymbolTitle() == "(", m_objIDToken contains ID

/*-----------------------------------------------------------------
| 函数名称 : CParser::Call
| 描  述 : 
| 返 回 值 : 
| 修改记录 : 2007-4-23 17:15:13  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseCallStatement(CSymbolInfo* pIDToken,CParserContext* pContext)
{
	CExpressionTreeNode* pExpressionNode    = NULL;
	CSyntaxTreeNode* pNewNode    = NULL;

	CSyntaxTreeNode *pFunctionNode = LookupDeclNode(pIDToken->GetSymbolTitle()); 
	if(NULL != pFunctionNode)
	{
		pExpressionNode = CreateExpressionNode(TNK_CALL_EXPR);
		if(pExpressionNode)
		{
			pExpressionNode->SetDataType(pFunctionNode->GetDataType());
			pExpressionNode->SetChildNode(0,pFunctionNode);
			pNewNode = AnalyseArguments(pExpressionNode,pContext);
			pExpressionNode->SetChildNode(1,pNewNode);

			AppendRef(pFunctionNode,pExpressionNode);
		}   

		if( !Match(TT_RPARAN) )          // Match ')'
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("missing ')'"));

			m_pScaner->PushBack();         // error recovery
		}
	}
	else
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("'%s' identifier not found")
			,pIDToken->GetSymbolTitle());
	}

	return pExpressionNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseArguments
| 描  述 : 分析调用函数的所传的参数，顺序是按c语言的反序
| Grammar:
| 29. args->args_list | empty
| 30. args_list->args_list `,` AnalyseFourteenthLevelOp | AnalyseFourteenthLevelOp
| 返 回 值 : 
| 修改记录 : 2007-4-24 14:01:19  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseArguments(CExpressionTreeNode* pCallExpressionNode,CParserContext* pContext)
{
	CSyntaxTreeNode *pLastNode = NULL;
	CSyntaxTreeNode *pNewNode = NULL;

	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if( m_pCurrentToken->GetType() == TT_RPARAN ) 
	{
		return NULL; //说明没有参数
	}

	do 
	{
		pNewNode = AnalyseThirteenthLevelOp(pContext);

		if( NULL != pNewNode)  // link all args together, the LAST argument is the FIRST in the list
		{        
			pNewNode->SetParent(pCallExpressionNode);     //不能删除
			AppendRef(pNewNode,pCallExpressionNode);

			pNewNode->SetChain( pLastNode);
			pLastNode = pNewNode; 
		}

		m_pCurrentToken = m_pScaner->GetNextToken(); 

		if( m_pCurrentToken->GetType() == TT_COMMA )
		{
			m_pCurrentToken = m_pScaner->GetNextToken(); 
		}
		else
		{
			if( m_pCurrentToken->GetType() == TT_RPARAN )
			{
				m_pScaner->PushBack();
			}
			break;
		}
	}while(m_pCurrentToken->GetType() != TT_EOF_TYPE 
		&& m_pCurrentToken->GetType() != TT_ERROR_TYPE);   

	return pLastNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseIfStatement
| 描  述 : Analyse if statement.
| 32: AnalyseIfStatement->`if` `(` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements
|       | `if` `(` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements `else` AnalyseCompoundStatements
| 返 回 值 : If the function is successful,return a pointer to 
|       a node or child tree to be created,Otherwise null.
| 测试注意 : if与else是否能正确地匹配,else总是与它前面最近的未配对
|       的if配对
| 修改记录 : 2007-4-23 17:26:46  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseIfStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CSyntaxTreeNode *pExpressionNode = NULL;
	CExpressionTreeNode *pNewNode = CreateStatementNode( TNK_IF_STMT, 
		m_pCurrentToken);
	if(NULL == pNewNode)
	{
		return NULL;
	}
	pNewNode->SetParent(pParent);    //不能删除

	if( !Match(TT_LPARAN) )  // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '(' in 'if' statement."));
	}
	else 
	{
		m_pCurrentToken = m_pScaner->GetNextToken(); 
	}
	// m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	pExpressionNode = AnalyseFourteenthLevelOp(pContext);
	pNewNode->SetChildNode(0,pExpressionNode);
	if( NULL == pExpressionNode )
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing conditional expressions in 'if' statement."));
	}
	else
	{
		AppendRef(pExpressionNode,pNewNode);
	}

	if( !Match(TT_RPARAN) ) // Match ')'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ')' in 'if' statement."));

		m_pScaner->PushBack();
	}

	CreateNewBindingLevel(pContext);
	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if(m_pCurrentToken->GetType() != TT_LBRACE)  
	{                          //说明只有一条语句
		m_pScaner->PushBack();
	}
	pExpressionNode = AnalyseCompoundStatements(pNewNode,pContext,m_pCurrentToken->GetType() != TT_LBRACE);
	PopBindingLevel();

	pNewNode->SetChildNode(1,pExpressionNode);   



	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if( m_pCurrentToken->GetType() == TT_ELSE )               //Analyse "else" part of "if" statement
	{
		CreateNewBindingLevel(pContext);

		m_pCurrentToken = m_pScaner->GetNextToken(); 
		if(m_pCurrentToken->GetType() != TT_LBRACE)  
		{                            //说明只有一条语句
			m_pScaner->PushBack();
		}
		pExpressionNode = AnalyseCompoundStatements(pNewNode,pContext,m_pCurrentToken->GetType() != TT_LBRACE);

		PopBindingLevel();

		pNewNode->SetChildNode(2,pExpressionNode);   
	} 
	else 
	{
		m_pScaner->PushBack();// push the next token back
	}

	return pNewNode;
}


// m_pCurrentToken->GetSymbolTitle() == "while"

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseWhileStatement
| 描  述 : Analyse while statement in function body.
| Grammar:
| 33. AnalyseWhileStatement->`while` `(` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements
| 返 回 值 : 
| 修改记录 : 2007-4-23 17:44:39  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseWhileStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CSyntaxTreeNode *pExpressionNode = NULL;
	CExpressionTreeNode *pNewNode = CreateStatementNode( TNK_WHILE_STMT, 
		m_pCurrentToken);
	if(NULL == pNewNode)
	{
		return NULL;
	}
	pNewNode->SetParent(pParent);    //不能删除

	if( !Match(TT_LPARAN) )  // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '(' in 'while' statement."));
	}
	else 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}
	// m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	const UINT LINE_INDEX = m_pCurrentToken->GetLineIndex();  //保存表达式的行号
	pExpressionNode = AnalyseFourteenthLevelOp(pContext);
	pNewNode->SetChildNode(0,pExpressionNode);
	if( NULL == pExpressionNode )
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE,LINE_INDEX,0,
			_T("missing conditional expressions in 'if' statement.")
			, m_pCurrentToken->GetLineIndex());
	}
	else
	{
		AppendRef(pExpressionNode,pNewNode);
	}
	if(pExpressionNode->IsNumericConstants())
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, LINE_INDEX,0,
			_T("Conditional expression is constant"));
	}

	if( !Match(TT_RPARAN) )              // Match ')'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ')' in 'while' statement."));

		m_pScaner->PushBack();
	}
	CreateNewBindingLevel(pContext);
	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if(m_pCurrentToken->GetType() != TT_LBRACE)  
	{                          //说明只有一条语句
		m_pScaner->PushBack();
	}
	pExpressionNode = AnalyseCompoundStatements(pNewNode,pContext,m_pCurrentToken->GetType() != TT_LBRACE);  //Analyse statements in while statement.
	PopBindingLevel();

	pNewNode->SetChildNode(1,pExpressionNode);   //


	return pNewNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseWhileStatement
| 描  述 : Analyse while statement in function body.
| Grammar:
| 33. AnalyseWhileStatement->`while` `(` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements
| 返 回 值 : 
| 修改记录 : 2007-4-23 17:44:39  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseDoStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{   
	CExpressionTreeNode *pNewNode = CreateStatementNode( TNK_DO_STMT, 
		m_pCurrentToken);
	if(NULL == pNewNode)
	{
		return NULL;
	}

	pNewNode->SetParent(pParent);

	CreateNewBindingLevel(pContext);
	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if(m_pCurrentToken->GetType() != TT_LBRACE)  
	{                          //说明只有一条语句
		m_pScaner->PushBack();
	}
	CSyntaxTreeNode* pExpressionNode = AnalyseCompoundStatements(pNewNode,pContext,m_pCurrentToken->GetType() != TT_LBRACE); //Analyse statements in while statement.
	PopBindingLevel();

	pNewNode->SetChildNode(1,pExpressionNode);   //

	if( !Match(TT_WHILE) )              // Match 'while'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing 'while' in 'do' statement."));

		m_pScaner->PushBack();
	}
	if( !Match(TT_LPARAN) )              // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '(' in 'do' statement."));

		m_pScaner->PushBack();
	}


	// m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	const UINT LINE_INDEX = m_pCurrentToken->GetLineIndex();  //保存表达式的行号
	pExpressionNode = AnalyseFourteenthLevelOp(pContext);
	pNewNode->SetChildNode(0,pExpressionNode);
	if( NULL == pExpressionNode )
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, LINE_INDEX,0,
			_T("missing conditional expressions in 'do' statement."));
	}
	else
	{
		AppendRef(pExpressionNode,pNewNode);
	}

	if(pExpressionNode->IsNumericConstants())
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, LINE_INDEX,0,
			_T("Conditional expression is constant"));
	}

	if( !Match(TT_SEMI) ) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ';'"));

		m_pScaner->PushBack();       // error recovery
	}

	return pNewNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseSwitchStatement
| 描  述 : Analyse 'switch' statement in function body.
| Grammar:
| 33. AnalyseWhileStatement->`switch` `(` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements
| 返 回 值 : 
| 修改记录 : 2007-4-23 17:44:39  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseSwitchStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CSyntaxTreeNode *pExpressionNode = NULL;
	CExpressionTreeNode *pNewNode = CreateStatementNode( TNK_SWITCH_STMT, 
		m_pCurrentToken);
	if(NULL == pNewNode)
	{
		return NULL;
	}
	pNewNode->SetParent(pParent);

	if( !Match(TT_LPARAN) )  // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '(' in 'switch' statement."));
	}
	else 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}
	// m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	pExpressionNode = AnalyseFourteenthLevelOp(pContext);
	pNewNode->SetChildNode(0,pExpressionNode);
	if( NULL == pExpressionNode )
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing conditional expressions in 'if' statement."));
	}
	else
	{
		AppendRef(pExpressionNode,pNewNode);
	}

	if( !Match(TT_RPARAN) )              // Match ')'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ')' in 'switch' statement."));

		m_pScaner->PushBack();
	}

	CreateNewBindingLevel(pContext);
	if( !Match(TT_LBRACE) )              // Match ')'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '{' in 'switch' statement."));

		m_pScaner->PushBack();
	}
	pExpressionNode = AnalyseCompoundStatements(pNewNode,pContext);          //Analyse statements in while statement.
	PopBindingLevel();

	pNewNode->SetChildNode(1,pExpressionNode);   


	return pNewNode;
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseReturnStatement
| 描  述 : Grammar:
| 38. AnalyseReturnStatement->`case` `lable` ':' | 'default' ':'
|       没有表达式就是
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:09:26  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseCaseStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CExpressionTreeNode* pNewNode = CreateStatementNode(
		(m_pCurrentToken->GetType() == TT_CASE)? TNK_CASE_LABEL:TNK_DEFAULT_LABEL,m_pCurrentToken );
	pNewNode->SetParent(pParent);

	m_pCurrentToken = m_pScaner->GetNextToken();
	if( m_pCurrentToken->GetType() != TT_COLON )
	{
		if(pNewNode->GetNodeType() == TNK_CASE_LABEL)
		{
			CSyntaxTreeNode* pConditon = AnalyseFourteenthLevelOp(pContext);
			pNewNode->SetChildNode(0,pConditon);
			if(!pConditon->IsNumericConstants())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
					_T("case expression not constant."));
			}
			else if(pConditon->GetNodeType() != TNK_INTEGER_CST)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
					_T(" illegal type for case expression."));
			}
		}

		if( !Match(TT_COLON) ) 
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
				_T("missing ':' in 'case' statement."));

			m_pScaner->PushBack();
		}
	}

	return pNewNode;
}



/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseForStatement
| 描  述 : Grammar:
| 34. AnalyseForStatement->`for` `(` AnalyseFactor `=` AnalyseFourteenthLevelOp `;` AnalyseFourteenthLevelOp `;` AnalyseFactor `=` AnalyseFourteenthLevelOp `)` AnalyseCompoundStatements
| m_pCurrentToken->GetSymbolTitle() == "for"
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:03:34  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseForStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CSyntaxTreeNode *pExpression = NULL;

	CExpressionTreeNode* pNewNode = CreateStatementNode( TNK_FOR_STMT, m_pCurrentToken);
	if(NULL == pNewNode)
	{
		return NULL;
	}
	pNewNode->SetParent(pParent);

	if( !Match(TT_LPARAN) ) // Match '('
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing '(' in 'for' statement."));
	}
	else 
	{
		m_pCurrentToken = m_pScaner->GetNextToken();
	}

	CreateNewBindingLevel(pContext);

	// m_pCurrentToken should be AnalyseFactor or ';'
	if( m_pCurrentToken->GetType() != TT_SEMI ) //说明有变量定义例如:for(int i = 0;i <15;i++)
	{
		UINT nPrefixAttr = AnalysePrefixAtti(); 

		m_pCurrentToken = m_pScaner->GetNextToken();
		CDataTypeTreeNode* pDataTypeTreeNode = AnalyseDataType(nPrefixAttr); 
		if(NULL != pDataTypeTreeNode)
		{
			pExpression = AnalyseVariablesDeclaration(pDataTypeTreeNode,nPrefixAttr,pContext);
			pNewNode->SetChildNode(0,pExpression);

			if( NULL == pExpression)
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
					_T("syntax error."));

				ConsumeUntil( TT_SEMI );
			} 
		}
		else
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,_T("invalid type '%s'")
				,CString(m_pCurrentToken->GetSymbolTitle()));
			ConsumeUntil( TT_SEMI );
		}
	}

	m_pCurrentToken = m_pScaner->GetNextToken();     // m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	const UINT LINE_INDEX = m_pCurrentToken->GetLineIndex();  //保存表达式的行号
	pExpression = AnalyseFourteenthLevelOp(pContext);  //分析FOR循环的条件表达式
	pNewNode->SetChildNode(1,pExpression);
	if( NULL == pExpression) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, LINE_INDEX,0,
			_T("missing the second parameter in 'for' statement, ignore the whole"));
		ConsumeUntil( TT_SEMI );
	}
	else
	{
		AppendRef(pExpression,pNewNode);
	}
	if(pExpression->IsNumericConstants())
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, LINE_INDEX,0,
			_T("Conditional expression is constant"));
	}

	if( !Match(TT_SEMI) ) // Match ';'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ';' in 'for' statement."));
	}

	// m_pCurrentToken should be the pFirstNode token of AnalyseFourteenthLevelOp
	m_pCurrentToken = m_pScaner->GetNextToken();
	if(m_pCurrentToken->GetType() != TT_RPARAN)
	{
		pExpression = AnalyseFifteenthLevelOp(pContext);  
		pNewNode->SetChildNode(2,pExpression);
		if( !Match(TT_RPARAN) ) // Match ';'
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
				_T("missing ')' in 'for' statement."));
		}
	}

	
	m_pCurrentToken = m_pScaner->GetNextToken(); 
	if(m_pCurrentToken->GetType() != TT_LBRACE)  
	{                          //说明只有一条语句
		m_pScaner->PushBack();
	}
	pExpression = AnalyseCompoundStatements(pNewNode,pContext,m_pCurrentToken->GetType() != TT_LBRACE); //Analyse statements in for statement.
	PopBindingLevel();

	pNewNode->SetChildNode(3,pExpression);

	return pNewNode;
}

// Grammar:
// 35. AnalyseGotoStatement->`goto` ID `;`
// m_pCurrentToken->GetSymbolTitle() == "goto"

/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseGotoStatement
| 描  述 : 
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:06:49  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseGotoStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	if( !Match(TT_IDENTIFY_TYPE) ) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("a indentify should follow 'goto'"));

		ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
		return NULL;
	}

	CExpressionTreeNode* pNewNode = CreateStatementNode( TNK_GOTO_STMT, m_pCurrentToken);
	pNewNode->SetParent(pParent);

	CSyntaxTreeNode* pLableNode = LookupDeclNode(m_pCurrentToken->GetSymbolTitle(),TRUE);
	if(NULL != pLableNode)
	{
		pNewNode->SetChildNode(0,pLableNode);
	}
	else
	{
		pContext->AppenLableRef(pNewNode,m_pCurrentToken->GetSymbolTitle());
	}


	if( !Match(TT_SEMI) ) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ';' in 'goto' statement."));
		m_pScaner->PushBack();
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseBreakStatement
| 描  述 : Grammar:
| 36. AnalyseBreakStatement->`break` `;`
| m_pCurrentToken->GetSymbolTitle() == "break"
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:08:04  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseBreakStatement(CSyntaxTreeNode* pParent,CParserContext* /*pContext*/)
{
	CSyntaxTreeNode* pNewNode = CreateStatementNode( TNK_BREAK_STMT, m_pCurrentToken);
	pNewNode->SetParent(pParent);

	if( !Match(TT_SEMI) ) // Match ';'
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ';' in 'break' statement."));

		m_pScaner->PushBack();
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseContinueStatement
| 描  述 : Grammar:
| 37. AnalyseContinueStatement->`continue` `;`
| m_pCurrentToken->GetSymbolTitle() = "continue"
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:08:53  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseContinueStatement(CSyntaxTreeNode* pParent,CParserContext* /*pContext*/)
{
	CSyntaxTreeNode* pNewNode = CreateStatementNode( TNK_CONTINUE_STMT,m_pCurrentToken);
	pNewNode->SetParent(pParent);

	if( !Match(TT_SEMI) ) 
	{
		m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
			_T("missing ';' in 'continue' statement."));

		m_pScaner->PushBack();
	}
	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::AnalyseReturnStatement
| 描  述 : Grammar:
| 38. AnalyseReturnStatement->`return` `;` | `return` AnalyseFourteenthLevelOp `;`
| m_pCurrentToken->GetSymbolTitle() = "return"
| 返 回 值 : 
| 修改记录 : 2007-4-23 18:09:26  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode* CParser::AnalyseReturnStatement(CSyntaxTreeNode* pParent,CParserContext* pContext)
{
	CExpressionTreeNode* pNewNode = CreateStatementNode( TNK_RETURN_STMT,m_pCurrentToken );
	pNewNode->SetParent(pParent);

	m_pCurrentToken = m_pScaner->GetNextToken();
	if( m_pCurrentToken->GetType() != TT_SEMI )
	{
		CSyntaxTreeNode* pReturnTreeNode = AnalyseFourteenthLevelOp(pContext);
		if(NULL != pReturnTreeNode)
		{
			CSyntaxTreeNode* pFunDeclNode = pContext->GetFunctionDeclNode();
			CDataTypeTreeNode* pDataType = pFunDeclNode->GetDataType();
			if(TNK_VOID_TYPE == pDataType->GetNodeType())
			{
				m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
					_T( "%s: 'void' function returning a value"),CString(pFunDeclNode->GetSymbol()));
			}
			pNewNode->SetDataType(pDataType);
			AppendRef(pReturnTreeNode,pNewNode);
		}

		pNewNode->SetChildNode(0,pReturnTreeNode);
		if( !Match(TT_SEMI) ) 
		{
			m_pSaxCCompileimpl->OutputErrMsg(TRUE, m_pCurrentToken->GetLineIndex(),0,
				_T("missing ';' in 'return' statement."));

			m_pScaner->PushBack();
		}
	}

	return pNewNode;
}


/*-----------------------------------------------------------------
| 函数名称 : CParser::Trace
| 描  述 : 
| 参  数 : CSyntaxTreeNode* pSyntaxTree——
|       char* pBuffer——
|       int nBufferLen——
| 返 回 值 : 
| 修改记录 : 2007-4-24 9:18:21  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::Trace( CSyntaxTreeNode* const pSyntaxTree)
{
	ATLASSERT(NULL != pSyntaxTree );
	PrintTree(pSyntaxTree);
}

// WriteSyntaxinfo the string into the trace file, which must be existed
void CParser::WriteSyntaxinfo( char* format, ... )
{
	const UINT ERR_BUFFER_LEN      = 2048;
	BYTE szCommonBuffer[ERR_BUFFER_LEN] = {0};

	va_list Parameters;   
	va_start( Parameters, format );

	_vsnprintf_s((char*)szCommonBuffer,ERR_BUFFER_LEN
		, ERR_BUFFER_LEN
		, format
		, Parameters);

	va_end( Parameters );   

	ATLTRACE("%s",(char*)szCommonBuffer);
}

/*-----------------------------------------------------------------
| 函数名称 : CParser::PrintTree
| 描  述 : 
| 参  数 : CSyntaxTreeNode* pSyntaxTreeRoorNode——
| 返 回 值 : 
| 修改记录 : 2007-4-24 9:17:51  -huangdy-  创建
-----------------------------------------------------------------*/
void CParser::PrintTree( CSyntaxTreeNode* const pSyntaxTreeRoorNode ) 
{
	m_strIndentSpace += " ";

	CSyntaxTreeNode* pSyntaxTreeNode = pSyntaxTreeRoorNode;   

	while( pSyntaxTreeNode != NULL )
	{
		switch( pSyntaxTreeNode->GetNodeType() ) 
		{
		case TNK_COMPOUND_STMT:
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			break;

		case TNK_DECL_STMT:
			{
				CDeclarationTreeNode* pDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				while(pDeclNode)
				{
					WriteSyntaxinfo( "%sVariable: %5d,%20s,%5d\r\n"
						,m_strIndentSpace,pDeclNode->GetUID(),pDeclNode->GetSymbol(),pDeclNode->GetLineIndex());
					if(pDeclNode->GetInitial())
					{
						WriteSyntaxinfo("%sInitial: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
						PrintTree(pDeclNode->GetInitial());
						WriteSyntaxinfo( "\r\n");
					}

					pDeclNode = (CDeclarationTreeNode*)pDeclNode->GetChain();
				}
			}
			break;

		case TNK_FUNCTION_DECL:
			WriteSyntaxinfo( "%sFunction: %5d,%20s,%5d\r\n"
				,m_strIndentSpace,pSyntaxTreeNode->GetUID(),pSyntaxTreeNode->GetSymbol(),pSyntaxTreeNode->GetLineIndex());
			PrintTree(((CDeclarationTreeNode*)pSyntaxTreeNode)->GetArguments());
			PrintTree(((CDeclarationTreeNode*)pSyntaxTreeNode)->GetSavedTree());     
			break;

		case TNK_PARA_STMT:
			{
				CDeclarationTreeNode* pDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				while(pDeclNode)
				{
					WriteSyntaxinfo( "%sParameter: %5d,%20s,%5d\r\n"
						,m_strIndentSpace,pDeclNode->GetUID(),pDeclNode->GetSymbol(),pDeclNode->GetLineIndex());
					pDeclNode = (CDeclarationTreeNode*)pDeclNode->GetChain();
				}
			}
			break;

			/*case TNK_READ_STMT:
			WriteSyntaxinfo( "Call read(), args:\r\n" );
			break;

			case TNK_WRITE_STMT:
			WriteSyntaxinfo( "Call Write(), args:\r\n" );
			break;

			case TNK_PRINTF_STMT:
			WriteSyntaxinfo( "printf( \"%s\" )\r\n", pSyntaxTreeNode->GetName() );
			break;*/

		case TNK_LABEL_STMT:
			{
				CDeclarationTreeNode* pSTMTDecl = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( "%sLable:%5d,\"%s\",%d\r\n",m_strIndentSpace,pSTMTDecl->GetUID()
					,pSTMTDecl->GetSymbol(),pSTMTDecl->GetLineIndex());
			}
			break;

		case TNK_GOTO_STMT:
			{
				CDeclarationTreeNode* pSTMTDecl = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( "%sStatement: %5d,goto %s,%d\r\n",m_strIndentSpace,pSyntaxTreeNode->GetUID()
					,pSTMTDecl->GetSymbol(),pSyntaxTreeNode->GetLineIndex());   
			}
			break;

		case TNK_CALL_EXPR:
			{
				CDeclarationTreeNode* pSTMTDecl = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);

				WriteSyntaxinfo( "%sCall %s(%d)\r\n",m_strIndentSpace
					,pSTMTDecl->GetSymbol(),pSTMTDecl->GetUID());

				WriteSyntaxinfo("%sArguments: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
				CSyntaxTreeNode* pArguments = ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1);
				while(NULL != pArguments)
				{
					PrintTree(pArguments);
					pArguments = pArguments->GetChain();
					if(pArguments)
					{
						WriteSyntaxinfo( " , ");
					}             
				}
			}
			break;

		case TNK_IF_STMT:     
			WriteSyntaxinfo( "%sStatement:if,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());

			WriteSyntaxinfo("%sCondition: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");

			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			if(NULL != ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(2))
			{
				WriteSyntaxinfo("%selse: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
				PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(2));
			}
			break;

		case TNK_FOR_STMT:
			WriteSyntaxinfo( "%sStatement:for,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));

			WriteSyntaxinfo("%sCondition: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			WriteSyntaxinfo( "\r\n");

			WriteSyntaxinfo((char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(2));
			WriteSyntaxinfo( "\r\n");

			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(3));
			break;

		case TNK_WHILE_STMT:   
			WriteSyntaxinfo( "%sStatement: while,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			WriteSyntaxinfo("%sCondition: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			break;



		case TNK_DO_STMT:
			WriteSyntaxinfo( "%sStatement: do,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			WriteSyntaxinfo("%sCondition: ",(char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			break;

		case TNK_CASE_LABEL:
			{
				CSyntaxTreeNode* pValue = ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( "%sStatement: case %d:\r\n",m_strIndentSpace
					,pValue->GetValue1());
			}
			break;

		case TNK_SWITCH_STMT:
			WriteSyntaxinfo( "%sStatement: switch,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			WriteSyntaxinfo("%sCondition:",(char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");

			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			break;

		case TNK_DEFAULT_LABEL:
			WriteSyntaxinfo( "%sStatement: default,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			break;

		case TNK_BREAK_STMT:   
			WriteSyntaxinfo( "%sStatement: break,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			break;

		case TNK_CONTINUE_STMT:
			WriteSyntaxinfo( "%sStatement: continue,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());
			break;

		case TNK_RETURN_STMT:   
			WriteSyntaxinfo( "%sStatement: return,%d\r\n",m_strIndentSpace
				,pSyntaxTreeNode->GetLineIndex());   
			WriteSyntaxinfo((char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");
			break;   

		case TNK_EXPR_STMT:
			WriteSyntaxinfo((char*)(LPCSTR)(m_strIndentSpace + " "));
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "\r\n");
			break;

		case TNK_QUESTION_MARK_EXPR:
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo("? ");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
			WriteSyntaxinfo(" : ");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(2));
			break;

		case TNK_COMMA_EXPR:
			{
				UINT i = 0;
				do
				{
					PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(i++));
					if(i < MAX_CHILDREN_NUMBER && ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(i) != NULL)
					{
						WriteSyntaxinfo(" , ");
					}
				}while(i < MAX_CHILDREN_NUMBER && ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(i) != NULL);
			}
			break;

		case TNK_MODIFY_EXPR:
		case TNK_PLUS_EXPR:
		case TNK_MINUS_EXPR:
		case TNK_MULT_EXPR:
		case TNK_TRUNC_DIV_EXP:
		case TNK_TRUNC_MOD_EXPR:
		case TNK_BIT_AND_EXPR:
		case TNK_BIT_IOR_EXPR:
		case TNK_BIT_XOR_EXPR:
		case TNK_LT_EXPR:
		case TNK_GT_EXPR:
		case TNK_EQ_EXPR:
		case TNK_NEQ_EXPR:
		case TNK_PLUS_ASSIGN:
		case TNK_MINUS_ASSIGN:
		case TNK_MULT_ASSIGN:
		case TNK_DIV_ASSIGN:
		case TNK_AND_ASSIGN:
		case TNK_XOR_ASSIGN:
		case TNK_OR_ASSIGN:
		case TNK_NGT_EXPR:
		case TNK_NLT_EXPR:
		case TNK_TRUTH_AND_EXPR:
		case TNK_TRUTH_OR_EXPR:
		case TNK_LSHIFT_EXPR:
		case TNK_RSHIFT_EXPR:
			WriteSyntaxinfo( "(");

			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( " %s ",m_pScaner->GetKeywordDescription(pSyntaxTreeNode->GetTokenType()));   
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));

			WriteSyntaxinfo( ")");
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_TRUTH_NOT_EXPR:
		case TNK_BIT_NOT_EXPR:
		case TNK_PREINCREMENT_EXPR:
		case TNK_PREDECREMENT_EXPR:
		case TNK_NEGATE_EXPR:
			WriteSyntaxinfo( "(");
			WriteSyntaxinfo( "%s%s",m_strIndentSpace
				,m_pScaner->GetKeywordDescription(pSyntaxTreeNode->GetTokenType()));        
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( ")");
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_POSTDECREMENT_EXPR:
		case TNK_POSTINCREMENT_EXPR:
			WriteSyntaxinfo( "(");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			WriteSyntaxinfo( "%s",m_pScaner->GetKeywordDescription(pSyntaxTreeNode->GetTokenType()));   
			WriteSyntaxinfo( ")");
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_VECTOR_TYPE:
			{
				CSyntaxTreeNode* pEntryNode = (CVectorTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetValue();
				while(pEntryNode)
				{
					PrintTree(pEntryNode);

					pEntryNode = pEntryNode->GetChain();

					if(pEntryNode)
					{
						WriteSyntaxinfo( ",");
					}
				}
			}
			break;

		case TNK_INTEGER_CST:
			WriteSyntaxinfo( " %d ",pSyntaxTreeNode->GetValue1() );
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_REAL_CST:
			WriteSyntaxinfo( " %f ", pSyntaxTreeNode->GetValue2() );
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_STRING_CST:
			WriteSyntaxinfo( " \"%s\" ",pSyntaxTreeNode->GetValue3() );
			pSyntaxTreeNode = NULL;  //控制退出循环
			break;

		case TNK_INDIRECT_REF:
			{
				CSyntaxTreeNode* pTmpNode = ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( "*%s(%d)", pTmpNode->GetSymbol(),pTmpNode->GetUID() );
				pSyntaxTreeNode = NULL;  //控制退出循环
			}
			break;

		case TNK_ARRAY_REF:
			{
				CDeclarationTreeNode* pArrayDecl = (CDeclarationTreeNode*)((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( "%s(%d)[", pArrayDecl->GetSymbol(),pArrayDecl->GetUID());
				PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(1));
				WriteSyntaxinfo( "]");   
				pSyntaxTreeNode = NULL;  //控制退出循环
			}
			break;

		case TNK_VAR_REF:
			{
				CSyntaxTreeNode* pDeclNode = ((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0);
				WriteSyntaxinfo( " %s(%d) ",pDeclNode->GetSymbol(),pDeclNode->GetUID());
				pSyntaxTreeNode = NULL;  //控制退出循环
			}
			break;

		case TNK_ADDR_EXPR:
			WriteSyntaxinfo( "&");
			PrintTree(((CExpressionTreeNode*)pSyntaxTreeNode)->GetChildNode(0));
			break;

		case TNK_TXT_CONSTANTS_TABLE:
			break;

		default:
			WriteSyntaxinfo( "Unkown node nKind\r\n" );
		}

		if(pSyntaxTreeNode)
		{
			pSyntaxTreeNode = pSyntaxTreeNode->GetChain();
		}
	}
	m_strIndentSpace.Delete(0,1);
}



//
///*-----------------------------------------------------------------
//| 函数名称 : CParser::ReadStatement
//| 描  述 : Analyse read statement.
//| Grammar:
//| 12. `read` `(` AnalyseFactor `)` `;`
//| 返 回 值 : 
//| 修改记录 : 2007-4-23 15:58:35  -huangdy-  创建
//-----------------------------------------------------------------*/
//CSyntaxTreeNode* CParser::ReadStatement(CDeclarationTreeNode* pContext)
//{
//   CSyntaxTreeNode* pNewNode = NULL;
//   
//   if( !Match(TT_LPARAN) )            // '('
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing '('")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return NULL;
//   }
//   m_pCurrentToken = m_pScaner->GetNextToken();
//
//   if( m_pCurrentToken->GetType() != TT_IDENTIFY_TYPE )
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("\"%s\" bad arguments.")
//        , m_pCurrentToken->GetLineIndex()
//        , m_pCurrentToken->GetSymbolTitle());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return NULL;
//   }
//
//   pNewNode = CreateStatementNode( TNK_READ_STMT, "read" );
//
//   pNewNode->SetChildNode(0, AnalyseFactor(pContext)); 
//   if( pNewNode->GetChildNode(0) != NULL ) 
//   {
//     pNewNode->GetChildNode(0)->SetFatherNode(pNewNode);
//   }
//
//   if( !Match(TT_RPARAN) )         // ')'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ')'")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return pNewNode;
//   }
//
//   if( !Match(TT_SEMI) )          // ';'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ';'")
//        , m_pCurrentToken->GetLineIndex());
//
//     m_pScaner->PushBack();        // error recovery
//   }
//
//   return pNewNode;
//}
//
///*-----------------------------------------------------------------
//| 函数名称 : CParser::WriteStatement
//| 描  述 : Analyse write statement.
//| Grammar:
//| 13. `write` `(` AnalyseFourteenthLevelOp `)` `;`
//| 返 回 值 : 
//| 修改记录 : 2007-4-23 16:06:12  -huangdy-  创建
//-----------------------------------------------------------------*/
//CSyntaxTreeNode* CParser::WriteStatement()
//{
//   CSyntaxTreeNode* pNewNode = NULL;
//   
//   if( !Match(TT_LPARAN) )        // '('
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing '('")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return NULL;
//   }
//   pNewNode = CreateStatementNode( TNK_WRITE_STMT, "write" );
//   m_pCurrentToken = m_pScaner->GetNextToken();
//
//   // m_pCurrentToken contains the pFirstNode token of AnalyseFourteenthLevelOp
//
//  CSyntaxTreeNode* pChildNode = AnalyseFourteenthLevelOp();
//   if(NULL != pChildNode)
//   {
//     pChildNode->SetFatherNode(pNewNode);
//     pNewNode->SetChildNode(0,pChildNode);
//   }   
//
//   if( !Match(TT_RPARAN) )    // ')'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ')'")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return pNewNode;
//   }
//   if( !Match(TT_SEMI) )         // ';'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ';'")
//        , m_pCurrentToken->GetLineIndex());
//
//     m_pScaner->PushBack();       // error recovery
//   }
//
//   return pNewNode;
//}
//
///*-----------------------------------------------------------------
//| 函数名称 : CParser::PrintfStatement
//| 描  述 : Analyse printf statement.
//| Grammar:
//| 14. `printf` `(` `"` STRING `"` `)` `;`
//| 返 回 值 : 
//| 修改记录 : 2007-4-23 16:12:58  -huangdy-  创建
//-----------------------------------------------------------------*/
//CSyntaxTreeNode* CParser::PrintfStatement()
//{
//   CSyntaxTreeNode* pNewNode = NULL;
//   
//   if( !Match(TT_LPARAN) )    // '('
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing '('")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return NULL;
//   }
//
//   m_pCurrentToken = m_pScaner->GetNextToken();
//   if( m_pCurrentToken->GetType() != TT_STRING_TYPE )
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("arguments should be strings.")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return NULL;
//   }
//
//   pNewNode = CreateStatementNode( SK_PRINTF, m_pCurrentToken->GetSymbolTitle());
//
//   if( !Match(TT_RPARAN) )    // ')'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ')'")
//        , m_pCurrentToken->GetLineIndex());
//
//     ConsumeUntil( TT_SEMI, TT_RBRACE );// error recovery
//     return pNewNode;
//   }
//
//   if( !Match(TT_SEMI) )         // ';'
//   {
//     m_pSaxCCompileimpl->OutputErrMsg(_T("syntax error, missing ';'")
//        , m_pCurrentToken->GetLineIndex());
//
//     m_pScaner->PushBack();       // error recovery
//   }
//
//   return pNewNode;
//}

