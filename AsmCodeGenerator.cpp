/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/AsmCodeGenerator.cpp,v 1.4 2014/05/07 13:03:05 administrator Exp $
* 
*******************************************************************************
* 
* Description:implementation file
******************************************************************************/

static const char _CppVersion[] = "@(#)  $Header: /cvsdata/vc/SaxCCompile/AsmCodeGenerator.cpp,v 1.4 2014/05/07 13:03:05 administrator Exp $";
#include "stdafx.h"
// $Nokeywords: $
#include "AsmCodeGenerator.h"
#include "parser.h"
#include <strsafe.h>
#include ".\saxccompileimpl.h"
#include <exception> 

using namespace std;  

class CEmitCodeException:public exception  
{  
public:
	CEmitCodeException(char* pErrMsg)
		: m_strErrMsg(pErrMsg)
	{
	}
public:  
   const char* what()const throw()  
   {  
        return m_strErrMsg;  
   }
private:
	CStringA    m_strErrMsg;
};

#pragma warning(disable : 4995)


const UINT SIZE256  = 256;
const UINT SIZE512  = 512;
const UINT SIZE1024 = 1024;

CAsmCodeGenerator::CAsmCodeGenerator(CSaxCCompileimpl* pSaxCCompileimpl)
: m_nLable(1)
, m_pSaxCCompileimpl(pSaxCCompileimpl)
{
}

CAsmCodeGenerator::~CAsmCodeGenerator()
{
}

void CAsmCodeGenerator::Reset()
{
	m_nLable = 1;
}

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateAsmCode
|  描    述  : 
|  参    数  : CSyntaxTreeNode* pSyntaxTreeRoot——
|              char* pOutBuffer——
|              int nBufferLen——
|  返 回 值  : 
|  修改记录  : 2007-4-27 9:40:49   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateAsmCode(CSyntaxTreeNode* const pSyntaxTreeRoot)
{
	try
	{
		if(NULL != pSyntaxTreeRoot)
		{
			// generating pTmpSyntaxTreeNode1-code
			GenerateDataSegment(pSyntaxTreeRoot);
			GenerateCodeSegment(pSyntaxTreeRoot);
		}
	}
	catch(CEmitCodeException& EmitCodeException)  
	{  
		ATLTRACE(_T("%s"),CString(EmitCodeException.what()));
	}  

}

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::EmitCode
|  描    述  : write codes to buffer, which must exist
|  参    数  :  char* format——
|              ...——
|  返 回 值  : 
|  修改记录  : 2007-4-27 9:59:36   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::EmitCodeEx( char* format, ... )
{
	va_list params;	
	va_start( params, format );

	if(!m_pSaxCCompileimpl->OutputCode((char*)format,params))
	{
		throw CEmitCodeException(("Emit code exception!"));  
	}

	va_end( params );
}
void CAsmCodeGenerator::EmitCode( char* pCode)
{
	if(!m_pSaxCCompileimpl->OutputCode((BYTE*)pCode,strlen(pCode)))
	{
		throw CEmitCodeException(("Emit code exception!"));  
	}
}

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::_inline_readc
|  描    述  :  read a character, use AX register, result is stored in AL
|  返 回 值  : 
|  修改记录  : 2007-4-27 10:53:15   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::_inline_readc()
{
	char pTemp[] =
		"\t mov \t ah, 1\r\n"
		"\t int \t21h\r\n"
		"\t mov \t ah, 0\r\n";
	EmitCode( pTemp );
}

