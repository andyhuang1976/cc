/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SaxCCompileimpl.cpp,v 1.14 2015/08/17 01:05:56 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/SaxCCompileimpl.cpp,v 1.14 2015/08/17 01:05:56 administrator Exp $";
// $Nokeywords: $
#include "StdAfx.h"
#include ".\saxccompileimpl.h"
#include ".\scaner.h"
#include "parser.h"
//#include "AsmCodeGenerator.h"
#include ".\generatesaxprogram.h"


CSaxCCompileimpl::CSaxCCompileimpl(void)
	: m_pScaner(NULL)
	, m_pParser(NULL)
	, m_pSaxProgramGenerater(NULL)
	//, m_pAsmCodeGenerator(NULL)
	, m_nLocation(0)
	, m_pOutBuffer(NULL)
	, m_nBufferLen(0)
	, m_nPos(0)
	, m_nErrors(0)
	, m_nWarning(0)
{
}

CSaxCCompileimpl::~CSaxCCompileimpl(void)
{
	if(NULL != m_pScaner)
	{
		delete m_pScaner;
	}

	if(NULL != m_pParser)
	{
		delete m_pParser;
	}
	if(NULL != m_pSaxProgramGenerater)
	{
		delete m_pSaxProgramGenerater;
	}
	//if(NULL != m_pAsmCodeGenerator)
	//{
	//   delete m_pAsmCodeGenerator;
	//}

	if(NULL != m_pOutBuffer)
	{
		delete []m_pOutBuffer;
	}

	ERRORINFO_ARRAY_ITR itr = m_arrErrMsgList.begin();
	while(m_arrErrMsgList.end() != itr)
	{
		delete (*itr);
		itr++;
	}
	m_arrErrMsgList.clear();

}

void CSaxCCompileimpl::Reset()
{
	m_arrErrMsgList.clear();

	if(NULL != m_pScaner)
	{
		m_pScaner->Reset();
	}

	if(NULL != m_pParser)
	{
		m_pParser->Reset();
	}

	//if(NULL != m_pAsmCodeGenerator)
	//{
	//   m_pAsmCodeGenerator->Reset();
	//}

	if(NULL != m_pSaxProgramGenerater)
	{
		m_pSaxProgramGenerater->Reset();
	}

	ERRORINFO_ARRAY_ITR itr = m_arrErrMsgList.begin();
	while(m_arrErrMsgList.end() != itr)
	{
		delete (*itr);
		itr++;
	}
	m_arrErrMsgList.clear();

	m_nPos   = 0;
	m_nErrors = 0;
	m_nWarning = 0;
}

const UINT ERR_BUFFER_LEN      = 2048;
BYTE szCommonBuffer[ERR_BUFFER_LEN] = {0};

void CSaxCCompileimpl::OutputErrMsg(BOOL nIsError,UINT nLineIndex,UINT nCode,TCHAR* format, ... )
{
	va_list params;

	va_start( params, format );
	_vsntprintf_s((TCHAR*)szCommonBuffer,ERR_BUFFER_LEN/2, ERR_BUFFER_LEN/2, format, params );
	va_end( params );

	m_arrErrMsgList.push_back(new CErrorInfo(nIsError,nLineIndex,nCode,(TCHAR*)szCommonBuffer));

	if(nIsError)
	{
		m_nErrors++;
	}
	else
	{
		m_nWarning++;
	}

#ifdef _DEBUG
	CString strErr((TCHAR*)szCommonBuffer);
	ATLTRACE(_T("%s in line %d: %s\r\n"),(nIsError)? _T("Error"):_T("warning"),nLineIndex,strErr);
#endif
}

