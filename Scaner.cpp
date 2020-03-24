/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/Scaner.cpp,v 1.13 2015/08/17 01:05:56 administrator Exp $
* 
*******************************************************************************
* 
* Description:C lexical analysis class implement
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/Scaner.cpp,v 1.13 2015/08/17 01:05:56 administrator Exp $";
// $Nokeywords: $

#include "StdAfx.h"
#include ".\scaner.h"
#include <ctype.h>
#include <strsafe.h>
#include ".\saxccompileimpl.h"
#include "parser.h"

CScaner::CScaner(CSaxCCompileimpl* pSaxCCompileimpl)
: m_pSourceCode(NULL)
, m_nSourceCodeLen(0)
, m_nCurrentScanPos(0)
, m_nCurrentLine(1)
, m_pSaxCCompileimpl(pSaxCCompileimpl)
, m_nScanPosSaved(0)
, m_nLineSaved(0)
, m_bPushBackFlag(FALSE)
{
   InitReservedKeywordHashMap();
}

CScaner::~CScaner(void)
{
}

typedef struct tagKeywordInfo
{
   AXC_SYMBOL_TYPE  nType;    //标识符类型
   CStringA     strSymbol;
   TREE_NODE_TYPE  nNodeType;  //语法树节点类型
}KEYWORD_INFO;