// read a integer
// need:
// data segment
// buf		db	64 dup(?)
// count	dw	?
void CAsmCodeGenerator::_readi()
{
	char pTemp[] = ";----------------------------------------------------\r\n"
		"; GENERAL PRUPOSE ROUTINES\r\n"
		";----------------------------------------------------\r\n"
		"; FOR READI\r\n"
		";----------------------------------------------------\r\n"
		";input a decimal, result in AX\r\n"
		"_call_readi_	proc	near pascal uses bx cx dx si\r\n"
		"\t mov \t count, 0\r\n"
		"readi@next:\r\n"
		"\t mov \t ah, 7\r\n"
		"\t int \t 21h\r\n"
		"\t cmp \t al, '-'\r\n"
		"\t je \t readi@onCHAR\r\n"
		"\t cmp \t al, '+'\r\n"
		"\t je \t readi@onCHAR\r\n"
		"\t cmp \t al, 08h	;DEL\r\n"
		"\t je \t readi@onDEL\r\n"
		"\t cmp \t al, 0dh	;CR\r\n"
		"\t je \t readi@onCR\r\n"
		"\t cmp \t al, '0'\r\n"
		"\t jb \t readi@next\r\n"
		"\t cmp \t al, '9'\r\n"
		"\t ja \t readi@next\r\n"
		" \t readi@onCHAR:\r\n"
		"\t cmp \t count, 63\r\n"
		"\t jae \t readi@next\r\n"
		";process the key input, and show it on screen\r\n"
		"\t mov \t si, count\r\n"
		"\t inc \t count\r\n"
		"\t mov \t buf[si], al\r\n"
		"\t mov \t dl, al\r\n"
		"\t mov \t ah, 02h\r\n"
		"\t int \t 21h\r\n"
		"\t jmp \t readi@next\r\n"
		"readi@onDEL:\r\n"
		"\t call \t keydel\r\n"
		"\t jmp \t readi@next\r\n"
		"readi@onCR:\r\n"
		"\t mov \t ah, 2\r\n"
		"\t mov \t dl, 0ah\r\n"
		"\t int \t 21h\r\n"
		"\t mov \t dl, 0dh\r\n"
		"\t int \t 21h\r\n"
		"\t mov \t si, count\r\n"
		"\t mov \t buf[si], 0\r\n"
		"\t call \t str2dec\r\n"
		"\t ret\r\n"
		"_call_readi_	 \t endp\r\n"
		";----------------------------------------------------\r\n"
		";set the cursor in DX\r\n"
		"set_cur \t proc	near pascal uses ax bx\r\n"
		"\t mov \t bh, 0\r\n"
		"\t mov \t ah, 02h\r\n"
		"\t int \t 10h\r\n"
		"\t ret\r\n"
		"set_cur \t endp\r\n"
		";----------------------------------------------------\r\n"
		";read current cursor to DX\r\n"
		"read_cur	proc	near pascal uses ax bx cx\r\n"
		"\t mov \t bh, 0\r\n"
		"\t mov \t ah, 03h\r\n"
		"\t int \t 10h\r\n"
		"\t ret\r\n"
		"read_cur  \t endp\r\n"
		";----------------------------------------------------\r\n"
		";procedure for key DEL\r\n"
		"keydel \t proc	near pascal uses bx cx dx\r\n"
		"\t cmp \t count, 0\r\n"
		"\t jle \t keydel@ret\r\n"
		"\t dec \t count\r\n"
		"\t call \t read_cur\r\n"
		"\t dec \t dl\r\n"
		"\t call \t set_cur\r\n"
		"\t mov \t bh, 0\r\n"
		"\t mov \t al, 20h\r\n"
		"\t mov \t cx, 1\r\n"
		"\t mov \t ah, 0ah\r\n"
		"\t int \t 10h\r\n"
		"keydel@ret:	\r\n"
		"\t ret\r\n"
		"keydel	 \t endp\r\n"
		";----------------------------------------------------\r\n"
		";result in AX\r\n"
		"str2dec \t proc	near pascal uses bx cx dx si\r\n"
		"	pLocal	minus:byte\r\n"
		";init\r\n"
		"\t mov \t minus, 0\r\n"
		"\t mov \t ax, 0\r\n"
		"\t mov \t bx, 10\r\n"
		"\t mov \t cx, 0\r\n"
		";\r\n"
		"\t cmp \t count, 0\r\n"
		"\t je \t str2dec@ret\r\n"
		"\t mov \t si, 0\r\n"
		"\t cmp \t buf[si], '-'\r\n"
		"\t je \t str2dec@onMINUS\r\n"
		"\t cmp \t buf[si], '+'\r\n"
		"\t je \t str2dec@onPLUS\r\n"
		"str2dec@next:\r\n"
		"\t cmp \t buf[si], '0'\r\n"
		"\t jb \t str2dec@complete\r\n"
		"\t cmp \t buf[si], '9'\r\n"
		"\t ja \t str2dec@complete\r\n"
		"\t mul \t bx\r\n"
		"\t mov \t cx, ax\r\n"
		"\t mov \t al, buf[si]\r\n"
		"\t sub \t al, '0'\r\n"
		"\t mov \t ah, 0\r\n"
		"\t add	ax, cx\r\n"
		"\t inc \t si\r\n"
		"\t jmp \t str2dec@next\r\n"
		"str2dec@onMINUS:\r\n"
		"\t mov \t minus, 1\r\n"
		"\t inc \t si\r\n"
		"\t jmp \t str2dec@next\r\n"
		"str2dec@onPLUS:\r\n"
		"\t inc \t si\r\n"
		"\t jmp \t str2dec@next\r\n"
		"str2dec@complete:\r\n"
		"\t cmp \t minus, 1\r\n"
		"\t jne \t str2dec@ret\r\n"
		"\t mov \t cx, ax\r\n"
		"\t mov \t ax, 0\r\n"
		"\t sub \t ax, cx\r\n"
		"str2dec@ret:\r\n"
		"\t ret\r\n"
		"str2dec \t endp\r\n";
	EmitCode( pTemp );
}

