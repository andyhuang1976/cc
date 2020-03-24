/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SaxCCompileimpl.h,v 1.7 2014/07/02 08:22:22 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include ".\saxccompile.h"
#include "FunctionArgsCheck.h"
#include <vector>

class CSyntaxTreeNode;
class CScaner;
class CParser;
class CGenerateSaxProgram;
class CAsmCodeGenerator;

#define DEBUG_SHOW_HELP_INFO

typedef std::vector<CString>        CSTRINGARRAY;

class CErrorInfo : public CErrorInfoBase
{
public:
	CErrorInfo(BOOL nIsError,UINT nLineIndex,UINT nCode,LPCTSTR lpMsg)
		: m_bIsError(nIsError)
		, m_nLineIndex(nLineIndex)
		, m_nCode(nCode)
		, m_strMessage(lpMsg)
	{
	}
public:
	BOOL GetIsError()          { return m_bIsError;}
	UINT GetLineIndex()        { return m_nLineIndex;}
	UINT GetCode()           { return m_nCode;}
	LPCTSTR GetMessage()        { return m_strMessage;}
public:
	DECLARE_MEMBER_AND_METHOD(BOOL,m_bIsError,IsError);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nLineIndex,LineIndex);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nCode,Code);
	DECLARE_MEMBER_AND_METHOD(CString,m_strMessage,Message);
};


typedef std::vector<CErrorInfo*>       ERRORINFO_ARRAY;
typedef std::vector<CErrorInfo*>::iterator  ERRORINFO_ARRAY_ITR;

class CSaxCCompileimpl:
	public CSaxCCompile
{
	friend class CScaner;
	friend class CParser;
	friend class CAsmCodeGenerator;
	friend class CGenerateSaxProgram;
public:
	CSaxCCompileimpl(void);
	virtual ~CSaxCCompileimpl(void);
public:
	virtual bool CompileAsAsmCode(char * const pSourceCode,int nLen,AX3_COMPILE_MODE nMode = ACM_MODE_X86);
	virtual BYTE* CompileAsSaxProgram(char* pSourceCode,int nLen,char* lpEntryFun,int nStackSize,AX3_COMPILE_MODE nMode = ACM_MODE_X86);
	virtual void UnloadProgram(BYTE* pProgram);
	virtual BYTE* LoadProgram(BYTE* pProgram,bool bDisableStack);
	virtual LONGLONG ExecuteProgram(BYTE* pExecuteProgram);
	virtual UINT GetErrorCount()               { return m_arrErrMsgList.size();}
	virtual CErrorInfoBase* GetErrorMessage(UINT nIndex);
public:
	void OutputErrMsg(BOOL nIsError,UINT nLineIndex,UINT nCode,TCHAR* format, ... );
protected:
	CSyntaxTreeNode* Compile(char* const pSourceCode,int nLen,UINT nMode);
	void Reset();
	//bool OutputCode(BYTE* pData,int nLen);
	//bool OutputCode( const char *format,va_list argptr );
private:
	DECLARE_MEMBER_AND_METHOD(UINT,m_nErrors,Errors);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nWarning,Warning);

	CScaner*        m_pScaner;
	CParser*        m_pParser;
	CGenerateSaxProgram*  m_pSaxProgramGenerater;
	// CAsmCodeGenerator*   m_pAsmCodeGenerator;
	//CFunctionArgsCheck     m_FunArgs;
	int             m_nLocation;
	ERRORINFO_ARRAY     m_arrErrMsgList;

	BYTE*          m_pOutBuffer;  
	int           m_nBufferLen;
	int           m_nPos;
};
