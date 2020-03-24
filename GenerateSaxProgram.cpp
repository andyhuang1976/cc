/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/GenerateSaxProgram.cpp,v 1.23 2015/08/17 01:05:56 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/GenerateSaxProgram.cpp,v 1.23 2015/08/17 01:05:56 administrator Exp $";
#include "StdAfx.h"
// $Nokeywords: $
#include ".\generatesaxprogram.h"
#include "parser.h"
#include <strsafe.h>
#include ".\saxccompileimpl.h"
#include <math.h>

#pragma warning(disable : 4995)

const char* COMMON_REGISTER_NAMESX86[] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
const char* COMMON_REGISTER_NAMESX64[] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"};
const char* SEGMENT_REGISTER[]         = {"CS","SS","DS","ES","FS","GS"};
const char* COMMON_REGISTER_NAMESX16[] = {"ax","cx","dx","bx","sp","bp","si","di"};
const char* COMMON_REGISTER_NAMESX8[]  = {"al","cl","dl","bl"};
const char* COMMON_REGISTER_NAMESX8Hi[] = {"ah","ch","dh","bh"};



#define RegName(REG)           (ACM_MODE_X86 == m_nMode)? COMMON_REGISTER_NAMESX86[REG]:COMMON_REGISTER_NAMESX64[REG]


const UINT NONE_SIB       = 0;
const UINT SIB_ADDR       = 4;
const UINT TMP_ADDR_BYTE4 = 0x1fff;

#define SWITCH_JMP_TYPE(nType) (JT_FALSE == nType)? JT_TRUE : JT_FALSE

char* GetDataType(int nSize)
{
	switch(nSize)
	{
	case 8:
		return "qword";
	case 4:
		return "dword";
	case 2:
		return "word";
	case 1:
		return "byte";
	default:
		ATLASSERT(FALSE);
	}
	return NULL;
}

int CalculationIndex(DWORD nNum)
{
	ATLASSERT(nNum >= 1 );

	if(1 == nNum)
	{
		return 0;
	}

	DWORD nBase = 2;
	for(int i = 1;i< 32;i++)
	{
		if(nNum == nBase)
		{
			return i;
		}
		nBase <<= 1;
	}

	return -1;
}

COMMON_REGISTERS Mask2Reg(COMMON_REGISTERS_MASK nRegMask)
{
	COMMON_REGISTERS nReg = RAX;

	switch(nRegMask)
	{
	case CRM_RAX:
		nReg = RAX;
		break;

	case CRM_RCX:
		nReg = RCX;
		break;

	case CRM_RDX:
		nReg = RDX;
		break;

	case CRM_RBX:
		nReg = RBX;
		break;

	case CRM_RSI:
		nReg = RSI;
		break;

	case CRM_RDI:
		nReg = RDI;
		break;
	default:
		ATLASSERT(FALSE);
	}

	return nReg;
}

