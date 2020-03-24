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

//1.没有分析数组的初试化,列int array1[] = {0,1,2,3,4};



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
	SYNTAX_TREE_NODE_ARRAY m_arrEntries;              //整个程序使用所有项目列表
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

// ·TNK_CONST_INTEGER结点此类结点代表一个双字整常数若此常数为有符号常数    则将值扩展成双字。 
//   ·TNK_CONST_REAL结点此类结点代表一个以‘double’或‘字符串’形式表示的实数值若此值以字符串形式表示则不允许对它做任何优化但允许交叉编译。 
//   ·TNK_CONST_STRING结点此类结点代表一个字符串常量。
//   ·TNK_CONST_COMPLEX结点此类结点代表一个复型常量。 
//   ·TNK_IDENTIFIER结点此类结点代表一个标识符。 
//   ·TNK_LIST结点此类结点代表一串相关结点。 
//   ·TNK_VECOTR结点此类结点代表一组结点向量。 
//   ·TNK_EXPRESSION结点此类结点代表一个表达式。 
//   ·TREE_BLOCK结点此类结点表示一个程序块的相关信息。 
//   ·TREE_DATA_TYPE结点此类结点代表数据类型。 
//   ·TREE_DECL结点此类结点代表一个被说明的符号名。 
//   ·TREE_VECTOR结点此类结点对应于机器模式中的vector模式主要用于并行执行表达式

enum TREE_NODE_TYPE
{ 
	TNK_UNKNOWN = 0, 

	//特殊结点代码 

	TNK_ERROR_MARK, /*标识任何出错的结构。在后续分析过程中，此类结点在任何上下文中被无错误接收，以避免对同一错误产生多个错误信息。除了TREE_CODE域外，这类结点不使用其它域。这是一类特殊结点。*/
	TNK_IDENTIFIER_NODE, /*表示一个名字。从内部看，它类似于STRING_CST结点。对任一特定名字，只产生一个对应的IDENTIFIER_NODE。用get_identifier访问它，当第一次用get_identifier 时，创建一个IDENTIFIER_NODE结点。这是一类特殊结点。*/
	TNK_TREE_LIST_NODE, /*这是一类特殊结点。有TREE_VALUE和TREE_PURPOSE两个域。这些结点通过TREE_CHAIN域相联。链中的元素存在于TREE_VALUE域，TREE_PURPOSE域只在某些情况下使用。*/
	TNK_TREE_VEC, /* 这是一类特殊结点，包含一个结点数组。*/
	TNK_BLOCK,   /*代表一个词法块，表示一个符号名层次块*/

