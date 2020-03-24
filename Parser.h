/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/Parser.h,v 1.19 2015/08/17 01:05:56 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include "scaner.h"
#include ".\global.h"
#include <map>
#include <vector>
#include ".\saxccompile.h"
#include <exception>

//1.û�з�������ĳ��Ի�,��int array1[] = {0,1,2,3,4};



class CSaxCCompileimpl;
class CSyntaxTreeNode;
class CIdentifierTreeNode;
class CParser;
class CDataTypeTreeNode;

typedef std::vector<CSyntaxTreeNode*>            SYNTAX_TREE_NODE_ARRAY;     
typedef std::vector<CSyntaxTreeNode*>::iterator  SYNTAX_TREE_NODE_ARRAY_ITR;  

typedef std::vector<CString>            CSTRING_ARRARY;    

const int MAX_CHILDREN_NUMBER  = 4;  // parse-tree node




class CParserBase
{ 
public:
	CParserBase()
	{
	}
public: 
	void AppendRef(CSyntaxTreeNode* pNodeReferenced,CSyntaxTreeNode* pSyntaxTreeNode);
protected:
	SYNTAX_TREE_NODE_ARRAY m_arrEntries;              //��������ʹ��������Ŀ�б�
};




class CParserException : public std::exception
{ 
public:
	CParserException(char* pErrMsg = NULL)
		: m_strErrMsg(pErrMsg)
	{
	}
public: 
	const char* what() const throw() 
	{ 
		return m_strErrMsg; 
	} 
private:
	CStringA  m_strErrMsg;
};

// ��TNK_CONST_INTEGER������������һ��˫�������������˳���Ϊ�з��ų�����    ��ֵ��չ��˫�֡� 
//   ��TNK_CONST_REAL������������һ���ԡ�double�����ַ�������ʽ��ʾ��ʵ��ֵ������ֵ���ַ�����ʽ��ʾ��������������κ��Ż��ᵫ��������롣 
//   ��TNK_CONST_STRING������������һ���ַ���������
//   ��TNK_CONST_COMPLEX������������һ�����ͳ����� 
//   ��TNK_IDENTIFIER������������һ����ʶ���� 
//   ��TNK_LIST������������һ����ؽ�㡣 
//   ��TNK_VECOTR������������һ���������� 
//   ��TNK_EXPRESSION������������һ�����ʽ�� 
//   ��TREE_BLOCK����������ʾһ�������������Ϣ�� 
//   ��TREE_DATA_TYPE�������������������͡� 
//   ��TREE_DECL������������һ����˵���ķ������� 
//   ��TREE_VECTOR����������Ӧ�ڻ���ģʽ�е�vectorģʽ����Ҫ���ڲ���ִ�б��ʽ

enum TREE_NODE_TYPE
{ 
	TNK_UNKNOWN = 0, 

	//��������� 

	TNK_ERROR_MARK, /*��ʶ�κγ���Ľṹ���ں������������У����������κ��������б��޴�����գ��Ա����ͬһ����������������Ϣ������TREE_CODE���⣬�����㲻ʹ������������һ�������㡣*/
	TNK_IDENTIFIER_NODE, /*��ʾһ�����֡����ڲ�������������STRING_CST��㡣����һ�ض����֣�ֻ����һ����Ӧ��IDENTIFIER_NODE����get_identifier������������һ����get_identifier ʱ������һ��IDENTIFIER_NODE��㡣����һ�������㡣*/
	TNK_TREE_LIST_NODE, /*����һ�������㡣��TREE_VALUE��TREE_PURPOSE��������Щ���ͨ��TREE_CHAIN�����������е�Ԫ�ش�����TREE_VALUE��TREE_PURPOSE��ֻ��ĳЩ�����ʹ�á�*/
	TNK_TREE_VEC, /* ����һ�������㣬����һ��������顣*/
	TNK_BLOCK,   /*����һ���ʷ��飬��ʾһ����������ο�*/

	//���ͽ�����
	TNK_DATA_TYPE_ID_MIN = 50, //���Ǿ������������ֻ�����ж�
	TNK_BYTE_TYPE,
	TNK_WORD_TYPE,
	TNK_DWORD_TYPE,
	TNK_LONG_TYPE,
	TNK_SHORT_TYPE,
	TNK_FLOAT_TYPE,
	TNK_LONGLONG_TYPE,
	TNK_INT_TYPE, /*����TYPE����࣬��ʾ���������е����ͣ�����C�е��ַ��ͣ�Ҳ����������ɢ���͵��ӷ�Χ���͡�����TYPE_MIN_VALUE��TYPE_MAX_VALUE��TYPE_PRECISION������ ��PASCAL�е������ͣ�TREE_DATA_TYPE��ָ���䳬���ͣ���һ��INTEGER_TYPE��ENUMERAL_TYPE��BOOLEAN_TYPE��������TREE_DATA_TYPEΪ�ա�*/
	TNK_DOUBLE_TYPE,  /*����TYPE����࣬��ʾC�����еĸ����˫�������ͣ���ͬ�ĸ��������ɻ���ģʽ��TYPE_SIZE��TYPE_PRECISION���֡�*/
	TNK_BOOLEAN_TYPE, /*��TYPE����ࡣ��ʾPASCAL�еĲ������ͣ�ֻ��true��false����ֵ������ʹ��������*/
	TNK_CHAR_TYPE,   /*��TYPE����࣬��ʾPASCAL�е�CHAR���ͣ�����C��ʹ�á�����ʹ�������� */
	TNK_VOID_TYPE,  /*����TYPE����࣬��ʾC�����е�void ���͡�*/ 
	TNK_COMPLEX_TYPE, /*����TYPE����࣬��ʾ�������ͣ�TREE_DATA_TYPE��ָ��ʵ�����鲿�����͡�*/ 
	TNK_VECTOR_TYPE, /*��ʾ�������ͣ����͵�TREE_DATA_TYPE ��ָ������Ԫ�ص��������͡�*/
	TNK_ENUMERAL_TYPE, /*����TYPE����࣬��ʾC�е�ö�����ͣ�������INTEGER_TYPE��㡣��ʾö��ֵ�ķ����� CONST_DECL��㶨�壬��TREE_DATA_TYPE����ָ�����ǣ��������TREE_VALUE��ָ��һ��TREE_LIST�����ɵ���������ÿ��Ԫ�ص�TREE_PURPOSE���ʾö������TREE_VALUE���ʾ��ֵ��ΪINTEGER_CST��㣩�� �����û�ж���ö�����������ж�����ǰ�����ã�����TYPE_SIZEΪ�ա���ǩ������TYPE_NAME��������Ժ����˴����ͣ�����Щ�涨���򽫱���䡣��RECORD_TYPE��UNION_TYPE��QUAL_UNION_TYPE��ǰ������Ҳ�����ƴ���*/
	TNK_POINTER_TYPE, /*����TYPE����࣬����ָ�����Ͷ��д˴��롣TREE_DATA_TYPEָ��ָ����ָ���͡� */
	TNK_OFFSET_TYPE,  /*����TYPE����ࡣ��ʾ�����ĳһ�����һ��ƫ��ָ�롣TREE_DATA_TYPE��ָ����ƫ�ƴ��Ķ�������ͣ�TYPE_OFFSET_BASETYPEָ����Խ������͡�*/
	TNK_REFERENCE_TYPE, /*����TYPE����࣬������ָ�����ͣ���ָ�����Ͳ�ͬ���ǣ����Զ�����ָ��ֵһ�£�����C++��*/
	TNK_METHOD_TYPE,  /*����TYPE����࣬��ʾһ�ֺ������͡��˺���Ϊ���Լ�ռ��һ������Ĳ�����Ϊ��һ���������Ҵ˲����������ڳ���������Ĳ������С� TREE_DATA_TYPEָ�򷵻����ͣ�TYPE_METHOD_BASETYPEָ�������������͡� TYPE_ARG_TYPES��ʾ�����Ĳ������������а������������ĵ�һ��������*/
	TNK_FILE_TYPE,   /*����TYPE����࣬��PASCAL��ʹ�ã������ڿ����ڻ�û����*/
	TNK_ARRAY_TYPE,   /*����TYPE����࣬��ʾC��PASCAL�е��������͡��������м���������� TREE_DATA_TYPEָ������Ԫ�ص����͡� TYPE_DOMAINָ�����ڼ��������ͣ���ֵ�ķ�Χ˵��������ĳ��ȡ� TYPE_POINTER_TO��TREE_DATA_TYPE (array_type)��ͨ����Ϊ�ա� TYPE_STRING_FLAGָ��һ���ַ�����������һ���ַ����飩�� TYPE_SEPָ���һ��Ԫ�ص���һ��Ԫ�صĵ�λ�����ʽ�� TYPE_SEP_UNITָ��ǰһ��λ�е�λ����*/
	TNK_SET_TYPE,    /*����TYPE����࣬��ʾPASCAL�еļ������͡�*/
	TNK_RECORD_TYPE,  /*����TYPE����࣬��ʾC�еĽṹ��PASCAL�еļ�¼����������У� TYPE_FIELDS��, ������FIELD_DECLS�����ʽ���ӽṹ�еĸ����� ��ǰ�����ô���ͬö�����͡�*/
	TNK_UNION_TYPE,   /*����TYPE����࣬��ʾC�е����ϣ������ڽṹ���͵�����ƫ��Ϊ0����ǰ�����ô���ͬö�����͡�*/ 
	TNK_QUAL_UNION_TYPE,/*����TYPE����࣬������UNION_TYPE����ÿһ��FIELD_DECL�е�DECL_QUALIFIER���еı��ʽ�����������е����ݣ���һ��DECL_QUALIFIER���ʽΪ��������������ϵ�ֵ��*/ 
	TNK_FUNCTION_TYPE, /*����TYPE����࣬��ʾ�������ͣ��������� TREE_DATA_TYPEָ�򷵻�ֵ����TYPE_ARG_TYPESָ��������ͱ���TREE_LIST�����ɡ� �������й����뺯���в�� ��ʾ���̵�����Ҳ����FUNCTION_TYPE���룬��TREE_DATA_TYPEΪ�ա�*/
	TNK_LANG_TYPE,   /*����TYPE����࣬����������ص�һ�����ͣ��������������������ǰ�ˣ�layout_type���̲�֪�����������������ͣ�����ǰ�˱����ֹ�����*/
	TNK_DATA_TYPE_ID_MAX = 100, //���Ǿ������������ֻ�����ж�

