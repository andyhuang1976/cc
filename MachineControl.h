/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/MachineControl.h,v 1.4 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
/*
R-It indicate register
M-It indicate memory
IMM-It indicate immediately data
*/
enum INSTRUCTION_CODE // Instructions of virtual machine 
{ 
   IC_PUSH_R8 = 1,     
   IC_PUSH_R16,
   IC_PUSH_R32,

   IC_POP_R8,
   IC_POP_R16,
   IC_POP_R32,

   IC_MOV_RM8,
   IC_MOV_RM16,
   IC_MOV_RM32,
   IC_MOV_MR8,
   IC_MOV_MR16,
   IC_MOV_MR32,
   IC_MOV_RIMM,
   IC_MOV_R2,

   IC_ADD_RM8,
   IC_ADD_RM16,
   IC_ADD_RM32,
   IC_ADD_RIMM,
   IC_ADD_R2,

   IC_SUB_RM8,
   IC_SUB_RM16,
   IC_SUB_RM32,
   IC_SUB_RIMM,
   IC_SUB_R2,

   IC_MUL_RM8,
   IC_MUL_RM16,
   IC_MUL_RM32,
   IC_MUL_RIMM,
   IC_MUL_R2,

   IC_DIV_RM8,
   IC_DIV_RM16,
   IC_DIV_RM32,
   IC_DIV_RIMM,
   IC_DIV_R2,

   IC_DEC_M,
   IC_DEC_R,

   IC_INC_M,
   IC_INC_R,

   IC_CMP_RM8,
   IC_CMP_RM16,
   IC_CMP_RM32,
   IC_CMP_MR8,
   IC_CMP_MR16,
   IC_CMP_MR32,
   IC_CMP_RIMM,
   IC_CMP_R2,

   IC_JMP_IMM,          //
   IC_JMP_R,
   IC_JA,
   IC_JAE,
   IC_JB,
   IC_JE,
   IC_JNE,
   IC_JLE,
   IC_JGE,

   IC_NOT,             //store result in EAX
   IC_AND,             //store result in EAX
   IC_OR,             //store result in EAX

   IC_EQ,             //store result in EAX
   IC_NEQ,             //store result in EAX
   IC_LT,             //store result in EAX
   IC_NGT,             //store result in EAX
   IC_NLT,             //store result in EAX
   IC_GT,             //store result in EAX

   IC_MOD,
  IC_BIT_AND,
   IC_BIT_OR,
   IC_BIT_XOR,

   IC_LEA,
   IC_CALL,
   IC_RET, 
   IC_XCHG_R2,
   IC_BAD 
};

//反序标记，用于标识指令中寄存器的顺序，正常情况序号小的在左边，比如IC_MOV_R2 RI_EAX,RI_EBX
const DWORD REVERSE_FLAG = 0x10000000; 
enum REGISTER_IDX
{
   RI_EAX = 0x00000100,
   RI_EBX = 0x00000200,
   RI_ECX = 0x00000400,
   RI_EDX = 0x00000800,
   RI_ESP = 0x00001000,
   RI_EBP = 0x00002000,
   RI_ESI = 0x00004000,
   RI_EDI = 0x00008000,
   RI_EFG = 0x00010000,    //flag register
   RI_EFP = 0x00020000,    //frame pointer
   RI_EIP = 0x00040000,    //IP pointer
   RI_EDS = 0x00080000,    //data segment pointer
   RI_NUL = 0x00000000,
};


#pragma pack(push,r1,1)  
typedef struct tagSaxProgramHead
{
   WORD     nIdentify;       //Sax program identify"sp"
   DWORD    nCodeSize;       //Sax program code size ,in byte
   DWORD    nStackSize;      //Request size of heap and stack to allocated by virtual machine
  DWORD    nIP;          //IP register's initialization offset value
   DWORD    nCS;          //Code segment offset
   WORD     nProgramHead;
}SAX_PROGRAM_HEAD;


const UINT SAX_PROGRAM_HEAD_SIZE = sizeof(SAX_PROGRAM_HEAD);

typedef struct
{
   DWORD       nCode;
   int        nArg;
}INSTRUCTION;

const int INSTRUCTION_SIZE = sizeof(INSTRUCTION);
#pragma pack(pop,r1)

extern void UnloadProgramVM(BYTE* pProgram);
extern BYTE* LoadProgramVM(BYTE* pProgram,bool bDisableStack);
extern LONGLONG  ExecuteProgramVM(BYTE* pExecuteProgram);