	//类型结点代码
	TNK_DATA_TYPE_ID_MIN = 50, //不是具体的数据类型只用于判断
	TNK_BYTE_TYPE,
	TNK_WORD_TYPE,
	TNK_DWORD_TYPE,
	TNK_LONG_TYPE,
	TNK_SHORT_TYPE,
	TNK_FLOAT_TYPE,
	TNK_LONGLONG_TYPE,
	TNK_INT_TYPE, /*属于TYPE结点类，表示所有语言中的整型，包括C中的字符型，也用于其它离散类型的子范围类型。包含TYPE_MIN_VALUE、TYPE_MAX_VALUE和TYPE_PRECISION三个域。 对PASCAL中的子类型，TREE_DATA_TYPE将指向其超类型（另一个INTEGER_TYPE、ENUMERAL_TYPE或BOOLEAN_TYPE）。否则TREE_DATA_TYPE为空。*/
	TNK_DOUBLE_TYPE,  /*属于TYPE结点类，表示C语言中的浮点和双精度类型，不同的浮点类型由机器模式、TYPE_SIZE和TYPE_PRECISION区分。*/
	TNK_BOOLEAN_TYPE, /*属TYPE结点类。表示PASCAL中的布尔类型，只有true和false两个值，不需使用特殊域*/
	TNK_CHAR_TYPE,   /*属TYPE结点类，表示PASCAL中的CHAR类型，不在C中使用。不需使用特殊域。 */
	TNK_VOID_TYPE,  /*属于TYPE结点类，表示C语言中的void 类型。*/ 
	TNK_COMPLEX_TYPE, /*属于TYPE结点类，表示复数类型，TREE_DATA_TYPE域指向实部和虚部的类型。*/ 
	TNK_VECTOR_TYPE, /*表示向量类型，类型的TREE_DATA_TYPE 域指向向量元素的数据类型。*/
	TNK_ENUMERAL_TYPE, /*属于TYPE结点类，表示C中的枚举类型，类似于INTEGER_TYPE结点。表示枚举值的符号用 CONST_DECL结点定义，但TREE_DATA_TYPE并不指向它们，这类结点的TREE_VALUE域指向一由TREE_LIST结点组成的链表，其中每个元素的TREE_PURPOSE域表示枚举名而TREE_VALUE域表示其值（为INTEGER_CST结点）。 如果还没有定义枚举名，但已有对它的前向引用，则其TYPE_SIZE为空。标签名存在TYPE_NAME域。如果在以后定义了此类型，则这些规定的域将被填充。对RECORD_TYPE、UNION_TYPE和QUAL_UNION_TYPE的前向引用也作相似处理。*/
	TNK_POINTER_TYPE, /*属于TYPE结点类，所有指针类型都有此代码。TREE_DATA_TYPE指向指针所指类型。 */
	TNK_OFFSET_TYPE,  /*属于TYPE结点类。表示相对于某一对象的一个偏移指针。TREE_DATA_TYPE域指向在偏移处的对象的类型，TYPE_OFFSET_BASETYPE指向相对结点的类型。*/
	TNK_REFERENCE_TYPE, /*属于TYPE结点类，类似于指针类型，与指针类型不同的是，它自动与所指向值一致，用于C++。*/
	TNK_METHOD_TYPE,  /*属于TYPE结点类，表示一种函数类型。此函数为其自己占据一个额外的参数作为第一个参数，且此参数不出现在程序所定义的参数链中。 TREE_DATA_TYPE指向返回类型，TYPE_METHOD_BASETYPE指向额外参数的类型。 TYPE_ARG_TYPES表示真正的参数链，此链中包含自身隐含的第一个参数。*/
	TNK_FILE_TYPE,   /*属于TYPE结点类，在PASCAL中使用，具体内客现在还没定。*/
	TNK_ARRAY_TYPE,   /*属于TYPE结点类，表示C或PASCAL中的数组类型。此类型有几个特殊的域。 TREE_DATA_TYPE指向数组元素的类型。 TYPE_DOMAIN指向用于检索的类型，其值的范围说明了数组的长度。 TYPE_POINTER_TO（TREE_DATA_TYPE (array_type)）通常不为空。 TYPE_STRING_FLAG指明一个字符串（而不是一个字符数组）。 TYPE_SEP指向从一个元素到下一个元素的单位数表达式。 TYPE_SEP_UNIT指明前一单位中的位数。*/
	TNK_SET_TYPE,    /*属于TYPE结点类，表示PASCAL中的集合类型。*/
	TNK_RECORD_TYPE,  /*属于TYPE结点类，表示C中的结构、PASCAL中的记录，特殊的域有： TYPE_FIELDS链, 此链以FIELD_DECLS结点形式连接结构中的各个域。 其前向引用处理同枚举类型。*/
	TNK_UNION_TYPE,   /*属于TYPE结点类，表示C中的联合，类似于结构类型但域间的偏移为0，其前向引用处理同枚举类型。*/ 
	TNK_QUAL_UNION_TYPE,/*属于TYPE结点类，类似于UNION_TYPE，但每一个FIELD_DECL中的DECL_QUALIFIER域中的表达式决定了联合中的内容，第一个DECL_QUALIFIER表达式为真的域将是整个联合的值。*/ 
	TNK_FUNCTION_TYPE, /*属于TYPE结点类，表示函数类型，特殊域有 TREE_DATA_TYPE指向返回值类型TYPE_ARG_TYPES指向参数类型表，由TREE_LIST结点组成。 若语言中过程与函数有差别， 表示过程的类型也具有FUNCTION_TYPE代码，但TREE_DATA_TYPE为空。*/
	TNK_LANG_TYPE,   /*属于TYPE结点类，是与语言相关的一种类型，其具体意义依赖于语言前端，layout_type过程不知道怎样处理这种类型，所以前端必须手工处理。*/
	TNK_DATA_TYPE_ID_MAX = 100, //不是具体的数据类型只用于判断

	//表达式结点代码,表示常数的树结点编码
	TNK_INTEGER_CST,  /*表示整常数，其内容在TREE_INT_CST_LOW和TREE_INT_CST_HIGH域中，每个域占32位，所以能表示64位整常数。PASCAL语言中的char类型的常数也表示为INTEGER_CST，PASCAL和C中的指针常量如 NIL 和NULL也表示为INTEGER_CST。C中的‘(int *)’也产生一个INTEGER_CST。 */
	TNK_REAL_CST,   /*表示实常数，其内容在TREE_REAL_CST域中。*/
	TNK_COMPLEX_CST,  /*表示复型常数，其内容在TREE_REALPART和TREE_IMAGPART域中。 上述两个域指向其它常数结点。 */
	TNK_VECTOR_CST,  /*表示向量常数，其内容在TREE_VECTOR_CST_ELTS域中。 */
	TNK_STRING_CST,   /*表示字符串常数，其内容在TREE_STRING_LENGTH和TREE_STRING_POINTER域中*/