	//���ʽ������,��ʾ��������������
	TNK_INTEGER_CST,  /*��ʾ����������������TREE_INT_CST_LOW��TREE_INT_CST_HIGH���У�ÿ����ռ32λ�������ܱ�ʾ64λ��������PASCAL�����е�char���͵ĳ���Ҳ��ʾΪINTEGER_CST��PASCAL��C�е�ָ�볣���� NIL ��NULLҲ��ʾΪINTEGER_CST��C�еġ�(int *)��Ҳ����һ��INTEGER_CST�� */
	TNK_REAL_CST,   /*��ʾʵ��������������TREE_REAL_CST���С�*/
	TNK_COMPLEX_CST,  /*��ʾ���ͳ�������������TREE_REALPART��TREE_IMAGPART���С� ����������ָ������������㡣 */
	TNK_VECTOR_CST,  /*��ʾ������������������TREE_VECTOR_CST_ELTS���С� */
	TNK_STRING_CST,   /*��ʾ�ַ�����������������TREE_STRING_LENGTH��TREE_STRING_POINTER����*/

	//���ʽ������,��ʾ���ֵ���������(���ж����ֵ����ýԱ�ʾΪ��_DECL��㣬ͬһ��ε�����ͨ��TREE_CHAIN��������ÿ��DECL��һ��DECL_NAME��ָ��һ��IDENTIFIER��㣨����Щ���֣����ţ�DECL_NAMEΪ�գ�)
	TNK_FUNCTION_DECL, /*��ʾһ�������������ĸ������� DECL_ARGUMENTSָ��һ����ʾ������PARM_DECL�� DECL_RESULTָ���ʾ��������ֵ��RESULT_DECL�� DECL_RESULT_TYPEָ�������ص����ͣ���ͨ����DECL_RESULT������һ�¡�������������һ�ֱ�DECL_RESULT���Ϳ�����͡���Ϊ���������������������ڴ˺����������ʱ����Ч�� DECL_FUNCTION_CODE��һ���������������ʾ�ڲ���������ֵ���ڲ�������һ��ö��ֵ����ֵ������Ӧ���ڲ�������*/
	TNK_LABEL_DECL,  /*��ʾһ���������*/ 
	TNK_CONST_DECL,  /*��ʾһ����������*/ 
	TNK_TYPE_DECL,   /*��ʾһ����������*/ 
	TNK_VAR_DECL,   /*��ʾһ����������*/
	TNK_PARM_DECL,   /*��ʾһ���Ʋ�������һ��������DECL_ARG_TYPEָ��ʵ�ʴ��ݵĲ������ͣ��������п���������ж�������Ͳ�һ�¡� */
	TNK_RESUCL_DECL,  /*��ʾ����ֵ���� */
	TNK_FIELD_DECL,  /*��ʾ������NAMESPACE_DECL   ��ʾ���ֿռ�˵�������ֿռ����������_DECL��DECL_CONTEXT���У��ṩһ�����ֵĲ�νṹ��*/


	//���ʽ������,��ʾ�ڴ����õĽ�����
	TNK_COMPONENT_REF, /*��ʾ�Խṹ�������е�һ�����������á�����0��ʾ�ṹ�����ϣ�����1��ʾһ����һ��FIELD_DECL��㣩�� */
	TNK_BIT_FIELD_REF, /*��ʾ����һ�������ڵ�һ��λ��������COMPONENT_REF����OMPONENT_REF��ͬ����λ���λ������ȷ�����ģ�������ͨ��FIELD_DECL����������0�ǽṹ�����ϡ� ����1��һ���������������ʵ�λ��������2��һ�������������ʵĵ�1λ��λ�á� �����ʵ���������з��ŵģ�Ҳ�������޷��ŵģ���TREE_UNSIGNED������� */
	TNK_INDIRECT_REF, /*��ʾC�����е�һԪ������*����PASCAL�����еġ��ޡ�����һ������ָ��ı��ʽ������ */
	TNK_VAR_REF,    /*һ�������Ĳο� */
	TNK_ARRAY_REF,   /*������ʣ�����0��ʾ���飬����1��һ�����������±��TREE_LIST��㡣 */
	TNK_ARRAY_RANGE_REF, /*������ARRAY_REF����֮ͬ���������ʾһ������Ƭ��(slice) ������0��ʾ���飬����1��ʾ������ʼ�㣬Ƭ�εĴ�С�ӱ��ʽ�������л�ȡ�� */
	TNK_VTABLE_REF,  /*Vtable��������������Ҫ����vtable�������ռ��� ������0��ʾһ���������û�ȼ۵ı��ʽ��������1��ʾvtable�Ļ�ַ (������һ��var_decl)�� ������2��ʾ��vtable�е�����(������һ��integer_cst)�� */
	TNK_CONSTRUCTOR,  /*����һ����ָ��������ɵļ���ֵ�� ��C�У���ֻ���ڽṹ�������ʼ���� ����1��һ��ָ��RTL��ָ�룬�ҽ��Գ���CONSTRUCTOR��Ч�� ����2��һ����TREE_LIST�����ɵķ���ֵ��*/


	//���ʽ������,��ʾ���ʽ�Ľ�����
	TNK_COMPOUND_EXPR, /*������������������ʽ����һ�����ʽ��ֵ�����ԣ��ڶ������ʽ��ֵ�����á� */
	TNK_MODIFY_EXPR,  /*��ֵ���ʽ��������0��ʾ��ֵ��������1��ʾ��ֵ��*/
	TNK_INIT_EXPR,   /*��ʼ�����ʽ��������0��ʾ����ʼ���ı�����������1��ʾ��ʼֵ�� */
	TNK_TARGET_EXPR,  /*������0��ʾһ����ʼ��Ŀ�꣬������1��ʾ��Ŀ��ĳ�ֵ������еĻ�������2��ʾ�˽���cleanup�� */
	TNK_COND_EXPR,   /*�������ʽ��������0��ʾ������������1��ʾ THEN ֵ��������2��ʾ ELSEֵ��*/
	TNK_BIND_EXPR,   /*����ȥ����ʱ����ֵ��Ҫ������ı��ʽ��C++ʹ�á� �ֲ�����˵��������RTL���ɺͿռ���䡣 ������0�Ǳ������VAR_DECL���� ������1��Ҫ����ı��ʽ�����ʽ�к��������еı�������������1��ֵ��BIND_EXPR��ֵ��*/ 
	/*������2��һ����˲�����Ӧ��BLOCK�����ڵ��ԡ������BIND_EXPR�� ��չ������BLOCK����е�TREE_USED��־��*/ 
	/*BIND_EXPR��������Щ��������Ϣ֪ͨɨ������������ʽ��������һ�������ļ��������BIND_EXPR�Ĵ��븺����Щ��������Ϣ����ɨ������ */
	/*һ��BIND_EXPR����չ����TREE_USED��־λΪ1������������õ�BIND_EXPR��������δ����չ�����ֹ�����TREE_USED�� */
	/*ΪʹBIND_EXPRȫ�̿ɼ����������Ĵ�����뽫����Ϊһ��subblock�����뺯����BLOCK����С�*/
	TNK_CALL_EXPR,   /*�������á�������0ָ������������1��һ������������һ��TREE_LIST�����ɣ����޲�����2��������2����CALL_EXPR_RTL��ʹ�á� */
	TNK_METHOD_CALL_EXPR,  /*����һ��������������0�ǵ��õķ�����������ΪMETHOD_TYPE�� ������1�Ǵ���������ı��ʽ��������2��һ�������� */
	TNK_WITH_CLEANUP_EXPR, /*ָ��һ��Ҫ��������Ӧ���������ͬʱ�����ֵ��������0��ʾ��ֵ������ı��ʽ��������1��һ�����մ����ֵ��RTL_EXPR��������2�Ǵ˶����������ʽ��������ʽ�к�RTL_EXPR����ʾ�˱��ʽ����������Ҫ�����ֵ������������ɵ�һ�����CLEANUP_POINT_EXPR��������ڵĻ�����ִ�еģ���������Ҫʱ���������������ֹ�����EXPAND_CLEANUPS_TO������ */
	TNK_CLEANUP_POINT_EXPR, /*����һ������㡣������0��ȷ���� cleanups �ı�����ı��ʽ�� */
	TNK_PLACEHOLDER_EXPR,  /*����һ����¼��������˱��ʽʱ���ṩһ�� WITH_RECORD_EXPR������˼�¼ �����ʽ����������Ѱ������˱��ʽ�ļ�¼��*/
	TNK_WITH_RECORD_EXPR,  /*�ṩһ�����ʽ���˱��ʽ����һ���������PLACEHOLDER_EXPR�ļ�¼��������1������������ļ�¼���������������0�е�PLACEHOLDER_EXPR������һ�¡� ���������������С������á����͵�����, ��ADA���ԡ�*/

