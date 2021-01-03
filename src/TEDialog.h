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
//----- TEDialog.h : Declaration of the CTEDialog
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "acui.h"
#include "afxwin.h"
#include "afxcmn.h"

typedef enum TETextType { UNKNOWN, DTEXT, MTEXT, DIMENSION } TETextType;
//const TCHAR* texttypenames[];
extern DWORD DoubleClick,DoubleClickbegin, DblClkDim, DblClkDTEXT, 
	DblClkMTEXT, ExitOnEnter, ExitOnCtrlEnter, FastYAlign;
#if _ACADTARGET > 22
extern bool DoubleClickFilter(MSG *pMsg);
#else
extern BOOL DoubleClickFilter(MSG *pMsg);
#endif
//-----------------------------------------------------------------------------
class CMsg : public CString {
private:
public:
	CMsg(UINT uID) 
	{
		CString();
		if (uID)
			if (!LoadString(_hdllInstance, uID, GetUserDefaultLangID()))
				LoadString(_hdllInstance, uID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}
};
//-----------------------------------------------------------------------------
class CFMsg : public CString {
private:
public:
	CFMsg(UINT uID,...)
	{
		CString();
		if (uID)
			if (!LoadString(_hdllInstance, uID, GetUserDefaultLangID()))
				LoadString(_hdllInstance, uID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
		// Format message into temporary buffer lpszTemp
		va_list argList;
		va_start(argList, uID);
		LPTSTR lpszTemp;
		if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
			GetBuffer(), 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
			lpszTemp == NULL)
		{
			AfxThrowMemoryException();
		}
// Copy lpszTemp into the result string
		CString::operator=(lpszTemp); // Compatible with VC6 and VC7.
// Clean-up
		LocalFree(lpszTemp);
		va_end(argList);
	}
};
//-----------------------------------------------------------------------------
class CTEDialog : public CAcUiDialog {
	DECLARE_DYNAMIC (CTEDialog)
protected:
	virtual void DoDataExchange (CDataExchange *pDX) ;
    //{{AFX_MSG(CTEDialog)
	afx_msg LRESULT OnAcadKeepFocus (WPARAM, LPARAM) ;
	//afx_msg LRESULT OnIdle (WPARAM, LPARAM) ;
	afx_msg void OnKickIdle();
	afx_msg LRESULT OnTESelChange (WPARAM, LPARAM) ;
	//afx_msg LRESULT OnSetCursor (WPARAM, LPARAM) ;
	afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT);
	afx_msg LRESULT OnContextMenu(WPARAM, LPARAM);
	afx_msg void OnClose();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeStyle();
	afx_msg void OnEnChangeHeight();
	afx_msg void OnEnChangeLineSpace();
	afx_msg void OnEnChangeWidth();
	afx_msg void OnSelChange(NMHDR *pSel, LRESULT* result );
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnAlignX();
	afx_msg void OnAlignY();
	afx_msg void OnUCase();
	afx_msg void OnLCase();
	afx_msg void OnFind();
	afx_msg void OnReplace();
	afx_msg void OnAbout();
	afx_msg void OnEditMenuItems(UINT nID);
	afx_msg void OnFont();
	afx_msg void OnDoubleclick();
	afx_msg void OnDblDTEXT();
	afx_msg void OnDblMTEXT();
	afx_msg void OnDblDIM();
	afx_msg void OnOptionExitOnEnter();
	afx_msg void OnOptionExitOnCtrlEnter();
	afx_msg void OnOptionFastYAlign();

	afx_msg LRESULT OnFindReplaceMessage(WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	//static BOOL inUse;
//	static HMODULE hRicheditLib;
	static CTEDialog *dlg;
	static UINT FindReplaceDialogMessage;
	static CFindReplaceDialog *pFindDlg;
	static void UpdateRegistry();
	AcDbObjectId EntId; //for DIM or MTEXT
	AcDbObjectIdArray DTextArr;
	double DtextAngle, LineSpace, TextHeight, WidthFactor;
	double cos_a, sin_a, dx, dy, x_1, y_1;
	long edSelStart, edSelEnd, edStartLn, edEndLn;
	long edCol;

	bool useevent, bNeedUpdStbar, textchanged;
	//bool isSearchNext;
	//int LastFindOper;
	HACCEL accel;
	HWND ControlOnIdle;
	HWND hwndDialog, hwndEdit, hwndStyle, hwndHeight, hwndWidth, hwndLineSpace, hwndAlignX, hwndAlignY;
	ads_name sset;
	enum { IDD = EDITDLG} ;
	enum TETextType texttype;
	void ShowDlg();
	virtual BOOL PreTranslateMessage(MSG*);
	void SwitchItem(DWORD &ItemVar, LPCTSTR ItemName);
	void SetFont(LPTSTR FontName, int FontSize);
	void GetLine(int i, CString& s);
	static void Cleanup();
	bool ParseSset();
	bool TryFoundMore(FINDREPLACE &fr);
	void SortDText();
	void FillStyles();
	void GetStyleId(CString& s, AcDbObjectId& StyleId);
	const TCHAR* GetTextStyle(int index);
	void SetTextStyle(int index, CString& s);
	int GetDlgItemFloat(int Item, double &AValue);
	void SetDlgItemFloat(int Item, double AValue);
	void setDTEXTHeight(int index, double height);
	AcGePoint3d GetInsPoint(AcDbObjectId EntId);
	void SetInsPoint(AcDbObjectId EntId,AcGePoint3d inspoint);
	double GetDTEXTLineSpace();
	double GetMTEXTLineSpace();
	Acad::ErrorStatus setMTextLSpace(double& lspace);
	void UpdateMenuItems();
	void GetEntText(AcDbEntity *pEnt, TETextType tt, CString &str);
	void GetObjIdText(AcDbObjectId eId, TETextType tt, CString &str);
	void settext();
	void gettext();
	void rtfSetText(const CString &s, DWORD flags);
	void rtfGetText(CString &s, DWORD flags);
	void GetSelParams();
	void ShowCarPos();
	void ShowTextParams(long edStartLn, long edEndLn);
	CTEDialog (CWnd *pParent =NULL, HINSTANCE hInstance =NULL);
	void UpdateStatus();
	BOOL OnInitDialog();
	virtual void OnInitDialogStart();
	virtual void OnInitDialogFinish();
	CRichEditCtrl edEdit;
	CButton btCancel;
	CComboBox cbStyle;
	//CEdit edHeight;
	//CEdit edLineSpace;
	//CEdit edWidth;
	CButton btAlignX;
	CButton btAlignY;
	CStatusBarCtrl stBar;
} ;
