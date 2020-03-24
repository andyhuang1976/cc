/******************************************************************************
* 
* SaxCCompile
* Version
* Copyright (C) 2007 SaxSoft Co., Ltd.
* All Rights Reserved
* $Header: /cvsdata/vc/SaxCCompile/SaxCCompile.cpp,v 1.4 2014/06/29 11:54:35 administrator Exp $
* 
*******************************************************************************
* 
* Description:
******************************************************************************/

static const char _CppVersion[] = "@(#) $Header: /cvsdata/vc/SaxCCompile/SaxCCompile.cpp,v 1.4 2014/06/29 11:54:35 administrator Exp $";
#include "StdAfx.h"
// $Nokeywords: $
#include ".\saxccompileimpl.h"
#include ".\saxccompile.h"

/*-----------------------------------------------------------------
| 函数名称 : CSaxCCompile::CreateSaxCCompile
| 描  述 : 
| 返 回 值 : 
| 修改记录 : 2007-4-22 16:37:23  -huangdy-  创建
-----------------------------------------------------------------*/
CSaxCCompile* CSaxCCompile::CreateSaxCCompile()
{
   CSaxCCompile* pResult = new CSaxCCompileimpl;
  return pResult;
}

/*-----------------------------------------------------------------
| 函数名称 : CSaxCCompile::DestroySaxCCompile
| 描  述 : 
| 参  数 : CSaxCCompile* pSaxCCompile——
| 返 回 值 : 
| 修改记录 : 2007-4-22 16:37:26  -huangdy-  创建
-----------------------------------------------------------------*/
void CSaxCCompile::DestroySaxCCompile(CSaxCCompile* pSaxCCompile)
{
   delete pSaxCCompile;
}