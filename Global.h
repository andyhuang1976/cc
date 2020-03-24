/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/Global.h,v 1.6 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once

//enum VARIABLE_TYPE_SIZE      // Define all kinds of variable's size
//{
//   VTS_CHAR     = 1,
//   VTS_BYTE     = 1,
//   VTS_WORD     = 2,
//   VTS_DWORD     = 4,
//   VTS_INT      = 4,
//   VTS_LONG     = 4,
//   VTS_SHORT     = 2,
//   VTS_FLOAT     = 4,
//   VTS_DOUBLE    = 8
//};


#define DECLARE_GET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
public: \
   TYPE Get##PROPERTY_NAME() const\
   { \
     return MEMBER_NAME; \
   } \

   
#define DECLARE_SET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
public: \
   void Set##PROPERTY_NAME(TYPE nVal) \
   { \
     MEMBER_NAME = nVal; \
   } \

#define DECLARE_PROPERTY_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
protected: \
DECLARE_GET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
DECLARE_SET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \


#define DECLARE_MEMBER_AND_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
protected: \
   TYPE MEMBER_NAME; \
DECLARE_GET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \
DECLARE_SET_METHOD(TYPE, MEMBER_NAME, PROPERTY_NAME) \