	//表达式结点代码,表示名字的树结点编码(所有对名字的引用皆表示为…_DECL结点，同一层次的名字通过TREE_CHAIN域串起来。每个DECL有一个DECL_NAME域指向一个IDENTIFIER结点（对有些名字，如标号，DECL_NAME为空）)
	TNK_FUNCTION_DECL, /*表示一个函数名，有四个特殊域。 DECL_ARGUMENTS指向一串表示参数的PARM_DECL。 DECL_RESULT指向表示函数返回值的RESULT_DECL。 DECL_RESULT_TYPE指向函数返回的类型，这通常和DECL_RESULT的类型一致。但⑴它可能是一种比DECL_RESULT类型宽的整型。⑵为方便内联函数，它可能在此函数编译完成时仍有效。 DECL_FUNCTION_CODE是一个代码数，非零表示内部函数。其值是内部函数的一个枚举值，此值代表相应的内部函数。*/
	TNK_LABEL_DECL,  /*表示一个标号名。*/ 
	TNK_CONST_DECL,  /*表示一个常数名。*/ 
	TNK_TYPE_DECL,   /*表示一个类型名。*/ 
	TNK_VAR_DECL,   /*表示一个变量名。*/
	TNK_PARM_DECL,   /*表示一个哑参名。有一个特殊域：DECL_ARG_TYPE指向实际传递的参数类型，此类型有可能与程序中定义的类型不一致。 */
	TNK_RESUCL_DECL,  /*表示返回值名。 */
	TNK_FIELD_DECL,  /*表示域名。NAMESPACE_DECL   表示名字空间说明，名字空间出现在其它_DECL的DECL_CONTEXT域中，提供一个名字的层次结构。*/


	//表达式结点代码,表示内存引用的结点编码
	TNK_COMPONENT_REF, /*表示对结构或联合中的一个分量的引用。参数0表示结构或联合，参数1表示一个域（一个FIELD_DECL结点）。 */
	TNK_BIT_FIELD_REF, /*表示访问一个对象内的一组位域。类似于COMPONENT_REF，与OMPONENT_REF不同的是位域的位置是明确给出的，而不是通过FIELD_DECL给出。参数0是结构或联合。 参数1是一棵树，给出被访问的位数。参数2是一棵树，给出访问的第1位的位置。 被访问的域可以是有符号的，也可以是无符号的，由TREE_UNSIGNED域决定。 */
	TNK_INDIRECT_REF, /*表示C语言中的一元操作‘*’或PASCAL语言中的‘＾’，有一个关于指针的表达式参数。 */
	TNK_VAR_REF,    /*一个变量的参考 */
	TNK_ARRAY_REF,   /*数组访问，参数0表示数组，参数1是一串关于索引下标的TREE_LIST结点。 */
	TNK_ARRAY_RANGE_REF, /*类似于ARRAY_REF，不同之处在于其表示一个数组片段(slice) 。参数0表示数组，参数1表示索引起始点，片段的大小从表达式的类型中获取。 */
	TNK_VTABLE_REF,  /*Vtable索引，此数据主要用于vtable的垃圾收集。 操作数0表示一个数组引用或等价的表达式。操作数1表示vtable的基址 (必须是一个var_decl)。 操作数2表示在vtable中的索引(必须是一个integer_cst)。 */
	TNK_CONSTRUCTOR,  /*返回一组由指定分量组成的集合值。 在C中，这只用于结构和数组初始化。 参数1是一个指向RTL的指针，且仅对常数CONSTRUCTOR有效。 参数2是一串由TREE_LIST结点组成的分量值。*/


