/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/Scaner.h,v 1.10 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:C lexical analysis class define
******************************************************************************/
#pragma once
#include <hash_map>
#include "global.h"
#include <vector>

class CSaxCCompileimpl;

const int SYMBOL_MAX_LENGTH = 512;
const int END_OF_SOURCE_CODE = -1;
//const int TOKEN_TYPE_COUNT = 69;

enum AXC_SYMBOL_TYPE
{
   TT_UNKNOWN   = 0,

   TT_INT     = 200, 
   TT_DOUBLE    = 201,
   TT_LONG     = 202,
   TT_CHAR     = 203,
   TT_FLOAT    = 204, 
   TT_SHORT    = 205,
   TT_BYTE     = 206,
   TT_BOOL     = 207,
   TT_VOID     = 208,
   TT_WORD     = 209,
   TT_DWORD    = 210,
   TT_LONGLONG   = 211,
   TT_MAX_DATA_TYPE = 255,

   // reserved Keyword
   TT_AUTO     = 256, 
   TT_UNSIGNED   = 257,
   TT_STRUCT    = 259,
   TT_BREAK    = 260, 
   TT_ELSE     = 261,   
   TT_SWITCH    = 263, 
   TT_CASE     = 264, 
   TT_ENUM     = 265, 
   TT_REGISTER   = 266,
   TT_TYPEDEF   = 267,   
   TT_EXTERN    = 269,
   TT_RETURN    = 270,
   TT_UNION    = 271,
   TT_CONST    = 272,   
   TT_CONTINUE   = 276,
   TT_FOR     = 277, 
   TT_SIGNED    = 278,
   
   TT_DEFAULT   = 280, 
   TT_GOTO     = 281, 
   TT_SIZEOF    = 282, 
   TT_VOLATILE   = 283,
   TT_DO      = 284,
   TT_IF      = 285, 
   TT_STATIC    = 286, 
   TT_WHILE    = 287,
   //TT_READ     = 289, 
   //TT_WRITE    = 290, 
   //TT_PRINTF    = 291,   
   TT_TRUE     = 293,
   TT_FALSE    = 294,

   // operations
   TT_ASSIGN    = 400,
   TT_PLUS     = 401, 
   TT_MINUS    = 402, 
   TT_MULT    = 403,
   TT_DIV     = 404,
   TT_MOD     = 405,
   TT_BITWISE_AND = 406, 
   TT_BITWISE_OR  = 407,
   TT_BITWISE_XOR = 408,
   TT_BITWISE_NOT = 409, 
   TT_LOGICAL_NOT = 410, 
   TT_LT      = 411, 
   TT_GT      = 412,
   TT_QUESTION_MARK = 413,

   // interpunctions
   TT_LPARAN    = 500,
   TT_RPARAN    = 501, 
   TT_LBRACE    = 502, 
   TT_RBRACE    = 503, 
   TT_LSQUARE   = 504,
   TT_RSQUARE   = 505, 
   TT_COMMA    = 506, 
   TT_DOT     = 507,
   TT_SEMI     = 508,
   TT_COLON    = 509,

   // complex operations
   TT_EQ      = 600,    /* == */
   TT_NEQ     = 601,    /* != */
   TT_PLUS_PLUS  = 602,    /* ++ */
   TT_MINUS_MINUS = 603,    /* -- */
   TT_PLUS_ASSIGN = 604,    /* += */
   TT_MINUS_ASSIGN = 605,    /* -= */ 
   TT_MULT_ASSIGN = 606,    /* *= */ 
   TT_DIV_ASSIGN  = 607,    /* /= */
   TT_AND_ASSIGN  = 608,    /* &= */
   TT_XOR_ASSIGN  = 609,    /* ^= */
   TT_OR_ASSIGN  = 610,    /* |= */
  TT_MOD_ASSIGN  = 611,    /* %= */
   TT_NGT     = 612,    /* <= */
   TT_NLT     = 613,    /* >= */
   TT_LOGICAL_AND = 614,    /* && */
   TT_LOGICAL_OR  = 615,    /* || */
   TT_LSHIFT    = 616,    /* << */
   TT_RSHIFT    = 617,    /* >> */
   TT_LSHIFT_ASSIGN = 618,    /* <<= */
   TT_RSHIFT_ASSIGN = 619,    /* >>= */

   TT_MAX_RESERVEDKEYWORD_ID = 699,
   // others
   TT_IDENTIFY_TYPE= 700,   
   TT_INTEGER_TYPE = 701,
   TT_REAL_TYPE  = 702,
   TT_STRING_TYPE = 703,
  TT_CHAR_TYPE  = 704,
   TT_ERROR_TYPE  = 705,
   TT_NULL_TYPE  = 706,
   TT_EOF_TYPE   = 707,
   TT_KEY_WORDS_TYPE  = 708
};