const static KEYWORD_INFO  arrReservedKeywords[] = 
{
   {TT_DOUBLE    , "double",TNK_DOUBLE_TYPE},
   {TT_INT     , "int",TNK_INT_TYPE},
   {TT_CHAR     , "char",TNK_CHAR_TYPE}, 
   {TT_BYTE     , "BYTE",TNK_BYTE_TYPE}, 
   {TT_FLOAT    , "float",TNK_FLOAT_TYPE}, 
   {TT_SHORT    , "short",TNK_SHORT_TYPE},
   {TT_UNSIGNED   , "unsigend",TNK_INT_TYPE},
   {TT_BOOL     , "bool",TNK_BOOLEAN_TYPE},
   {TT_LONG     , "long",TNK_LONG_TYPE}, 
   {TT_VOID     , "void",TNK_VOID_TYPE},
   {TT_WORD     , "WORD",TNK_WORD_TYPE},
   {TT_DWORD    , "DWORD",TNK_DWORD_TYPE},
   {TT_LONGLONG   , "LONGLONG",TNK_LONGLONG_TYPE},

   {TT_AUTO     , "auto",TNK_UNKNOWN},
   {TT_BREAK    , "break",TNK_UNKNOWN},
   {TT_STRUCT    , "struct",TNK_UNKNOWN}, 
   {TT_ELSE     , "else",TNK_UNKNOWN},

   {TT_SWITCH    , "switch",TNK_UNKNOWN}, 
   {TT_CASE     , "case",TNK_UNKNOWN}, 
   {TT_ENUM     , "enum",TNK_UNKNOWN}, 
   {TT_REGISTER   , "register",TNK_UNKNOWN},
   {TT_TYPEDEF   , "typedef",TNK_UNKNOWN},
   {TT_EXTERN    , "extern",TNK_UNKNOWN},
   {TT_RETURN    , "return",TNK_UNKNOWN},
   {TT_UNION    , "union",TNK_UNKNOWN},
   {TT_CONST    , "const",TNK_UNKNOWN}, 
   {TT_CONTINUE   , "continue",TNK_UNKNOWN},
   {TT_FOR     , "for",TNK_UNKNOWN}, 
   {TT_SIGNED    , "singed",TNK_UNKNOWN},

   {TT_DEFAULT   , "default",TNK_UNKNOWN}, 
   {TT_GOTO     , "goto",TNK_UNKNOWN}, 
   {TT_SIZEOF    , "sizeof",TNK_UNKNOWN}, 
   {TT_VOLATILE   , "volatile",TNK_UNKNOWN},
   {TT_DO      , "do",TNK_UNKNOWN},
   {TT_IF      , "if",TNK_UNKNOWN}, 
   {TT_STATIC    , "static",TNK_UNKNOWN}, 
   {TT_WHILE    , "while",TNK_UNKNOWN},
   //{TT_READ     , "read"}, 
   //{TT_WRITE    , "write"}, 
   //{TT_PRINTF    , "printf"},

   //{TT_TRUE     , "true",TNK_UNKNOWN},
   //{TT_FALSE    , "false",TNK_UNKNOWN},

   // operations
   {TT_ASSIGN    , "=",TNK_MODIFY_EXPR},
   {TT_PLUS     , "+",TNK_PLUS_EXPR}, 
   {TT_MINUS    , "-",TNK_MINUS_EXPR}, 
   {TT_MULT    , "*",TNK_MULT_EXPR},
   {TT_DIV     , "/",TNK_TRUNC_DIV_EXP},
   {TT_MOD     , "%",TNK_TRUNC_MOD_EXPR},

   {TT_BITWISE_AND , "&",TNK_BIT_AND_EXPR}, 
   {TT_BITWISE_OR  , "|",TNK_BIT_IOR_EXPR},
   {TT_BITWISE_XOR , "^",TNK_BIT_XOR_EXPR},
   {TT_BITWISE_NOT , "~",TNK_BIT_NOT_EXPR}, 

   {TT_LOGICAL_NOT , "!",TNK_TRUTH_NOT_EXPR}, 
   {TT_LT      , "<",TNK_LT_EXPR}, 
   {TT_GT      , ">",TNK_GT_EXPR},
   {TT_QUESTION_MARK, "?",TNK_QUESTION_MARK_EXPR},

   // interpunctions
   {TT_LPARAN    , "(",TNK_UNKNOWN},
   {TT_RPARAN    , ")",TNK_UNKNOWN}, 
   {TT_LBRACE    , "{",TNK_UNKNOWN}, 
   {TT_RBRACE    , "}",TNK_UNKNOWN}, 
   {TT_LSQUARE   , "[",TNK_UNKNOWN},
   {TT_RSQUARE   , "]",TNK_UNKNOWN}, 
   {TT_COMMA    , ",",TNK_COMMA_EXPR}, 
   {TT_DOT     , ".",TNK_UNKNOWN},
   {TT_SEMI     , ";",TNK_UNKNOWN},
   {TT_COLON    , ":",TNK_UNKNOWN},

   // complex operations
   {TT_EQ      , "==",TNK_EQ_EXPR},   
   {TT_NEQ     , "!=",TNK_NEQ_EXPR},   
   {TT_PLUS_PLUS  , "++",TNK_PREINCREMENT_EXPR},   
   {TT_MINUS_MINUS , "--",TNK_PREDECREMENT_EXPR},   
   {TT_PLUS_ASSIGN , "+=",TNK_PLUS_ASSIGN},   
   {TT_MINUS_ASSIGN , "-=",TNK_MINUS_ASSIGN},   
   {TT_MULT_ASSIGN , "*=",TNK_MULT_ASSIGN},    
   {TT_DIV_ASSIGN  , "/=",TNK_DIV_ASSIGN}, 
   {TT_MOD_ASSIGN  , "%=",TNK_MOD_ASSIGN}, 
   {TT_AND_ASSIGN  , "&=",TNK_AND_ASSIGN},   
   {TT_XOR_ASSIGN  , "^=",TNK_XOR_ASSIGN},   
   {TT_OR_ASSIGN  , "|=",TNK_OR_ASSIGN},   
   {TT_NGT     , ">=",TNK_NGT_EXPR},    
   {TT_NLT     , "<=",TNK_NLT_EXPR},   
   {TT_LOGICAL_AND , "&&",TNK_TRUTH_AND_EXPR},   
   {TT_LOGICAL_OR  , "||",TNK_TRUTH_OR_EXPR}, 
   {TT_LSHIFT    , "<<",TNK_LSHIFT_EXPR},   
   {TT_RSHIFT    , ">>",TNK_RSHIFT_EXPR}, 
   {TT_LSHIFT_ASSIGN, "<<=",TNK_LSHIFT_ASSIGN_EXPR},   
   {TT_RSHIFT_ASSIGN, ">>=",TNK_RSHIFT_ASSIGN_EXPR},
};