/*-----------------------------------------------------------------
| 函数名称 : CSaxCCompileimpl::Compile
| 描  述 : 
| 参  数 : char* pSourceCode——
|       char* pErrorBuffer——
|       int nBufferLen——
| 返 回 值 : TRUE----
|       FALSE---
| 修改记录 : 2007-4-22 16:40:42  -huangdy-  创建
-----------------------------------------------------------------*/
CSyntaxTreeNode * CSaxCCompileimpl::Compile(char* const pSourceCode,int nLen,UINT nMode)
{
	if(NULL == m_pScaner)
	{
		m_pScaner = new CScaner(this);
	}
	if(NULL == m_pParser)
	{
		m_pParser = new CParser(this,nMode);
		m_pParser->SetScaner(m_pScaner);
	}
	ATLASSERT(m_pScaner);
	ATLASSERT(m_pParser);

	CSyntaxTreeNode *pSyntaxTree = m_pParser->BuildSyntaxTree(pSourceCode,nLen);

	if(NULL != pSyntaxTree && GetErrors() == 0)
	{
#ifdef DEBUG_SHOW_HELP_INFO
		m_pParser->Trace(pSyntaxTree);
#endif

	}

	return pSyntaxTree;
}


//bool CSaxCCompileimpl::OutputCode( const char *format,va_list argptr )
//{
//  int nSize = _vsnprintf_s( (char*)(m_pOutBuffer + m_nPos),m_nBufferLen - m_nPos,_TRUNCATE, format, argptr );
//  if(-1 == nSize)
//  {
//     BYTE* pNewBuffer = new BYTE[m_nBufferLen + 1024];
//     ATLASSERT(NULL != pNewBuffer);
//     if(NULL == pNewBuffer)
//     {
//       return FALSE;
//     }
//     memcpy(pNewBuffer,m_pOutBuffer,m_nPos);
//     m_nBufferLen += 1024;

//     delete []m_pOutBuffer;
//     m_pOutBuffer = pNewBuffer;

//     if(-1 == _vsnprintf_s( (char*)(m_pOutBuffer + m_nPos),m_nBufferLen - m_nPos,_TRUNCATE, format, argptr ))
//     {
//       return FALSE;
//     }
//  }

//  return true;
//}

//bool CSaxCCompileimpl::OutputCode(BYTE* pData,int nLen)
//{
//  ATLASSERT(NULL != pData);
//  ATLASSERT(nLen > 0);

//  if(nLen > (m_nBufferLen - m_nPos))  //说明缓冲区小，需要重新分配
//  {
//     BYTE* pNewBuffer = new BYTE[m_nBufferLen + 1024];
//     ATLASSERT(NULL != pNewBuffer);
//     if(NULL == pNewBuffer)
//     {
//       return FALSE;
//     }
//     memcpy(pNewBuffer,m_pOutBuffer,m_nPos);
//     m_nBufferLen += 1024;

//     delete []m_pOutBuffer;
//     m_pOutBuffer = pNewBuffer;
//  }

//   memcpy(m_pOutBuffer + m_nPos,pData,nLen);

//   return TRUE;
//}