	//��ʾ����������Ľ�����(������Ĳ�������������ͬ�Ļ���ģʽ���������ͬ���Ļ���ģʽ��)
	TNK_PLUS_ASSIGN,
	TNK_MINUS_ASSIGN,
	TNK_MULT_ASSIGN,
	TNK_DIV_ASSIGN,
	TNK_MOD_ASSIGN,
	TNK_AND_ASSIGN,
	TNK_XOR_ASSIGN,
	TNK_OR_ASSIGN,
	TNK_LSHIFT_ASSIGN_EXPR,
	TNK_RSHIFT_ASSIGN_EXPR,
	TNK_PLUS_EXPR,   /* �Ӳ������������������� */
	TNK_MINUS_EXPR,  /*���������������������� */
	TNK_MULT_EXPR,   /*�˲����� */
	TNK_TRUNC_DIV_EXP, /*��������������β�����ֱ��س��������ϣ���������Ϊʵ�ͣ���Ŀǰ����֧�֡�����ͨ��Ϊ���������Ͳ�����������ͬ���͡�*/
	TNK_CEIL_DIV_EXPR, /*����������������ȡ����*/ 
	TNK_FLOOR_DIV_EXPR, /*����������������ȡ����*/
	TNK_ROUND_DIV_EXPR, /*����������������������.*/
	TNK_TRUNC_MOD_EXPR, /*��Ӧ��TRUNC_DIV_EXPR����������� */
	TNK_CEIL_MOD_EXPR, /*��Ӧ��CEIL_DIV_EXP����������� */
	TNK_FLOOR_MOD_EXPR, /* ��Ӧ��FLOOR_DIV_EXPR���������*/ 
	TNK_ROUND_MOD_EXP, /*��Ӧ��ROUND_DIV_EXP����������� */
	TNK_RDIV_EXPR,   /*ʵ�����������ý��Ϊʵ�����������������������ͬ�����͡������ϣ���������Ϊ��������Ŀǰ��֧��ʵ�Ͳ���������������ͱ��������������һ�¡�*/
	TNK_EXACT_DIV_EXPR, /*������������ĳ���������C�е�ָ��������� �������������������������� ���������ֲ�ͬ�ضϷ�ʽ�Ľ�ʵ��ת��Ϊ�������Ĳ�����CONVERT_EXPRҲ�����ڽ�һ��ʵ��ת���������� */
	TNK_FIX_TRUNC_EXPR, 
	TNK_FIX_CEIL_EXPR,
	TNK_FIX_FLOOR_EXPR,
	TNK_FIX_ROUND_EXPR, /*��ת������ͬ����������������������ת������ֻ��1���������� */
	TNK_FLOAT_EXPR,   /*������תΪʵ����ֻ��1���������� */
	TNK_NEGATE_EXPR,  /*һԪ�󷴲�������������������ͬһ���͡� */
	TNK_MIN_EXPR,    /*����Сֵ�������������������� */
	TNK_MAX_EXPR,    /*�����ֵ�������������������� */
	TNK_ABS_EXPR,    /*�����ֵ��������һ���������� */
	TNK_FFS_EXPR,    /*FFS��������һ���������� */
	TNK_LSHIFT_EXPR,
	TNK_RSHIFT_EXPR,  /*�ֱ��ʾ���ƺ����Ʋ����������������������������Ϊ�޷������ͣ���Ϊ�߼���λ������Ϊ������λ���ڶ��������������ƶ���λ����������SImode�������ģʽ���һ��������һ�¡� ATE_EXPR RROTATE_EXPR �ֱ��ʾѭ�����ƺ�ѭ�����ƣ�����˵��ͬ�ϡ� */
	TNK_BIT_IOR_EXPR,  /*λͬ���������A|B��A��B��������������*/
	TNK_BIT_XOR_EXPR,  /*λ����������A��B�� BIT_AND_EXPR λ���������A&B�� */
	TNK_BIT_AND_EXPR,  /*λ���������A&B�� BIT_NOT_EXPR λ�ǲ�����*/
	TNK_BIT_NOT_EXPR,  /*~��һԪ�������������ҽ���ԣ������þ��ǽ���������ÿһλ��ת�������������~1100B=0011B������ʮ���ƾ���~12D=3D*/
	TNK_TRUTH_ANDIF_EXPR,
	TNK_TRUTH_ORIT_EXPR, /*��ʾ�߼�����߼������������ܴӵ�һ����������ֵ���Щ���ʽ��ֵ���򲻼���ڶ�����������ֵ�� */
	TNK_TRUTH_AND_EXPR,
	TNK_TRUTH_OR_EXPR,
	TNK_TRUTH_XOR_EXPR,
	TNK_TRUTH_NOT_EXPR, /*�ֱ��ʾ�߼��롢�����ͷǲ����������Ƿ���Ҫ�ڶ�����������ֵ�����Ǽ���ڶ����������� ��Щ���ʽ��TRUTH_NOT_EXPRֻ��һ���������⣬���඼��������������������Ϊ����ֵ��ֻȡ�����������ֵ��*/
	TNK_LT_EXPR,
	TNK_LE_EXPR,
	TNK_GT_EXPR,
	TNK_GE_EXPR,
	TNK_EQ_EXPR,
	TNK_NGT_EXPR,
	TNK_NLT_EXPR,
	TNK_NEQ_EXPR,     /*�ֱ��ʾ����������֮���С�ڡ�С�ڵ��ڡ����ڡ����ڵ��ڡ����ںͲ����ڲ����� ����EQ_EXPR��NE_EXPR������κ����ͽ��в���������ֻ��������͡�ָ�롢ö�ٻ�ʵ�ͽ��в�������������£������������������ͬ���͡�����ǲ����͡�*/
	TNK_ORDERED_EXPR,
	TNK_UNORDERED_EXPR, /*Additional relational operators for floating point unordered */
	TNK_UNEQ_EXPR,
	TNK_UNGE_EXPR,
	TNK_UNGT_EXPR,
	TNK_UNLT_EXPR,
	TNK_UNLE_EXPR,    /*These are equivalent to unordered or These are equivalent to unordered*/
	TNK_IN_EXPR,
	TNK_SET_LE_EXPR,
	TNK_CARD_EXPR,
	TNK_RANGE_EXPR,   /*��PASCAL�еļ��Ͻ��еĲ�����Ŀǰ��ûʹ�á�*/
	TNK_CONVERT_EXPR,  /*��һ��ֵ�����ͽ���ת�������е�ת����������ʽ��ת�����������ʾ��CONVERT_EXPR��ֻ��һ���������� */
	TNK_NOP_EXPR,    /*��ʾһ���������κδ����ת����ֻ��һ����������*/
	TNK_NON_LVALUE_EXP, /*��ʾһ�����������ͬ��ֵ������֤��ֵ��Ϊ��ֵ��ֻ��һ����������*/
	TNK_VIEW_CONVERT_EXPR, /*��ʾ������ĳ�����͵Ķ��󿴳ɾ�����һ�����͡�����������ʾ�ĺ����Ӧ��ADA�����е�"Unchecked Conversion"�����¶�Ӧ��C�����е�*(type2 *)&X��������Ψһ�Ĳ������ǽ������ɾ�����һ�����͵��Ǹ�ֵ������������ͱ��ʽ�����ʹ�С��ͬ����˲����޶��壨It is undefined if the type of the input and of the expression have different sizes�����˽�����Ҳ������ΪMODIFY_EXPR��LHS����ʱ��û�����������ݸ�ֵ���������������������Ӧ������TREE_ADDRESSABLE��*/
	TNK_SAVE_EXPR,   /*��ʾ����һ�ε�����õ��ı��ʽ����һ�����������Ǵ˱��ʽ���ڶ���������ָ��һ��������������SAVE_EXPR�ڴ˺����в�����������������Ϊ���ʽ��Ӧ��RTL��ֻ�е��˱��ʽ�������Ժ�RTL��Ų�Ϊ��.*/
	TNK_UNSAVE_EXPR,  /*UNSAVE_EXPR���ʽ�Ĳ�����0����unsave���Ǹ�ֵ�����ǿ��Խ����е�_EXPR�����TARGET_EXPRs�� SAVE_EXPRs�� CALL_EXPRs ��RTL_EXPRs����Щ����ֵһ�Σ��������ι�ֵ���ı��ʽ��������ΪUNSAVE_EXPR��ʹ�ö���Щ���ʽ��һ���µ�expand_expr�����ܵ��¶���Щ���ʽ�����¹�ֵ�����������ڲ�ͬ�ĵط�����һ�������б���������չʱ���˱��ʽ�ǳ����á�*/
	TNK_RTL_EXPR,    /*��ʾһ����RTL�ѱ���չ��һ�����У��ҵ����ʽ����չʱ��Ӧ�����������е�һ�����ʽ�� ��һ��������ָ�򽫱�������RTL������insn�����еĵ�һ� �ڶ�������ָ���ʾ�����RTL���ʽ*/
	TNK_ADDR_EXPR,   /*C�е� & ��������ֵ�ǲ�����ֵ���ڵ�λ�ĵ�ַ���������ɾ�����һģʽ�����ģʽΪPmode�� */
	TNK_REFERENCE_EXPR, /*��ʾһ������ֵ���û�ָ��һ�������ָ�롣 */
	TNK_ENTRY_VALUE_EXPR, /*������Щ��Ҫ��̬����������ʹ�á���������һ�����������������һ����Epmode�ĺ�������ֵ�� */
	TNK_FDESC_EXPR,   /*������0���������������part N of a function descriptor of type ptr_mode */
	TNK_COMPLEX_EXPR,  /*����������ͬ���͵�ʵ�ͻ����Ͳ�����������һ��������Ӧ�������͵ĸ���ֵ�� */
	TNK_CONJ_EXPR,   /*ȡ�������ĸ�������Ը����Ͳ�����ֵ�������������ͬ�����͡� */
	TNK_REALPART_EXPR,
	TNK_IMAGPART_EXPR, /*�ֱ��ʾ������ʵ�����鲿��ֻ�Ը������ݽ��в�����*/ 
	TNK_PREDECREMENT_EXPR,
	TNK_PREINCREMENT_EXPR,
	TNK_POSTDECREMENT_EXPR,
	TNK_POSTINCREMENT_EXPR, /*�ֱ��Ӧ��C�е�++�ͣ������������������������ڶ���������˵��ÿ�������ĳ̶ȣ���ָ����ԣ��ڶ���������Ϊ��ָ����Ĵ�С��*/
	TNK_VA_ARG_EXPR,  /*����ʵ�� `va_arg'.*/
	TNK_RY_CATCH_EXPR, /*�Բ�����1���й�ֵ�����ҽ����ڴ˹�ֵ������throwһ���쳣ʱ���ŶԲ�����2���й�ֵ������WITH_CLEANUP_EXPR��ͬ�ĵط����ڳ���throwһ���쳣��������Զ���Բ�����2���й�ֵ��*/
	TNK_TRY_FINALLY_EXPR, /*�Բ�����1���й�ֵ��������2��һ��cleanup���ʽ���ڴӴ˱��ʽ�˳����������쳣��������֮ǰ����ֵ���˲���������CLEANUP_POINT_EXPR��WITH_CLEANUP_EXPR�Ľ�ϣ���CLEANUP_POINT_EXPR��WITH_CLEANUP_EXPR������cleanup���ʽ����������ĵء���  TRY_FINALLY_EXPR �����һ����ת��ת��һ��cleanup���̡���cleanup �ǵ�ǰ�����е�ʵ����䣨���û����ڴ����öϵ㣩ʱ��Ӧ��ʹ��TRY_FINALLY_EXPR��*/
	TNK_GOTO_SUBROUTINE_EXPR, /*����ʵ��TRY_FINALLY_EXPR �е�cleanups������expand_expr����ǰ�˲�����������0�Ǵ������ǽ�Ҫ���õĹ��̵Ŀ�ʼ����rtx��������1����洢���̷��ص�ַ�ı�����rtx��  ������Щ���ʽ�������õ�ֵ�������и����á�*/
	TNK_LABEL_EXPR,   /*��װ������һ����Ŷ��壬Ψһ��һ��������ΪLABEL_DECL��㡣�˱��ʽ�����ͱ���Ϊ�գ���ֵ������*/
	TNK_GOTO_EXPR,   /*��ʾGOTO��䡣Ψһ��һ��������ΪLABEL_DECL��㡣���ʽ�����ͱ���Ϊ�գ���ֵ������*/
	TNK_RETURN_EXPR,  /*��ʾreturn��䣬��Ψһ��һ�����������й�ֵ��Ȼ��ӵ�ǰ�������ء����ʽ�����ͱ���Ϊ�գ���ֵ������*/
	TNK_EXIT_EXPR,   /*��ʾ���ڲ�ѭ���������˳���Ψһ��һ��������Ϊ�˳������������ʽ�����ͱ���Ϊ�գ���ֵ������*/
	TNK_LOOP_EXPR,   /*��ʾһ��ѭ����Ψһ��һ����������ʾѭ�����塣�˱��ʽ�������EXIT_EXPR�����򣬱�ʾ����һ������ѭ���� ���ʽ�����ͱ���Ϊ�գ���ֵ������*/
	TNK_LABELED_BLOCK_EXPR, /*��ʾһ������ŵĿ飬������0�Ǳ�ʾ������ı�ţ�������1��ʾ�顣*/
	TNK_EXIT_BLOCK_EXPR,/*��ʾ�˳�һ������ŵĿ飬���ܻ᷵��һ��ֵ��������0��һ�����˳���LABELED_BLOCK_EXPR��������1�Ƿ���ֵ������ֵ����Ϊ��*/
	TNK_EXPR_WITH_FILE_LOCATION, /*��һ������㣨ͨ����һ�����ʽ��ע����Դ����λ����Ϣ���ļ�����(EXPR_WFL_FILENAME)������(EXPR_WFL_LINENO)������(EXPR_WFL_COLNO)������չ����ͬEXPR_WFL_NODE�������EXPR_WFL_EMIT_LINE_NOTE�����������emitһ����ע����Ϣ������������������JAVAǰ����ʹ�á�*/
	TNK_SWITCH_EXPR,  /*Switch���ʽ��������0������ִ�з�֦�ı��ʽ��������1����CASEֵ������֯��ʽ��ǰ��ʵ����ء�*/
	TNK_EXC_PTR_EXPR,  /*The exception object from the runtime*/
	TNK_QUESTION_MARK_EXPR,  /*c��������������������?*/
	TNK_COMMA_EXPR,   //���������

	//��C��C++��ص���������
	TNK_SRCLOC,     /*��סԴ����λ�õ�һ����㡣*/
	TNK_SIZEOF_EXPR,
	TNK_ARROW_EXPR,
	TNK_ALIGNOF_EXPR,
	TNK_EXPR_STMT,   /*��ʾһ�����ʽ��䣬��EXPR_STMT_EXPR����ȡ���ʽ�� */
	TNK_COMPOUND_STMT, /*��ʾһ���ɴ������������Ŀ飬������ΪCOMPOUND_BODY�� */
	TNK_PARA_STMT,   /*��ʾ����˵����� */
	TNK_DECL_STMT,   /*��ʾ�ֲ�˵����䣬������ΪDECL_STMT_DECL�� */
	TNK_IF_STMT,    /*��ʾIF��䣬�������ֱ�Ϊ IF_COND��THEN_CLAUSE�� ELSE_CLAUSE��*/ 
	TNK_FOR_STMT,    /*��ʾfor��䣬�������ֱ�ΪFOR_INIT_STMT��FOR_COND��FOR_EXPR��FOR_BODY�� */
	TNK_WHILE_STMT,   /*��ʾwhile��䣬�������ֱ�ΪWHILE_COND��WHILE_BODY�� */
	TNK_DO_STMT,    /*��ʾdo��䣬�������ֱ�ΪDO_BODY��DO_COND�� */
	TNK_RETURN_STMT,  /*��ʾreturn��䣬������ΪRETURN_EXPR�� */
	TNK_BREAK_STMT,   /*��ʾbreak��䡣 */
	TNK_CONTINUE_STMT, /*��ʾcontinue��䡣 */
	TNK_SWITCH_STMT,  /*��ʾswitch��䣬�������ֱ�ΪSWITCH_COND��SWITCH_BODY��SWITCH_TYPE��*/
	TNK_GOTO_STMT,   /*��ʾgoto��䣬������ΪGOTO_DESTINATION�� */
	TNK_LABEL_STMT,   /*��ʾlabel��䣬������ΪLABEL_DECL����ͨ����LABEL_STMT_LABEL������ */
	TNK_ASM_STMT,    /*��ʾһ��inline�����䡣*/
	TNK_SCOPE_STMT,   /*��ʾһ��scope�Ŀ�ʼ����������SCOPE_BEGIN_P holds�����ʾscope�Ŀ�ʼ�����SCOPE_END_P holds�����ʾscope�Ľ��������SCOPE_NULLIFIED_P holds �����ʾ��scope��û�б�����SCOPE_STMT_BLOCK ��ʾ������scope�ж���ı�����BLOCK�� */
	TNK_FILE_STMT,   /*��ʾһ��spot where a function changes files����û���������壬FILE_STMT_FILENAME ��ʾ�ļ������֡� */
	TNK_DEFAULT_LABEL,   
	TNK_CASE_LABEL,   /*��ʾCASE�еı�ţ��������ֱ�ΪCASE_LOW��CASE_HIGH�����CASE_LOWΪNULL_TREE����ʾ���Ϊdefault�����CASE_HIGHΪNULL_TREE����ʾ�����һ��������case��š�CASE_LABEL_DECL����˽���LABEL_DECL*/
	TNK_STMT_EXPR,   /*��ʾһ�������ʽ��STMT_EXPR_STMT ��ʾ�ɱ��ʽ��������䡣 */
	TNK_COMPOUND_LITERAL_EXPR, /*��ʾC99 compound literal��COMPOUND_LITERAL_EXPR_DECL_STMT is the a DECL_STMT containing the decl for the anonymous object represented by the COMPOUND_LITERAL; the DECL_INITIAL of that decl is the CONSTRUCTOR that initializes the compound literal.*/
	TNK_CLEANUP_STMT,  /*marks the point at which a declaration is fully  constructed. If, after this point, the CLEANUP_DECL goes out of  scope, the CLEANUP_EXPR must be run.*/
	TNK_TXT_CONSTANTS_TABLE,//�����г��ֵ������ַ�������,��Ϊ���ǲ��ܱ����뵽����ָ����,���Ա���洢��ӳ���ļ���ָ����  

	TNK_READ_STMT,
	TNK_WRITE_STMT,
	TNK_PRINTF_STMT
};

enum NODE_COMMON_ATTRIBUTE         //�﷨���ڵ��������
{
	NCA_NONE       = 0,
	NCA_SIDE_EFFECTS_FLAG = 1,
	NCA_CONSTANT_FLAG  = 2,
	NCA_ADDRESSABLE_FLAG = 4,
	NCA_VOLATILE_FLAG  = 8,
	NCA_READONLY_FLAG  = 0x10,
	NCA_ASM_WRITTEN_FLAG = 0x40,
	NCA_NOWARNING_FLAG  = 0x80,
	NCA_USED_FLAG    = 0x100,
	NCA_NOTHROW_FLAG   = 0x200,
	NCA_STATIC_FLAG   = 0x400,
	NCA_PUBLIC_FLAG   = 0x800,
	NCA_PRIVATE_FLAG   = 0x1000,
	NCA_PROTECTED_FLAG  = 0x2000,
	NCA_DEPRECATED_FLAG = 0x40000,
	NCA_INVARIANT_FLAG  = 0x80000,
	NCA_LANG_FLAG_0   = 0x100000,
	NCA_LANG_FLAG_1   = 0x200000,
	NCA_LANG_FLAG_2   = 0x400000,
	NCA_LANG_FLAG_3   = 0x800000,
	NCA_LANG_FLAG_4   = 0x1000000,
	NCA_LANG_FLAG_5   = 0x2000000,
	NCA_LANG_FLAG_6   = 0x4000000,
	NCA_VISITED     = 0x8000000
};

class CSyntaxTreeNode
{
public:
	CSyntaxTreeNode(TREE_NODE_TYPE nNodeKind = TNK_UNKNOWN) 
		: m_pParent(NULL)
		, m_pReferences( NULL )
		, m_pChain( NULL )
		, m_pDataType(NULL)
		, m_nLineIndex( 0 )
		, m_nNodeType(nNodeKind)
		, m_nTokenType(TT_NULL_TYPE)
		, m_nCommonAttribute(NCA_NONE)
		, m_pParser(NULL)
		, m_nOffset(0)