// print the integer in BX
void CAsmCodeGenerator::_writei()
{
	char pTemp[] = ";----------------------------------------------------\r\n"
		"; FOR WRITEI\r\n"
		";----------------------------------------------------\r\n"
		";show decimal in BX\r\n"
		"_call_showi_	proc	near pascal uses ax cx dx\r\n"
		"	pLocal	can_show:byte\r\n"
		"\t mov \t can_show, 0\r\n"
		"\t cmp \t bx, 0\r\n"
		"\t jge \t show@show\r\n"
		"\t mov \t ax, bx\r\n"
		"\t mov \t bx, 0\r\n"
		"\t sub \t bx, ax\r\n"
		"\t mov \t ah, 2\r\n"
		"\t mov \t dl, '-'\r\n"
		"\t int \t 21h\r\n"
		"show@show:\r\n"
		"\t mov \t cx, 10000d\r\n"
		"\t cmp \t bx, cx\r\n"
		"\t jb \t show@1\r\n"
		"\t mov \t can_show, 1\r\n"
		"\t call \t dec_div\r\n"
		"show@1:\r\n"
		"\t mov \t cx, 1000d\r\n"
		"\t cmp \t can_show, 1\r\n"
		"\t je \t show@2\r\n"
		"\t cmp \t bx, cx\r\n"
		"\t jb \t show@3\r\n"
		"\t mov \t can_show, 1\r\n"
		"show@2:\r\n"
		"\t call \t dec_div\r\n"
		"show@3:\r\n"
		"\t mov \t cx, 100d\r\n"
		"\t cmp \t can_show, 1\r\n"
		"\t je \t show@4\r\n"
		"\t cmp \t bx, cx\r\n"
		"\t jb \t show@5\r\n"
		"\t mov \t can_show, 1\r\n"
		"show@4:\r\n"
		"\t call \t dec_div\r\n"
		"show@5:\r\n"
		"\t mov \t cx, 10d\r\n"
		"\t cmp \t can_show, 1\r\n"
		"\t je \t show@6\r\n"
		"\t cmp \t bx, cx\r\n"
		"\t jb \t show@7\r\n"
		"\t mov \t can_show, 1\r\n"
		"show@6:\r\n"
		"\t call \t dec_div\r\n"
		"show@7:\r\n"
		"\t mov \t cx, 1d\r\n"
		"\t call \t dec_div\r\n"
		"\t ret\r\n"
		"_call_showi_	 \t endp\r\n"
		";----------------------------------------------------\r\n"
		"dec_div \t proc	near pascal uses ax\r\n"
		"\t mov \t ax, bx\r\n"
		"\t mov \t dx, 0\r\n"
		"	div \t cx\r\n"
		"\t mov \t bx, dx\r\n"
		"\t mov \t dl, al\r\n"
		"	add \t dl, 30h\r\n"
		"\t mov \t ah, 2\r\n"
		"\t int \t 21h\r\n"
		"dec_div@ret:\r\n"
		"\t ret\r\n"
		"dec_div	 \t endp\r\n";
	EmitCode( pTemp );
}


/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateDataSegment
|  描    述  : generate data segment for global data and data in _main()
|  参    数  : CSyntaxTreeNode* pSyntaxTreeRoot——
|  返 回 值  : 
|  修改记录  : 2007-4-27 9:44:55   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateDataSegment( CSyntaxTreeNode* const pSyntaxTreeRoot)
{
	CSyntaxTreeNode *pTmpSyntaxTreeNode1 = NULL;
	CSyntaxTreeNode *pTmpSyntaxTreeNode2 = NULL;

	// add variables used in default routines
	char* pTemp =   "	.model	small\r\n"
		"	.386\r\n"
		"	.stack	200h\r\n"
		";----------------------------------------------------\r\n"
		"	.data\r\n"
		";variables used in _call_readi_ routine\r\n"
		"buf		db	64 dup(?)\r\n"
		"count	dw	?\r\n"
		";return values\r\n"
		"_return	dw	?\r\n"
		";global and _main variables\r\n";
	EmitCode( pTemp );

	// add global and _main variables
	pTmpSyntaxTreeNode1 = pSyntaxTreeRoot;
	while(NULL == pTmpSyntaxTreeNode1 ) 
	{
		if( pTmpSyntaxTreeNode1->GetNodeType() == TNK_VAR_DECL )
		{
			GenerateDataItem( pTmpSyntaxTreeNode1 );
		}
		else 
		{
			CIdentifierTreeNode* pIdentifier = pTmpSyntaxTreeNode1->GetName();
			if(NULL != pIdentifier)
			{
				if( strcmp(pIdentifier->GetTitle(),"main") == 0 ) 
				{
					pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode1->GetChildNode(0);   // main(...) parameters, common none
					while(NULL != pTmpSyntaxTreeNode2 ) 
					{
						GenerateDataItem( pTmpSyntaxTreeNode2 );
						pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
					}

					pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode1->GetChildNode(1);    // pLocal variables in _main
					while(NULL != pTmpSyntaxTreeNode2 ) 
					{
						if( pTmpSyntaxTreeNode2->GetNodeType() == TNK_VAR_DECL )
						{
							GenerateDataItem( pTmpSyntaxTreeNode2 );
						}
						pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
					}
				}
			}
		}
		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
	}
	// end of data segment

}

