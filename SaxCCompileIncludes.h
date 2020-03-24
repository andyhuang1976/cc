/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SaxCCompileIncludes.h,v 1.2 2014/04/11 14:33:31 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/
#pragma once
#include "SaxCCompile.h"


#ifdef _DEBUG
#pragma comment(lib, "SaxCCompileD.lib") 
#pragma message("Automatically linking with SaxCCompileD.lib")
#else
#pragma comment(lib, "SaxCCompile.lib") 
#pragma message("Automatically linking with SaxCCompile.lib")
#endif