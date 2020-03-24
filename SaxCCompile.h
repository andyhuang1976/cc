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
   DWORD   nMagic;       //识别码"Ax3" 0x41783300
   WORD   nMachine;      //1:32位,2:64位
   DWORD   nSize;        //整个程序的尺寸
   DWORD   nGlobleDataSize;   //全局变量的尺寸
   DWORD   nCodeSize;      //所有代码尺寸
   WORD   nFunctions;     //函数的个数
   WORD   nFunctionsTableSize; //函数及便宜地址对照表
   DWORD   nRePostionEntries;  //程序中需要重新定位的项目(主要指应用全局变量和函数的地方),是个一维数组,每个项目占4个字节
   ULONGLONG nEntryPoint;     //指定入口函数的偏移地址,否认是第一个函数
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