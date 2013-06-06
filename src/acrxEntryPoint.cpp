// (C) Copyright 2002-2005 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.h
//-----------------------------------------------------------------------------
#include "StdAfx.h"
//-----------------------------------------------------------------------------
#define szRDS _RXST("MTMD")

//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CMtmdEditApp : public AcRxArxApp {
public:
	CMtmdEditApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt);
		//CTEDialog::hRicheditLib = LoadLibrary(_T("riched20.dll"));
		CTEDialog::UpdateRegistry();
		acedRegisterFilterWinMsg(&DoubleClickFilter);
		//if (DoubleClick!=0) MTMeditorreactor::reac = new MTMeditorreactor(true);
		return (retCode);
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt);
		acedRemoveFilterWinMsg(&DoubleClickFilter);
		//if (DoubleClick && MTMeditorreactor::reac) 
		//	delete MTMeditorreactor::reac;
		//FreeLibrary(CTEDialog::hRicheditLib);
		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}

	// - MtmdEdit._TE command (do not rename)
	static void MtmdEdit_TE(void)
	{
		struct resbuf* filterlist = acutBuildList(-4, _T("<or"),RTDXF0, _T("TEXT"),
			RTDXF0, _T("MTEXT"),RTDXF0, _T("DIMENSION"), -4, _T("or>"), 0);
		CTEDialog* & dlg = CTEDialog::dlg;
		if (dlg == NULL) dlg = new CTEDialog();
		int result=acedSSGet(NULL, NULL, NULL, filterlist, dlg->sset);
		if (RTNORM == result)
		{
			acDocManager->disableDocumentActivation();
			if (dlg->ParseSset()) dlg->ShowDlg();
			acDocManager->enableDocumentActivation();
			acedGrText(-3, NULL, 0); 
			acedRedraw(NULL, 0); 
			acedSSFree(dlg->sset); 
		}
		CTEDialog::Cleanup();
		acutRelRb(filterlist);
	}
} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CMtmdEditApp)

ACED_ARXCOMMAND_ENTRY_AUTO(CMtmdEditApp, MtmdEdit, _TE, TE, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET | ACRX_CMD_INTERRUPTIBLE , NULL)