// 

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateDataItem
|  描    述  : generate an item in data segment for node pTmpSyntaxTreeNode1
|              all types use the same type "DW"
|  参    数  : CSyntaxTreeNode* pTmpSyntaxTreeNode——
|  返 回 值  : 
|  修改记录  : 2007-4-27 10:05:02   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateDataItem( CSyntaxTreeNode* pTmpSyntaxTreeNode )
{
	char pTemp1[SIZE1024] = {"?"};
	char pTemp2[SIZE1024] = {0};

	if( pTmpSyntaxTreeNode->GetExtraDataType() == EDT_ARRAY)
	{
		StringCchPrintfA(pTemp1,SIZE1024,"%d dup(?)\r\n", pTmpSyntaxTreeNode->GetArraySize());
	}
	StringCchPrintfA(pTemp2,SIZE1024,"%s@%s\tdw\t%s\r\n", pTmpSyntaxTreeNode->GetScope(),
		pTmpSyntaxTreeNode->GetName(), pTemp1 );

	EmitCode( pTemp2 );
}

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateCodeSegment
|  描    述  : generate code segment
|  参    数  : const CSyntaxTreeNode* pSyntaxTreeRoot——
|  返 回 值  : 
|  修改记录  : 2007-4-27 10:12:57   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateCodeSegment( CSyntaxTreeNode* const pSyntaxTreeRoot)
{
	char pTemp[SIZE512] = ";----------------------------------------------------\r\n"
		"	.code\r\n"
		";_MAIN PROC\r\n"
		"start\tproc	far\r\n"
		"\t mov \t ax, @data\r\n"
		"\t mov \t ds, ax\r\n"
		";\r\n";
	EmitCode( pTemp );

	CSyntaxTreeNode* pTmpSyntaxTreeNode1 = pSyntaxTreeRoot;
	while( pTmpSyntaxTreeNode1 )                               //First,Process the main function.
	{
		if( pTmpSyntaxTreeNode1->GetNodeType() == TNK_FUNCTION_DECL 
			&& strcmp(pTmpSyntaxTreeNode1->GetName(),"main") == 0 )
		{
			GenerateFunctionCode( pTmpSyntaxTreeNode1 );
			break;
		}
		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
	}

	pTmpSyntaxTreeNode1 = pSyntaxTreeRoot;                   //Second,Process other fucntion.
	while( pTmpSyntaxTreeNode1 ) 
	{
		if( pTmpSyntaxTreeNode1->GetNodeType() == TNK_FUNCTION_DECL 
			&& strcmp(pTmpSyntaxTreeNode1->GetName(),"main") != 0  )
		{			
			memset(pTemp,0,SIZE512);
			StringCchPrintfA(pTemp,SIZE512,";----------------------------------------------------\r\n"
				"%s	proc	near pascal uses ax bx cx dx\r\n", 
				(LPCTSTR)pTmpSyntaxTreeNode1->GetName());// for parameters

			EmitCode( pTemp );
			GenerateFunctionCode( pTmpSyntaxTreeNode1 );
		}
		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
	}
	// add general purpose routines
	_readi();
	_writei();
	// end of code segment

	EmitCode(  ";----------------------------------------------------\r\n"
		"	end 	start\r\n" );
}

/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateFunctionCode
|  描    述  : generate codes for one function
|  参    数  : CSyntaxTreeNode* pNode——
|  返 回 值  : 
|  修改记录  : 2007-4-27 10:18:39   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateFunctionCode( CSyntaxTreeNode* pNode )
{	
	CSyntaxTreeNode* pTmpSyntaxTreeNode1 = pNode->GetChildNode(0);
	if(strcmp(pNode->GetName(),"main") != 0 )
	{
		GenerateLocalVar( pNode );
	}

	pTmpSyntaxTreeNode1 = pNode->GetChildNode(1);
	while( pTmpSyntaxTreeNode1 )
	{
		GenerateStatement( pTmpSyntaxTreeNode1 );
		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
	}

	char pTemp[SIZE256] = {0};
	if(strcmp(pNode->GetName() ,"main") == 0 )
	{
		StringCchPrintfA(pTemp,SIZE256, "\t mov \t ax, 4c00h\r\n"
			"\t int \t 21h\r\n"
			"start \t endp\r\n" );
	}
	else
	{
		StringCchPrintfA(pTemp,SIZE256, "\t mov \t ax, 4c00h\r\n"
			"\t int \t 21h\r\n"
			"start		 \t endp\r\n",
			pNode->GetName());		
	}

	EmitCode( pTemp );
}

// generate pLocal variables
void CAsmCodeGenerator::GenerateLocalVar( CSyntaxTreeNode* pNode )
{
	BOOL bFirst       = TRUE;

	char pLocal[SIZE1024] = {0};
	char pTemp[SIZE512]   = {0};
	char pTemp2[SIZE512]  = {0};

	CSyntaxTreeNode* pTmpSyntaxTreeNode1 = pNode->GetChildNode(0);      // parameters
	while(NULL != pTmpSyntaxTreeNode1 )
	{
		StringCchPrintfA(pTemp,SIZE512,", %s@%s:word\r\n", pTmpSyntaxTreeNode1->GetScope(),
			pTmpSyntaxTreeNode1->GetName(), pTemp );	
		EmitCode( pTemp );

		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
		memset(pTemp,0,SIZE512);
	}
	EmitCode( "\r\n" );

	pTmpSyntaxTreeNode1 = pNode->GetChildNode(1);// make pLocal variables
	while( pTmpSyntaxTreeNode1 ) 
	{
		if( pTmpSyntaxTreeNode1->GetNodeType() == TNK_VAR_DECL )
		{
			if( pTmpSyntaxTreeNode1->GetExtraDataType() == EDT_ARRAY) 
			{
				StringCchPrintfA(pTemp2 , SIZE512 , "%s@%s[%d]:word", 
					pTmpSyntaxTreeNode1->GetScope(),
					pTmpSyntaxTreeNode1->GetName(),
					pTmpSyntaxTreeNode1->GetArraySize() );	
			}
			else 
			{
				StringCchPrintfA(pTemp2 , SIZE512 ,"%s@%s[%d]:word", 
					pTmpSyntaxTreeNode1->GetScope(),
					pTmpSyntaxTreeNode1->GetName() );	
			}

			if( bFirst )
			{
				bFirst = FALSE;
				StringCchPrintfA(pTemp , SIZE512 ,  "\tlocal\t%s", pTemp2);	
			} 
			else
			{
				StringCchPrintfA(pTemp , SIZE512 ,  ", %s", pTemp2);	
			}
			strncat_s(pLocal,pTemp,strlen(pTemp));
			memset(pTemp,0,SIZE512);
			memset(pTemp2,0,SIZE512);
		}

		pTmpSyntaxTreeNode1 = pTmpSyntaxTreeNode1->GetChain();
	}

	if( !bFirst ) 
	{
		strncat_s(pLocal,"\r\n",2);
		EmitCode( pLocal );
	}
}