	{  
	}
	virtual ~CSyntaxTreeNode() 
	{
	}

	friend void CParserBase::AppendRef(CSyntaxTreeNode* pNodeReferenced,CSyntaxTreeNode* pSyntaxTreeNode);

public:
	virtual LONGLONG GetValue1()                 { return 0; }
	virtual double GetValue2()                   { return 0; }
	virtual CStringA GetValue3()                 { return ""; }

	virtual CSyntaxTreeNode* GetReal()           { return NULL;}
	virtual CSyntaxTreeNode* GetImag()           { return NULL;}

	virtual UINT GetUID()                        { return 0;}
	virtual CIdentifierTreeNode* GetName()       { return NULL; }

	virtual CSyntaxTreeNode* GetPurpose()        { return NULL;}
	virtual CSyntaxTreeNode* GetValue()          { return NULL;}

	virtual UINT GetSize()                       { return 0;}
	virtual CSyntaxTreeNode* GetArguments()      { return NULL;}
	virtual UINT GetComplexity()                 { return 0;}

	virtual void AppendDeclAttribute(UINT /*nAttribute*/) {}
	virtual BOOL IsUnsigned()                    { return FALSE;}
public:
	void AppendCommonAttribute(UINT nAttribute)  { m_nCommonAttribute |= nAttribute;} 
	void RemoveCommonAttribute(UINT nAttribute)  { m_nCommonAttribute -= (nAttribute & m_nCommonAttribute);} 
	BOOL IsNumericConstants()                    { return (TNK_INTEGER_CST ==m_nNodeType) || (TNK_REAL_CST ==m_nNodeType);}
	BOOL IsVariable()                            { return (TNK_CONST_DECL ==m_nNodeType) || (TNK_VAR_DECL ==m_nNodeType) || (TNK_PARM_DECL ==m_nNodeType);}
	BOOL IsExpression();
	BOOL IsCmpExpression();
	BOOL IsArithmeticExpression();
	BOOL IsInterger();
	BOOL IsFloat();
	BOOL IsNumeric();
	BOOL Ismodifiable();
public: 
	CSyntaxTreeNode* GetLastChainNode();
	LPCSTR GetSymbol();
protected:
	CSyntaxTreeNode* AddRef(CSyntaxTreeNode* pSyntaxTreeNode);
private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pParent,Parent);     //���׽ڵ�
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pReferences,References); //���øýڵ�ýڵ��б�
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pChain,Chain);     //������ֵܽڵ�
	DECLARE_MEMBER_AND_METHOD(CDataTypeTreeNode*,m_pDataType,DataType); //������ڵ�
	DECLARE_MEMBER_AND_METHOD(int,m_nLineIndex,LineIndex);       //��Դ���������ڵ�����
	DECLARE_MEMBER_AND_METHOD(TREE_NODE_TYPE,m_nNodeType,NodeType);   //�ڵ�����
	DECLARE_MEMBER_AND_METHOD(AXC_SYMBOL_TYPE,m_nTokenType,TokenType); //��Ӧ�ķ���
	DECLARE_MEMBER_AND_METHOD(UINT,m_nCommonAttribute,CommonAttribute); //�ο�ö��"NODE_COMMON_ATTRIBUTE"
	DECLARE_MEMBER_AND_METHOD(CParser*,m_pParser,Parser);        //�ο�ö��"NODE_COMMON_ATTRIBUTE"

	DECLARE_MEMBER_AND_METHOD(int,m_nOffset,Offset);          //���ڴ��봴��,�洢������̬����������ӳ���ļ��е�ƫ��
};

class CConstTreeNode :public CSyntaxTreeNode
{  
public:
	CConstTreeNode(TREE_NODE_TYPE nNodeKind)
		: CSyntaxTreeNode(nNodeKind)
		, m_nRef(0)
	{
	}
public:
	virtual CConstTreeNode* operator=( double /*pFactor*/)     { return this;}
	virtual CConstTreeNode* operator=( CConstTreeNode* /*pFactor*/) { return this;}

	virtual CConstTreeNode* operator+=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator-=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator*=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator/=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator%=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator<<=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator>>=( CConstTreeNode* /*pFactor*/) { return this;}

	virtual CConstTreeNode* operator<( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator<=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator>( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator>=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator==( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator!=( CConstTreeNode* /*pFactor*/) { return this;}

	virtual CConstTreeNode* operator&=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator^=( CConstTreeNode* /*pFactor*/) { return this;}
	virtual CConstTreeNode* operator|=( CConstTreeNode* /*pFactor*/) { return this;}
public:
	void AddRef()          { m_nRef++; }
	void Release()          { m_nRef--; }
private:
	DECLARE_MEMBER_AND_METHOD(int,m_nRef,Ref);           //����ר�õ����ü���  
};

class CConstIntTreeNode :public CConstTreeNode
{  
public:
	CConstIntTreeNode(LONGLONG nValue = 0)
		: CConstTreeNode(TNK_INTEGER_CST)
		, m_nValue(nValue)
	{
	}
public:
	virtual UINT GetSize()             { return 8;} 
	virtual CConstIntTreeNode* operator=( double pFactor)
	{ 
		m_nValue = (LONGLONG)pFactor;
		return this;
	}
	virtual CConstIntTreeNode* operator=( CConstTreeNode* pFactor)
	{ 
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue = pFactor->GetValue1();   
			}
			else
			{
				m_nValue = LONGLONG(pFactor->GetValue2());
			}
		}     
		return this;
	}
	virtual CConstIntTreeNode* operator+=( CConstTreeNode* pFactor)
	{
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue += pFactor->GetValue1();   
			}
			else
			{
				m_nValue = LONGLONG(m_nValue + pFactor->GetValue2());
			}
		}        

		return this;
	}
	virtual CConstIntTreeNode* operator-=( CConstTreeNode* pFactor)
	{
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue -= pFactor->GetValue1();   
			}
			else
			{
				m_nValue = LONGLONG(m_nValue - pFactor->GetValue2());
			}
		}        

		return this;
	}

	virtual CConstIntTreeNode* operator*=( CConstTreeNode* pFactor)
	{
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue *= pFactor->GetValue1();   
			}
			else
			{
				m_nValue = LONGLONG(m_nValue * pFactor->GetValue2());
			}
		}        

		return this;
	}
	virtual CConstIntTreeNode* operator/=( CConstTreeNode* pFactor) 
	{ 
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue /= pFactor->GetValue1();   
			}
			else if(LONGLONG(pFactor->GetValue2()) > 0)
			{
				m_nValue /= LONGLONG(pFactor->GetValue2());
			}
			else
			{
				throw CParserException(("Divisor can not be 0!"));
			}
		}   

		return this;
	}
	virtual CConstIntTreeNode* operator%=( CConstTreeNode* pFactor) 
	{
		if(pFactor->GetValue1() > 0)
		{
			m_nValue %= pFactor->GetValue1();   
		}
		else if(LONGLONG(pFactor->GetValue2()) > 0)
		{
			m_nValue %= LONGLONG(pFactor->GetValue2());
		}
		else
		{
			throw CParserException(("Divisor can not be 0!"));
		}

		return this;
	}
	virtual CConstIntTreeNode* operator<<=( CConstTreeNode* pFactor)     
	{ 
		m_nValue <<= pFactor->GetValue1();
		return this;
	}
	virtual CConstIntTreeNode* operator>>=( CConstTreeNode* pFactor)
	{ 
		m_nValue >>= pFactor->GetValue1();
		return this;
	}

	virtual CConstIntTreeNode* operator<( CConstTreeNode* pFactor) 
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue < pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue < pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstIntTreeNode* operator<=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue <= pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue <= pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstIntTreeNode* operator>( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue > pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue > pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstIntTreeNode* operator>=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue >= pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue >= pFactor->GetValue2())? 1:0;
		}
		return this;
	}

	virtual CConstIntTreeNode* operator&=( CConstTreeNode* pFactor)
	{ 
		m_nValue &= pFactor->GetValue1();
		return this;
	}
	virtual CConstIntTreeNode* operator^=( CConstTreeNode* pFactor)
	{ 
		m_nValue ^= pFactor->GetValue1();
		return this;
	}
	virtual CConstIntTreeNode* operator|=( CConstTreeNode* pFactor)
	{ 
		m_nValue |= pFactor->GetValue1();
		return this;
	}

	virtual CConstIntTreeNode* operator==( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue == pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue == pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstIntTreeNode* operator!=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue != pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue != pFactor->GetValue2())? 1:0;
		}
		return this;
	}
public:
	virtual LONGLONG GetValue1()           { return m_nValue; }
private:
	DECLARE_MEMBER_AND_METHOD(LONGLONG,m_nValue,Value);
};


class CConstRealTreeNode: public CConstTreeNode
{ 
public:
	CConstRealTreeNode(double nValue = 0.0)
		: CConstTreeNode(TNK_REAL_CST)
		, m_nValue(nValue)
	{
	}
public:
	virtual UINT GetSize()             { return 8;} 
	virtual CConstRealTreeNode* operator=( double pFactor)
	{ 
		m_nValue = pFactor;
		return this;
	}

