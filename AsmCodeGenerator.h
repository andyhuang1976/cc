/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/AsmCodeGenerator.h,v 1.2 2014/04/11 14:33:31 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/

class CSyntaxTreeNode;
class CSaxCCompileimpl;

class CAsmCodeGenerator
{
public:
	CAsmCodeGenerator(CSaxCCompileimpl* pSaxCCompileimpl);
	~CAsmCodeGenerator();

// Operation
public:
	void			GenerateAsmCode(CSyntaxTreeNode* const pSyntaxTreeRoot);
	void            Reset();
// help routines
private:
	// code generation routines
	void			GenerateDataSegment( CSyntaxTreeNode* const pSyntaxTreeRoot);
	void			GenerateDataItem( CSyntaxTreeNode* pTmpSyntaxTreeNode );
	void			GenerateCodeSegment( CSyntaxTreeNode* const pSyntaxTreeRoot);
	void			GenerateFunctionCode(CSyntaxTreeNode* pNode );
	void			GenerateLocalVar( CSyntaxTreeNode* pNode );
	void			GenerateStatement( CSyntaxTreeNode* pTmpSyntaxTreeNode, int nLable1 = 0, int nLable2 = 0);
	BOOL			IsAddress( CSyntaxTreeNode* pNode );
	void			EmitCodeEx( char* format, ... );
	void            EmitCode( char* pCode);
	// 80x86 ASM routines
	void			_inline_readc();
	void			_readi();
	void			_writei();
private:
	int				         m_nLable;       // generator unique lable;	
	CSaxCCompileimpl*        m_pSaxCCompileimpl;
};

