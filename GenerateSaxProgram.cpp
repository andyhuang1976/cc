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
| �������� : CGenerateSaxProgram::CalculateComplement
| ��  �� : ����ָ����ֵ�Ĳ���
| ��  �� : int nRelativeDisplacement->�����RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
*  ��������  : CConditionExprJmpRef::WriteRealAddr
*  ��    ��  : ������ʵ�ĵ�ַ
*  ��    ��  : JMP_TYPE nJmpType������Ҫ�޸ĵ�����
*              UINT nRealAddr������ʵ�ĵ�ַ
*             CGenerateSaxProgram* pSaxProgram����
*             CContext* pContext��������������
*             UINT nStartPos����ʼλ��
*  �� �� ֵ  : 
*  �޸ļ�¼  : 2005-02-28 10:56:22   -huangdy-   ����
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
			pSaxProgram->WriteDwordData((*itr) - 4/*ƫ�Ƶ�ַ�ĳ���*/,nRealAddr - (*itr),*pContext);
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
| �������� : CContext::RequestReg
| ��  �� : ����ʹ��ָ���ļĴ���������Ĵ������з��ؼĴ�����ţ�����
|    ����Ƿ��п��е��з��أ����򷵻أ�
| ��  �� : COMMON_REGISTERS_MASK nMask->�Ĵ�������
| ��  �� : BOOL bOther->ʹ�������Ŀ��мĴ���
| �� �� ֵ : �Ĵ������
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CContext::ReserveReg
| ��  �� : ����һ���Ĵ���,��ʾ�üĴ治����
| ��  �� : COMMON_REGISTERS nReg->��ǩ���ƼĴ������
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
-----------------------------------------------------------------*/
void CContext::ReserveReg(COMMON_REGISTERS nReg)
{
	m_nRegisterUsedMask |= Reg2Mask(nReg);
}
/*-----------------------------------------------------------------
| �������� : CContext::FreeReg
| ��  �� : �ͷ�һ���Ĵ���
| ��  �� : COMMON_REGISTERS nReg->��ǩ���ƼĴ������
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
-----------------------------------------------------------------*/
void CContext::FreeReg(COMMON_REGISTERS nReg)
{
	m_nRegisterUsedMask ^= Reg2Mask(nReg);	
}

