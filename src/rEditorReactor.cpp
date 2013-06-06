/////////////////////////////////////////////
// AcEditorReactor reactors.

#include "StdAfx.h"

AcEdCommand * cmd = NULL;
AcRxFunctionPtr oldcmdaddr = NULL;
//MTMeditorreactor* MTMeditorreactor::reac = NULL;

/*void MyAcDbDoubleClickEdit::startEdit(AcDbEntity *pEnt,AcGePoint3d pt) {
	acutPrintf(_T("\nHello from MyAcDbDoubleClickEdit"));
}
void MyAcDbDoubleClickEdit::finishEdit() {
}*/
/*
MTMeditorreactor::MTMeditorreactor(const bool autoInitAndRelease)
{
	m_autoInitAndRelease = autoInitAndRelease;
	if (m_autoInitAndRelease)
		if (NULL != acedEditor)
			acedEditor->addReactor(this);
		else
			m_autoInitAndRelease = false;
}

MTMeditorreactor::~MTMeditorreactor()
{
	if (m_autoInitAndRelease)
		if (NULL != acedEditor)
			acedEditor->removeReactor(this);
	reac = NULL;
}
*/
/*-----------------------------------*/
/*
void MTMeditorreactor::beginDoubleClick(const AcGePoint3d& clickPoint)
{
	DoubleClickbegin=0;
	ads_name sset;
	struct resbuf* filterlist = acutBuildList(RTDXF0, _T("ATTDEF"), 0);
	int result=acedSSGet(_T("_I"), NULL, NULL, filterlist, sset);
	if (RTNORM == result)
	{
		acedSSFree(sset);
		return;
	}
	if (DblClkDim)
	{
		filterlist = acutBuildList(RTDXF0, _T("DIMENSION"), 0);
		int result=acedSSGet(_T("_I"), NULL, NULL, filterlist, sset);
		if (RTNORM == result) // There are DIM objects in selection
		{
			//long slen;
			//result = acedSSLength(sset, &slen);
			//if (slen == 1)
			//	acDocManager->sendStringToExecute(curDoc(),_T("_TE\n"),false,false,false);
			DoubleClickbegin=1;
			acedSSFree(sset);
		}
	}

	if (DblClkDTEXT)
	{
		filterlist = acutBuildList(RTDXF0, _T("TEXT"), 0);
		int result=acedSSGet(_T("_I"), NULL, NULL, filterlist, sset);
		if (RTNORM == result) // There are DTEXT objects in selection
		{
			DoubleClickbegin=1;
			acedSSFree(sset);
		}
	}
	if (DblClkMTEXT)
	{
		filterlist = acutBuildList(RTDXF0, _T("MTEXT"), 0);
		int result=acedSSGet(_T("_I"), NULL, NULL, filterlist, sset);
		if (RTNORM == result) // There are MTEXT objects in selection
		{
			DoubleClickbegin=1;
			acedSSFree(sset);
		}
	}
//	DoubleClickbegin=1;
}
//-----------------------------------------------------------------------------
void MTMeditorreactor::commandWillStart(const ACHAR* cmdStr)
{
	ads_name sset;
	if (0==DoubleClickbegin)
		return;
	DoubleClickbegin = 0;
	if ((_tcscmp(cmdStr, _T("MTEDIT")) == 0)||(_tcscmp(cmdStr, _T("DDEDIT")) == 0)||
		(_tcscmp(cmdStr, _T("PROPERTIES")) == 0))
	{
		int result=acedSSGet(NULL, NULL, NULL, NULL, sset);
		if  (RTNORM == result)
			acedSSFree(sset);
		acDocManager->sendStringToExecute(curDoc(),_T("\x1B\x1B_TE\n_si\n_p\n"),false,false,false);
	} 
}

*/