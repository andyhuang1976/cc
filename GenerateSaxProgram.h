/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/GenerateSaxProgram.h,v 1.15 2014/12/24 10:55:21 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include <vector>
#include ".\machinecontrol.h"
#include ".\saxccompileimpl.h"

const int DEFAULT_BINARY_CODE_BUFFER_LEN = 1024 * 2;
const int PARAMETERS_BASE_OFFSET     = 8;
const int DEFAULT_STACK_SIZE       = 1024 * 1;


typedef std::vector<INSTRUCTION*> BREAK_STATEMENT_VECTOR;

class CSyntaxTreeNode;
class CDeclarationTreeNode;
class CExpressionTreeNode;
class CSymbolTable;
class CSymbol;
class CSaxCCompileimpl;
class CTxtConstantsTreeNode;
class CGenerateSaxProgram;
class CContext;

/*���� �Ĵ������루����˵IDֵ��
�� 16��64λͨ�üĴ����ǣ� 0000 ~ 1111��Ҳ���ǣ�0 ~ 15
8��32λͨ�üĴ����ǣ� 000 ~ 111 Ҳ���ǣ�0 ~ 7
�� 6���μĴ����ı����ǣ�000 ~ 101 Ҳ���ǣ�0 ~ 5
�� MMX�Ĵ��������ǣ� 000 ~ 111 Ҳ���ǣ�0 ~ 7
�� 16��XMM�Ĵ��������ǣ� 0000 ~ 1111 Ҳ���ǣ�0 ~ 15
8��XMM�Ĵ��������ǣ�000 ~ 111 Ҳ���ǣ�0 ~ 7

��ν�Ĵ��������ǼĴ�����Ӧ�Ķ����Ʊ��룬��˳�������壬������ı��

RAX/ES/MMX0/XMM0 -> 0000
RCX/CS/MMX1/XMM1 -> 0001
RDX/SS/MMX2/XMM2 -> 0010
RBX/DS/MMX3/XMM3 -> 0011
RSP/FS/MMX4/XMM4  -> 0100
RBP/GS/MMX5/XMM5 -> 0101
RSI/MMX6/XMM6   -> 0110
RDI/MMX7/XMM7   -> 0111
R8/XMM8  -> 1000
R9/XMM9  -> 1001
R10/XMM10 -> 1010
R11/XMM11 -> 1011
R12/XMM12 -> 1100
R13/XMM13 -> 1101
R14/XMM14 -> 1110
R15/XMM15 -> 1111
*/

enum COMMON_REGISTERS
{
	RAX = 0,  //000
	RCX = 1,  //001
	RDX = 2,  //010
	RBX = 3,  //011
	RSP = 4,  //100
	RBP = 5,  //101
	RSI = 6,  //110
	RDI = 7,  //111
	R8  = 8,
	R9  = 9,
	R10 = 10,
	R11 = 11,
	R12 = 12,
	R13 = 13,
	R14 = 14,
	R15 = 15,

	CR_INVALID = 0xFF
};

enum COMMON_REGISTERS_MASK
{
	CRM_RAX = 1,  
	CRM_RCX = 2,  
	CRM_RDX = 4, 
	CRM_RBX = 8,  
	CRM_RSP = 16, 
	CRM_RBP = 32, 
	CRM_RSI = 64,  
	CRM_RDI = 128, 
	CRM_R8  = 256,
	CRM_R9  = 512,
	CRM_R10 = 1024,
	CRM_R11 = 2048,
	CRM_R12 = 4096,
	CRM_R13 = 80192,
	CRM_R14 = 160384,
	CRM_R15 = 320768
};

enum SEGMENT_REGISTERS
{
	CS = 0,
	SS = 1,
	DS = 2,
	ES = 3,
	FS = 4,
	GS = 5
};

#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AL 0
#define CL 1
#define DL 2
#define BL 3


/*.һ��ָ�������ѡ��ָ��ǰ׺(˳������),ָ��ǰ׺��Ϊ����,ÿһ�����һЩ�����ǰ׺��.
�����κ�ָ��,ǰ׺���Դ�������(��1,2,3,4)�����ѡ,�������ǲ����ִ���*/
enum INSTRUCTION_PREFIX    
{
	PREFIX_INVALID  = 0,    // ��Ч��ǰ׺
	/*��1:�������ظ�ǰ׺*/
	PREFIX_LOCK    = 0x0F,   // Lock
	PREFIX_REPNE   = 0xF2,   // �����ڴ�������I/Oָ��,Ҳ�ɱ�����ĳЩָ���ǿ����ǰ׺
	PREFIX_REP    = 0xF2,   // �����ڴ�������I/Oָ��,Ҳ�ɱ�����ĳЩָ���ǿ����ǰ׺