/*-----------------------------------------------------------------
| 函数名称 : CSaxCCompileimpl::CompileAsSaxProgram
| 描  述 : 
| 参  数 : char* pSourceCode——
|       char* pErrorBuffer——
|       int nErrBufferLen——
| 返 回 值 : 
| 修改记录 : 2007-4-29 11:11:23  -huangdy-  创建
-----------------------------------------------------------------*/
BYTE* CSaxCCompileimpl::CompileAsSaxProgram(char* pSourceCode
	,int nLen
	, char* lpEntryFun
	,int nStackSize
	,AX3_COMPILE_MODE nMode)
{
	BYTE *pSaxProgram = NULL;

	try
	{
		if(NULL == m_pSaxProgramGenerater)
		{
			m_pSaxProgramGenerater = new CGenerateSaxProgram(this);
		}
		ATLASSERT(NULL != m_pSaxProgramGenerater);

		Reset();

		if(NULL != m_pSaxProgramGenerater)
		{
			CSyntaxTreeNode * pSyntaxTree = Compile(pSourceCode,nLen,nMode);
			if(NULL != pSyntaxTree && m_nErrors <= 0)
			{
				m_pSaxProgramGenerater->SetEnableMasm(TRUE);
				pSaxProgram = m_pSaxProgramGenerater->GenerateProgram(pSyntaxTree,lpEntryFun,nStackSize,nMode);          
			}
		}
	}
	catch(CParserException& objParserException) 
	{
		OutputErrMsg(TRUE,0,0,_T("error: %s"),objParserException.what());
	}

	m_pParser->Reset();

	return pSaxProgram;
}
/*-----------------------------------------------------------------
| 函数名称 : CSaxCCompileimpl::CompileAsAsmCode
| 描  述 : 
| 参  数 : char* pSourceCode——
|       int nLen——
| 返 回 值 : TRUE----
|       FALSE---
| 修改记录 : 2007-4-27 14:29:11  -huangdy-  创建
-----------------------------------------------------------------*/
bool CSaxCCompileimpl::CompileAsAsmCode(char * const /*pSourceCode*/,int /*nLen*/,AX3_COMPILE_MODE /*nMode*/)
{
	bool bResult = false;

	/*Reset();

	try
	{
	if(NULL == m_pOutBuffer)
	{
	m_nBufferLen = ((nLen / 1024) + 1) * 1024;
	m_pOutBuffer = new BYTE[m_nBufferLen];     
	}
	ATLASSERT(NULL != m_pOutBuffer);

	if(NULL == m_pAsmCodeGenerator)
	{
	m_pAsmCodeGenerator = new CAsmCodeGenerator(this);
	}
	ATLASSERT(NULL != m_pAsmCodeGenerator);

	CSyntaxTreeNode * pSyntaxTree = Compile(pSourceCode,nLen,nMode);
	if(NULL != pSyntaxTree && m_arrErrMsgList.size() == 0)
	{
	m_pAsmCodeGenerator->GenerateAsmCode(pSyntaxTree);
	bResult = true;
	}
	}
	catch(CParserException& objParserException) 
	{
	return NULL;
	}*/

	return bResult;
}

void CSaxCCompileimpl::UnloadProgram(BYTE* pProgram)
{
	if(NULL != pProgram)
	{
		//AX3_PROGRAM_HEADER* pProgramHeader = (AX3_PROGRAM_HEADER*)pProgram;
		VirtualFree(pProgram,0,MEM_RELEASE);
	}
}
BYTE* CSaxCCompileimpl::LoadProgram(BYTE* pProgram,bool /*bDisableStack*/)
{
	BYTE* pResult = NULL;
	ATLASSERT(NULL != pProgram);

	if(NULL != pProgram)
	{
		AX3_PROGRAM_HEADER* pProgramHeader = (AX3_PROGRAM_HEADER*)pProgram;
		if(MAGIC_NUMBER == pProgramHeader->nMagic)
		{
			pResult = (BYTE*)VirtualAlloc(0,pProgramHeader->nSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE); 
			if(pResult)
			{
				if(memcpy(pResult,pProgram,pProgramHeader->nSize) != NULL)
				{
					((AX3_PROGRAM_HEADER*)pResult)->nEntryPoint += (UINT)pResult;

					DWORD* pRePosTable = (DWORD*)(pResult + sizeof(AX3_PROGRAM_HEADER) + pProgramHeader->nGlobleDataSize
						+ pProgramHeader->nCodeSize);
					for(UINT i = 0;i < pProgramHeader->nRePostionEntries;i++)
					{
						if(pProgramHeader->nMachine == ACM_MODE_X86)
						{
							*(DWORD*)(pResult + pRePosTable[i]) += (DWORD)pResult;
						}
						else
						{
							*(LONGLONG*)(pResult + pRePosTable[i]) += (UINT)pResult;
						}
					}				
				}
				else
				{
					OutputErrMsg(TRUE,0,0,_T("Fail to load program!"));
				}
			}
		}
		else
		{
			OutputErrMsg(TRUE,0,0,_T("Unrecognized format"));
		}
	}

	return pResult;
}
LONGLONG CSaxCCompileimpl::ExecuteProgram(BYTE* pExecuteProgram)
{
	return ExecuteProgramVM(pExecuteProgram);
}

CErrorInfoBase* CSaxCCompileimpl::GetErrorMessage(UINT nIndex)
{
	if(nIndex < m_arrErrMsgList.size())
	{
		return m_arrErrMsgList[nIndex];
	}

	return NULL;
}

