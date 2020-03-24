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
| �������� : CSaxCCompile::CreateSaxCCompile
| ��  �� : 
| �� �� ֵ : 
| �޸ļ�¼ : 2007-4-22 16:37:23  -huangdy-  ����
-----------------------------------------------------------------*/
CSaxCCompile* CSaxCCompile::CreateSaxCCompile()
{
   CSaxCCompile* pResult = new CSaxCCompileimpl;
  return pResult;
}

/*-----------------------------------------------------------------
| �������� : CSaxCCompile::DestroySaxCCompile
| ��  �� : 
| ��  �� : CSaxCCompile* pSaxCCompile����
| �� �� ֵ : 
| �޸ļ�¼ : 2007-4-22 16:37:26  -huangdy-  ����
-----------------------------------------------------------------*/
void CSaxCCompile::DestroySaxCCompile(CSaxCCompile* pSaxCCompile)
{
   delete pSaxCCompile;
}