/*-----------------------------------------------------------------
| 函数名称 : CScaner::InitReservedKeywordHashMap
| 描  述 : Initialize the reserved keyword GetHashKey map,
| 修改记录 : 2007-4-21 15:55:46  -huangdy-  创建
-----------------------------------------------------------------*/
void CScaner::InitReservedKeywordHashMap(void)
{
   for(UINT i = 0 ;i < sizeof(arrReservedKeywords) / sizeof(KEYWORD_INFO); i++ )
   {
     m_hmReservedKeyword.insert(CSTRING2ID_PAIR(
        (char*)(LPCSTR)(arrReservedKeywords[i].strSymbol),i));
   } 
   
}


/*-----------------------------------------------------------------
| 函数名称 : CScaner::GetKeywordDescription
| 描  述 : 
| 参  数 : AXC_SYMBOL_TYPE nTokenType——
| 返 回 值 : 
| 修改记录 : 2007-4-24 8:55:21  -huangdy-  创建
-----------------------------------------------------------------*/
char* CScaner::GetKeywordDescription(AXC_SYMBOL_TYPE nTokenType)
{
   char* pResult = NULL;

   for(UINT i = 0 ;i < sizeof(arrReservedKeywords) / sizeof(KEYWORD_INFO); i++ )
   {
     if(arrReservedKeywords[i].nType == nTokenType)
     {
        pResult = (char*)(LPCSTR)(arrReservedKeywords[i].strSymbol);
        break;
     }   
   }
   

   return pResult;
}
/*-----------------------------------------------------------------
| 函数名称 : CScaner::LookupReserverKeyword
| 描  述 : Look up reserver keyword by name in GetHashKey map.
| 参  数 : char* pString——A pointer to a string.
| 返 回 值 : If find the reserver keyword,then return reserver keyword ID,
|       Otherwise return symboy ID.
| 修改记录 : 2007-4-22 9:31:59  -huangdy-  创建
-----------------------------------------------------------------*/
AXC_SYMBOL_TYPE CScaner::LookupReserverKeyword(char* pString,CSymbolInfo &objCurrentToken)
{
   AXC_SYMBOL_TYPE ttSymbolType = TT_IDENTIFY_TYPE;

   stdext::hash_map<char*,UINT>::const_iterator itr;
   itr = m_hmReservedKeyword.find(pString);
   if(m_hmReservedKeyword.end() != itr)
   {
     ttSymbolType = arrReservedKeywords[(AXC_SYMBOL_TYPE)itr->second].nType;
     objCurrentToken.SetNodeType(arrReservedKeywords[(AXC_SYMBOL_TYPE)itr->second].nNodeType);
   }
   objCurrentToken.SetType(ttSymbolType);

   return ttSymbolType;
}