/*-----------------------------------------------------------------
| �������� : CContext::OnRegChanged
| ��  �� : �ı�Ĵ���ֵ�¼�,������ֵĬ���Ǵ��RDX:RAX��,���õ�ǰ����
|          ������Ҫ�Ǳ����ظ�װ��
| ��  �� : COMMON_REGISTERS nReg->ֵ���ı�ļĴ���
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CContext::CreateLableRefrence
| ��  �� : ����һ����ǩ���òο���¼
| ��  �� : LPCSTR lpName->��ǩ����
| �� �� ֵ : �������Ķ���
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CContext::LookupLableRefrence
| ��  �� : ��ѯһ����ǩ���òο���¼
| ��  �� : LPCSTR lpName->��ǩ����
| �� �� ֵ : ��ǩ����ָ�����
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CContext::CreateSTMTBindingLevel
| ��  �� : Ϊ��ǰ����䴴��һ��״̬����
| ��  �� : UINT nStartPos->��ǰ���Ŀ�ʼ����
| �� �� ֵ : �������Ķ���
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CContext::PopSTMTBindingLevel
| ��  �� : ɾ��m_arrSTMTBindingLevel�������Ľڵ�
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
			m_pCurSTMTBindingLevel = *(m_arrSTMTBindingLevel.rbegin()); //�������һ���ڵ�Ϊ��ǰ�ڵ�
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
| �������� : CAssemble::AllocatBinaryCodeBuffer
| ��  �� : 
| ��  �� : size_t pLength����
| �� �� ֵ : TRUE----
|       FALSE---
| �޸ļ�¼ : 2007-4-28 13:46:15  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::RequestSpace
| ��  �� : ����ȫ�ֱ���
| Parameter : ��Ҫ����ռ�Ĵ�С
| �� �� ֵ : ���ر�����ռ��ͷָ��
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateMasmCmd
| ��  �� : �������ָ��
| ��  �� : BYTE nCode->ָ�����
| �޸ļ�¼ : 2014-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::WriteDwordData
| ��  �� : �����ӳ���ļ��е�ָ��λ��ֱ��д������
| ��  �� : UINT nOffset->д������ݵ�λ��ƫ��
| ��  �� : DWORD nData->��Ҫд�������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::WriteByteData
| ��  �� : �����ӳ���ļ��е�ָ��λ��ֱ��д������
| ��  �� : UINT nOffset->д������ݵ�λ��ƫ��
| ��  �� : BYTE nData->��Ҫд�������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ���䵥�ֽ�ָ�����
| ��  �� : BYTE nCode->ָ�����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode)
{
	BYTE* pBuffer = RequestSpace(1);
	ATLASSERT(pBuffer);

	*pBuffer = nOpCode;
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ���䵥�ֽ�ָ�����+�ĸ��ֽڵĲ���
| ��  �� : BYTE nOpCode->ָ�����
| ��  �� : DWORD nPara->ָ�����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode,BYTE nPara)
{
	BYTE* pBuffer = RequestSpace(2);
	ATLASSERT(pBuffer);

	pBuffer[0]      = nOpCode;
	pBuffer[1]      = nPara;
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ���䵥�ֽ�ָ�����+�ĸ��ֽڵĲ���
| ��  �� : BYTE nOpCode->ָ�����
| ��  �� : DWORD nPara->ָ�����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::EmitInstruction(BYTE nOpCode,DWORD nPara)
{
	BYTE* pBuffer = RequestSpace(5);
	ATLASSERT(pBuffer);

	pBuffer[0]      = nOpCode;
	*(DWORD*)&pBuffer[1] = nPara;
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ����˫�ֽ�ָ�����+�ĸ��ֽڵĲ���
| ��  �� : BYTE nOpCode->ָ�����1
| ��  �� : BYTE nCode2->ָ�����2
| ��  �� : DWORD nPara->ָ�����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ����˫�ֽ�ָ�����+�ĸ��ֽڵĲ���
| ��  �� : BYTE nOpCode->ָ�����1
| ��  �� : BYTE nCode2->ָ�����2
| ��  �� : DWORD nPara->ָ�����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::EmitInstruction
| ��  �� : ����ָ��:ǰ׺+����+Ѱַ��ʽ(ModRM)+nRD+8λ������
| ��  �� : BYTE nPreFix->ָ��ǰ׺
| ��  �� : BYTE nOpCode->ָ�����
| ��  �� : BYTE nModRM->Ѱַ��ʽ
| ��  �� : DWORD nDisplacement->��MOD = 00B��ʾû�л�,�Ҵ���һ��32λ
|       ��ƫ�����������ʾdisp8��disp32 + [EBP].���ṩ���µ�Ѱַ��ʽ:
| ��  �� : BYTE nIMM32->������
| ��  �� : int nIMMBytes->��������λ��
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::EmitInstruction(BYTE nPreFix,BYTE nOpCode,BYTE nModRM
	,BYTE nSIB,int nDisplacement,int nIMM32,int nIMMBytes/* = 4*/,BOOL bStatic)
{
	UINT nInstLen = 0;
	if(PREFIX_INVALID != nPreFix)
	{
		nInstLen++;               // nPreFix����
	}
	nInstLen += 2;               // nOpCode + nModRM ����

	const MOD MODE_TYPE = MOD(nModRM >> 6);
	if(MODE_TYPE != MODE11 && (nModRM & 7) == SIB_ADDR)
	{
		nInstLen ++;              // SIB����
	}

	switch(MODE_TYPE)   // ƫ����
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

	nInstLen += nIMMBytes;           // nIMM32����

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
| �������� : CGenerateSaxProgram::PUSH
| ��  �� : ��ָ���ļĴ����ƽ�ջ
| ��  �� : REGISTERS nRegister�����Ĵ������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::PUSH
| ��  �� : ��ָ���ĶμĴ����ƽ�ջ,������Ϊʲֻ��FS,GS��Ҫ"0FH"ǰ׺
| ��  �� : REGISTERS nRegister�����Ĵ������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::PUSH_IMM
| ��  �� : ��һ���������ƽ�ջ
| ��  �� : DWORD nValue����������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::PUSH_IMM
| ��  �� : ��һ��������ֵ�ƽ�ջ
| ��  �� : DWORD nAddr����������ַ
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::PUSH_M32(DWORD nAddr)
{
	EmitInstruction(PREFIX_INVALID,0xFF,ModRM(MODE00,6,RBP),NONE_SIB,nAddr,0,0,FALSE); //FF,000+110(ָ�����)+101(��ʾ�������ڴ��ַ)
	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [(%Xh)] \r\n","push",nAddr);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::POP
| ��  �� : ��ջ����ȡ���ĸ��ֽڵ�ֵ������ָ����ͨ�üĴ�����
| ��  �� : REGISTERS nRegister�����Ĵ������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::POP
| ��  �� : ��ջ����ȡ���ĸ��ֽڵ�ֵ������ָ���ĶμĴ�����
| ��  �� : REGISTERS nRegister�����Ĵ������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::POP
| ��  �� : ��ջ����ȡ���ĸ��ֽڵ�ֵ������ָ���ı�����
| ��  �� : REGISTERS nRegister�����Ĵ������
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV
| ��  �� : ���(MODE11 == nMod)�ǰ�nSrcReg�Ĵ������ֵ�ƶ���nDstReg�Ĵ�����
��    �����ǰ�nSrcReg�Ĵ�����ָ�ı����е�ֵ�ƶ���nDstReg�Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg����Ŀ�ļĴ���
| ��  �� : COMMON_REGISTERS nSrcReg����Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_IV8_IMM
| ��  �� : ��һ���������ƶ����ֲ���8λ������
| ��  �� : int nRelativeDisplacement->�����RBP�������λ��
| ��  �� : BYTE nValue��>������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_IV16_IMM
| ��  �� : ��һ���������ƶ����ֲ���16λ������
| ��  �� : int nRelativeDisplacement->�����RBP�������λ��
| ��  �� : BYTE nValue��>������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_LIV32_IMM
| ��  �� : ��һ���������ƶ����ֲ���32λ������
| ��  �� : int nRelativeDisplacement->�����RBP�Ĵ�����λ��
| ��  �� : BYTE nValue��>������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_LIV32_OFFSET
| ��  �� : ��һ��ƫ�Ƶ�ַ�ƶ����ֲ���32λ������
| ��  �� : int nRelativeDisplacement->�����RBP�Ĵ�����λ��
| ��  �� : BYTE nOffset��>������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LIV32_OFFSET(int nRelativeDisplacement,DWORD nOffset)
{
	EmitInstruction(PREFIX_INVALID,0xc7,ModRM((abs(nRelativeDisplacement) < 127)? MODE01 : MODE10,0,RBP)
		,NONE_SIB,nRelativeDisplacement,nOffset,4,FALSE);
	AppendRef(GetOffset() - 4);   //����ɾ��

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [ebp %+d], offset(%X) \r\n","mov"
			,nRelativeDisplacement,nOffset);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::MOV_GIV32_OFFSET
| ��  �� : ��һ��ƫ�Ƶ�ַ�ƶ���ȫ�ֵ�32λ������
| ��  �� : DWORD nDisplacement->λ��
| ��  �� : BYTE nOffset��>ƫ�Ƶ�ַ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV32_OFFSET(DWORD nDisplacement,DWORD nOffset)
{
	EmitInstruction(PREFIX_INVALID,0xc7,ModRM(MODE00,0,RBP),NONE_SIB,nDisplacement,nOffset,4,FALSE);
	AppendRef(GetOffset() - 8);   //����ɾ��
	AppendRef(GetOffset() - 4);    //����ɾ��

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s dword ptr [(%Xh)], offset(%d) \r\n","mov",nDisplacement,nOffset);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::MOV_GFV_IMM
| ��  �� : ��һ���������������ƶ����ֲ���64λ(double)������,��������ͬ��
|       ����,�������ַ���
| ��  �� : int nRelativeDisplacement->�����RBP�Ĵ�����λ��
| ��  �� : DWORD nDisplacement->��������ӡ���ļ��е�λ��
| ��  �� : BIT_TYPE nBitType->Ŀ�ı���������32�򣶣�λ 
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_LFV_IMM(int nRelativeDisplacement,DWORD nDisplacement,BIT_TYPE nBitType)
{
	FLD(nDisplacement,BT_BIT64/*��Ϊ��������д��ӡ���ļ���ʱ��ȫ��ת��Ϊ�ˣ���λ��˫���ȵĸ�����*/);
	FSTP(nRelativeDisplacement,nBitType);
}


/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::MOV_GFV_IMM
| ��  �� : ��һ���������������ƶ�����̬�ı�����,��������ͬ��
|       ����,�������ַ���
| ��  �� : DWORD nDisplacement1->��̬������ӡ���ļ��е�λ��
| ��  �� : DWORD nDisplacement2->��������ӡ���ļ��е�λ��
| ��  �� : BIT_TYPE nBitType->Ŀ�ı���������32�򣶣�λ     
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GFV_IMM(DWORD nDisplacement1,DWORD nDisplacement2,BIT_TYPE nBitType)
{
	FLD(nDisplacement2,BT_BIT64/*��Ϊ��������д��ӡ���ļ���ʱ��ȫ��ת��Ϊ�ˣ���λ��˫���ȵĸ�����*/);
	FSTP(nDisplacement1,nBitType);   
}



/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::MOV_LIV8_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ����ֲ�8λ������
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_GIV8_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ�����̬��8λ������
| ��  �� : DWORD nRelativeDisplacement->��̬������ӡ���ļ��е�λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_LIV16_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ����ֲ�16λ������
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_GIV16_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ�����̬��16λ������
| ��  �� : DWORD nRelativeDisplacement->��̬������ӡ���ļ��е�λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV16_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg)
{
	if(RAX == nSrcReg)   //RAX��Ҫ��������
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
| �������� : CGenerateSaxProgram::MOV_LIV32_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ����ֲ�32λ������
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_GIV32_REG
| ��  �� : ��һ���Ĵ����е�ֵ�ƶ�����̬��32λ������
| ��  �� : DWORD nDisplacement->��̬������ӡ���ļ��е�λ��
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MOV_GIV32_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg)
{
	if(RAX == nSrcReg)   //RAX��Ҫ��������
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
| �������� : CGenerateSaxProgram::MOV_REG_IMM32
| ��  �� : ��ֵһ���������ƶ���nDstReg�Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg����Ŀ�ļĴ���
| ��  �� : DWORD nValue��������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_REG_LIV32
| ��  �� : ��һ���ֲ�32λ������ֵ�ƶ����Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ���
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_REG_GIV32
| ��  �� : ��һ����̬��32λ������ֵ�ƶ����Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ���
| ��  �� : DWORD nDisplacement->��̬������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOVZX_REG_LIV16
| ��  �� : ��һ���ֲ�16λ������ֵ�ƶ����Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ���
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MOV_REG_LIV8
| ��  �� : ��һ���ֲ�8λ������ֵ�ƶ����Ĵ�����
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ���
| ��  �� : DWORD nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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


//�����Ǹ���������ָ����غ���

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::FLD
| ��  �� : ��һ��������(����ʹ������Ҳ�����Ǿ�̬��ȫ�ֱ���)
|       �ƶ����������Ĵ�����
| ��  �� : DWORD nDisplacement->��������ӡ���ļ��е�λ��
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FLD
| ��  �� : ��һ��������(�ֲ�����)�ƶ����������Ĵ�����
| ��  �� : int nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FSTP
| ��  �� : �Ѹ������Ĵ����е����ƶ���һ��������������(�����Ǿ�̬,ȫ�ֱ���,�ֲ�����)
| ��  �� : int nOffset->�ֲ����������RBP�Ĵ�����λ�ƻ���ȫ�ֱ�����λ��
| ��  �� : BOOL bStatic->ȫ�ֱ�����־
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FST
| ��  �� : �Ѹ������Ĵ����е����ƶ���һ��������������(�����Ǿ�̬��ȫ�ֱ���)
| ��  �� : DWORD nDisplacement->��������ӡ���ļ��е�λ��
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FADDP
| ��  �� : �Ƚ���ʵ���ӷ���ST(i)��ST(i)+ST(0)��Ȼ�����һ�γ�ջ����
| ��  �� : DWORD i->ST(i)
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FADD_M64REAL
| ��  �� : ʵ���ӷ���ST(0)��ST(0)+m64real
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FSUB_M64REAL
| ��  �� : ʵ��������ST(0)��ST(0)-m64real
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FSUB_M64REAL
| ��  �� : ʵ��������ST(0)��m64real-ST(0)
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FSUBP
| ��  �� : �Ƚ���ʵ��������ST(i)��ST(i)-ST(0)��Ȼ�����һ�γ�ջ����
| ��  �� : DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FMULP
| ��  �� : ʵ���˷���ST(i)��ST(i)*ST(0)��ִ��һ�γ�ջ����
| ��  �� : DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FMUL_M64REAL
| ��  �� : ʵ���˷���ST(0)��ST(0)*m64real
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FIDIV_M64REAL
| ��  �� : ʵ��������ST(0)��ST(0)/m64real
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FDIVR_M64REAL
| ��  �� : ����������ST(0)��m32int/ST(0)
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FDIVP
| ��  �� : ʵ��������ST(i)��ST(i)/ST(0)��ִ��һ�γ�ջ����
| ��  �� :DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FILD
| ��  �� : ��һ������(�ֲ�����)�ƶ����������Ĵ�����
| ��  �� : int nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FLD
| ��  �� : ��ST(i)ѹջ����װ��ST(0)
| ��  �� : int nIndex->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FSTP_INDIRECT
| ��  �� : �Ѹ������Ĵ����е����ƶ����Ĵ�����ָʾ�ı�����
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType/* = BT_BIT32*/)
{
	if(RSP == nDstReg)     //��������
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
| �������� : CGenerateSaxProgram::FISTP_INDIRECT
| ��  �� : �Ѹ������Ĵ����е����ƶ����Ĵ�����ָʾ�ı�����
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : BOOL nBitType->��������32λ��64λ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FISTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType/* = BT_BIT32*/)
{

	if(RSP == nDstReg)     //��������
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
| �������� : CGenerateSaxProgram::FSTP
| ��  �� : ��ST(0)���Ƶ�ST(i)��ִ��һ�γ�ջ����
| ��  �� : int nIndex->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FCOMP_M64REAL
| ��  �� : ʵ���Ƚϣ�ST(0)-m64real�����ñ�־λ��ִ��һ�γ�ջ����
| ��  �� : DWORD nDisplacement->������������ӡ���ļ��е�λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FCOMP
| ��  �� : ʵ���Ƚϣ�ST(0)-ST(i)�����ñ�־λ��ִ��һ�γ�ջ����
| ��  �� : DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FCOMPP
| ��  �� : ʵ���Ƚϣ�ST(0)-ST(i)�����ñ�־λ��ִ�����γ�ջ����
| ��  �� : DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FLDZ
| ��  �� : ����C1 (C0, C2, C3δ����)   ��+0.0ѹջ����װ��ST(0)
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FUCOMPP
| ��  �� : �Ƚ�ST(0)��ST(1)��ִ��һ�γ�ջ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FUCOMP
| ��  �� : �Ƚ�ST(0)��ST(1)��ִ��һ�γ�ջ����
| ��  �� : DWORD i->�������Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FNSTSW
| ��  �� : ��FPU״̬�ֱ��浽AX�����������θ����쳣
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FCHS
| ��  �� : ������������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FNSTCW
| ��  �� : ��FPU�����ֱ��浽m2byte�����������θ����쳣
| ��  �� : int nRelativeDisplacement->�����RBP��ƫ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FLDCW
| ��  �� : ��m2byteװ��FPU������
| ��  �� : int nRelativeDisplacement->�����RBP��ƫ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FISTP
| ��  �� : ��m2byteװ��FPU������
| ��  �� : int nRelativeDisplacement->�����RBP��ƫ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FISTP
| ��  �� : ��m2byteװ��FPU������
| ��  �� : int nRelativeDisplacement->�����RBP��ƫ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::AND_IMM32
| ��  �� : Performs a logical AND of the two operands replacing the destination with the result. 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : DWORD nValue->32������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::AND
| ��  �� : Performs a logical AND of the two operands replacing the destination with the result. 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::ADD
| ��  �� : Adds "src" to "dest" and replacing the original contents of "dest".
Both operands are binary. 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::ADD_IMM32
| ��  �� : ��ָ���Ĵ����е�ֵ������һ��������
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : int nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ADD_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,0,nDstReg),NONE_SIB,0,nValue,1,FALSE); 
	}
	else
	{
		if(RAX == nDstReg) //��������
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
| �������� : CGenerateSaxProgram::ADC_IMM32
| ��  �� : ��ָ���Ĵ����е�ֵ������һ��������
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : DWORD nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::INC
| ��  �� : Adds one to destination unsigned binary operand. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SUB
| ��  �� : The source is subtracted from the destination and the result is
stored in the destination. 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SUB
| ��  �� : ��ָ���Ĵ����е�ֵ����ȥһ�������е�ֵ
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : DWORD nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SUB_IMM32
| ��  �� : ��ָ���Ĵ����е�ֵ����ȥһ��������
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : int nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SUB_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext)
{
	if(abs(nValue) <= 127)
	{
		EmitInstruction(PREFIX_INVALID,0x83,ModRM(MODE11,5,nDstReg),NONE_SIB,0,nValue,1,FALSE);
	}
	else
	{
		if(RAX == nDstReg) //��������
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
| �������� : CGenerateSaxProgram::SBB
| ��  �� : ��ָ���Ĵ����е�ֵ����ȥһ�������е�ֵ
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : DWORD nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SBB_IMM32
| ��  �� : Subtracts the source from the destination, and subtracts 1 extra if
the Carry Flag is set. Results are returned in "dest".
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : DWORD nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SBB_IMM32(COMMON_REGISTERS nDstReg,DWORD nValue,CContext &objContext)
{
	if(nValue < 127) //��������
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
| �������� : CGenerateSaxProgram::NEG
| ��  �� : Subtracts the destination from 0 and saves the 2s complement of
|       "dest" back into "dest". 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SBB
| ��  �� : SSubtracts the source from the destination, and subtracts 1 extra if
the Carry Flag is set. Results are returned in "dest". 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::XOR_IMM32
| ��  �� : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : int nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::XOR
| ��  �� : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::OR
| ��  �� : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::OR_IMM32
| ��  �� : Performs a bitwise exclusive OR of the operands and returns
the result in the destination. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : int nValue->������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::NOT
| ��  �� : Inverts the bits of the "dest" operand forming the 1s complement. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::IMUL
| ��  �� : Signed multiplication of accumulator by "src" with result placed
in the accumulator. If the source operand is a byte value, it
is multiplied by AL and the result stored in AX. If the source
operand is a word value it is multiplied by AX and the result is
stored in DX:AX. Other variations of this instruction allow
specification of source and destination registers as well as a
third immediate factor. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::IMUL
| ��  �� : Signed multiplication of accumulator by "src" with result placed
in the accumulator. If the source operand is a byte value, it
is multiplied by AL and the result stored in AX. If the source
operand is a word value it is multiplied by AX and the result is
stored in DX:AX. Other variations of this instruction allow
specification of source and destination registers as well as a
third immediate factor. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| ��  �� : �Ѳ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::IMUL
| ��  �� : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MUL
| ��  �� : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MUL
| ��  �� : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| ��  �� :int nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::DEC
| ��  �� : Unsigned binary subtraction of one from the destination. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::DIV
| ��  �� : Converts signed DWORD in EAX to a signed quad word in EDX:EAX by
extending the high order bit of EAX throughout EDX 
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::DIV
| ��  �� : Unsigned binary division of accumulator by source. If the source
divisor is a byte value then AX is divided by "src" and the quotient
is placed in AL and the remainder in AH. If source operand is a word
value, then DX:AX is divided by "src" and the quotient is stored in AX
and the remainder in DX. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::IDIV
| ��  �� : Signed binary division of accumulator by source. If source is a
byte value, AX is divided by "src" and the quotient is stored in
AL and the remainder in AH. If source is a word value, DX:AX is
divided by "src", and the quotient is stored in AL and the
remainder in DX. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHL
| ��  �� : �߼�����imm8��(�˷���r/m16=r/m16*(2^imm8))
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : DWORD nValue->���ƵĴ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHL
| ��  �� : �߼�����CL��(�˷���r/m16=r/m16*(2^CL))
| ��  �� : COMMON_REGISTERS nReg1->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHLD
| ��  �� : SHLD shifts "dest" to the left "count" times and the bit positions
opened are filled with the most significant bits of "src"
| ��  �� : COMMON_REGISTERS nReg1->�Ĵ������
| ��  �� : COMMON_REGISTERS nReg2->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHR
| ��  �� : �߼�����imm8��(�޷��ų�����r/m32=r/m32 / (2^imm8))
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : DWORD nValue->���ƵĴ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHR
| ��  �� : �߼�����imm8��(�޷��ų�����r/m32=r/m32 / (2^imm8))
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SHRD
| ��  �� : shifts "dest" to the right "count" times and the bit positions
opened are filled with the least significant bits of the second
operand. Only the 5 lower bits of "count" are used. 
| ��  �� : COMMON_REGISTERS nReg1->�Ĵ������
| ��  �� : COMMON_REGISTERS nReg2->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::RCR
| ��  �� : ����λѭ������imm8��
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : DWORD nValue->���ƵĴ���
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SAR
| ��  �� : Shifts the destination right by "count" bits with the current sign
bit replicated in the leftmost bit. The Carry Flag contains the
last bit shifted out. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SAR
| ��  �� : Shifts the destination right by "count" bits with the current sign
bit replicated in the leftmost bit. The Carry Flag contains the
last bit shifted out. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETE
| ��  �� : Sets the byte in the operand to 1 if the Zero Flag is set,
otherwise sets the operand to 0. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETNE
| ��  �� : SETNE/SETNZ are different mnemonics for the same instruction 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETG
| ��  �� : Sets the byte in the operand to 1 if the Zero Flag is set,
otherwise sets the operand to 0. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETGE
| ��  �� : Sets the byte in the operand to 1 if the Sign Flag equals the
Overflow Flag, otherwise sets the operand to 0. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETL
| ��  �� : Sets the byte in the operand to 1 if the Sign Flag is not equal
to the Overflow Flag, otherwise sets the operand to 0. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETLE
| ��  �� : Sets the byte in the operand to 1 if the Zero Flag is set or the
Sign Flag is not equal to the Overflow Flag, otherwise sets the
operand to 0. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETE
| ��  �� : Performs a logical AND of the two operands updating the flags
register without saving the result. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : BYTE nValue->�����Ե�ֵ
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::TEST_BIT8
| ��  �� : �ȽϼĴ�������8λ��ֵ,��bLowΪ��ʱnReg��al,bl,cl,dl
|       ������ah,bh,ch,dh
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : int nValue->�ж�ֵ
| ��  �� : BOOL bLow->�Ĵ�������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::SETE
| ��  �� : Performs a logical AND of the two operands updating the flags
register without saving the result. 
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::CMP_IMM8
| ��  �� : �Ĵ����е�ֵ��һ��8λ�������Ƚ�
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : BYTE nValue->8λ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::CMP_IMM32
| ��  �� : �Ĵ����е�ֵ��һ��32λ�������Ƚ�
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : BYTE nValue->32λ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::CMP
| ��  �� : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� :int nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::CMP
| ��  �� : Unsigned multiply of the accumulator by the source. If "src" is
a byte value, then AL is used as the other multiplicand and the
result is placed in AX. If "src" is a word value, then AX is
multiplied by "src" and DX:AX receives the result. If "src" is
a double word value, then EAX is multiplied by "src" and EDX:EAX
receives the result. The 386+ uses an early out algorithm which
makes multiplying any size value in EAX as fast as in the 8 or 16
bit registers. 
| ��  �� : COMMON_REGISTERS nReg1->�Ĵ������
| ��  �� : COMMON_REGISTERS nReg2->�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::CMP_BIT8
| ��  �� : �ȽϼĴ�������8λ��ֵ,��bLowΪ��ʱnReg��al,bl,cl,dl
|       ������ah,bh,ch,dh
| ��  �� : COMMON_REGISTERS nReg->�Ĵ������
| ��  �� : int nValue->�ж�ֵ
| ��  �� : BOOL bLow->�Ĵ�������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::JMP_IMM32
| ��  �� : ������������תָ��
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| ����:    û������ǰ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JMP_IMM32(int nRelativeDisplacement,CContext &/*objContext*/)
{  
	if(abs(nRelativeDisplacement) < 127)
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 2; //�������ǰ��,��ȥjmpָ������ĳ���2
		}
		EmitInstruction(0xEB,(BYTE)CalculateComplement(nRelativeDisplacement));
	}
	else
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 5; //�������ǰ��,��ȥjmpָ������ĳ���5
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
			nRelativeDisplacement -= 2; //�������ǰ��,��ȥjmpָ������ĳ���2
		}
		EmitInstruction((BYTE)nInst1,(BYTE)CalculateComplement(nRelativeDisplacement));
	}
	else
	{
		if(nRelativeDisplacement < 0)
		{
			nRelativeDisplacement -= 6; //�������ǰ��,��ȥjmpָ������ĳ���6
		}
		EmitInstruction(PREFIX_LOCK,(BYTE)(nInst1 + 0x10),CalculateComplement(nRelativeDisplacement));
	}

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %+d\r\n" ,JMP_INSTRUCTION_NAME[nInst1 - 0x70],nRelativeDisplacement);
	}

}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JE
| ��  �� : Causes execution to branch to "label" if the Zero Flag is clear or
the Sign Flag equals the Overflow Flag. Signed comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JE(int nRelativeDisplacement,CContext &/*objContext*/)
{	
	JMP_BASE(JIC_JE,nRelativeDisplacement);	
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JNE
| ��  �� : Causes execution to branch to "label" if the Zero Flag is clear. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNE,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JGE
| ��  �� : Causes execution to branch to "label" if the Sign Flag equals
the Overflow Flag. Signed comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JGE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNL,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JC
| ��  �� : Causes execution to branch to "label" if the Carry Flag is set.
Functionally similar to JB and JNAE. Unsigned comparison. Causes execution to
branch to "label" if the Carry Flag is set.Functionally similar to JB and JNAE. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JC(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JA
| ��  �� : Causes execution to branch to "label" if the Carry Flag and Zero Flag
are both clear. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JA(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JA,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JB
| ��  �� :Causes execution to branch to "label" if the Carry Flag is set.
Functionally similar to JC. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JB(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JBE
| ��  �� : Causes execution to branch to "label" if the Carry Flag or
the Zero Flag is set. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JBE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JBE,nRelativeDisplacement);

}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JNS
| ��  �� : Causes execution to branch to "label" if the Sign Flag is clear. Signed comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNS(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNS,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JAE
| ��  �� : Causes execution to branch to "label" if the Carry Flag is clear.
Functionally similar to JNC. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JAE(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNB,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JG
| ��  �� : Causes execution to branch to "label" if the Zero Flag is clear or
the Sign Flag equals the Overflow Flag. Signed comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JG(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JG,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JL
| ��  �� : Causes execution to branch to "label" if the Sign Flag is not equal
to Overflow Flag. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JL(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JL,nRelativeDisplacement);
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JP
| ��  �� : Causes execution to branch to "label" if the Parity Flag is set. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JP(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JP,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JNP
| ��  �� : Causes execution to branch to "label" if the Parity
|       Flag is clear. Unsigned comparison. 
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNP(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNP,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::JNP
| ��  �� : ��������ת
| ��  �� : int nRelativeDisplacement->����ڵ�ǰ��תָ���λ��
|       ��ǰ��ʱ����
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::JNG(int nRelativeDisplacement,CContext &/*objContext*/)
{
	JMP_BASE(JIC_JNG,nRelativeDisplacement);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::LEA
| ��  �� : �õ�һ���ֲ�������ƫ�Ƶ�ַ���ŵ�ָ���ļĴ���
| ��  �� : COMMON_REGISTERS nDstReg->�Ĵ������
| ��  �� : int nRelativeDisplacement->�ֲ����������RBP�Ĵ�����λ��
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::XCHG
| ��  �� : Exchanges contents of source and destination. 
| ��  �� : COMMON_REGISTERS nDstReg->Ŀ��Ĵ������
| ��  �� : COMMON_REGISTERS nSrcReg->Դ�Ĵ������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::RET
| ��  �� : ������������
| �޸ļ�¼ : 2014-5-2 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::MovIntValtoReg
| ��  �� : �ƶ�һ������(�����򵥱������������á�ָ������)�е�ֵ��ָ���ļĴ�����
| ��  �� : COMMON_REGISTERS nDstReg����Ŀ�ļĴ���
| ��  �� : CSyntaxTreeNode* pValue���������Ǽ򵥱���
| ��  �� : COMMON_REGISTERS nExtDstReg������չĿ�ļĴ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::MovIntValtoReg(CSyntaxTreeNode* pValue
	,COMMON_REGISTERS nSrcReg,COMMON_REGISTERS nExtDstReg,CContext &objContext)
{
	if(objContext.GetCurVar() == pValue && RAX == nSrcReg) 
	{//������ֵ�Ѿ��ڼĴ������˲���Ҫ���¼���
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
		|| pValue->GetNodeType() == TNK_VAR_REF)   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValue)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
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
	case TNK_DWORD_TYPE:           //��Щ������Ϊ����ת�ݵ�ʱ��,��ջ�ж���ռ���ĸ��ֽ�
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

	if(pValue->GetNodeType() == TNK_INDIRECT_REF)   //ָ�����ñ���a = *b;
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

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������
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
					FreeReg(RCX,objContext,nStatus);    //�������˳���෴
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��       
				}
				else
				{
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������

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
			if(!bStatic)  //�Ǿ�̬���� 
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
| �������� : CGenerateSaxProgram::MovIntValtoReg
| ��  �� : �ƶ�ָ���ļĴ����е�ֵ��һ������(�����򵥱�������������
|       ��ָ������)��
| ��  �� : CSyntaxTreeNode* pVariableNode����Ŀ�����
| ��  �� : COMMON_REGISTERS nSrcReg����Դ�Ĵ���
| ��  �� : COMMON_REGISTERS nExtSrcReg����Դ�Ĵ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	if(pVariableNode->GetNodeType() == TNK_ARRAY_REF //˵������������,����: Test[9]
		|| pVariableNode->GetNodeType() == TNK_VAR_REF)   
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
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

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{		
					UINT nStatus = 0;
					COMMON_REGISTERS nTmpReg = RequestReg(CRM_RCX,FALSE,objContext,nStatus);					
					if(RAX == nSrcReg || RAX == nExtSrcReg)
					{
						MOV(nTmpReg,RAX,objContext); 
					}
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������

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
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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

					GenerateExprStatementsCode(pArryIndex,objContext);           //�������������±꣬�ŵ�EAX�Ĵ�������

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
					OFFSET += (int)(pArryIndex->GetValue1() * DATA_SIZE);    //ֱ�Ӽ��������
				}
			}
		}

		if(!bHasEmited)
		{
			if(RAX == nSrcReg && bStatic)      //�ѼĴ���RAX�е�ֵ�ƶ�����̬��������Ҫ��������
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
				if(RAX == nExtSrcReg && bStatic)    //�ѼĴ���RAX�е�ֵ�ƶ�����̬��������Ҫ��������
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
			if(!bStatic)  //�Ǿ�̬���� 
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
| �������� : CGenerateSaxProgram::Offset
| ��  �� : ���ĳ��������ƫ�Ƶ�ַ���ŵ���һ��������
| ��  �� : CDeclarationTreeNode* pVariableNode1�������ƫ�Ƶ�ַ�ı���
| ��  �� : CDeclarationTreeNode* pVariableNode2��������ƫ�Ƶ�ַ�ı���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::Offset(CSyntaxTreeNode* pVariableNode1,CSyntaxTreeNode* pVariableNode2)
{
	ATLASSERT(pVariableNode1&& pVariableNode2);

	if(((CDeclarationTreeNode*)pVariableNode1)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
	{
		MOV_LIV32_OFFSET(pVariableNode1->GetOffset(),pVariableNode2->GetOffset());
	}
	else
	{
		MOV_GIV32_OFFSET(pVariableNode1->GetOffset(),pVariableNode2->GetOffset());
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::SetIntVariable
| ��  �� : �������α�����ֵ
| ��  �� : CDeclarationTreeNode* pVariableNode���������õı���
| ��  �� : DWORD nValue���������õı�����ֵ
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::SetIntVariable(CSyntaxTreeNode* pVariableNode
	,LONGLONG nValue,CContext& objContext)
{
	ATLASSERT(pVariableNode);

	CDataTypeTreeNode* pDataTypeNode = pVariableNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pVariableNode;
	if(pVariableNode->GetNodeType() == TNK_ARRAY_REF
		|| pVariableNode->GetNodeType() == TNK_VAR_REF)   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pVariableNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
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

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������

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
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ��������
				}
				else
				{
					GenerateExprStatementsCode(pArryIndex,objContext);           //�������������±꣬�ŵ�EAX�Ĵ�������
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
| �������� : CGenerateSaxProgram::FLDVariable
| ��  �� : ��ָ�������е�����װ�ص��������Ĵ�����
| ��  �� : CSyntaxTreeNode* pValueNode���������ڵ�
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FLDVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF 
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}

	if(pValueNode->IsNumericConstants())  //�ǳ���
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
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
			FILD(-(objContext.GetOffset() + 4));        //װ�ص��������Ĵ�����
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
					MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //���ڴ��ַ�ŵ�RAX�Ĵ�����
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

							MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

							if(!pArryIndex->IsNumericConstants())
							{					
								GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������
								EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
								if(m_bEnableMasm)
								{
									GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
										,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
						
								}							
							}
							else
							{
								OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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
								GenerateExprStatementsCode(pArryIndex,objContext);           //�������������±꣬�ŵ�EAX�Ĵ�������
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
								OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ��������
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
| �������� : CGenerateSaxProgram::FLDVariable
| ��  �� : ��ST(0)���Ƶ�m64real��m32real�����У�ִ��һ�γ�ջ����
| ��  �� : CSyntaxTreeNode* pValueNode���������ڵ�
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTPVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF )   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
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
		FNSTCW(-(objContext.GetOffset() + 2));         //��FPU�����ֱ��浽ջ��
		MOVZX_REG_LIV16(RAX,-(objContext.GetOffset() + 2),objContext);  //�ٽ�FPU�������ƶ���RAX�Ĵ�����
		OR_IMM32(RAX,0x0C00,objContext);                 //�޸�RC(�������)11(��0����ض�)http://book.51cto.com/art/200907/135136.htm
		MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);   //
		FLDCW(-(objContext.GetOffset() + 8));         //����װ�������
		FISTP(-(objContext.GetOffset() + 8));         //��ST(0)��32λ�������浽m32int��ִ��һ�γ�ջ����
		MOV_REG_LIV8(RDX,-(objContext.GetOffset() + 8),objContext);
		FLDCW(-(objContext.GetOffset() + 2)); 
		MovRegtoIntVal(pValueNode,RAX,RDX,objContext);
		return;

	case TNK_BOOLEAN_TYPE:                   //�������˼
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
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //���ڴ��ַ�ŵ�RAX�Ĵ�����
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
				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
					}							
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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
					GenerateExprStatementsCode(pArryIndex,objContext);           //�������������±꣬�ŵ�EAX�Ĵ�������

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
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ��������
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
| �������� : CGenerateSaxProgram::FSTVariable
| ��  �� : ��ST(0)���Ƶ�m64real��m32real������
| ��  �� : CSyntaxTreeNode* pValueNode���������ڵ�
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::FSTVariable(CSyntaxTreeNode* pValueNode,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
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
		FNSTCW(-(objContext.GetOffset() + 2));         //��FPU�����ֱ��浽ջ��
		MOVZX_REG_LIV16(RAX,-(objContext.GetOffset() + 2),objContext);  //�ٽ�FPU�������ƶ���RAX�Ĵ�����
		OR_IMM32(RAX,0x0C00,objContext);                 //�޸�RC(�������)11(��0����ض�)http://book.51cto.com/art/200907/135136.htm
		MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);   //
		FLDCW(-(objContext.GetOffset() + 8));         //����װ�������
		FIST(-(objContext.GetOffset() + 8));          //��ST(0)��32λ�������浽m32int��ִ��һ�γ�ջ����
		MOV_REG_LIV8(RDX,-(objContext.GetOffset() + 8),objContext);
		FLDCW(-(objContext.GetOffset() + 2)); 
		MovRegtoIntVal(pValueNode,RAX,RDX,objContext);
		return;

	case TNK_BOOLEAN_TYPE:                   //�������˼
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
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,RDX,objContext);  //���ڴ��ַ�ŵ�RAX�Ĵ�����
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
				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext); //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);             //�������������±꣬�ŵ�EAX�Ĵ�������
					EmitInstruction(PREFIX_INVALID,nOpCode,ModRM(MODE00,ModRM_REG,SIB_ADDR),SIB(nIndex,RAX,RBX),0,0,0,FALSE);
					if(m_bEnableMasm)
					{
						GenerateMasmCmd("%-10s %s ptr [rax * %d + rbx]\r\n"
							,strInst,GetDataType(pDataTypeNode->GetSize()),pDataTypeNode->GetSize());
					}							
				}
				else
				{
					OFFSET  = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
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
					GenerateExprStatementsCode(pArryIndex,objContext);           //�������������±꣬�ŵ�EAX�Ĵ�������

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
					OFFSET += (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ��������
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
| �������� : CGenerateSaxProgram::CompareIntVariable
| ��  �� : �ж�ĳ��������ֵ
| ��  �� : CSyntaxTreeNode* pValueNode���������ڵ�
| ��  �� : LONGLONG nValue����������ֵ
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::CompareIntVariable(CSyntaxTreeNode* pValueNode,LONGLONG nValue,CContext& objContext)
{
	CDataTypeTreeNode* pDataTypeNode = pValueNode->GetDataType();

	CDeclarationTreeNode* pVariableDeclNode = (CDeclarationTreeNode*)pValueNode;
	if(pValueNode->GetNodeType() == TNK_ARRAY_REF
		|| pValueNode->GetNodeType() == TNK_VAR_REF)   //˵������������,����: Test[9]
	{
		pVariableDeclNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pValueNode)->GetChildNode(0);   
	}

	MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
	BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
	int OFFSET  = pVariableDeclNode->GetOffset();
	if(((CDeclarationTreeNode*)pVariableDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
	{
		MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
		bStatic  = FALSE;
	}
	const UINT DATA_SIZE = pDataTypeNode->GetSize();

	UINT nRefPos = 0;

	if(pValueNode->GetNodeType() == TNK_INDIRECT_REF)
	{
		MovIntValtoReg(((CExpressionTreeNode*)pValueNode)->GetChildNode(0),RAX,CR_INVALID,objContext);  //���ڴ��ַ�ŵ�RAX�Ĵ�����

		if(DATA_SIZE == 8)
		{
			EmitInstruction((BYTE)((DATA_SIZE == 2)? PREFIX_OPERAND_SIZE : PREFIX_INVALID)
				,(DATA_SIZE == 1)? 0x80: 0x81
				,ModRM(MODE01,7,RAX),NONE_SIB,4,nValue>>32,4,FALSE);
			JNE(0,objContext);                        //���������ָ��
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

				MovIntValtoReg(pSrcVarNode,RBX,CR_INVALID,objContext);   //�ѻ�����ַ�ƶ���RBX�Ĵ�����

				if(!pArryIndex->IsNumericConstants())
				{					
					GenerateExprStatementsCode(pArryIndex,objContext);   //�������������±꣬�ŵ�EAX�Ĵ�������
					
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

						JNE(0,objContext);                        //���������ָ��
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
					OFFSET = (int)(pArryIndex->GetValue1() * pDataTypeNode->GetSize());    //ֱ�Ӽ����ƫ��
					
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

						JNE(0,objContext);                        //���������ָ��
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
					OFFSET += (int)(pArryIndex->GetValue1() * DATA_SIZE);    //ֱ�Ӽ����ƫ��       
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

						JNE(0,objContext);                        //���������ָ��
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

				JNE(0,objContext);                        //���������ָ��
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
| �������� : CGenerateSaxProgram::FloatingCmpCommonCode
| ��  �� : �������Ƚ�ͨ�ô���
| ��  �� : CSyntaxTreeNode* pParantExprNode�����Ƚϱ���ǵĸ��ڵ�
| ��  �� : TREE_NODE_TYPE nOperator������������
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	pConditionExprJmpRef->AppendRefPos(pConditionExprJmpRef->GetJmpType(),GetOffset());  //ֻ��¼ʧ����ת��λ��

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
| �������� : CGenerateSaxProgram::IntegerOperator
| ��  �� : �����������Ǹ�����������,�ṹ�浽�������Ĵ���
| ��  �� : CSyntaxTreeNode* pLeftExprNode������߲�����
| ��  �� : CSyntaxTreeNode* pRightExprNode�����ұ߲�����
| ��  �� : TREE_NODE_TYPE nOperator������������
| ����ע�� : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
			FILD(-(objContext.GetOffset() + 4));        //װ�ص��������Ĵ�����

			switch(nOperator) 
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pRightExprNode);      //����Ǹ�����
				break;

			case TNK_MINUS_EXPR:
				FSUB_M64REAL(pRightExprNode);      //����Ǹ�����
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pRightExprNode);      //����Ǹ�����
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
			GenerateExprStatementsCode(pRightExprNode,objContext); //�϶��Ǹ�����
			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pLeftExprNode);      //����Ǹ�����
				break;

			case TNK_MINUS_EXPR:
				FSUBR_M64REAL(pLeftExprNode);      //����Ǹ�����
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pLeftExprNode);      //����Ǹ�����
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
			MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);    //�ѽ���ƶ���ջ��
			FILD(-(objContext.GetOffset() + 4));          //װ�ص��������Ĵ�����

			GenerateExprStatementsCode(pRightExprNode,objContext); //�϶��Ǹ�����

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
				FADD_M64REAL(pRightExprNode);      //����Ǹ�����
				break;

			case TNK_MINUS_EXPR:
				FSUB_M64REAL(pRightExprNode);
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pRightExprNode);      //����Ǹ�����
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
			GenerateExprStatementsCode(pRightExprNode,objContext); //����Ϊ�κ�����

			if(pRightNodeDataType->GetNodeType() == TNK_LONGLONG_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
				FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //װ�ص��������Ĵ�����
			}
			else if(pRightNodeDataType->GetNodeType() != TNK_FLOAT_TYPE
				&& pRightNodeDataType->GetNodeType() != TNK_DOUBLE_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
				FILD(-(objContext.GetOffset() + 4));        //װ�ص��������Ĵ�����
			}

			switch(nOperator)
			{
			case TNK_PLUS_EXPR:
				FADD_M64REAL(pLeftExprNode);      //����Ǹ�����
				break;

			case TNK_MINUS_EXPR:
				FSUBR_M64REAL(pLeftExprNode);
				break;

			case TNK_MULT_EXPR:
				FMUL_M64REAL(pLeftExprNode);      //����Ǹ�����
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
				MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
				FILD(-(objContext.GetOffset() + 8));        //װ�ص��������Ĵ�����
			}
			else if(pRightNodeDataType->GetNodeType() != TNK_FLOAT_TYPE
				&& pRightNodeDataType->GetNodeType() != TNK_DOUBLE_TYPE)
			{
				MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
				FILD(-(objContext.GetOffset() + 4));        //װ�ص��������Ĵ�����
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
| �������� : CGenerateSaxProgram::IntegerOperator
| ��  �� : ������������͵�����,�ṹ�嵽�Ĵ���RDX:RAX��
| ��  �� : CSyntaxTreeNode* pLeftExprNode������߲�����
| ��  �� : CSyntaxTreeNode* pRightExprNode�����ұ߲�����
| ��  �� : TREE_NODE_TYPE nOperator������������
| ע��   : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	BOOL bIsUnsigned          = pParantExprNode->IsUnsigned();   //�õ����ʽ�Ƿ����޷���

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
		pConditionExprJmpRef->AppendRefPos(pConditionExprJmpRef->GetJmpType(),GetOffset());  //ֻ��¼ʧ����ת��λ��
	}
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateSimpleAssignmentOp
| ��  �� : Ϊ��ֵ��䴴������
| ��  �� : CDeclarationTreeNode* pVariableNode��������ֵ�Ľڵ�
| ��  �� : CSyntaxTreeNode* pValue����ֵ�ڵ�,����ʹ����,����,���ʽ
| ��ע:�˺����Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	case TNK_BOOLEAN_TYPE:          //Ϊһ���ֽڵı�����ֵ
	case TNK_SHORT_TYPE:
	case TNK_WORD_TYPE:
	case TNK_INT_TYPE:
	case TNK_LONG_TYPE:
	case TNK_DWORD_TYPE:           //��Щ������Ϊ����ת�ݵ�ʱ��,��ջ�ж���ռ���ĸ��ֽ�
	case TNK_LONGLONG_TYPE:         //�ص����
		if(pValue->IsNumericConstants())   //�ǳ���
		{
			LONGLONG nValue = LONGLONG(pValue->GetValue1() + pValue->GetValue2());
			if(pVarDataTypeNode->GetNodeType() == TNK_BOOLEAN_TYPE)
			{
				nValue = (nValue != 0)? 1: 0;
			}
			SetIntVariable(pVariableNode, nValue,objContext);       
		}
		else                 //����˵����һ�����ʽ
		{
			GenerateExprStatementsCode(pValue,objContext);      //���ʽ��ֵ����ŵ�RAX�Ĵ�������   
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
		if(!pValue->IsExpression())                   //����һ�����ʽ
		{
			FLDVariable(pValue,objContext);
		}
		else                              //����˵����һ�����ʽ
		{
			GenerateExprStatementsCode(pValue,objContext);       //���ʽ��ֵ�����ʵ������ŵ��������Ĵ�������,�������RAX�Ĵ�������   

			CDataTypeTreeNode* pValueDataTypeNode = pValue->GetDataType();
			if(pValueDataTypeNode->IsInterger())            //˵�����ʽ�Ľ��������,��ô�ȿ�������ջ,�ú���ת�Ƶ��������Ĵ���
			{               
				if(pVariableNode->GetSize() == 8)
				{
					MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
					MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
					FILD(-(objContext.GetOffset() + 8),BT_BIT64);
				}
				else
				{
					MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
					FILD(-(objContext.GetOffset() + 4),BT_BIT32);
				}
			}          
		}

		FSTPVariable(pVariableNode,objContext);
		break;

	case TNK_POINTER_TYPE:
		switch(pValue->GetNodeType())
		{
		case TNK_ADDR_EXPR:               //ȡ��ַ����
			GenerateExprStatementsCode(pValue,objContext);      //���ʽ��ֵ����RAX�Ĵ�������   
			MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);            
			break;

		case TNK_VAR_DECL:        //ָ�����
			{
				CDataTypeTreeNode* pLocalVarDataTypeNode = pValue->GetDataType();
				if(pLocalVarDataTypeNode->GetNodeType() == TNK_ARRAY_TYPE)           //�������׵�ַ�ƶ���һ��ָ�������
				{
					if(((CDeclarationTreeNode*)pValue)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //˵���Ǿֲ�����
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
			if(pValue->IsNumericConstants())   //�ǳ���
			{
				SetIntVariable(pVariableNode, pValue->GetValue1(),objContext);       
			}
			else 
			{
				GenerateExprStatementsCode(pValue,objContext);      //���ʽ��ֵ����RAX�Ĵ�������   
				MovRegtoIntVal(pVariableNode,RAX,CR_INVALID,objContext);
			}
		}
		break;

	case TNK_REFERENCE_TYPE:
		switch(pValue->GetNodeType())
		{
		case TNK_VAR_DECL:
			if(((CDeclarationTreeNode*)pValue)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //˵���Ǿֲ�����
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

	case TNK_ARRAY_TYPE:    //˵����������Ҫ����ʼ��
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
| �������� : CGenerateSaxProgram::GenerateLableSTMTCode
| ��  �� : �����ǩ���,�����䲻�����κδ���
| ��  �� : CDeclarationTreeNode* pNode������ǩ�����ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
				WriteDwordData(pLableRefrenceInfo->GetRefenceOffset(i) - 4/*JMPָ�����һ���ֽ�*/
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
| �������� : CGenerateSaxProgram::GenerateGotoSTMTCode
| ��  �� : ����goto���
| ��  �� : CDeclarationTreeNode* pNode������ǩ�����ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateGotoSTMTCode(CDeclarationTreeNode* pNode,CContext& objContext)
{
	if(pNode->GetOffset() > 0)   //˵��Ҫ��ת��λ���Ѿ�����,����ǩ�����goto����ǰ��
	{
		JMP_IMM32(pNode->GetOffset() - GetOffset(),objContext);    
	}
	else
	{
		JMP_IMM32(GetOffset(),objContext); //�˴��Ĳ���û���κ���˼,��󽫱�ʵ�ʵĲ���ȡ��

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
| �������� : CGenerateSaxProgram::GenerateCallSTMTCode
| ��  �� : ����Call���
| ��  �� : CExpressionTreeNode* pNode����call���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateCallSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext)
{
	UINT nParaSize = 0;      //������ջ��ռ�õĿռ�
	ATLASSERT(pExpressionNode->GetNodeType() == TNK_CALL_EXPR);

	CDeclarationTreeNode* pFunctionDeclNode = (CDeclarationTreeNode*)pExpressionNode->GetChildNode(0);
	CExpressionTreeNode* pParaExpressionNode = (CExpressionTreeNode*)pFunctionDeclNode->GetArguments(); //�õ����������б�
	if(NULL != pParaExpressionNode)
	{
		CDeclarationTreeNode* pParaDeclNodes  = (CDeclarationTreeNode*)pParaExpressionNode->GetChildNode(0); //�õ����������б�

		if(objContext.GetRegisterUsedMask() & CRM_RDX)
		{
			PUSH(RDX);
		}
		//����Ĵ����ǲ�����ѹ��ջ
		CDataTypeTreeNode* pParaVarDataTypeNode  = NULL;
		CDataTypeTreeNode* pParaDeclDataTypeNode = NULL;
		CSyntaxTreeNode* pParaValueNodes = pExpressionNode->GetChildNode(1);  //�õ�����ֵ�б�
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
			case TNK_DWORD_TYPE:                      //��Щ������Ϊ����ת�ݵ�ʱ��,��ջ�ж���ռ���ĸ��ֽ�
				objContext.IncOffset(4);
				if(pParaValueNodes->IsNumericConstants())            //�ǳ���
				{
					PUSH_IMM32(DWORD(pParaValueNodes->GetValue1() + pParaValueNodes->GetValue2()));        
				}
				else                            //����˵����һ�����ʽ
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //���ʽ��ֵ����ŵ�RAX�Ĵ�������
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
				PUSH_IMM32(0);                          //��ָ���Ŀ������ջ�з��䣴���ֽڣ������ܱ�"sub esp,4"��  

				if(pParaValueNodes->IsNumericConstants())            //�ǳ���
				{
					FLDVariable(pParaValueNodes,objContext);
				}
				else
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //���ʽ��ֵ����ŵ�RAX�Ĵ�������

					if(pParaVarDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
						FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //װ�ص��������Ĵ�����
					}
					else if(pParaVarDataTypeNode->IsInterger())
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
						FILD(-(objContext.GetOffset() + 4));               //װ�ص��������Ĵ�����      
					}						
				}
				FSTP_INDIRECT(RSP);

				nParaSize += 4;
				break;

			case TNK_DOUBLE_TYPE:
				objContext.IncOffset(8);
				SUB_IMM32(RSP,8,objContext);                      //��ָ���Ŀ������ջ�з���8���ֽ�            

				if(pParaValueNodes->IsNumericConstants())            //�ǳ���
				{
					FLDVariable(pParaValueNodes,objContext);
				}
				else
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //���ʽ��ֵ����ŵ�RAX�Ĵ�������

					if(pParaVarDataTypeNode->GetNodeType() == TNK_LONGLONG_TYPE)
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
						FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //װ�ص��������Ĵ�����
					}
					else if(pParaVarDataTypeNode->IsInterger())
					{
						MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
						FILD(-(objContext.GetOffset() + 4));               //װ�ص��������Ĵ�����      
					}						
				}
				FSTP_INDIRECT(RSP,BT_BIT64);

				nParaSize += 8;
				break;

			case TNK_POINTER_TYPE:
				switch(pParaValueNodes->GetNodeType())
				{
				case TNK_ADDR_EXPR:               //ȡ��ַ����
					GenerateExprStatementsCode(pParaValueNodes,objContext);      //���ʽ��ֵ����RAX�Ĵ�������   
					PUSH(RAX);           
					break;

				case TNK_VAR_DECL:        //ָ�����
					if(pParaVarDataTypeNode->GetNodeType() == TNK_ARRAY_TYPE)           //�������׵�ַ�ƶ���һ��ָ�������
					{
						if(((CDeclarationTreeNode*)pParaValueNodes)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //˵���Ǿֲ�����
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
					if(pParaValueNodes->IsNumericConstants())   //�ǳ���
					{
						PUSH_IMM32((DWORD)pParaValueNodes->GetValue1());       
					}
					else 
					{
						GenerateExprStatementsCode(pParaValueNodes,objContext);      //���ʽ��ֵ����RAX�Ĵ�������   
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
					if(((CDeclarationTreeNode*)pParaValueNodes)->GetDeclAttribute() ^ NDA_STATIC_FLAG) //˵���Ǿֲ�����
					{
						LEA(RAX,pParaValueNodes->GetOffset(),objContext);
						PUSH(RAX);       
					}
					else
					{
						PUSH_IMM32(pParaValueNodes->GetOffset());        
						AppendRef(GetOffset() - 4);    //��̬����,��¼�ο���ַ        
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

			case TNK_LONGLONG_TYPE:         //�ص����
				objContext.IncOffset(8);
				if(pParaValueNodes->IsNumericConstants())            //�ǳ���
				{
					LONGLONG nTmp = pParaValueNodes->GetValue1() + (LONGLONG)pParaValueNodes->GetValue2();
					PUSH_IMM32(nTmp >> 32);    
					PUSH_IMM32(nTmp & 0xFFFFFFFF);
				}
				else                            //����˵����һ�����ʽ
				{
					GenerateExprStatementsCode(pParaValueNodes,objContext);   //���ʽ��ֵ����ŵ�RAX�Ĵ�������
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

						PUSH(RDX);  //���Ƹ�λ4���ֽ�
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

	EmitInstruction(0xE8,CalculateComplement(pFunctionDeclNode->GetOffset() - GetOffset() - 5)); //�����λ��

	if(m_bEnableMasm)
	{
		GenerateMasmCmd("%-10s %s (%x)\r\n","call",pFunctionDeclNode->GetSymbol()
			,pFunctionDeclNode->GetOffset());
	}
	objContext.DecOffset(nParaSize);
	ADD_IMM32(RSP,nParaSize,objContext); //�ƶ�ջָ��,�ͷ�Ϊ��������ռ�

	if(objContext.GetRegisterUsedMask() & CRM_RDX)
	{
		POP(RDX,objContext);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprAddrCode
| ��  �� : ��ȡһ�������ʽ(�򵥱���,����Ԫ������,ָ������)�ĵ�ַ
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateExprAddrCode( CExpressionTreeNode* pNode,CContext& objContext)
{
	CDeclarationTreeNode* pExprNode = (CDeclarationTreeNode*)((CExpressionTreeNode*)pNode)->GetChildNode(0);
	ATLASSERT(pExprNode);
	if(pExprNode->GetNodeType() == TNK_ARRAY_REF)         //����&name[12]
	{
		CDeclarationTreeNode* pArrayDeclNode = (CDeclarationTreeNode*)(((CExpressionTreeNode*)pExprNode)->GetChildNode(0));

		MOD MODE_TYPE = MODE00;  //Ĭ��Ѱַ��Ϊ��̬����
		BOOL bStatic = TRUE;   //Ĭ��Ѱַ��Ϊ��̬����
		const int OFFSET  = pArrayDeclNode->GetOffset();
		if(((CDeclarationTreeNode*)pArrayDeclNode)->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //�Ǿ�̬���� 
		{
			MODE_TYPE = (abs(OFFSET) < 127)? MODE01:MODE10;   
			bStatic  = FALSE;
		}

		CDataTypeTreeNode* pDataTypeNode   = pArrayDeclNode->GetDataType()->GetDataType();    //�õ������Ԫ������ 
		GenerateExprStatementsCode(((CExpressionTreeNode*)pExprNode)->GetChildNode(1),objContext); //�����±�,����ֵ�����RAX�Ĵ�������
		const int nScale  = CalculationIndex(pDataTypeNode->GetSize());


		UINT nStatus = 0;
		COMMON_REGISTERS nFreeReg = RequestReg(CRM_RCX,TRUE,objContext,nStatus);
		
		EmitInstruction(PREFIX_INVALID,0x8D,ModRM(MODE_TYPE,nFreeReg,SIB_ADDR),SIB(nScale,RAX,RBP)
			,pArrayDeclNode->GetOffset(),0,0,bStatic);        //�ѵ�ַ�ŵ�"nFreeReg"�Ĵ�����
		if(m_bEnableMasm)
		{
			if(bStatic) //˵���Ǿֲ�����
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
		if(pExprNode->GetDeclAttribute() ^ NDA_STATIC_FLAG) //˵���Ǿֲ�����
		{
			LEA(RAX,pExprNode->GetOffset(),objContext);
		}
		else
		{
			MOV_REG_IMM32(RAX,pExprNode->GetOffset(),objContext);        
			AppendRef(GetOffset() - 4);    //��̬����,��¼�ο���ַ 
		}
	}
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::ExtractFactorValue
| ��  �� : ��ȡһ�������������ֵ
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::ExtractFactorValue( CSyntaxTreeNode* pNode,CContext& objContext)
{
	switch(pNode->GetNodeType())
	{
	case TNK_STRING_CST:
		MOV_REG_IMM32(RAX,pNode->GetOffset(),objContext);
		AppendRef(GetOffset() - 4);    //��̬����,��¼�ο���ַ 
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
| �������� : CGenerateSaxProgram::GenerateExprLogicalNotCode
| ��  �� : �������߼��Ǳ��ʽ
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateExprBitNotCode
| ��  �� : ����δȥȡ������
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateExprIncreaseCode
| ��  �� : ����++����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
				|| pParentExprNode->GetNodeType() == TNK_RETURN_STMT)  //ֻ�е�ǰ���ʽ�ĸ��ڵ��Ǳ��ʽʱ,���б�Ҫ�ƽ�ջ
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
			FLDVariable(pLeftExprNode,objContext);   //ע�����ε���FLDVariable���Ƕ����,�����һ��װ��Ҫ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprDecreaseCode
| ��  �� : ����--����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
			FLDVariable(pLeftExprNode,objContext);   //ע�����ε���FLDVariable���Ƕ����,�����һ��װ��Ҫ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprNegateCode
| ��  �� : ������(-)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}
}
/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::RequestReg
| ��  �� : ����һ���Ĵ���,����üĴ�������,�Ͱ����ƽ�ջ
| ��  �� : COMMON_REGISTERS_MASK nRegMask�����Ĵ�������
| ��  �� : BOOL bOther�����Ƿ���Բ����������мĴ���
| ��  �� : CContext& objContext���������Ķ���
| ��  �� : UINT& nStatus���������ƽ�ջ�Ĵ���������
| ��  �� : ������ļĴ������
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::FreeReg
| ��  �� : �ͷ�һ���Ĵ���,����üĴ������ƽ�ջ�ͳ�ջ
| ��  �� : COMMON_REGISTERS_MASK nRegMask�����Ĵ�������
| ��  �� : CContext& objContext���������Ķ���
| ��  �� : UINT nStatus���������ƽ�ջ�Ĵ���������,��RequestReg����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateExprMultCode
| ��  �� : ����˷�(��)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��ע�����Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
			{//pLeftNodeDataTypeΪ���������û��������
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
				IMUL(nFreeReg,objContext);   //�з��ų˷���EDX:EAX��EAX*r/m32
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
				else          //eg. LONLONG * LONGLONG,�ص����
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

	default: //�������������Ǳ��ʽ
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
| �������� : CGenerateSaxProgram::GenerateExprDivCode
| ��  �� : ����˷�(/)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
			SUB_IMM32(RSP,16,objContext);          //��ջ�з���ռ������洢���������� 

			GenerateExprStatementsCode(pRightExprNode,objContext);
			if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)  //eg. int * LONGLONG
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);         //�ѳ����ĵ�λ�ƶ���ջ��
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);         //�ѳ����ĸ�λ�ƶ���ջ��

			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);       //�ѱ������ĵ�λ�ƶ���ջ��
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);       //�ѱ������ĸ�λ�ƶ���ջ��

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
			UINT nL5Offset = GetOffset();        //����L5ƫ�Ƶ�ַ
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
			ADD_IMM32(RSP,16,objContext);  //�ͷſռ�

			FreeReg(RDI,objContext,nStatus);
			FreeReg(RSI,objContext,nStatus);
			FreeReg(RBX,objContext,nStatus);
			FreeReg(RCX,objContext,nStatus);
		}       
		break;

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprModCode
| ��  �� : ����ģ(%)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
		MOV(RAX,RDX,objContext);  //RDX�洢������
		break;   

	case TNK_LONGLONG_TYPE:
		{
			UINT nStatus = 0;

			const int CUR_BSP = objContext.GetOffset();
			SUB_IMM32(RSP,16,objContext);          //��ջ�з���ռ������洢���������� 

			GenerateExprStatementsCode(pRightExprNode,objContext);
			if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 8),RAX);         //�ѽ���ƶ���ջ��
			MOV_LIV32_REG(-(CUR_BSP + 4),RDX);         //�ѽ���ƶ���ջ��

			GenerateExprStatementsCode(pLeftExprNode,objContext);
			if(pLeftNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE) 
			{
				XOR(RDX,RDX,objContext);
				CDQ(objContext);
			}
			MOV_LIV32_REG(-(CUR_BSP + 16),RAX);       //�ѱ������ĵ�λ�ƶ���ջ��
			MOV_LIV32_REG(-(CUR_BSP + 12),RDX);       //�ѱ������ĸ�λ�ƶ���ջ��

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
			UINT nL5Offset = GetOffset();     //����L5ƫ�Ƶ�ַ
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
			ADD_IMM32(RSP,16,objContext);  //�ͷſռ�

			FreeReg(RDI,objContext,nStatus);
			FreeReg(RSI,objContext,nStatus);
			FreeReg(RBX,objContext,nStatus);
			FreeReg(RCX,objContext,nStatus);
		}       
		break;
	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprPlusCode
| ��  �� : ����ӷ�(+)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��ע:�Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprMinusCode
| ��  �� : �������(-)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ����ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprSHLCode
| ��  �� : ��������(<<)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}


/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprSHRCode
| ��  �� : ��������(>>)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExpComparisonOpCode
| ��  �� : ����Ƚ�(>,>=,<,<=)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

		BOOL bIsUnsigned = pNode->IsUnsigned();   //�õ����ʽ�Ƿ����޷���

		//UINT nTrueRefPos = 0,nFalseRefPos1 = 0,nFalseRefPos2 = 0/*,nEndRefPos = 0*/;
		const UINT nOperator = pNode->GetNodeType();
		if(pLeftExprNode->IsNumericConstants() || pRightExprNode->IsNumericConstants())
		{
			LONGLONG nConstVlaue = 0;
			if(pLeftExprNode->IsNumericConstants())   //����Ĵ��벻�ܺϲ�
			{
				GenerateExprStatementsCode(pRightExprNode,objContext);
				if(pRightNodeDataType->GetNodeType() != TNK_LONGLONG_TYPE)
				{
					XOR(RDX,RDX,objContext);
				}
				nConstVlaue = pLeftExprNode->GetValue1();

				nJmpType = (JT_FALSE == nJmpType)? JT_TRUE:JT_FALSE;   //����������,Ҫ��Ľ��ҲҪ�ı�
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                     //�Ƚϵ�λ����
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //�Ƚϵ�λ����
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //�Ƚϵ�λ����
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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
				
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //�Ƚϵ�λ����
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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
				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //�Ƚϵ�λ����
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
				CMP_IMM32(RDX,nConstVlaue >> 32);                       //�Ƚϸ�λ
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

				CMP_IMM32(RAX,nConstVlaue & 0xffffffff);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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
				
				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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
				
				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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
				
				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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
				
				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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
				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
				CMP(RDX,RBX);                      //�Ƚϸ�λ
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

				CMP(RAX,RCX);                      //�Ƚϵ�λ����
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
| �������� : CGenerateSaxProgram::GenerateExprBitAndCode
| ��  �� : ����λ��(&)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ����ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprBitXORCode
| ��  �� : ����λ���(^)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ����ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExprBitORCode
| ��  �� : ����λ��(|)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
	default: //�������������Ǳ��ʽ
		ATLASSERT(FALSE);
	}   
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExpAndOpCode
| ��  �� : �����߼���(&&)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	JMP_TYPE nJmpType = JT_FALSE;  //ֻ����һ����"||"����Ҫ����JT_TRUE��ת
	if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR
		&& ((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)
	{
		nJmpType = JT_TRUE;
	}

	pConditionExprJmpRef->SetJmpType(nJmpType);
	GenerateExprStatementsCode(pRightExprNode,objContext);


	if(((CExpressionTreeNode*)pParentNode)->GetChildNode(0) == pNode)   //ֻ����һ����"||"���ҵ�ǰ����ʽ����߽ڵ�
	{
		if(pParentNode->GetNodeType() == TNK_TRUTH_OR_EXPR )
		{
			pConditionExprJmpRef->WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext,nStartPos);
		}	
	}
	
}


/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateExpOrOpCode
| ��  �� : �����߼���(||)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	JMP_TYPE nJmpType = JT_FALSE;  //ֻ����һ����"||"���㲢�Ҳ�������Ҫ����JT_TRUE��ת
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
| �������� : CGenerateSaxProgram::GenerateExpQuestionOpCode
| ��  �� : �����ʺ�(?)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : ����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateExpCommaOpCode
| ��  �� : ������(,)����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateExprStatementsCode
| ��  �� : ������ʽ���,���ʽ��ֵ��������ν��ŵ�RAX�Ĵ�����
|       ʵ�ͷ��ڸ������Ĵ�����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

			if(NULL != pParent && (pParent->IsArithmeticExpression()  //����Ƚ������ڱ�����������������,�ͷ���EAX
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

			if(NULL != pParent && (pParent->IsArithmeticExpression()  //����Ƚ������ڱ�����������������,�ͷ���EAX
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

	//�����Ǵ����������ʽ��Ϊ�������ʽ
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
			FLDZ();                     //��+0.0ѹջ����װ��ST(0)
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
| �������� : CGenerateSaxProgram::GenerateIFSTMTCode
| ��  �� : ����IF..ELSE���
| ��  �� : CExpressionTreeNode* pNode����IF���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| ��  ע : �Ѿ�����
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
		JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128*/,objContext); 
		nEndRefPos = GetOffset();
	}
	
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	//else:
	if(pExpressionNode->GetChildNode(2) != NULL)
	{		
		GenerateCompoundStatementsCode((CExpressionTreeNode*)(pExpressionNode->GetChildNode(2)),objContext);
		//End:
		WriteDwordData(nEndRefPos - 4/*��Ե�ַ����*/,(BYTE)(GetOffset() - nEndRefPos),objContext); 
	}

	objContext.SetConditionExprJmpRef(NULL);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateForSTMTCode
| ��  �� : ����Forѭ�����,ѭ������������ͳһ����,���ٵ��ദ��
| ��  �� : CExpressionTreeNode* pNode����IF���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateForSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel();
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	
	CSyntaxTreeNode* pThridPart = pExpressionNode->GetChildNode(2);
	if(pThridPart)
	{
		JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128*/,objContext);     //����:cmp
	}


	pSTMTBindingLevel->SetStartPos(GetOffset());          // ��ʼλ�ò���������������������תָ��
	GenerateExprStatementsCode(pThridPart,objContext);    //����������ֱ��ʽ

	//cmp:
	if(pThridPart)
	{
		WriteDwordData(pSTMTBindingLevel->GetStartPos() - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,GetOffset() - pSTMTBindingLevel->GetStartPos(),objContext); 
	}

	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	CSyntaxTreeNode* pConditon = pExpressionNode->GetChildNode(1);
	GenerateExprStatementsCode(pConditon,objContext); //�����������ʽ

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	GenerateCompoundStatementsCode(((CExpressionTreeNode*)pExpressionNode->GetChildNode(3)),objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext);

	//End:
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();	
	objContext.SetConditionExprJmpRef(NULL);
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateWhileSTMTCode
| ��  �� : ����whileѭ�����
| ��  �� : CExpressionTreeNode* pNode����IF���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateWhileSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel(GetOffset());
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	//start:
	CConditionExprJmpRef objConditionExprJmpRef;
	objContext.SetConditionExprJmpRef(&objConditionExprJmpRef);

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext); //�����������ʽ

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	GenerateCompoundStatementsCode(((CExpressionTreeNode*)pExpressionNode->GetChildNode(1)),objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext); //����"start"λ��

	//End:
	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();
}



/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateDoSTMTCode
| ��  �� : ����Do..whileѭ�����
| ��  �� : CExpressionTreeNode* pNode����do���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext); //�����������ʽ

	objConditionExprJmpRef.WriteRealAddr(JT_TRUE,GetOffset(),this,&objContext);
	JMP_IMM32(pSTMTBindingLevel->GetStartPos() - GetOffset(),objContext);

	objConditionExprJmpRef.WriteRealAddr(JT_FALSE,GetOffset(),this,&objContext);

	//End:
	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}

	objContext.PopSTMTBindingLevel();
}


/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateSwitchSTMTCode
| ��  �� : ����Do..whileѭ�����
| ��  �� : CExpressionTreeNode* pNode����do���ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateSwitchSTMTCode(CExpressionTreeNode* pExpressionNode
	,CContext& objContext)
{
	CSTMTBindingLevel* pSTMTBindingLevel = objContext.CreateSTMTBindingLevel(GetOffset());
	pSTMTBindingLevel->SetNodeType(pExpressionNode->GetNodeType());

	GenerateExprStatementsCode(pExpressionNode->GetChildNode(0),objContext);  //������ʽ

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
			JE(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
			pSTMTBindingLevel->AppendCaseRef(GetOffset());
			break;    

		case TNK_DEFAULT_LABEL:   
			if(pSTMTBindingLevel->GetCaseRef() > 0) //���default�ڵ�һ��λ�ý�������ת
			{
				JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
				pSTMTBindingLevel->AppendCaseRef(GetOffset());        
			}
			pSTMTBindingLevel->SetDefaultIndex(pSTMTBindingLevel->GetCaseRef());

			bDefaultLable = TRUE;
			break;
		}

		pChildNode = (CExpressionTreeNode*)pChildNode->GetChain();
	}

	if(!bDefaultLable)  //���û��"default"��Ҫ����һ����ĩβ����תָ��
	{
		JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
		pSTMTBindingLevel->AppendRefrence(GetOffset());
	}

	GenerateCompoundStatementsCode(pCompoundSTMTNode,objContext);

	//End:
	for(UINT i = 0;i < pSTMTBindingLevel->GetRefrences();i++)
	{
		WriteDwordData(pSTMTBindingLevel->GetRefenceOffset(i) - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,GetOffset() - pSTMTBindingLevel->GetRefenceOffset(i),objContext);
	}


	objContext.PopSTMTBindingLevel();
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateCompoundStatementsCode
| ��  �� : �����ɴ������������Ĵ����
| ��  �� : CExpressionTreeNode* pNode�������ʽ�ڵ�
| ��  �� : CContext& objContext���������Ķ���
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
				JMP_IMM32(pCurSTMTBindingLevel->GetStartPos() - GetOffset(),objContext); //ֱ���������Ŀ�ʼλ��
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
						JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
					}
				}
				else
				{
					JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
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

				WriteDwordData(pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex) - 4/*ƫ�Ƶ�ַ�ĳ���*/
					,GetOffset() - pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex),objContext); //��ʵ�ʵ�ַ��д����ת����

				pCurSTMTBindingLevel->IncCaseIndex();
			}
			break;

		case TNK_DEFAULT_LABEL:
			{
				CSTMTBindingLevel* pCurSTMTBindingLevel = objContext.GetCurSTMTBindingLevel();
				ATLASSERT(pCurSTMTBindingLevel);
				const UINT nIncCaseIndex = pCurSTMTBindingLevel->GetCaseIndex();
				if(nIncCaseIndex != 0)  //˵��"defauilt"û������ǰ��
				{
					WriteDwordData(pCurSTMTBindingLevel->GetCaseRefOffset(nIncCaseIndex) - 4/*ƫ�Ƶ�ַ�ĳ���*/
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
							MOV_LIV32_REG(-(objContext.GetOffset() + 8),RAX);  //�ѽ���ƶ���ջ��
							MOV_LIV32_REG(-(objContext.GetOffset() + 4),RDX);  //�ѽ���ƶ���ջ��
							FILD(-(objContext.GetOffset() + 8),BT_BIT64);      //װ�ص��������Ĵ�����
						}
						else if(pDataType2->IsInterger())
						{
							MOV_LIV32_REG(-(objContext.GetOffset() + 4),RAX);  //�ѽ���ƶ���ջ��
							FILD(-(objContext.GetOffset() + 4));               //װ�ص��������Ĵ�����      
						}

					}
				}

				if(!(pChildNode->GetParent()->GetNodeType() == TNK_FUNCTION_DECL
					&& NULL == pChildNode->GetChain()))  //���������һ��"return"��䲻��Ҫ������תָ��
				{
					CRefrenceInfo* pRetRefrenceInfo = objContext.GetRetRefrenceInfo(); 
					JMP_IMM32(TMP_ADDR_BYTE4/*�õ�ַ�����滻,��ΨһҪ���Ǵ���128,ȷ����ַ���ĸ��ֽ�*/,objContext);
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
| �������� : CGenerateSaxProgram::GenerateFunctionCode
| ��  �� : Ϊ������������
| ��  �� : CSyntaxTreeNode* pNode����
| �޸ļ�¼ : 2007-5-2 8:41:34  -huangdy-  ����
-----------------------------------------------------------------*/
void CGenerateSaxProgram::GenerateFunctionCode( CDeclarationTreeNode* pNode )
{
	ATLASSERT(pNode);

	pNode->SetOffset(GetOffset());

	m_pPrvPosWrited = m_pCurPosWrited;
	GenerateMasmCmd(";Function: %s\r\n",pNode->GetSymbol());

	const static UINT SP_BASE_OFFSET = 12;  //�������Ĵ���(RBX,RSI,TDI)��ֵ�ƽ���ջ,������12
	CContext objContext(SP_BASE_OFFSET);  
	PUSH(RBP);     
	MOV(RBP,RSP,objContext);
	PUSH(RBX);
	PUSH(RSI);
	PUSH(RDI);

	
	CSyntaxTreeNode* pFunctonBody = pNode->GetSavedTree(); 
	const int nLocalVarSize = SumLocalNonStaticVariable((CExpressionTreeNode*)pFunctonBody,objContext);  //ͳ��������������ľֲ��Ǿ�̬����
	if( nLocalVarSize > 0)
	{
		SUB_IMM32(RSP,nLocalVarSize,objContext);  //�ƶ�ջָ��,Ϊ�ֲ���������ռ�
	}

	GenerateCompoundStatementsCode((CExpressionTreeNode*)pFunctonBody,objContext);

	CRefrenceInfo* pRetRefrenceInfo = objContext.GetRetRefrenceInfo(); 
	for(UINT i = 0;i < pRetRefrenceInfo->GetRefrences();i++)
	{
		WriteDwordData(pRetRefrenceInfo->GetRefenceOffset(i) - 4/*ƫ�Ƶ�ַ�ĳ���*/
			,(GetOffset() - pRetRefrenceInfo->GetRefenceOffset(i)),objContext); 
	}

	if( nLocalVarSize > 0)
	{
		objContext.DecOffset(nLocalVarSize);
		ADD_IMM32(RSP,nLocalVarSize,objContext);  //�ƶ�ջָ��,�ͷ�������������ľֲ������ռ�
	}

	POP(RDI,objContext);
	POP(RSI,objContext);
	POP(RBX,objContext);
	MOV(RSP,RBP,objContext);
	POP(RBP,objContext);
	RET();
}

/*-----------------------------------------------------------------
| �������� : CAsmCodeGenerator::GenerateCodeSegment
| ��  �� : generate code segment
| �� �� ֵ : ���ش���εĳߴ�
| �޸ļ�¼ : 2007-4-27 10:12:57  -huangdy-  ����
-----------------------------------------------------------------*/
UINT CGenerateSaxProgram::GenerateCodeSegment(AX3_PROGRAM_HEADER* pProgramHeader,char* lpEntryFun)
{   
	UINT nResult = GetOffset(); 

	CSyntaxTreeNode* pSyntaxTree = m_pSyntaxTreeRoot;
	while(NULL != pSyntaxTree)
	{
		if(TNK_FUNCTION_DECL == pSyntaxTree->GetNodeType())  //��������
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
| �������� : CGenerateSaxProgram::GenerateStaticVariable
| ��  �� : Ϊȫ�ֱ��������ó�ʼֵ
| ��  �� : CDeclarationTreeNode* pDeclNode����������ı���
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
| �������� : CGenerateSaxProgram::GenerateStaticVariable
| ��  �� : ��ʼ��ȫ�ֱ���
| ��  �� : CDeclarationTreeNode* pDeclNode����������ı���
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
	if(NULL != pInitial)     //˵������Ҫ����ʼ��
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
	else         //����ȫ������Ϊ0
	{
		m_pSaxCCompileimpl->OutputErrMsg(FALSE, pDeclNode->GetLineIndex(),0,
			_T("'%s (%d)': uninitialized static variable used"),CString(pDeclNode->GetSymbol()),pDeclNode->GetUID());

		memset(pSpace,0,pDataTypeNode->GetSize());
	}

	return pDataTypeNode->GetSize();
}

/*-----------------------------------------------------------------
| �������� : CGenerateSaxProgram::GenerateTxtConstants
| ��  �� : ��������е����ֳ���,�����ַ�������,���ֳ���
| ��  �� : CExpressionTreeNode* pNode����������Ľڵ�
| �� �� ֵ : �������о�̬�����ĳߴ��С
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
			*(double*)pSpace = (double)pConstTreeNode->GetValue1();  //������Ҳת��Ϊ��������ʽ,��Ϊ����Ϊ����������׼����
			break;

		case TNK_REAL_CST:
			*(double*)pSpace = pConstTreeNode->GetValue2();
			break;

		case TNK_STRING_CST:
			memcpy(pSpace,(char*)(LPCSTR)pConstTreeNode->GetValue3(),pConstTreeNode->GetSize());
			break;
		}   

		nLocalVarSize += pConstTreeNode->GetSize();


		CConstTreeNode* pConstTreeNode1 = NULL;          //����Ĵ�������ͬ�ĳ�������ͬƫ��,�����ظ��洢
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
| �������� : CGenerateSaxProgram::GenerateLocalStaticVariable
| ��  �� : �������еľ�̬����
| ��  �� : CExpressionTreeNode* pNode����������Ľڵ�,Ӧ����TNK_COMPOUND_STMT
| �� �� ֵ : �������о�̬�����ĳߴ��С
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
						if(pDeclTreeNode->GetNodeType() == TNK_VAR_DECL)       //���������ǲ���Ҫ����ռ�
						{
							if(pDeclTreeNode->GetDeclAttribute() & NDA_STATIC_FLAG) //ֻ������̬����
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
| �������� : CGenerateSaxProgram::GenerateStaticData
| ��  �� : ����ȫ�ֱ����ͺ����еľ�̬����
| �� �� ֵ : ��������ȫ�ֱ����ĳߴ��С
| �޸ļ�¼ : 2007-4-27 16:16:50  -huangdy-  ����
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
		case TNK_FUNCTION_DECL:   //��������
			nResult += GenerateLocalStaticVariable((CExpressionTreeNode*)((CDeclarationTreeNode*)pSyntaxTree)->GetSavedTree());
			break;

		case TNK_DECL_STMT:    //��������
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
| �������� : CGenerateSaxProgram::SumNonStaticVarSize
| ��  ��   : generate local variables 
| ��  ��   : CDeclarationTreeNode* pNode���������ڵ�
| �� �� ֵ : ��ǰ��������ֵĲ������ĳߴ�(��������̬����) 
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
-----------------------------------------------------------------*/
int CGenerateSaxProgram::SumNonStaticVarSize( CDeclarationTreeNode* pNode,CContext& objContext)
{
	int nSize = 0; 
	ATLASSERT(pNode);

	CDataTypeTreeNode* pDataTypeNode = NULL;
	while(pNode)
	{
		if(pNode->GetNodeType() == TNK_VAR_DECL)       //���������ǲ���Ҫ����ռ�
		{
			if(pNode->GetReferences() != NULL)
			{
				if(pNode->GetDeclAttribute() ^ NDA_STATIC_FLAG)  //��̬��������Ҫ������ջ
				{
					pDataTypeNode = pNode->GetDataType();
					pNode->SetOffset(0-(objContext.GetOffset() + pDataTypeNode->GetSize()));  //�ֲ�������ջ�е�ƫ��,�Ǹ�����,������ֵ = [BP + offset]
					objContext.IncOffset(pDataTypeNode->GetSize());
					nSize += pDataTypeNode->GetSize();

					ATLTRACE(_T("'%s(%d)': allocated local variable (offset:-%xh,size:%d)\r\n")
						,CString(pNode->GetSymbol()),pNode->GetUID(),abs(pNode->GetOffset()),pDataTypeNode->GetSize());

					if(NULL != pNode->GetInitial())       //˵������Ҫ����ʼ��
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
| �������� : CGenerateSaxProgram::SumLocalNonStaticVariable
| ��  ��   : Sum size of function local variables
| ��  ��   : CDeclarationTreeNode* pNode���������ڵ�
| �� �� ֵ : ��ǰ���������ֵĲ������ĳߴ�
| �޸ļ�¼ : 2014-5-20 20:58:54  -huangdy-  ����
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
						if(pDeclTreeNode->GetNodeType() == TNK_VAR_DECL)       //���������ǲ���Ҫ����ռ�
						{
							if(pDeclTreeNode->GetDeclAttribute() ^ NDA_STATIC_FLAG) //ֻ������̬����
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
//;        |ȫ�ֱ���,���� |
//;        |�еľ�̬������ |
//;        |�ı�����    |
//;        |---------------|
//;        | ����     |
//;        |---------------|
//;        | ���¶�λ���� |
//;        |---------------|
//;        |����������   |
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
		pProgramHeader->nGlobleDataSize = GenerateStaticData();  //д�뾲̬����
		pProgramHeader->nCodeSize    = GenerateCodeSegment(pProgramHeader,lpEntryFun);  //д�����

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

		pProgramHeader->nRePostionEntries = m_arrReferences.size();     //д���ض�λ ��Ŀ
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
			if(TNK_FUNCTION_DECL == pSyntaxTree->GetNodeType())  //��������
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