	virtual CConstRealTreeNode* operator=( CConstTreeNode* pFactor)
	{ 
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue = (double)pFactor->GetValue1();   
			}
			else
			{
				m_nValue = double(pFactor->GetValue2());
			}
		}     
		return this;
	}

	virtual CConstRealTreeNode* operator+=( CConstTreeNode* pFactor)
	{
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue += pFactor->GetValue1();   
			}
			else
			{
				m_nValue += pFactor->GetValue2();
			}
		}        

		return this;
	}
	virtual CConstRealTreeNode* operator-=( CConstTreeNode* pFactor) 
	{ 
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue -= pFactor->GetValue1();   
			}
			else if(pFactor->GetValue2() > 0)
			{
				m_nValue -= pFactor->GetValue2();
			}
			else
			{
				throw CParserException(("Divisor can not be 0!"));
			}
		}   

		return this;
	}
	virtual CConstRealTreeNode* operator*=( CConstTreeNode* pFactor)
	{
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue *= pFactor->GetValue1();   
			}
			else
			{
				m_nValue *= pFactor->GetValue2();
			}
		}        

		return this;
	}
	virtual CConstRealTreeNode* operator/=( CConstTreeNode* pFactor) 
	{ 
		ATLASSERT(NULL != pFactor);   
		if(NULL != pFactor)
		{
			if(pFactor->GetValue1() > 0)
			{
				m_nValue /= pFactor->GetValue1();   
			}
			else if(pFactor->GetValue2() > 0)
			{
				m_nValue /= pFactor->GetValue2();
			}
			else
			{
				throw CParserException(("Divisor can not be 0!"));
			}
		}   

		return this;
	}
	virtual CConstRealTreeNode* operator%=( CConstTreeNode* pFactor) 
	{
		if(pFactor->GetValue1() > 0)
		{
			m_nValue = double(int(m_nValue) % pFactor->GetValue1());   
		}
		else if((pFactor->GetValue2()) > 0)
		{
			m_nValue = int(m_nValue) % int(pFactor->GetValue2());
		}
		else
		{
			throw CParserException(("Divisor can not be 0!"));
		}

		return this;
	}

	virtual CConstRealTreeNode* operator<( CConstTreeNode* pFactor) 
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue < pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue < pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstRealTreeNode* operator<=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue <= pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue <= pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstRealTreeNode* operator>( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue > pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue > pFactor->GetValue2())? 1:0;
		}
		return this;
	}
	virtual CConstRealTreeNode* operator>=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue >= pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue >= pFactor->GetValue2())? 1:0;
		}
		return this;
	}

	virtual CConstRealTreeNode* operator==( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue == pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue == pFactor->GetValue2())? 1:0;
		}
		return this;
	}

	virtual CConstRealTreeNode* operator!=( CConstTreeNode* pFactor)
	{ 
		if(pFactor->GetNodeType() == TNK_INTEGER_CST)
		{
			m_nValue = (m_nValue != pFactor->GetValue1())? 1:0;
		}
		else
		{
			m_nValue = (m_nValue != pFactor->GetValue2())? 1:0;
		}
		return this;
	}
public:
	virtual double GetValue2()            { return m_nValue; }
private:
	DECLARE_MEMBER_AND_METHOD(double,m_nValue,Value);
};

class CConstStringTreeNode: public CConstTreeNode
{ 
public:
	CConstStringTreeNode(char* pValue = NULL)
		: CConstTreeNode(TNK_STRING_CST)
		, m_strValue(pValue)

	{}
public:
	virtual UINT GetSize()             { return m_strValue.GetLength();} 
public:
	virtual CStringA GetValue3()           { return m_strValue; }
private:
	DECLARE_MEMBER_AND_METHOD(CStringA,m_strValue,Value);
};

class CComplexTreeNode: public CSyntaxTreeNode
{
public:
	CComplexTreeNode()
		: m_pReal(NULL)
		, m_pImag(NULL)
	{
	}
	virtual ~CComplexTreeNode()
	{
	}
	virtual CSyntaxTreeNode* GetReal()       { return m_pReal;}
	virtual CSyntaxTreeNode* GetImag()       { return m_pImag;}
private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pReal,Real);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pImag,Imag);
};

class CIdentifierTreeNode: public CSyntaxTreeNode
{
public:
	CIdentifierTreeNode(UINT nID = 0,char* pTitle = NULL)
		: m_nID(nID)
		, m_strTitle(pTitle)
	{
	}
private:
	DECLARE_MEMBER_AND_METHOD(UINT,m_nID,ID);
	DECLARE_MEMBER_AND_METHOD(CStringA,m_strTitle,Title);
};


class CListTreeNode: public CSyntaxTreeNode
{
public:
	CListTreeNode()
		: m_pPurpose(NULL)
		, m_pValue(NULL)
	{
	}
	virtual ~CListTreeNode()
	{
	}
public:
	virtual CSyntaxTreeNode* GetPurpose()      { return m_pPurpose;}
	virtual CSyntaxTreeNode* GetValue()       { return m_pValue;}
private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pPurpose,Purpose);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pValue,Value);
};


class CRefListTreeNode: public CSyntaxTreeNode
{
public:
	CRefListTreeNode(CSyntaxTreeNode* pValue = NULL)
		: m_pValue(pValue)
	{
	}
	virtual ~CRefListTreeNode()
	{
	}
private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pValue,Value);
};


class CVectorTreeNode: public CSyntaxTreeNode
{ 
public:
	CVectorTreeNode(UINT nCount = 0,CSyntaxTreeNode* pValue = NULL)
		: CSyntaxTreeNode(TNK_VECTOR_TYPE)
		, m_nCount(nCount)
		, m_pValue(pValue)
	{
	}
	~CVectorTreeNode()
	{
	}
public:
	virtual CSyntaxTreeNode* GetValue()       { return m_pValue;}
	virtual UINT GetSize();
private:
	DECLARE_MEMBER_AND_METHOD(UINT,m_nCount,Count);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pValue,Value);
};

class CTxtConstantsTreeNode: public CSyntaxTreeNode
{ 
public:
	CTxtConstantsTreeNode()
		: CSyntaxTreeNode(TNK_TXT_CONSTANTS_TABLE)
	{
	}
public:
	UINT GetConstantsCount()         { return m_arrConstants.size();}
	CConstTreeNode* GetConstantNode(UINT nIndex);
	void AppendConstantNode(CConstTreeNode* pConstTreeNode);
	void RmoveConstantNode(CSyntaxTreeNode* pConstTreeNode);
private:
	SYNTAX_TREE_NODE_ARRAY  m_arrConstants;
};

class CExpressionTreeNode: public CSyntaxTreeNode
{
public:
	CExpressionTreeNode(TREE_NODE_TYPE nNodeKind = TNK_UNKNOWN)
		: CSyntaxTreeNode(nNodeKind)
	{
		memset(m_arrOperands,0,sizeof(m_arrOperands));
	}
	virtual ~CExpressionTreeNode()
	{
	}
public:
	CSyntaxTreeNode* GetChildNode(UINT nIndex)   { return (nIndex < MAX_CHILDREN_NUMBER)? m_arrOperands[nIndex]:NULL;}
	void SetChildNode(UINT nIndex,CSyntaxTreeNode* pChild)
	{
		if(nIndex < MAX_CHILDREN_NUMBER)
		{
			m_arrOperands[nIndex] = pChild;
			if(NULL != pChild)
			{
				pChild->SetParent(this);
			}
		}
	}

	virtual UINT GetComplexity()
	{
		UINT nComplexity = 0;

		if(this->IsArithmeticExpression() || this->IsCmpExpression())
		{
			nComplexity = 1;
		}

		for(UINT i = 0;i < MAX_CHILDREN_NUMBER;i++)
		{
			if(NULL == m_arrOperands[i])
			{
				break;
			}
			nComplexity += m_arrOperands[i]->GetComplexity();
		}

		return nComplexity;
	}
	virtual BOOL IsUnsigned();
private:
	CSyntaxTreeNode*  m_arrOperands[MAX_CHILDREN_NUMBER];
};


class CBlockTreeNode: public CSyntaxTreeNode
{ 
public:
	CBlockTreeNode()
		: m_nHandlerBlockFlag(0)
		, m_nAbstractFlag(0)
		, m_nBlockNum(0)
		, m_pVars(NULL)
		, m_pTypeTags(NULL)
		, m_pSubblocks(NULL)
		, m_pSupercontext(NULL)
		, m_pSbstractOrigin(NULL)
		, m_pFragmentOrigin(NULL)
		, m_pFragmentChain(NULL)
	{
	}

private:
	DECLARE_MEMBER_AND_METHOD(BYTE,m_nHandlerBlockFlag,HandlerBlockFlag);
	DECLARE_MEMBER_AND_METHOD(BYTE,m_nAbstractFlag,AbstractFlag);
	DECLARE_MEMBER_AND_METHOD(DWORD,m_nBlockNum,BlockNum);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pVars,Vars);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pTypeTags,TypeTags);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSubblocks,Subblocks);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSupercontext,Supercontext);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSbstractOrigin,SbstractOrigin);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pFragmentOrigin,FragmentOrigin);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pFragmentChain,FragmentChain);
};

enum NODE_DECL_ATTRIBUTE         //�﷨���ڵ��������
{ 
	NDA_EXTERNAL_FLAG   = 0x1, 
	NDA_STATIC_FLAG    = 0x2, 
	NDA_REGDECL_FLAG    = 0x4, 
	NDA_INLINE_FLAG    = 0x8, 
	NDA_BIT_FIELD_FLAG   = 0x10, 
	NDA_VIRTUAL_FLAG    = 0x20, 
	NDA_IGNORED_FLAG    = 0x40, 
	NDA_ABSTRACT_FLAG   = 0x80, 
	NDA_IN_SYSTEM_HEADER_FLAG = 0x100, 
	NDA_COMMON_FLAG    = 0x200, 
	NDA_DEFER_OUTPUT    = 0x400, 
	NDA_TRANSPARENT_UNION = 0x800, 
	NDA_STATIC_CTOR_FLAG  = 0x1000, 
	NDA_STATIC_DTOR_FLAG  = 0x2000, 
	NDA_ARTIFICIAL_FLAG  = 0x4000, 
	NDA_WEAK_FLAG     = 0x8000, 
	NDA_NON_ADDR_CONST_P  = 0x10000, 
	NDA_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT = 0x20000, 
	NDA_COMDAT_FLAG    = 0x40000, 
	NDA_MALLOC_FLAG    = 0x80000, 
	NDA_NO_LIMIT_STACK   = 0x100000, 
	NDA_PURE_FLAG     = 0x200000, 

	NDA_NON_ADDRESSABLE  = 0x400000, 
	NDA_USER_ALIGN     = 0x800000, 
	NDA_UNINLINABLE    = 0x1000000, 
	NDA_UNSIGNED_FLAG   = 0x2000000,

