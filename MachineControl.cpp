/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/MachineControl.cpp,v 1.4 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/MachineControl.cpp,v 1.4 2014/06/29 11:54:35 administrator Exp $";
#include "StdAfx.h"
// $Nokeywords: $

#include ".\machinecontrol.h"


/*-----------------------------------------------------------------
| 函数名称 : UnloadProgram
| 描  述 : 
| 参  数 : BYTE* pProgram——
| 返 回 值 : 
| 修改记录 : 2007-5-3 18:08:14  -huangdy-  创建
-----------------------------------------------------------------*/
void UnloadProgramVM(BYTE* pProgram)
{
   if(NULL != pProgram)
   {
     delete []pProgram;
   }
}

/*-----------------------------------------------------------------
| 函数名称 : LoadProgram
| 描  述 : Load program into memory
| 参  数 : BYTE* pProgram——
|       bool bDisableStack——
| 修改记录 : 2007-5-3 18:10:38  -huangdy-  创建
-----------------------------------------------------------------*/
BYTE* LoadProgramVM(BYTE* pProgram,bool bDisableStack)
{
   if(NULL == pProgram)
   {
     return NULL;
   }
   SAX_PROGRAM_HEAD* pProgramHead = (SAX_PROGRAM_HEAD*)pProgram;
   if(pProgramHead->nIdentify != 0x5053)
   {
     return NULL;
   }

   int nExecuteProgram = pProgramHead->nCS + pProgramHead->nCodeSize + 
     ((bDisableStack)? pProgramHead->nStackSize:0);
   BYTE* pExecuteProgram = new BYTE[nExecuteProgram];
   if(NULL != pExecuteProgram)
   {     
     memcpy(pExecuteProgram,pProgram,SAX_PROGRAM_HEAD_SIZE);
     ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nIdentify = 0x6988;
     ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nStackSize = 
        (bDisableStack)? pProgramHead->nStackSize:0;

     memset(pExecuteProgram + SAX_PROGRAM_HEAD_SIZE,0,pProgramHead->nCS - SAX_PROGRAM_HEAD_SIZE);

     memcpy(pExecuteProgram + pProgramHead->nCS,
        pProgram + SAX_PROGRAM_HEAD_SIZE, pProgramHead->nCodeSize);
   }

  return pExecuteProgram;
}