/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::GenerateStatement
|  描    述  : 
|  参    数  : const CSyntaxTreeNode* pTmpSyntaxTreeNode——
|              int nLable1——
|              int nLable2——
|  返 回 值  : 
|  修改记录  : 2007-4-27 10:40:56   -huangdy-   创建
-----------------------------------------------------------------*/
void CAsmCodeGenerator::GenerateStatement( CSyntaxTreeNode* pTmpSyntaxTreeNode
										  , int nLable1, int nLable2 )
{
	CSyntaxTreeNode* pTmpSyntaxTreeNode2 = NULL;
	size_t i;
    int _lab1 = 0, _lab2 = 0;
	char szTemp[SIZE1024] = {0};

	if( NULL == pTmpSyntaxTreeNode)
	{
		return;
	}
	switch( pTmpSyntaxTreeNode->GetNodeType()) 
	{
	case TNK_STATEMENT:
		switch( pTmpSyntaxTreeNode->GetStatementKink()) 
		{
		case TNK_READ_STMT:
			EmitCode( ";read statement\r\n" );
			// read in a char or a num
			if( pTmpSyntaxTreeNode->GetTokenType() == TT_CHAR ) 
			{
				_inline_readc();
			}
			else
			{
				EmitCode( "\tcall\t_call_readi_\r\n" );
			}
			// store
			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
			if( pTmpSyntaxTreeNode2->GetExtraDataType() == EDT_ARRAY) 
			{
				EmitCode( "\t push \t ax\r\n" );
				if( IsAddress(pTmpSyntaxTreeNode2) )
				{
					EmitCodeEx( "\t mov \t bx, %s@%s\r\n",
						pTmpSyntaxTreeNode2->GetScope(),
						pTmpSyntaxTreeNode2->GetName());
				}
				else 
				{
					EmitCodeEx( "\tlea \t bx, %s@%s\r\n", 
						pTmpSyntaxTreeNode2->GetScope(),
						pTmpSyntaxTreeNode2->GetName());
				}
				EmitCode( "\t push \t bx\r\n" );
				GenerateStatement( pTmpSyntaxTreeNode2->GetChildNode(0));
				EmitCode( "\t pop \t si\r\n" );
				EmitCode( "\t add \t si, si\r\n" );// all use "WORD"
				EmitCode( "\t pop \t bx\r\n" );
				EmitCode( "\t pop \t ax\r\n" );
				EmitCode( "\t mov \t [bx + si], ax\r\n" );
			} 
			else
			{
				EmitCodeEx( "\t mov \t %s@%s, ax\r\n", 
					pTmpSyntaxTreeNode2->GetScope(),
					pTmpSyntaxTreeNode2->GetName());
			}
			break;

		case TNK_WRITE_STMT:
			EmitCode( ";write statement\r\n" );
			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
			if( pTmpSyntaxTreeNode->GetTokenType() == TT_CHAR 
				|| pTmpSyntaxTreeNode->GetTokenType() == TT_CHAR_TYPE )
			{
				if( pTmpSyntaxTreeNode2->GetNodeType() == TNK_EXPRESSION 
					&& pTmpSyntaxTreeNode2->GetStatementKink() == EK_CONST )
				{
					char* pName =  pTmpSyntaxTreeNode2->GetName();
					EmitCode( "\t mov \t ah, 2\r\n" );
					EmitCodeEx( "\t mov \t dl, '%c'\r\n",pName[0] );
				} 
				else 
				{
					GenerateStatement( pTmpSyntaxTreeNode2 );
					EmitCode( "\t pop \t dx\r\n" );
					EmitCode( "\t mov \t ah, 2\r\n" );
				}
				EmitCode( "\t int \t 21h\r\n" );
			} 
			else 
			{
				GenerateStatement( pTmpSyntaxTreeNode2 );
				EmitCode( "\t pop \t bx\r\n" );
				EmitCode( "\t call \t _call_showi_\r\n" );
			}
			break;

		case SK_PRINTF:
			{
				EmitCode( ";printf statement\r\n" );
				EmitCode( "\t push \t ax\r\n" );
				EmitCode( "\t push \t dx\r\n" );
				EmitCode( "\t mov \t ah, 2\r\n" );

				char* pName = pTmpSyntaxTreeNode->GetName();
				for( i = 0; i < strlen(pName); i++ ) 
				{
					if( pName[i] == '\n' )
					{
						EmitCode( "\t mov \t dl, 00ah\r\n"
							"\t int \t 21h\r\n"
							"\t mov \t dl, 00dh\r\n"
							"\t int \t 21h\r\n" );
					} 
					else
					{
						EmitCodeEx( "\t mov \t dl, %.3xh\r\n", (int)pName[i] );
						EmitCode( "\t int \t 21h\r\n" );
					}
				}
				EmitCode( "\t pop \t dx\r\n" );
				EmitCode( "\t pop \t ax\r\n" );
				EmitCode( ";end of printf statement\r\n" );
			}
			break;

		case TNK_LABEL_STMT:
			EmitCodeEx( ";lable %s@%s\r\n", 
				pTmpSyntaxTreeNode2->GetScope(),
				pTmpSyntaxTreeNode2->GetName());
			EmitCodeEx( "%s@%s:\r\n", 
				pTmpSyntaxTreeNode2->GetScope(),
				pTmpSyntaxTreeNode2->GetName());
			break;

		case TNK_GOTO_STMT:
			EmitCodeEx( ";goto %s@%s\r\n", 
				pTmpSyntaxTreeNode2->GetScope(),
				pTmpSyntaxTreeNode2->GetName());
			EmitCodeEx( "\t jmp \t %s@%s\r\n", 
				pTmpSyntaxTreeNode2->GetScope(),
				pTmpSyntaxTreeNode2->GetName());
			break;

		case SK_IF:
			EmitCode( ";start of if statement\r\n" );
			EmitCode( ";if conditions\r\n" );

			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
			GenerateStatement( pTmpSyntaxTreeNode2 );
			EmitCode( "\t pop \t ax\r\n" );
			EmitCode( "\t cmp \t ax, 0\r\n" );
			_lab1 = m_nLable++;
			EmitCodeEx( "\t je  \t L%d\r\n", _lab1 );
			EmitCode( ";if statements\r\n" );

			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(1);
			while( pTmpSyntaxTreeNode2 ) 
			{
				GenerateStatement( pTmpSyntaxTreeNode2, nLable1, nLable2 );
				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
			}

			if( pTmpSyntaxTreeNode->GetChildNode(2))
			{
				_lab2 = m_nLable++;
				EmitCodeEx( "\t jmp \t L%d\r\n", _lab2 );
			}

			EmitCodeEx( "L%d:\r\n", _lab1 );
			if( pTmpSyntaxTreeNode->GetChildNode(2) ) 
			{
				EmitCode( ";else statements\r\n" );

				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(2);
				while( pTmpSyntaxTreeNode2 )
				{
					GenerateStatement( pTmpSyntaxTreeNode2, nLable1, nLable2 ); 
					pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
				}
				EmitCodeEx( "L%d:\r\n", _lab2 );
			}
			EmitCode( ";end of if statement\r\n" );
			break;

		case SK_WHILE:
			EmitCode( ";start of while statement\r\n" );
			_lab1 = m_nLable++;
			EmitCodeEx( "L%d:\r\n", _lab1 );
			EmitCode( ";while conditions\r\n" );

			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
			GenerateStatement( pTmpSyntaxTreeNode2 );
			EmitCode( "\t pop \t ax\r\n" );
			EmitCode( "\t cmp \t ax, 0\r\n" );
			_lab2 = m_nLable++;
			EmitCodeEx( "\t je \t L%d\r\n", _lab2 );

			EmitCode( ";while statements\r\n" );
			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(1);
			while( pTmpSyntaxTreeNode2 ) 
			{
				GenerateStatement( pTmpSyntaxTreeNode2, _lab1, _lab2 );
				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
			}
			EmitCodeEx( "\t jmp \t L%d\r\n", _lab1 );
			EmitCodeEx( "L%d:\r\n", _lab2 );
			EmitCode( ";end of while statement\r\n" );
			break;

		case TNK_BREAK_STMT:
			EmitCode( ";break statement\r\n" );
			EmitCodeEx( "\t jmp \t L%d", nLable2 );
			break;

		case TNK_CONTINUE_STMT:
			EmitCode( ";continue statement\r\n" );
			EmitCodeEx( "\t jmp \t L%d\r\n", nLable1 );
			break;

		case TNK_RETURN_STMT:
			EmitCode( ";return statement\r\n" );
			if( pTmpSyntaxTreeNode->GetChildNode(0) 
				&& strcmp(pTmpSyntaxTreeNode->GetName() , "main") != 0 )
			{
				GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(0));
				EmitCode( "\t pop \t ax\r\n" );
				EmitCode( "\t mov \t _return, ax\r\n" );
			}

			if( pTmpSyntaxTreeNode->GetChain() ||
				(pTmpSyntaxTreeNode->GetFatherNode() 
				&& pTmpSyntaxTreeNode->GetFatherNode()->GetNodeType() != TNK_FUNCTION_DECL) )
			{
				EmitCode( "\t ret" );
			}
			break;

		case TNK_CALL_EXPR:
			EmitCodeEx( ";call '%s(...)'\r\n", pTmpSyntaxTreeNode->GetName());
			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
			while( pTmpSyntaxTreeNode2 ) 
			{
				GenerateStatement( pTmpSyntaxTreeNode2 ); 
				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChain();
			}

			EmitCodeEx( "\t call \t %s\r\n", (LPCTSTR)pTmpSyntaxTreeNode->GetName());
			if( pTmpSyntaxTreeNode->GetTokenType() != TNK_CALL_EXPR ) 
			{
				EmitCode( "\t mov \t ax, _return\r\n" );
				EmitCode( "\t push \t ax\r\n" );
			}
			break;
		}
		break;

	case TNK_EXPRESSION:
		switch( pTmpSyntaxTreeNode->GetExpressionKind() ) 
		{
		case EK_CONST:
			if( pTmpSyntaxTreeNode->GetTokenType() == TT_CHAR_TYPE ) 
			{
				char* pName = pTmpSyntaxTreeNode->GetName();
				EmitCodeEx( "\t mov \t al, '%c'\r\n", pName[0] );
				EmitCode( "\t mov \t ah, 0\r\n" );
			}
			else
			{
				EmitCodeEx( "\t mov \t ax, %s\r\n", pTmpSyntaxTreeNode->GetName() );
			}
			EmitCode( "\t push \t ax\r\n" );
			break;

		case EK_IDENTIFY:
			if( pTmpSyntaxTreeNode->GetExtraDataType() == EDT_ARRAY) 
			{
				if( IsAddress(pTmpSyntaxTreeNode) )
				{
					EmitCodeEx( "\t mov \t bx, %s@%s\r\n", 
						pTmpSyntaxTreeNode->GetScope(),
						pTmpSyntaxTreeNode->GetName());
				}
				else
				{
					EmitCodeEx( "\t lea \t bx, %s@%s\r\n",
						pTmpSyntaxTreeNode->GetScope(),
						pTmpSyntaxTreeNode->GetName());
				}

				if( pTmpSyntaxTreeNode->GetFatherNode() 
					&& pTmpSyntaxTreeNode->GetFatherNode()->GetNodeType() == TNK_STATEMENT 
					&& pTmpSyntaxTreeNode->GetFatherNode()->GetStatementKink() == TNK_CALL_EXPR )
				{
					EmitCode( "\t mov \t ax, bx\r\n" );// passing its base-address to the call function is OK
				}
				else 
				{
					GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(0) );
					EmitCode( "\t pop \t si\r\n" );
					EmitCode( "\t add \t si, si\r\n" );// all use "WORD"
					EmitCode( "\t mov \t ax, [bx + si]\r\n" );
				}
			} 
			else
			{
				EmitCodeEx( "\t mov \t ax, %s@%s\r\n", 
					pTmpSyntaxTreeNode->GetScope(),
					pTmpSyntaxTreeNode->GetName());
			}
			EmitCode( "\t push \t ax" );
			break;

		case EK_OPERATION:
			if( strcmp(pTmpSyntaxTreeNode->GetName(), "=") == 0 ) 
			{
				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetFatherNode();
				if( pTmpSyntaxTreeNode2 
					&& strcmp(pTmpSyntaxTreeNode2->GetName() , "=") == 0 
					&& pTmpSyntaxTreeNode2->GetChildNode(0) == pTmpSyntaxTreeNode )
				{
					break; // continuous ASSIGN the same ID
				}

				EmitCode( ";\r\n" );

				GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(1));
				// process the left-hand of "="
				pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetChildNode(0);
				while( pTmpSyntaxTreeNode2 
					&& (pTmpSyntaxTreeNode2->GetNodeType() != TNK_EXPRESSION 
					|| pTmpSyntaxTreeNode2->GetExpressionKind() != EK_IDENTIFY) )
				{
					pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode2->GetChildNode(0);
				}

				if( pTmpSyntaxTreeNode2->GetExtraDataType() == EDT_ARRAY ) 
				{
					if( IsAddress(pTmpSyntaxTreeNode2) )
					{
						EmitCodeEx( "\t mov \t bx, %s@%s\r\n",
							pTmpSyntaxTreeNode2->GetScope(),
							pTmpSyntaxTreeNode2->GetName());
					}
					else
					{
						EmitCodeEx( "\t lea \t bx, %s@%s\r\n", 
							pTmpSyntaxTreeNode2->GetScope(),
							pTmpSyntaxTreeNode2->GetName());
					}

					GenerateStatement( pTmpSyntaxTreeNode2->GetChildNode(0));
					EmitCode( "\t pop \t si\r\n" );
					EmitCode( "\t add \t si, si\r\n" );
					EmitCode( "\t add \t bx, si\r\n" );
					EmitCode( "\t pop \t ax\r\n" );
					EmitCode( "\t mov \t [bx], ax\r\n" );
				} 
				else
				{
					EmitCode( "\t pop \t ax\r\n" );
					EmitCodeEx( "\t mov \t %s@%s, ax\r\n", 
						pTmpSyntaxTreeNode2->GetScope(),
						pTmpSyntaxTreeNode2->GetName());
				}
			}
			else if( strcmp(pTmpSyntaxTreeNode->GetName(), "!") == 0 ) 
			{
				GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(0));
				_lab1 = m_nLable++; 
				_lab2 = m_nLable++;

				memset(szTemp,0,SIZE512);
				StringCchPrintfA(szTemp	, SIZE512
					, "\t pop \t ax\r\n"
					"\t cmp \t ax, 0\r\n"
					"\t je \t __not@@%d\r\n"
					"\t mov \t ax, 0\r\n"
					"\t jmp \t __not@@%d\r\n"
					"__not@@%d:\r\n"
					"\t mov \t ax, 1\r\n"
					"__not@@%d:\r\n", 
					_lab1, _lab2, _lab1, _lab2 );
				EmitCode( szTemp );
			} 
			else 
			{
				// binary operations
				GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(0));
				GenerateStatement( pTmpSyntaxTreeNode->GetChildNode(1));

				EmitCode( "pop \tbx" );
				EmitCode( "pop \tax" );

				if( strcmp(pTmpSyntaxTreeNode->GetName(), "==") == 0 ) 
				{
					_lab1 = m_nLable++; 
					_lab2 = m_nLable++;
					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						, "\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t je \t __eq@@%d\r\n"
						"\t mov \t ax, 0\r\n"
						"\t jmp \t __eq@@%d\r\n"
						"__eq@@%d:\r\n"
						"\t mov \t ax, 1\r\n"
						"__eq@@%d:\r\n", 
						_lab1, _lab2, _lab1, _lab2 );

					EmitCode( szTemp );
				} 
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "!=") == 0 ) 
				{
					_lab1 = m_nLable++;

					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						,"\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t je \t __neq@@%d\r\n"
						"\t mov \t ax, 1\r\n"
						"__neq@@%d:\r\n", _lab1, _lab1);					

					EmitCode( szTemp );
				} 
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "<") == 0 ) 
				{
					_lab1 = m_nLable++;
					_lab2 = m_nLable++;

					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						,"\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t jl \t __lt@@%d\r\n"
						"\t mov \t ax, 0\r\n"
						"\t jmp \t __lt@@%d\r\n"
						"__lt@@%d:\r\n"
						"\t mov \t ax, 1\r\n"
						"__lt@@%d:\r\n", _lab1, _lab2, _lab1, _lab2 );		

					EmitCode( szTemp );
				} 
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "<=") == 0 ) 
				{
					_lab1 = m_nLable++;
					_lab2 = m_nLable++;

					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						,"\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t jle \t __ngt@@%d\r\n"
						"\t mov \t ax, 0\r\n"
						"\t jmp \t __ngt@@%d\r\n"
						"__ngt@@%d:\r\n"
						"\t mov \t ax, 1\r\n"
						"__ngt@@%d:\r\n", _lab1, _lab2, _lab1, _lab2);		

					EmitCode( szTemp );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), ">=") == 0 ) 
				{
					_lab1 = m_nLable++; 
					_lab2 = m_nLable++;

					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						,"\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t jge \t __nlt@@%d\r\n"
						"\t mov \t ax, 0\r\n"
						"\t jmp \t __nlt@@%d\r\n"
						"__nlt@@%d:\r\n"
						"\t mov \t ax, 1\r\n"
						"__nlt@@%d:\r\n", _lab1, _lab2, _lab1, _lab2 );	

					EmitCode( szTemp );
				} 
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), ">") == 0 ) 
				{
					_lab1 = m_nLable++; 
					_lab2 = m_nLable++;

					memset(szTemp,0,SIZE512);
					StringCchPrintfA(szTemp	, SIZE512
						,"\t sub \t ax, bx\r\n"
						"\t cmp \t ax, 0\r\n"
						"\t jg \t __gt@@%d\r\n"
						"\t mov \t ax, 0\r\n"
						"\t jmp \t __gt@@%d\r\n"
						"__gt@@%d:\r\n"
						"\t mov \t ax, 1\r\n"
						"__gt@@%d:\r\n", _lab1, _lab2, _lab1, _lab2 );
					
					EmitCode( szTemp );
				} 
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "+") == 0 ) 
				{
					EmitCode( "\t add \t ax, bx\r\n" );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "-") == 0 ) 
				{
					EmitCode( "\t sub \t ax, bx\r\n" );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "*") == 0 ) 
				{
					EmitCode( "\t imul \t bx\r\n" );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "/") == 0 ) 
				{
					EmitCode( "\t idiv \t bx\r\n" );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "&&") == 0 ) 
				{
					EmitCode( "\t and \t ax, bx\r\n" );
				}
				else if( strcmp(pTmpSyntaxTreeNode->GetName(), "||") == 0 ) 
				{
					EmitCode( "\t or  \t ax, bx\r\n" );
				}
			}
			// situations
			pTmpSyntaxTreeNode2 = pTmpSyntaxTreeNode->GetFatherNode();
			if( pTmpSyntaxTreeNode2 
				&& (pTmpSyntaxTreeNode2->GetNodeType() == TNK_EXPRESSION 
				|| (pTmpSyntaxTreeNode2->GetNodeType() == TNK_STATEMENT
				&& pTmpSyntaxTreeNode == pTmpSyntaxTreeNode2->GetChildNode(0)) ))
			{
				EmitCode( "\t push \t ax\r\n" );
			}
			break;
		}
		break;
	}
}
/*-----------------------------------------------------------------
|  函数名称  : CAsmCodeGenerator::IsAddress
|  描    述  : in this simple situation, pNode->szName is a parameter passed as array address
|  参    数  : CSyntaxTreeNode* pNode——
|  返 回 值  : TRUE----Yes
|              FALSE---No
|  修改记录  : 2007-4-27 10:55:44   -huangdy-   创建
-----------------------------------------------------------------*/
BOOL CAsmCodeGenerator::IsAddress( CSyntaxTreeNode* pNode )
{
	if( !pNode || pNode->GetNodeType() != TNK_EXPRESSION 
		|| pNode->GetStatementKink() != EK_IDENTIFY )
	{
		return FALSE;
	}

	CSyntaxTreeNode* pTmpSyntaxTreeNode = pNode->GetFatherNode();
	while( pTmpSyntaxTreeNode
		&& pTmpSyntaxTreeNode->GetNodeType() != TNK_FUNCTION_DECL )
	{
		pTmpSyntaxTreeNode = pTmpSyntaxTreeNode->GetFatherNode();
	}

	pTmpSyntaxTreeNode = pTmpSyntaxTreeNode->GetChildNode(0); 
	while( pTmpSyntaxTreeNode )
	{
		if( pTmpSyntaxTreeNode->GetExtraDataType() == EDT_ARRAY 
			&& strcmp(pTmpSyntaxTreeNode->GetName(), pNode->GetName()) == 0) 
		{
			return TRUE;
		}

		pTmpSyntaxTreeNode = pTmpSyntaxTreeNode->GetChain();
	}

	return FALSE;
}