UINT Reg2Mask(COMMON_REGISTERS nReg)
{
	UINT nRegMask = 0;

	switch(nReg)
	{
	case RAX:
		nRegMask = CRM_RAX;
		break;
	case RCX:
		nRegMask = CRM_RCX;
		break;
	case RDX:
		nRegMask = CRM_RDX;
		break;
	case RBX:
		nRegMask = CRM_RBX;
		break;

	case RDI:
		nRegMask = CRM_RDI;
		break;

	case RSI:
		nRegMask = CRM_RSI;
		break;

	default:
		ATLASSERT(FALSE);
	}

	return nRegMask;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CalculateComplement
| 描  述 : 计算指定数值的补码
| 参  数 : int nRelativeDisplacement->相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/

DWORD CalculateComplement(int nRelativeDisplacement)
{
	return (nRelativeDisplacement > 0)? nRelativeDisplacement : (~abs(nRelativeDisplacement) + 1); 
}
/*-----------------------------CConditionExprJmpRef----------------------------------------------*/

void CConditionExprJmpRef::Reset()
{
	m_arrTrueRefPos.clear();
	m_arrFalseRefPos.clear();
}

void CConditionExprJmpRef::AppendRefPos(JMP_TYPE nJmpType,UINT nRefPos)
{
	if(JT_FALSE == nJmpType)
	{
		m_arrFalseRefPos.push_back(nRefPos);
	}
	else
	{
		m_arrTrueRefPos.push_back(nRefPos);
	}
}
/*********************************************************************
*  函数名称  : CConditionExprJmpRef::WriteRealAddr
*  描    述  : 填入真实的地址
*  参    数  : JMP_TYPE nJmpType——需要修改的类型
*              UINT nRealAddr——真实的地址
*             CGenerateSaxProgram* pSaxProgram——
*             CContext* pContext——分析上下文
*             UINT nStartPos—开始位置
*  返 回 值  : 
*  修改记录  : 2005-02-28 10:56:22   -huangdy-   创建
*********************************************************************/
void CConditionExprJmpRef::WriteRealAddr(JMP_TYPE nJmpType,UINT nRealAddr
	,CGenerateSaxProgram* pSaxProgram,CContext* pContext,UINT nStartPos)
{
	ATLASSERT(pSaxProgram);
	ATLASSERT(pContext);

	OFFSET_ARRAY& refOffsetArr =  (JT_FALSE == nJmpType)? m_arrFalseRefPos : m_arrTrueRefPos;
	
	OFFSET_ARRAY_ITR itr = refOffsetArr.begin();
	while(itr != refOffsetArr.end())
	{
		if(nStartPos < *itr)
		{
			pSaxProgram->WriteDwordData((*itr) - 4/*偏移地址的长度*/,nRealAddr - (*itr),*pContext);
			itr = refOffsetArr.erase(itr);
		}
		else
		{
			itr++;
		}
	}

}
/*-----------------------------CContext----------------------------------------------*/
CContext::CContext(int nOffset/* = 0*/)
	: m_nOffset(nOffset)
	, m_nRegisterUsedMask(0)
	, m_pCurSTMTBindingLevel(NULL)
	, m_pCurVar(NULL)
	, m_pConditionExprJmpRef(NULL)
{
}

CContext::~CContext()
{
	LABLE_REFRENCE_INFO_ARRAY_ITR itr = m_arrLableRefrenceInfo.begin();
	while(m_arrLableRefrenceInfo.end() != itr)
	{
		delete (*itr);
		itr++;
	}

	STMTBINDINGLEVEL_ARRAY_ITR itr1 = m_arrSTMTBindingLevel.begin();
	while(m_arrSTMTBindingLevel.end() != itr1)
	{
		delete (*itr1);
		itr1++;
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CContext::RequestReg
| 描  述 : 请求使用指定的寄存器，如果寄存器空闲返回寄存器编号，否则
|    检查是否有空闲的有返回，否则返回０
| 参  数 : COMMON_REGISTERS_MASK nMask->寄存器掩码
| 参  数 : BOOL bOther->使用其它的空闲寄存器
| 返 回 值 : 寄存器编号
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
COMMON_REGISTERS CContext::RequestReg(COMMON_REGISTERS_MASK nMask,BOOL bOther)
{
	COMMON_REGISTERS nRegcode = CR_INVALID;
	if(!(nMask & m_nRegisterUsedMask))
	{
		nRegcode = Mask2Reg(nMask);		
	}
	else if(bOther)
	{
		if(!(CRM_RCX & m_nRegisterUsedMask))
		{
			nRegcode = RCX;
		}    
		else if(!(CRM_RBX & m_nRegisterUsedMask))
		{
			nRegcode = RBX;
		}
		else if(!(CRM_RSI & m_nRegisterUsedMask))
		{
			nRegcode = RSI;
		}    
		else if(!(CRM_RDI & m_nRegisterUsedMask))
		{
			nRegcode = RDI;
		}
	}

	return nRegcode;
}
/*-----------------------------------------------------------------
| 函数名称 : CContext::ReserveReg
| 描  述 : 保留一个寄存器,表示该寄存不能用
| 参  数 : COMMON_REGISTERS nReg->标签名称寄存器编号
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
void CContext::ReserveReg(COMMON_REGISTERS nReg)
{
	m_nRegisterUsedMask |= Reg2Mask(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CContext::FreeReg
| 描  述 : 释放一个寄存器
| 参  数 : COMMON_REGISTERS nReg->标签名称寄存器编号
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
void CContext::FreeReg(COMMON_REGISTERS nReg)
{
	m_nRegisterUsedMask ^= Reg2Mask(nReg);	
}

/*-----------------------------------------------------------------
| 函数名称 : CContext::OnRegChanged
| 描  述 : 改变寄存器值事件,变量的值默认是存放RDX:RAX中,采用当前这种
|          方法主要是避免重复装在
| 参  数 : COMMON_REGISTERS nReg->值被改变的寄存器
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
void CContext::OnRegChanged(COMMON_REGISTERS nReg)
{
	if(NULL != m_pCurVar)
	{
		if(RAX == nReg)
		{
			m_pCurVar = NULL;
		}
		else if(RDX == nReg)
		{
			CDataTypeTreeNode* pDataTypeNode = m_pCurVar->GetDataType();		
			if(pDataTypeNode && pDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
			{
				m_pCurVar = NULL;
			}
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CContext::CreateLableRefrence
| 描  述 : 创建一个标签引用参考记录
| 参  数 : LPCSTR lpName->标签名称
| 返 回 值 : 被创建的对象
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
CLableRefrenceInfo* CContext::CreateLableRefrence(LPCSTR lpName)
{
	ATLASSERT(lpName);

	CLableRefrenceInfo* pLableRefrenceInfo = NULL;
	if(NULL != lpName)
	{
		pLableRefrenceInfo = new CLableRefrenceInfo(lpName);
		if(pLableRefrenceInfo)
		{
			m_arrLableRefrenceInfo.push_back(pLableRefrenceInfo);
		}
	}

	return pLableRefrenceInfo;
}

/*-----------------------------------------------------------------
| 函数名称 : CContext::LookupLableRefrence
| 描  述 : 查询一个标签引用参考记录
| 参  数 : LPCSTR lpName->标签名称
| 返 回 值 : 标签对象指针对象
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/

CLableRefrenceInfo* CContext::LookupLableRefrence(LPCSTR lpName)
{
	ATLASSERT(lpName);

	LABLE_REFRENCE_INFO_ARRAY_ITR itr = m_arrLableRefrenceInfo.begin();
	while(m_arrLableRefrenceInfo.end() != itr)
	{
		if((*itr)->GetName() == lpName)
		{
			return *itr;
		}
		itr++;
	}

	return NULL;
}
/*-----------------------------------------------------------------
| 函数名称 : CContext::CreateSTMTBindingLevel
| 描  述 : 为当前的语句创建一个状态对象
| 参  数 : UINT nStartPos->当前语句的开始便宜
| 返 回 值 : 被创建的对象
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
CSTMTBindingLevel* CContext::CreateSTMTBindingLevel(UINT nStartPos)
{
	CSTMTBindingLevel* pNewSTMTBindingLevel = new CSTMTBindingLevel(nStartPos);
	if(pNewSTMTBindingLevel)
	{
		m_arrSTMTBindingLevel.push_back(pNewSTMTBindingLevel);
		m_pCurSTMTBindingLevel = pNewSTMTBindingLevel;
	}

	return pNewSTMTBindingLevel;
}
/*-----------------------------------------------------------------
| 函数名称 : CContext::PopSTMTBindingLevel
| 描  述 : 删除m_arrSTMTBindingLevel数组最后的节点
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
void CContext::PopSTMTBindingLevel()
{
	if(m_arrSTMTBindingLevel.size() > 0)
	{
		delete m_pCurSTMTBindingLevel;
		m_pCurSTMTBindingLevel = NULL;

		m_arrSTMTBindingLevel.pop_back();

		if(m_arrSTMTBindingLevel.size() > 0)
		{
			m_pCurSTMTBindingLevel = *(m_arrSTMTBindingLevel.rbegin()); //设置最后一个节点为当前节点
		}
	}
}

/*---------------------------CGenerateSaxProgram------------------------------------------------*/
CGenerateSaxProgram::CGenerateSaxProgram(CSaxCCompileimpl* pSaxCCompileimpl)
	: m_nLable(1)
	, m_pCodeBuffer(NULL)
	, m_nCodeBufferLen(0)
	, m_pCurPosWrited(NULL)
	, m_pPrvPosWrited(NULL)
	, m_pSaxCCompileimpl(pSaxCCompileimpl)
	, m_pSyntaxTreeRoot(NULL)
	, m_nMode(ACM_MODE_X86)
	, m_bEnableMasm(FALSE)
{
}

CGenerateSaxProgram::~CGenerateSaxProgram(void)
{
	if(NULL != m_pCodeBuffer)
	{
		delete []m_pCodeBuffer;
	}   
}

void CGenerateSaxProgram::Reset()
{
	m_nLable    = 1;
	m_pCurPosWrited = m_pCodeBuffer;
	m_pPrvPosWrited = m_pCodeBuffer;

	m_strMasmCode.Empty();
	m_arrReferences.clear();
}
/*-----------------------------------------------------------------
| 函数名称 : CAssemble::AllocatBinaryCodeBuffer
| 描  述 : 
| 参  数 : size_t pLength——
| 返 回 值 : TRUE----
|       FALSE---
| 修改记录 : 2007-4-28 13:46:15  -huangdy-  创建
-----------------------------------------------------------------*/
BOOL CGenerateSaxProgram::AllocatBinaryCodeBuffer(size_t pLength)
{
	if(pLength > m_nCodeBufferLen)
	{
		BYTE* pTmpBuffer = new BYTE[pLength];
		if(NULL != pTmpBuffer)
		{   
			const UINT nOffset = GetOffset();

			memcpy(pTmpBuffer,m_pCodeBuffer,m_nCodeBufferLen);
			delete []m_pCodeBuffer;

			m_pCodeBuffer   = pTmpBuffer;
			m_nCodeBufferLen = pLength;

			m_pCurPosWrited = m_pCodeBuffer;
			MoveWritingPtr(nOffset);
		}
	}

	return (NULL != m_pCodeBuffer);
}



/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::RequestSpace
| 描  述 : 处理全局变量
| Parameter : 需要分配空间的大小
| 返 回 值 : 返回被分配空间的头指针
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
BYTE* CGenerateSaxProgram::RequestSpace(UINT nLen)
{
	BYTE* pResult = NULL;
	ATLASSERT(nLen > 0);
	if(m_pCodeBuffer + m_nCodeBufferLen < m_pCurPosWrited + nLen)
	{
		AllocatBinaryCodeBuffer(m_nCodeBufferLen + 1024);
	}

	pResult     = m_pCurPosWrited;
	m_pPrvPosWrited = m_pCurPosWrited;
	m_pCurPosWrited += nLen;

	return pResult;
}

void CGenerateSaxProgram::AppendRef(UINT nOffset)
{
	m_arrReferences.push_back(nOffset);
	//ATLTRACE(_T("offset:%d\r\n"),nOffset);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateMasmCmd
| 描  述 : 创建汇编指令
| 参  数 : BYTE nCode->指令编码
| 修改记录 : 2014-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateMasmCmd(char* format, ... )
{
	char szBuffer[256] = {0};
	va_list params;

	va_start( params, format );
	_vsnprintf_s(szBuffer,256, 256, format, params );
	va_end( params );   

	m_strMasmCode += szBuffer;
#ifdef _DEBUG
	ATLTRACE(_T("%08X  "),m_pPrvPosWrited - m_pCodeBuffer);

	const UINT nCmdLen = m_pCurPosWrited - m_pPrvPosWrited;
	if(nCmdLen > 0)
	{
		CString strInstruction,strTmp;

		for(UINT i = 0;i < nCmdLen;i++)
		{
			strTmp.Format(_T("%02X "),m_pPrvPosWrited[i]);
			strInstruction += strTmp;
		}
		ATLTRACE(_T("%-32s"),strInstruction);
	}

	ATLTRACE(_T("%s"),CString(szBuffer));
#endif
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::WriteDwordData
| 描  述 : 向代码映像文件中的指定位置直接写入输入
| 参  数 : UINT nOffset->写入的数据的位置偏移
| 参  数 : DWORD nData->需要写入的数据
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::WriteDwordData(UINT nOffset,DWORD nData,CContext& objContext)
{
	if(nOffset < m_nCodeBufferLen)
	{
		*(DWORD*)&m_pCodeBuffer[nOffset] = nData;
	}

	objContext.SetCurVar(NULL);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::WriteByteData
| 描  述 : 向代码映像文件中的指定位置直接写入输入
| 参  数 : UINT nOffset->写入的数据的位置偏移
| 参  数 : BYTE nData->需要写入的数据
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::WriteByteData(UINT nOffset,BYTE nData,CContext& objContext)
{
	if(nOffset < m_nCodeBufferLen)
	{
		m_pCodeBuffer[nOffset] = nData;
	}

	objContext.SetCurVar(NULL);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射单字节指令代码
| 参  数 : BYTE nCode->指令编码
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode)
{
	BYTE* pBuffer = RequestSpace(1);
	ATLASSERT(pBuffer);

	*pBuffer = nOpCode;
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射单字节指令代码+四个字节的参数
| 参  数 : BYTE nOpCode->指令编码
| 参  数 : DWORD nPara->指令参数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode,BYTE nPara)
{
	BYTE* pBuffer = RequestSpace(2);
	ATLASSERT(pBuffer);

	pBuffer[0]      = nOpCode;
	pBuffer[1]      = nPara;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射单字节指令代码+四个字节的参数
| 参  数 : BYTE nOpCode->指令编码
| 参  数 : DWORD nPara->指令参数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode,DWORD nPara)
{
	BYTE* pBuffer = RequestSpace(5);
	ATLASSERT(pBuffer);

	pBuffer[0]      = nOpCode;
	*(DWORD*)&pBuffer[1] = nPara;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射双字节指令代码+四个字节的参数
| 参  数 : BYTE nOpCode->指令编码1
| 参  数 : BYTE nCode2->指令编码2
| 参  数 : DWORD nPara->指令参数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nPreFix,BYTE nOpCode,BYTE nPara)
{
	if(PREFIX_INVALID != nPreFix)
	{
		BYTE* pBuffer = RequestSpace(3);
		ATLASSERT(pBuffer);

		pBuffer[0]      = nPreFix;
		pBuffer[1]      = nOpCode;
		pBuffer[2]      = nPara;
	}
	else
	{
		BYTE* pBuffer = RequestSpace(2);
		ATLASSERT(pBuffer);

		pBuffer[0]      = nOpCode;
		pBuffer[1]      = nPara;
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射双字节指令代码+四个字节的参数
| 参  数 : BYTE nOpCode->指令编码1
| 参  数 : BYTE nCode2->指令编码2
| 参  数 : DWORD nPara->指令参数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nPreFix,BYTE nOpCode,DWORD nPara)
{
	if(PREFIX_INVALID != nPreFix)
	{
		BYTE* pBuffer = RequestSpace(6);
		ATLASSERT(pBuffer);

		pBuffer[0]      = nPreFix;
		pBuffer[1]      = nOpCode;
		*(DWORD*)&pBuffer[2] = nPara;
	}
	else
	{
		BYTE* pBuffer = RequestSpace(5);
		ATLASSERT(pBuffer);

		pBuffer[0]      = nOpCode;
		*(DWORD*)&pBuffer[1] = nPara;
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::EmitInstruction
| 描  述 : 发射指令:前缀+代码+寻址方式(ModRM)+nRD+8位任意数
| 参  数 : BYTE nPreFix->指令前缀
| 参  数 : BYTE nOpCode->指令编码
| 参  数 : BYTE nModRM->寻址方式
| 参  数 : DWORD nDisplacement->若MOD = 00B表示没有基,且带有一个32位
|       的偏移量；否则表示disp8或disp32 + [EBP].即提供如下的寻址方式:
| 参  数 : BYTE nIMM32->立即数
| 参  数 : int nIMMBytes->立即数的位数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::EmitInstruction(BYTE nPreFix,BYTE nOpCode,BYTE nModRM
	,BYTE nSIB,int nDisplacement,int nIMM32,int nIMMBytes/* = 4*/,BOOL bStatic)
{
	UINT nInstLen = 0;
	if(PREFIX_INVALID != nPreFix)
	{
		nInstLen++;               // nPreFix长度
	}
	nInstLen += 2;               // nOpCode + nModRM 长度

	const MOD MODE_TYPE = MOD(nModRM >> 6);
	if(MODE_TYPE != MODE11 && (nModRM & 7) == SIB_ADDR)
	{
		nInstLen ++;              // SIB长度
	}

	switch(MODE_TYPE)   // 偏移量
	{
	case MODE00:
		if((nModRM & 7) == RBP)
		{
			nInstLen += 4;
		}
		else if((nModRM & 7) == SIB_ADDR)
		{
			if((nSIB & 7) == RBP)
			{
				nInstLen += 4;
			}
		}
		break;

	case MODE01:
		nInstLen += 1;
		break;

	case MODE10:
		nInstLen += 4;
		break;	
	}

	nInstLen += nIMMBytes;           // nIMM32长度

	BYTE* pBuffer = RequestSpace(nInstLen);
	ATLASSERT(pBuffer);

	nInstLen = 0;
	if(PREFIX_INVALID != nPreFix)
	{
		pBuffer[nInstLen++] = nPreFix;
	}
	pBuffer[nInstLen++] = nOpCode;
	pBuffer[nInstLen++] = nModRM;
	if(MODE_TYPE != MODE11 && (nModRM & 7) == SIB_ADDR)
	{
		pBuffer[nInstLen++] = nSIB;
	}

	UINT nResult = 0;
	switch(MODE_TYPE)   
	{
	case MODE00:
		if((nModRM & 7) == RBP)
		{
			*(DWORD*)&pBuffer[nInstLen] = CalculateComplement(nDisplacement);
			nInstLen += 4;
			nResult += 4;
		}
		else if((nModRM & 7) == SIB_ADDR)
		{
			if((nSIB & 7) == RBP)
			{
				*(DWORD*)&pBuffer[nInstLen] = CalculateComplement(nDisplacement);
				nInstLen += 4;
				nResult += 4;
			}
		}
		break;

	case MODE01:
		pBuffer[nInstLen++]     = (BYTE)CalculateComplement(nDisplacement);
		nResult++;
		break;	

	case MODE10:
		*(DWORD*)&pBuffer[nInstLen] = CalculateComplement(nDisplacement);
		nInstLen += 4;
		nResult += 4;
		break;
	}

	switch(nIMMBytes)
	{
	case 1:
		pBuffer[nInstLen]      = (BYTE)CalculateComplement(nIMM32);
		nResult += 1;
		break;

	case 2:
		*(WORD*)&pBuffer[nInstLen] = (WORD)CalculateComplement(nIMM32);
		nResult += 2;
		break;

	case 4:
		*(DWORD*)&pBuffer[nInstLen] = (DWORD)CalculateComplement(nIMM32);
		nResult += 4;
		break;

		//default:
		//    ATLASSERT(FALSE);
	} 

	if(bStatic)
	{
		AppendRef(GetOffset() - nResult);
	}

	return nResult;
}



/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::PUSH
| 描  述 : 把指定的寄存器推进栈
| 参  数 : REGISTERS nRegister——寄存器编号
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::PUSH(COMMON_REGISTERS nRegister)
{
	EmitInstruction((BYTE)(0x50 + nRegister));  //OPCode = 01010reg   
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s \r\n","push",RegName(nRegister));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::PUSH
| 描  述 : 把指定的段寄存器推进栈,不明白为什只有FS,GS需要"0FH"前缀
| 参  数 : REGISTERS nRegister——寄存器编号
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::PUSH(SEGMENT_REGISTERS nRegister)
{
	switch(nRegister)
	{
	case FS:
		EmitInstruction(PREFIX_LOCK,(BYTE)0xA0);   
		break;

	case GS:
		EmitInstruction(PREFIX_LOCK,(BYTE)0xA8);   
		break;

	default:
		EmitInstruction(BYTE(nRegister + 0x06));  //OPCode = 000reg110 
	}

	if(m_bEnableMasm && nRegister < 6)
	{
		GenerateMasmCmd("%-10s %s \r\n","push",SEGMENT_REGISTER[nRegister]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::PUSH_IMM
| 描  述 : 把一个立即数推进栈
| 参  数 : DWORD nValue——立即数
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::PUSH_IMM32(DWORD nValue)
{
	EmitInstruction(0x68,nValue);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %Xh \r\n","push",nValue);
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::PUSH_IMM
| 描  述 : 把一个变量的值推进栈
| 参  数 : DWORD nAddr——变量地址
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::PUSH_M32(DWORD nAddr)
{
	EmitInstruction(PREFIX_INVALID,0xFF,ModRM(MODE00,6,RBP),NONE_SIB,nAddr,0,0,FALSE); //FF,000+110(指令代码)+101(表示后面是内存地址)
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [(%Xh)] \r\n","push",nAddr);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::POP
| 描  述 : 从栈里面取出四个字节的值并放入指定的通用寄存器中
| 参  数 : REGISTERS nRegister——寄存器编号
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::POP(COMMON_REGISTERS nRegister,CContext &objContext)
{
	EmitInstruction((BYTE)(0x58 + nRegister));  //OPCode = 01011reg   
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s \r\n","pop",RegName(nRegister));
	}

	objContext.OnRegChanged(nRegister);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::POP
| 描  述 : 从栈里面取出四个字节的值并放入指定的段寄存器中
| 参  数 : REGISTERS nRegister——寄存器编号
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::POP(SEGMENT_REGISTERS nRegister)
{
	switch(nRegister)
	{
	case ES:
		EmitInstruction(0x07); 
		break;

	case SS:
		EmitInstruction(0x17); 
		break;

	case DS:
		EmitInstruction(0x1F); 
		break;

	case FS:
		EmitInstruction(PREFIX_LOCK,(BYTE)0xA1);   
		break;

	case GS:
		EmitInstruction(PREFIX_LOCK,(BYTE)0xA9);   
		break;

	default:
		ATLASSERT(FALSE);
	}

	if(m_bEnableMasm && nRegister < 6)
	{
		GenerateMasmCmd("%-10s %s \r\n","pop",SEGMENT_REGISTER[nRegister]);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::POP
| 描  述 : 从栈里面取出四个字节的值并放入指定的变量中
| 参  数 : REGISTERS nRegister——寄存器编号
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::POP_M32(DWORD nAddr)
{
	EmitInstruction(PREFIX_INVALID,0x8F,ModRM(MODE00,0,RBP),NONE_SIB,nAddr,0,0,FALSE); 

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [(%Xh)] \r\n","pop",nAddr);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV
| 描  述 : 如果(MODE11 == nMod)是把nSrcReg寄存器里的值移动到nDstReg寄存器里
｜    否则是把nSrcReg寄存器所指的变量中的值移动到nDstReg寄存器里
| 参  数 : COMMON_REGISTERS nDstReg——目的寄存器
| 参  数 : COMMON_REGISTERS nSrcReg——源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext,MOD nMod)
{
	EmitInstruction(0x8b,ModRM(nMod,nDstReg,nSrcReg)); // 0x8b,mod + reg1 + reg2

	if(m_bEnableMasm)
	{
		if(MODE11 == nMod)
		{
			GenerateMasmCmd("%-10s %s, %s \r\n","mov",RegName(nDstReg),RegName(nSrcReg));
		}
		else
		{
			GenerateMasmCmd("%-10s %s, dword ptr [%s] \r\n","mov",RegName(nDstReg),RegName(nSrcReg));
		}
	}

	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_IV8_IMM
| 描  述 : 把一个立即数移动到局部的8位变量中
| 参  数 : int nRelativeDisplacement->相对于RBP俱存器的位移
| 参  数 : BYTE nValue—>立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_IV8_IMM(int nOffset,BOOL bStatic,int nValue)
{
	MOD nModeType = MODE00;
	if(!bStatic)
	{
		nModeType = (abs(nOffset) < 127)?MODE01:MODE10;
	}
	EmitInstruction(PREFIX_INVALID,0xC6,ModRM(nModeType,0,RBP),NONE_SIB,nOffset,nValue,1,bStatic);

	if(m_bEnableMasm)
	{
		if(bStatic)
		{
			GenerateMasmCmd("%-10s byte ptr [(%0Xh)],%d \r\n" ,"mov",nOffset,nValue);
		}
		else
		{
			GenerateMasmCmd("%-10s byte ptr [ebp %+d],%d\r\n","mov",nOffset,nValue);
		}
	}

}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_IV16_IMM
| 描  述 : 把一个立即数移动到局部的16位变量中
| 参  数 : int nRelativeDisplacement->相对于RBP俱存器的位移
| 参  数 : BYTE nValue—>立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_IV16_IMM(int nOffset,BOOL bStatic,int nValue)
{
	MOD nModeType = MODE00;
	if(!bStatic)
	{
		nModeType = (abs(nOffset) < 127)?MODE01:MODE10;
	}
	EmitInstruction(PREFIX_OPERAND_SIZE,0xC7,ModRM(nModeType,0,RBP),NONE_SIB,nOffset,nValue,2,bStatic);

	if(m_bEnableMasm)
	{
		if(bStatic)
		{
			GenerateMasmCmd("%-10s word ptr [(%0Xh)],%d \r\n" ,"mov",nOffset,nValue);
		}
		else
		{
			GenerateMasmCmd("%-10s word ptr [ebp %+d],%d\r\n","mov",nOffset,nValue);
		}
	}   
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_LIV32_IMM
| 描  述 : 把一个立即数移动到局部的32位变量中
| 参  数 : int nRelativeDisplacement->相对于RBP寄存器的位移
| 参  数 : BYTE nValue—>立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV32_IMM(int nOffset,BOOL bStatic,int nValue)
{
	MOD nModeType = MODE00;
	if(!bStatic)
	{
		nModeType = (abs(nOffset) < 127)?MODE01:MODE10;
	}
	EmitInstruction(PREFIX_INVALID,0xC7,ModRM(nModeType,0,RBP),NONE_SIB,nOffset,nValue,4,bStatic);

	if(m_bEnableMasm)
	{
		if(bStatic)
		{
			GenerateMasmCmd("%-10s dword ptr [(%0Xh)],%d \r\n" ,"mov",nOffset,nValue);
		}
		else
		{
			GenerateMasmCmd("%-10s dword ptr [ebp %+d],%d\r\n","mov",nOffset,nValue);
		}
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_LIV32_OFFSET
| 描  述 : 把一个偏移地址移动到局部的32位变量中
| 参  数 : int nRelativeDisplacement->相对于RBP寄存器的位移
| 参  数 : BYTE nOffset—>立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV32_OFFSET(int nRelativeDisplacement,DWORD nOffset)
{
	EmitInstruction(PREFIX_INVALID,0xc7,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,0,RBP)
		,NONE_SIB,nRelativeDisplacement,nOffset,4,FALSE);
	AppendRef(GetOffset() - 4);   //不能删除

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+d], offset(%X) \r\n","mov"
			,nRelativeDisplacement,nOffset);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GIV32_OFFSET
| 描  述 : 把一个偏移地址移动到全局的32位变量中
| 参  数 : DWORD nDisplacement->位移
| 参  数 : BYTE nOffset—>偏移地址
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV32_OFFSET(DWORD nDisplacement,DWORD nOffset)
{
	EmitInstruction(PREFIX_INVALID,0xc7,ModRM(MODE00,0,RBP),NONE_SIB,nDisplacement,nOffset,4,FALSE);
	AppendRef(GetOffset() - 8);   //不能删除
	AppendRef(GetOffset() - 4);    //不能删除

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [(%Xh)], offset(%d) \r\n","mov",nDisplacement,nOffset);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GFV_IMM
| 描  述 : 把一个立即浮点数数移动到局部的64位(double)变量中,浮点数不同于
|       整数,而类似字符串
| 参  数 : int nRelativeDisplacement->相对于RBP寄存器的位移
| 参  数 : DWORD nDisplacement->立即数在印像文件中的位移
| 参  数 : BIT_TYPE nBitType->目的变量的类型32或６４位 
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LFV_IMM(int nRelativeDisplacement,DWORD nDisplacement,BIT_TYPE nBitType)
{
	FLD(nDisplacement,BT_BIT64/*因为立即数在写入印象文件的时候全部转化为了６４位的双精度的浮点数*/);
	FSTP(nRelativeDisplacement,nBitType);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GFV_IMM
| 描  述 : 把一个立即浮点数数移动到静态的变量中,浮点数不同于
|       整数,而类似字符串
| 参  数 : DWORD nDisplacement1->静态变量在印像文件中的位移
| 参  数 : DWORD nDisplacement2->立即数在印像文件中的位移
| 参  数 : BIT_TYPE nBitType->目的变量的类型32或６４位     
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GFV_IMM(DWORD nDisplacement1,DWORD nDisplacement2,BIT_TYPE nBitType)
{
	FLD(nDisplacement2,BT_BIT64/*因为立即数在写入印象文件的时候全部转化为了６４位的双精度的浮点数*/);
	FSTP(nDisplacement1,nBitType);   
}



/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_LIV8_REG
| 描  述 : 把一个寄存器中的值移动到局部8位变量中
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV8_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg)
{
	EmitInstruction(PREFIX_INVALID,0x88,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nSrcReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s byte ptr [ebp %+d],%s \r\n","mov"
			,nRelativeDisplacement,COMMON_REGISTER_NAMESX8[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GIV8_REG
| 描  述 : 把一个寄存器中的值移动到静态的8位变量中
| 参  数 : DWORD nRelativeDisplacement->静态变量在印像文件中的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV8_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg)
{
	EmitInstruction(PREFIX_INVALID,0x88,ModRM(MODE00,nSrcReg,RBP),NONE_SIB,nDisplacement,0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s byte ptr [%Xh],%s \r\n","mov" 
			,nDisplacement,COMMON_REGISTER_NAMESX8[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_LIV16_REG
| 描  述 : 把一个寄存器中的值移动到局部16位变量中
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV16_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg)
{
	EmitInstruction(PREFIX_OPERAND_SIZE,0x89,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10
		,nSrcReg,RBP),NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s word ptr [ebp %+d],%s \r\n","mov"
			,nRelativeDisplacement,COMMON_REGISTER_NAMESX16[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GIV16_REG
| 描  述 : 把一个寄存器中的值移动到静态的16位变量中
| 参  数 : DWORD nRelativeDisplacement->静态变量在印像文件中的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV16_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg)
{
	if(RAX == nSrcReg)   //RAX需要单独处理
	{
		EmitInstruction(PREFIX_OPERAND_SIZE,0xa3,nDisplacement);
	}
	else
	{
		EmitInstruction(PREFIX_OPERAND_SIZE,0x89,ModRM(MODE00,nSrcReg,RBP),NONE_SIB,nDisplacement,0,0,FALSE);
	}
	AppendRef(GetOffset() - 4);   

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s word ptr [%Xh],%s \r\n","mov"
			,nDisplacement,COMMON_REGISTER_NAMESX16[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_LIV32_REG
| 描  述 : 把一个寄存器中的值移动到局部32位变量中
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV32_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg)
{
	EmitInstruction(PREFIX_INVALID,0x89,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nSrcReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+d],%s \r\n","mov"
			,nRelativeDisplacement,COMMON_REGISTER_NAMESX86[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_GIV32_REG
| 描  述 : 把一个寄存器中的值移动到静态的32位变量中
| 参  数 : DWORD nDisplacement->静态变量在印像文件中的位移
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV32_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg)
{
	if(RAX == nSrcReg)   //RAX需要单独处理
	{
		EmitInstruction(0xa3,nDisplacement);
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0x89,ModRM(MODE00,nSrcReg,RBP),NONE_SIB,nDisplacement,0,0,FALSE);
	}
	AppendRef(GetOffset() - 4);   

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [%Xh],%s \r\n","mov"
			,nDisplacement,COMMON_REGISTER_NAMESX86[nSrcReg]);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_REG_IMM32
| 描  述 : 把值一个立即数移动到nDstReg寄存器里
| 参  数 : COMMON_REGISTERS nDstReg——目的寄存器
| 参  数 : DWORD nValue—立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_REG_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	EmitInstruction((BYTE)(0xB8 | nDstReg),CalculateComplement(nValue)); 

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %Xh \r\n","mov",RegName(nDstReg),nValue);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_REG_LIV32
| 描  述 : 把一个局部32位变量的值移动到寄存器中
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_REG_LIV32(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0x8b,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,dword ptr [ebp %+d]\r\n","mov"
			,COMMON_REGISTER_NAMESX86[nDstReg],nRelativeDisplacement);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_REG_GIV32
| 描  述 : 把一个静态的32位变量的值移动到寄存器中
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器
| 参  数 : DWORD nDisplacement->静态变量在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_REG_GIV32(COMMON_REGISTERS nDstReg,DWORD nDisplacement,CContext &objContext)
{
	if(RAX == nDstReg)
	{
		EmitInstruction(0xa1,nDisplacement); 
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0x8b,ModRM(MODE00,nDstReg,RBP),NONE_SIB,nDisplacement,0,0,FALSE); 
	}
	AppendRef(GetOffset() - 4);   

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, dword ptr[%Xh] \r\n","mov",RegName(nDstReg),nDisplacement);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOVZX_REG_LIV16
| 描  述 : 把一个局部16位变量的值移动到寄存器中
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOVZX_REG_LIV16(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0xb7,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,word ptr [ebp %+d] \r\n","movzx"
			,nRelativeDisplacement,RegName(nDstReg));
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MOV_REG_LIV8
| 描  述 : 把一个局部8位变量的值移动到寄存器中
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器
| 参  数 : DWORD nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_REG_LIV8(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0x8A,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,byte ptr [ebp %+d] \r\n","mov"
			,COMMON_REGISTER_NAMESX8[nDstReg],nRelativeDisplacement);
	}
	objContext.OnRegChanged(nDstReg);
}


//以下是浮点数运算指令相关函数

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLD
| 描  述 : 把一个浮点数(可以使立即数也可以是静态或全局变量)
|       移动到浮点数寄存器中
| 参  数 : DWORD nDisplacement->浮点数在印像文件中的位移
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLD(DWORD nDisplacement,BIT_TYPE nBitType)
{
	EmitInstruction(PREFIX_INVALID,(BT_BIT32 == nBitType)? 0xD9:0xDD,ModRM(MODE00,0,RBP)
		,NONE_SIB,nDisplacement,0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [(%Xh)]\r\n","fld"
			,(BT_BIT32 == nBitType)? "dword":"qword",nDisplacement);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLD
| 描  述 : 把一个浮点数(局部变量)移动到浮点数寄存器中
| 参  数 : int nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLD(int nRelativeDisplacement,BIT_TYPE nBitType)
{
	EmitInstruction(PREFIX_INVALID,(BT_BIT32 == nBitType)? 0xD9:0xDD
		,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,0, RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [ebp %+d]\r\n" 
			,"fld",(BT_BIT32 == nBitType)? "dword":"qword",nRelativeDisplacement);
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSTP
| 描  述 : 把浮点数寄存器中的数移动到一个浮点数变量中(可以是静态,全局变量,局部变量)
| 参  数 : int nOffset->局部变量相对于RBP寄存器的位移或者全局便来的位移
| 参  数 : BOOL bStatic->全局变量标志
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTP(int nOffset,BOOL bStatic,BIT_TYPE nBitType)
{
	MOD nModeType = MODE00;
	if(!bStatic)
	{
		nModeType = (abs(nOffset) < 127)? MODE01 : MODE10;
	}

	EmitInstruction(PREFIX_INVALID,(BT_BIT32 == nBitType)? 0xD9:0xDD
		,ModRM(nModeType,3,RBP),NONE_SIB,nOffset,0,0,bStatic);

	if(m_bEnableMasm)
	{
		if(bStatic)
		{
			GenerateMasmCmd("%-10s %s ptr [(%Xh)]\r\n","fstp"
				,(BT_BIT32 == nBitType)? "dword":"qword",nOffset);
		}
		else
		{
			GenerateMasmCmd("%-10s %s ptr [ebp %+d]\r\n","fstp"
				,(BT_BIT32 == nBitType)? "dword":"qword",nOffset);
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FST
| 描  述 : 把浮点数寄存器中的数移动到一个浮点数变量中(可以是静态或全局变量)
| 参  数 : DWORD nDisplacement->浮点数在印像文件中的位移
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FST(DWORD nDisplacement,BIT_TYPE nBitType)
{
	EmitInstruction(PREFIX_INVALID,(BT_BIT32 == nBitType)? 0xD9:0xDD,ModRM(MODE00,2,RBP)
		,NONE_SIB,nDisplacement,0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [(%Xh)]\r\n"
			,"fst",(BT_BIT32 == nBitType)? "dword":"qword",nDisplacement);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FADDP
| 描  述 : 先进行实数加法：ST(i)←ST(i)+ST(0)，然后进行一次出栈操作
| 参  数 : DWORD i->ST(i)
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
----------------------------------------------------------------*/
void CGenerateSaxProgram::FADDP(DWORD i)
{
	EmitInstruction(0xDE,(BYTE)(0xc0 + i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s ST(i),ST(0)\r\n","faddp");
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FADD_M64REAL
| 描  述 : 实数加法：ST(0)←ST(0)+m64real
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FADD_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,0,RBP),NONE_SIB,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fadd",
			(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSUB_M64REAL
| 描  述 : 实数减法：ST(0)←ST(0)-m64real
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSUB_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,4,RBP),NONE_SIB,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fsub"
			,(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSUB_M64REAL
| 描  述 : 实数减法：ST(0)←m64real-ST(0)
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSUBR_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,5,RBP),NONE_SIB
		,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fsubr"
			,(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSUBP
| 描  述 : 先进行实数减法：ST(i)←ST(i)-ST(0)，然后进行一次出栈操作
| 参  数 : DWORD i->浮点数寄存器编号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSUBP(DWORD i)
{
	ATLASSERT(i >= 1);
	EmitInstruction(0xDE,BYTE(0xE8 + i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d),st(0)\r\n","fsubp",i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FMULP
| 描  述 : 实数乘法：ST(i)←ST(i)*ST(0)，执行一次出栈操作
| 参  数 : DWORD i->浮点数寄存器编号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FMULP(DWORD i)
{
	EmitInstruction(0xDE,ModRM(MODE11,1,i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d),st(0)\r\n","fmulp",i);
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FMUL_M64REAL
| 描  述 : 实数乘法：ST(0)←ST(0)*m64real
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FMUL_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,1,RBP),NONE_SIB,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fmul"
			, (pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FIDIV_M64REAL
| 描  述 : 实数除法：ST(0)←ST(0)/m64real
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FDIV_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,6,RBP),NONE_SIB
		,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fdiv"
			,(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FDIVR_M64REAL
| 描  述 : 被整数除：ST(0)←m32int/ST(0)
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FDIVR_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,7,RBP),NONE_SIB,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fdivr"
			,(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FDIVP
| 描  述 : 实数除法：ST(i)←ST(i)/ST(0)，执行一次出栈操作
| 参  数 :DWORD i->浮点数寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FDIVP(DWORD i)
{
	ATLASSERT(i > 0);
	EmitInstruction(0xDE,ModRM(MODE11,7,i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d),st(0)\r\n","fdivp",i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FILD
| 描  述 : 把一个整数(局部变量)移动到浮点数寄存器中
| 参  数 : int nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FILD(int nRelativeDisplacement,BIT_TYPE nBitType)
{
	EmitInstruction(PREFIX_INVALID,(BT_BIT32 == nBitType)? 0xDB:0xDF,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10
		,(BT_BIT32 == nBitType)? 0 : 5,RBP),NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [ebp %+d]\r\n" 
			,"fild",(BT_BIT32 == nBitType)? "dword":"qword",nRelativeDisplacement);
	}
}



/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLD
| 描  述 : 将ST(i)压栈，即装入ST(0)
| 参  数 : int nIndex->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLD(int nIndex)
{
	EmitInstruction(0XD9,BYTE(0xC0 + nIndex));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d)\r\n","fld",nIndex);
	}
}




/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSTP_INDIRECT
| 描  述 : 把浮点数寄存器中的数移动到寄存器所指示的变量中
| 参  数 : COMMON_REGISTERS nDstReg->寄存器编号
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType/* = BT_BIT32*/)
{
	if(RSP == nDstReg)     //单独处理
	{
		EmitInstruction((BT_BIT32 == nBitType)? 0xD9:0xDD,ModRM(MODE00, 3,RSP),(BYTE)0x24);
	}
	else
	{
		EmitInstruction((BT_BIT32 == nBitType)? 0xD9:0xDD,ModRM(MODE00, 3,nDstReg));
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [%s]\r\n" ,"fstp",(BT_BIT32 == nBitType)? "dword":"qword"
			,RegName(nDstReg));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FISTP_INDIRECT
| 描  述 : 把浮点数寄存器中的数移动到寄存器所指示的变量中
| 参  数 : COMMON_REGISTERS nDstReg->寄存器编号
| 参  数 : BOOL nBitType->浮点数是32位或64位
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FISTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType/* = BT_BIT32*/)
{

	if(RSP == nDstReg)     //单独处理
	{
		EmitInstruction((BT_BIT32 == nBitType)? 0xDB : 0xDF,ModRM(MODE00, (BT_BIT32 == nBitType)?3:7,RSP),(BYTE)0x24);
	}
	else
	{
		EmitInstruction((BT_BIT32 == nBitType)? 0xDB : 0xDF,ModRM(MODE00, (BT_BIT32 == nBitType)?3:7,nDstReg));
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [%s]\r\n" ,"fistp",(BT_BIT32 == nBitType)? "dword":"qword"
			,RegName(nDstReg));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSTP
| 描  述 : 将ST(0)复制到ST(i)，执行一次出栈操作
| 参  数 : int nIndex->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTP(int nIndex)
{
	EmitInstruction(0xDD,BYTE(0xD8 + nIndex));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d)\r\n","fstp",nIndex);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FCOMP_M64REAL
| 描  述 : 实数比较：ST(0)-m64real，设置标志位，执行一次出栈操作
| 参  数 : DWORD nDisplacement->浮点立即数在印像文件中的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FCOMP_M64REAL(CSyntaxTreeNode* pValueNode)
{
	EmitInstruction(PREFIX_INVALID,0xDC,ModRM(MODE00,3,RBP),NONE_SIB,pValueNode->GetOffset(),0,0,TRUE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s qword ptr (%f)[(%Xh)]\r\n","fcomp"
			,(pValueNode->GetValue1() + pValueNode->GetValue2())
			,pValueNode->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FCOMP
| 描  述 : 实数比较：ST(0)-ST(i)，设置标志位，执行一次出栈操作
| 参  数 : DWORD i->浮点数寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FCOMP(DWORD i)
{
	EmitInstruction(0xD8,BYTE(0xD8 + i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s ST(%d)\r\n","fcomp", i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FCOMPP
| 描  述 : 实数比较：ST(0)-ST(i)，设置标志位，执行两次出栈操作
| 参  数 : DWORD i->浮点数寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FCOMPP(DWORD i)
{
	EmitInstruction(0xDE,(BYTE)(0xD8 + i));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s ST(%d)\r\n","fcompp", i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLDZ
| 描  述 : 设置C1 (C0, C2, C3未定义)   将+0.0压栈，即装入ST(0)
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLDZ()
{
	EmitInstruction(0xd9,(BYTE)0xEE);
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s\r\n","fldz");
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FUCOMPP
| 描  述 : 比较ST(0)和ST(1)，执行一次出栈操作
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FUCOMPP(int i)
{
	EmitInstruction(0xDA,(BYTE)(0xE8 + i));
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s st(%d)\r\n","fucompp",i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FUCOMP
| 描  述 : 比较ST(0)和ST(1)，执行一次出栈操作
| 参  数 : DWORD i->浮点数寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FUCOMP(int i)
{
	EmitInstruction(0xdd,(BYTE)(0xE8 + 1));
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s std(%d)\r\n","fucomp",i);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FNSTSW
| 描  述 : 将FPU状态字保存到AX，不检查非屏蔽浮点异常
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FNSTSW(CContext &objContext)
{
	EmitInstruction(0xdf,(BYTE)0xE0);
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s ax\r\n","fnstsw");
	}
	objContext.OnRegChanged(RAX);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FCHS
| 描  述 : 浮点数正负求反
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FCHS()
{
	EmitInstruction(0xD9,(BYTE)0xE0);
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s ax\r\n","fchs");
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FNSTCW
| 描  述 : 将FPU控制字保存到m2byte，不检查非屏蔽浮点异常
| 参  数 : int nRelativeDisplacement->相对于RBP的偏移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FNSTCW(int nRelativeDisplacement)
{
	EmitInstruction(PREFIX_INVALID,0xD9,ModRM((abs(nRelativeDisplacement) <= 127) ? MODE01: MODE10,7,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s word ptr [ebp %+d]\r\n","fnstcw",nRelativeDisplacement);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLDCW
| 描  述 : 从m2byte装入FPU控制字
| 参  数 : int nRelativeDisplacement->相对于RBP的偏移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLDCW(int nRelativeDisplacement)
{
	EmitInstruction(PREFIX_INVALID,0xD9,ModRM((abs(nRelativeDisplacement) <= 127) ? MODE01: MODE10,5,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s word ptr [ebp %+d]\r\n","fldcw",nRelativeDisplacement);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FISTP
| 描  述 : 从m2byte装入FPU控制字
| 参  数 : int nRelativeDisplacement->相对于RBP的偏移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FISTP(int nRelativeDisplacement)
{
	EmitInstruction(PREFIX_INVALID,0xDB,ModRM((abs(nRelativeDisplacement) <= 127) ? MODE01: MODE10,3,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+d]\r\n","fistp",nRelativeDisplacement);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FISTP
| 描  述 : 从m2byte装入FPU控制字
| 参  数 : int nRelativeDisplacement->相对于RBP的偏移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FIST(int nRelativeDisplacement,BIT_TYPE nBitType)
{
	EmitInstruction(PREFIX_INVALID,(nBitType == BT_BIT32)? 0xDF:0xDB,ModRM((abs(nRelativeDisplacement) <= 127) ? MODE01: MODE10,2,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+d]\r\n","fist",nRelativeDisplacement);
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::AND_IMM32
| 描  述 : Performs a logical AND of the two operands replacing the destination with the result. 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : DWORD nValue->32立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::AND_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{   
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,4,nDstReg),NONE_SIB,0,nValue,1,FALSE);
	}
	else
	{
		if(RAX == nDstReg)
		{
			EmitInstruction(0x25,CalculateComplement(nValue));
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,4,nDstReg),NONE_SIB,0,nValue,4,FALSE);
		}
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %xh \r\n","and",RegName(nDstReg),nValue);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::AND
| 描  述 : Performs a logical AND of the two operands replacing the destination with the result. 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::AND(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{   
	EmitInstruction(0x23,ModRM(MODE11,nDstReg,nSrcReg));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %s \r\n","and",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::ADD
| 描  述 : Adds "src" to "dest" and replacing the original contents of "dest".
Both operands are binary. 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ADD(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{   
	EmitInstruction(0x03,ModRM(MODE11,nDstReg,nSrcReg)); 

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %s \r\n","add",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::ADD_IMM32
| 描  述 : 把指定寄存器中的值机加上一个立即数
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : int nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ADD_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,0,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg) //单独处理
		{
			EmitInstruction(0x05,CalculateComplement(nValue)); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,0,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %X \r\n","add",RegName(nDstReg),nValue);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::ADC_IMM32
| 描  述 : 把指定寄存器中的值机加上一个立即数
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : DWORD nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ADC_IMM32(COMMON_REGISTERS nDstReg,DWORD nValue,CContext &objContext)
{
	if(nValue <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,2,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg)
		{
			EmitInstruction(0x15,nValue); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,2,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %X \r\n","adc",RegName(nDstReg),nValue);
	}

	objContext.OnRegChanged(nDstReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::INC
| 描  述 : Adds one to destination unsigned binary operand. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::INC(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction((BYTE)(0x40 + nReg)); 

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n","inc",RegName(nReg));
	}

	objContext.OnRegChanged(nReg);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SUB
| 描  述 : The source is subtracted from the destination and the result is
stored in the destination. 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SUB(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{   
	EmitInstruction(0x2B,ModRM(MODE11,nDstReg,nSrcReg)); 

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %s \r\n","sub",RegName(nDstReg),RegName(nSrcReg));
	}

	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SUB
| 描  述 : 把指定寄存器中的值机减去一个变量中的值
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : DWORD nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SUB(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0x2B,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [ebp %+d]\r\n" 
			,"sub",RegName(nDstReg),nRelativeDisplacement);
	}

	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SUB_IMM32
| 描  述 : 把指定寄存器中的值机减去一个立即数
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : int nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SUB_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,5,nDstReg),NONE_SIB,0,nValue,1,FALSE);
	}
	else
	{
		if(RAX == nDstReg) //单独处理
		{
			EmitInstruction(0x2D,CalculateComplement(nValue)); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,5,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}

	objContext.OnRegChanged(nDstReg);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %Xh \r\n","sub",RegName(nDstReg),nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SBB
| 描  述 : 把指定寄存器中的值机减去一个变量中的值
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : DWORD nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SBB(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0x1B,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s ptr [ebp %+d]\r\n","sbb",RegName(nDstReg),nRelativeDisplacement);
	}

	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SBB_IMM32
| 描  述 : Subtracts the source from the destination, and subtracts 1 extra if
the Carry Flag is set. Results are returned in "dest".
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : DWORD nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SBB_IMM32(COMMON_REGISTERS nDstReg,DWORD nValue,CContext &objContext)
{
	if(nValue < 127) //单独处理
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,3,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg)
		{
			EmitInstruction(0x1d,nValue); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,3,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}
	objContext.OnRegChanged(nDstReg);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s, %Xh \r\n","sbb",RegName(nDstReg),nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::NEG
| 描  述 : Subtracts the destination from 0 and saves the 2s complement of
|       "dest" back into "dest". 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::NEG(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,3,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"neg",RegName(nReg));
	}

	objContext.OnRegChanged(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SBB
| 描  述 : SSubtracts the source from the destination, and subtracts 1 extra if
the Carry Flag is set. Results are returned in "dest". 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/

void CGenerateSaxProgram::SBB(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{
	EmitInstruction(0x1b,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n" ,"neg",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::XOR_IMM32
| 描  述 : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : int nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::XOR_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,6,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg)
		{
			EmitInstruction(0x35,CalculateComplement(nValue)); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,6,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"xor",RegName(nDstReg),nValue);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::XOR
| 描  述 : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::XOR(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{
	EmitInstruction(0x33,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n" ,"xor",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::OR
| 描  述 : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::OR(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{
	EmitInstruction(0x0b,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n" ,"or",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::OR_IMM32
| 描  述 : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : int nValue->立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::OR_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,1,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg)
		{
			EmitInstruction(0x0D,CalculateComplement(nValue)); 
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,1,nDstReg),NONE_SIB,0,nValue,4,FALSE); 
		}
	}
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"or",RegName(nDstReg),nValue);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::NOT
| 描  述 : Inverts the bits of the "dest" operand forming the 1s complement. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::NOT(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,2,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"not",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IMUL
| 描  述 : Signed multiplication of accumulator by "src" with result placed
in the accumulator. If the source operand is a byte value, it
is multiplied by AL and the result stored in AX. If the source
operand is a word value it is multiplied by AX and the result is
stored in DX:AX. Other variations of this instruction allow
specification of source and destination registers as well as a
third immediate factor. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::IMUL(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0xAF,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n","imul",RegName(nDstReg),RegName(nSrcReg));
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IMUL
| 描  述 : Signed multiplication of accumulator by "src" with result placed
in the accumulator. If the source operand is a byte value, it
is multiplied by AL and the result stored in AX. If the source
operand is a word value it is multiplied by AX and the result is
stored in DX:AX. Other variations of this instruction allow
specification of source and destination registers as well as a
third immediate factor. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 测  试 : 已测试
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::IMUL(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,int nValue,CContext &objContext)
{
	if(0 == nValue)
	{
		MOV_REG_IMM32(nDstReg,0,objContext);
	}
	else if(1 == nValue)
	{
		if(nDstReg != nSrcReg)
		{
			MOV(nDstReg,nSrcReg,objContext);
		}
	}
	else
	{
		int nIndex = CalculationIndex(nValue);
		if(-1 != nIndex)
		{
			if(nDstReg != nSrcReg)
			{
				MOV(nDstReg,nSrcReg,objContext);
			}
			SHL(nDstReg,(BYTE)nIndex,objContext);
		}
		else
		{
			if(abs(nValue) < 127)
			{
				EmitInstruction(PREFIX_INVALID,0x6b,ModRM(MODE11,nDstReg,nSrcReg),NONE_SIB,0,nValue,1,FALSE);
			}
			else
			{
				EmitInstruction(PREFIX_INVALID,0x69,ModRM(MODE11,nDstReg,nSrcReg),NONE_SIB,0,nValue,4,FALSE); 
			}
			if(m_bEnableMasm)
			{
				GenerateMasmCmd("%-10s %s,%s,%+d\r\n" ,"imul",RegName(nDstReg),RegName(nSrcReg),nValue);
			}
		}
	}

	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IMUL
| 描  述 : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::IMUL(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,5,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"imul",RegName(nReg));
	}

	objContext.OnRegChanged(RAX);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MUL
| 描  述 : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MUL(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,4,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"mul",RegName(nReg));
	}

	objContext.OnRegChanged(RAX);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MUL
| 描  述 : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| 参  数 :int nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MUL(int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0xF7,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,4,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+dh]\r\n","mul",nRelativeDisplacement);
	}

	objContext.OnRegChanged(RAX);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::DEC
| 描  述 : Unsigned binary subtraction of one from the destination. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::DEC(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction((BYTE)(0x48 + nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"dec",RegName(nReg));
	}

	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::DIV
| 描  述 : Converts signed DWORD in EAX to a signed quad word in EDX:EAX by
extending the high order bit of EAX throughout EDX 
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CDQ(CContext &objContext)
{
	EmitInstruction(0x99); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s\r\n","cdq");
	}
	objContext.OnRegChanged(RDX);

}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::DIV
| 描  述 : Unsigned binary division of accumulator by source. If the source
divisor is a byte value then AX is divided by "src" and the quotient
is placed in AL and the remainder in AH. If source operand is a word
value, then DX:AX is divided by "src" and the quotient is stored in AX
and the remainder in DX. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::DIV(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,6,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"div", RegName(nReg));
	}

	objContext.OnRegChanged(RAX);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IDIV
| 描  述 : Signed binary division of accumulator by source. If source is a
byte value, AX is divided by "src" and the quotient is stored in
AL and the remainder in AH. If source is a word value, DX:AX is
divided by "src", and the quotient is stored in AL and the
remainder in DX. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::IDIV(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xF7,ModRM(MODE11,7,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"idiv",RegName(nReg));
	}

	objContext.OnRegChanged(RAX);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHL
| 描  述 : 逻辑左移imm8次(乘法：r/m16=r/m16*(2^imm8))
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : DWORD nValue->左移的次数
| 备  注 : 已经测试
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHL(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0xC1,ModRM(MODE11,4,nReg),NONE_SIB,0,nValue,1,FALSE); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%x\r\n" ,"shl",RegName(nReg),nValue);
	}
	objContext.OnRegChanged(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHL
| 描  述 : 逻辑左移CL次(乘法：r/m16=r/m16*(2^CL))
| 参  数 : COMMON_REGISTERS nReg1->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHL(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xD3,ModRM(MODE11,4,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,cl\r\n" ,"shl",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHLD
| 描  述 : SHLD shifts "dest" to the left "count" times and the bit positions
opened are filled with the most significant bits of "src"
| 参  数 : COMMON_REGISTERS nReg1->寄存器序号
| 参  数 : COMMON_REGISTERS nReg2->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHLD(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0xA5,ModRM(MODE11,nReg2,nReg1)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s,cl\r\n" ,"shld",RegName(nReg1),RegName(nReg2));
	}

	objContext.OnRegChanged(nReg1);
	objContext.OnRegChanged(nReg2);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHR
| 描  述 : 逻辑右移imm8次(无符号除法：r/m32=r/m32 / (2^imm8))
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : DWORD nValue->右移的次数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0xC1,ModRM(MODE11,5,nReg),NONE_SIB,0,nValue,1,FALSE); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("shr   %s,%x\r\n" ,RegName(nReg),nValue);
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHR
| 描  述 : 逻辑右移imm8次(无符号除法：r/m32=r/m32 / (2^imm8))
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHR(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xD3,ModRM(MODE11,5,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,cl\r\n" ,"shr",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SHRD
| 描  述 : shifts "dest" to the right "count" times and the bit positions
opened are filled with the least significant bits of the second
operand. Only the 5 lower bits of "count" are used. 
| 参  数 : COMMON_REGISTERS nReg1->寄存器序号
| 参  数 : COMMON_REGISTERS nReg2->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SHRD(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0xAD,ModRM(MODE11,nReg2,nReg1)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s,cl\r\n" ,"shrd",RegName(nReg1),RegName(nReg2));
	}

	objContext.OnRegChanged(nReg1);
	objContext.OnRegChanged(nReg2);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::RCR
| 描  述 : 带进位循环右移imm8次
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : DWORD nValue->右移的次数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::RCR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0xC1,ModRM(MODE11,3,nReg),NONE_SIB,0,nValue,1,FALSE); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%x\r\n" ,"rcr",RegName(nReg),nValue);
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SAR
| 描  述 : Shifts the destination right by "count" bits with the current sign
bit replicated in the leftmost bit. The Carry Flag contains the
last bit shifted out. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SAR(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(0xD3,ModRM(MODE11,7,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,cl\r\n" ,"sar",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SAR
| 描  述 : Shifts the destination right by "count" bits with the current sign
bit replicated in the leftmost bit. The Carry Flag contains the
last bit shifted out. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SAR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0xC1,ModRM(MODE11,7,nReg),NONE_SIB,0,nValue,1,FALSE); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"sar",RegName(nReg),nValue);
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETE
| 描  述 : Sets the byte in the operand to 1 if the Zero Flag is set,
otherwise sets the operand to 0. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETE(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x94,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"sete",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETNE
| 描  述 : SETNE/SETNZ are different mnemonics for the same instruction 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETNE(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x95,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"sete",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETG
| 描  述 : Sets the byte in the operand to 1 if the Zero Flag is set,
otherwise sets the operand to 0. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETG(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x9F,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"setg",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETGE
| 描  述 : Sets the byte in the operand to 1 if the Sign Flag equals the
Overflow Flag, otherwise sets the operand to 0. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETGE(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x9D,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"setge",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETL
| 描  述 : Sets the byte in the operand to 1 if the Sign Flag is not equal
to the Overflow Flag, otherwise sets the operand to 0. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETL(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x9C,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"setl",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETLE
| 描  述 : Sets the byte in the operand to 1 if the Zero Flag is set or the
Sign Flag is not equal to the Overflow Flag, otherwise sets the
operand to 0. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SETLE(COMMON_REGISTERS nReg,CContext &objContext)
{
	EmitInstruction(PREFIX_LOCK,0x9E,ModRM(MODE11,0,nReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n" ,"setle",RegName(nReg));
	}
	objContext.OnRegChanged(nReg);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETE
| 描  述 : Performs a logical AND of the two operands updating the flags
register without saving the result. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : BYTE nValue->被测试的值
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::TEST(COMMON_REGISTERS nReg,int nValue)
{
	ATLASSERT(nReg < RSP);
	if(RAX == nReg)
	{
		EmitInstruction(0xA9,CalculateComplement(nValue)); 
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0xF7,ModRM(MODE11,0,nReg),NONE_SIB,0,nValue,4,FALSE); 
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"test",COMMON_REGISTER_NAMESX8[nReg],nValue);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::TEST_BIT8
| 描  述 : 比较寄存器其中8位的值,当bLow为真时nReg是al,bl,cl,dl
|       否则是ah,bh,ch,dh
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : int nValue->判断值
| 参  数 : BOOL bLow->寄存器类型
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::TEST_BIT8(COMMON_REGISTERS nReg,int nValue,BOOL bLow/* = TRUE*/)
{
	ATLASSERT(nReg <= RBX);

	if(bLow)
	{
		if(RAX == nReg)
		{
			EmitInstruction(0xA8,(BYTE)CalculateComplement(nValue));
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0xF6,ModRM(MODE11,0,nReg),NONE_SIB,0,nValue,1,FALSE);
		}
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0xF6,ModRM(MODE11,0,4 | nReg),NONE_SIB,0,nValue,1,FALSE);
	}


	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n","test",bLow? COMMON_REGISTER_NAMESX8[nReg]:COMMON_REGISTER_NAMESX8Hi[nReg],nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SETE
| 描  述 : Performs a logical AND of the two operands updating the flags
register without saving the result. 
| 参  数 : COMMON_REGISTERS nDstReg->寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::TEST(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg)
{
	EmitInstruction(0x85,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n" ,"test",RegName(nDstReg),RegName(nSrcReg));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CMP_IMM8
| 描  述 : 寄存器中的值与一个8位立即数比较
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : BYTE nValue->8位立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CMP_IMM8(COMMON_REGISTERS nReg,BYTE nValue)
{
	if(AL == nReg)
	{
		EmitInstruction(0x3C,nValue); 
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0x80,ModRM(MODE11,7,nReg),NONE_SIB,0,nValue,1,FALSE); 
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"cmp",COMMON_REGISTER_NAMESX8[nReg],nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CMP_IMM32
| 描  述 : 寄存器中的值与一个32位立即数比较
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : BYTE nValue->32位立即数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CMP_IMM32(COMMON_REGISTERS nReg,int nValue)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,7,nReg),NONE_SIB,0,nValue,1,FALSE);
	}
	else
	{
		if(RAX == nReg)
		{
			EmitInstruction(0x3D,CalculateComplement(nValue));
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x81,ModRM(MODE11,7,nReg),NONE_SIB,0,nValue,4,FALSE);
		}
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n" ,"cmp",RegName(nReg),nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CMP
| 描  述 : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 :int nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CMP(COMMON_REGISTERS nReg,int nRelativeDisplacement)
{
	EmitInstruction(PREFIX_INVALID,0x3b,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,dword ptr [ebp %+d]\r\n","cmp",RegName(nReg),nRelativeDisplacement);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CMP
| 描  述 : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| 参  数 : COMMON_REGISTERS nReg1->寄存器序号
| 参  数 : COMMON_REGISTERS nReg2->寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CMP(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2)
{   
	EmitInstruction(0x3b,ModRM(MODE11,nReg1,nReg2));

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n","cmp",RegName(nReg1),RegName(nReg2));
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CMP_BIT8
| 描  述 : 比较寄存器其中8位的值,当bLow为真时nReg是al,bl,cl,dl
|       否则是ah,bh,ch,dh
| 参  数 : COMMON_REGISTERS nReg->寄存器序号
| 参  数 : int nValue->判断值
| 参  数 : BOOL bLow->寄存器类型
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CMP_BIT8(COMMON_REGISTERS nReg,int nValue,BOOL bLow/* = TRUE*/)
{
	ATLASSERT(nReg <= RBX);

	if(bLow)
	{
		if(RAX == nReg)
		{
			EmitInstruction(0x3C,(BYTE)CalculateComplement(nValue));
		}
		else
		{
			EmitInstruction(PREFIX_INVALID,0x80,ModRM(MODE11,7,nReg),NONE_SIB,0,nValue,1,FALSE);
		}
	}
	else
	{
		EmitInstruction(PREFIX_INVALID,0x80,ModRM(MODE11,7,4 + nReg),NONE_SIB,0,nValue,1,FALSE);
	}


	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%d\r\n","cmp",bLow? COMMON_REGISTER_NAMESX8[nReg]:COMMON_REGISTER_NAMESX8Hi[nReg],nValue);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JMP_IMM32
| 描  述 : 产生无条件跳转指令
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 测试:    没测试向前跳
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JMP_IMM32(int nRelativeDisplacement,CContext &/*objContext*/)
{  
	if(abs(nRelativeDisplacement) < 127)
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 2; //如果是向前跳,减去jmp指令自身的长度2
		}
		EmitInstruction(0xEB,(BYTE)CalculateComplement(nRelativeDisplacement));
	}
	else
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 5; //如果是向前跳,减去jmp指令自身的长度5
		}

		EmitInstruction(0xE9,CalculateComplement(nRelativeDisplacement));
	}


	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %+d\r\n" ,"jmp",nRelativeDisplacement);
	}
}

const char* JMP_INSTRUCTION_NAME[] = {"jo","jno","jb","jnb","je","jne","jbe","ja"
	,"js","jns","jp","jnp","jl","jnl","jng","jg"};

void CGenerateSaxProgram::JMP_BASE(JUM_INSTRUCTION_CODE nInst1,int nRelativeDisplacement)
{
	if(abs(nRelativeDisplacement) < 127)
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 2; //如果是向前跳,减去jmp指令自身的长度2
		}
		EmitInstruction((BYTE)nInst1,(BYTE)CalculateComplement(nRelativeDisplacement));
	}
	else
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 6; //如果是向前跳,减去jmp指令自身的长度6
		}
		EmitInstruction(PREFIX_LOCK,(BYTE)(nInst1 + 0x10),CalculateComplement(nRelativeDisplacement));
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %+d\r\n" ,JMP_INSTRUCTION_NAME[nInst1 - 0x70],nRelativeDisplacement);
	}

}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JE
| 描  述 : Causes execution to branch to "label" if the Zero Flag is clear or
the Sign Flag equals the Overflow Flag. Signed comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JE(int nRelativeDisplacement,CContext &/*objContext*/)
{	
	JMP_BASE(JIC_JE,nRelativeDisplacement);	
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JNE
| 描  述 : Causes execution to branch to "label" if the Zero Flag is clear. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNE,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JGE
| 描  述 : Causes execution to branch to "label" if the Sign Flag equals
the Overflow Flag. Signed comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JGE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNL,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JC
| 描  述 : Causes execution to branch to "label" if the Carry Flag is set.
Functionally similar to JB and JNAE. Unsigned comparison. Causes execution to
branch to "label" if the Carry Flag is set.Functionally similar to JB and JNAE. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JC(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JA
| 描  述 : Causes execution to branch to "label" if the Carry Flag and Zero Flag
are both clear. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JA(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JA,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JB
| 描  述 :Causes execution to branch to "label" if the Carry Flag is set.
Functionally similar to JC. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JB(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JBE
| 描  述 : Causes execution to branch to "label" if the Carry Flag or
the Zero Flag is set. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JBE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JBE,nRelativeDisplacement);

}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JNS
| 描  述 : Causes execution to branch to "label" if the Sign Flag is clear. Signed comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNS(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNS,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JAE
| 描  述 : Causes execution to branch to "label" if the Carry Flag is clear.
Functionally similar to JNC. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JAE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JG
| 描  述 : Causes execution to branch to "label" if the Zero Flag is clear or
the Sign Flag equals the Overflow Flag. Signed comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JG(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JG,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JL
| 描  述 : Causes execution to branch to "label" if the Sign Flag is not equal
to Overflow Flag. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JL(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JL,nRelativeDisplacement);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JP
| 描  述 : Causes execution to branch to "label" if the Parity Flag is set. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JP(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JP,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JNP
| 描  述 : Causes execution to branch to "label" if the Parity
|       Flag is clear. Unsigned comparison. 
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNP(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNP,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::JNP
| 描  述 : 不大于跳转
| 参  数 : int nRelativeDisplacement->相对于当前跳转指令的位移
|       往前跳时负数
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNG(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNG,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::LEA
| 描  述 : 得到一个局部变量的偏移地址并放到指定的寄存器
| 参  数 : COMMON_REGISTERS nDstReg->寄存器编号
| 参  数 : int nRelativeDisplacement->局部变量相对于RBP寄存器的位移
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::LEA(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext)
{
	EmitInstruction(PREFIX_INVALID,0x8D,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,nDstReg,RBP)
		,NONE_SIB,nRelativeDisplacement,0,0,FALSE);

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,[ebp %+d]\r\n" ,"lea",RegName(nDstReg),nRelativeDisplacement);
	}
	objContext.OnRegChanged(nDstReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::XCHG
| 描  述 : Exchanges contents of source and destination. 
| 参  数 : COMMON_REGISTERS nDstReg->目标寄存器序号
| 参  数 : COMMON_REGISTERS nSrcReg->源寄存器序号
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::XCHG(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext)
{
	EmitInstruction(0x87,ModRM(MODE11,nDstReg,nSrcReg)); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s,%s\r\n","xchg",RegName(nDstReg),RegName(nSrcReg));
	}

	objContext.OnRegChanged(nDstReg);
	objContext.OnRegChanged(nSrcReg);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::RET
| 描  述 : 产生返回命令
| 修改记录 : 2014-5-2 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/

void CGenerateSaxProgram::RET()
{
	EmitInstruction(0xC3); 
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("ret\r\n");
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MovIntValtoReg
| 描  述 : 移动一个变量(包括简单便量、数组引用、指针引用)中的值到指定的寄存器中
| 参  数 : COMMON_REGISTERS nDstReg——目的寄存器
| 参  数 : CSyntaxTreeNode* pValue——必须是简单变量
| 参  数 : COMMON_REGISTERS nExtDstReg——扩展目的寄存器
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MovIntValtoReg(CSyntaxTreeNode* pValue
	,COMMON_REGISTERS nSrcReg,COMMON_REGISTERS nExtDstReg,CContext &objContext)
{
	if(objContext.GetCurVar() == pValue && RAX == nSrcReg) 
	{//变量的值已经在寄存器中了不需要重新加载
		return;
	}

	ATLASSERT(pValue);
	ATLASSERT(CR_INVALID != nSrcReg);
	if(pValue->IsNumericConstants())
	{
		MOV_REG_IMM32(nSrcReg,pValue->GetValue1() & 0xFFFFFFFF,objContext);
		if(pValue->GetValue1() > 0xFFFFFFFF)
		{
			MOV_REG_IMM32(nExtDstReg,pValue->GetValue1() >> 32,objContext);
		}
		return;
	}

	CDataTypeTreeNode* pDataTypeNode = pValue->GetDataType();
#ifdef _DEBUG
	if(pDataTypeNode->GetSize() == 8)
	{
		ATLASSERT(CR_INVALID != nExtDstReg);
	}
#endif
	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValue;
	if(pValue->GetNodeType() == TNK_ARRAY_REF
		|| pValue->GetNodeType() == TNK_VAR_REF)   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValue)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	const int     nIndex    = CalculationIndex(pDataTypeNode->GetSize());
	CStringA      strInstName = "mov"; 
	INSTRUCTION_PREFIX nInstPrefix = PREFIX_INVALID;
	BYTE        nOpCode   = 0;

	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:
		nInstPrefix = PREFIX_LOCK;
		nOpCode   = 0xB6;
		strInstName = "movzx"; 
		break;

	case TNK_CHAR_TYPE:
		nInstPrefix = PREFIX_LOCK;
		nOpCode   = 0xBE;
		strInstName = "movsx";
		break;    

	case TNK_SHORT_TYPE:
		nInstPrefix = PREFIX_LOCK;
		nOpCode   = 0xBF;
		strInstName = "movsx";
		break;   

	case TNK_WORD_TYPE:
		nInstPrefix = PREFIX_LOCK;
		nOpCode   = 0xB7;
		strInstName = "movzx";
		break;   

	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           //这些类型作为参数转递的时候,在栈中都是占用四个字节
	case TNK_POINTER_TYPE:
	case TNK_LONGLONG_TYPE:
	case TNK_REFERENCE_TYPE:
		nOpCode = 0x8B;
		strInstName = "mov";
		break;

	case TNK_FLOAT_TYPE:
		nOpCode = 0xD9;
		strInstName = "fld";
		break;

	case TNK_DOUBLE_TYPE:
		nOpCode = 0xDD;
		strInstName = "fld";
		break;

	default:
		ATLASSERT(FALSE);
	}

	if(pValue->GetNodeType() == TNK_INDIRECT_REF)   //指针引用比如a = *b;
	{
		UINT nStatus = 0;
		COMMON_REGISTERS nAddrReg = RequestReg(CRM_RBX,FALSE,objContext,nStatus);

		MovIntValtoReg(((CExpressionTreeNode*)pValue)->GetChildNode(0),nAddrReg,CR_INVALID,objContext);

		EmitInstruction((BYTE)nInstPrefix,nOpCode
			,ModRM(MODE00,nSrcReg,nAddrReg),NONE_SIB,0,0,0,FALSE);
		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %s,%s ptr [%s]\r\n"
				,strInstName,RegName(nSrcReg),GetDataType(pDataTypeNode->GetSize()),RegName(nAddrReg));
		}

		if(8 == pDataTypeNode->GetSize())
		{       
			EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE01,nExtDstReg,nAddrReg)
				,NONE_SIB,4,0,0,FALSE);

			if(m_bEnableMasm)
			{
				GenerateMasmCmd("%-10s %s,%s ptr [%s + 4]\r\n"
					,strInstName,RegName(nExtDstReg),GetDataType(pDataTypeNode->GetSize()),RegName(nAddrReg));
			}
		}
		FreeReg(nAddrReg,objContext,nStatus);
	}
	else
	{
		BOOL bOutputMsg = TRUE;
		BOOL bHasEmited = FALSE;
		if(pValue->GetNodeType() == TNK_ARRAY_REF)
		{			
			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pValue)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pValue)->GetChildNode(0);

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);			

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面
					RequestReg(CRM_RCX,FALSE,objContext,nStatus);
					MOV(RCX,RAX,objContext);

					EmitInstruction((BYTE)nInstPrefix,nOpCode
						,ModRM(MODE00,nSrcReg,SIB_ADDR),SIB(nIndex,RCX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s,%s ptr [rcx * %d + rbx]\r\n"
							,strInstName,RegName(nSrcReg),GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
					}

					if(8 == pDataTypeNode->GetSize())
					{       
						EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE01,nExtDstReg,SIB_ADDR)
							,SIB(nIndex,RCX,RBX),4,0,0,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s,%s ptr [rcx * %d + rbx + 4]\r\n"
								,strInstName,RegName(nExtDstReg),GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
						}
					}
					FreeReg(RCX,objContext,nStatus);    //和请求的顺序相反
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;

					EmitInstruction((BYTE)nInstPrefix,nOpCode
						,ModRM(MODE_TYPE,nSrcReg,RBX),NONE_SIB,OFFSET,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s,%s ptr [rbx + %d]\r\n"
							,strInstName,RegName(nSrcReg),GetDataType(pDataTypeNode->GetSize()),OFFSET);
					}
				
					if(8 == pDataTypeNode->GetSize())
					{    
						MODE_TYPE = (OFFSET + 4)? MODE01:MODE10; 
						EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,nExtDstReg,RBX)
							,NONE_SIB,OFFSET + 4,0,0,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s,%s ptr [rbx + %d]\r\n"
								,strInstName,RegName(nExtDstReg),GetDataType(pDataTypeNode->GetSize()),OFFSET + 4);
						}
					}						
				}
				FreeReg(RBX,objContext,nStatus);

				bHasEmited = TRUE;	
				bOutputMsg = FALSE;
			}
			else
			{				
				if(pArryIndex->IsNumericConstants())
				{
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移       
				}
				else
				{
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面

					UINT nStatus = 0;
					COMMON_REGISTERS nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);

					if(pDataTypeNode->GetSize() == 1)
					{
						if(bStatic)
						{
							UINT nDispPos = EmitInstruction((BYTE)nInstPrefix,nOpCode
								,ModRM(MODE10,nSrcReg,RCX),NONE_SIB,OFFSET,0,0,FALSE);
							AppendRef(GetOffset() - nDispPos);
						}
						else
						{
							EmitInstruction((BYTE)nInstPrefix,nOpCode
								,ModRM(MODE_TYPE,nSrcReg,SIB_ADDR),SIB(nIndex,RCX,RBP),OFFSET,0,0,FALSE);
						}
					}
					else
					{
						EmitInstruction((BYTE)nInstPrefix,nOpCode
							,ModRM(MODE_TYPE,nSrcReg,SIB_ADDR),SIB(nIndex,RCX,RBP),OFFSET,0,0,bStatic);

						if(pDataTypeNode->GetSize() == 8)
						{
							OFFSET += 4;

							EmitInstruction((BYTE)nInstPrefix,nOpCode
								,ModRM(MODE_TYPE,nExtDstReg,SIB_ADDR),SIB(nIndex,RCX,RBP),OFFSET,0,0,bStatic);
						}
					}
					FreeReg(nFreeReg,objContext,nStatus);

					bHasEmited = TRUE;
				}
			}
		}

		if(!bHasEmited)
		{
			EmitInstruction((BYTE)nInstPrefix,nOpCode
				,ModRM(MODE_TYPE,nSrcReg,RBP),NONE_SIB,OFFSET,0,0,bStatic);

			if(pDataTypeNode->GetSize() == 8)
			{
				OFFSET += 4;

				EmitInstruction((BYTE)nInstPrefix,nOpCode
					,ModRM(MODE_TYPE,nExtDstReg,RBP),NONE_SIB,OFFSET,0,0,bStatic);
			}
		}

		if(bOutputMsg && m_bEnableMasm)
		{
			if(!bStatic)  //非静态变量 
			{
				GenerateMasmCmd("%-10s %s,%s ptr (%s)[rbp %+d]\r\n"
					,strInstName,RegName(nSrcReg),GetDataType(pDataTypeNode->GetSize())
					,pVariableDeclNode->GetSymbol(),pValue->GetOffset());
			}
			else
			{
				GenerateMasmCmd("%-10s %s,%s ptr [%s]\r\n"
					,strInstName,RegName(nSrcReg),GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol());
			}
		}
	}  

	objContext.SetCurVar(pValue);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::MovIntValtoReg
| 描  述 : 移动指定的寄存器中的值到一个变量(包括简单变量，数组引用
|       ，指针引用)中
| 参  数 : CSyntaxTreeNode* pVariableNode——目标变量
| 参  数 : COMMON_REGISTERS nSrcReg——源寄存器
| 参  数 : COMMON_REGISTERS nExtSrcReg——源寄存器
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MovRegtoIntVal(CSyntaxTreeNode* pVariableNode
	,COMMON_REGISTERS nSrcReg,COMMON_REGISTERS nExtSrcReg,CContext &objContext)
{
	ATLASSERT(pVariableNode);
	ATLASSERT(CR_INVALID != nSrcReg);

	CDataTypeTreeNode* pDataTypeNode = pVariableNode->GetDataType();
#ifdef _DEBUG
	if(pDataTypeNode->GetSize() == 8)
	{
		ATLASSERT(CR_INVALID != nExtSrcReg);
	}
#endif

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pVariableNode;
	if(pVariableNode->GetNodeType() == TNK_ARRAY_REF //说明是数组引用,比如: Test[9]
		|| pVariableNode->GetNodeType() == TNK_VAR_REF)   
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	const UINT DATA_SIZE = pDataTypeNode->GetSize();
	const int nIndex   = CalculationIndex(pDataTypeNode->GetSize());

	if(pVariableNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		UINT nStatus = 0;
		COMMON_REGISTERS nAddrReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);

		MovIntValtoReg(((CExpressionTreeNode*)pVariableNode)->GetChildNode(0),nAddrReg,CR_INVALID,objContext);    

		EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
			,(1 == DATA_SIZE)?0x88 : 0x89
			,ModRM(MODE00,nSrcReg,nAddrReg));

		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %s ptr [%s], %s\r\n"
				,"mov",GetDataType(DATA_SIZE),RegName(nAddrReg),RegName(nSrcReg));
		}

		if(8 == DATA_SIZE)
		{       
			EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
				,(1 == DATA_SIZE)?0x88 : 0x89
				,ModRM(MODE01,nExtSrcReg,nAddrReg)
				,NONE_SIB,4,0,0,FALSE);

			if(m_bEnableMasm)
			{
				GenerateMasmCmd("%-10s %s ptr [%s + 4], %s\r\n"
					,"mov",GetDataType(DATA_SIZE),RegName(nAddrReg),RegName(nSrcReg));
			}
		}
		FreeReg(nAddrReg,objContext,nStatus);
	}
	else
	{
		BOOL bHasEmited = FALSE;
		if(pVariableNode->GetNodeType() == TNK_ARRAY_REF)
		{
			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pVariableNode)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);			

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{		
					UINT nStatus = 0;
					COMMON_REGISTERS nTmpReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);					
					if(RAX == nSrcReg || RAX == nExtSrcReg)
					{
						MOV(nTmpReg,RAX,objContext); 
					}
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面

					EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
						,(1 == DATA_SIZE)?0x88 : 0x89
						,ModRM(MODE00,(RAX == nSrcReg)? nTmpReg:nSrcReg,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx],%s\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),RegName(nSrcReg));
					}

					if(8 == pDataTypeNode->GetSize())
					{       
						EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
							,(1 == DATA_SIZE)?0x88 : 0x89,ModRM(MODE01,(RAX == nExtSrcReg)? nTmpReg:nExtSrcReg,SIB_ADDR)
							,SIB(nIndex,RAX,RBX),4,0,0,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx + 4],%s\r\n"
								,"mov",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),RegName(nExtSrcReg));
						}
					}
					FreeReg(RCX,objContext,nStatus);					
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;

					EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
						,(1 == DATA_SIZE)?0x88 : 0x89
						,ModRM(MODE_TYPE,nSrcReg,RBX),NONE_SIB,OFFSET,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rbx + %d],%s\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),OFFSET,RegName(nSrcReg));
					}					

					if(8 == pDataTypeNode->GetSize())
					{    
						MODE_TYPE = (OFFSET + 4)? MODE01:MODE10; 
						EmitInstruction(BYTE((2 == DATA_SIZE)? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
							,(1 == DATA_SIZE)?0x88 : 0x89,ModRM(MODE_TYPE,nExtSrcReg,RBX)
							,NONE_SIB,OFFSET + 4,0,0,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rbx + %d],%s\r\n"
								,"mov",GetDataType(pDataTypeNode->GetSize()),OFFSET + 4,RegName(nExtSrcReg));
						}
					}						
				}

				FreeReg(RBX,objContext,nStatus);
				bHasEmited = TRUE;			
			}
			else
			{
				if(!pArryIndex->IsNumericConstants())
				{
					objContext.ReserveReg(nSrcReg);
					objContext.ReserveReg(nExtSrcReg);

					UINT nStatus = 0;
					COMMON_REGISTERS nTmpReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);					
					if(RAX == nSrcReg || RAX == nExtSrcReg)
					{
						MOV(nTmpReg,RAX,objContext); 
					}

					GenerateExprStatementsCode(pArryIndex,objContext);           //计算数组索引下标，放到EAX寄存器里面

					objContext.FreeReg(nSrcReg);
					objContext.FreeReg(nExtSrcReg);

					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0x88 : 0x89
						,ModRM(MODE_TYPE,(RAX == nSrcReg)? nTmpReg:nSrcReg,SIB_ADDR),SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic);   

					if(DATA_SIZE == 8)
					{
						OFFSET += 4;

						EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
							,(DATA_SIZE == 1)? 0x88 : 0x89
							,ModRM(MODE_TYPE,(RAX == nExtSrcReg)? nTmpReg : nExtSrcReg,SIB_ADDR),SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic);   
					}		
					FreeReg(nTmpReg,objContext,nStatus);

					bHasEmited = TRUE;
				}
				else
				{
					OFFSET += (int)(pArryIndex->GetValue1() * DATA_SIZE);    //直接计算出便宜
				}
			}
		}

		if(!bHasEmited)
		{
			if(RAX == nSrcReg && bStatic)      //把寄存器RAX中的值移动到静态变量中需要单独处理
			{
				EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
					,(DATA_SIZE == 1)? 0xa2 : 0xa3
					,(DWORD)OFFSET);
				AppendRef(GetOffset() - 4);
			}
			else
			{
				EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
					,(DATA_SIZE == 1)? 0x88 : 0x89
					,ModRM(MODE_TYPE,nSrcReg,RBP),NONE_SIB,OFFSET,0,0,bStatic);   
			}

			if(DATA_SIZE == 8)
			{
				OFFSET += 4;
				if(RAX == nExtSrcReg && bStatic)    //把寄存器RAX中的值移动到静态变量中需要单独处理
				{
					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0xa2 : 0xa3
						,(DWORD)OFFSET);
					AppendRef(GetOffset() - 4);
				}
				else
				{
					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0x88 : 0x89
						,ModRM(MODE_TYPE,nExtSrcReg,RBP),NONE_SIB,OFFSET,0,0,bStatic);   
				}   
			}			
		}  

		if(m_bEnableMasm)
		{
			if(!bStatic)  //非静态变量 
			{
				GenerateMasmCmd("%-10s %s ptr (%s)[rbp %+d],%s\r\n"
					,"mov",GetDataType(DATA_SIZE),pVariableDeclNode->GetSymbol(),pVariableDeclNode->GetOffset(),RegName(nSrcReg));
			}
			else
			{
				GenerateMasmCmd("%-10s %s ptr [%s],%s\r\n"
					,"mov",GetDataType(DATA_SIZE),pVariableDeclNode->GetSymbol(),RegName(nSrcReg));
			}
		}
	}   

	objContext.SetCurVar(pVariableNode);
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::Offset
| 描  述 : 获得某个变量的偏移地址并放到另一个变量中
| 参  数 : CDeclarationTreeNode* pVariableNode1——存放偏移地址的变量
| 参  数 : CDeclarationTreeNode* pVariableNode2——被求偏移地址的变量
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::Offset(CSyntaxTreeNode* pVariableNode1,CSyntaxTreeNode* pVariableNode2)
{
	ATLASSERT(pVariableNode1&& pVariableNode2);

	if(((CDeclarationTreeNode*)pVariableNode1)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MOV_LIV32_OFFSET(pVariableNode1->GetOffset(),pVariableNode2->GetOffset());
	}
	else
	{
		MOV_GIV32_OFFSET(pVariableNode1->GetOffset(),pVariableNode2->GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SetIntVariable
| 描  述 : 设置整形变量的值
| 参  数 : CDeclarationTreeNode* pVariableNode——被设置的变量
| 参  数 : DWORD nValue——将设置的变量的值
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SetIntVariable(CSyntaxTreeNode* pVariableNode
	,LONGLONG nValue,CContext& objContext)
{
	ATLASSERT(pVariableNode);

	CDataTypeTreeNode* pDataTypeNode = pVariableNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pVariableNode;
	if(pVariableNode->GetNodeType() == TNK_ARRAY_REF
		|| pVariableNode->GetNodeType() == TNK_VAR_REF)   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}

	const int nIMMBytes = (pDataTypeNode->GetSize() == 8)? 4 : pDataTypeNode->GetSize();
	const int nIndex  = CalculationIndex(pDataTypeNode->GetSize());
	BYTE nSIB      = NONE_SIB;
	BYTE nModRm     = ModRM(MODE_TYPE,0,RBP);

	if(pVariableNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		MovIntValtoReg(((CExpressionTreeNode*)pVariableNode)->GetChildNode(0),RAX,CR_INVALID,objContext);

		EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
			,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
			,ModRM(MODE00,0,RAX)
			,NONE_SIB
			,0
			,(int)(nValue & 0xFFFFFFFF)
			,nIMMBytes,FALSE);

		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %s ptr [rax], %d\r\n"
				,"mov",GetDataType(pDataTypeNode->GetSize()),nValue & 0xFFFFFFFF);
		}

		if(8 == pDataTypeNode->GetSize())
		{       
			EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
				,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
				,ModRM(MODE01,0,RAX)
				,NONE_SIB
				,4
				,(int)(nValue >> 32)
				,nIMMBytes,FALSE);

			if(m_bEnableMasm)
			{
				GenerateMasmCmd("%-10s %s ptr [rax + 4], %d\r\n"
					,"mov",GetDataType(pDataTypeNode->GetSize()),nValue >> 32);
			}
		}
	}
	else
	{
		BOOL bHasEmited = FALSE;
		if(pVariableNode->GetNodeType() == TNK_ARRAY_REF)
		{
			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pVariableNode)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);			

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面

					EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
						,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
						,ModRM(MODE00,0,SIB_ADDR),SIB(nIndex,RAX,RBX),0,(int)(nValue & 0xFFFFFFFF)
						,nIMMBytes,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx],%+d\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),nValue & 0xFFFFFFFF);
					}

					if(8 == pDataTypeNode->GetSize())
					{       
						EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
							,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
							,ModRM(MODE01,0,SIB_ADDR)
							,SIB(nIndex,RAX,RBX),4,(int)(nValue >> 32)
							,nIMMBytes,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx + 4],%+d\r\n"
								,"mov",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),nValue >> 32);
						}
					}
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10; 
					
					EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
						,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
						,ModRM(MODE_TYPE,0,RBX),NONE_SIB,OFFSET,nValue & 0xFFFFFFFF,nIMMBytes,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rbx + %d],%+d\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),OFFSET,nValue & 0xFFFFFFFF);
					}					

					if(8 == pDataTypeNode->GetSize())
					{    
						MODE_TYPE = (OFFSET + 4)? MODE01:MODE10; 
						EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
							,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
							,ModRM(MODE_TYPE,0,RBX)
							,NONE_SIB,OFFSET + 4,nValue >> 32,nIMMBytes,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rbx + %d],%+d\r\n"
								,"mov",GetDataType(pDataTypeNode->GetSize()),OFFSET + 4,nValue >> 32);
						}
					}				
				}

				FreeReg(RBX,objContext,nStatus);
				bHasEmited = TRUE;			
			}
			else
			{
				if(pArryIndex->IsNumericConstants())
				{
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出便宜
				}
				else
				{
					GenerateExprStatementsCode(pArryIndex,objContext);           //计算数组索引下标，放到EAX寄存器里面
					nSIB  = SIB(nIndex,RAX,RBP);
					nModRm = ModRM(MODE_TYPE,0,SIB_ADDR);
				}
			}
		}

		if(!bHasEmited)
		{
			UINT nDispPos = EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
				,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
				,nModRm
				,nSIB
				,OFFSET
				,(int)(nValue & 0xFFFFFFFF)
				,nIMMBytes,bStatic);

			if(8 == pDataTypeNode->GetSize())
			{
				if(m_bEnableMasm)
				{
					if(!bStatic)
					{
						GenerateMasmCmd("%-10s %s ptr (%s)[ebp %+d], %d\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol(),OFFSET,nValue);
					}
					else
					{
						GenerateMasmCmd("%-10s %s ptr [%s], %d\r\n"
							,"mov",GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol(),nValue);
					}
				}

				OFFSET += 4;
				nDispPos = EmitInstruction(BYTE((2 == pDataTypeNode->GetSize())? PREFIX_OPERAND_SIZE:PREFIX_INVALID)
					,(1 == pDataTypeNode->GetSize())?0xc6 : 0xc7
					,nModRm
					,nSIB
					,OFFSET
					,(int)(nValue >> 32)
					,nIMMBytes,bStatic);
			}

			if(m_bEnableMasm)
			{
				if(!bStatic)
				{
					GenerateMasmCmd("%-10s %s ptr (%s)[ebp %+d], %d\r\n"
						,"mov",GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol(),OFFSET,nValue);
				}
				else
				{
					GenerateMasmCmd("%-10s %s ptr [%s], %d\r\n"
						,"mov",GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol(),nValue);
				}
			}
		} 
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLDVariable
| 描  述 : 把指定变量中的数字装载到浮点数寄存器中
| 参  数 : CSyntaxTreeNode* pValueNode——变量节点
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLDVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF 
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}

	if(pValueNode->IsNumericConstants())  //是常量
	{
		EmitInstruction(PREFIX_INVALID,0xDD,ModRM(MODE00,0, RBP),0,OFFSET,0,0,TRUE);

		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %f\r\n" ,"fld",pValueNode->GetValue1() + pValueNode->GetValue2());
		}
	}
	else
	{
		switch(pDataTypeNode->GetNodeType())
		{
		case TNK_BYTE_TYPE:
		case TNK_BOOLEAN_TYPE: 
		case TNK_CHAR_TYPE:
		case TNK_SHORT_TYPE:
		case TNK_WORD_TYPE:
		case TNK_DWORD_TYPE:
			MovIntValtoReg(pValueNode,RAX,RDX,objContext);
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
			FILD(-(objContext.GetOffset() + 4));        //装载到浮点数寄存器中
			break;   

		default:
			{
				CStringA strInst = "fld";
				BYTE nOpCode  = 0xDB;
				BYTE ModRM_REG = 0;
				switch(pDataTypeNode->GetNodeType())
				{
				case TNK_LONGLONG_TYPE:
					nOpCode = 0xDF;
					ModRM_REG = 5;
					strInst = "fild";
					break;

				case TNK_INT_TYPE:
				case TNK_LONG_TYPE:
					nOpCode = 0xDB;
					strInst = "fild";
					break;

				case TNK_FLOAT_TYPE:
					nOpCode = 0xD9;
					break;

				case TNK_DOUBLE_TYPE:
					nOpCode = 0xDD;
					break;   

				default:
					ATLASSERT(FALSE);
				}

				if(pValueNode->GetNodeType() == TNK_INDIRECT_REF)
				{
					MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //把内存地址放到RAX寄存器中
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,RAX),0,0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [eax]\r\n",strInst,GetDataType(pDataTypeNode->GetSize()));
					}
				}
				else
				{
					BOOL bHasEmited = FALSE;
					if(pValueNode->GetNodeType() == TNK_ARRAY_REF)
					{
						const int nIndex  = CalculationIndex(pDataTypeNode->GetSize());

						CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pValueNode)->GetChildNode(1);
						CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pValueNode)->GetChildNode(0);

						if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
						{
							UINT nStatus = 0;
							RequestReg(CRM_RBX,FALSE,objContext,nStatus);			

							MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

							if(!pArryIndex->IsNumericConstants())
							{					
								GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面
								EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
								if(m_bEnableMasm)
								{
									GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
										,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
						
								}							
							}
							else
							{
								OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
								if(OFFSET > 0)
								{
									MODE_TYPE = (OFFSET)? MODE01:MODE10; 
								}
								EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBX)
									,NONE_SIB,OFFSET,0,0,FALSE);
								
								if(m_bEnableMasm)
								{
									GenerateMasmCmd("%-10s %s ptr [rbx + %d]\r\n"
										,strInst,GetDataType(pDataTypeNode->GetSize()),OFFSET);
								}											
							}

							FreeReg(RBX,objContext,nStatus);
							bHasEmited = TRUE;
						}
						else
						{
							if(!pArryIndex->IsNumericConstants())
							{
								GenerateExprStatementsCode(pArryIndex,objContext);           //计算数组索引下标，放到EAX寄存器里面
								EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,SIB_ADDR)
									,SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic);
								if(m_bEnableMasm)
								{
									GenerateMasmCmd("%-10s %s ptr [rax * %d + rbp + %d]\r\n"
										,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),OFFSET);
								}	

								bHasEmited = TRUE;
							}
							else
							{
								OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出便宜
							}
						}
					}

					if(!bHasEmited)
					{
						EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBP)
							,NONE_SIB,OFFSET,0,0,bStatic); 
						if(m_bEnableMasm)
						{
							if(!bStatic)
							{
								GenerateMasmCmd("%-10s %s ptr (%s)[ebp %+d]\r\n" 
									,strInst,GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol(),OFFSET);
							}
							else
							{
								GenerateMasmCmd("%-10s %s ptr [%s]\r\n"
									,strInst,GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol());
							}
						}
					} 					
				}
			}
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FLDVariable
| 描  述 : 将ST(0)复制到m64real或m32real变量中，执行一次出栈操作
| 参  数 : CSyntaxTreeNode* pValueNode——变量节点
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTPVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF )   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	BYTE nOpCode  = 0;
	BYTE ModRM_REG = 0;
	CStringA strInst = "fistp";
	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_BYTE_TYPE:
	case TNK_CHAR_TYPE:
		FNSTCW(-(objContext.GetOffset() + 2));         //将FPU控制字保存到栈中
		MOVZX_REG_LIV16(RAX,-(objContext.GetOffset() + 2),objContext);  //再将FPU控制字移动到RAX寄存器中
		OR_IMM32(RAX,0x0C00,objContext);                 //修改RC(舍入控制)11(超0方向截断)http://book.51cto.com/art/200907/135136.htm
		MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);   //
		FLDCW(-(objContext.GetOffset() + 8));         //重新装入控制字
		FISTP(-(objContext.GetOffset() + 8));         //将ST(0)以32位整数保存到m32int，执行一次出栈操作
		MOV_REG_LIV8(RDX,-(objContext.GetOffset() + 8),objContext);
		FLDCW(-(objContext.GetOffset() + 2)); 
		MovRegtoIntVal(pValueNode,RAX,RDX,objContext);
		return;

	case TNK_BOOLEAN_TYPE:                   //不理解意思
		{
			FLDZ();
			FUCOMPP();
			FNSTSW(objContext);
			MOV_REG_IMM32(RDX,1,objContext);
			TEST_BIT8(RAX,0x44);
			JP(0,objContext);
			UINT nRefPos = GetOffset();
			MOV_REG_IMM32(RDX,0,objContext);
			WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
			MovRegtoIntVal(pValueNode,RDX,CR_INVALID,objContext);        
			return;
		}

	case TNK_WORD_TYPE:
	case TNK_SHORT_TYPE:
		nOpCode  = 0xDF;
		ModRM_REG = 3;
		break;

	case TNK_DWORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
		nOpCode  = 0xDB;
		ModRM_REG = 3;
		break;   

	case TNK_LONGLONG_TYPE:
		nOpCode  = 0xDF;
		ModRM_REG = 7;
		break;

	case TNK_FLOAT_TYPE:
		nOpCode  = 0xD9;
		ModRM_REG = 3;
		strInst  = "fstp";
		break;

	case TNK_DOUBLE_TYPE:
		nOpCode  = 0xDD;
		ModRM_REG = 3;
		strInst  = "fstp";
		break;   

	default:
		ATLASSERT(FALSE);
	}

	if(pValueNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //把内存地址放到RAX寄存器中
		EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,RAX),0,0,0,0,FALSE);

		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %s ptr [eax]\r\n",strInst,GetDataType(pDataTypeNode->GetSize()));
		}
	}
	else
	{
		BOOL bHasEmited = FALSE;
		if(pValueNode->GetNodeType() == TNK_ARRAY_REF)
		{
			const int nIndex       = CalculationIndex(pDataTypeNode->GetSize());
			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pValueNode)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pValueNode)->GetChildNode(0);

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);	
				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
					}							
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					if(OFFSET > 0)
					{
						MODE_TYPE = (OFFSET)? MODE01:MODE10; 
					}
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBX)
						,NONE_SIB,OFFSET,0,0,FALSE);

					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rbx + %d]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),OFFSET);
					}											
				}

				FreeReg(RBX,objContext,nStatus);
				bHasEmited = TRUE;
			}
			else
			{
				if(!pArryIndex->IsNumericConstants())
				{
					GenerateExprStatementsCode(pArryIndex,objContext);           //计算数组索引下标，放到EAX寄存器里面

					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,SIB_ADDR)
						,SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic); 
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbp + %d]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),OFFSET);
					}	

					bHasEmited = TRUE;
				}
				else
				{
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出便宜
				}
			}
		}

		if(!bHasEmited)
		{
			EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBP),NONE_SIB,OFFSET,0,0,bStatic); 
			if(m_bEnableMasm)
			{
				if(!bStatic)
				{
					GenerateMasmCmd("%-10s %s ptr (%s)[ebp %+d]\r\n" 
						,strInst,pVariableDeclNode->GetSymbol(),GetDataType(pDataTypeNode->GetSize()),OFFSET);
				}
				else
				{
					GenerateMasmCmd("%-10s %s ptr [%s]\r\n"
						,strInst,GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol());
				}
			}
		} 		
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FSTVariable
| 描  述 : 将ST(0)复制到m64real或m32real变量中
| 参  数 : CSyntaxTreeNode* pValueNode——变量节点
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	BYTE nOpCode     = 0;
	const BYTE ModRM_REG = 2;
	CStringA strInst   = "fist";
	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_BYTE_TYPE:
	case TNK_CHAR_TYPE:
		FNSTCW(-(objContext.GetOffset() + 2));         //将FPU控制字保存到栈中
		MOVZX_REG_LIV16(RAX,-(objContext.GetOffset() + 2),objContext);  //再将FPU控制字移动到RAX寄存器中
		OR_IMM32(RAX,0x0C00,objContext);                 //修改RC(舍入控制)11(超0方向截断)http://book.51cto.com/art/200907/135136.htm
		MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);   //
		FLDCW(-(objContext.GetOffset() + 8));         //重新装入控制字
		FIST(-(objContext.GetOffset() + 8));          //将ST(0)以32位整数保存到m32int，执行一次出栈操作
		MOV_REG_LIV8(RDX,-(objContext.GetOffset() + 8),objContext);
		FLDCW(-(objContext.GetOffset() + 2)); 
		MovRegtoIntVal(pValueNode,RAX,RDX,objContext);
		return;

	case TNK_BOOLEAN_TYPE:                   //不理解意思
		{
			FLDZ();
			FUCOMP(1);
			FNSTSW(objContext);
			MOV_REG_IMM32(RDX,1,objContext);
			TEST_BIT8(RAX,0x44);
			JP(0,objContext);
			UINT nRefPos = GetOffset();
			MOV_REG_IMM32(RDX,0,objContext);
			WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
			MovRegtoIntVal(pValueNode,RDX,CR_INVALID,objContext);
			return;
		}

	case TNK_WORD_TYPE:
	case TNK_SHORT_TYPE:
		nOpCode  = 0xDF;
		//ModRM_REG = 2;
		break;

	case TNK_DWORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_LONGLONG_TYPE:
		nOpCode  = 0xDB;
		//ModRM_REG = 2;
		break;

	case TNK_FLOAT_TYPE:
		nOpCode  = 0xD9;
		//ModRM_REG = 2;
		strInst  = "fst";
		break;

	case TNK_DOUBLE_TYPE:
		nOpCode  = 0xDD;
		//ModRM_REG = 2;
		strInst  = "fst";
		break;   

	default:
		ATLASSERT(FALSE);
	}

	if(pValueNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //把内存地址放到RAX寄存器中
		EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,RAX),NONE_SIB,0,0,0,FALSE);

		if(m_bEnableMasm)
		{
			GenerateMasmCmd("%-10s %s ptr [eax]\r\n",strInst,GetDataType(pDataTypeNode->GetSize()));
		}

		if(pDataTypeNode->GetSize() == 8 && pDataTypeNode->GetNodeType() & TNK_LONGLONG_TYPE)
		{
			EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE01,ModRM_REG,RAX),NONE_SIB,4,0,0,FALSE);

			if(m_bEnableMasm)
			{
				GenerateMasmCmd("%-10s %s ptr [eax + 4]\r\n",strInst,GetDataType(pDataTypeNode->GetSize()));
			}
		}
	}
	else
	{
		BOOL bHasEmited = FALSE;
		if(pValueNode->GetNodeType() == TNK_ARRAY_REF)
		{
			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pDataTypeNode)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pValueNode)->GetChildNode(0);
			const int nIndex       = CalculationIndex(pDataTypeNode->GetSize());

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);	
				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //计算数组索引下标，放到EAX寄存器里面
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
					}							
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					if(OFFSET > 0)
					{
						MODE_TYPE = (OFFSET)? MODE01:MODE10; 
					}
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBX)
						,NONE_SIB,OFFSET,0,0,FALSE);

					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rbx + %d]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),OFFSET);
					}											
				}

				FreeReg(RBX,objContext,nStatus);
				bHasEmited = TRUE;
			}
			else
			{
				if(!pArryIndex->IsNumericConstants())
				{
					GenerateExprStatementsCode(pArryIndex,objContext);           //计算数组索引下标，放到EAX寄存器里面

					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,SIB_ADDR)
						,SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic);   

					if(pDataTypeNode->GetSize() == 8 && pDataTypeNode->GetNodeType() & TNK_LONGLONG_TYPE)
					{
						OFFSET += 4;
						EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,SIB_ADDR)
							,SIB(nIndex,RAX,RBP),OFFSET,0,0,bStatic);
					}

					bHasEmited = TRUE;
				}
				else
				{
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出便宜
				}
			}
		}

		if(!bHasEmited)
		{
			EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBP),NONE_SIB,OFFSET,0,0,bStatic);     
			if(pDataTypeNode->GetSize() == 8 && pDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
			{
				OFFSET += 4;
				EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE_TYPE,ModRM_REG,RBP),NONE_SIB,OFFSET,0,0,bStatic);   
			}
		}   

		if(m_bEnableMasm)
		{
			if(!bStatic)
			{
				GenerateMasmCmd("%-10s %s ptr (%s)[ebp %+d]\r\n" 
					,strInst,pVariableDeclNode->GetSymbol(),GetDataType(pDataTypeNode->GetSize()),OFFSET);
			}
			else
			{
				GenerateMasmCmd("%-10s %s ptr [%s]\r\n"
					,strInst,GetDataType(pDataTypeNode->GetSize()),pVariableDeclNode->GetSymbol());
			}
		}
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::CompareIntVariable
| 描  述 : 判断某个变量的值
| 参  数 : CSyntaxTreeNode* pValueNode——变量节点
| 参  数 : LONGLONG nValue——被检测的值
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CompareIntVariable(CSyntaxTreeNode* pValueNode,LONGLONG nValue,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //说明是数组引用,比如: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
	BOOL bStatic = TRUE;   //默认寻址是为静态变量
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	const UINT DATA_SIZE = pDataTypeNode->GetSize();

	UINT nRefPos = 0;

	if(pValueNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,CR_INVALID,objContext);  //把内存地址放到RAX寄存器中

		if(DATA_SIZE == 8)
		{
			EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
				,(DATA_SIZE == 1)? 0x80: 0x81
				,ModRM(MODE01,7,RAX),NONE_SIB,4,nValue>>32,4,FALSE);
			JNE(0,objContext);                        //跳过下面的指令
			nRefPos = GetOffset();
		}

		EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
			,(DATA_SIZE == 1)? 0x80: 0x81
			,ModRM(MODE00,7,RAX),NONE_SIB,0,nValue & 0xFFFFFFFF,(DATA_SIZE >= 8)? 4:DATA_SIZE,FALSE);

		if(DATA_SIZE == 8)
		{
			WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
		}
	}
	else
	{
		BOOL bHasEmited = FALSE;
		if(pValueNode->GetNodeType() == TNK_ARRAY_REF)
		{
			const int nIndex  = CalculationIndex(DATA_SIZE);

			CSyntaxTreeNode* pArryIndex = ((CExpressionTreeNode*)pValueNode)->GetChildNode(1);
			CSyntaxTreeNode* pSrcVarNode = ((CExpressionTreeNode*)pValueNode)->GetChildNode(0);

			if(pSrcVarNode->GetDataType()->GetNodeType() == TNK_POINTER_TYPE)
			{
				UINT nStatus = 0;
				RequestReg(CRM_RBX,FALSE,objContext,nStatus);			

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext);   //把基础地址移动到RBX寄存器中

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);   //计算数组索引下标，放到EAX寄存器里面
					
					if(DATA_SIZE == 8)
					{
						EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
							,(DATA_SIZE == 1)? 0x80: 0x81
							,ModRM(MODE01,7,SIB_ADDR),SIB(nIndex,RAX,RBX),4,nValue>>32,4,FALSE);

						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx + 4],%+d\r\n"
								,"cmp",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),nValue>>32);
						}

						JNE(0,objContext);                        //跳过下面的指令
						nRefPos = GetOffset();
					}

					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0x80: 0x81
						,ModRM(MODE00,7,SIB_ADDR),SIB(nIndex,RAX,RBX),0,nValue & 0xFFFFFFFF,(DATA_SIZE >= 8)? 4:DATA_SIZE,FALSE);

					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx],%+d\r\n"
							,"cmp",GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize(),nValue & 0xFFFFFFFF);
					}

					if(DATA_SIZE == 8)
					{
						WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
					}

				}
				else
				{
					OFFSET = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //直接计算出偏移
					
					if(DATA_SIZE == 8)
					{
						MODE_TYPE = (OFFSET + 4)? MODE01:MODE10; 
						EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
							,(DATA_SIZE == 1)? 0x80: 0x81
							,ModRM(MODE_TYPE,7,RBX),NONE_SIB,4,nValue>>32,4,FALSE);
						if(m_bEnableMasm)
						{
							GenerateMasmCmd("%-10s %s ptr [rbx %+d],%+d\r\n"
								,"cmp",GetDataType(pDataTypeNode->GetSize()),OFFSET + 4,nValue>>32);
						}

						JNE(0,objContext);                        //跳过下面的指令
						nRefPos = GetOffset();

						MODE_TYPE = MODE00;
					}

					if(OFFSET != 0)
					{
						MODE_TYPE = (OFFSET)? MODE01:MODE10; 
					}
					
					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0x80: 0x81
						,ModRM(MODE_TYPE,7,RBX),NONE_SIB,OFFSET,nValue & 0xFFFFFFFF,(DATA_SIZE >= 8)? 4:DATA_SIZE,FALSE);

					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rbx %+d],%+d\r\n"
							,"cmp",GetDataType(pDataTypeNode->GetSize()),OFFSET,nValue & 0xFFFFFFFF);
					}

					if(DATA_SIZE == 8)
					{
						WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
					}					
				}
				
				FreeReg(RBX,objContext,nStatus);
				bHasEmited = TRUE;
			}
			else
			{
				if(pArryIndex->IsNumericConstants())
				{
					OFFSET += (int)(pArryIndex->GetValue1() * DATA_SIZE);    //直接计算出偏移       
				}
				else
				{					
					GenerateExprStatementsCode(pArryIndex,objContext); 

					if(DATA_SIZE == 8)
					{
						EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
							,(DATA_SIZE == 1)? 0x80: 0x81
							,ModRM(MODE_TYPE,7,SIB_ADDR),SIB(nIndex,RAX,RBP),OFFSET + 4,nValue>>32,4,bStatic);

						if(m_bEnableMasm)
						{
							if(bStatic)
							{
								GenerateMasmCmd("%-10s %s ptr [%s + 4],%+d\r\n","cmp"
									,GetDataType(DATA_SIZE),pValueNode->GetSymbol(),nValue);
							}
							else
							{
								GenerateMasmCmd("%-10s %s ptr [ebp %+d],%+d\r\n","cmp"
									,GetDataType(DATA_SIZE),OFFSET + 4,nValue);
							}
						}

						JNE(0,objContext);                        //跳过下面的指令
						nRefPos = GetOffset();
					}

					EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
						,(DATA_SIZE == 1)? 0x80: 0x81
						,ModRM(MODE_TYPE,7,SIB_ADDR),SIB(nIndex,RAX,RBP),OFFSET,nValue & 0xFFFFFFFF,(DATA_SIZE >= 8)? 4:DATA_SIZE,bStatic);

					if(m_bEnableMasm)
					{
						if(bStatic)
						{
							GenerateMasmCmd("%-10s %s ptr [%s],%+d\r\n","cmp"
								,GetDataType(DATA_SIZE),pValueNode->GetSymbol(),nValue);
						}
						else
						{
							GenerateMasmCmd("%-10s %s ptr [ebp %+d],%+d\r\n","cmp"
								,GetDataType(DATA_SIZE),OFFSET + 4,nValue);
						}
					}

					if(DATA_SIZE == 8)
					{
						WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
					}

					bHasEmited = TRUE;
				}
			}
		}

		if(!bHasEmited)
		{
			if(DATA_SIZE == 8)
			{
				EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
					,(DATA_SIZE == 1)? 0x80: 0x81
					,ModRM(MODE_TYPE,7,RBP),NONE_SIB,OFFSET + 4,nValue>>32,4,bStatic);

				if(m_bEnableMasm)
				{
					if(bStatic)
					{
						GenerateMasmCmd("%-10s %s ptr [%s + 4],%+d\r\n","cmp"
							,GetDataType(DATA_SIZE),pValueNode->GetSymbol(),nValue);
					}
					else
					{
						GenerateMasmCmd("%-10s %s ptr [ebp %+d],%+d\r\n","cmp"
							,GetDataType(DATA_SIZE),OFFSET + 4,nValue);
					}
				}

				JNE(0,objContext);                        //跳过下面的指令
				nRefPos = GetOffset();
			}

			EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
				,(DATA_SIZE == 1)? 0x80: 0x81
				,ModRM(MODE_TYPE,7,RBP),NONE_SIB,OFFSET,nValue & 0xFFFFFFFF,(DATA_SIZE >= 8)? 4:DATA_SIZE,bStatic);

			if(DATA_SIZE == 8)
			{
				WriteByteData(nRefPos - 1,(BYTE)(GetOffset() - nRefPos),objContext); 
			} 

			if(m_bEnableMasm)
			{
				if(bStatic)
				{
					GenerateMasmCmd("%-10s %s ptr [%s],%+d\r\n","cmp"
						,GetDataType(DATA_SIZE),pValueNode->GetSymbol(),nValue);
				}
				else
				{
					GenerateMasmCmd("%-10s %s ptr [ebp %+d],%+d\r\n","cmp"
						,GetDataType(DATA_SIZE),pValueNode->GetOffset(),nValue);
				}
			}         
		}
	}

	

	SETE(RAX,objContext);
	AND_IMM32(RAX,0xFF,objContext);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FloatingCmpCommonCode