	/*��2-������ǰ׺,�γ�Խǰ׺�����ı�Ĭ�϶�Ѱַ��ͨ����ַѰַ�����ݶλ��߶�ջ�Σ��������
	��ָ��ǰ����϶γ�Խǰ׺���Ϳ��Է��ʵ��������ڵ�����*/
	PREFIX_CS     = 0x2E,
	PREFIX_SS     = 0x36,
	PREFIX_DS     = 0x3E,
	PREFIX_ES     = 0x26,
	PREFIX_FS     = 0x64,
	PREFIX_GS     = 0x65,

	/*��2-��֧��ʾ*/
	PREFIX_BRANCH_NOT_TAKEN = 0x2E, //��֧��������(������Jccָ����)
	PREFIX_BRANCH_TAKEN = 0x3E,   //��֧������(������Jccָ����)

	/*��3*/
	PREFIX_OPERAND_SIZE = 0x66,   //66H����������С����ǰ׺,Ҳ�ɱ�����ĳЩָ���ǿ����ǰ׺.��������С����ǰ׺���������16λ��32λ��������С���л�.��������һ����������Ĭ��ֵ,��ʹ�����ǰ׺��ѡ���Ĭ��ֵ

	/*��4*/
	PREFIX_ADDRESS_SIZE = 0x67   //��ַ�ߴ�����ǰ׺
};

enum MOD
{
	MODE00 = 0,    //�ṩ [base] ��ʽ�� memory Ѱַ
	MODE01,       //�ṩ [base + disp8] ��ʽ�� memory Ѱַ
	MODE10,       //�ṩ [base + disp32] ��ʽ�� memory Ѱַ
	MODE11       //�ṩ register Ѱַ��
};

enum BIT_TYPE
{
	BT_BIT32 = 0,
	BT_BIT64
};

enum JUM_INSTRUCTION_CODE     //��תָ���б�
{
	JIC_UNKNOWN = 0,
	JIC_JO = 0x70, 	//�����ת 	�� 	$70 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNO, 	//�������ת 	�� 	$71 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JB, 	//������ת 	�� 	$72 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNB, 	//��������ת 	�� 	$73 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JE,	    //�����ת 	�� 	$74 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNE, 	//������ת 	�� 	$75 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JBE, 	//��������ת 	�� 	$76 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JA, 	//������ת 	�� 	$77 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JS, 	//������ת 	�� 	$78 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNS, 	//�Ǹ���ת 	�� 	$79 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JP, 	//��ż��ת 	�� 	$7A 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNP, 	//����ż��ת 	�� 	$7B 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JL, 	//С����ת 	�� 	$7C 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNL, 	//��С����ת 	�� 	$7D 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JNG, 	//��������ת 	�� 	$7E 	�� 	�� 	�� 	�� 	10 	�� 	�� 	8086 	�� 	��
	JIC_JG, 	//������ת 	�� 	$7F 	�� 	�� 	�� 	��
};



typedef std::vector<UINT>            OFFSET_ARRAY;
typedef std::vector<UINT>::iterator  OFFSET_ARRAY_ITR;

class CRefrenceInfo
{
public:
	CRefrenceInfo()
	{
	}
public:
	void AppendRefrence(UINT nOffset)       { m_arrOffsets.push_back(nOffset); }
	UINT GetRefrences()                     { return m_arrOffsets.size();}
	UINT GetRefenceOffset(UINT nIndex)
	{
		if( nIndex < m_arrOffsets.size())
		{
			return m_arrOffsets[nIndex];
		}
		return 0;
	}
private:
	OFFSET_ARRAY       m_arrOffsets;        //���ü�¼,����һ��ƫ�Ƽ�¼
};

class CLableRefrenceInfo : public CRefrenceInfo
{
public:
	CLableRefrenceInfo(LPCSTR lpName)
		:m_strName(lpName)
	{
	}
private:
	DECLARE_MEMBER_AND_METHOD(CStringA,m_strName,Name);  //��ǩ������
};

