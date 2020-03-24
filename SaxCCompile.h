/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SaxCCompile.h,v 1.7 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once

#define DllExport  __declspec( dllexport )

enum AX3_COMPILE_MODE
{
   ACM_MODE_X86 = 1,
   ACM_MODE_X64
};

const UINT MAGIC_NUMBER = 0x41783300;

typedef struct tagAx3ProgramHeader
{
   DWORD   nMagic;       //ʶ����"Ax3" 0x41783300
   WORD   nMachine;      //1:32λ,2:64λ
   DWORD   nSize;        //��������ĳߴ�
   DWORD   nGlobleDataSize;   //ȫ�ֱ����ĳߴ�
   DWORD   nCodeSize;      //���д���ߴ�
   WORD   nFunctions;     //�����ĸ���
   WORD   nFunctionsTableSize; //���������˵�ַ���ձ�
   DWORD   nRePostionEntries;  //��������Ҫ���¶�λ����Ŀ(��ҪָӦ��ȫ�ֱ����ͺ����ĵط�),�Ǹ�һά����,ÿ����Ŀռ4���ֽ�
   ULONGLONG nEntryPoint;     //ָ����ں�����ƫ�Ƶ�ַ,�����ǵ�һ������
}AX3_PROGRAM_HEADER;

class DllExport CErrorInfoBase
{
public:
   CErrorInfoBase() {}
public:
  virtual BOOL GetIsError() = 0;
  virtual UINT GetLineIndex() = 0;
  virtual UINT GetCode() = 0;
  virtual LPCTSTR GetMessage() = 0;
};

class DllExport CSaxCCompile
{
public:
   CSaxCCompile(void) {}
   virtual ~CSaxCCompile(void){}
public:
   static CSaxCCompile* CreateSaxCCompile();
   static void DestroySaxCCompile(CSaxCCompile* pSaxCCompile);
   
public:
   virtual bool CompileAsAsmCode(char * const pSourceCode,int nLen,AX3_COMPILE_MODE nMode = ACM_MODE_X86) = 0;
   virtual BYTE* CompileAsSaxProgram(char* pSourceCode,int nLen,char* lpEntryFun,int nStackSize,AX3_COMPILE_MODE nMode = ACM_MODE_X86) = 0;
   virtual void UnloadProgram(BYTE* pProgram) = 0;
   virtual BYTE* LoadProgram(BYTE* pProgram,bool bDisableStack) = 0;
   virtual LONGLONG ExecuteProgram(BYTE* pExecuteProgram) = 0;
   virtual UINT GetErrorCount() = 0;
   virtual CErrorInfoBase* GetErrorMessage(UINT nIndex) = 0;
};