| 描  述 : 浮点数比较通用代码
| 参  数 : CSyntaxTreeNode* pParantExprNode——比较表达是的父节点
| 参  数 : TREE_NODE_TYPE nOperator——运算类型
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FloatingCmpCommonCode(CSyntaxTreeNode* /*pParantExprNode*/
	,UINT nOperator,CContext& objContext)
{
	CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
	ATLASSERT(pConditionExprJmpRef);
	JUM_INSTRUCTION_CODE nInstCode = JIC_UNKNOWN;

	switch(nOperator)
	{
	case TNK_LT_EXPR:                   //"<"
		TEST_BIT8(RAX,0x41);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNE : JIC_JE;
		break;

	case TNK_NLT_EXPR:                  //"<="
		TEST_BIT8(RAX,0x1);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNE : JIC_JE;
		break;

	case TNK_GT_EXPR:                  //">"
		TEST_BIT8(RAX,0x5);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JP : JIC_JNP;
		break;

	case TNK_NGT_EXPR:                 //">="
		TEST_BIT8(RAX,0x41);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JP : JIC_JNP;
		break;

	case TNK_EQ_EXPR:                  //"=="
		TEST_BIT8(RAX,0x44);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JP : JIC_JNP;
		break;

	case TNK_NEQ_EXPR:                 //"!="
		TEST_BIT8(RAX,0x44);
		nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNP : JIC_JP;
		break;
	}
	JMP_BASE(nInstCode,TMP_ADDR_BYTE4);
	pConditionExprJmpRef->AppendRefPos(pConditionExprJmpRef->GetJmpType(),GetOffset());  //只记录失败跳转的位置

//	MOV_REG_IMM32(RAX,1,objContext);
//	JMP_IMM32(0,objContext);
//	UINT nEndPos = GetOffset();
////False:
//	WriteByteData(nFalsePos - 1,(BYTE)(GetOffset() - nFalsePos),objContext);
//	XOR(RAX,RAX,objContext);
////End:
//	WriteByteData(nEndPos - 1,(BYTE)(GetOffset() - nEndPos),objContext);	
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IntegerOperator
| 描  述 : 处理运算结果是浮点数的运算,结构存到浮点数寄存器
| 参  数 : CSyntaxTreeNode* pLeftExprNode——左边操作数
| 参  数 : CSyntaxTreeNode* pRightExprNode——右边操作数
| 参  数 : TREE_NODE_TYPE nOperator——运算类型
| 测试注意 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FloatingOperator(CSyntaxTreeNode* pLeftExprNode
	,CSyntaxTreeNode* pRightExprNode,CExpressionTreeNode* pParantExprNode,CContext& objContext)
{
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	UINT nOperator = pParantExprNode->GetNodeType();

	switch(pLeftNodeDataType->GetNodeType())
	{
	case TNK_CHAR_TYPE:
	case TNK_BOOLEAN_TYPE: 
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		if(pRightExprNode->IsNumericConstants())
		{
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
			FILD(-(objContext.GetOffset() + 4));        //装载到浮点数寄存器中

			switch(nOperator) 
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pRightExprNode);      //结果是浮点数
				break;

			case TNK_MINUS_EXPR:
				FSUB_M64REAL(pRightExprNode);      //结果是浮点数
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pRightExprNode);      //结果是浮点数
				break;

			case TNK_TRUNC_DIV_EXP:
				FDIV_M64REAL(pRightExprNode);
				break;

			default:
				FCOMP_M64REAL(pRightExprNode);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);		
			}
		}
		else if(pLeftExprNode->IsNumericConstants())
		{
			GenerateExprStatementsCode(pRightExprNode,objContext); //肯定是浮点数
			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pLeftExprNode);      //结果是浮点数
				break;

			case TNK_MINUS_EXPR:
				FSUBR_M64REAL(pLeftExprNode);      //结果是浮点数
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pLeftExprNode);      //结果是浮点数
				break;

			case TNK_TRUNC_DIV_EXP:
				FDIVR_M64REAL(pLeftExprNode); 
				break;

			default:
				FCOMP_M64REAL(pLeftExprNode);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);				
			}
		}
		else
		{
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);    //把结果移动到栈中
			FILD(-(objContext.GetOffset() + 4));          //装载到浮点数寄存器中

			GenerateExprStatementsCode(pRightExprNode,objContext); //肯定是浮点数

			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADDP(1);
				break;

			case TNK_MINUS_EXPR:
				FSUBP(1);
				break;

			case TNK_MULT_EXPR:
				FMULP(1);
				break;

				//case TNK_TRUNC_MOD_EXPR:
			case TNK_TRUNC_DIV_EXP:
				FDIVP(1);
				break;

			default:
				FCOMPP(1);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);	
			}
		}
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		if(pRightExprNode->IsNumericConstants())
		{
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pRightExprNode);      //结果是浮点数
				break;

			case TNK_MINUS_EXPR:
				FSUB_M64REAL(pRightExprNode);
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pRightExprNode);      //结果是浮点数
				break;

				//case TNK_TRUNC_MOD_EXPR:
			case TNK_TRUNC_DIV_EXP:
				FDIV_M64REAL(pRightExprNode); 
				break;

			default:
				FCOMP_M64REAL(pRightExprNode);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);	
			}
		}
		else if(pLeftExprNode->IsNumericConstants())
		{
			GenerateExprStatementsCode(pRightExprNode,objContext); //可能为任何类型

			if(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
				FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //装载到浮点数寄存器中
			}
			else if(pRightNodeDataType->GetNodeType() != TNK_FLOAT_TYPE
				&& pRightNodeDataType->GetNodeType() != TNK_DOUBLE_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
				FILD(-(objContext.GetOffset() + 4));        //装载到浮点数寄存器中
			}

			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pLeftExprNode);      //结果是浮点数
				break;

			case TNK_MINUS_EXPR:
				FSUBR_M64REAL(pLeftExprNode);
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pLeftExprNode);      //结果是浮点数
				break;

				//case TNK_TRUNC_MOD_EXPR:
			case TNK_TRUNC_DIV_EXP:
				FDIVR_M64REAL(pLeftExprNode);
				break;

			default:                
				FCOMP_M64REAL(pLeftExprNode);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);
			}
		}
		else
		{
			GenerateExprStatementsCode(pLeftExprNode,objContext);

			GenerateExprStatementsCode(pRightExprNode,objContext);   
			if(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
				FILD(-(objContext.GetOffset() + 8));        //装载到浮点数寄存器中
			}
			else if(pRightNodeDataType->GetNodeType() != TNK_FLOAT_TYPE
				&& pRightNodeDataType->GetNodeType() != TNK_DOUBLE_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
				FILD(-(objContext.GetOffset() + 4));        //装载到浮点数寄存器中
			}

			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADDP(1);
				break;

			case TNK_MINUS_EXPR:
				FSUBP(1);
				break;

			case TNK_MULT_EXPR:
				FMULP(1);
				break;

			case TNK_TRUNC_DIV_EXP:
				FDIVP(1);
				break;

			default:
				FCOMPP(1);
				FNSTSW(objContext);
				FloatingCmpCommonCode(pParantExprNode,nOperator,objContext);
			}
		}
		break;
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::IntegerOperator
| 描  述 : 处理简单数据类型的运算,结构村到寄存器RDX:RAX中
| 参  数 : CSyntaxTreeNode* pLeftExprNode——左边操作数
| 参  数 : CSyntaxTreeNode* pRightExprNode——右边操作数
| 参  数 : TREE_NODE_TYPE nOperator——运算类型
| 注意   : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::IntegerOperator(CSyntaxTreeNode* pLeftExprNode
	,CSyntaxTreeNode* pRightExprNode,CExpressionTreeNode* pParantExprNode,CContext& objContext)
{
	ATLASSERT(pLeftExprNode);
	ATLASSERT(pRightExprNode);
	ATLASSERT(pParantExprNode);

	CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
	ATLASSERT(pConditionExprJmpRef);
	JUM_INSTRUCTION_CODE nInstCode = JIC_UNKNOWN;

	UINT       nStatus        = 0;
	COMMON_REGISTERS nFreeReg = CR_INVALID;
	UINT nOperator            = pParantExprNode->GetNodeType();
	BOOL bIsUnsigned          = pParantExprNode->IsUnsigned();   //得到表达式是否是无符号

	if(pLeftExprNode->IsNumericConstants())
	{
		GenerateExprStatementsCode(pRightExprNode,objContext);

		switch(nOperator)
		{
		case TNK_PLUS_EXPR:
			ADD_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);
			break;

		case TNK_MINUS_EXPR:
			MOV_REG_IMM32(RDX,(int)pLeftExprNode->GetValue1(),objContext);
			SUB(RDX,RAX,objContext);
			MOV(RAX,RDX,objContext);
			break;

		case TNK_MULT_EXPR:
			IMUL(RAX,RAX,(int)pLeftExprNode->GetValue1(),objContext);
			break;

		case TNK_TRUNC_DIV_EXP:
		case TNK_TRUNC_MOD_EXPR:
			nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			MOV(nFreeReg,RAX,objContext);
			MOV_REG_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);
			CDQ(objContext);
			IDIV(nFreeReg,objContext);
			FreeReg(nFreeReg,objContext,nStatus);
			break;	

		case TNK_LSHIFT_EXPR:
			nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			MOV(nFreeReg,RAX,objContext);
			MOV_REG_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);
			SHL(RAX,objContext);
			FreeReg(nFreeReg,objContext,nStatus);
			break;

		case TNK_RSHIFT_EXPR:
			nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			MOV(nFreeReg,RAX,objContext);
			MOV_REG_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);
			SHR(RAX,objContext);
			FreeReg(nFreeReg,objContext,nStatus);
			break;

		case TNK_LT_EXPR:			
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1());
			if(bIsUnsigned)
			{ 	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JBE : JIC_JA; }
			else
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JL : JIC_JG; }			
			break;

		case TNK_NLT_EXPR:
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1()); 
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JB : JIC_JNB;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNG : JIC_JNL; }	
			break;

		case TNK_GT_EXPR:
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1());
			if(bIsUnsigned)
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNB : JIC_JB; }
			else
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JG : JIC_JL; }	
			break;

		case TNK_NGT_EXPR:
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1());  
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JA : JIC_JBE; }
			else
			{  nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNL : JIC_JNG; }	
			break;

		case TNK_EQ_EXPR:
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1()); 
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNE:JIC_JE;
			break;

		case TNK_NEQ_EXPR:
			CMP_IMM32(RAX,(int)pLeftExprNode->GetValue1());  
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JE:JIC_JNE;
			break;

		case TNK_BIT_AND_EXPR:
			AND_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);  
			break;

		case TNK_BIT_XOR_EXPR:
			XOR_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);  
			break;

		case TNK_BIT_IOR_EXPR:
			OR_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext); 
			break;
		}
		
	}
	else if(pRightExprNode->IsNumericConstants())
	{
		GenerateExprStatementsCode(pLeftExprNode,objContext);

		switch(nOperator)
		{
		case TNK_PLUS_EXPR:
			ADD_IMM32(RAX,(int)pRightExprNode->GetValue1(),objContext);
			break;

		case TNK_MINUS_EXPR:
			SUB_IMM32(RAX,(int)pRightExprNode->GetValue1(),objContext);
			break;

		case TNK_MULT_EXPR:
			IMUL(RAX,RAX,(int)pRightExprNode->GetValue1(),objContext);
			break;

		case TNK_TRUNC_MOD_EXPR:
		case TNK_TRUNC_DIV_EXP:
			nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
			MovIntValtoReg(pRightExprNode,nFreeReg,CR_INVALID,objContext);
			CDQ(objContext);
			IDIV(nFreeReg,objContext);
			FreeReg(nFreeReg,objContext,nStatus);
			break;

		case TNK_LSHIFT_EXPR:
			SHL(RAX,(BYTE)pRightExprNode->GetValue1(),objContext);
			break;

		case TNK_RSHIFT_EXPR:
			SHR(RAX,(BYTE)pRightExprNode->GetValue1(),objContext);
			break;

		case TNK_LT_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1());
			if(bIsUnsigned)
			{ 	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNB:JIC_JB; }
			else
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNL:JIC_JL; }			
			break;

		case TNK_NLT_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1());  
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JA:JIC_JBE;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JG:JIC_JNG; }		
			break;

		case TNK_GT_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1());
			if(bIsUnsigned)
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JBE:JIC_JA; }
			else
			{	nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNG:JIC_JG; }		
			break;

		case TNK_NGT_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1()); 
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JB:JIC_JNB;	}
			else
			{  nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JL:JIC_JNL; }		
			break;

		case TNK_EQ_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1()); 
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNE:JIC_JE;
			break;

		case TNK_NEQ_EXPR:
			CMP_IMM32(RAX,(int)pRightExprNode->GetValue1()); 
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JE:JIC_JNE;
			break;

		case TNK_BIT_AND_EXPR:
			AND_IMM32(RAX,(int)pRightExprNode->GetValue1(),objContext);  
			break;

		case TNK_BIT_XOR_EXPR:
			XOR_IMM32(RAX,(int)pRightExprNode->GetValue1(),objContext);  
			break;

		case TNK_BIT_IOR_EXPR:
			OR_IMM32(RAX,(int)pRightExprNode->GetValue1(),objContext); 
			break;
		}
	}
	else
	{
		GenerateExprStatementsCode(pRightExprNode,objContext);
		nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);
		MOV(nFreeReg,RAX,objContext);

		GenerateExprStatementsCode(pLeftExprNode,objContext);

		switch(nOperator)
		{
		case TNK_PLUS_EXPR:
			ADD(RAX,nFreeReg,objContext);
			break;

		case TNK_MINUS_EXPR:
			SUB(RAX,nFreeReg,objContext);
			break;

		case TNK_MULT_EXPR:
			IMUL(nFreeReg,objContext);
			break;

		case TNK_TRUNC_MOD_EXPR:
		case TNK_TRUNC_DIV_EXP:
			CDQ(objContext);
			IDIV(nFreeReg,objContext);
			break;

		case TNK_LSHIFT_EXPR:
			SHL(RAX,objContext);
			break;

		case TNK_RSHIFT_EXPR:
			SHR(RAX,objContext);
			break;

		case TNK_LT_EXPR:
			CMP(RAX,nFreeReg);
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNB:JIC_JB;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNL:JIC_JL; }		
			break;

		case TNK_NLT_EXPR:
			CMP(RAX,nFreeReg);
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JA:JIC_JBE;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JG:JIC_JNG; }		                  
			break;

		case TNK_GT_EXPR:
			CMP(RAX,nFreeReg);
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JBE:JIC_JA;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNG:JIC_JG;	}	
			break;

		case TNK_NGT_EXPR:
			CMP(RAX,nFreeReg);
			if(bIsUnsigned)
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JB:JIC_JNB;	}
			else
			{ nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JL:JIC_JNL;	}	
			break;

		case TNK_EQ_EXPR:
			CMP(RAX,nFreeReg);
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JNE:JIC_JE;
			break;

		case TNK_NEQ_EXPR:
			CMP(RAX,nFreeReg);
			nInstCode = (pConditionExprJmpRef->GetJmpType() == JT_FALSE)? JIC_JE:JIC_JNE; 
			break;

		case TNK_BIT_AND_EXPR:
			AND(RAX,nFreeReg,objContext);  
			break;

		case TNK_BIT_XOR_EXPR:
			XOR(RAX,nFreeReg,objContext);  
			break;

		case TNK_BIT_IOR_EXPR:
			OR(RAX,nFreeReg,objContext); 
			break;
		}
		FreeReg(nFreeReg,objContext,nStatus);
		
	}	

	if(pParantExprNode->IsCmpExpression())
	{
		JMP_BASE(nInstCode,TMP_ADDR_BYTE4);
		pConditionExprJmpRef->AppendRefPos(pConditionExprJmpRef->GetJmpType(),GetOffset());  //只记录失败跳转的位置
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateSimpleAssignmentOp
| 描  述 : 为赋值语句创建代码
| 参  数 : CDeclarationTreeNode* pVariableNode——被赋值的节点
| 参  数 : CSyntaxTreeNode* pValue——值节点,可以使常量,变量,表达式
| 备注:此函数已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateSimpleAssignmentOp( CDeclarationTreeNode* pVariableNode
	,CSyntaxTreeNode* pValue,CContext& objContext)
{
	ATLASSERT(pVariableNode && pValue);

	CDataTypeTreeNode* pVarDataTypeNode = pVariableNode->GetDataType();
	CDataTypeTreeNode* pValueDataType = pValue->GetDataType();

	switch(pVarDataTypeNode->GetNodeType())
	{
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:          //为一个字节的变量赋值
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           //这些类型作为参数转递的时候,在栈中都是占用四个字节
	case TNK_LONGLONG_TYPE:         //重点测试
		if(pValue->IsNumericConstants())   //是常量
		{
			LONGLONG nValue = LONGLONG(pValue->GetValue1() + pValue->GetValue2());
			if(pVarDataTypeNode->GetNodeType() == TNK_BOOLEAN_TYPE)
			{
				nValue = (nValue != 0)? 1: 0;
			}
			SetIntVariable(pVariableNode, nValue,objContext);       
		}
		else                 //否则说明是一个表达式
		{
			GenerateExprStatementsCode(pValue,objContext);      //表达式的值将存放到RAX寄存器里面   
			if(pValueDataType->IsInterger())
			{
				if(pValueDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
				{
					AND_IMM32(RAX,0xFF,objContext);
				}
				if(pVarDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE
					&& pValueDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}
				MovRegtoIntVal(pVariableNode,RAX,RDX,objContext);   
			}
			else
			{
				FSTPVariable(pVariableNode,objContext);
			}
		}    
		break;

	case TNK_FLOAT_TYPE:
	case TNK_DOUBLE_TYPE:
		if(!pValue->IsExpression())                   //不是一个表达式
		{
			FLDVariable(pValue,objContext);
		}
		else                              //否则说明是一个表达式
		{
			GenerateExprStatementsCode(pValue,objContext);       //表达式的值如果是实数将存放到浮点数寄存器里面,否则放在RAX寄存器里面   

			CDataTypeTreeNode* pValueDataTypeNode = pValue->GetDataType();
			if(pValueDataTypeNode->IsInterger())            //说明表达式的结果是整数,那么先拷贝到堆栈,让后再转移到浮点数寄存器
			{               
				if(pVariableNode->GetSize() == 8)
				{
					MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
					MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
					FILD(-(objContext.GetOffset() + 8),BT_BIT64);
				}
				else
				{
					MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
					FILD(-(objContext.GetOffset() + 4),BT_BIT32);
				}
			}          
		}

		FSTPVariable(pVariableNode,objContext);
		break;

	case TNK_POINTER_TYPE:
		switch(pValue->GetNodeType())
		{
		case TNK_ADDR_EXPR:               //取地址符号
			GenerateExprStatementsCode(pValue,objContext);      //表达式的值放在RAX寄存器里面   
			MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);            
			break;

		case TNK_VAR_DECL:        //指针变量
			{
				CDataTypeTreeNode* pLocalVarDataTypeNode = pValue->GetDataType();
				if(pLocalVarDataTypeNode->GetNodeType() == TNK_ARRAY_TYPE)           //把数组首地址移动到一个指针变量中
				{
					if(((CDeclarationTreeNode*)pValue)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //说明是局部变量
					{
						LEA(RAX,pValue->GetOffset(),objContext);
						MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);        
					}
					else
					{
						Offset(pVariableNode,pValue);        
					}
				}
				else
				{
					MovIntValtoReg(pValue,RAX,CR_INVALID,objContext);
					MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);
				}
			}
			break;

		case TNK_INDIRECT_REF:
			MovIntValtoReg(pValue,RAX,CR_INVALID,objContext);
			MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);				
			break;

		default:
			if(pValue->IsNumericConstants())   //是常量
			{
				SetIntVariable(pVariableNode, pValue->GetValue1(),objContext);       
			}
			else 
			{
				GenerateExprStatementsCode(pValue,objContext);      //表达式的值放在RAX寄存器里面   
				MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);
			}
		}
		break;

	case TNK_REFERENCE_TYPE:
		switch(pValue->GetNodeType())
		{
		case TNK_VAR_DECL:
			if(((CDeclarationTreeNode*)pValue)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //说明是局部变量
			{
				LEA(RAX,pValue->GetOffset(),objContext);
				MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);        
			}
			else
			{
				Offset(pVariableNode,pValue);        
			}
			break;

		case TNK_INDIRECT_REF: //eg. *p
			MovIntValtoReg(((CExpressionTreeNode*)pValue)->GetChildNode(0),RAX,CR_INVALID,objContext);
			MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);       
			break;

		default:
			ATLASSERT(FALSE);
		}
		break;

	case TNK_ARRAY_TYPE:    //说明是数组需要被初始化
		{
			int nOffset = pVariableNode->GetOffset();
			const BOOL bStatic = (pVariableNode->GetDeclAttribute() & NDA_STATIC_FLAG);
			CDataTypeTreeNode* pBaseDataTypeTreeNode = pVarDataTypeNode->GetDataType();
			CSyntaxTreeNode* pEntryValue = ((CVectorTreeNode*)pValue)->GetValue();
			while(NULL != pEntryValue)
			{
				switch(pBaseDataTypeTreeNode->GetNodeType())
				{
				case TNK_BYTE_TYPE:
				case TNK_CHAR_TYPE:  
				case TNK_BOOLEAN_TYPE:
					MOV_IV8_IMM(nOffset,bStatic,BYTE(pEntryValue->GetValue1()? pEntryValue->GetValue1():pEntryValue->GetValue2()));
					break;

				case TNK_WORD_TYPE:
				case TNK_SHORT_TYPE:
					MOV_IV16_IMM(nOffset,bStatic,WORD(pEntryValue->GetValue1()? pEntryValue->GetValue1():pEntryValue->GetValue2()));
					break;

				case TNK_DWORD_TYPE: 
				case TNK_LONG_TYPE:                
				case TNK_VOID_TYPE:    
				case TNK_INT_TYPE: 
					MOV_LIV32_IMM(nOffset,bStatic,DWORD(pEntryValue->GetValue1()? pEntryValue->GetValue1():pEntryValue->GetValue2()));
					break;

				case TNK_LONGLONG_TYPE:
					MOV_LIV32_IMM(nOffset,bStatic,LONGLONG(pEntryValue->GetValue1()? pEntryValue->GetValue1():pEntryValue->GetValue2()) &0xFFFFFFFF);
					MOV_LIV32_IMM(nOffset + 4,bStatic,LONGLONG(pEntryValue->GetValue1()? pEntryValue->GetValue1():pEntryValue->GetValue2()) >> 32);
					break;

				case TNK_FLOAT_TYPE:
					FLDVariable(pEntryValue,objContext);
					FSTP(nOffset,bStatic,BT_BIT32);
					break;

				case TNK_DOUBLE_TYPE: 
					FLDVariable(pEntryValue,objContext);
					FSTP(nOffset,bStatic,BT_BIT64);
					break;

				default:
					ATLASSERT(FALSE);
				}
				nOffset += pBaseDataTypeTreeNode->GetSize();
				pEntryValue = pEntryValue->GetChain();
			}
		}
		break;

	default:
		ATLASSERT(FALSE);
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateLableSTMTCode
| 描  述 : 处理标签语句,这个语句不产生任何代码
| 参  数 : CDeclarationTreeNode* pNode——标签申明节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateLableSTMTCode(CDeclarationTreeNode* pNode,CContext& objContext)
{
	pNode->SetOffset(GetOffset());

	CIdentifierTreeNode* pID = pNode->GetName();
	if(NULL != pID)
	{
		CLableRefrenceInfo* pLableRefrenceInfo = objContext.LookupLableRefrence(pID->GetTitle());
		if(NULL != pLableRefrenceInfo)
		{
			for(UINT i = 0;i < pLableRefrenceInfo->GetRefrences();i++)
			{
				WriteDwordData(pLableRefrenceInfo->GetRefenceOffset(i) - 4/*JMP指令代码一个字节*/
					,GetOffset() - pLableRefrenceInfo->GetRefenceOffset(i),objContext);
			}
		}    
	}


	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%s: \r\n",pNode->GetSymbol());
	}

}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateGotoSTMTCode
| 描  述 : 处理goto语句
| 参  数 : CDeclarationTreeNode* pNode——标签申明节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateGotoSTMTCode(CDeclarationTreeNode* pNode,CContext& objContext)
{
	if(pNode->GetOffset() > 0)   //说明要跳转的位置已经存在,即标签语句在goto语句的前面
	{
		JMP_IMM32(pNode->GetOffset() - GetOffset(),objContext);    
	}
	else
	{
		JMP_IMM32(GetOffset(),objContext); //此处的参数没有任何意思,最后将被实际的参数取代

		CIdentifierTreeNode* pID = pNode->GetName();
		if(NULL != pID)
		{
			CLableRefrenceInfo* pLableRefrenceInfo = objContext.LookupLableRefrence(pID->GetTitle());
			if(NULL == pLableRefrenceInfo)
			{
				pLableRefrenceInfo = objContext.CreateLableRefrence(pID->GetTitle());
			}
			if(NULL != pLableRefrenceInfo)
			{
				pLableRefrenceInfo->AppendRefrence(GetOffset());
			}
		}		
	}

	/*if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s\r\n","jmp",pNode->GetSymbol());
	}*/
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateCallSTMTCode
| 描  述 : 处理Call语句
| 参  数 : CExpressionTreeNode* pNode——call表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateCallSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext)
{
	UINT nParaSize = 0;      //参数在栈中占用的空间
	ATLASSERT(pExpressionNode->GetNodeType() == TNK_CALL_EXPR);

	CDeclarationTreeNode* pFunctionDeclNode = (CDeclarationTreeNode*)pExpressionNode->GetChildNode(0);
	CExpressionTreeNode* pParaExpressionNode = (CExpressionTreeNode*)pFunctionDeclNode->GetArguments(); //得到参数定义列表
	if(NULL != pParaExpressionNode)
	{
		CDeclarationTreeNode* pParaDeclNodes  = (CDeclarationTreeNode*)pParaExpressionNode->GetChildNode(0); //得到参数定义列表

		if(objContext.GetRegisterUsedMask() & CRM_RDX)
		{
			PUSH(RDX);
		}
		//下面的代码是不参数压进栈
		CDataTypeTreeNode* pParaVarDataTypeNode  = NULL;
		CDataTypeTreeNode* pParaDeclDataTypeNode = NULL;
		CSyntaxTreeNode* pParaValueNodes = pExpressionNode->GetChildNode(1);  //得到参数值列表
		while(NULL != pParaDeclNodes)
		{
			pParaVarDataTypeNode = pParaValueNodes->GetDataType();
			pParaDeclDataTypeNode = pParaDeclNodes->GetDataType();

			switch(pParaDeclDataTypeNode->GetNodeType())
			{
			case TNK_CHAR_TYPE:
			case TNK_BYTE_TYPE:
			case TNK_BOOLEAN_TYPE:          
			case TNK_SHORT_TYPE:
			case TNK_WORD_TYPE:
			case TNK_INT_TYPE:
			case TNK_LONG_TYPE:
			case TNK_DWORD_TYPE:                      //这些类型作为参数转递的时候,在栈中都是占用四个字节
				objContext.IncOffset(4);
				if(pParaValueNodes->IsNumericConstants())            //是常量
				{
					PUSH_IMM32(DWORD(pParaValueNodes->GetValue1() + pParaValueNodes->GetValue2()));        
				}
				else                            //否则说明是一个表达式
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //表达式的值将存放到RAX寄存器里面
					if(pParaVarDataTypeNode->IsFloat())
					{
						PUSH_IMM32(0); 
						FISTP_INDIRECT(RSP);
					}
					else
					{
						PUSH(RAX);
					}
				}
				nParaSize += 4;
				break;

			case TNK_FLOAT_TYPE:
				objContext.IncOffset(4);
				PUSH_IMM32(0);                          //该指令的目的是在栈中分配４个字节，它可能比"sub esp,4"快  

				if(pParaValueNodes->IsNumericConstants())            //是常量
				{
					FLDVariable(pParaValueNodes,objContext);
				}
				else
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //表达式的值将存放到RAX寄存器里面

					if(pParaVarDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
						FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //装载到浮点数寄存器中
					}
					else if(pParaVarDataTypeNode->IsInterger())
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
						FILD(-(objContext.GetOffset() + 4));               //装载到浮点数寄存器中      
					}						
				}
				FSTP_INDIRECT(RSP);

				nParaSize += 4;
				break;

			case TNK_DOUBLE_TYPE:
				objContext.IncOffset(8);
				SUB_IMM32(RSP,8,objContext);                      //该指令的目的是在栈中分配8个字节            

				if(pParaValueNodes->IsNumericConstants())            //是常量
				{
					FLDVariable(pParaValueNodes,objContext);
				}
				else
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //表达式的值将存放到RAX寄存器里面

					if(pParaVarDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
						FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //装载到浮点数寄存器中
					}
					else if(pParaVarDataTypeNode->IsInterger())
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
						FILD(-(objContext.GetOffset() + 4));               //装载到浮点数寄存器中      
					}						
				}
				FSTP_INDIRECT(RSP,BT_BIT64);

				nParaSize += 8;
				break;

			case TNK_POINTER_TYPE:
				switch(pParaValueNodes->GetNodeType())
				{
				case TNK_ADDR_EXPR:               //取地址符号
					GenerateExprStatementsCode(pParaValueNodes,objContext);      //表达式的值放在RAX寄存器里面   
					PUSH(RAX);           
					break;

				case TNK_VAR_DECL:        //指针变量
					if(pParaVarDataTypeNode->GetNodeType() == TNK_ARRAY_TYPE)           //把数组首地址移动到一个指针变量中
					{
						if(((CDeclarationTreeNode*)pParaValueNodes)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //说明是局部变量
						{
							LEA(RAX,pParaValueNodes->GetOffset(),objContext);
							PUSH(RAX);       
						}
						else
						{
							SUB_IMM32(RSP,(ACM_MODE_X86 == m_nMode)? 4:8,objContext); 
							MOV_LIV32_OFFSET(-(objContext.GetOffset() + 4),pParaValueNodes->GetOffset());								
						}
					}
					else
					{
						MovIntValtoReg(pParaValueNodes,RAX,CR_INVALID,objContext);
						PUSH(RAX); 
					}					
					break;

				case TNK_INDIRECT_REF:
					MovIntValtoReg(pParaValueNodes,RAX,CR_INVALID,objContext);
					PUSH(RAX); 			
					break;

				default:
					if(pParaValueNodes->IsNumericConstants())   //是常量
					{
						PUSH_IMM32((DWORD)pParaValueNodes->GetValue1());       
					}
					else 
					{
						GenerateExprStatementsCode(pParaValueNodes,objContext);      //表达式的值放在RAX寄存器里面   
						PUSH(RAX);
					}
				}

				objContext.IncOffset((ACM_MODE_X86 == m_nMode)? 4:8);
				nParaSize += (ACM_MODE_X86 == m_nMode)? 4:8;
				break;

			case TNK_REFERENCE_TYPE:
				objContext.IncOffset((ACM_MODE_X86 == m_nMode)? 4:8);
				switch(pParaValueNodes->GetNodeType())
				{
				case TNK_VAR_DECL:
					if(((CDeclarationTreeNode*)pParaValueNodes)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //说明是局部变量
					{
						LEA(RAX,pParaValueNodes->GetOffset(),objContext);
						PUSH(RAX);       
					}
					else
					{
						PUSH_IMM32(pParaValueNodes->GetOffset());        
						AppendRef(GetOffset() - 4);    //静态变量,记录参考地址        
					}
					break;

				case TNK_INDIRECT_REF: //eg. *p
					MovIntValtoReg(((CExpressionTreeNode*)pParaValueNodes)->GetChildNode(0),RAX,CR_INVALID,objContext);
					PUSH(RAX);       
					break;

				default:
					ATLASSERT(FALSE);
				}
				nParaSize += (ACM_MODE_X86 == m_nMode)? 4:8;
				break;

			case TNK_LONGLONG_TYPE:         //重点测试
				objContext.IncOffset(8);
				if(pParaValueNodes->IsNumericConstants())            //是常量
				{
					LONGLONG nTmp = pParaValueNodes->GetValue1() + (LONGLONG)pParaValueNodes->GetValue2();
					PUSH_IMM32(nTmp >> 32);    
					PUSH_IMM32(nTmp & 0xFFFFFFFF);
				}
				else                            //否则说明是一个表达式
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //表达式的值将存放到RAX寄存器里面
					if(pParaVarDataTypeNode->IsFloat())
					{
						SUB_IMM32(RSP,8,objContext); 
						FISTP_INDIRECT(RSP,BT_BIT64);
					}
					else
					{
						if(pParaVarDataTypeNode->GetNodeType() != TNK_LONGLONG_TYPE)
						{
							XOR(RDX,RDX,objContext);
						}

						PUSH(RDX);  //先推高位4个字节
						PUSH(RAX);
					}
				}
				nParaSize += 8;
				break;

			default:
				ATLASSERT(FALSE);
			}

			pParaValueNodes = pParaValueNodes->GetChain();
			pParaDeclNodes = (CDeclarationTreeNode*)pParaDeclNodes->GetChain();
		}
	}

	EmitInstruction(0xE8,CalculateComplement(pFunctionDeclNode->GetOffset() - GetOffset() - 5)); //是相对位置

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s (%x)\r\n","call",pFunctionDeclNode->GetSymbol()
			,pFunctionDeclNode->GetOffset());
	}
	objContext.DecOffset(nParaSize);
	ADD_IMM32(RSP,nParaSize,objContext); //移动栈指针,释放为参数分配空间

	if(objContext.GetRegisterUsedMask() & CRM_RDX)
	{
		POP(RDX,objContext);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprAddrCode
| 描  述 : 提取一个数表达式(简单变量,数组元素引用,指针引用)的地址
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprAddrCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CDeclarationTreeNode* pExprNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pNode)->GetChildNode(0);
	ATLASSERT(pExprNode);
	if(pExprNode->GetNodeType() == TNK_ARRAY_REF)         //类似&name[12]
	{
		CDeclarationTreeNode* pArrayDeclNode = (CDeclarationTreeNode*)(((CExpressionTreeNode*)pExprNode)->GetChildNode(0));

		MOD MODE_TYPE = MODE00;  //默认寻址是为静态变量
		BOOL bStatic = TRUE;   //默认寻址是为静态变量
		const int OFFSET  = pArrayDeclNode->GetOffset();
		if(((CDeclarationTreeNode*)pArrayDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //非静态变量 
		{
			MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
			bStatic  = FALSE;
		}

		CDataTypeTreeNode* pDataTypeNode   = pArrayDeclNode->GetDataType()->GetDataType();    //得到数组的元素类型 
		GenerateExprStatementsCode(((CExpressionTreeNode*)pExprNode)->GetChildNode(1),objContext); //计算下标,并把值存放在RAX寄存器里面
		const int nScale  = CalculationIndex(pDataTypeNode->GetSize());


		UINT nStatus = 0;
		COMMON_REGISTERS nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
		
		EmitInstruction(PREFIX_INVALID,0x8D,ModRM(MODE_TYPE,nFreeReg,SIB_ADDR),SIB(nScale,RAX,RBP)
			,pArrayDeclNode->GetOffset(),0,0,bStatic);        //把地址放到"nFreeReg"寄存器中
		if(m_bEnableMasm)
		{
			if(bStatic) //说明是局部变量
			{
				GenerateMasmCmd("%-10s %s,[%s + %s*%d %+d]\r\n","lea",
					RegName(nFreeReg),RegName(RBP),RegName(RAX),1<<nScale,OFFSET);
			}
			else
			{       
				GenerateMasmCmd("%-10s %s,%s[%s]\r\n","lea",
					RegName(nFreeReg),pArrayDeclNode->GetSymbol(),RegName(RAX));
			}
		}

		MOV(RAX,nFreeReg,objContext);
		FreeReg(RCX,objContext,nStatus);		
	}
	else if(pExprNode->GetNodeType() == TNK_INDIRECT_REF) 
	{
		CSyntaxTreeNode* pVarDeclNode = ((CExpressionTreeNode*)pExprNode)->GetChildNode(0);
		MovIntValtoReg(pVarDeclNode,RAX,CR_INVALID,objContext);
	}
	else
	{
		if(pExprNode->GetDeclAttribute() ^ NDA_STATIC_FLAG) //说明是局部变量
		{
			LEA(RAX,pExprNode->GetOffset(),objContext);
		}
		else
		{
			MOV_REG_IMM32(RAX,pExprNode->GetOffset(),objContext);        
			AppendRef(GetOffset() - 4);    //静态变量,记录参考地址 
		}
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::ExtractFactorValue
| 描  述 : 提取一个常量或变量的值
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ExtractFactorValue( CSyntaxTreeNode* pNode,CContext& objContext)
{
	switch(pNode->GetNodeType())
	{
	case TNK_STRING_CST:
		MOV_REG_IMM32(RAX,pNode->GetOffset(),objContext);
		AppendRef(GetOffset() - 4);    //静态变量,记录参考地址 
		break;

	case TNK_INTEGER_CST:
		MOV_REG_IMM32(RAX,DWORD(pNode->GetValue1()),objContext);
		break;

	case TNK_REAL_CST:
		FLDVariable(pNode,objContext);
		break;   

	case TNK_ADDR_EXPR:
		GenerateExprAddrCode((CExpressionTreeNode*)pNode,objContext);
		break;

	default:
		{
			CDataTypeTreeNode* pDataTypeNode = pNode->GetDataType();   
			if(pDataTypeNode->IsInterger() || pDataTypeNode->GetNodeType() == TNK_POINTER_TYPE)
			{
				MovIntValtoReg(pNode,RAX,RDX,objContext);
			}
			else if(pDataTypeNode->IsFloat())
			{
				FLDVariable(pNode,objContext);
			}
		}    
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprLogicalNotCode
| 描  述 : 处理处理逻辑非表达式
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprLogicalNotCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode  = pNode->GetChildNode(0);
	CDataTypeTreeNode* pDataTypeNode = pLeftExprNode->GetDataType();

	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_LONGLONG_TYPE:
		XOR(RDX,RDX,objContext);
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:          
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:	
		if(pLeftExprNode->IsExpression())
		{
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(TNK_LONGLONG_TYPE == pDataTypeNode->GetNodeType())
			{
				OR(RAX,RDX,objContext);
				JNE(0,objContext);         
				UINT nPos1 = GetOffset();
				MOV_REG_IMM32(RAX,1,objContext);
				JMP_IMM32(0,objContext);      
				UINT nPos2 = GetOffset();
				WriteByteData(nPos1 - 1,(BYTE)(GetOffset() - nPos1),objContext);
				XOR(RAX,RAX,objContext);
				WriteByteData(nPos2 - 1,(BYTE)(GetOffset() - nPos2),objContext);		
			}
			else
			{
				NEG(RAX,objContext);
				SBB(RAX,RAX,objContext);
				ADD_IMM32(RAX,1,objContext);
			}
		}
		else
		{
			CompareIntVariable(pLeftExprNode,0,objContext); 
		}
		break;

	case TNK_FLOAT_TYPE:
	case TNK_DOUBLE_TYPE:
		{//TODO: Tested
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			FLDZ();
			FUCOMPP();
			FNSTSW(objContext);
			TEST_BIT8(RAX,0x44);
			JE(7,objContext);         
			UINT nPos1 = GetOffset();
			MOV_REG_IMM32(RAX,1,objContext);
			JMP_IMM32(2,objContext);      
			UINT nPos2 = GetOffset();
			WriteByteData(nPos1 - 1,(BYTE)(GetOffset() - nPos1),objContext);
			XOR(RAX,RAX,objContext);
			WriteByteData(nPos2 - 1,(BYTE)(GetOffset() - nPos2),objContext);
		}
		break;   

	default:
		ATLASSERT(FALSE);	
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprBitNotCode
| 描  述 : 处理按未去取反运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprBitNotCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode  = pNode->GetChildNode(0);
	CDataTypeTreeNode* pDataTypeNode = pLeftExprNode->GetDataType();

	GenerateExprStatementsCode(pLeftExprNode,objContext);
	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_LONGLONG_TYPE:
		NOT(RDX,objContext);

	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:
	case TNK_BOOLEAN_TYPE:     //warning C4804: '~' : unsafe use of type 'bool' in operation       
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		NOT(RAX,objContext);
		break;

	default:
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprIncreaseCode
| 描  述 : 处理++运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprIncreaseCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode  = pNode->GetChildNode(0);
	CDataTypeTreeNode* pDataTypeNode = pLeftExprNode->GetDataType();

	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE:     //warning C4804: '~' : unsafe use of type 'bool' in operation  
		if(pNode->GetNodeType() == TNK_PREINCREMENT_EXPR)
		{
			SetIntVariable(pLeftExprNode,1,objContext);
			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
		}
		else
		{
			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
			SetIntVariable(pLeftExprNode,1,objContext);       
		}
		break;

	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		if(pNode->GetNodeType() == TNK_PREINCREMENT_EXPR)
		{
			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
			ADD_IMM32(RAX,1,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,CR_INVALID,objContext);
		}
		else
		{
			CSyntaxTreeNode* pParentExprNode = pNode->GetParent();
			ATLASSERT(pParentExprNode);

			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
			if(pParentExprNode->IsExpression()
				|| pParentExprNode->GetNodeType() == TNK_RETURN_STMT)  //只有当前表达式的父节点是表达式时,才有必要推进栈
			{
				PUSH(RAX);
			}
			ADD_IMM32(RAX,1,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,CR_INVALID,objContext);
			if(pParentExprNode->IsExpression()|| pParentExprNode->GetNodeType() == TNK_RETURN_STMT)
			{
				POP(RAX,objContext);  
			}		
		}
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		if(pNode->GetNodeType() == TNK_PREINCREMENT_EXPR)
		{
			CSyntaxTreeNode* pRightExprNode  = pNode->GetChildNode(1);
			FLDVariable(pLeftExprNode,objContext);
			FADD_M64REAL(pRightExprNode);
			FSTVariable(pLeftExprNode,objContext);
		}
		else
		{
			CSyntaxTreeNode* pRightExprNode  = pNode->GetChildNode(1);
			FLDVariable(pLeftExprNode,objContext);   //注意两次调用FLDVariable不是多余的,后面的一次装载要弹出
			FLDVariable(pLeftExprNode,objContext);
			FADD_M64REAL(pRightExprNode);
			FSTPVariable(pLeftExprNode,objContext);
		}
		break;   

	case TNK_LONGLONG_TYPE:
		if(pNode->GetNodeType() == TNK_PREINCREMENT_EXPR)
		{
			MovIntValtoReg(pLeftExprNode,RAX,RDX,objContext);
			ADD_IMM32(RAX,1,objContext);
			ADC_IMM32(RDX,0,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,RDX,objContext);
		}
		else
		{
			CSyntaxTreeNode* pParentExprNode = pNode->GetParent();
			ATLASSERT(pParentExprNode);

			MovIntValtoReg(pLeftExprNode,RAX,RDX,objContext);
			if(pParentExprNode->IsExpression()|| pParentExprNode->GetNodeType() == TNK_RETURN_STMT)
			{
				PUSH(RAX);
				PUSH(RDX);
			}
			ADD_IMM32(RAX,1,objContext);
			ADC_IMM32(RDX,0,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,RDX,objContext);
			if(pParentExprNode->IsExpression()|| pParentExprNode->GetNodeType() == TNK_RETURN_STMT)
			{
				POP(RDX,objContext);
				POP(RAX,objContext);  
			}
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprDecreaseCode
| 描  述 : 处理--运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprDecreaseCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode  = pNode->GetChildNode(0);
	CDataTypeTreeNode* pDataTypeNode = pLeftExprNode->GetDataType();

	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		if(pNode->GetNodeType() == TNK_PREDECREMENT_EXPR)
		{
			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
			SUB_IMM32(RAX,1,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,CR_INVALID,objContext);
		}
		else
		{
			MovIntValtoReg(pLeftExprNode,RAX,CR_INVALID,objContext);
			if(pNode->GetParent())
			{
				PUSH(RAX);
			}
			SUB_IMM32(RAX,1,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,CR_INVALID,objContext);
			if(pNode->GetParent())
			{
				POP(RAX,objContext);  
			}		
		}
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		if(pNode->GetNodeType() == TNK_PREDECREMENT_EXPR)
		{
			CSyntaxTreeNode* pRightExprNode  = pNode->GetChildNode(1);
			FLDVariable(pLeftExprNode,objContext);
			FSUB_M64REAL(pRightExprNode);
			FSTVariable(pLeftExprNode,objContext);
		}
		else
		{
			CSyntaxTreeNode* pRightExprNode  = pNode->GetChildNode(1);
			FLDVariable(pLeftExprNode,objContext);   //注意两次调用FLDVariable不是多余的,后面的一次装载要弹出
			FLDVariable(pLeftExprNode,objContext);
			FSUB_M64REAL(pRightExprNode);
			FSTPVariable(pLeftExprNode,objContext);
		}
		break;   

	case TNK_LONGLONG_TYPE:
		if(pNode->GetNodeType() == TNK_PREDECREMENT_EXPR)
		{
			MovIntValtoReg(pLeftExprNode,RAX,RDX,objContext);
			SUB_IMM32(RAX,1,objContext);
			ADC_IMM32(RDX,0,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,RDX,objContext);
		}
		else
		{
			MovIntValtoReg(pLeftExprNode,RAX,RDX,objContext);
			if(pNode->GetParent())
			{
				PUSH(RAX);
				PUSH(RDX);
			}
			SUB_IMM32(RAX,1,objContext);
			ADC_IMM32(RDX,0,objContext);
			MovRegtoIntVal(pLeftExprNode,RAX,RDX,objContext);
			if(pNode->GetParent())
			{
				POP(RDX,objContext);
				POP(RAX,objContext);  
			}
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprNegateCode
| 描  述 : 处理负号(-)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprNegateCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode  = pNode->GetChildNode(0);
	CDataTypeTreeNode* pDataTypeNode = pLeftExprNode->GetDataType();

	GenerateExprStatementsCode(pLeftExprNode,objContext);
	switch(pDataTypeNode->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE:  
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		NEG(RAX,objContext);
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		FCHS();
		break;   

	case TNK_LONGLONG_TYPE:
		NEG(RAX,objContext);
		ADC_IMM32(RDX,0,objContext);
		NEG(RDX,objContext);
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::RequestReg
| 描  述 : 请求一个寄存器,如果该寄存器不空,就把它推进栈
| 参  数 : COMMON_REGISTERS_MASK nRegMask——寄存器掩码
| 参  数 : BOOL bOther——是否可以采用其它空闲寄存器
| 参  数 : CContext& objContext——上下文对象
| 参  数 : UINT& nStatus——保存推进栈寄存器的掩码
| 返  回 : 被请求的寄存器编号
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
COMMON_REGISTERS CGenerateSaxProgram::RequestReg(COMMON_REGISTERS_MASK nRegMask
	,BOOL bOther,CContext& objContext,UINT& nStatus)
{
	COMMON_REGISTERS nFreeReg = objContext.RequestReg(nRegMask,bOther);
	if(CR_INVALID == nFreeReg)
	{
		nFreeReg = Mask2Reg(nRegMask);
		PUSH(nFreeReg);

		nStatus |= nRegMask;
	}
	objContext.ReserveReg(nFreeReg);

	return nFreeReg;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::FreeReg
| 描  述 : 释放一个寄存器,如果该寄存器被推进栈就出栈
| 参  数 : COMMON_REGISTERS_MASK nRegMask——寄存器掩码
| 参  数 : CContext& objContext——上下文对象
| 参  数 : UINT nStatus——保存推进栈寄存器的掩码,由RequestReg返回
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FreeReg(COMMON_REGISTERS nReg,CContext& objContext,UINT nStatus)
{
	objContext.FreeReg(nReg);

	UINT nRegMask = Reg2Mask(nReg);

	if(nRegMask & nStatus)
	{
		POP(nReg,objContext);
	}
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprMultCode
| 描  述 : 处理乘法(＊)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备注：　已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprMultCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		FloatingOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
		break;

	case TNK_LONGLONG_TYPE:
		ATLASSERT(ACM_MODE_X86 == m_nMode);
		switch(pLeftNodeDataType->GetNodeType())
		{
		case TNK_CHAR_TYPE:
		case TNK_BOOLEAN_TYPE: 
		case TNK_BYTE_TYPE:        
		case TNK_SHORT_TYPE:
		case TNK_WORD_TYPE:
		case TNK_INT_TYPE:
		case TNK_LONG_TYPE:
		case TNK_DWORD_TYPE:           
		case TNK_POINTER_TYPE:   
			{//pLeftNodeDataType为常量的情况没有做处理
				UINT nStatus = 0;
				COMMON_REGISTERS nFreeReg = CR_INVALID;				

				if(!pLeftExprNode->IsExpression())
				{
					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MovIntValtoReg(pLeftExprNode,nFreeReg,CR_INVALID,objContext);
				}
				else
				{
					GenerateExprStatementsCode(pLeftExprNode,objContext);
					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);
				}
				GenerateExprStatementsCode(pRightExprNode,objContext); 
				ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);

				IMUL(RDX,nFreeReg,objContext);
				MOV(RBX,RDX,objContext);
				IMUL(nFreeReg,objContext);   //有符号乘法：EDX:EAX←EAX*r/m32
				ADD(RDX,RBX,objContext);

				FreeReg(nFreeReg,objContext,nStatus);				
			}
			break;

		case TNK_LONGLONG_TYPE: 
			{
				UINT nStatus = 0;

				if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)  //eg. int * LONGLONG
				{
					if(pRightExprNode->IsExpression())
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);

						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						MOV(RCX,RAX,objContext);
					}
					else
					{
						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						MovIntValtoReg(pRightExprNode,RCX,CR_INVALID,objContext);
					}

					GenerateExprStatementsCode(pLeftExprNode,objContext);
					IMUL(RDX,RCX,objContext);

					RequestReg(CRM_RBX,FALSE,objContext,nStatus);
					MOV(RBX,RDX,objContext);
					MUL(RCX,objContext);
					ADD(RDX,RBX,objContext);
				}
				else          //eg. LONLONG * LONGLONG,重点测试
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);
					PUSH(RDX);

					RequestReg(CRM_RCX,FALSE,objContext,nStatus);
					MOV(RCX,RAX,objContext);  // ecx has BLO					
					GenerateExprStatementsCode(pLeftExprNode,objContext);

					RequestReg(CRM_RBX,FALSE,objContext,nStatus);

					PUSH(RAX);
					MOV(RAX,RDX,objContext);  //eax has AHI

					MUL(RCX,objContext);    //AHI * BLO
					MOV(RBX,RAX,objContext);  //save result

					POP(RAX,objContext);     //eax has ALO
					POP(RDX,objContext);     //eax has BHO

					IMUL(RDX,RAX,objContext);  //ALO * BHO
					ADD(RBX,RDX,objContext);   //ebx = ((ALO * BHI) + (AHI * BLO))

					MUL(RCX,objContext);     //ALI * BLO
					ADD(RDX,RBX,objContext);   //now edx has all the LO*HI stuff
				}

				FreeReg(RBX,objContext,nStatus);
				FreeReg(RCX,objContext,nStatus);			
			}
			break;
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

//;        -----------------
//;        |        |
//;        |---------------|
//;        |        |
//;        |--divisor (b)--|
//;        |        |
//;        |---------------|
//;        |        |
//;        |--dividend (a)-|
//;        |        |
//;        |---------------|
//;        | return addr** |
//;        |---------------|
//;        |   EDI   |
//;        |---------------|
//;        |   ESI   |
//;        |---------------|
//;    ESP---->|   EBX   |
//;        -----------------

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprDivCode
| 描  述 : 处理乘法(/)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprDivCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);    
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		FloatingOperator(pLeftExprNode,pRightExprNode,pNode,objContext);       
		break;

	case TNK_LONGLONG_TYPE:
		ATLASSERT(ACM_MODE_X86 == m_nMode);
		{
			UINT nStatus = 0;

			UINT nPos = 0;
			const int CUR_BSP = objContext.GetOffset();
			SUB_IMM32(RSP,16,objContext);          //在栈中分配空间用来存储两个操作数 

			GenerateExprStatementsCode(pRightExprNode,objContext);
			if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)  //eg. int * LONGLONG
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);         //把除数的低位移动到栈中
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);         //把除数的高位移动到栈中

			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);       //把被除数的低位移动到栈中
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);       //把被除数的高位移动到栈中

			RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			RequestReg(CRM_RBX,FALSE,objContext,nStatus);
			RequestReg(CRM_RSI,FALSE,objContext,nStatus);
			RequestReg(CRM_RDI,FALSE,objContext,nStatus);

			//Determine sign of the result (edi = 0 if result is positive, non-zero
			// otherwise) and make operands positive.
			XOR(RDI,RDI,objContext);                   //result sign assumed positive
			OR(RDX,RDX,objContext);                    //test to see if signed
			JGE(0x14,objContext);                     //jge   short L1,kip rest if a is already positive
			nPos = GetOffset();
			INC(RDI,objContext);                     //complement result sign flag
			NEG(RDX,objContext);                     //make a positive
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);        //save positive value
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);        
			//L1:      
			WriteByteData(nPos - 1,(BYTE)(GetOffset() - nPos),objContext);

			MOV_REG_LIV32(RAX,-(CUR_BSP + 8),objContext);
			MOV_REG_LIV32(RDX,-(CUR_BSP + 4),objContext);
			OR(RDX,RDX,objContext);
			JGE(0x14,objContext);                     //jge   short L2,kip rest if a is already positive
			nPos = GetOffset();
			INC(RDI,objContext);                     //complement result sign flag
			NEG(RDX,objContext);                     //make a positive
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);        //save positive value
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);        
			//L2:      
			WriteByteData(nPos - 1,(BYTE)(GetOffset() - nPos),objContext);

			/*
			; Now do the divide. First look to see if the divisor is less than 4194304K.
			; If so, then we can use a simple algorithm with word divides, otherwise
			; things get a little more complex.
			;
			; NOTE - rdx currently contains the high order word of DVSR*/
			OR(RDX,RDX,objContext);              //check to see if divisor < 4194304K
			JNE(0x18,objContext);               //jne   short L3    ; nope, gotta do this the hard way
			nPos = GetOffset();
			MOV(RCX,RAX,objContext);              //load divisor
			MOV_REG_LIV32(RAX,-(CUR_BSP + 12),objContext);   //load high word of dividend
			XOR(RDX,RDX,objContext);
			DIV(RCX,objContext);                //eax <- high order bits of quotient
			MOV(RBX,RAX,objContext);              //save high bits of quotient
			MOV_REG_LIV32(RAX,-(CUR_BSP + 16),objContext);   //eax <- remainder:lo word of dividend
			DIV(RCX,objContext);                //eax <- low order bits of quotient
			MOV(RDX,RBX,objContext);              //edx:eax <- quotient
			JMP_IMM32(0x41,objContext);            //jmp   short L4,set sign, restore stack and return
			UINT nL4RefPos = GetOffset();
			//L3:      
			WriteByteData(nPos - 1,(BYTE)(GetOffset() - nPos),objContext);

			/*Here we do it the hard way. Remember, rdx contains the high word of DVSR*/
			MOV(RBX,RDX,objContext);             // ebx:ecx <- divisor
			MOV_REG_LIV32(RCX,-(CUR_BSP + 8),objContext);

			MOV_REG_LIV32(RAX,-(CUR_BSP + 16),objContext);  //edx:eax <- dividend
			MOV_REG_LIV32(RDX,-(CUR_BSP + 12),objContext); 
			//L5:
			UINT nL5Offset = GetOffset();        //保存L5偏移地址
			SHR(RBX,1,objContext);              //shift divisor right one bit
			RCR(RCX,1,objContext);
			SHR(RDX,1,objContext);              //shift dividend right one bit
			RCR(RAX,1,objContext);
			OR(RBX,RBX,objContext);
			JNE(nL5Offset - GetOffset(),objContext);     //jnz   short L5    ; loop until divisor < 4194304K
			DIV(RCX,objContext);               //now divide, ignore remainder
			MOV(RSI,RAX,objContext);             //save quotient

			/*We may be off by one, so to check, we will multiply the quotient
			; by the divisor and check the result against the orignal dividend
			; Note that we must also check for overflow, which can occur if the
			; dividend is close to 2**64 and the quotient is off by 1.*/
			MUL(int(-(CUR_BSP + 4)),objContext);        //QUOT * HIWORD(DVSR)
			MOV(RCX,RAX,objContext);
			MOV_REG_LIV32(RAX,-(CUR_BSP + 8),objContext);
			MUL(RSI,objContext);                //QUOT * LOWORD(DVSR)
			ADD(RDX,RCX,objContext);              //EDX:EAX = QUOT * DVSR
			JC(0x0E,objContext);                //carry means Quotient is off by 1
			UINT nL6RefPos1 = GetOffset();

			/*do long compare here between original dividend and the result of the
			; multiply in edx:eax. If original is larger or equal, we are ok, otherwise
			; subtract one (1) from the quotient.*/
			CMP(RDX,-(CUR_BSP + 4));           //compare hi words of result and original
			JA(0x08,objContext);                //ja   short L6,if result > original, do subtract
			UINT nL6RefPos2 = GetOffset();
			JB(0x07,objContext);                //jb   short L7 ; if result < original, we are ok
			UINT nL7RefPos1 = GetOffset();     
			CMP(RAX,-(CUR_BSP + 8));           //hi words are equal, compare lo words
			JBE(0x1,objContext);                //jbe   short L7  ; if less or equal we are ok, else subtract
			UINT nL7RefPos2 = GetOffset(); 
			//L6:

			WriteByteData(nL6RefPos1 - 1,(BYTE)(GetOffset() - nL6RefPos1),objContext);
			WriteByteData(nL6RefPos2 - 1,(BYTE)(GetOffset() - nL6RefPos2),objContext);

			DEC(RSI,objContext);

			//L7:
			WriteByteData(nL7RefPos1 - 1,(BYTE)(GetOffset() - nL7RefPos1),objContext);
			WriteByteData(nL7RefPos2 - 1,(BYTE)(GetOffset() - nL7RefPos2),objContext);

			XOR(RDX,RDX,objContext);
			MOV(RAX,RSI,objContext);

			/*Just the cleanup left to do. edx:eax contains the quotient. Set the sign
			; according to the save value, cleanup the stack, and return.*/

			//L4:
			WriteByteData(nL4RefPos - 1,(BYTE)(GetOffset() - nL4RefPos),objContext);

			DEC(RDI,objContext);           //check to see if result is negative
			JNE(0x07,objContext);          //jnz   short L8    ; if EDI == 0, result should be negative
			UINT nL8RefPos1 = GetOffset(); 

			NEG(RDX,objContext);           //otherwise, negate the result
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);


			//L8:
			WriteByteData(nL8RefPos1 - 1,(BYTE)(GetOffset() - nL8RefPos1),objContext);

			// Restore the saved registers and return.
			ADD_IMM32(RSP,16,objContext);  //释放空间

			FreeReg(RDI,objContext,nStatus);
			FreeReg(RSI,objContext,nStatus);
			FreeReg(RBX,objContext,nStatus);
			FreeReg(RCX,objContext,nStatus);
		}       
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprModCode
| 描  述 : 处理模(%)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprModCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);
		MOV(RAX,RDX,objContext);  //RDX存储是余数
		break;   

	case TNK_LONGLONG_TYPE:
		{
			UINT nStatus = 0;

			const int CUR_BSP = objContext.GetOffset();
			SUB_IMM32(RSP,16,objContext);          //在栈中分配空间用来存储两个操作数 

			GenerateExprStatementsCode(pRightExprNode,objContext);
			if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);         //把结果移动到栈中
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);         //把结果移动到栈中

			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);       //把被除数的低位移动到栈中
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);       //把被除数的高位移动到栈中

			RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			RequestReg(CRM_RBX,FALSE,objContext,nStatus);
			RequestReg(CRM_RSI,FALSE,objContext,nStatus);
			RequestReg(CRM_RDI,FALSE,objContext,nStatus);

			//Determine sign of the result (edi = 0 if result is positive, non-zero
			// otherwise) and make operands positive.
			XOR(RDI,RDI,objContext);                   //result sign assumed positive
			OR(RDX,RDX,objContext);                    //test to see if signed
			JGE(0x14,objContext);                     //jge   short L1,kip rest if a is already positive
			UINT nL1RefPos = GetOffset();
			INC(RDI,objContext);                     //complement result sign flag
			NEG(RDX,objContext);                     //make a positive
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);        //save positive value
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);        
			//L1:      
			WriteByteData(nL1RefPos - 1,(BYTE)(GetOffset() - nL1RefPos),objContext);

			MOV_REG_LIV32(RAX,-(CUR_BSP + 8),objContext);
			MOV_REG_LIV32(RDX,-(CUR_BSP + 4),objContext);
			OR(RDX,RDX,objContext);
			JGE(0x13,objContext);                     //jge   short L2,kip rest if a is already positive
			UINT nL2RefPos = GetOffset();
			NEG(RDX,objContext);                     //make a positive
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);        //save positive value
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);        
			//L2:      
			WriteByteData(nL2RefPos - 1,(BYTE)(GetOffset() - nL2RefPos),objContext);

			/*
			; Now do the divide. First look to see if the divisor is less than 4194304K.
			; If so, then we can use a simple algorithm with word divides, otherwise
			; things get a little more complex.
			;
			; NOTE - rdx currently contains the high order word of DVSR*/
			OR(RDX,RDX,objContext);              //check to see if divisor < 4194304K
			JNE(0x18,objContext);               //jne   short L3    ; nope, gotta do this the hard way
			UINT nL3RefPos = GetOffset();
			MOV(RCX,RAX,objContext);              //load divisor
			MOV_REG_LIV32(RAX,-(CUR_BSP + 12),objContext);   //load high word of dividend
			XOR(RDX,RDX,objContext);
			DIV(RCX,objContext);                //eax <- high order bits of quotient
			MOV_REG_LIV32(RAX,-(CUR_BSP + 16),objContext);   //edx:eax <- remainder:lo word of dividend
			DIV(RCX,objContext);
			MOV(RAX,RDX,objContext);              //save high bits of quotient
			XOR(RDX,RDX,objContext);
			DEC(RDI,objContext);                //check result sign flag
			JNS(0,objContext);                 //negate result, restore stack and return
			UINT nL4RefPos = GetOffset();
			JMP_IMM32(0,objContext);              //result sign ok, restore stack and return
			UINT nL8RefPos2 = GetOffset();
			//L3:      
			WriteByteData(nL3RefPos - 1,(BYTE)(GetOffset() - nL3RefPos),objContext);

			/*Here we do it the hard way. Remember, rdx contains the high word of DVSR*/
			MOV(RBX,RDX,objContext);             // ebx:ecx <- divisor
			MOV_REG_LIV32(RCX,-(CUR_BSP + 8),objContext);

			MOV_REG_LIV32(RAX,-(CUR_BSP + 16),objContext);  //edx:eax <- dividend
			MOV_REG_LIV32(RDX,-(CUR_BSP + 12),objContext);
			//L5:
			UINT nL5Offset = GetOffset();     //保存L5偏移地址
			SHR(RBX,1,objContext);              //shift divisor right one bit
			RCR(RCX,1,objContext);
			SHR(RDX,1,objContext);              //shift dividend right one bit
			RCR(RAX,1,objContext);
			OR(RBX,RBX,objContext);
			JNE(nL5Offset - GetOffset(),objContext);     //jnz   short L5    ; loop until divisor < 4194304K
			DIV(RCX,objContext);               //now divide, ignore remainder

			/*We may be off by one, so to check, we will multiply the quotient
			; by the divisor and check the result against the orignal dividend
			; Note that we must also check for overflow, which can occur if the
			; dividend is close to 2**64 and the quotient is off by 1.*/

			MOV(RCX,RAX,objContext);             //save a copy of quotient in ECX
			MUL(-(CUR_BSP + 4),objContext);
			XCHG(RCX,RAX,objContext);             //save product, get quotient in EAX
			MUL(-(CUR_BSP + 8),objContext);
			ADD(RDX,RCX,objContext);             //EDX:EAX = QUOT * DVSR
			JC(0x0E,objContext);               //carry means Quotient is off by 1
			UINT nL6RefPos1 = GetOffset();

			/*do long compare here between original dividend and the result of the
			; multiply in edx:eax. If original is larger or equal, we are ok, otherwise
			; subtract one (1) from the quotient.*/
			CMP(RDX,-(CUR_BSP + 12));         //compare hi words of result and original
			JA(0x08,objContext);                //ja   short L6,if result > original, do subtract
			UINT nL6RefPos2 = GetOffset();
			JB(0x07,objContext);                //jb   short L7 ; if result < original, we are ok
			UINT nL7RefPos1 = GetOffset();     
			CMP(RAX,-(CUR_BSP + 16));         //hi words are equal, compare lo words
			JBE(0x1,objContext);                //jbe   short L7  ; if less or equal we are ok, else subtract
			UINT nL7RefPos2 = GetOffset(); 
			//L6:
			WriteByteData(nL6RefPos1 - 1,(BYTE)(GetOffset() - nL6RefPos1),objContext);
			WriteByteData(nL6RefPos2 - 1,(BYTE)(GetOffset() - nL6RefPos2),objContext);

			SUB(RAX,-(CUR_BSP + 8),objContext);         //subtract divisor from result
			SBB(RDX,-(CUR_BSP + 4),objContext);

			//L7:
			/*; Calculate remainder by subtracting the result from the original dividend.
			; Since the result is already in a register, we will do the subtract in the
			; opposite direction and negate the result if necessary.*/

			WriteByteData(nL7RefPos1 - 1,(BYTE)(GetOffset() - nL7RefPos1),objContext);
			WriteByteData(nL7RefPos2 - 1,(BYTE)(GetOffset() - nL7RefPos2),objContext);

			SUB(RAX,-(CUR_BSP + 16),objContext);         //subtract divisor from result
			SBB(RDX,-(CUR_BSP + 12),objContext);

			/*; Now check the result sign flag to see if the result is supposed to be positive
			; or negative. It is currently negated (because we subtracted in the 'wrong'
			; direction), so if the sign flag is set we are done, otherwise we must negate
			; the result to make it positive again.*/

			DEC(RDI,objContext);           //check to see if result is negative
			JNE(0x07,objContext);          //jnz   short L8    ; if EDI == 0, result should be negative
			UINT nL8RefPos1 = GetOffset(); 
			//L4:
			WriteByteData(nL4RefPos - 1,(BYTE)(GetOffset() - nL4RefPos),objContext);

			NEG(RDX,objContext);           //otherwise, negate the result
			NEG(RAX,objContext);
			SBB_IMM32(RDX,0,objContext);
			//L8:
			WriteByteData(nL8RefPos1 - 1,(BYTE)(GetOffset() - nL8RefPos1),objContext);
			WriteByteData(nL8RefPos2 - 1,(BYTE)(GetOffset() - nL8RefPos2),objContext);

			// Restore the saved registers and return.
			ADD_IMM32(RSP,16,objContext);  //释放空间

			FreeReg(RDI,objContext,nStatus);
			FreeReg(RSI,objContext,nStatus);
			FreeReg(RBX,objContext,nStatus);
			FreeReg(RCX,objContext,nStatus);
		}       
		break;
	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprPlusCode
| 描  述 : 处理加法(+)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备注:已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprPlusCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
		if(pExprDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
		{
			SETNE(RAX,objContext);
		}
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		FloatingOperator(pLeftExprNode,pRightExprNode,pNode,objContext);
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);		

			if(pLeftExprNode->IsNumericConstants())
			{				
				GenerateExprStatementsCode(pRightExprNode,objContext);  //
				if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}
				ADD_IMM32(RAX,pLeftExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				ADC_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);

			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);  // 
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				ADD_IMM32(RAX,pRightExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				ADC_IMM32(RDX,pRightExprNode->GetValue1() >> 32,objContext);    
			}
			else
			{
				UINT nStatus = 0;
				COMMON_REGISTERS nFreeReg = CR_INVALID;
				if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);   

					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);

					GenerateExprStatementsCode(pLeftExprNode,objContext);

					ADD(RAX,nFreeReg,objContext);
					ADC_IMM32(RDX,0,objContext);  

					FreeReg(nFreeReg,objContext,nStatus);
				}
				else
				{
					if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						GenerateExprStatementsCode(pLeftExprNode,objContext);   
						nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
						MOV(nFreeReg,RAX,objContext);

						GenerateExprStatementsCode(pRightExprNode,objContext);
						ADD(RAX,nFreeReg,objContext);
						ADC_IMM32(RDX,0,objContext);

						FreeReg(nFreeReg,objContext,nStatus);
					}
					else
					{						
						GenerateExprStatementsCode(pRightExprNode,objContext);

						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						RequestReg(CRM_RBX,FALSE,objContext,nStatus);

						MOV(RBX,RDX,objContext); 
						MOV(RCX,RAX,objContext); 

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						ADD(RAX,RCX,objContext);
						ADC_IMM32(RDX,0,objContext);
						ADD(RDX,RBX,objContext);

						FreeReg(RBX,objContext,nStatus);
						FreeReg(RCX,objContext,nStatus);						
					}           
				}
			}   
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprMinusCode
| 描  述 : 处理减法(-)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备　注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprMinusCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
		if(pExprDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
		{
			SETNE(RAX,objContext);
		}
		break;

	case TNK_FLOAT_TYPE:       
	case TNK_DOUBLE_TYPE:
		FloatingOperator(pLeftExprNode,pRightExprNode,pNode,objContext);
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);

			UINT nStatus = 0;
			COMMON_REGISTERS nFreeReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);

			if(pLeftExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);  //
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{   
					ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);
					MOV(nFreeReg,RAX,objContext);

					MOV_REG_IMM32(RAX,(int)pLeftExprNode->GetValue1(),objContext);
					SUB(RAX,nFreeReg,objContext);

					MOV(nFreeReg,RDX,objContext);
					MOV_REG_IMM32(RDX,0,objContext);
					SBB(RDX,nFreeReg,objContext);   
				}
				else
				{
					MOV(nFreeReg,RAX,objContext);
					MOV_REG_IMM32(RAX,pLeftExprNode->GetValue1() & 0xFFFFFFFF,objContext);

					SUB(RAX,nFreeReg,objContext);
					if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						MOV_REG_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);
						SBB_IMM32(RDX,0,objContext);   
					}
					else
					{
						MOV(nFreeReg,RDX,objContext);
						MOV_REG_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);
						SBB(RDX,nFreeReg,objContext);   
					}
				}
			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);  //  
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				SUB_IMM32(RAX,pRightExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				SBB_IMM32(RDX,pRightExprNode->GetValue1() >> 32,objContext);        
			}
			else
			{
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);   
					ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);

					MOV(nFreeReg,RAX,objContext);

					GenerateExprStatementsCode(pLeftExprNode,objContext);

					SUB(RAX,nFreeReg,objContext);

					MOV(nFreeReg,RDX,objContext);
					MOV_REG_IMM32(RDX,0,objContext);
					SBB(RDX,nFreeReg,objContext);   
				}
				else
				{
					if(pRightExprNode->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);   
						MOV(nFreeReg,RAX,objContext);

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						SUB(RAX,nFreeReg,objContext);
						SBB_IMM32(RDX,0,objContext);   
					}
					else
					{
						RequestReg(CRM_RBX,FALSE,objContext,nStatus);

						GenerateExprStatementsCode(pRightExprNode,objContext);
						MOV(RBX,RDX,objContext); 
						MOV(RCX,RAX,objContext); 

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						SUB(RAX,RCX,objContext);
						SBB(RDX,RBX,objContext);

						FreeReg(RBX,objContext,nStatus);
					}           
				}
			}    

			FreeReg(RCX,objContext,nStatus);
			break;
		}
	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprSHLCode
| 描  述 : 处理左移(<<)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprSHLCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	//CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	//CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
		if(pExprDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
		{
			SETNE(RAX,objContext);
		}
		break;

	case TNK_LONGLONG_TYPE:
		{
			UINT nStatus = 0;

			GenerateExprStatementsCode(pRightExprNode,objContext);
			MOV(RCX,RAX,objContext); 

			RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			GenerateExprStatementsCode(pLeftExprNode,objContext);

			CMP_IMM8(RCX,64);     //Handle shifts of 64 or more bits (all get 0)
			JAE(0x15,objContext);        //jae   short RETZERO
			UINT nRETZERORefPos1 = GetOffset();

			CMP_IMM8(RCX,32);     //Handle shifts of 64 or more bits (all get 0)
			JAE(0x6,objContext);        //jae   short MORE32
			UINT nMORE32RefPos1 = GetOffset();
			SHLD(RDX,RAX,objContext);
			SHL(RAX,objContext);
			JMP_IMM32(0,objContext);
			UINT nRetRefPos1 = GetOffset();
			//MORE32:
			WriteByteData(nMORE32RefPos1 - 1,(BYTE)(GetOffset() - nMORE32RefPos1),objContext); 
			MOV(RDX,RAX,objContext);
			XOR(RAX,RAX,objContext);
			AND_IMM32(RCX,31,objContext);
			SHL(RDX,objContext);

			JMP_IMM32(0,objContext);
			UINT nRetRefPos2 = GetOffset();
			//RETZERO:
			WriteByteData(nRETZERORefPos1 - 1,(BYTE)(GetOffset() - nRETZERORefPos1),objContext); 

			XOR(RAX,RAX,objContext);
			XOR(RDX,RDX,objContext);
			//RET:
			WriteByteData(nRetRefPos1 - 1,(BYTE)(GetOffset() - nRetRefPos1),objContext); 
			WriteByteData(nRetRefPos2 - 1,(BYTE)(GetOffset() - nRetRefPos2),objContext); 

			FreeReg(RCX,objContext,nStatus);
		}
		break;
	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprSHRCode
| 描  述 : 处理右移(>>)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprSHRCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	//CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	//CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
		if(pExprDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
		{
			SETNE(RAX,objContext);
		}
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);

			UINT nStatus = 0;

			GenerateExprStatementsCode(pRightExprNode,objContext);
			MOV(RCX,RAX,objContext); 

			RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			GenerateExprStatementsCode(pLeftExprNode,objContext);

			CMP_IMM8(RCX,64);     //Handle shifts of 64 or more bits (all get 0)
			JAE(0x15,objContext);        //jae   short RETSIGN
			UINT nRETSIGNRefPos1 = GetOffset();

			CMP_IMM8(RCX,32);     //Handle shifts of 64 or more bits (all get 0)
			JAE(0x6,objContext);        //jae   short MORE32
			UINT nMORE32RefPos1 = GetOffset();
			SHRD(RAX,RDX,objContext);
			SAR(RDX,objContext);
			JMP_IMM32(0,objContext);
			UINT nRetRefPos1 = GetOffset();
			//MORE32:
			WriteByteData(nMORE32RefPos1 - 1,(BYTE)(GetOffset() - nMORE32RefPos1),objContext); 
			MOV(RAX,RDX,objContext);
			SAR(RDX,31,objContext);
			AND_IMM32(RCX,31,objContext);
			SAR(RAX,objContext);

			JMP_IMM32(0,objContext);
			UINT nRetRefPos2 = GetOffset();
			//RETSIGN:
			WriteByteData(nRETSIGNRefPos1 - 1,(BYTE)(GetOffset() - nRETSIGNRefPos1),objContext); 

			SAR(RDX,31,objContext);
			MOV(RAX,RDX,objContext);
			//RET:
			WriteByteData(nRetRefPos1 - 1,(BYTE)(GetOffset() - nRetRefPos1),objContext); 
			WriteByteData(nRetRefPos2 - 1,(BYTE)(GetOffset() - nRetRefPos2),objContext); 

			FreeReg(RCX,objContext,nStatus);
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExpComparisonOpCode
| 描  述 : 处理比较(>,>=,<,<=)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExpComparisonOpCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode        = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType  = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode       = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	if(pLeftNodeDataType->IsFloat()	|| pRightNodeDataType->IsFloat())
	{
		FloatingOperator(pLeftExprNode,pRightExprNode,pNode,objContext);
	}
	else if(pLeftNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE
		|| pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
	{
		CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
		ATLASSERT(pConditionExprJmpRef);
		JMP_TYPE nJmpType    = pConditionExprJmpRef->GetJmpType();
		JMP_TYPE nResultType = nJmpType;

		BOOL bIsUnsigned = pNode->IsUnsigned();   //得到表达式是否是无符号

		//UINT nTrueRefPos = 0,nFalseRefPos1 = 0,nFalseRefPos2 = 0/*,nEndRefPos = 0*/;
		const UINT nOperator = pNode->GetNodeType();
		if(pLeftExprNode->IsNumericConstants() || pRightExprNode->IsNumericConstants())
		{
			LONGLONG nConstVlaue = 0;
			if(pLeftExprNode->IsNumericConstants())   //下面的代码不能合并
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);
				if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}
				nConstVlaue = pLeftExprNode->GetValue1();

				nJmpType = (JT_FALSE == nJmpType)? JT_TRUE:JT_FALSE;   //操作数交换,要求的结果也要改变
			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}
				nConstVlaue = pRightExprNode->GetValue1();
			}

			switch(nOperator)
			{
			case TNK_LT_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                     //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((nJmpType == nResultType)? JIC_JNB : JIC_JA,TMP_ADDR_BYTE4);   //JIC_JAE ==  JIC_JNB
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE((nJmpType == nResultType)? JIC_JB : JIC_JBE,TMP_ADDR_BYTE4);   
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;

			case TNK_NLT_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((nJmpType == nResultType)?JIC_JA : JIC_JNB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE((nJmpType == nResultType)? JIC_JBE : JIC_JB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;

			case TNK_GT_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((nJmpType == nResultType)?JIC_JBE : JIC_JB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE((nJmpType == nResultType)? JIC_JA : JIC_JNB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;

			case TNK_NGT_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());
				}
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((nJmpType == nResultType)? JIC_JB : JIC_JB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE((nJmpType == nResultType)? JIC_JNB : JIC_JA,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;

			case TNK_EQ_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());					
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());	
				}
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;

			case TNK_NEQ_EXPR:
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nResultType),GetOffset());					
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}

				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nResultType,GetOffset());
				}
				break;
			}			

			////true:
			//WriteByteData(nTrueRefPos - 1,(BYTE)((BYTE)(GetOffset() - nTrueRefPos)),objContext); 
			//MOV_REG_IMM32(RAX,1,objContext);
			//JMP_IMM32(0x0A,objContext);
			//nEndRefPos = GetOffset();
			////FALSE:
			//WriteByteData(nFalseRefPos1 - 1,(BYTE)((BYTE)(GetOffset() - nFalseRefPos1)),objContext); 
			//WriteByteData(nFalseRefPos2 - 1,(BYTE)((BYTE)(GetOffset() - nFalseRefPos2)),objContext); 
			//MOV_REG_IMM32(RAX,0,objContext);
			////End:     
			//WriteByteData(nEndRefPos - 1,(BYTE)((BYTE)(GetOffset() - nEndRefPos)),objContext); 
		}
		else
		{
			UINT nStatus = 0;
			GenerateExprStatementsCode(pRightExprNode,objContext);

			MOV(RCX,RAX,objContext);
			if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
			{
				XOR(RDX,RDX,objContext);
			}
			else
			{
				MOV(RBX,RDX,objContext);
			}
			RequestReg(CRM_RBX,FALSE,objContext,nStatus);
			RequestReg(CRM_RCX,FALSE,objContext,nStatus);
			
			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
			{
				XOR(RDX,RDX,objContext);
			}

			switch(nOperator)
			{
			case TNK_LT_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				
				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNB,TMP_ADDR_BYTE4);   //JIC_JAE ==  JIC_JNB
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JB,TMP_ADDR_BYTE4);   
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;

			case TNK_NLT_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				
				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JA,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JBE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;

			case TNK_GT_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				
				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JBE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JA,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;

			case TNK_NGT_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				else
				{
					JMP_BASE((bIsUnsigned)? JIC_JA:JIC_JG,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());

					JMP_BASE((bIsUnsigned)? JIC_JB:JIC_JL,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());
				}
				
				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JNB,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;

			case TNK_EQ_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());					
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());	
				}
				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;

			case TNK_NEQ_EXPR:
				CMP(RDX,RBX);                      //比较高位
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(SWITCH_JMP_TYPE(nJmpType),GetOffset());					
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}

				CMP(RAX,RCX);                      //比较地位部分
				if(JT_FALSE == nJmpType)
				{ 
					JMP_BASE(JIC_JE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				else
				{
					JMP_BASE(JIC_JNE,TMP_ADDR_BYTE4);
					pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
				}
				break;
			}
			////true:
			//WriteByteData(nTrueRefPos - 1,(BYTE)(GetOffset() - nTrueRefPos),objContext); 
			//MOV_REG_IMM32(RAX,1,objContext);
			//JMP_IMM32(0x0A,objContext);
			//nEndRefPos = GetOffset();
			////FALSE:
			//WriteByteData(nFalseRefPos1 - 1,(BYTE)(GetOffset() - nFalseRefPos1),objContext); 
			//WriteByteData(nFalseRefPos2 - 1,(BYTE)(GetOffset() - nFalseRefPos2),objContext); 
			//MOV_REG_IMM32(RAX,0,objContext);
			////End:     
			//WriteByteData(nEndRefPos - 1,(BYTE)(GetOffset() - nEndRefPos),objContext); 

			FreeReg(RCX,objContext,nStatus);
			FreeReg(RBX,objContext,nStatus);			
		}

		//pNode->SetTrueRefPos(nTrueRefPos);
		//pNode->SetFalseRefPos1(nFalseRefPos1);
		//pNode->SetFalseRefPos2(nFalseRefPos2);
	}
	else
	{
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);   
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprBitAndCode
| 描  述 : 处理位与(&)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备　注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprBitAndCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode        = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType  = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode       = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);       
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);

			UINT nStatus = 0;
			COMMON_REGISTERS nFreeReg = CR_INVALID;

			if(pLeftExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);  //
				if(pRightExprNode->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				AND_IMM32(RAX,pLeftExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				AND_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);

			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);  //  
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				AND_IMM32(RAX,pRightExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				AND_IMM32(RDX,pRightExprNode->GetValue1() >> 32,objContext);    
			}
			else
			{
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);   
					ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);

					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);
					GenerateExprStatementsCode(pLeftExprNode,objContext);

					AND(RAX,nFreeReg,objContext);
					XOR(RDX,RDX,objContext);

					FreeReg(nFreeReg,objContext,nStatus);
				}
				else
				{
					if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);  
						nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
						MOV(nFreeReg,RAX,objContext);

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						AND(RAX,nFreeReg,objContext);
						XOR(RDX,RDX,objContext);
						FreeReg(nFreeReg,objContext,nStatus);
					}
					else
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);

						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						RequestReg(CRM_RBX,FALSE,objContext,nStatus);
						MOV(RBX,RDX,objContext); 
						MOV(RCX,RAX,objContext); 

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						AND(RAX,RCX,objContext);
						AND(RDX,RBX,objContext);

						FreeReg(RBX,objContext,nStatus);
						FreeReg(RCX,objContext,nStatus);
					}           
				}
			}    
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprBitXORCode
| 描  述 : 处理位异或(^)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备　注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprBitXORCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);       
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);

			UINT nStatus = 0;
			COMMON_REGISTERS nFreeReg = CR_INVALID;

			if(pLeftExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);  //
				if(pRightExprNode->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				XOR_IMM32(RAX,pLeftExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				XOR_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);

			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);  //  
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				XOR_IMM32(RAX,pRightExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				XOR_IMM32(RDX,pRightExprNode->GetValue1() >> 32,objContext);    
			}
			else
			{
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);   
					ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);

					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);
					GenerateExprStatementsCode(pLeftExprNode,objContext);

					XOR(RAX,nFreeReg,objContext);
					XOR_IMM32(RDX,0,objContext);

					FreeReg(nFreeReg,objContext,nStatus);
				}
				else
				{
					if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);  
						nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
						MOV(nFreeReg,RAX,objContext);

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						XOR(RAX,nFreeReg,objContext);
						XOR_IMM32(RDX,0,objContext);

						FreeReg(nFreeReg,objContext,nStatus);
					}
					else
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);

						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						RequestReg(CRM_RBX,FALSE,objContext,nStatus);
						MOV(RBX,RDX,objContext); 
						MOV(RCX,RAX,objContext); 

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						XOR(RAX,RCX,objContext);
						XOR(RDX,RBX,objContext);

						FreeReg(RBX,objContext,nStatus);
						FreeReg(RCX,objContext,nStatus);
					}           
				}
			}    
		}
		break;

	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprBitORCode
| 描  述 : 处理位或(|)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprBitORCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode    = pNode->GetChildNode(0);
	CDataTypeTreeNode* pLeftNodeDataType = pLeftExprNode->GetDataType();

	CSyntaxTreeNode* pRightExprNode    = pNode->GetChildNode(1);
	CDataTypeTreeNode* pRightNodeDataType = pRightExprNode->GetDataType();

	CDataTypeTreeNode* pExprDataType = pNode->GetDataType();

	switch(pExprDataType->GetNodeType())
	{
	case TNK_BOOLEAN_TYPE: 
	case TNK_CHAR_TYPE:
	case TNK_BYTE_TYPE:        
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           
	case TNK_POINTER_TYPE:
		IntegerOperator(pLeftExprNode,pRightExprNode,pNode,objContext);       
		break;

	case TNK_LONGLONG_TYPE:
		{
			ATLASSERT(ACM_MODE_X86 == m_nMode);

			UINT nStatus = 0;
			COMMON_REGISTERS nFreeReg = CR_INVALID;

			if(pLeftExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);  //
				if(pRightExprNode->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				OR_IMM32(RAX,pLeftExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				OR_IMM32(RDX,pLeftExprNode->GetValue1() >> 32,objContext);
			}
			else if(pRightExprNode->IsNumericConstants())
			{
				GenerateExprStatementsCode(pLeftExprNode,objContext);  //  
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}

				OR_IMM32(RAX,pRightExprNode->GetValue1() & 0xFFFFFFFF,objContext);
				OR_IMM32(RDX,pRightExprNode->GetValue1() >> 32,objContext);    
			}
			else
			{
				if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					GenerateExprStatementsCode(pRightExprNode,objContext);   
					ATLASSERT(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE);

					nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
					MOV(nFreeReg,RAX,objContext);
					GenerateExprStatementsCode(pLeftExprNode,objContext);

					OR(RAX,nFreeReg,objContext);
					OR_IMM32(RDX,0,objContext);

					FreeReg(nFreeReg,objContext,nStatus);
				}
				else
				{
					if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);  
						nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
						MOV(nFreeReg,RAX,objContext);

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						OR(RAX,nFreeReg,objContext);
						OR_IMM32(RDX,0,objContext);

						FreeReg(nFreeReg,objContext,nStatus);
					}
					else
					{
						GenerateExprStatementsCode(pRightExprNode,objContext);

						RequestReg(CRM_RCX,FALSE,objContext,nStatus);
						RequestReg(CRM_RBX,FALSE,objContext,nStatus);
						MOV(RBX,RDX,objContext); 
						MOV(RCX,RAX,objContext); 

						GenerateExprStatementsCode(pLeftExprNode,objContext);

						OR(RAX,RCX,objContext);
						OR(RDX,RBX,objContext);

						FreeReg(RBX,objContext,nStatus);
						FreeReg(RCX,objContext,nStatus);
					}           
				}
			}    
		}
		break;
	default: //操作数不可能是表达式
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExpAndOpCode
| 描  述 : 处理逻辑与(&&)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExpAndOpCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode       = pNode->GetChildNode(0);
	CSyntaxTreeNode* pRightExprNode       = pNode->GetChildNode(1);

	CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
	UINT nStartPos = GetOffset();

	pConditionExprJmpRef->SetJmpType(JT_FALSE);
	GenerateExprStatementsCode(pLeftExprNode,objContext);

	CSyntaxTreeNode* pParentNode = pNode->GetParent();
	ATLASSERT(pParentNode);

	JMP_TYPE nJmpType = JT_FALSE;  //只有下一级是"||"运算要求是JT_TRUE跳转
	if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR
		&& ((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)
	{
		nJmpType = JT_TRUE;
	}

	pConditionExprJmpRef->SetJmpType(nJmpType);
	GenerateExprStatementsCode(pRightExprNode,objContext);


	if(((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)   //只有上一级是"||"并且当前运算式其左边节点
	{
		if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR )
		{
			pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext,nStartPos);
		}	
	}
	
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExpOrOpCode
| 描  述 : 处理逻辑或(||)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExpOrOpCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{
	CSyntaxTreeNode* pLeftExprNode        = pNode->GetChildNode(0);
	CSyntaxTreeNode* pRightExprNode       = pNode->GetChildNode(1);

	CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();

	UINT nStartPos = GetOffset();
	pConditionExprJmpRef->SetJmpType(JT_TRUE);
	GenerateExprStatementsCode(pLeftExprNode,objContext);


	pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext,nStartPos);

	CSyntaxTreeNode* pParentNode = pNode->GetParent();
	ATLASSERT(pParentNode);

	JMP_TYPE nJmpType = JT_FALSE;  //只有下一级是"||"运算并且不是最后的要求是JT_TRUE跳转
	if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR
		&& ((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)
	{
		nJmpType = JT_TRUE;
	}

	pConditionExprJmpRef->SetJmpType(nJmpType);
	GenerateExprStatementsCode(pRightExprNode,objContext);


	if(((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)
	{
		if(pParentNode->GetNodeType() == TNK_TRUTH_AND_EXPR)
		{
			pConditionExprJmpRef->WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext,nStartPos);
		}
		else if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR)
		{
			pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext,nStartPos);
		}
	}

}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExpQuestionOpCode
| 描  述 : 处理问号(?)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExpQuestionOpCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{   
	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	GenerateExprStatementsCode(pNode->GetChildNode(0),objContext);
	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);

	//TRUE:
	GenerateExprStatementsCode(pNode->GetChildNode(1),objContext);
	JMP_IMM32(0x0A,objContext);
	UINT nEndRefPos = GetOffset();

	//FALSE:
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);
	GenerateExprStatementsCode(pNode->GetChildNode(2),objContext);

	//end:
	WriteByteData(nEndRefPos - 1,(BYTE)(GetOffset() - nEndRefPos),objContext); 
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExpCommaOpCode
| 描  述 : 处理逗号(,)运算
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExpCommaOpCode( CExpressionTreeNode* pNode
	,CContext& objContext)
{   
	CSyntaxTreeNode* pExprNode = NULL;
	for(UINT i = 0;i < MAX_CHILDREN_NUMBER;i++)
	{
		pExprNode = pNode->GetChildNode(i);
		if(NULL == pExprNode)
		{
			break;
		}
		GenerateExprStatementsCode(pExprNode,objContext);
	}   
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateExprStatementsCode
| 描  述 : 处理表达式语句,表达式的值如果是整形将放到RAX寄存器中
|       实型放在浮点数寄存器中
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprStatementsCode( CSyntaxTreeNode* pNode,CContext& objContext)
{
	if(NULL == pNode)
	{
		return;
	}

	CSyntaxTreeNode* pParent = pNode->GetParent();
	ATLASSERT(NULL != pParent);
	if(NULL == pParent)
	{
		return;
	}

	switch(pNode->GetNodeType())
	{
	case TNK_CALL_EXPR:  
		GenerateCallSTMTCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_TRUTH_NOT_EXPR:
		GenerateExprLogicalNotCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_BIT_NOT_EXPR:
		GenerateExprBitNotCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_PREINCREMENT_EXPR:
	case TNK_POSTINCREMENT_EXPR:
		GenerateExprIncreaseCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_PREDECREMENT_EXPR:
	case TNK_POSTDECREMENT_EXPR:
		GenerateExprDecreaseCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_NEGATE_EXPR:
		GenerateExprNegateCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_MULT_EXPR:
		GenerateExprMultCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_TRUNC_DIV_EXP:
		GenerateExprDivCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_TRUNC_MOD_EXPR:
		GenerateExprModCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_PLUS_EXPR:
		GenerateExprPlusCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_MINUS_EXPR:
		GenerateExprMinusCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_LSHIFT_EXPR:
		GenerateExprSHLCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_RSHIFT_EXPR:
		GenerateExprSHRCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_LT_EXPR:
	case TNK_NLT_EXPR:
	case TNK_GT_EXPR:
	case TNK_NGT_EXPR:
	case TNK_EQ_EXPR:
	case TNK_NEQ_EXPR:
		{
			CConditionExprJmpRef objConditionExprJmpRef;

			if(NULL != pParent && (pParent->IsArithmeticExpression()  //如果比较运算在被包含在算数运算中,就返回EAX
				|| pParent->GetNodeType() == TNK_RETURN_STMT          
				|| pParent->GetNodeType() == TNK_CALL_EXPR))
			{			
				objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);
			}

			GenerateExpComparisonOpCode((CExpressionTreeNode*)pNode,objContext);

			if(NULL != pParent && (pParent->IsArithmeticExpression() 
				|| pParent->GetNodeType() == TNK_RETURN_STMT   
				|| pParent->GetNodeType() == TNK_CALL_EXPR))
			{
				CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
				ATLASSERT(pConditionExprJmpRef);

				//true:
				pConditionExprJmpRef->WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);			

				MOV_REG_IMM32(RAX,1,objContext);
				JMP_IMM32(0x0A,objContext);
				UINT nEndRefPos = GetOffset();

				//FALSE:
				pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);	
				MOV_REG_IMM32(RAX,0,objContext);

				//End:     
				WriteByteData(nEndRefPos - 1,(BYTE)((BYTE)(GetOffset() - nEndRefPos)),objContext); 

				objContext.SetConditionExprJmpRef(NULL);
			}
		}
		break;

	case TNK_BIT_AND_EXPR:
		GenerateExprBitAndCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_BIT_XOR_EXPR:
		GenerateExprBitXORCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_BIT_IOR_EXPR:
		GenerateExprBitORCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_TRUTH_AND_EXPR:
	case TNK_TRUTH_OR_EXPR:
		{
			CConditionExprJmpRef objConditionExprJmpRef;

			if(NULL != pParent && (pParent->IsArithmeticExpression()  //如果比较运算在被包含在算数运算中,就返回EAX
				|| pParent->GetNodeType() == TNK_RETURN_STMT          
				|| pParent->GetNodeType() == TNK_CALL_EXPR))
			{			
				objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);
			}

			if(pNode->GetNodeType() == TNK_TRUTH_AND_EXPR)
			{ GenerateExpAndOpCode((CExpressionTreeNode*)pNode,objContext); }
			else
			{ GenerateExpOrOpCode((CExpressionTreeNode*)pNode,objContext); }

			if(NULL != pParent && (pParent->IsArithmeticExpression() 
				|| pParent->GetNodeType() == TNK_RETURN_STMT   
				|| pParent->GetNodeType() == TNK_CALL_EXPR))
			{
				CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
				ATLASSERT(pConditionExprJmpRef);

				//true:
				pConditionExprJmpRef->WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);			

				MOV_REG_IMM32(RAX,1,objContext);
				JMP_IMM32(0x0A,objContext);
				UINT nEndRefPos = GetOffset();

				//FALSE:
				pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);	
				MOV_REG_IMM32(RAX,0,objContext);

				//End:     
				WriteByteData(nEndRefPos - 1,(BYTE)((BYTE)(GetOffset() - nEndRefPos)),objContext); 

				objContext.SetConditionExprJmpRef(NULL);
			}
		}
		break;

	case TNK_MODIFY_EXPR:
		GenerateSimpleAssignmentOp(((CDeclarationTreeNode*)((CExpressionTreeNode*)pNode)->GetChildNode(0)), 
			((CExpressionTreeNode*)pNode)->GetChildNode(1), objContext);
		break;

	case TNK_QUESTION_MARK_EXPR:
		GenerateExpQuestionOpCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_COMMA_EXPR:
		GenerateExpCommaOpCode((CExpressionTreeNode*)pNode,objContext);
		break;

	case TNK_VAR_REF:
		ExtractFactorValue(((CExpressionTreeNode*)pNode)->GetChildNode(0),objContext);
		break;

	default:
		ExtractFactorValue(pNode,objContext);
	}	

	//下面是处理算术表达式作为条件表达式
	BOOL bFlag = FALSE;
	switch(pParent->GetNodeType())
	{
	case TNK_TRUTH_OR_EXPR:
	case TNK_TRUTH_AND_EXPR:
		bFlag = TRUE;
		break;

	case TNK_QUESTION_MARK_EXPR:
		bFlag = (((CExpressionTreeNode*)pParent)->GetChildNode(0) == pNode);
		break;

	case TNK_IF_STMT:
		bFlag = (((CExpressionTreeNode*)pParent)->GetChildNode(0) == pNode);
		break;

	case TNK_WHILE_STMT:
		bFlag = (((CExpressionTreeNode*)pParent)->GetChildNode(0) == pNode);
		break;

	case TNK_DO_STMT:
		bFlag = (((CExpressionTreeNode*)pParent)->GetChildNode(0) == pNode);
		break;

	case TNK_FOR_STMT:
		bFlag = (((CExpressionTreeNode*)pParent)->GetChildNode(1) == pNode);
		break;
	}
	
	if(bFlag && !pNode->IsCmpExpression() && pNode->GetNodeType() != TNK_TRUTH_OR_EXPR 
		&& pNode->GetNodeType() != TNK_TRUTH_AND_EXPR)
	{
		CConditionExprJmpRef* pConditionExprJmpRef = objContext.GetConditionExprJmpRef();
		ATLASSERT(pConditionExprJmpRef);

		JUM_INSTRUCTION_CODE nJumInstCode = JIC_UNKNOWN;

		JMP_TYPE nJmpType = pConditionExprJmpRef->GetJmpType();
		CDataTypeTreeNode* pDataType = pNode->GetDataType();

		if(pDataType->IsFloat())
		{
			FLDZ();                     //将+0.0压栈，即装入ST(0)
			FUCOMPP();
			FNSTSW(objContext);
			TEST_BIT8(RAX,0x44);
			nJumInstCode = (JT_TRUE == nJmpType)? JIC_JP:JIC_JNP;
		}
		else if(pDataType->GetNodeType() == TNK_LONGLONG_TYPE)
		{
			OR(RAX,RDX,objContext);
			nJumInstCode = (JT_TRUE == nJmpType)? JIC_JNE:JIC_JE;
		}
		else
		{
			if(pDataType->GetNodeType() == TNK_BOOLEAN_TYPE)
			{
				CMP_BIT8(RAX,0);
			}
			else
			{
				TEST(RAX,RAX);
			}
			nJumInstCode = (JT_TRUE == nJmpType)? JIC_JNE:JIC_JE;
		}

		JMP_BASE(nJumInstCode,TMP_ADDR_BYTE4); 
		pConditionExprJmpRef->AppendRefPos(nJmpType,GetOffset());
	}
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateIFSTMTCode
| 描  述 : 处理IF..ELSE语句
| 参  数 : CExpressionTreeNode* pNode——IF表达式节点
| 参  数 : CContext& objContext——上下文对象
| 备  注 : 已经测试
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateIFSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext)
{
	ATLASSERT(pExpressionNode->GetNodeType() == TNK_IF_STMT);

	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext);
	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);

	GenerateCompoundStatementsCode((CExpressionTreeNode*)(pExpressionNode->GetChildNode(1)),objContext);

	UINT nEndRefPos = 0;
	if(pExpressionNode->GetChildNode(2) != NULL)
	{
		JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128*/,objContext); 
		nEndRefPos = GetOffset();
	}
	
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	//else:
	if(pExpressionNode->GetChildNode(2) != NULL)
	{		
		GenerateCompoundStatementsCode((CExpressionTreeNode*)(pExpressionNode->GetChildNode(2)),objContext);
		//End:
		WriteDwordData(nEndRefPos - 4/*相对地址长度*/,(BYTE)(GetOffset() - nEndRefPos),objContext); 
	}

	objContext.SetConditionExprJmpRef(NULL);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateForSTMTCode
| 描  述 : 处理For循环语句,循环计数变量被统一分配,不再单多处理
| 参  数 : CExpressionTreeNode* pNode——IF表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateForSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel();
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	
	CSyntaxTreeNode* pThridPart = pExpressionNode->GetChildNode(2);
	if(pThridPart)
	{
		JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128*/,objContext);     //跳到:cmp
	}


	pSTMTBindingLevel->SetStartPos(GetOffset());          // 开始位置不包括变量定义和上面的跳转指令
	GenerateExprStatementsCode(pThridPart,objContext);    //计算第三部分表达式

	//cmp:
	if(pThridPart)
	{
		WriteDwordData(pSTMTBindingLevel->GetStartPos() - 4/*偏移地址的长度*/
			,GetOffset() - pSTMTBindingLevel->GetStartPos(),objContext); 
	}

	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	CSyntaxTreeNode* pConditon = pExpressionNode->GetChildNode(1);
	GenerateExprStatementsCode(pConditon,objContext); //计算条件表达式

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	GenerateCompoundStatementsCode(((CExpressionTreeNode*)pExpressionNode->GetChildNode(3)),objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext);

	//End:
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*偏移地址的长度*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();	
	objContext.SetConditionExprJmpRef(NULL);
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateWhileSTMTCode
| 描  述 : 处理while循环语句
| 参  数 : CExpressionTreeNode* pNode——IF表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateWhileSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel(GetOffset());
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	//start:
	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext); //计算条件表达式

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	GenerateCompoundStatementsCode(((CExpressionTreeNode*)pExpressionNode->GetChildNode(1)),objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext); //跳到"start"位置

	//End:
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*偏移地址的长度*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();
}