	//表达式结点代码,表示表达式的结点编码
	TNK_COMPOUND_EXPR, /*包含待计算的两个表达式，第一个表达式的值被忽略，第二个表达式的值被利用。 */
	TNK_MODIFY_EXPR,  /*赋值表达式，操作数0表示左值，操作数1表示右值。*/
	TNK_INIT_EXPR,   /*初始化表达式，操作数0表示被初始化的变量，操作数1表示初始值。 */
	TNK_TARGET_EXPR,  /*操作数0表示一个初始化目标，操作数1表示此目标的初值，如果有的话操作数2表示此结点的cleanup。 */
	TNK_COND_EXPR,   /*条件表达式，操作数0表示条件，操作数1表示 THEN 值，操作数2表示 ELSE值。*/
	TNK_BIND_EXPR,   /*用于去分配时，其值需要被清除的表达式，C++使用。 局部变量说明，包括RTL生成和空间分配。 操作数0是变量表的VAR_DECL链。 操作数1是要计算的表达式（表达式中含变量表中的变量）。操作数1的值即BIND_EXPR的值。*/ 
	/*操作数2是一个与此层次相对应的BLOCK，用于调试。如果此BIND_EXPR被 扩展，则置BLOCK结点中的TREE_USED标志。*/ 
	/*BIND_EXPR不负责将这些变量的信息通知扫描器。如果表达式串来自于一个输入文件，则产生BIND_EXPR的代码负责将这些变量的信息告诉扫描器。 */
	/*一旦BIND_EXPR被扩展，则TREE_USED标志位为1。如果调试需用到BIND_EXPR，但它又未被扩展，则手工设置TREE_USED域。 */
	/*为使BIND_EXPR全程可见，产生它的代码必须将它作为一个subblock而存入函数的BLOCK结点中。*/
	TNK_CALL_EXPR,   /*函数调用。操作数0指向函数，操作数1是一个参数链（由一串TREE_LIST结点组成）。无操作数2，操作数2的域被CALL_EXPR_RTL宏使用。 */
	TNK_METHOD_CALL_EXPR,  /*调用一个方法。操作数0是调用的方法（其类型为METHOD_TYPE） 操作数1是代表方法自身的表达式。操作数2是一串参数。 */
	TNK_WITH_CLEANUP_EXPR, /*指定一个要伴随它对应的清除动作同时计算的值。操作数0表示其值需清除的表达式，操作数1是一个最终代表该值的RTL_EXPR，操作数2是此对象的清除表达式。清除表达式中含RTL_EXPR，表示此表达式怎样作用于要清除的值。清除动作是由第一个封闭CLEANUP_POINT_EXPR（如果存在的话）来执行的，否则，若需要时，调用者有责任手工调用EXPAND_CLEANUPS_TO函数。 */
	TNK_CLEANUP_POINT_EXPR, /*定义一个清除点。操作数0是确保有 cleanups 的被清除的表达式。 */
	TNK_PLACEHOLDER_EXPR,  /*代表一个记录，当估算此表达式时，提供一个 WITH_RECORD_EXPR来替代此记录 。表达式的类型用于寻找替代此表达式的记录。*/
	TNK_WITH_RECORD_EXPR,  /*提供一个表达式，此表达式引用一个用以替代PLACEHOLDER_EXPR的记录。操作数1包含用来替代的记录，其类型与操作数0中的PLACEHOLDER_EXPR的类型一致。 上述两代码用于有“自引用”类型的语言, 如ADA语言。*/