typedef std::vector<CLableRefrenceInfo*>            LABLE_REFRENCE_INFO_ARRAY;
typedef std::vector<CLableRefrenceInfo*>::iterator  LABLE_REFRENCE_INFO_ARRAY_ITR;


class CSTMTBindingLevel : public CRefrenceInfo
{
public:
	CSTMTBindingLevel(UINT nStartPos = 0)
		: m_nStartPos(nStartPos)
		, m_nCaseIndex(0)
		, m_nDefaultIndex(-1)
	    , m_nNodeType(0)
	{
	}
public:
	void AppendCaseRef(UINT nOffset)       { m_arrCaseOffsets.push_back(nOffset); }
	UINT GetCaseRef()                      { return m_arrCaseOffsets.size();}
	UINT GetCaseRefOffset(UINT nIndex)
	{
		if( nIndex < m_arrCaseOffsets.size())
		{
			return m_arrCaseOffsets[nIndex];
		}
		return 0;
	}
	void IncCaseIndex()              { m_nCaseIndex++;}
private:
	DECLARE_MEMBER_AND_METHOD(UINT,m_nStartPos,StartPos);      
	OFFSET_ARRAY       m_arrCaseOffsets;            //���ü�¼,����һ��ƫ�Ƽ�¼
	DECLARE_MEMBER_AND_METHOD(UINT,m_nCaseIndex,CaseIndex);     
	DECLARE_MEMBER_AND_METHOD(int,m_nDefaultIndex,DefaultIndex); 
    DECLARE_MEMBER_AND_METHOD(UINT,m_nNodeType,NodeType);   //�ڵ�����
};

typedef std::vector<CSTMTBindingLevel*>            STMTBINDINGLEVEL_ARRAY;
typedef std::vector<CSTMTBindingLevel*>::iterator  STMTBINDINGLEVEL_ARRAY_ITR;

enum JMP_TYPE
{
    JT_FALSE = 0,
	JT_TRUE
};
class CConditionExprJmpRef
{
public:
	CConditionExprJmpRef() 
: m_nJmpType(JT_FALSE)
 {}
	~CConditionExprJmpRef() {}
public:
    void Reset();
    void AppendRefPos(JMP_TYPE nJmpType,UINT nRefPos);
    void WriteRealAddr(JMP_TYPE nJmpType,UINT nRealAddr,CGenerateSaxProgram* pSaxProgram,CContext* pContext,UINT nStartPos = 0);
private:
	DECLARE_MEMBER_AND_METHOD(JMP_TYPE,m_nJmpType,JmpType);
	OFFSET_ARRAY             m_arrTrueRefPos;
	OFFSET_ARRAY             m_arrFalseRefPos;
};

class CContext
{
public:
	CContext(int nOffset = 0);
	~CContext();
public:
	void IncOffset(int nValue) { m_nOffset += nValue;}
	void DecOffset(int nValue) { m_nOffset -= nValue;}
public:
	void AppendRegisterUsedMask(DWORD nRegisterUsedMask) {  m_nRegisterUsedMask |= nRegisterUsedMask;}
	void RemoveRegisterUsedMask(DWORD nRegisterUsedMask) {  m_nRegisterUsedMask ^= (nRegisterUsedMask);}
	COMMON_REGISTERS RequestReg(COMMON_REGISTERS_MASK nMask,BOOL bOther = TRUE);
	void ReserveReg(COMMON_REGISTERS nReg);
	void FreeReg(COMMON_REGISTERS nReg);
    void OnRegChanged(COMMON_REGISTERS nReg);
public:
	CLableRefrenceInfo* CreateLableRefrence(LPCSTR lpName);
	CLableRefrenceInfo* LookupLableRefrence(LPCSTR lpName);
	CSTMTBindingLevel* CreateSTMTBindingLevel(UINT nStartPos = 0);
	void PopSTMTBindingLevel();
	CRefrenceInfo* GetRetRefrenceInfo()  { return &m_arrRetRefrenceInfo; }
private:
	DECLARE_MEMBER_AND_METHOD(int,m_nOffset,Offset);                //��¼��ǰ��RSP��RBP֮���ƫ�Ƶľ���ֵ,������žֲ�����
	LABLE_REFRENCE_INFO_ARRAY m_arrLableRefrenceInfo;
	CRefrenceInfo             m_arrRetRefrenceInfo;                //���ص�ַ�ο���Ϣ
	STMTBINDINGLEVEL_ARRAY    m_arrSTMTBindingLevel;
	DECLARE_MEMBER_AND_METHOD(DWORD,m_nRegisterUsedMask,RegisterUsedMask);
	DECLARE_MEMBER_AND_METHOD(CSTMTBindingLevel*,m_pCurSTMTBindingLevel,CurSTMTBindingLevel);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pCurVar,CurVar);   //�Ĵ���RAX,RDX��ֵ����Ӧ�ı���
    DECLARE_MEMBER_AND_METHOD(CConditionExprJmpRef*,m_pConditionExprJmpRef,ConditionExprJmpRef);
};