/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateDoSTMTCode
| 描  述 : 处理Do..while循环语句
| 参  数 : CExpressionTreeNode* pNode——do表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateDoSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel(GetOffset());
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	//start:
	GenerateCompoundStatementsCode(((CExpressionTreeNode*)pExpressionNode->GetChildNode(1)),objContext);

	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext); //计算条件表达式

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext);

	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	//End:
	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*偏移地址的长度*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateSwitchSTMTCode
| 描  述 : 处理Do..while循环语句
| 参  数 : CExpressionTreeNode* pNode——do表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateSwitchSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel(GetOffset());
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext);  //计算表达式

	CExpressionTreeNode* pCompoundSTMTNode = (CExpressionTreeNode*)pExpressionNode->GetChildNode(1);
	ATLASSERT(pCompoundSTMTNode->GetNodeType() == TNK_COMPOUND_STMT);
	CExpressionTreeNode* pChildNode = (CExpressionTreeNode*)pCompoundSTMTNode->GetChildNode(0);

	BOOL bDefaultLable = FALSE;
	CConstIntTreeNode* pConstIntTreeNode = NULL;
	while(pChildNode)
	{
		pConstIntTreeNode = (CConstIntTreeNode*)pChildNode->GetChildNode(0);
		switch(pChildNode->GetNodeType())
		{
		case TNK_CASE_LABEL:
			CMP_IMM32(RAX,(int)pConstIntTreeNode->GetValue1());
			JE(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
			pSTMTBindingLevel->AppendCaseRef(GetOffset());
			break;    

		case TNK_DEFAULT_LABEL:   
			if(pSTMTBindingLevel->GetCaseRef() > 0) //如果default在第一个位置将不用跳转
			{
				JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
				pSTMTBindingLevel->AppendCaseRef(GetOffset());        
			}
			pSTMTBindingLevel->SetDefaultIndex(pSTMTBindingLevel->GetCaseRef());

			bDefaultLable = TRUE;
			break;
		}

		pChildNode = (CExpressionTreeNode*)pChildNode->GetChain();
	}

	if(!bDefaultLable)  //如果没有"default"需要增加一个到末尾的跳转指令
	{
		JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
		pSTMTBindingLevel->AppendRefrence(GetOffset());
	}

	GenerateCompoundStatementsCode(pCompoundSTMTNode,objContext);

	//End:
	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*偏移地址的长度*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}


	objContext.PopSTMTBindingLevel();
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateCompoundStatementsCode
| 描  述 : 处理由大括符括起来的代码块
| 参  数 : CExpressionTreeNode* pNode——表达式节点
| 参  数 : CContext& objContext——上下文对象
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateCompoundStatementsCode(CExpressionTreeNode* pNode,CContext& objContext)
{
	ATLASSERT(pNode && pNode->GetNodeType() == TNK_COMPOUND_STMT);

	CExpressionTreeNode* pChildNode = (CExpressionTreeNode*)pNode->GetChildNode(0);
	while(pChildNode)
	{
		switch(pChildNode->GetNodeType())
		{
		case TNK_LABEL_STMT:
			GenerateLableSTMTCode((CDeclarationTreeNode*)pChildNode->GetChildNode(0),objContext);
			break;

		case TNK_GOTO_STMT:
			GenerateGotoSTMTCode((CDeclarationTreeNode*)pChildNode->GetChildNode(0),objContext);
			break;

		case TNK_EXPR_STMT:
			GenerateExprStatementsCode(pChildNode->GetChildNode(0),objContext);
			break;

		case TNK_IF_STMT:
			GenerateIFSTMTCode(pChildNode,objContext);
			break;

		case TNK_CONTINUE_STMT:
			{
				CSTMTBindingLevel* pCurSTMTBindingLevel = objContext.GetCurSTMTBindingLevel();
				ATLASSERT(pCurSTMTBindingLevel);
				JMP_IMM32(pCurSTMTBindingLevel->GetStartPos() - GetOffset(),objContext); //直接跳到语句的开始位置
			}
			break;

		case TNK_BREAK_STMT:
			{
				CSTMTBindingLevel* pCurSTMTBindingLevel = objContext.GetCurSTMTBindingLevel();
				ATLASSERT(pCurSTMTBindingLevel);
				if(pCurSTMTBindingLevel->GetNodeType() == TNK_SWITCH_STMT)
				{
					const UINT nIncCaseIndex = pCurSTMTBindingLevel->GetCaseIndex();
					if(pCurSTMTBindingLevel->GetDefaultIndex() > 0 ||
						nIncCaseIndex != pCurSTMTBindingLevel->GetCaseRef())
					{
						JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
					}
				}
				else
				{
					JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
				}

				pCurSTMTBindingLevel->AppendRefrence(GetOffset());
			}
			break;

		case TNK_FOR_STMT:
			GenerateForSTMTCode(pChildNode,objContext);
			break;

		case TNK_WHILE_STMT:
			GenerateWhileSTMTCode(pChildNode,objContext);
			break;

		case TNK_DO_STMT:
			GenerateDoSTMTCode(pChildNode,objContext);
			break;

		case TNK_SWITCH_STMT:
			GenerateSwitchSTMTCode(pChildNode,objContext);
			break;

		case TNK_CASE_LABEL:
			{
				CSTMTBindingLevel* pCurSTMTBindingLevel = objContext.GetCurSTMTBindingLevel();
				ATLASSERT(pCurSTMTBindingLevel);
				const UINT nIncCaseIndex = pCurSTMTBindingLevel->GetCaseIndex();

				WriteDwordData(pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex) - 4/*偏移地址的长度*/
					,GetOffset() - pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex),objContext); //把实际地址填写到跳转表中

				pCurSTMTBindingLevel->IncCaseIndex();
			}
			break;

		case TNK_DEFAULT_LABEL:
			{
				CSTMTBindingLevel* pCurSTMTBindingLevel = objContext.GetCurSTMTBindingLevel();
				ATLASSERT(pCurSTMTBindingLevel);
				const UINT nIncCaseIndex = pCurSTMTBindingLevel->GetCaseIndex();
				if(nIncCaseIndex != 0)  //说明"defauilt"没有在最前面
				{
					WriteDwordData(pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex) - 4/*偏移地址的长度*/
						,GetOffset() - pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex),objContext);

					pCurSTMTBindingLevel->IncCaseIndex();
				}
			}
			break;

		case TNK_RETURN_STMT:
			{
				CSyntaxTreeNode* pResultExprSTMT = pChildNode->GetChildNode(0);
				if(pResultExprSTMT != NULL)
				{
					GenerateExprStatementsCode(pResultExprSTMT,objContext);

					CDataTypeTreeNode* pDataType1 = pChildNode->GetDataType();
					CDataTypeTreeNode* pDataType2 = pResultExprSTMT->GetDataType();
					if(pDataType1->IsInterger())
					{
						if(pDataType2->IsFloat())
						{
							if(pDataType1->GetNodeType() == TNK_LONGLONG_TYPE)
							{
								XOR(RDX,RDX,objContext);
							}

							FISTP(-(objContext.GetOffset() + 8));
							MOV_REG_LIV32(RAX,-(objContext.GetOffset() + 8),objContext);
						}
					}
					else if(pDataType1->IsFloat())
					{
						if(pDataType2->GetNodeType() == TNK_LONGLONG_TYPE)
						{
							MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //把结果移动到栈中
							MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //把结果移动到栈中
							FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //装载到浮点数寄存器中
						}
						else if(pDataType2->IsInterger())
						{
							MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //把结果移动到栈中
							FILD(-(objContext.GetOffset() + 4));               //装载到浮点数寄存器中      
						}

					}
				}

				if(!(pChildNode->GetParent()->GetNodeType() == TNK_FUNCTION_DECL
					&& NULL == pChildNode->GetChain()))  //函数中最后一个"return"语句不需要产生跳转指令
				{
					CRefrenceInfo* pRetRefrenceInfo = objContext.GetRetRefrenceInfo(); 
					JMP_IMM32(TMP_ADDR_BYTE4/*该地址将被替换,它唯一要求是大于128,确保地址是四个字节*/,objContext);
					pRetRefrenceInfo->AppendRefrence(GetOffset());
				}
			}
			break;

		case TNK_COMPOUND_STMT:
			GenerateCompoundStatementsCode(pChildNode,objContext);
			break;
		}
		pChildNode = (CExpressionTreeNode*)pChildNode->GetChain();
	}
}


/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateFunctionCode
| 描  述 : 为函数创建代码
| 参  数 : CSyntaxTreeNode* pNode——
| 修改记录 : 2007-5-2 8:41:34  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateFunctionCode( CDeclarationTreeNode* pNode )
{
	ATLASSERT(pNode);

	pNode->SetOffset(GetOffset());

	m_pPrvPosWrited = m_pCurPosWrited;
	GenerateMasmCmd(";Function: %s\r\n",pNode->GetSymbol());

	const static UINT SP_BASE_OFFSET = 12;  //把三个寄存器(RBX,RSI,TDI)的值推进了栈,所以是12
	CContext objContext(SP_BASE_OFFSET);  
	PUSH(RBP);     
	MOV(RBP,RSP,objContext);
	PUSH(RBX);
	PUSH(RSI);
	PUSH(RDI);

	
	CSyntaxTreeNode* pFunctonBody = pNode->GetSavedTree(); 
	const int nLocalVarSize = SumLocalNonStaticVariable((CExpressionTreeNode*)pFunctonBody,objContext);  //统计整个函数分配的局部非静态变量
	if( nLocalVarSize > 0)
	{
		SUB_IMM32(RSP,nLocalVarSize,objContext);  //移动栈指针,为局部变量分配空间
	}

	GenerateCompoundStatementsCode((CExpressionTreeNode*)pFunctonBody,objContext);

	CRefrenceInfo* pRetRefrenceInfo = objContext.GetRetRefrenceInfo(); 
	for(UINT i = 0;i < pRetRefrenceInfo->GetRefrences();i++)
	{
		WriteDwordData(pRetRefrenceInfo->GetRefenceOffset(i) - 4/*偏移地址的长度*/
			,(GetOffset() - pRetRefrenceInfo->GetRefenceOffset(i)),objContext); 
	}

	if( nLocalVarSize > 0)
	{
		objContext.DecOffset(nLocalVarSize);
		ADD_IMM32(RSP,nLocalVarSize,objContext);  //移动栈指针,释放整个函数分配的局部变量空间
	}

	POP(RDI,objContext);
	POP(RSI,objContext);
	POP(RBX,objContext);
	MOV(RSP,RBP,objContext);
	POP(RBP,objContext);
	RET();
}

/*-----------------------------------------------------------------
| 函数名称 : CAsmCodeGenerator::GenerateCodeSegment
| 描  述 : generate code segment
| 返 回 值 : 返回代码段的尺寸
| 修改记录 : 2007-4-27 10:12:57  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateCodeSegment(AX3_PROGRAM_HEADER* pProgramHeader,char* lpEntryFun)
{   
	UINT nResult = GetOffset(); 

	CSyntaxTreeNode* pSyntaxTree = m_pSyntaxTreeRoot;
	while(NULL != pSyntaxTree)
	{
		if(TNK_FUNCTION_DECL == pSyntaxTree->GetNodeType())  //函数定义
		{
			GenerateFunctionCode( (CDeclarationTreeNode*)pSyntaxTree );

			CIdentifierTreeNode* pID = ((CDeclarationTreeNode*)pSyntaxTree)->GetName();
			if((NULL == lpEntryFun && pProgramHeader->nEntryPoint == 0) || pID->GetTitle() == lpEntryFun)
			{
				pProgramHeader->nEntryPoint = pSyntaxTree->GetOffset();
			}
		}
		pSyntaxTree = pSyntaxTree->GetChain();
	}   

	return GetOffset() - nResult;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateStaticVariable
| 描  述 : 为全局变量的设置初始值
| 参  数 : CDeclarationTreeNode* pDeclNode——被处理的变量
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateStaticVariableInit(BYTE* pSpace
	,UINT nDataType,CSyntaxTreeNode* pInitial)
{
	switch(nDataType)
	{
	case TNK_BOOLEAN_TYPE:
		*pSpace = ((pInitial->GetValue1() + pInitial->GetValue2()) != 0)? 1 : 0;
		break;

	case TNK_BYTE_TYPE:
	case TNK_CHAR_TYPE:  
		*pSpace = BYTE(pInitial->GetValue1()? pInitial->GetValue1():pInitial->GetValue2());
		break;

	case TNK_WORD_TYPE:
	case TNK_SHORT_TYPE:
		*(WORD*)pSpace = WORD(pInitial->GetValue1()? pInitial->GetValue1():pInitial->GetValue2());
		break;

	case TNK_DWORD_TYPE: 
	case TNK_LONG_TYPE:                
	case TNK_VOID_TYPE:    
	case TNK_INT_TYPE: 
		*(DWORD*)pSpace = DWORD(pInitial->GetValue1()? pInitial->GetValue1():pInitial->GetValue2());
		break;

	case TNK_FLOAT_TYPE:
		*(float*)pSpace = float(pInitial->GetValue2()? pInitial->GetValue2():pInitial->GetValue1());
		break;

	case TNK_DOUBLE_TYPE: 
		*(double*)pSpace = double(pInitial->GetValue2()? pInitial->GetValue2():pInitial->GetValue1());
		break;

	case TNK_LONGLONG_TYPE:
		*(LONGLONG*)pSpace = LONGLONG(pInitial->GetValue2()? pInitial->GetValue2():pInitial->GetValue1());
		break;
	}

}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateStaticVariable
| 描  述 : 初始化全局变量
| 参  数 : CDeclarationTreeNode* pDeclNode——被处理的变量
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateStaticVariable(CDeclarationTreeNode* pDeclNode)
{
	ATLASSERT(pDeclNode);
	if(NULL == pDeclNode)
	{
		return 0;
	}
	if(pDeclNode->GetReferences() == NULL)
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, pDeclNode->GetLineIndex(),0,
			_T("'%s (%d)': unreferenced static variable"),CString(pDeclNode->GetSymbol()),pDeclNode->GetUID());

		return 0;
	}

	CDataTypeTreeNode* pDataTypeNode = pDeclNode->GetDataType();
	ATLASSERT(NULL != pDataTypeNode);
	if(NULL == pDataTypeNode)
	{
		return 0;
	}
	ATLTRACE(_T("'%s (%d)': allocated static variable (offset:%xh,size:%d)\r\n")
		,CString(pDeclNode->GetSymbol()),pDeclNode->GetUID(),GetOffset(),pDataTypeNode->GetSize());

	pDeclNode->SetOffset(GetOffset());
	BYTE* pSpace = RequestSpace(pDataTypeNode->GetSize());

	CSyntaxTreeNode* pInitial = pDeclNode->GetInitial();
	if(NULL != pInitial)     //说明变量要被初始化
	{
		switch(pDataTypeNode->GetNodeType())
		{
		case TNK_ARRAY_TYPE:  //
			{
				CDataTypeTreeNode* pBaseDataTypeTreeNode = pDataTypeNode->GetDataType();
				CSyntaxTreeNode* pValue = ((CVectorTreeNode*)pInitial)->GetValue();
				while(NULL != pValue)
				{
					GenerateStaticVariableInit(pSpace,pBaseDataTypeTreeNode->GetNodeType(),pValue);

					pSpace += pBaseDataTypeTreeNode->GetSize();
					pValue = pValue->GetChain();
				}
			}
			break;

		default:
			if(pInitial->IsNumericConstants())
			{
				GenerateStaticVariableInit(pSpace,pDataTypeNode->GetNodeType(),pInitial);
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(FALSE, pDeclNode->GetLineIndex(),0,
					_T("'%s (%d)': expect a constant "),CString(pDeclNode->GetSymbol()),pDeclNode->GetUID());
			}
		}
	}
	else         //否则全部设置为0
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, pDeclNode->GetLineIndex(),0,
			_T("'%s (%d)': uninitialized static variable used"),CString(pDeclNode->GetSymbol()),pDeclNode->GetUID());

		memset(pSpace,0,pDataTypeNode->GetSize());
	}

	return pDataTypeNode->GetSize();
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateTxtConstants
| 描  述 : 处理程序中的文字常量,包括字符串常量,数字常量
| 参  数 : CExpressionTreeNode* pNode——被处理的节点
| 返 回 值 : 返回所有静态变量的尺寸大小
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateTxtConstants(CTxtConstantsTreeNode* pNode)
{
	UINT nLocalVarSize = 0;
	ATLASSERT(pNode && pNode->GetNodeType() == TNK_TXT_CONSTANTS_TABLE);

	CConstTreeNode* pConstTreeNode = NULL;
	for(UINT i = 0;i < pNode->GetConstantsCount();i++)
	{
		pConstTreeNode = pNode->GetConstantNode(i);
		if(pConstTreeNode->GetOffset() > 0)
		{
			continue;
		}

		pConstTreeNode->SetOffset(GetOffset());   

		BYTE* pSpace = RequestSpace(pConstTreeNode->GetSize());
		switch(pConstTreeNode->GetNodeType())
		{
		case TNK_INTEGER_CST:
			*(double*)pSpace = (double)pConstTreeNode->GetValue1();  //把整形也转换为浮点数格式,因为这是为浮点数运算准备的
			break;

		case TNK_REAL_CST:
			*(double*)pSpace = pConstTreeNode->GetValue2();
			break;

		case TNK_STRING_CST:
			memcpy(pSpace,(char*)(LPCSTR)pConstTreeNode->GetValue3(),pConstTreeNode->GetSize());
			break;
		}   

		nLocalVarSize += pConstTreeNode->GetSize();


		CConstTreeNode* pConstTreeNode1 = NULL;          //下面的代码让相同的常量有相同偏移,避免重复存储
		BOOL bEqual = FALSE;
		for(UINT j = i + 1;j < pNode->GetConstantsCount();j++)
		{
			pConstTreeNode1 = pNode->GetConstantNode(j);
			if(pConstTreeNode1->GetNodeType() == pConstTreeNode->GetNodeType())
			{
				switch(pConstTreeNode->GetNodeType())
				{
				case TNK_INTEGER_CST:
					bEqual = (pConstTreeNode1->GetValue1() == pConstTreeNode->GetValue1());
					break;

				case TNK_REAL_CST:
					bEqual = (pConstTreeNode1->GetValue2() == pConstTreeNode->GetValue2());
					break;

				case TNK_STRING_CST:
					bEqual = (pConstTreeNode1->GetValue3() == pConstTreeNode->GetValue3());
					break;
				}   

				if(bEqual)
				{
					pConstTreeNode1->SetOffset(pConstTreeNode->GetOffset());
				}
			}
		}
	}

	//#ifdef _DEBUG
	//   for(UINT i = 0;i < pNode->GetConstantsCount();i++)
	//   {
	//    pConstTreeNode = pNode->GetConstantNode(i);
	//    ATLTRACE(_T("%f: %d\r\n"),pConstTreeNode->GetValue1() + pConstTreeNode->GetValue2()
	//       ,pConstTreeNode->GetOffset());
	//   }
	//#endif
	return nLocalVarSize;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateLocalStaticVariable
| 描  述 : 处理函数中的静态变量
| 参  数 : CExpressionTreeNode* pNode——被处理的节点,应该是TNK_COMPOUND_STMT
| 返 回 值 : 返回所有静态变量的尺寸大小
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateLocalStaticVariable(CExpressionTreeNode* pNode)
{
	UINT nLocalVarSize = 0;
	ATLASSERT(pNode && pNode->GetNodeType() == TNK_COMPOUND_STMT);
	CExpressionTreeNode* pChildNode = (CExpressionTreeNode*)pNode->GetChildNode(0);
	while(pChildNode)
	{
		for(UINT i = 0; i < 4;i++)
		{
			CExpressionTreeNode* pOpExprNode = (CExpressionTreeNode*)pChildNode->GetChildNode(i);
			if(NULL == pOpExprNode)
			{
				break;
			}

			switch(pOpExprNode->GetNodeType())
			{
			case TNK_DECL_STMT:
				{
					CDeclarationTreeNode* pDeclTreeNode = (CDeclarationTreeNode*)pOpExprNode->GetChildNode(0);
					while(NULL != pDeclTreeNode)
					{
						if(pDeclTreeNode->GetNodeType() == TNK_VAR_DECL)       //常量定义是不需要分配空间
						{
							if(pDeclTreeNode->GetDeclAttribute() & NDA_STATIC_FLAG) //只分析静态变量
							{
								nLocalVarSize += GenerateStaticVariable(pDeclTreeNode);
							}
						}

						pDeclTreeNode = (CDeclarationTreeNode*)pDeclTreeNode->GetChain();
					}
				}
				break;			

			case TNK_COMPOUND_STMT:
				nLocalVarSize += GenerateLocalStaticVariable(pOpExprNode);
				break;
			}
		}

		pChildNode = (CExpressionTreeNode*)pChildNode->GetChain();
	}

	return nLocalVarSize;
}
/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::GenerateStaticData
| 描  述 : 处理全局变量和函数中的静态变量
| 返 回 值 : 返回所有全局变量的尺寸大小
| 修改记录 : 2007-4-27 16:16:50  -huangdy-  创建
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateStaticData()
{
	UINT nResult = 0;
	CDeclarationTreeNode* pDeclNode = NULL;
	CSyntaxTreeNode* pSyntaxTree = m_pSyntaxTreeRoot;
	while(NULL != pSyntaxTree)
	{
		switch(pSyntaxTree->GetNodeType())
		{
		case TNK_FUNCTION_DECL:   //函数定义
			nResult += GenerateLocalStaticVariable((CExpressionTreeNode*)((CDeclarationTreeNode*)pSyntaxTree)->GetSavedTree());
			break;

		case TNK_DECL_STMT:    //变量定义
			pDeclNode = (CDeclarationTreeNode*)(((CExpressionTreeNode*)pSyntaxTree)->GetChildNode(0));
			while(NULL != pDeclNode)
			{
				nResult += GenerateStaticVariable(pDeclNode);   

				pDeclNode = (CDeclarationTreeNode*)pDeclNode->GetChain();
			}
			break;

		case TNK_TXT_CONSTANTS_TABLE:
			nResult += GenerateTxtConstants((CTxtConstantsTreeNode*)pSyntaxTree);
			break;
		}

		pSyntaxTree = pSyntaxTree->GetChain();
	}

	return nResult;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SumNonStaticVarSize
| 描  述   : generate local variables 
| 参  数   : CDeclarationTreeNode* pNode——申明节点
| 返 回 值 : 当前语句申明局的部变量的尺寸(不包括静态变量) 
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/
int CGenerateSaxProgram::SumNonStaticVarSize( CDeclarationTreeNode* pNode,CContext& objContext)
{
	int nSize = 0; 
	ATLASSERT(pNode);

	CDataTypeTreeNode* pDataTypeNode = NULL;
	while(pNode)
	{
		if(pNode->GetNodeType() == TNK_VAR_DECL)       //常量定义是不需要分配空间
		{
			if(pNode->GetReferences() != NULL)
			{
				if(pNode->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //静态变量不需要分配在栈
				{
					pDataTypeNode = pNode->GetDataType();
					pNode->SetOffset(0-(objContext.GetOffset() + pDataTypeNode->GetSize()));  //局部变量在栈中的偏移,是个负数,变量的值 = [BP + offset]
					objContext.IncOffset(pDataTypeNode->GetSize());
					nSize += pDataTypeNode->GetSize();

					ATLTRACE(_T("'%s(%d)': allocated local variable (offset:-%xh,size:%d)\r\n")
						,CString(pNode->GetSymbol()),pNode->GetUID(),abs(pNode->GetOffset()),pDataTypeNode->GetSize());

					if(NULL != pNode->GetInitial())       //说明变量要被初始化
					{
						GenerateSimpleAssignmentOp(pNode,pNode->GetInitial(),objContext);						
					}
					else
					{ 
						m_pSaxCCompileimpl->OutputErrMsg(FALSE, pNode->GetLineIndex(),0,
							_T("'%s (%d)': uninitialized local variable used"),CString(pNode->GetSymbol()),pNode->GetUID());
					}
				}
			}
			else
			{
				m_pSaxCCompileimpl->OutputErrMsg(FALSE, pNode->GetLineIndex(),0,
					_T("'%s (%d)': unreferenced local variable"),CString(pNode->GetSymbol()),pNode->GetUID());
			}
		}
		pNode = (CDeclarationTreeNode*)pNode->GetChain();
	}

	return nSize;
}

/*-----------------------------------------------------------------
| 函数名称 : CGenerateSaxProgram::SumLocalNonStaticVariable
| 描  述   : Sum size of function local variables
| 参  数   : CDeclarationTreeNode* pNode——申明节点
| 返 回 值 : 当前函数申明局的部变量的尺寸
| 修改记录 : 2014-5-20 20:58:54  -huangdy-  创建
-----------------------------------------------------------------*/

UINT CGenerateSaxProgram::SumLocalNonStaticVariable(CExpressionTreeNode* pNode,CContext& objContext)
{
	UINT nLocalVarSize = 0;
	ATLASSERT(pNode && pNode->GetNodeType() == TNK_COMPOUND_STMT);
	CExpressionTreeNode* pChildNode = (CExpressionTreeNode*)pNode->GetChildNode(0);
	while(pChildNode)
	{
		for(UINT i = 0; i < 4;i++)
		{
			CExpressionTreeNode* pOpExprNode = (CExpressionTreeNode*)pChildNode->GetChildNode(i);
			if(NULL == pOpExprNode)
			{
				break;
			}

			switch(pOpExprNode->GetNodeType())
			{
			case TNK_DECL_STMT:
				{
					CDeclarationTreeNode* pDeclTreeNode = (CDeclarationTreeNode*)pOpExprNode->GetChildNode(0);
					while(NULL != pDeclTreeNode)
					{
						if(pDeclTreeNode->GetNodeType() == TNK_VAR_DECL)       //常量定义是不需要分配空间
						{
							if(pDeclTreeNode->GetDeclAttribute() ^ NDA_STATIC_FLAG) //只分析静态变量
							{
								nLocalVarSize += SumNonStaticVarSize(pDeclTreeNode,objContext);
							}
						}

						pDeclTreeNode = (CDeclarationTreeNode*)pDeclTreeNode->GetChain();
					}
				}
				break;			

			case TNK_COMPOUND_STMT:
				nLocalVarSize += SumLocalNonStaticVariable(pOpExprNode,objContext);
				break;
			}
		}

		pChildNode = (CExpressionTreeNode*)pChildNode->GetChain();
	}

	return nLocalVarSize;
}

//;        -----------------
//;        | HEADER    |
//;        |---------------|
//;        |全局变量,函数 |
//;        |中的静态变量和 |
//;        |文本常量    |
//;        |---------------|
//;        | 代码     |
//;        |---------------|
//;        | 重新定位数据 |
//;        |---------------|
//;        |导出函数表   |
//;        |---------------|

BYTE* CGenerateSaxProgram::GenerateProgram(CSyntaxTreeNode* pSyntaxTreeRoot
	,char* lpEntryFun
	,int /*nStackSize*/
	,AX3_COMPILE_MODE nMode)
{
	m_pSyntaxTreeRoot = pSyntaxTreeRoot;
	SetMode(nMode);
	m_strMasmCode.Empty();

	m_arrReferences.clear();

	if(AllocatBinaryCodeBuffer(DEFAULT_BINARY_CODE_BUFFER_LEN))
	{
		AX3_PROGRAM_HEADER* pProgramHeader = (AX3_PROGRAM_HEADER*)RequestSpace(sizeof(AX3_PROGRAM_HEADER));
		memset(pProgramHeader,0,sizeof(AX3_PROGRAM_HEADER));

		pProgramHeader->nMagic  = MAGIC_NUMBER;
		pProgramHeader->nMachine = WORD(nMode);
		pProgramHeader->nGlobleDataSize = GenerateStaticData();  //写入静态数据
		pProgramHeader->nCodeSize    = GenerateCodeSegment(pProgramHeader,lpEntryFun);  //写入代码

	/*	CAtlFile objOutput;
		HRESULT hr = objOutput.Create(_T("I:\\output.txt"),GENERIC_WRITE,FILE_SHARE_READ,CREATE_ALWAYS);
		if(SUCCEEDED(hr))
		{
			CStringA strTmp;
							
			strTmp.Format("GlobleDataSize: %u, CodeSize = %u"
				,pProgramHeader->nGlobleDataSize,pProgramHeader->nCodeSize);
			objOutput.Write((LPCSTR)strTmp,strTmp.GetLength());
			objOutput.Close();
		}*/

		pProgramHeader->nRePostionEntries = m_arrReferences.size();     //写入重定位 项目
		if(pProgramHeader->nRePostionEntries > 0)
		{
			DWORD* pBuffer = (DWORD*)RequestSpace(m_arrReferences.size() * 4);
			ATLASSERT(pBuffer);
			for(UINT i = 0;i < m_arrReferences.size();i++)
			{
				pBuffer[i] = m_arrReferences[i]; 
			}
		}

		CSyntaxTreeNode* pSyntaxTree = m_pSyntaxTreeRoot;
		while(NULL != pSyntaxTree)
		{
			if(TNK_FUNCTION_DECL == pSyntaxTree->GetNodeType())  //函数定义
			{
				CIdentifierTreeNode* pID = ((CDeclarationTreeNode*)pSyntaxTree)->GetName();
				CStringA strName = pID->GetTitle();

				BYTE* pBuffer = RequestSpace(strName.GetLength() + 1 + 4);
				memcpy(pBuffer,strName,strName.GetLength());
				pBuffer[strName.GetLength()] = 0;

				*(DWORD*)&(pBuffer[strName.GetLength() + 1]) = pSyntaxTree->GetOffset();   

				pProgramHeader->nFunctions++;
				pProgramHeader->nFunctionsTableSize += (WORD)(strName.GetLength() + 1 + 4);
			}
			pSyntaxTree = pSyntaxTree->GetChain();
		}

		pProgramHeader->nSize = sizeof(AX3_PROGRAM_HEADER) + pProgramHeader->nGlobleDataSize
			+ pProgramHeader->nCodeSize + pProgramHeader->nFunctionsTableSize + pProgramHeader->nRePostionEntries * 4;
	}

	return m_pCodeBuffer;
}