	//表示简单算术运算的结点代码(术运算的操作数必须有相同的机器模式，结果具有同样的机器模式。)
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
	TNK_PLUS_EXPR,   /* 加操作，有两个操作数。 */
	TNK_MINUS_EXPR,  /*减操作，有两个操作数。 */
	TNK_MULT_EXPR,   /*乘操作。 */
	TNK_TRUNC_DIV_EXP, /*整除法，其结果的尾数部分被截除。理论上，操作数可为实型，但目前还不支持。其结果通常为定点数，和操作数具有相同类型。*/
	TNK_CEIL_DIV_EXPR, /*整除法，其结果向上取整。*/ 
	TNK_FLOOR_DIV_EXPR, /*整除法，其结果向下取整。*/
	TNK_ROUND_DIV_EXPR, /*整除法，其结果做四舍五入.*/
	TNK_TRUNC_MOD_EXPR, /*对应于TRUNC_DIV_EXPR的求余操作。 */
	TNK_CEIL_MOD_EXPR, /*对应于CEIL_DIV_EXP的求余操作。 */
	TNK_FLOOR_MOD_EXPR, /* 对应于FLOOR_DIV_EXPR的求余操作*/ 
	TNK_ROUND_MOD_EXP, /*对应于ROUND_DIV_EXP的求余操作。 */
	TNK_RDIV_EXPR,   /*实数除法，所得结果为实数。两个操作数必须具有相同的类型。理论上，操作数可为整数，但目前仅支持实型操作数。结果的类型必须与操作数类型一致。*/
	TNK_EXACT_DIV_EXPR, /*不需四舍五入的除法，用于C中的指针减操作。 上述各操作都有两个操作数。 下面是四种不同截断方式的将实数转换为定点数的操作。CONVERT_EXPR也可用于将一个实数转换成整数。 */
	TNK_FIX_TRUNC_EXPR, 
	TNK_FIX_CEIL_EXPR,
	TNK_FIX_FLOOR_EXPR,
	TNK_FIX_ROUND_EXPR, /*其转换含义同四种整数除法。上述四种转换操作只有1个操作数。 */
	TNK_FLOAT_EXPR,   /*将整数转为实数，只有1个操作数。 */
	TNK_NEGATE_EXPR,  /*一元求反操作。结果与操作数具有同一类型。 */
	TNK_MIN_EXPR,    /*求最小值操作，有两个操作数。 */
	TNK_MAX_EXPR,    /*求最大值操作，有两个操作数。 */
	TNK_ABS_EXPR,    /*求绝对值操作，有一个操作数。 */
	TNK_FFS_EXPR,    /*FFS操作，有一个操作数。 */
	TNK_LSHIFT_EXPR,
	TNK_RSHIFT_EXPR,  /*分别表示左移和右移操作，有两个操作数。如果操作数为无符号类型，则为逻辑移位；否则为算术移位。第二个操作数是需移动的位数，必须是SImode，结果的模式与第一个操作数一致。 ATE_EXPR RROTATE_EXPR 分别表示循环左移和循环右移，具体说明同上。 */
	TNK_BIT_IOR_EXPR,  /*位同或操作，即A|B（A、B代表两个操作）*/
	TNK_BIT_XOR_EXPR,  /*位异或操作，即A＾B。 BIT_AND_EXPR 位与操作，即A&B。 */
	TNK_BIT_AND_EXPR,  /*位与操作，即A&B。 BIT_NOT_EXPR 位非操作。*/
	TNK_BIT_NOT_EXPR,  /*~是一元操作符，具有右结合性，其作用就是将操作数的每一位翻转，例如二进制下~1100B=0011B，换成十进制就是~12D=3D*/
	TNK_TRUTH_ANDIF_EXPR,
	TNK_TRUTH_ORIT_EXPR, /*表示逻辑与和逻辑或操作。如果能从第一个操作数的值获得些表达式的值，则不计算第二个操作数的值。 */
	TNK_TRUTH_AND_EXPR,
	TNK_TRUTH_OR_EXPR,
	TNK_TRUTH_XOR_EXPR,
	TNK_TRUTH_NOT_EXPR, /*分别表示逻辑与、或、异或和非操作，不管是否需要第二个操作数的值，总是计算第二个操作数。 这些表达式除TRUTH_NOT_EXPR只有一个操作数外，其余都有两个操作数，操作数为布尔值或只取零或非零的整数值。*/
	TNK_LT_EXPR,
	TNK_LE_EXPR,
	TNK_GT_EXPR,
	TNK_GE_EXPR,
	TNK_EQ_EXPR,
	TNK_NGT_EXPR,
	TNK_NLT_EXPR,
	TNK_NEQ_EXPR,     /*分别表示两个操作数之间的小于、小于等于、大于、大于等于、等于和不等于操作。 其中EQ_EXPR和NE_EXPR允许对任何类型进行操作，其它只允许对整型、指针、枚举或实型进行操作。所有情况下，操作数都必须具有相同类型。结果是布尔型。*/
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
	TNK_RANGE_EXPR,   /*对PASCAL中的集合进行的操作，目前还没使用。*/
	TNK_CONVERT_EXPR,  /*对一个值的类型进行转换。所有的转换，包括隐式的转换，都必须表示成CONVERT_EXPR，只有一个操作数。 */
	TNK_NOP_EXPR,    /*表示一个不产生任何代码的转换。只有一个操作数。*/
	TNK_NON_LVALUE_EXP, /*表示一个与其参数相同的值，但保证此值不为左值。只有一个操作数。*/
	TNK_VIEW_CONVERT_EXPR, /*表示将具有某种类型的对象看成具有另一种类型。此类结点所表示的含义对应于ADA语言中的"Unchecked Conversion"，大致对应于C语言中的*(type2 *)&X操作。其唯一的操作数是将被看成具有另一种类型的那个值。如果输入数和表达式的类型大小不同，则此操作无定义（It is undefined if the type of the input and of the expression have different sizes）。此结点代码也可以作为MODIFY_EXPR的LHS，此时并没有真正的数据赋值动作发生。在这种情况下应该设置TREE_ADDRESSABLE。*/
	TNK_SAVE_EXPR,   /*表示计算一次但多次用到的表达式。第一个操作数即是此表达式；第二个操作数指向一个函数名，其中SAVE_EXPR在此函数中产生；第三个操作数为表达式对应的RTL，只有当此表达式被计算以后，RTL域才不为空.*/
	TNK_UNSAVE_EXPR,  /*UNSAVE_EXPR表达式的操作数0代表unsave的那个值。我们可以将所有的_EXPR结点如TARGET_EXPRs、 SAVE_EXPRs、 CALL_EXPRs 和RTL_EXPRs等这些仅估值一次（不允许多次估值）的表达式重新设置为UNSAVE_EXPR，使得对这些表达式的一次新的expand_expr调用能导致对这些表达式的重新估值。当我们想在不同的地方重用一棵树但有必须重新扩展时，此表达式非常有用。*/
	TNK_RTL_EXPR,    /*表示一个其RTL已被扩展成一个序列，且当表达式被扩展时，应当流出该序列的一个表达式。 第一个操作数指向将被流出的RTL，它是insn序列中的第一项。 第二个操作指向表示结果的RTL表达式*/
	TNK_ADDR_EXPR,   /*C中的 & 操作，其值是操作数值所在单位的地址，操作数可具有任一模式，结果模式为Pmode。 */
	TNK_REFERENCE_EXPR, /*表示一个非左值引用或指向一个对象的指针。 */
	TNK_ENTRY_VALUE_EXPR, /*仅在那些需要静态链的语言中使用。操作数是一个函数常量；结果是一具有Epmode的函数变量值。 */
	TNK_FDESC_EXPR,   /*操作数0函数常量，结果是part N of a function descriptor of type ptr_mode */
	TNK_COMPLEX_EXPR,  /*给定两个相同类型的实型或整型操作数，返回一个具有相应复数类型的复数值。 */
	TNK_CONJ_EXPR,   /*取操作数的复共轭，仅对复类型操作，值与操作数具有相同的类型。 */
	TNK_REALPART_EXPR,
	TNK_IMAGPART_EXPR, /*分别表示复数的实部和虚部，只对复型数据进行操作。*/ 
	TNK_PREDECREMENT_EXPR,
	TNK_PREINCREMENT_EXPR,
	TNK_POSTDECREMENT_EXPR,
	TNK_POSTINCREMENT_EXPR, /*分别对应于C中的++和－－操作。有两个操作数，第二个操作数说明每次增减的程度，对指针而言，第二个操作数为所指对象的大小。*/
	TNK_VA_ARG_EXPR,  /*用于实现 `va_arg'.*/
	TNK_RY_CATCH_EXPR, /*对操作数1进行估值，当且仅当在此估值过程中throw一个异常时，才对操作数2进行估值。这与WITH_CLEANUP_EXPR不同的地方在于除非throw一个异常，否则永远不对操作数2进行估值。*/
	TNK_TRY_FINALLY_EXPR, /*对操作数1进行估值，操作数2是一个cleanup表达式，在从此表达式退出（正常、异常或跳出）之前被估值。此操作类似于CLEANUP_POINT_EXPR和WITH_CLEANUP_EXPR的结合，但CLEANUP_POINT_EXPR和WITH_CLEANUP_EXPR常常将cleanup表达式拷贝到所需的地。而  TRY_FINALLY_EXPR 则产生一个跳转，转到一个cleanup过程。当cleanup 是当前函数中的实际语句（如用户想在此设置断点）时，应该使用TRY_FINALLY_EXPR。*/
	TNK_GOTO_SUBROUTINE_EXPR, /*用于实现TRY_FINALLY_EXPR 中的cleanups。它由expand_expr而非前端产生。操作数0是代表我们将要调用的过程的开始处的rtx，操作数1代表存储过程返回地址的变量的rtx。  下面这些表达式都无有用的值，但都有副作用。*/
	TNK_LABEL_EXPR,   /*封装成语句的一个标号定义，唯一的一个操作数为LABEL_DECL结点。此表达式的类型必须为空，其值被忽略*/
	TNK_GOTO_EXPR,   /*表示GOTO语句。唯一的一个操作数为LABEL_DECL结点。表达式的类型必须为空，其值被忽略*/
	TNK_RETURN_EXPR,  /*表示return语句，对唯一的一个操作数进行估值，然后从当前函数返回。表达式的类型必须为空，其值被忽略*/
	TNK_EXIT_EXPR,   /*表示从内层循环中条件退出。唯一的一个操作数为退出的条件。表达式的类型必须为空，其值被忽略*/
	TNK_LOOP_EXPR,   /*表示一个循环。唯一的一个操作数表示循环的体。此表达式必须包含EXIT_EXPR，否则，表示它是一个无穷循环。 表达式的类型必须为空，其值被忽略*/
	TNK_LABELED_BLOCK_EXPR, /*表示一个带标号的块，操作数0是标示块结束的标号，操作数1表示块。*/
	TNK_EXIT_BLOCK_EXPR,/*表示退出一个带标号的块，可能会返回一个值。操作数0是一个欲退出的LABELED_BLOCK_EXPR，操作数1是返回值，返回值可能为空*/
	TNK_EXPR_WITH_FILE_LOCATION, /*给一个树结点（通常是一个表达式）注释上源程序位置信息：文件名字(EXPR_WFL_FILENAME)；行数(EXPR_WFL_LINENO)和列数(EXPR_WFL_COLNO)。其扩展动作同EXPR_WFL_NODE。如果有EXPR_WFL_EMIT_LINE_NOTE，则必须首先emit一个行注释信息。第三个操作数仅在JAVA前端中使用。*/
	TNK_SWITCH_EXPR,  /*Switch表达式，操作数0是用于执行分枝的表达式，操作数1包含CASE值，其组织方式和前端实现相关。*/
	TNK_EXC_PTR_EXPR,  /*The exception object from the runtime*/
	TNK_QUESTION_MARK_EXPR,  /*c语言里面的条件运算符号?*/
	TNK_COMMA_EXPR,   //逗号运算符