class CGenerateSaxProgram
{
public:
	CGenerateSaxProgram(CSaxCCompileimpl* pSaxCCompileimpl);
	virtual ~CGenerateSaxProgram(void);
public:
	BYTE* GenerateProgram(CSyntaxTreeNode* pSyntaxTreeRoot,char* lpEntryFun,int nStackSize = DEFAULT_STACK_SIZE,AX3_COMPILE_MODE nMode = ACM_MODE_X86);
	void Reset();
	CStringA GetMasmCode()     { return m_strMasmCode; }

	void WriteDwordData(UINT nOffset,DWORD nData,CContext& objContext);
	void WriteByteData(UINT nOffset,BYTE nData,CContext& objContext);

protected:
	BOOL AllocatBinaryCodeBuffer(size_t pLength);
protected:
	UINT GetOffset()        { return (UINT)(m_pCurPosWrited - m_pCodeBuffer);}
	void MoveWritingPtr(int nLen)  { m_pCurPosWrited += nLen;}
	BYTE* RequestSpace(UINT nLen);
	void AppendRef(UINT nOffset);
protected:
	COMMON_REGISTERS RequestReg(COMMON_REGISTERS_MASK nRegMask,BOOL bOther,CContext& objContext,UINT& nStatus);
	void FreeReg(COMMON_REGISTERS nReg,CContext& objContext,UINT nStatus);

	void EmitInstruction(BYTE nOpCode);
	void EmitInstruction(BYTE nOpCode,BYTE nPara);
	void EmitInstruction(BYTE nOpCode,DWORD nPara);

	void EmitInstruction(BYTE nPreFix,BYTE nOpCode,BYTE nPara);
	void EmitInstruction(BYTE nPreFix,BYTE nOpCode,DWORD nPara);
	UINT EmitInstruction(BYTE nPreFix,BYTE nOpCode,BYTE nModRM,BYTE nSIB,int nDisplacement,int nIMM32,int nIMMBytes,BOOL bStatic);

	void PUSH(COMMON_REGISTERS nRegister);
	void PUSH(SEGMENT_REGISTERS nRegister);
	void PUSH_IMM32(DWORD nValue);
	void PUSH_M32(DWORD nAddr);

	void POP(COMMON_REGISTERS nRegister,CContext &objContext);
	void POP(SEGMENT_REGISTERS nRegister);
	void POP_M32(DWORD nAddr);

	void MOV(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext,MOD nMod = MODE11);

	void MOV_IV8_IMM(int nOffset,BOOL bStatic,int nValue);
	void MOV_IV16_IMM(int nOffset,BOOL bStatic,int nValue);
	void MOV_LIV32_IMM(int nOffset,BOOL bStatic,int nValue);

	void MOV_LIV32_OFFSET(int nRelativeDisplacement,DWORD nOffset);
	void MOV_GIV32_OFFSET(DWORD nDisplacement,DWORD nOffset);

	void MOV_LFV_IMM(int nRelativeDisplacement,DWORD nDisplacement,BIT_TYPE nBitType = BT_BIT32);
	void MOV_GFV_IMM(DWORD nDisplacement1,DWORD nDisplacement2,BIT_TYPE nBitType = BT_BIT32);