	NDA_CONST_FLAG     = 0x4000000,
	NDA_LANG_FLAG_0    = 0x8000000,
	NDA_LANG_FLAG_1    = 0x10000000,
	NDA_LANG_FLAG_2    = 0x20000000,
	NDA_LANG_FLAG_3    = 0x40000000,
	NDA_LANG_FLAG_4    = 0x80000000,
};

class CDeclarationTreeNode: public CSyntaxTreeNode
{ 
public:
	CDeclarationTreeNode(TREE_NODE_TYPE nNodeKind = TNK_UNKNOWN)
		: CSyntaxTreeNode(nNodeKind)
		, m_nUID(0)
		, m_nFileNameID(0)
		, m_nMachineMode(0)
		, m_nDeclAttribute(0)
		, m_nPointerDepth(0)

		, m_pName(NULL)
		, m_pContext(NULL)
		, m_pArguments(NULL)
		, m_pResult(NULL)
		, m_pInitial(NULL)
		, m_pAbstractOrigin(NULL)
		, m_pAssemblerName(NULL)
		, m_pSectionName(NULL)
		, m_pAttributes(NULL)
		, m_pSavedTree(NULL)
	{
	}
	virtual ~CDeclarationTreeNode()
	{
	}

public:
	virtual UINT GetSize();
	virtual void AppendDeclAttribute(UINT nAttribute)  { m_nDeclAttribute |= nAttribute;} 
	virtual CIdentifierTreeNode* GetName()             { return m_pName; }
	virtual CSyntaxTreeNode* GetArguments()            { return m_pArguments;}
	virtual UINT GetUID()                              { return m_nUID;}
	virtual BOOL IsUnsigned();
private:
	DECLARE_MEMBER_AND_METHOD(int,m_nUID,UID);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nFileNameID,FileNameID);
	DECLARE_MEMBER_AND_METHOD(BYTE,m_nMachineMode,MachineMode);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nDeclAttribute,DeclAttribute);  //�ο�NODE_DECL_ATTRIBUTE
	DECLARE_MEMBER_AND_METHOD(int,m_nPointerDepth,PointerDepth);

	DECLARE_MEMBER_AND_METHOD(CIdentifierTreeNode*,m_pName,Name);   
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pContext,Context); /*��FIELD_DECLS����У�����ָ��RECORD_TYPE��UNION_TYPE��QUAL_UNION_TYPE��㣬˵����FIELD�����������е�һ����ɳ�Ա��
																	 ��VAR_DECL��PARM_DECL��FUNCTION_DECL��LABEL_DECL��CONST_DECL����У�����ָ������˷�������FUNCTION_DECL��㣬���˷��������С��ļ���Χ���������������ΪNULL_TREE��*/
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pArguments,Arguments);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pResult,Result);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pInitial,Initial); //������ֵ����Ϊ�����ʽ
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAbstractOrigin,AbstractOrigin);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAssemblerName,AssemblerName);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSectionName,SectionName);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAttributes,Attributes);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSavedTree,SavedTree);
};


enum NODE_TYPE_ATTRIBUTE         //�﷨���ڵ��������
{ 
	NTA_STRING_FLAG       = 0x1, 
	NTA_NO_FORCE_BLK_FLAG    = 0x2,
	NTA_NEEDS_CONSTRUCTING_FLAG = 0x3,
	NTA_TRANSPARENT_UNION_FLAG = 0x4,
	NTA_PACKED_FLAG       = 0x10,
	NTA_RESTRICT_FLAG      = 0x20,   


	NTA_LANG_FLAG_0 = 0x80,
	NTA_LANG_FLAG_1 = 0x100,
	NTA_LANG_FLAG_2 = 0x200,
	NTA_LANG_FLAG_3 = 0x400,
	NTA_LANG_FLAG_4 = 0x800,
	NTA_LANG_FLAG_5 = 0x1000,
	NTA_LANG_FLAG_6 = 0x2000,
	NTA_LANG_FLAG_7 = 0x4000,
	NTA_LANG_FLAG_8 = 0x8000,

	NTA_USER_ALIGN = 0x10000,
	NTA_INT_ALIGN  = 0x20000,
};

enum ALIGN_MODE     //���뷽ʽ
{
	AM_BYTE = 1,
	AM_WORD,
	AM_DWORD,
	AM_QWORD
};
class CDataTypeTreeNode: public CSyntaxTreeNode
{ 
public:
	CDataTypeTreeNode(TREE_NODE_TYPE nNodeKind = TNK_UNKNOWN,UINT nSize = 4)
		: CSyntaxTreeNode(nNodeKind)
		, m_nUID(0)
		, m_nBits(0)
		, m_nPrecision(0)
		, m_nTypeAttribute(0)
		, m_nAlign(AM_BYTE)
		, m_nPointerDepth(0)
		, m_nSize(nSize)
		, m_nMinval(0)
		, m_nMaxval(0)

		, m_pName(NULL)
		, m_pAttributes(NULL)
		, m_pValues(NULL)
		, m_pPointerTo(NULL)
		, m_pReferenceTo(NULL)
		, m_pNextVariant(NULL)
		, m_pMainVariant(NULL)
		, m_pBinfo(NULL)
		, m_pContext(NULL)

	{
		symtab.m_nAddress = 0;
	}
	virtual ~CDataTypeTreeNode()
	{
	}
public:
	CDataTypeTreeNode* GetPointerBaseType(int& nDepth)
	{
		nDepth = 0;
		CDataTypeTreeNode* pResult = this;
		while(pResult->GetNodeType() == TNK_POINTER_TYPE)
		{
			nDepth += pResult->GetPointerDepth();
			pResult = pResult->GetDataType();
		}

		return pResult;
	}
public:
	void AppendTypeAttribute(UINT nTypeAttribute)  { m_nTypeAttribute |= nTypeAttribute;}
	virtual UINT GetUID()              { return m_nUID;}
	virtual CIdentifierTreeNode* GetName()      { return m_pName; }
	virtual UINT GetSize()              { return m_nSize;}

private:
	DECLARE_MEMBER_AND_METHOD(UINT,m_nUID,UID);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nBits,Bits);
	DECLARE_MEMBER_AND_METHOD(BYTE,m_nPrecision,Precision);
	DECLARE_MEMBER_AND_METHOD(UINT,m_nTypeAttribute,TypeAttribute);  //�ο�NODE_DECL_ATTRIBUTE
	DECLARE_MEMBER_AND_METHOD(ALIGN_MODE,m_nAlign,Align);       
	DECLARE_MEMBER_AND_METHOD(BYTE,m_nPointerDepth,PointerDepth);  
	DECLARE_MEMBER_AND_METHOD(UINT,m_nSize,Size);          
	DECLARE_MEMBER_AND_METHOD(UINT,m_nMinval,Minval);        
	DECLARE_MEMBER_AND_METHOD(UINT,m_nMaxval,Maxval);         

	union
	{
		int  m_nAddress;
		char *m_pPointer;
	}symtab;

	DECLARE_MEMBER_AND_METHOD(CIdentifierTreeNode*,m_pName,Name);       //�����ɾ��
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAttributes,Attributes);   //�����ɾ��
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pValues,Values);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pPointerTo,PointerTo);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pReferenceTo,ReferenceTo);

	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pNextVariant,NextVariant);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pMainVariant,MainVariant);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pBinfo,Binfo);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pContext,Context);
};

class CLableRef
{
public:
	CLableRef(CSyntaxTreeNode* pGotoSMTM = NULL,char* pLableName = NULL)
		: m_pGotoSMTM(pGotoSMTM)
		, m_strName(pLableName)
	{
	}

private:
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pGotoSMTM,GotoSMTM); 
	DECLARE_MEMBER_AND_METHOD(CStringA,m_strName,Name); 	
};
typedef std::vector<CLableRef*>            LABLEREF_ARRAY;     
typedef std::vector<CLableRef*>::iterator  LABLEREF_ARRAY_ITR;  


class CParserContext
{
public:
	CParserContext(CSyntaxTreeNode* pFunctionDeclNode = NULL,CSaxCCompileimpl* pSaxCCompileimpl=NULL)
		: m_pFunctionDeclNode(pFunctionDeclNode)
		, m_pSaxCCompileimpl(pSaxCCompileimpl)
	{
	}
	~CParserContext();
public:
	CLableRef* AppenLableRef(CSyntaxTreeNode* pGotoSMTM,char* pLableName);
	void RelatedLableRef(CSyntaxTreeNode* pLableDeclNode);
private:
	DECLARE_MEMBER_AND_METHOD(CSaxCCompileimpl*,m_pSaxCCompileimpl,SaxCCompileimpl); 
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pFunctionDeclNode,FunctionDeclNode); //ָ���������ĺ����ڵ�
	LABLEREF_ARRAY              m_arrLableRefs;
};

extern TCHAR const  CBINDINGLEVEL_MEMORY_POOL_ID[];

class CBindingLevel
{
public:
	CBindingLevel(CParserContext* pContext = NULL);
	~CBindingLevel();
public:
	void AppendDeclNode(CSyntaxTreeNode* pDeclNode);
	CSyntaxTreeNode* LookupDeclNode(char* pName,BOOL bLable = FALSE);
	void AppendTagNode(CSyntaxTreeNode* pDeclNode);
	CSyntaxTreeNode* LookupTagNode(char* pName);
public:
	USE_CLASS_MEMORY_POOL(CBindingLevel,16,4,CBINDINGLEVEL_MEMORY_POOL_ID)
private:
	DECLARE_MEMBER_AND_METHOD(CParserContext*,m_pContext,Context); //ָ���������ĺ�������������
	SYNTAX_TREE_NODE_ARRAY  m_arrDeclNodes;             //ָ�����еı����������������Լ�typedef���͵�_DECL�ڵ�
	SYNTAX_TREE_NODE_ARRAY  m_arrTags;               //ָ�����еĽṹ�����Ϻ�ö�ٶ���
};