	//与C和C++相关的树结点介绍
	TNK_SRCLOC,     /*记住源代码位置的一个结点。*/
	TNK_SIZEOF_EXPR,
	TNK_ARROW_EXPR,
	TNK_ALIGNOF_EXPR,
	TNK_EXPR_STMT,   /*表示一个表达式语句，用EXPR_STMT_EXPR来获取表达式。 */
	TNK_COMPOUND_STMT, /*表示一个由大括符括起来的块，操作数为COMPOUND_BODY。 */
	TNK_PARA_STMT,   /*表示参数说明语句 */
	TNK_DECL_STMT,   /*表示局部说明语句，操作数为DECL_STMT_DECL。 */
	TNK_IF_STMT,    /*表示IF语句，操作数分别为 IF_COND、THEN_CLAUSE和 ELSE_CLAUSE。*/ 
	TNK_FOR_STMT,    /*表示for语句，操作数分别为FOR_INIT_STMT、FOR_COND、FOR_EXPR和FOR_BODY。 */
	TNK_WHILE_STMT,   /*表示while语句，操作数分别为WHILE_COND和WHILE_BODY。 */
	TNK_DO_STMT,    /*表示do语句，操作数分别为DO_BODY和DO_COND。 */
	TNK_RETURN_STMT,  /*表示return语句，操作数为RETURN_EXPR。 */
	TNK_BREAK_STMT,   /*表示break语句。 */
	TNK_CONTINUE_STMT, /*表示continue语句。 */
	TNK_SWITCH_STMT,  /*表示switch语句，操作数分别为SWITCH_COND、SWITCH_BODY和SWITCH_TYPE。*/
	TNK_GOTO_STMT,   /*表示goto语句，操作数为GOTO_DESTINATION。 */
	TNK_LABEL_STMT,   /*表示label语句，操作数为LABEL_DECL，可通过宏LABEL_STMT_LABEL来访问 */
	TNK_ASM_STMT,    /*表示一条inline汇编语句。*/
	TNK_SCOPE_STMT,   /*标示一个scope的开始或结束。如果SCOPE_BEGIN_P holds，则表示scope的开始，如果SCOPE_END_P holds，则表示scope的结束。如果SCOPE_NULLIFIED_P holds ，则表示此scope中没有变量。SCOPE_STMT_BLOCK 表示包含此scope中定义的变量的BLOCK。 */
	TNK_FILE_STMT,   /*标示一个spot where a function changes files。它没有其它语义，FILE_STMT_FILENAME 表示文件的名字。 */
	TNK_DEFAULT_LABEL,   
	TNK_CASE_LABEL,   /*表示CASE中的标号，操作数分别为CASE_LOW和CASE_HIGH，如果CASE_LOW为NULL_TREE，表示标号为default，如果CASE_HIGH为NULL_TREE，表示标号是一个正常的case标号。CASE_LABEL_DECL代表此结点的LABEL_DECL*/
	TNK_STMT_EXPR,   /*表示一个语句表达式，STMT_EXPR_STMT 表示由表达式给出的语句。 */
	TNK_COMPOUND_LITERAL_EXPR, /*表示C99 compound literal，COMPOUND_LITERAL_EXPR_DECL_STMT is the a DECL_STMT containing the decl for the anonymous object represented by the COMPOUND_LITERAL; the DECL_INITIAL of that decl is the CONSTRUCTOR that initializes the compound literal.*/
	TNK_CLEANUP_STMT,  /*marks the point at which a declaration is fully  constructed. If, after this point, the CLEANUP_DECL goes out of  scope, the CLEANUP_EXPR must be run.*/
	TNK_TXT_CONSTANTS_TABLE,//程序中出现的所有字符串常量,因为它们不能被编码到机器指令中,所以必须存储到映像文件的指令区  