	void MOV_LIV8_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg);
	void MOV_GIV8_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg);

	void MOV_LIV16_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg);
	void MOV_GIV16_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg);

	void MOV_LIV32_REG(int nRelativeDisplacement,COMMON_REGISTERS nSrcReg);
	void MOV_GIV32_REG(DWORD nDisplacement,COMMON_REGISTERS nSrcReg);

	void MOV_REG_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);
	void MOV_REG_LIV32(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);
	void MOV_REG_GIV32(COMMON_REGISTERS nDstReg,DWORD nDisplacement,CContext &objContext);

	void MOVZX_REG_LIV16(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);
	void MOV_REG_LIV8(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);

	//�����Ǹ���������ָ��
	void FLD(DWORD nDisplacement,BIT_TYPE nBitType = BT_BIT32);
	void FLD(int nRelativeDisplacement,BIT_TYPE nBitType/* = BT_BIT32*/);

	void FSTP(int nOffset,BOOL bStatic,BIT_TYPE nBitType = BT_BIT32);

	void FST(DWORD nDisplacement,BIT_TYPE nBitType = BT_BIT32);
	void FADD_M64REAL(CSyntaxTreeNode* pValueNode);
	void FADDP(DWORD i);

	void FSUB_M64REAL(CSyntaxTreeNode* pValueNode);
	void FSUBR_M64REAL(CSyntaxTreeNode* pValueNode);
	void FSUBP(DWORD i);

	void FMUL_M64REAL(CSyntaxTreeNode* pValueNode);
	void FMULP(DWORD i);

	void FDIV_M64REAL(CSyntaxTreeNode* pValueNode);
	void FDIVR_M64REAL(CSyntaxTreeNode* pValueNode);
	void FDIVP(DWORD i);

	void FILD(int nRelativeDisplacement,BIT_TYPE nBitType = BT_BIT32);
	void FLD(int nIndex);

	void FSTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType = BT_BIT32);
    void FISTP_INDIRECT(COMMON_REGISTERS nDstReg,BIT_TYPE nBitType = BT_BIT32);
	void FSTP(int nIndex);
	void FCOMP_M64REAL(CSyntaxTreeNode* pValueNode);
	void FCOMP(DWORD i);
	void FCOMPP(DWORD i);

	void FLDZ();
	void FUCOMPP(int i = 1);
	void FUCOMP(int i);
	void FNSTSW(CContext &objContext);
	void FCHS();
	void FNSTCW(int nRelativeDisplacement);
	void FLDCW(int nRelativeDisplacement);
	void FISTP(int nRelativeDisplacement);
	void FIST(int nRelativeDisplacement,BIT_TYPE nBitType = BT_BIT32);
	//����������ָ�����


	void AND_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);
	void AND(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);

	void XOR_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);
	void XOR(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);

	void OR(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);
	void OR_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);

	void ADD(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);
	void ADD_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);
	void ADC_IMM32(COMMON_REGISTERS nDstReg,DWORD nValue,CContext &objContext);
	void INC(COMMON_REGISTERS nReg,CContext &objContext);

	void SUB(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);
	void SUB(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);
	void SUB_IMM32(COMMON_REGISTERS nDstReg,int nValue,CContext &objContext);
	void SBB(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);
	void SBB_IMM32(COMMON_REGISTERS nDstReg,DWORD nValue,CContext &objContext);
	void NEG(COMMON_REGISTERS nReg,CContext &objContext);
	void SBB(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);

	void NOT(COMMON_REGISTERS nReg,CContext &objContext);
	void IMUL(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);
	void IMUL(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,int nValue,CContext &objContext);
	void IMUL(COMMON_REGISTERS nReg,CContext &objContext);
	void MUL(COMMON_REGISTERS nReg,CContext &objContext);
	void MUL(int nRelativeDisplacement,CContext &objContext);  
	void DEC(COMMON_REGISTERS nReg,CContext &objContext);

	void CDQ(CContext &objContext);
	void DIV(COMMON_REGISTERS nReg,CContext &objContext);
	void IDIV(COMMON_REGISTERS nReg,CContext &objContext);
	void SHL(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext);
	void SHL(COMMON_REGISTERS nReg,CContext &objContext);
	void SHLD(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2,CContext &objContext);

	void SHR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext);
	void SHR(COMMON_REGISTERS nReg,CContext &objContext);
	void SHRD(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2,CContext &objContext);

	void RCR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext);

	void SAR(COMMON_REGISTERS nReg,CContext &objContext);
	void SAR(COMMON_REGISTERS nReg,BYTE nValue,CContext &objContext);

	void SETE(COMMON_REGISTERS nReg,CContext &objContext);  //���/��,SETE/SETZ are different mnemonics for the same instruction 
	void SETNE(COMMON_REGISTERS nReg,CContext &objContext);  //����,SETNE/SETNZ are different mnemonics for the same instruction 

	void SETG(COMMON_REGISTERS nReg,CContext &objContext);  //����,SETGE/SETNL are different mnemonics for the same instruction 
	void SETGE(COMMON_REGISTERS nReg,CContext &objContext);  //���ڻ����,SETGE/SETNL are different mnemonics for the same instruction 

	void SETL(COMMON_REGISTERS nReg,CContext &objContext);  //С��,SETL/SETNGE are different mnemonics for the same instruction 
	void SETLE(COMMON_REGISTERS nReg,CContext &objContext);  //С�ڻ����,SETLE/SETNG are different mnemonics for the same instruction 

	void TEST(COMMON_REGISTERS nReg,int nValue);
	void TEST_BIT8(COMMON_REGISTERS nReg,int nValue,BOOL nLow = FALSE);
	void TEST(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg);

	void LEA(COMMON_REGISTERS nDstReg,int nRelativeDisplacement,CContext &objContext);

	void CMP_IMM8(COMMON_REGISTERS nReg,BYTE nValue);
	void CMP_IMM32(COMMON_REGISTERS nReg,int nValue);
	void CMP(COMMON_REGISTERS nReg,int nRelativeDisplacement);
	void CMP(COMMON_REGISTERS nReg1,COMMON_REGISTERS nReg2);
	void CMP_BIT8(COMMON_REGISTERS nReg,int nValue,BOOL bLow = TRUE);

	void JMP_IMM32(int nRelativeDisplacement,CContext &objContext);
	void JMP_BASE(JUM_INSTRUCTION_CODE nInst1,int nRelativeDisplacement);
	void JE(int nRelativeDisplacement,CContext &objContext);     //JE/JZ are different mnemonics for the same instruction 
	void JNE(int nRelativeDisplacement,CContext &objContext);    //JNE/JNZ are different mnemonics for the same instruction 
	void JGE(int nRelativeDisplacement,CContext &objContext);    //JGE/JNL are different mnemonics for the same instruction 
	void JC(int nRelativeDisplacement,CContext &objContext);     //Functionally similar to JB and JNAE. Unsigned comparison. 
	void JA(int nRelativeDisplacement,CContext &objContext);     //JA/JNBE are different mnemonics for the same instruction 
	void JB(int nRelativeDisplacement,CContext &objContext);     //JB/JNAE are different mnemonics for the same instruction 
	void JBE(int nRelativeDisplacement,CContext &objContext);    //JBE/JNA are different mnemonics for the same instruction 
	void JNS(int nRelativeDisplacement,CContext &objContext);  
	void JAE(int nRelativeDisplacement,CContext &objContext);    //JAE/JNB are different mnemonics for the same instruction 
	void JG(int nRelativeDisplacement,CContext &objContext);     //JG/JNLE are different mnemonics for the same instruction 
	void JL(int nRelativeDisplacement,CContext &objContext);     //JL/JNGE are different mnemonics for the same instruction 
	void JP(int nRelativeDisplacement,CContext &objContext);     //JP/JPE are different mnemonics for the same instruction 
	void JNP(int nRelativeDisplacement,CContext &objContext);    //JNE/JPO are different mnemonics for the same instruction 
	void JNG(int nRelativeDisplacement,CContext &objContext);

	void XCHG(COMMON_REGISTERS nDstReg,COMMON_REGISTERS nSrcReg,CContext &objContext);

	void RET();