/*-----------------------------------------------------------------
| 函数名称 : CScaner::GetNextToken
| 描  述 : Get m_pNext a token information by scan source code.
| 参  数 : CSymbolInfo& m_objCurrentToken——A "CSymbolInfo" object to
|       returned token information.
| 返 回 值 : TRUE----success
|       FALSE---fail
| 修改记录 : 2007-4-21 15:57:35  -huangdy-  创建
-----------------------------------------------------------------*/
CSymbolInfo* CScaner::GetNextToken()
{
   SCAN_STATE_TYPE nState   = SST_START;
   bool       bSave   = false;  
   BOOL       bIsInteger = TRUE;  //判断是否是整数

   if(m_bPushBackFlag)
   {
     m_bPushBackFlag = false;

     return &m_objCurrentToken;    
   }
   char szSymbol[SYMBOL_MAX_LENGTH] = {0};
   UINT nSymbolLen = 0;

   char chNextChar = '\0';
   char chCurrent = '\0';
   while(SST_DONE != nState)
   {
     chCurrent = GetNextChar();
     if(chCurrent < 0)
     {
        m_objCurrentToken.SetType(TT_EOF_TYPE);
        break;
     }
     switch(nState)
     {
     case SST_START:
        if(isdigit((int)chCurrent))
        {
          nState = SST_INNUMERAL; 
          bSave = true;
        }
        else if('.' == chCurrent )
        {
          bIsInteger = false;   //确定是小数
          nState = SST_INNUMERAL; 
          bSave = true;
        }
        else if(isalpha((int)chCurrent) || '_' == chCurrent)
        {
          nState = SST_INIDENTIFY;
          bSave = true;
        }
        else if('"' == chCurrent)
        {
          nState = SST_INSTRING;
        }
        else if('\'' == chCurrent)
        {
          nState = SST_INCHAR;
        }
        else if('\r' == chCurrent || '\n' == chCurrent 
          || '\t' == chCurrent || ' ' == chCurrent)
        {
          if('\n' == chCurrent)
          {
             m_nCurrentLine++;
          }
        }
        else 
        {
          nState = SST_DONE;
          szSymbol[0] = chCurrent;
          LookupReserverKeyword(szSymbol,m_objCurrentToken);

          if(TT_NULL_TYPE == m_objCurrentToken.GetType())
          {
             m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,Unknow operation."));
             break;
          }                    

          switch(chCurrent)
          {
          case '=':
             if('=' == GetNextChar())          /* == */
             {
               m_objCurrentToken.SetType(TT_EQ);
               m_objCurrentToken.SetNodeType(TNK_EQ_EXPR);
               memcpy(szSymbol,"==",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '+':
             chNextChar = GetNextChar();
             if('+' == chNextChar)      /* ++ */
             {
               m_objCurrentToken.SetType(TT_PLUS_PLUS);
               m_objCurrentToken.SetNodeType(TNK_PREINCREMENT_EXPR);
               memcpy(szSymbol,"++",2);
             }
             else if('=' == chNextChar)   /* += */
             {
               m_objCurrentToken.SetType(TT_PLUS_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_PLUS_ASSIGN);
               memcpy(szSymbol,"+=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '-':
             chNextChar = GetNextChar();
             if('-' == chNextChar)      /* -- */
             {
               m_objCurrentToken.SetType(TT_MINUS_MINUS);
               m_objCurrentToken.SetNodeType(TNK_PREDECREMENT_EXPR);
               memcpy(szSymbol,"--",2);
             }
             else if('=' == chNextChar)   /* -= */
             {
               m_objCurrentToken.SetType(TT_MINUS_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_MINUS_ASSIGN);
               memcpy(szSymbol,"-=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '*':
             if('=' == GetNextChar())          /* *= */
             {
               m_objCurrentToken.SetType(TT_MULT_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_MULT_ASSIGN);
               memcpy(szSymbol,"*=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;        

          case '&':
             chNextChar = GetNextChar();
             if('&' == chNextChar)          /* && */
             {
               m_objCurrentToken.SetType(TT_LOGICAL_AND);
               m_objCurrentToken.SetNodeType(TNK_TRUTH_AND_EXPR);
               memcpy(szSymbol,"&&",2);
             }
             else if('=' == chNextChar)          /* &= */
             {
               m_objCurrentToken.SetType(TT_AND_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_AND_ASSIGN);
               memcpy(szSymbol,"&=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '|':
             chNextChar = GetNextChar();
             if('|' == chNextChar)          /* || */
             {
               m_objCurrentToken.SetType(TT_LOGICAL_OR);
               m_objCurrentToken.SetNodeType(TNK_TRUTH_OR_EXPR);
               memcpy(szSymbol,"||",2);
             }
             else if('=' == chNextChar)       /* |= */
             {
               m_objCurrentToken.SetType(TT_OR_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_OR_ASSIGN);
               memcpy(szSymbol,"|=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;   

          case '^':
             chNextChar = GetNextChar();
             if('=' == chNextChar)          /* ^= */
             {
               m_objCurrentToken.SetType(TT_XOR_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_XOR_ASSIGN);
               memcpy(szSymbol,"^=",2);             
             }
             
             else
             {
               UnGetNextChar();
             }
             break;   

          case '%':
             if('=' == GetNextChar())          /* %= */
             {
               m_objCurrentToken.SetType(TT_MOD_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_MOD_ASSIGN);
               memcpy(szSymbol,"%=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '!':
             if('=' == GetNextChar())          /* != */
             {
               m_objCurrentToken.SetType(TT_NEQ);
               m_objCurrentToken.SetNodeType(TNK_NEQ_EXPR);
               memcpy(szSymbol,"!=",2);
             }
             else
             {
               UnGetNextChar();
             }
             break;

          case '<':
             switch(GetNextChar())
             {
             case '=':
               m_objCurrentToken.SetType(TT_NLT);
               m_objCurrentToken.SetNodeType(TNK_NLT_EXPR);
               memcpy(szSymbol,"<=",2);
               break;

             case '<':
               if(GetNextChar() == '=')
               {
                  m_objCurrentToken.SetType(TT_LSHIFT_ASSIGN);
                  m_objCurrentToken.SetNodeType(TNK_LSHIFT_ASSIGN_EXPR);
                  memcpy(szSymbol,"<<=",3);
               }
               else
               {
                  m_objCurrentToken.SetType(TT_LSHIFT);
                  m_objCurrentToken.SetNodeType(TNK_LSHIFT_EXPR);
                  memcpy(szSymbol,"<<",2);

                  UnGetNextChar();
               }
               break;
             default:
               UnGetNextChar();
             }             
             break;

          case '>':
             switch(GetNextChar())
             {
             case '=':
               m_objCurrentToken.SetType(TT_NGT);
               m_objCurrentToken.SetNodeType(TNK_NGT_EXPR);
               memcpy(szSymbol,">=",2);
               break;

             case '>':
               if(GetNextChar() == '=')
               {
                  m_objCurrentToken.SetType(TT_RSHIFT_ASSIGN);
                  m_objCurrentToken.SetNodeType(TNK_RSHIFT_ASSIGN_EXPR);
                  memcpy(szSymbol,">>=",3);
               }
               else
               {
                  m_objCurrentToken.SetType(TT_RSHIFT);
                  m_objCurrentToken.SetNodeType(TNK_RSHIFT_EXPR);
                  memcpy(szSymbol,">>",2);
                  UnGetNextChar();
               }
               break;
             default:
               UnGetNextChar();
             }             
             break;

          case '/':
             chNextChar = GetNextChar();
             if('=' == chNextChar)           /* /= */        
             {
               m_objCurrentToken.SetType(TT_DIV_ASSIGN);
               m_objCurrentToken.SetNodeType(TNK_DIV_ASSIGN);
               memcpy(szSymbol,"/=",2);
             }
             else if('/' == chNextChar)         //It indicate a comment 
             {
               nState = SST_INCOMMENT1;
               m_objCurrentToken.SetType(TT_NULL_TYPE);
             }
             else if('*' == chNextChar)          //It indicate a comment 
             {
               nState = SST_INCOMMENT2;
               m_objCurrentToken.SetType(TT_NULL_TYPE);
             }
             else
             {
               UnGetNextChar();
             }
             break;          
          }
        }
        break;

     case SST_INIDENTIFY:
        if(!isalnum( (int)chCurrent ) && '_' != chCurrent)
        {          
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_IDENTIFY_TYPE);
          UnGetNextChar();
        }
        else if(nSymbolLen >= SYMBOL_MAX_LENGTH)
        {
          m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0
             ,_T("Line %d,Symbol too length to exceed 255 bytes."));
        }
        break;

     case SST_INNUMERAL:
        if('.' == chCurrent)
        {
          bIsInteger = FALSE;
        }
        else if(!isdigit((int)chCurrent) )
        {   
          bSave = false;
          nState = SST_DONE;          
          m_objCurrentToken.SetType(bIsInteger? TT_INTEGER_TYPE:TT_REAL_TYPE);             
          UnGetNextChar();
        }
        break;

     case SST_INSTRING:
        bSave = true;
        if('"' == chCurrent)
        {
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_STRING_TYPE);
        }     
        else if('\\' == chCurrent)  // escape
        {
          chCurrent = GetNextChar();
          if(END_OF_SOURCE_CODE == chCurrent)
          {
             bSave = false;
             nState = SST_DONE;
             m_objCurrentToken.SetType(TT_ERROR_TYPE);
             m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,String define error."));
          }
        }
        else if('\r' == chCurrent || '\n' == chCurrent)
        {
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_ERROR_TYPE);
          m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,String define error."));
        }
        else if(nSymbolLen >= SYMBOL_MAX_LENGTH)
        {
          m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,String too length to exceed 255 bytes."));
        }
        break;

     case SST_INCHAR:
        bSave = true;
        if('\'' == chCurrent)
        {
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_CHAR_TYPE);
        }     
        else if('\\' == chCurrent)
        {
          chCurrent = GetNextChar();
          if(END_OF_SOURCE_CODE == chCurrent)
          {
             bSave = false;
             nState = SST_DONE;
             m_objCurrentToken.SetType(TT_ERROR_TYPE);
             m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,Char define error."));

          }
        }
        else if('\r' == chCurrent || '\n' == chCurrent)
        {
          // error msg: syntax error in line %d: new line in constant
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_ERROR_TYPE);
          m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,Char define error."));
        }
        else if(nSymbolLen > 1)
        {
          bSave = false;
          nState = SST_DONE;
          m_objCurrentToken.SetType(TT_ERROR_TYPE);
          m_pSaxCCompileimpl->OutputErrMsg(TRUE,m_nCurrentLine,0,_T("Line %d,Char length exceed 1 bytes."));
        }
        break;

     case SST_INCOMMENT1:
        if('\r' == chCurrent || '\n' == chCurrent)
        {          
          nState = SST_START;
        }
        break;

     case SST_INCOMMENT2:
        if('*' == chCurrent)
        {
          if('/' == GetNextChar())
          {
             nState = SST_START;
          }
        }
        else if('\r' == chCurrent || '\n' == chCurrent)
        {
          if('\n' == chCurrent)
          {
             m_nCurrentLine++;
          }
        }
        break;

     case SST_DONE:          
     default:               //Should never happen
        nState = SST_DONE;
        m_objCurrentToken.SetType(TT_ERROR_TYPE);
     }

     if(bSave && nSymbolLen < SYMBOL_MAX_LENGTH)
     {
        szSymbol[nSymbolLen] = chCurrent;
        nSymbolLen++;
     }

     if(SST_DONE == nState)
     {
        m_objCurrentToken.SetSymbol(szSymbol);
        m_objCurrentToken.SetLineIndex(m_nCurrentLine);

        if(TT_IDENTIFY_TYPE == m_objCurrentToken.GetType())
        {
          LookupReserverKeyword(szSymbol,m_objCurrentToken);
        }
        ATLTRACE(_T("%s\r\n"),CString(szSymbol));
     }
   }

   return &m_objCurrentToken;
}


/*-----------------------------------------------------------------
| 函数名称 : CScaner::Reset
| 描  述 : Reset the scanet 
| 修改记录 : 2007-4-22 9:22:42  -huangdy-  创建
-----------------------------------------------------------------*/
void CScaner::Reset()
{
   m_pSourceCode    = NULL;
   m_nSourceCodeLen  = 0;
   m_nCurrentScanPos  = 0;
   m_nCurrentLine   = 0;
   m_nScanPosSaved   = 0;
   m_nLineSaved    = 0;
}

void CScaner::SaveStatusPoint()
{
   m_nScanPosSaved = m_nCurrentScanPos;
   m_nLineSaved  = m_nCurrentLine;
}

void CScaner::RecoverStatusPoint()
{
   m_bPushBackFlag  = FALSE;
   m_nCurrentScanPos = m_nScanPosSaved;
   m_nCurrentLine  = m_nLineSaved;
}