	TNK_READ_STMT,
	TNK_WRITE_STMT,
	TNK_PRINTF_STMT
};

enum NODE_COMMON_ATTRIBUTE         //语法树节点基本属性
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
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pParent,Parent);     //父亲节点
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pReferences,References); //引用该节点得节点列表
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pChain,Chain);     //和面的兄弟节点
	DECLARE_MEMBER_AND_METHOD(CDataTypeTreeNode*,m_pDataType,DataType); //数据类节点
	DECLARE_MEMBER_AND_METHOD(int,m_nLineIndex,LineIndex);       //在源代码中所在的行数
	DECLARE_MEMBER_AND_METHOD(TREE_NODE_TYPE,m_nNodeType,NodeType);   //节点类型
	DECLARE_MEMBER_AND_METHOD(AXC_SYMBOL_TYPE,m_nTokenType,TokenType); //对应的符号
	DECLARE_MEMBER_AND_METHOD(UINT,m_nCommonAttribute,CommonAttribute); //参考枚举"NODE_COMMON_ATTRIBUTE"
	DECLARE_MEMBER_AND_METHOD(CParser*,m_pParser,Parser);        //参考枚举"NODE_COMMON_ATTRIBUTE"

	DECLARE_MEMBER_AND_METHOD(int,m_nOffset,Offset);          //用于代码创建,存储常量或静态变量或函数在映像文件中的偏移
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
	DECLARE_MEMBER_AND_METHOD(int,m_nRef,Ref);           //常量专用的引用计数  
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