protected:
	void MovIntValtoReg(CSyntaxTreeNode* pValue,COMMON_REGISTERS nSrcReg,COMMON_REGISTERS nExtDstReg,CContext &objContext);
	void MovRegtoIntVal(CSyntaxTreeNode* pVariableNode,COMMON_REGISTERS nSrcReg,COMMON_REGISTERS nExtDstReg,CContext &objContext);

	void Offset(CSyntaxTreeNode* pVariableNode1,CSyntaxTreeNode* pVariableNode2);
	void SetIntVariable(CSyntaxTreeNode* pVariableNode,LONGLONG nValue,CContext& objContext);
	void FLDVariable(CSyntaxTreeNode* pValueNode,CContext& objContext);
	void FSTPVariable(CSyntaxTreeNode* pValueNode,CContext& objContext);
	void FSTVariable(CSyntaxTreeNode* pValueNode,CContext& objContext);
	//void INCVariable(CSyntaxTreeNode* pValueNode,CContext& objContext);
	void CompareIntVariable(CSyntaxTreeNode* pValueNode,LONGLONG nValue,CContext& objContext);


	void IntegerOperator(CSyntaxTreeNode* pLeftExprNode,CSyntaxTreeNode* pRightExprNode,CExpressionTreeNode* pParantExprNode,CContext& objContext);
	void FloatingCmpCommonCode(CSyntaxTreeNode* pParantExprNode,UINT nOperator,CContext& objContext);
	void FloatingOperator(CSyntaxTreeNode* pLeftExprNode,CSyntaxTreeNode* pRightExprNode,CExpressionTreeNode* pParantExprNode,CContext& objContext);

	UINT GenerateCodeSegment(AX3_PROGRAM_HEADER* pProgramHeader,char* lpEntryFun);
	UINT GenerateStaticData();
	void GenerateStaticVariableInit(BYTE* pSpace,UINT nDataType,CSyntaxTreeNode* pInitial);
	UINT GenerateStaticVariable(CDeclarationTreeNode* pDeclNode);
	void GenerateSimpleAssignmentOp( CDeclarationTreeNode* pVariableNode,CSyntaxTreeNode* pValue,CContext& objContext);
	UINT GenerateLocalStaticVariable(CExpressionTreeNode* pNode);
	UINT GenerateTxtConstants(CTxtConstantsTreeNode* pNode);
	void GenerateFunctionCode(CDeclarationTreeNode* pNode );
	void GenerateCompoundStatementsCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprAddrCode( CExpressionTreeNode* pNode,CContext& objContext);
	void ExtractFactorValue( CSyntaxTreeNode* pNode,CContext& objContext);
	void GenerateExprLogicalNotCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprBitNotCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprIncreaseCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprDecreaseCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprNegateCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprMultCode(CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprDivCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprModCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprPlusCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprMinusCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprSHLCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprSHRCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExpComparisonOpCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprBitAndCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprBitXORCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExprBitORCode( CExpressionTreeNode* pNode,CContext& objContext);

	void GenerateExpAndOpCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExpOrOpCode( CExpressionTreeNode* pNode,CContext& objContext);

	void GenerateExpQuestionOpCode( CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateExpCommaOpCode( CExpressionTreeNode* pNode,CContext& objContext);

	void GenerateExprStatementsCode(CSyntaxTreeNode* pNode,CContext& objContext);
	void GenerateLableSTMTCode(CDeclarationTreeNode* pNode,CContext& objContext);
	void GenerateGotoSTMTCode(CDeclarationTreeNode* pNode,CContext& objContext); 
	void GenerateCallSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);
	void GenerateIFSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);
	void GenerateForSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);
	void GenerateWhileSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);
	void GenerateDoSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);
	void GenerateSwitchSTMTCode(CExpressionTreeNode* pExpressionNode,CContext& objContext);

	int SumNonStaticVarSize( CDeclarationTreeNode* pNode,CContext& objContext);
	UINT SumLocalNonStaticVariable(CExpressionTreeNode* pNode,CContext& objContext);
	void GenerateMasmCmd(char* format, ... );