enum SCAN_STATE_TYPE
{
   SST_START   = 1,
   SST_INCOMMENT1 = 2,
   SST_INCOMMENT2 = 3,
   SST_INIDENTIFY = 4,
   SST_INNUMERAL = 5,
   SST_INSTRING  = 6,
   SST_INCHAR   = 7,
   SST_DONE    = 8
};

class CSymbolInfo
{
public:
   CSymbolInfo(AXC_SYMBOL_TYPE nType = TT_ERROR_TYPE,char* pSymbol = NULL,UINT nLineIndex = 0)
     : m_nType(nType)
     , m_strSymbol(pSymbol)
     , m_nNodeType(0)
     , m_nLineIndex(nLineIndex)
   {
   }
   ~CSymbolInfo()
   {
   }

   CSymbolInfo& operator=(CSymbolInfo& objTokenInfo)
   {
     m_nType    = objTokenInfo.m_nType;
     m_strSymbol  = objTokenInfo.m_strSymbol;
     m_nLineIndex = objTokenInfo.m_nLineIndex;
     m_nNodeType  = objTokenInfo.m_nNodeType;
     return *this;
   }
public:
   char* GetSymbolTitle()        { return (char*)(LPCSTR)m_strSymbol; }
private:
   DECLARE_MEMBER_AND_METHOD(AXC_SYMBOL_TYPE,m_nType,Type);
   DECLARE_MEMBER_AND_METHOD(UINT,m_nNodeType,NodeType);
   DECLARE_MEMBER_AND_METHOD(CStringA,m_strSymbol,Symbol);
   DECLARE_MEMBER_AND_METHOD(UINT,m_nLineIndex,LineIndex);
};


template<class _Kty>
class CReservedKeywordCompare :public std::hash_compare<_Kty>
{
public:
   BOOL operator()(const _Kty& _Keyval1, const _Kty& _Keyval2) const
   {
     BOOL bResult = TRUE;
     if(strcmp(_Keyval1,_Keyval2) == 0)
     {
        bResult = FALSE;        
     }     
   
     return bResult;
   }   
   size_t operator()(const _Kty& _Keyval) const
   {        
     size_t nHash = 0; 
     for(size_t i = 0 ; i < strlen(_Keyval);i++)
     {
        nHash = (nHash<<5) + nHash +_Keyval[i];
     }
     return nHash;
   }
};
typedef stdext::hash_map<char*,UINT,CReservedKeywordCompare<char*> > CSTRING2ID_HASH_MAP;
typedef std::pair<char*,UINT>                    CSTRING2ID_PAIR;

typedef std::vector<CSymbolInfo>                   SYMBOLINFO_ARRAY;
class CScaner
{
public:
   CScaner(CSaxCCompileimpl* pSaxCCompileimpl);
   virtual ~CScaner(void);
public:
   void SetSourceCode(char* const pSourceCode,int nSourceCodeLen) 
   {
     m_pSourceCode  = pSourceCode;
     m_nSourceCodeLen = nSourceCodeLen;
   }
   int GetCurrentLine()                 { return m_nCurrentLine;}
   void PushBack()                   { m_bPushBackFlag = TRUE;} 
   char* GetKeywordDescription(AXC_SYMBOL_TYPE nTokenType);
public:  
   CSymbolInfo* GetCurrentToken()            { return &m_objCurrentToken;}
   CSymbolInfo* GetNextToken();
   void Reset();
   void SaveStatusPoint();
   void RecoverStatusPoint();
protected:
   char GetNextChar()          
   { 
     return (m_nCurrentScanPos < m_nSourceCodeLen)? m_pSourceCode[m_nCurrentScanPos++]:EOF;
   }
   void UnGetNextChar()         { m_nCurrentScanPos--;}
   AXC_SYMBOL_TYPE LookupReserverKeyword(char* pString,CSymbolInfo &objCurrentToken);
   void InitReservedKeywordHashMap(void);
private:
   CSTRING2ID_HASH_MAP     m_hmReservedKeyword;
   char            *m_pSourceCode;
   int             m_nSourceCodeLen;
   int             m_nCurrentScanPos;
   int             m_nCurrentLine;
   CSymbolInfo         m_objCurrentToken;
   CSaxCCompileimpl*      m_pSaxCCompileimpl;
   int             m_nScanPosSaved;
   int             m_nLineSaved;
   BOOL            m_bPushBackFlag;
};