enum NODE_DECL_ATTRIBUTE         //语法树节点基本属性
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
	DECLARE_MEMBER_AND_METHOD(UINT,m_nDeclAttribute,DeclAttribute);  //参考NODE_DECL_ATTRIBUTE
	DECLARE_MEMBER_AND_METHOD(int,m_nPointerDepth,PointerDepth);

	DECLARE_MEMBER_AND_METHOD(CIdentifierTreeNode*,m_pName,Name);   
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pContext,Context); /*在FIELD_DECLS结点中，此域指向RECORD_TYPE、UNION_TYPE或QUAL_UNION_TYPE结点，说明此FIELD是上述类型中的一个组成成员。
																	 在VAR_DECL、PARM_DECL、FUNCTION_DECL、LABEL_DECL和CONST_DECL结点中，此域指向包含此符号名的FUNCTION_DECL结点，若此符号名具有“文件范围”的作用域，则此域为NULL_TREE。*/
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pArguments,Arguments);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pResult,Result);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pInitial,Initial); //变量的值可以为表达算式
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAbstractOrigin,AbstractOrigin);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAssemblerName,AssemblerName);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSectionName,SectionName);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAttributes,Attributes);
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pSavedTree,SavedTree);
};


enum NODE_TYPE_ATTRIBUTE         //语法树节点基本属性
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

enum ALIGN_MODE     //对齐方式
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
	DECLARE_MEMBER_AND_METHOD(UINT,m_nTypeAttribute,TypeAttribute);  //参考NODE_DECL_ATTRIBUTE
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

	DECLARE_MEMBER_AND_METHOD(CIdentifierTreeNode*,m_pName,Name);       //不序号删除
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pAttributes,Attributes);   //不序号删除
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
	DECLARE_MEMBER_AND_METHOD(CSyntaxTreeNode*,m_pFunctionDeclNode,FunctionDeclNode); //指向所隶属的函数节点
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
	DECLARE_MEMBER_AND_METHOD(CParserContext*,m_pContext,Context); //指向所隶属的函数分析上下文
	SYNTAX_TREE_NODE_ARRAY  m_arrDeclNodes;             //指向所有的变量、常量、函数以及typedef类型的_DECL节点
	SYNTAX_TREE_NODE_ARRAY  m_arrTags;               //指向所有的结构、联合和枚举定义
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
	DECLARE_MEMBER_AND_METHOD(UINT,m_nMode,Mode); //参考枚举ACM_MODE_X86,ACM_MODE_X64
	CScaner*                 m_pScaner;
	CSymbolInfo*             m_pCurrentToken;
	char                     m_szScope[SYMBOL_MAX_LENGTH];
	CStringA                 m_strIndentSpace; 
	CSaxCCompileimpl*        m_pSaxCCompileimpl;
	SYNTAX_TREE_NODE_ARRAY   m_arrDataTypes;             //整个程序使用的数据类型表 
	CSTRING_ARRARY           m_arrFilesList;             //应用程序涉及的所有文件
	SYNTAX_TREE_NODE_ARRAY   m_arrIdentifiers;
	UINT                     m_nMaxID;
	BINDING_LEVEL_ARRAY      m_arrBindingLevels;           //类似栈当进入一个分析层次(比如:函数或者一对"{}"之间的代码块)时将创建，退出后将删除
	CBindingLevel*           m_pCurrentBindingLevel;
	CTxtConstantsTreeNode*   m_pTxtConstantsList;          //程序中文字常量列表包括数字常量,字符串数据和字符常量


};