private:
	DECLARE_MEMBER_AND_METHOD(AX3_COMPILE_MODE,m_nMode,Mode);
	DECLARE_MEMBER_AND_METHOD(BOOL,m_bEnableMasm,EnableMasm);  //generator masm code
	int                      m_nLable;         // generator unique lable;
	BYTE*                    m_pCodeBuffer;
	size_t                   m_nCodeBufferLen;
	BYTE*                    m_pCurPosWrited;     //��ǰд��λ��
	BYTE*                    m_pPrvPosWrited;     //ǰһ��д��λ��   
	CSaxCCompileimpl*        m_pSaxCCompileimpl;
	CSyntaxTreeNode*         m_pSyntaxTreeRoot;
	CStringA                 m_strMasmCode;
	OFFSET_ARRAY             m_arrReferences;     //ȫ�ֱ���,��̬�����ͺ���Ӧ�ü�¼
};


/*ModRM �ֽڣ���Ϊ��mod-reg-r/m �� 2-3-3 ���������ֽڡ�
ModRM.mod ���ṩѰַģʽ�� ModRM.reg �����ṩ�Ĵ��� ID��ModRM.r/m �ṩ register �� memory �� ID */

inline BYTE ModRM(MOD nModRM_Mod,UINT nModRM_REG,UINT nModRM_RM)
{ 
	return BYTE((nModRM_Mod << 6) | (nModRM_REG << 3) | nModRM_RM);
}

inline BYTE SIB(UINT SACLE,UINT INDEX,UINT BASE)
{ 
	return BYTE((SACLE << 6) | (INDEX << 3) | BASE);
}