/*-----------------------------------------------------------------
| 函数名称 : ExecuteProgram
| 描  述 : 
| 参  数 : BYTE* pProgram——
| 返 回 值 : 
| 修改记录 : 2007-5-3 19:53:24  -huangdy-  创建
-----------------------------------------------------------------*/
LONGLONG  ExecuteProgramVM(BYTE* pExecuteProgram)
{
   if(NULL == pExecuteProgram || ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nIdentify != 0x6988)
   {
     return 0;
   }
  
   register LONGLONG _EAX = 0;
   register LONGLONG _EBX = 0;
   LONGLONG _ECX = 0;
   //LONGLONG _EDX = 0;
   //LONGLONG _EBP = 0;
   LONGLONG _ESI = 0;
   //LONGLONG _EDI = 0;
   //LONGLONG _EFG = 0; 

   LONGLONG _ECS = (LONGLONG)pExecuteProgram + ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nCS;
   LONGLONG _EIP = ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nIP;
   LONGLONG _EDS = (LONGLONG)(pExecuteProgram) + SAX_PROGRAM_HEAD_SIZE;   
   LONGLONG _ESP = (LONGLONG)pExecuteProgram + ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nCS + 
     ((SAX_PROGRAM_HEAD*)pExecuteProgram)->nCodeSize;
   LONGLONG _EFP = _ESP;

  INSTRUCTION* pCurrentInstruction = NULL;
   LONGLONG   nTmp = 0;
  do
   {
     pCurrentInstruction = (INSTRUCTION*)(_ECS + _EIP);
     switch(pCurrentInstruction->nCode)
     {
     case IC_PUSH_R8|RI_EAX: 
        *((BYTE*)_ESP) = (BYTE)_EAX;
        _ESP += 1;
        break;
     case IC_PUSH_R16|RI_EAX:
        *((WORD*)_ESP) = (WORD)_EAX;
        _ESP += 2;
        break;
     case IC_PUSH_R32|RI_EFP:
        *((DWORD*)_ESP) = (DWORD)_EFP;
        _ESP += 4;
        break;
     case IC_PUSH_R32|RI_EAX:
        *((DWORD*)_ESP) = (DWORD)_EAX;
        _ESP += 4;
        break;
     case IC_POP_R8|RI_ECX:
        _ESP -= 1;
        _ECX = *((BYTE*)_ESP);        
        break;
     case IC_POP_R16|RI_ECX:
        _ESP -= 2;
        _ECX = *((WORD*)_ESP);
        break;
     case IC_POP_R32|RI_EFP:
        _ESP -= 4;
        _EFP = *((DWORD*)_ESP);
        break;
     case IC_POP_R32|RI_EIP:
        _ESP -= 4;
        _EIP = *((DWORD*)_ESP);
        break;

     case IC_POP_R32|RI_ECX:
        _ESP -= 4;
        _ECX = *((DWORD*)_ESP);
        break;
     case IC_POP_R32|RI_EBX:
        _ESP -= 4;
        _EBX = *((DWORD*)_ESP);
        break;

     case IC_MOV_RM8|RI_EAX|RI_ECX:
        _EAX = *((BYTE*)_ECX);
        break;

     case IC_MOV_RM16|RI_EAX|RI_ECX:
        _EAX = *((WORD*)_ECX);
        break;

     case IC_MOV_RM32|RI_EAX|RI_ECX:
        _EAX = *((DWORD*)_ECX);
        break;

     case IC_MOV_RM32|RI_EBX|RI_ECX:
        _EBX = *((DWORD*)_ECX);
        break;

     case IC_MOV_MR8|RI_ECX|RI_EAX|REVERSE_FLAG:
        *((BYTE*)_ECX) = (BYTE)_EAX;
        break;
     case IC_MOV_MR16|RI_ECX|RI_EAX|REVERSE_FLAG:
        *((WORD*)_ECX) = (WORD)_EAX;
        break;
     case IC_MOV_MR32|RI_ECX|RI_EAX|REVERSE_FLAG:
        *((DWORD*)_ECX) = (DWORD)_EAX;
        break;

     case IC_MOV_RIMM|RI_EAX:
        _EAX = pCurrentInstruction->nArg;
        break;
     case IC_MOV_R2|RI_EFP|RI_ESP|REVERSE_FLAG:
        _EFP = _ESP;
        break;

     case IC_MOV_R2|RI_ESP|RI_EFP:
        _ESP = _EFP;
        break;

     case IC_MOV_R2|RI_EAX|RI_ECX:
        _EAX = _ECX;
        break;

     case IC_MOV_R2|RI_ESI|RI_EAX|REVERSE_FLAG:
        _ESI = _EAX;
        break;
     case IC_MOV_R2|RI_EBX|RI_EAX|REVERSE_FLAG:
        _EBX = _EAX;
        break;     

     case IC_ADD_RM8|RI_EAX|RI_EBX:
        _EAX += *((BYTE*)_EBX);
        break;
     case IC_ADD_RM16|RI_EAX|RI_EBX:
        _EAX += *((WORD*)_EBX);
        break;
     case IC_ADD_RM32|RI_EAX|RI_EBX:
        _EAX += *((DWORD*)_EBX);
        break;
     case IC_ADD_RIMM|RI_EAX:
        _EAX += pCurrentInstruction->nArg;
        break;
     case IC_ADD_R2|RI_ECX|RI_ESI:
        _ECX += _ESI;
        break;
     case IC_ADD_R2|RI_EAX|RI_EBX:
        _EAX += _EBX;
        break;

     case IC_SUB_RM8|RI_EAX|RI_EBX:
        _EAX -= *((BYTE*)_EBX);
        break;
     case IC_SUB_RM16|RI_EAX|RI_EBX:
        _EAX -= *((WORD*)_EBX);
        break;
     case IC_SUB_RM32|RI_EAX|RI_EBX:
        _EAX -= *((DWORD*)_EBX);
        break;
     case IC_SUB_RIMM|RI_EAX:
        _EAX -= pCurrentInstruction->nArg;
        break;
     case IC_SUB_R2|RI_EAX|RI_EBX:
        _EAX -= _EBX;
        break;
        
     case IC_MUL_RM8|RI_EAX|RI_EBX:
        _EAX *= *((BYTE*)_EBX);
        break;
     case IC_MUL_RM16|RI_EAX|RI_EBX:
        _EAX *= *((WORD*)_EBX);
        break;
     case IC_MUL_RM32|RI_EAX|RI_EBX:
        _EAX *= *((DWORD*)_EBX);
        break;
     case IC_MUL_RIMM|RI_EAX:
        _EAX *= pCurrentInstruction->nArg;
        break;
     case IC_MUL_RIMM|RI_ESI:
        _ESI *= pCurrentInstruction->nArg;
        break;
     case IC_MUL_R2|RI_EAX|RI_EBX:
        _EAX *= _EBX;
        break;

     case IC_DIV_RM8|RI_EAX|RI_EBX:
        if(*((BYTE*)_EBX) != 0)
        {
          _EAX /= *((BYTE*)_EBX);
        }
        else
        {
          return 0;
        }
        break;
     case IC_DIV_RM16|RI_EAX|RI_EBX:
        if(*((WORD*)_EBX) != 0)
        {
          _EAX /= *((WORD*)_EBX);
        }
        else
        {
          return 0;
        }
        break;
     case IC_DIV_RM32|RI_EAX|RI_EBX:
        if(*((DWORD*)_EBX) != 0)
        {
          _EAX /= *((DWORD*)_EBX);
        }
        else
        {
          return 0;
        }
        break;
     case IC_DIV_RIMM|RI_EAX:
        if(pCurrentInstruction->nArg != 0)
        {
          _EAX /= pCurrentInstruction->nArg;
        }
        else
        {
          return 0;
        }
        break;
     case IC_DIV_R2|RI_EAX|RI_EBX:
        if(_EBX != 0)
        {
          _EAX /= _EBX;
        }
        else
        {
          return 0;
        }
        break;
     
     case IC_DEC_M|RI_EAX:
        *((int*)_EAX) -= 1;
        break;        
     case IC_DEC_R|RI_EAX:
        _EAX -= 1;
        break;
     case IC_INC_M|RI_EAX:
        *((int*)_EAX) += 1;
        break;        
     case IC_INC_R|RI_EAX:
        _EAX += 1;
        break;

     case IC_CMP_RM8|RI_EAX|RI_EBX:
        nTmp = _EAX - *((BYTE*)_EBX);
        break;
     case IC_CMP_RM16|RI_EAX|RI_EBX:
        nTmp = _EAX - *((WORD*)_EBX);
        break;
     case IC_CMP_RM32|RI_EAX|RI_EBX:
        nTmp = _EAX - *((DWORD*)_EBX);
        break;
     case IC_CMP_MR8|RI_EAX|RI_EBX:
        nTmp = *((BYTE*)_EAX) - _EBX;
        break;
     case IC_CMP_MR16|RI_EAX|RI_EBX:
        nTmp = *((WORD*)_EAX) - _EBX;
        break;
     case IC_CMP_MR32|RI_EAX|RI_EBX:
        nTmp = *((DWORD*)_EAX) - _EBX;
        break;
     case IC_CMP_RIMM|RI_EAX:
        nTmp = _EAX - pCurrentInstruction->nArg;
        break;
     case IC_CMP_R2|RI_EAX|RI_EBX:
        nTmp = _EAX - _EBX;
        break;

     case IC_JMP_IMM:          //
        _EIP = pCurrentInstruction->nArg;
        continue;
     case IC_JMP_R|RI_EAX:
        _EIP = _EAX;
        continue;
     case IC_JE:
        if(0 == nTmp)
        {
          _EIP = pCurrentInstruction->nArg;
          continue;
        }
        break;

     case IC_NOT|RI_EAX:    
        _EAX = pCurrentInstruction->nArg > 0;        
        break;

     case IC_AND|RI_EAX|RI_EBX:   
        _EAX = (_EAX > 0 && _EBX > 0);
        break;
     case IC_OR|RI_EAX|RI_EBX:            
        _EAX = (_EAX > 0 || _EBX > 0);
        break;
   
     case IC_EQ|RI_EAX|RI_EBX: 
        _EAX = (_EAX == _EBX);
        break;
     case IC_NEQ|RI_EAX|RI_EBX:  
        _EAX = (_EAX != _EBX);
        break;
     case IC_LT|RI_EAX|RI_EBX:  
        _EAX = (_EAX < _EBX);
        break;
     case IC_NGT|RI_EAX|RI_EBX:  
        _EAX = (_EAX <= _EBX);
        break;
     case IC_NLT|RI_EAX|RI_EBX:  
        _EAX = (_EAX >= _EBX);
        break;
     case IC_GT|RI_EAX|RI_EBX:  
        _EAX = (_EAX > _EBX);
        break;

     case IC_MOD|RI_EAX|RI_EBX:
        _EAX %= _EBX;
        break;
     case IC_BIT_AND|RI_EAX|RI_EBX:
        _EAX &= _EBX;
        break;
     case IC_BIT_OR|RI_EAX|RI_EBX:
        _EAX |= _EBX;
        break;
     case IC_BIT_XOR|RI_EAX|RI_EBX:
        _EAX ^= _EBX;
        break;

     case IC_LEA|RI_ECX|RI_EFP:
        _ECX = _EFP + pCurrentInstruction->nArg;
        break;
     case IC_LEA|RI_ECX|RI_EDS:
        _ECX = _EDS + pCurrentInstruction->nArg;
        break;

     case IC_CALL:
        _EIP = pCurrentInstruction->nArg;
        continue;

     case IC_XCHG_R2|RI_EBX|RI_EAX:
     case IC_XCHG_R2|RI_EBX|RI_EAX|REVERSE_FLAG:
        nTmp = _EBX;
        _EBX = _EAX;
        _EAX = nTmp;
        break;

     case IC_XCHG_R2|RI_EBX|RI_ECX:
        nTmp = _EBX;
        _EBX = _ECX;
        _ECX = nTmp;
        break;

     case IC_RET:
        break;
     default:
        assert(false);
     }
     _EIP += INSTRUCTION_SIZE;

   }while(IC_RET != pCurrentInstruction->nCode );

  return _EAX;
}