typedef std::vector<CBindingLevel*>                    BINDING_LEVEL_ARRAY;     
typedef std::vector<CBindingLevel*>::iterator          BINDING_LEVEL_ARRAY_ITR; 
typedef std::vector<CBindingLevel*>::reverse_iterator  BINDING_LEVEL_ARRAY_RTR; 




class CParser : public CParserBase
{
public:
	CParser(CSaxCCompileimpl* pSaxCCompileimpl,UINT nMode);
	virtual ~CParser();
public:
	void          SetScaner(CScaner* pScaner)  { m_pScaner = pScaner;}
public:
	CSyntaxTreeNode*    BuildSyntaxTree(char* const pSourceCode,int nSourceCodeLen);
	void          Trace( CSyntaxTreeNode* const pNode);
	void          Reset();
public:
	CBindingLevel*     CreateNewBindingLevel(CParserContext* pContext = NULL);
	void          PopBindingLevel();
	CSyntaxTreeNode*    LookupDeclNode(char* pName,BOOL bLable = FALSE);
	CSyntaxTreeNode*    LookupTagNode(char* pName);
protected:
	CConstIntTreeNode*   LookupConstIntTreeNode(LONGLONG nValue);
	CConstRealTreeNode*   LookupConstRealTreeNode(double nValue);
	CConstStringTreeNode*  LookupConstStringTreeNode(char* pValue);

	CConstIntTreeNode*   CreateConstIntTreeNode(LONGLONG nValue);
	CConstRealTreeNode*   CreateConstRealTreeNode(double nValue);
	CConstStringTreeNode*  CreateConstStringTreeNode(char* pValue);

	CIdentifierTreeNode*  GetIdentifier(char* pName);
	CIdentifierTreeNode*  LookupIdentifier(char* pName);
	CDataTypeTreeNode*   GetInternalDataType(AXC_SYMBOL_TYPE nTokenType,char* pName);
	CDataTypeTreeNode*   LookupInternalDataType(TREE_NODE_TYPE nTokenType);   
	CDataTypeTreeNode*   LookupPointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPointerDepth);   
	CDataTypeTreeNode*   LookupRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode);

	CDataTypeTreeNode*   GetPointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPointerDepth);   
	CDataTypeTreeNode*   GetArrayDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nElements);
	CDataTypeTreeNode*   GetRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode);


	CDeclarationTreeNode*  CreateDeclNode( TREE_NODE_TYPE nKind, CSymbolInfo* pCurrentToken);
	CDataTypeTreeNode*   CreateTypeNode( TREE_NODE_TYPE nKind, CSymbolInfo* pCurrentToken);
	CDeclarationTreeNode*  CreateIntConstDeclNode(TREE_NODE_TYPE nKind, char* pName,int nValue);
	CExpressionTreeNode*  CreateInternalConst();

	CExpressionTreeNode*  CreateStatementNode( TREE_NODE_TYPE nKind, CSymbolInfo* pCurrentToken);
	CExpressionTreeNode*  CreateExpressionNode( TREE_NODE_TYPE nKind,UINT nLineIndex = 0,AXC_SYMBOL_TYPE nSymbolType = TT_UNKNOWN);
	CExpressionTreeNode*  CreateExpressionNode(CSymbolInfo* pCurrentToken);
	CExpressionTreeNode*  CreateExpressionNode(CSymbolInfo* pCurrentToken,CDataTypeTreeNode* pDataType,CSyntaxTreeNode *pExprNode1,CSyntaxTreeNode *pExprNode2 = NULL);
	CExpressionTreeNode*  CreateExpressionNode(CSymbolInfo* pCurrentToken,AXC_SYMBOL_TYPE nDataType,char* lpName,CSyntaxTreeNode *pExprNode1,CSyntaxTreeNode *pExprNode2 = NULL);
	CExpressionTreeNode*  CreateExpressionNode(TREE_NODE_TYPE nKind,UINT nLineIndex,AXC_SYMBOL_TYPE nSymbolType,CDataTypeTreeNode* pDataType,CSyntaxTreeNode *pExprNode1,CSyntaxTreeNode *pExprNode2 = NULL);
	CExpressionTreeNode*  CreateExpressionNode(TREE_NODE_TYPE nKind,UINT nLineIndex,AXC_SYMBOL_TYPE nSymbolType,CSyntaxTreeNode *pExprNode1,CSyntaxTreeNode *pExprNode2);
	CExpressionTreeNode*  CreateExpressionNode(CSymbolInfo* pCurrentToken,CSyntaxTreeNode *pExprNode1,CSyntaxTreeNode *pExprNode2);



	void          Destroy(CSyntaxTreeNode* pNode);
	BOOL          Match( AXC_SYMBOL_TYPE nTokenType);
	void          ConsumeUntil( AXC_SYMBOL_TYPE nTokenType );
	void          ConsumeUntil( AXC_SYMBOL_TYPE nTokenType1, AXC_SYMBOL_TYPE nTokenType2 );
	CConstIntTreeNode*   ConvertConstRealToInt(CConstTreeNode* pSrc);
protected:
	UINT          AnalysePrefixAtti();
	CDataTypeTreeNode*   AnalyseDataType(UINT nPrefixAttr);
	CDataTypeTreeNode*   AnalysePointerDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode);
	CDataTypeTreeNode*   AnalyseRefrenceDataType(CDataTypeTreeNode* pBaseDataTypeTreeNode);
	CVectorTreeNode*    AnalyseArrayInitialization(CDeclarationTreeNode* pArrayNode,CParserContext* pContext);
	// Grammar functions   

	CSyntaxTreeNode*     AnalyseProgram();
	CSyntaxTreeNode*     AnalyseDeclaration(CDataTypeTreeNode* pDataTypeTreeNode,UINT nPrefixAttr);
	CSyntaxTreeNode*     AnalyseVariablesDeclaration(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPrefixAttr,CParserContext* pContext = NULL);
	CSyntaxTreeNode*     AnalyseFunctionsDefine(CDataTypeTreeNode* pBaseDataTypeTreeNode,UINT nPrefixAttr = 0);
	CSyntaxTreeNode*     AnalyseParameters(CDeclarationTreeNode* pFunctionNode);
	CSyntaxTreeNode*     AnalyseCompoundStatements(CSyntaxTreeNode* pParent,CParserContext* pContext,BOOL nSingleStatement = FALSE);
	//CSyntaxTreeNode*     ReadStatement(CDeclarationTreeNode* pContext);
	//CSyntaxTreeNode*     WriteStatement();
	//CSyntaxTreeNode*     PrintfStatement();

	CSyntaxTreeNode*     AnalyseExpressionStatement(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseIfStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseWhileStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseSwitchStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseCaseStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseDoStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseForStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseGotoStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseBreakStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseContinueStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseReturnStatement(CSyntaxTreeNode* pParent,CParserContext* pContext);

	CSyntaxTreeNode*    AnalyseFifteenthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseFourteenthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseThirteenthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseTwelfthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseEleventhLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseTenthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseNinthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseEighthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseSeventhLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseSixthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseFifthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseFourthLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseThirdLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseSecondLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseFirstLevelOp(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseFactor(CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseCallStatement(CSymbolInfo* pIDToken,CParserContext* pContext);
	CSyntaxTreeNode*     AnalyseArguments(CExpressionTreeNode* pCallExpressionNode,CParserContext* pContext);
	CSyntaxTreeNode*    AnalyseSizeof(CParserContext* pContext);

	BOOL          CheckDuplicateCase(CSyntaxTreeNode* pSTMTNodes,CSyntaxTreeNode* pCaseNode);
	BOOL          LookParentSTMT(CSyntaxTreeNode* pParent,TREE_NODE_TYPE nNodeType1,TREE_NODE_TYPE nNodeType2 = TNK_UNKNOWN,TREE_NODE_TYPE nNodeType3 = TNK_UNKNOWN);

	void TypeCompatibleCheckingofPointer(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode);
	void TypeCheckingforArrayInit(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode);
	void TypeCompatibleChecking(CSyntaxTreeNode* pLeftNode,CSyntaxTreeNode* pRightNode,BOOL bInitCheck = FALSE);
	void TypeCheckingSecondLevelOp(AXC_SYMBOL_TYPE NODE_TYPE,CSyntaxTreeNode* pOperand);
	void TypeCheckingThirdLevelOp(AXC_SYMBOL_TYPE NODE_TYPE,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
	void TypeCheckingFourthLevelOp(AXC_SYMBOL_TYPE NODE_TYPE,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
	void TypeCheckingCompareOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
	void TypeCheckingBitOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
	void TypeCheckingLogicalOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
	void TypeCheckingFifthLevelOp(LPCSTR lpOperator,CSyntaxTreeNode* pOperand1,CSyntaxTreeNode* pOperand2);
protected:
	void          WriteSyntaxinfo( char* format, ... );   
	void          PrintTree( CSyntaxTreeNode* const pSyntaxTreeRoorNode);
private:  
	DECLARE_MEMBER_AND_METHOD(UINT,m_nMode,Mode); //�ο�ö��ACM_MODE_X86,ACM_MODE_X64
	CScaner*                 m_pScaner;
	CSymbolInfo*             m_pCurrentToken;
	char                     m_szScope[SYMBOL_MAX_LENGTH];
	CStringA                 m_strIndentSpace; 
	CSaxCCompileimpl*        m_pSaxCCompileimpl;
	SYNTAX_TREE_NODE_ARRAY   m_arrDataTypes;             //��������ʹ�õ��������ͱ� 
	CSTRING_ARRARY           m_arrFilesList;             //Ӧ�ó����漰�������ļ�
	SYNTAX_TREE_NODE_ARRAY   m_arrIdentifiers;
	UINT                     m_nMaxID;
	BINDING_LEVEL_ARRAY      m_arrBindingLevels;           //����ջ������һ���������(����:��������һ��"{}"֮��Ĵ����)ʱ���������˳���ɾ��
	CBindingLevel*           m_pCurrentBindingLevel;
	CTxtConstantsTreeNode*   m_pTxtConstantsList;          //���������ֳ����б�������ֳ���,�ַ������ݺ��ַ�����


};

