//-----------------------------------------------------------------------------
//----- TEDialog.cpp : Implementation of CTEDialog
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "TEDialog.h"
//-----------------------------------------------------------------------------
LPCTSTR texttypenames[]={_T("UNKNOWN"),_T("DTEXT"),_T("MTEXT"),_T("DIM")};
LPCTSTR pAppInfoLocFull = _T("Software\\Mtmlab\\MtmdEdit");
LPCTSTR AppName = _T("MTMDEDIT");
LPCTSTR CommandName = _T("TE");
LPCTSTR sFontSize = _T("FontSize");
LPCTSTR sFontName = _T("FontName");
LPCTSTR sDefFontName = _T("MS Sans Serif");
LPCTSTR sExitOnEnter = _T("ExitOnEnter");
LPCTSTR sExitOnCtrlEnter = _T("ExitOnCtrlEnter");
LPCTSTR sFastYAlign = _T("FastYAlign");
//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC (CTEDialog, CAcUiDialog)

BEGIN_MESSAGE_MAP(CTEDialog, CAcUiDialog)
	//{{AFX_MSG_MAP(CTEDialog)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_TESELCHANGE, OnTESelChange)
	ON_WM_SETCURSOR()
	ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_EDIT, &CTEDialog::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_STYLE, &CTEDialog::OnEnChangeStyle)
	ON_EN_CHANGE(IDC_HEIGHT, &CTEDialog::OnEnChangeHeight)
	ON_EN_CHANGE(IDC_LINESPACE, &CTEDialog::OnEnChangeLineSpace)
	ON_EN_CHANGE(IDC_WIDTH, &CTEDialog::OnEnChangeWidth)
	ON_NOTIFY(EN_SELCHANGE, IDC_EDIT, CTEDialog::OnSelChange)
	ON_BN_CLICKED(IDCANCEL, &CTEDialog::OnClose)
	ON_BN_CLICKED(IDC_ALIGNX, &CTEDialog::OnAlignX)
	ON_BN_CLICKED(IDC_ALIGNY, &CTEDialog::OnAlignY)
	ON_COMMAND(ID_ALIGNX, &CTEDialog::OnAlignX)
	ON_COMMAND(ID_ALIGNY, &CTEDialog::OnAlignY)
	ON_COMMAND(ID_UPPERCASE, &CTEDialog::OnUCase)
	ON_COMMAND(ID_LOWERCASE, &CTEDialog::OnLCase)
	ON_COMMAND(ID_FIND, &CTEDialog::OnFind)
	ON_COMMAND(ID_REPLACE, &CTEDialog::OnReplace)
	ON_COMMAND(ID_ABOUT, &CTEDialog::OnAbout)
	ON_COMMAND_RANGE(ID_UNDO, ID_SELECTALL, OnEditMenuItems)
	ON_REGISTERED_MESSAGE(FindReplaceDialogMessage, OnFindReplaceMessage)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FONT, &CTEDialog::OnFont)
	ON_COMMAND(ID_OPTIONS_EXITONENTER, &CTEDialog::OnOptionExitOnEnter)
	ON_COMMAND(ID_OPTIONS_EXITONCTRLENTER, &CTEDialog::OnOptionExitOnCtrlEnter)
	ON_COMMAND(ID_OPTIONS_FASTYALIGN, &CTEDialog::OnOptionFastYAlign)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
CTEDialog *CTEDialog::dlg = NULL;
CFindReplaceDialog *CTEDialog::pFindDlg = NULL;
UINT CTEDialog::FindReplaceDialogMessage = RegisterWindowMessage(FINDMSGSTRING);
DWORD ExitOnEnter=0;
DWORD ExitOnCtrlEnter = 0;
DWORD FastYAlign = 0;
TCHAR EditorFontName[LF_FACESIZE];
DWORD EditorFontSize;
//-----------------------------------------------------------------------------
HMENU MyLoadMenu(HINSTANCE hInst, LPCTSTR MenuName) {
	LANGID langid = GetUserDefaultLangID();
	HRSRC hrsrc = ::FindResourceEx(hInst,RT_MENU,MenuName,langid);
	if (!hrsrc) {
		langid =  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
		hrsrc = ::FindResourceEx(hInst,RT_MENU,MenuName, langid);
	}
	HGLOBAL hglb = ::LoadResource(hInst, hrsrc);
	LPVOID lpsz = ::LockResource(hglb);
	return ::LoadMenuIndirect(lpsz);
}
//-----------------------------------------------------------------------------
BOOL FilterWinMsg(MSG* Amsg)
{
	if ((Amsg->message < WM_KEYFIRST)||(Amsg->message > WM_KEYLAST))
		return FALSE;
	if (GetParent(Amsg->hwnd) != CTEDialog::dlg->hwndDialog)
		return FALSE;
	return CTEDialog::dlg->PreTranslateMessage(Amsg);
}
//-----------------------------------------------------------------------------
LONG QueryRegDWORD(HKEY hkey, LPCTSTR ValueName, DWORD &AValue)
{
	DWORD datasize = sizeof(AValue);
	DWORD tmpVal;
	LONG res = RegQueryValueEx(hkey, ValueName, NULL, 
		NULL, (LPBYTE)&tmpVal,(LPDWORD)&datasize);
	if (ERROR_SUCCESS == res)
		AValue = tmpVal;
	return res;
}
/*---------------------------------------------*/
LONG QueryRegBuffer(HKEY hkey, LPCTSTR ValueName, int ASize, LPBYTE AValue)
{
	DWORD datasize = ASize;
	return RegQueryValueEx(hkey, ValueName, NULL, 
		NULL, (LPBYTE)AValue,(LPDWORD)&datasize);
}
/*---------------------------------------------*/
LONG CreateKey(HKEY hkey, LPCTSTR subkey, HKEY &hkResult)
{
	return RegCreateKeyEx(hkey, subkey, 0,
		_T(""), REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, &hkResult, NULL);
}
/*---------------------------------------------*/
void CTEDialog::SwitchItem(DWORD &ItemVar, LPCTSTR ItemName)
{
	HKEY prodKey;

	ItemVar = !ItemVar;
	UpdateMenuItems();
	if (CreateKey(HKEY_CURRENT_USER, pAppInfoLocFull, prodKey) == ERROR_SUCCESS)
	{
		RegSetValueEx(prodKey, ItemName, 0, REG_DWORD, 
			(CONST BYTE *)&ItemVar, sizeof(ItemVar));
		RegCloseKey(prodKey);
	}
}
//-----------------------------------------------------------------------------
void CTEDialog::UpdateRegistry()
{
    HKEY prodKey;
	if (CreateKey(HKEY_CURRENT_USER, pAppInfoLocFull, prodKey) != ERROR_SUCCESS)
		return;
	QueryRegDWORD(prodKey, sExitOnEnter, (DWORD&)ExitOnEnter);
	QueryRegDWORD(prodKey, sExitOnCtrlEnter, (DWORD&)ExitOnCtrlEnter);
	QueryRegDWORD(prodKey, sFastYAlign, (DWORD&)FastYAlign);
	if (QueryRegBuffer(prodKey, sFontName, sizeof(EditorFontName), 
		(LPBYTE)&EditorFontName) != ERROR_SUCCESS)
			lstrcpyn(EditorFontName, sDefFontName, LF_FACESIZE);
	if (QueryRegDWORD(prodKey, sFontSize, (DWORD&)EditorFontSize) != ERROR_SUCCESS)
		EditorFontSize = 10;
    RegCloseKey(prodKey);
}
//-----------------------------------------------------------------------------
INT_PTR CALLBACK AboutDlgProc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
	case WM_INITDIALOG:	{
		HWND hwndText = GetDlgItem(wnd, IDC_TEXT);
		SendMessage(hwndText, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
		LRESULT evmask = SendMessage(hwndText, EM_SETEVENTMASK, NULL, NULL);
		SendMessage(hwndText, EM_SETEVENTMASK, 0, evmask | ENM_LINK);
		SendMessage(hwndText, EM_AUTOURLDETECT, TRUE, 0); 
		SetDlgItemText(wnd, IDC_TEXT, CMsg(IDS_ABOUT)); 
		return TRUE;
	}
	case WM_COMMAND:
		switch(LOWORD(wparam)) {
			case IDCANCEL: EndDialog(wnd, 0);
		}
	case WM_NOTIFY:
		switch(wparam) {
		case IDC_TEXT: 
			{
				ENLINK * penLink = (ENLINK *)lparam;
				if (penLink->msg == WM_LBUTTONDOWN) {
					TEXTRANGE rg;
					TCHAR buf[100];
					rg.chrg = penLink->chrg;
					rg.lpstrText = (LPTSTR)&buf;
					SendMessage(GetDlgItem(wnd, IDC_TEXT), EM_GETTEXTRANGE, 0, (LPARAM)&rg);
					ShellExecute(wnd, _T("open"), rg.lpstrText, 0, 0, SW_SHOWNORMAL);
				}
					
			}
		}
    default:
		return FALSE;
	}
	return true;
}
//-----------------------------------------------------------------------------
void refreshDisplay()
{
        actrTransactionManager->queueForGraphicsFlush();
        actrTransactionManager->flushGraphics();
        acedUpdateDisplay();
}
//-----------------------------------------------------------------------------
void SetEntText(AcDbEntity *pEnt, CString& txt) {
	if (pEnt->isKindOf(AcDbMText::desc()))
		AcDbMText::cast(pEnt)->setContents(txt);
	else if (pEnt->isKindOf(AcDbDimension::desc()))
		AcDbDimension::cast(pEnt)->setDimensionText(txt);
}
//-----------------------------------------------------------------------------
void SetEntHeight(AcDbEntity *pEnt, double& h) {
	if (pEnt->isKindOf(AcDbMText::desc()))
		AcDbMText::cast(pEnt)->setTextHeight(h);
	else if (pEnt->isKindOf(AcDbDimension::desc()))
		AcDbDimension::cast(pEnt)->setDimtxt(h);
}
//-----------------------------------------------------------------------------

CTEDialog::CTEDialog (CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAcUiDialog (CTEDialog::IDD, pParent, hInstance) {
	SetDialogName(CMsg(IDS_DLGNAME));
}
//-----------------------------------------------------------------------------
void CTEDialog::Cleanup() {
	if (CTEDialog::dlg == NULL) return;
	delete CTEDialog::dlg;
	CTEDialog::dlg = NULL;
}
//-----------------------------------------------------------------------------
void CTEDialog::ShowDlg() {
	AfxInitRichEdit2();
	DoModal();
 }
//-----------------------------------------------------------------------------
void CTEDialog::OnInitDialogStart() {
}
//-----------------------------------------------------------------------------
BOOL CTEDialog::OnInitDialog() {
	AcDbEntity *pEnt;
	if (texttype==MTEXT){
		GetMTEXTLineSpace();
		AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForRead));
		TextHeight=((AcDbMText*)pEnt)->textHeight();
		WidthFactor=((AcDbMText*)pEnt)->width();
		pEnt->close();
	}
	else if (texttype==DIMENSION){
		AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForRead));
		TextHeight=((AcDbDimension*)pEnt)->dimtxt();
		pEnt->close();
	}
	else if (texttype==DTEXT) {
		GetDTEXTLineSpace();
	}

	CAcUiDialog::OnInitDialog();
	accel =::LoadAccelerators(_hdllInstance, MAKEINTRESOURCE(IDR_ACCEL));
	hwndDialog = m_hWnd;
	::SetMenu(m_hWnd, ::MyLoadMenu(_hdllInstance, MAKEINTRESOURCE(MENU1)));
	GetDlgItem(IDC_EDIT, &hwndEdit);
	GetDlgItem(IDC_STYLE, &hwndStyle);
	GetDlgItem(IDC_HEIGHT, &hwndHeight);
	GetDlgItem(IDC_LINESPACE, &hwndLineSpace);
	GetDlgItem(IDC_WIDTH, &hwndWidth);
	GetDlgItem(IDC_ALIGNX, &hwndAlignX);
	GetDlgItem(IDC_ALIGNY, &hwndAlignY);
	edEdit.SetTextMode(TM_PLAINTEXT);
	SetFont(EditorFontName, EditorFontSize);
	edEdit.SetEventMask(ENM_CHANGE | ENM_SELCHANGE | ENM_KEYEVENTS);
	FillStyles();

	if (texttype==DIMENSION)
	{
		GetDlgItem(IDC_LINESPACE)->EnableWindow(false);
		GetDlgItem(IDC_WIDTH)->EnableWindow(false);
	}
	if ((texttype==DIMENSION)||(texttype==MTEXT)) {
		cbStyle.SelectString(-1, GetTextStyle(edEndLn)); 
		GetDlgItem(IDC_ALIGNX)->EnableWindow(false);
		GetDlgItem(IDC_ALIGNY)->EnableWindow(false);
	}
	settext();
	UpdateMenuItems();
	useevent = true;
	return TRUE;
}
//-----------------------------------------------------------------------------
static DLGCTLINFO  dlginfo[] = {
    {IDCANCEL, MOVEX, 100},
    {IDC_STATUSBAR, ELASTICX | MOVEY, 100},
    {IDC_EDIT, ELASTICXY, 100},
};
void CTEDialog::OnInitDialogFinish() {
	SetControlProperty((PDLGCTLINFO)&dlginfo, sizeof(dlginfo)/sizeof(DLGCTLINFO));
	UpdateStatus();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnSize(UINT nType, int cx, int cy) {
	CAdUiDialog::OnSize(nType, cx, cy);
	UpdateStatus();
}
//-----------------------------------------------------------------------------
void AFXAPI DDX_CustomDouble(CDataExchange* pDX, int nIDC, double& value) {
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (pDX->m_bSaveAndValidate) {
		int nLen = ::GetWindowTextLength(hWndCtrl);
		TCHAR* strbuf = new TCHAR[++nLen];
		::GetWindowText(hWndCtrl, strbuf, nLen);
		int status = acdbDisToF(strbuf,-1,&value);
		delete strbuf;
	} else {
		TCHAR strbuf[30];
		acdbRToS(value,-1,-1,strbuf); 
		AfxSetWindowText(hWndCtrl, (TCHAR*)&strbuf);
	}
}
//-----------------------------------------------------------------------------
void CTEDialog::DoDataExchange (CDataExchange *pDX) {
	CAcUiDialog::DoDataExchange (pDX) ;
	DDX_Control(pDX, IDC_EDIT, edEdit);
	DDX_Control(pDX, IDCANCEL, btCancel);
	DDX_Control(pDX, IDC_STYLE, cbStyle);
	DDX_CustomDouble(pDX, IDC_HEIGHT, TextHeight);
	DDX_CustomDouble(pDX, IDC_LINESPACE, LineSpace);
	DDX_CustomDouble(pDX, IDC_WIDTH, WidthFactor);
	DDX_Control(pDX, IDC_ALIGNX, btAlignX);
	DDX_Control(pDX, IDC_ALIGNY, btAlignY);
	DDX_Control(pDX, IDC_STATUSBAR, stBar);
}
//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnKickIdle (WPARAM, LPARAM) {
	if (!bNeedUpdStbar) return false;
	bNeedUpdStbar = false;
	UINT txt = 0;
	if (ControlOnIdle == hwndStyle)
		txt = IDS_TEXTSTYLE;
	else if (ControlOnIdle == hwndHeight)
		txt = IDS_TEXTHEIGHT;
	else if (ControlOnIdle == hwndLineSpace)
		txt = IDS_LINESPACE;
	else if (ControlOnIdle == hwndWidth)
	{
		if (texttype == MTEXT)
			txt = IDS_MTEXTWIDTH;
		else 
			txt = IDS_WIDTHFACTOR;
	}
	else if (ControlOnIdle == hwndAlignX)
		txt = IDS_ALIGNX;
	else if (ControlOnIdle == hwndAlignY)
		txt = IDS_ALIGNY;
	stBar.SetText(CMsg(txt),0,0);
	return false;
}
//-----------------------------------------------------------------------------
void CTEDialog::SetFont(LPTSTR FontName, int FontSize)
{
	CHARFORMAT2 chf;
	ZeroMemory(&chf, sizeof(chf));
	chf.cbSize = sizeof(chf);
	chf.dwMask = CFM_SIZE | CFM_FACE;
	chf.wWeight = FW_REGULAR;

	HDC DC = ::GetDC(0);
	chf.yHeight = -MulDiv (FontSize*10, GetDeviceCaps(DC, LOGPIXELSY), 36); 
	::ReleaseDC(0,DC);
	
	lstrcpyn(chf.szFaceName, FontName, LF_FACESIZE);
	::SendMessage(hwndEdit, EM_SETCHARFORMAT, 0, LPARAM(&chf));
}
//-----------------------------------------------------------------------------
BOOL CTEDialog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) {
	if (ControlOnIdle != pWnd->GetSafeHwnd())
	{
		ControlOnIdle = pWnd->GetSafeHwnd();
		bNeedUpdStbar = true;
	}
	return CAcUiDialog::OnSetCursor(pWnd, nHitTest, message);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnUCase()
{
	CString s;
	rtfGetText(s, 2);
	s.MakeUpper();
	rtfSetText(s, 2);
	gettext();
} 
//-----------------------------------------------------------------------------
void CTEDialog::OnLCase()
{
	CString s;
	rtfGetText(s, 2);
	s.MakeLower();
	rtfSetText(s, 2);
	gettext();
} 
//-----------------------------------------------------------------------------
void CTEDialog::GetLine(int i, CString& s) {
	int li = edEdit.LineIndex(i);
	int linelen = edEdit.LineLength(li);
	edEdit.GetLine(i, s.GetBuffer(linelen+sizeof(int)), linelen+sizeof(int));
	s.ReleaseBufferSetLength(linelen);
}
//-----------------------------------------------------------------------------
void CTEDialog::SetInsPoint(AcDbObjectId EntId,AcGePoint3d inspoint)
{
	AcDbText * pEnt;
	AcGePoint3d pospoint,alignpoint,trans;
	TextHorzMode hormode;
	TextVertMode vertmode;
	AOK(acdbOpenAcDbEntity((AcDbEntity*&)pEnt,EntId, AcDb::kForRead));
	pospoint=pEnt->position();
	alignpoint=pEnt->alignmentPoint();
	hormode=pEnt->horizontalMode();
	vertmode=pEnt->verticalMode();
	pEnt->close();
	AOK(acdbOpenAcDbEntity((AcDbEntity*&)pEnt,EntId, AcDb::kForWrite));
	if ((vertmode==kTextBase)&&((hormode==kTextLeft)||(hormode==kTextAlign)||(hormode==kTextFit)))
		pEnt->setPosition(inspoint);
	else pEnt->setAlignmentPoint(inspoint);
	if ((hormode==kTextAlign)||(hormode==kTextFit)){
		alignpoint[0]+=inspoint[0]-pospoint[0];
		alignpoint[1]+=inspoint[1]-pospoint[1];
		alignpoint[2]+=inspoint[2]-pospoint[2];
		pEnt->setAlignmentPoint(alignpoint);
	}
	pEnt->close();
}
//-----------------------------------------------------------------------------
AcGePoint3d CTEDialog::GetInsPoint(AcDbObjectId EntId) {
	AcGePoint3d inspoint;
	AcDbText * pEnt;
	TextHorzMode hormode;
	TextVertMode vertmode;
	AOK(acdbOpenAcDbEntity((AcDbEntity*&)pEnt,EntId, AcDb::kForRead));
	hormode=pEnt->horizontalMode();
	vertmode=pEnt->verticalMode();
	if ((vertmode==kTextBase)&&((hormode==kTextLeft)||
		(hormode==kTextAlign)||(hormode==kTextFit)))
		inspoint=pEnt->position();
	else inspoint=pEnt->alignmentPoint();
	pEnt->close();
	return inspoint;
}
//-----------------------------------------------------------------------------
double CTEDialog::GetDTEXTLineSpace() {
	AcGePoint3d p1,p2;
	double d1,d2,d3,d4;
	AcDbObjectId newEntId, OwnerId;
	AcDbEntity * pEnt;
	if (DTextArr.length()>0) {
		p1=GetInsPoint(DTextArr.at(0));
		x_1=p1[0];
		y_1=p1[1];
		AOK(acdbOpenAcDbEntity(pEnt,DTextArr.at(0), AcDb::kForRead));
		TextHeight=AcDbText::cast(pEnt)->height();
		DtextAngle=AcDbText::cast(pEnt)->rotation(); 
		sin_a=sin(DtextAngle);
		cos_a=cos(DtextAngle);
		pEnt->close();
	}
	if (DTextArr.length()>1) 
	{
		p2=GetInsPoint(DTextArr[1]);
		d1=p1[1]*cos_a;
		d2=p1[0]*sin_a;
		d3=p2[1]*cos_a;
		d4=p2[0]*sin_a;
	    LineSpace=d1-d2-d3+d4;
	}
	dx=LineSpace*sin_a;
	dy=LineSpace*cos_a;
	return LineSpace;
}
//-----------------------------------------------------------------------------
double CTEDialog::GetMTEXTLineSpace()
{
	AcDbEntity * pEnt;
	AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForRead));
	LineSpace = ((AcDbMText*)pEnt)->lineSpacingFactor();
	pEnt->close();
	return LineSpace;
}
//-----------------------------------------------------------------------------
void CTEDialog::SortDText()
{
	bool sorted=false;
	int len=DTextArr.length(); 
	AcGePoint3d p;
	double x_1,x_2,y_1,y_2;
    while (!sorted)
	{
		sorted=true;
		for (int i=1;i<len;i++)
		{
			p=GetInsPoint(DTextArr.at(i-1));
			x_1=p[0];
			y_1=p[1]*cos_a-x_1*sin_a;
			p=GetInsPoint(DTextArr.at(i));
			x_2=p[0];
			y_2=p[1]*cos_a-x_2*sin_a;
			if (y_2>y_1)
			{
				DTextArr.swap((i-1),i);
				sorted=false;
			}
		}
	}
}
//-----------------------------------------------------------------------------
bool CTEDialog::ParseSset() {
    AcDbEntity * pEnt;
	ads_name ent;
	Adesk::Int32 slen;
	acedSSLength(sset,&slen);
	if (slen==0) return false;
	for (long i=0;i<slen;i++)
	{
		acedSSName(sset,i,ent);
		acdbGetObjectId(EntId, ent);
		AOK(acdbOpenAcDbEntity(pEnt, EntId, AcDb::kForRead));
		if (pEnt->isKindOf(AcDbText::desc()))
			DTextArr.append(EntId);
		pEnt->close();
	}
	texttype = DTEXT;
	if (DTextArr.logicalLength()==0)
	{
		acedSSName(sset,0,ent);
		acdbGetObjectId(EntId, ent);
		AOK(acdbOpenAcDbEntity(pEnt, EntId, AcDb::kForRead));
		if (pEnt->isKindOf(AcDbMText::desc()))
			texttype = MTEXT;
		else if (pEnt->isKindOf(AcDbDimension::desc()))
			texttype = DIMENSION;
		else {
			texttype = UNKNOWN;
			acutPrintf(CMsg(IDS_ALLOWEDENT));
		}
		pEnt->close();
	}
	if (texttype == UNKNOWN) return false;
	if (texttype == DTEXT) //DTEXT
	{
		GetDTEXTLineSpace();
		SortDText();
	}
	return true;
}
//-----------------------------------------------------------------------------
void CTEDialog::UpdateStatus(){
	CRect rect;
	stBar.GetClientRect(&rect);
	int r = rect.right;
	int widths[4] = { r-190,r-140, r-90, r };  
	stBar.SetParts(4,widths);
	stBar.SetText(texttypenames[texttype], 2,0);
}
//-----------------------------------------------------------------------------
void CTEDialog::FillStyles() {
	AcDbTextStyleTable* pStyleTable = NULL;
	AOK(acdbHostApplicationServices()->workingDatabase()->getTextStyleTable(pStyleTable, AcDb::kForRead));
	AcDbTextStyleTableIterator* pIterator =NULL;
	pStyleTable->newIterator(pIterator);
	AcDbTextStyleTableRecord *pStyle = NULL;
	const TCHAR * name;
	cbStyle.ResetContent();
	while (!pIterator->done())
	{
		pIterator->getRecord(pStyle, AcDb::kForRead);
		pStyle->getName(name);
		if(name != NULL)
			if (name[0] != NULL)
				cbStyle.AddString(name);
		pStyle->close();
		pIterator->step();
	}
	delete pIterator;
	pStyleTable->close();
}
//-----------------------------------------------------------------------------
const TCHAR* CTEDialog::GetTextStyle(int index)
{
	AcDbObjectId TextStyleId, ObjId;
	AcDbEntity * pEnt;
	const TCHAR * str;
	AcDbTextStyleTableRecord * StyleTableRecord;
	if (texttype==DTEXT) ObjId = DTextArr[index];
	else ObjId = EntId;
	AOK(acdbOpenAcDbEntity(pEnt, ObjId, AcDb::kForRead));
	if (texttype==DTEXT)
		TextStyleId = AcDbText::cast(pEnt)->textStyle();
	else if (texttype==MTEXT)
		TextStyleId = AcDbMText::cast(pEnt)->textStyle();
	else if (texttype==DIMENSION)
		TextStyleId = AcDbDimension::cast(pEnt)->dimtxsty();
	pEnt->close();
	AOK(acdbOpenAcDbObject((AcDbObject*&)StyleTableRecord, TextStyleId,AcDb::kForRead));
	StyleTableRecord->getName(str);
	StyleTableRecord->close();
	return str;
}
//-----------------------------------------------------------------------------
void CTEDialog::GetStyleId(CString& s, AcDbObjectId& StyleId)
{
	AcDbTextStyleTable* pStyleTable;
	AcDbTextStyleTableRecord *pStyle;
	const TCHAR * name;
	AOK(acdbHostApplicationServices()->workingDatabase()->getTextStyleTable(pStyleTable, AcDb::kForRead));
	AcDbTextStyleTableIterator* pIterator;
	pStyleTable->newIterator(pIterator);
	for( ; !pIterator->done(); pIterator->step())
	{
		AOK(pIterator->getRecord(pStyle, AcDb::kForRead));
		pStyle->getName(name);
		if(name != NULL)
			if (_tcscmp(name,s)==0) 
				StyleId = pStyle->objectId();
		pStyle->close();
	}
	delete pIterator;
	pStyleTable->close();
}
//-----------------------------------------------------------------------------
void CTEDialog::SetTextStyle(int index, CString& s)
{
	AcDbObjectId StyleId, ObjId;
	AcDbEntity * pEnt;
	GetStyleId(s, StyleId);
	if (texttype==DTEXT) ObjId = DTextArr[index];
	else ObjId = EntId;
	AOK(acdbOpenAcDbEntity(pEnt, ObjId, AcDb::kForWrite));
	if (texttype==DTEXT)
		AcDbText::cast(pEnt)->setTextStyle(StyleId);
	else if (texttype==MTEXT)
		AcDbMText::cast(pEnt)->setTextStyle(StyleId);
	else if (texttype==DIMENSION)
		AcDbDimension::cast(pEnt)->setDimtxsty(StyleId);
	pEnt->close();
}
//-----------------------------------------------------------------------------
void CTEDialog::UpdateMenuItems() {
	int IsCheck  = MF_UNCHECKED;
	CMenu * mnu = GetMenu();
	mnu->CheckMenuItem(ID_OPTIONS_EXITONENTER, MF_BYCOMMAND | 
		(ExitOnEnter ? MF_CHECKED : MF_UNCHECKED)); 
	mnu->CheckMenuItem(ID_OPTIONS_EXITONCTRLENTER, MF_BYCOMMAND | 
		(ExitOnCtrlEnter ? MF_CHECKED : MF_UNCHECKED)); 
	mnu->CheckMenuItem(ID_OPTIONS_FASTYALIGN, MF_BYCOMMAND | 
		(FastYAlign ? MF_CHECKED : MF_UNCHECKED)); 
	
	if (texttype != DTEXT)
	{
		mnu->EnableMenuItem(ID_ALIGNX, MF_BYCOMMAND | MF_GRAYED);	
		mnu->EnableMenuItem(ID_ALIGNY, MF_BYCOMMAND | MF_GRAYED);	
	}
}
//-----------------------------------------------------------------------------
void CTEDialog::GetObjIdText(AcDbObjectId eId, TETextType tt, CString &str) {
	AcDbEntity *pEnt;
	AOK(acdbOpenAcDbEntity(pEnt,eId, AcDb::kForRead));
	GetEntText(pEnt, tt, str);
	pEnt->close();
}
//-----------------------------------------------------------------------------
void CTEDialog::GetEntText(AcDbEntity *pEnt, TETextType tt, CString &str) {
	const ACHAR *s = NULL;
#ifdef _BRX
	if (tt == DTEXT) s=((AcDbText*)pEnt)->textString();
	else if (tt == MTEXT) s=((AcDbMText*)pEnt)->contents();
	else if (tt == DIMENSION) s=((AcDbDimension*)pEnt)->dimensionText();
	str = s;
	if (s != NULL) delete s;
#else
	if (pEnt->hasFields()) {
		AcDbField * pField;
		pEnt->getField(_T("TEXT"), pField, AcDb::kForRead);
		s = pField->getFieldCode(AcDbField::kAddMarkers);            
		str = s;
		pField->close();
	} 
	else {
		if (tt == DTEXT) s=((AcDbText*)pEnt)->textString();
		else if (tt == MTEXT) s=((AcDbMText*)pEnt)->contents();
		else if (tt == DIMENSION) s=((AcDbDimension*)pEnt)->dimensionText();
		str = s;
		if (s != NULL) delete s;
	}
#endif
}
//-----------------------------------------------------------------------------
void CTEDialog::rtfGetText(CString &s, DWORD flags)
{
	int textlen;
	textlen = edEdit.GetTextLengthEx(flags, 1200);
	LPTSTR buf = s.GetBuffer(textlen);
	GETTEXTEX st;
	st.cb = (textlen+1)*2;
	st.flags = flags;
	st.codepage = 1200;
	st.lpDefaultChar = NULL;
	st.lpUsedDefChar = NULL;
	edEdit.SendMessage(EM_GETTEXTEX, (WPARAM)&st, (LPARAM)buf);
	s.ReleaseBuffer();
	return;
}
//-----------------------------------------------------------------------------
void CTEDialog::rtfSetText(const CString &s, DWORD flags) {
	long selstart, selend;
	edEdit.GetSel(selstart, selend);
	SETTEXTEX st;
	st.flags = flags;
	st.codepage = 1200;
	DWORD oldmask = edEdit.GetEventMask();
    edEdit.SetEventMask(oldmask & ~ENM_CHANGE);
	edEdit.SendMessage(EM_SETTEXTEX, (WPARAM)&st, (LPARAM)(LPCTSTR)s);
	edEdit.SendMessage(EM_SETSEL, selstart, selend);
    edEdit.SetEventMask(oldmask);
}
//-----------------------------------------------------------------------------
void CTEDialog::gettext()
{
	AcGePoint3d textpos;
	AcDbObjectId newEntId, OwnerId;
	AcDbEntity *pEnt, *pNewEnt;
	CString s;
	AcAxDocLock lock(acdbCurDwg());
	int linecount,dtextcount,mincount;//, textlen;
	if ((texttype==MTEXT) || (texttype==DIMENSION)) {
		rtfGetText(s, GT_DEFAULT/*GT_USECRLF*/); 
		s.Replace(_T("\r"), _T("\\P"));
		AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForWrite));
		SetEntText(pEnt, s);
		pEnt->close();
#ifndef _BRX
		acdbEvaluateFields(EntId, AcDbField::kPreview);
#endif
		refreshDisplay();
		return;
	}
	//DTEXT
	//ControlDTEXTLimit(hWnd);
	linecount=edEdit.GetLineCount();
	dtextcount=DTextArr.length();
	mincount=dtextcount>linecount?linecount:dtextcount;
	for (int i=0;i<mincount;i++)
	{
		GetLine(i,s);
		AOK(acdbOpenAcDbEntity(pEnt,DTextArr[i], AcDb::kForWrite));
		AcDbText::cast(pEnt)->setTextString(s);
		pEnt->close();
#ifndef _BRX
		acdbEvaluateFields(DTextArr[i], AcDbField::kPreview);
#endif
	}

	if (dtextcount>=linecount) //кол-во строк убавились или осталось прежним
		for (int i=dtextcount-1;i>=linecount;i--)
		{
			AOK(acdbOpenAcDbEntity(pEnt,DTextArr[i], AcDb::kForWrite));
			DTextArr.removeAt(i);
			pEnt->erase();
			pEnt->close();
		}
	else //добавились строки.
		for (int i=dtextcount;i<linecount;i++)
		{
			GetLine(i,s);
			EntId=DTextArr[i-1];
			AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForRead));
			pNewEnt=AcDbEntity::cast(pEnt->clone());
			OwnerId=pEnt->ownerId();
			pEnt->close();
			AcDbBlockTableRecord *pBlockTableRecord;
			AOK(acdbOpenAcDbObject((AcDbObject*&)pBlockTableRecord,OwnerId,AcDb::kForWrite));
			pBlockTableRecord->appendAcDbEntity(newEntId, pNewEnt);
			pBlockTableRecord->close();
			AcDbText::cast(pNewEnt)->setTextString(s);
			pNewEnt->close();
#ifndef _BRX
			acdbEvaluateFields(newEntId, AcDbField::kPreview);
#endif
			textpos=GetInsPoint(EntId);
			textpos[0]+=dx;
			textpos[1]-=dy;
			SetInsPoint(newEntId,textpos);
			DTextArr.append(newEntId);
		}
	refreshDisplay();
	return;
}//gettext
//-----------------------------------------------------------------------------
void CTEDialog::settext()
{
	CString s,tmps;
	int arrlen = DTextArr.length();
	if (texttype==DTEXT){
		for (int i=0;i<arrlen;i++)
		{
			GetObjIdText(DTextArr[i], texttype, tmps);
			if (i>0) s.Append(_T("\r\n"));
			s.Append(tmps);
		}
	}
	else {
		GetObjIdText(EntId, texttype, s);
		s.Replace(_T("\\P"), _T("\r\n")); 
	}
	rtfSetText(s, 0); 
	return;
}
//-----------------------------------------------------------------------------
void CTEDialog::ShowTextParams(long edStartLn, long edEndLn)
{
	AcDbText * pEnt;
	double prevHeight, Height;
	double prevwf, wf;
	AcDbObjectId prevStyleId, StyleId;
	ACHAR strbuf[30];
	if (texttype != DTEXT)
		return;
	useevent = false;
	bool heightequal=true, styleequal=true, wfequal=true;
	for (long i=edStartLn; i<=edEndLn; i++)
	{
		AOK(acdbOpenAcDbEntity((AcDbEntity*&)pEnt,DTextArr[i], AcDb::kForRead));
		Height=pEnt->height();
		wf=pEnt->widthFactor();
		StyleId=pEnt->textStyle();
		pEnt->close();
		if (i==edStartLn)
		{
			prevHeight = Height;
			prevwf = wf;
			prevStyleId = StyleId;
			continue;
		}
		if (prevHeight != Height)
			heightequal=false;
		if (prevwf != wf)
			wfequal=false;
		if (prevStyleId != StyleId)
			styleequal=false;
	}
	if (heightequal)
		acdbRToS(Height,-1,-1,strbuf);
	else
		strbuf[0]=0;
	GetDlgItem(IDC_HEIGHT)->SetWindowText(strbuf);
	if (wfequal)
		acdbRToS(wf,-1,-1,strbuf);
	else
		strbuf[0]=0;
	GetDlgItem(IDC_WIDTH)->SetWindowText(strbuf);
	if (styleequal)
		cbStyle.SelectString(-1,GetTextStyle(edEndLn)); 
	else
		cbStyle.SetCurSel(-1);
	useevent = true;

}
//-----------------------------------------------------------------------------
void CTEDialog::GetSelParams() {
	edEdit.GetSel(edSelStart, edSelEnd);
	edEndLn = edEdit.LineFromChar(edSelEnd);
	edStartLn = edEdit.LineFromChar(edSelStart);
	edCol = edSelEnd - edEdit.LineIndex(edEndLn);
}
//-----------------------------------------------------------------------------
void CTEDialog::ShowCarPos() {
	GetSelParams();
	CString s;
	s.Format(CMsg(IDS_CURSORPOS),edEndLn+1,edCol+1); 
	stBar.SetText(s, 3, 0);
}
//-----------------------------------------------------------------------------
int CTEDialog::GetDlgItemFloat(int Item, double &AValue)
{
	HWND wnd = ::GetDlgItem(hwndDialog, Item); 
	UINT sellen = ::GetWindowTextLength(wnd);
	LPTSTR strbuf = new TCHAR[++sellen];
	::GetWindowText(wnd, strbuf, sellen);
	int status = acdbDisToF(strbuf,-1,&AValue);
	delete strbuf;
	return status;
}
//-----------------------------------------------------------------------------
void CTEDialog::SetDlgItemFloat(int Item, double AValue)
{
	TCHAR strbuf[30];
	acdbRToS(AValue,-1,-1,strbuf); 
	::SetWindowText(::GetDlgItem(hwndDialog, Item), (LPTSTR)&strbuf);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnClose()
{
	EndDialog(FALSE);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnAlignX()
{
	if (texttype != DTEXT)
		return;
	double x1,x2,y2,y_2;
	AcGePoint3d p;
	x1=x_1*cos_a+y_1*sin_a;
	AcAxDocLock lock(acdbCurDwg());
	for (int i=1;i<DTextArr.length();i++)
	{
		p=GetInsPoint(DTextArr.at(i));
		x2=p[0];
		y2=p[1];
		y_2=x2*sin_a-y2*cos_a;
		p[0]=x1*cos_a+y_2*sin_a;
		p[1]=x1*sin_a-y_2*cos_a;
		SetInsPoint(DTextArr.at(i),p);
	}
	refreshDisplay();
	ShowCarPos();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnAlignY()
{
	if (texttype != DTEXT)
		return;
	double x2,y1,y2,x_2;
	AcGePoint3d p;
	y1=x_1*sin_a-y_1*cos_a;
	AcAxDocLock lock(acdbCurDwg());
	for (int i=1;i<DTextArr.length();i++)
	{
		p=GetInsPoint(DTextArr.at(i));
		x2=p[0];
		y2=p[1];
		x_2=x2*cos_a+y2*sin_a;
		p[0]=x_2*cos_a+(y1+LineSpace*i)*sin_a;
		p[1]=x_2*sin_a-(y1+LineSpace*i)*cos_a;
		SetInsPoint(DTextArr.at(i),p);
	}
	refreshDisplay();
	ShowCarPos();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeEdit1()
{
	gettext();
}
//-----------------------------------------------------------------------------
void CTEDialog::setDTEXTHeight(int index, double height)
{
	AcDbEntity * pEnt;
	AOK(acdbOpenAcDbEntity(pEnt,DTextArr.at(index), AcDb::kForWrite));
	AcDbText::cast(pEnt)->setHeight(height);
	pEnt->close();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeStyle()
{
	CString s;
	if (!useevent) return;
	AcAxDocLock lock(acdbCurDwg());
	cbStyle.GetWindowText(s);
	GetSelParams();
	long startline = edStartLn;
	if (texttype==DTEXT) {
		while (startline<=edEndLn) SetTextStyle(startline++,s);
	} else if ((texttype==MTEXT) || (texttype==DIMENSION))
		SetTextStyle(0, s); 
	refreshDisplay();
	ShowCarPos();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeHeight()
{
	int res;
	double textheight;
	if (!useevent) return;
	AcAxDocLock lock(acdbCurDwg());
	res = GetDlgItemFloat(IDC_HEIGHT, textheight);
	// TODO !!! if error then show message in status window
	if (res!=RTNORM) return;
	GetSelParams();
	long startline = edStartLn;
	if (texttype==DTEXT) {
		while (startline<=edEndLn) setDTEXTHeight(startline++,textheight);
	} else if ((texttype==MTEXT) || (texttype==DIMENSION)) {
		AcDbEntity * pEnt;
		AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForWrite));
		SetEntHeight(pEnt, textheight); 
		pEnt->close();
	}
	refreshDisplay();
	ShowCarPos();
}
//-----------------------------------------------------------------------------
Acad::ErrorStatus CTEDialog::setMTextLSpace(double& lspace)
{
	AcDbEntity * pEnt;
	AOK(acdbOpenAcDbEntity(pEnt,EntId, AcDb::kForWrite));
	Acad::ErrorStatus res = ((AcDbMText*)pEnt)->setLineSpacingFactor(LineSpace);
	pEnt->close();
	return res;
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeLineSpace()
{
	int res;
	if (!useevent) return;
	AcAxDocLock lock(acdbCurDwg());
	res = GetDlgItemFloat(IDC_LINESPACE, LineSpace);
	// TODO !!! if error then show message in status window
	if (res!=RTNORM) return;
	// TODO: make common function to get selection params
	if (texttype==DTEXT)
	{
		dx=LineSpace*sin_a;
		dy=LineSpace*cos_a;
		if (FastYAlign) 
			OnAlignY();
	}
	else if (texttype==MTEXT)
		if (setMTextLSpace(LineSpace) == eOk)
			refreshDisplay();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeWidth()
{
	int res;
	double w;
	AcDbEntity * pEnt;
	if (!useevent) return;
	AcAxDocLock lock(acdbCurDwg());
	res = GetDlgItemFloat(IDC_WIDTH, w);
	// TODO !!! if error then show message in status window
	if (res!=RTNORM) return;
	GetSelParams();
	long startline = edStartLn;
	if (texttype==DTEXT) {
	while (startline<=edEndLn) {
		AOK(acdbOpenAcDbEntity(pEnt,DTextArr[startline++], AcDb::kForWrite));
		AcDbText::cast(pEnt)->setWidthFactor(w);
		pEnt->close();
	}
	} else if (texttype==MTEXT) {
		AcDbMText * pEnt;
		AOK(acdbOpenAcDbEntity((AcDbEntity*&)pEnt,EntId, AcDb::kForWrite));
		pEnt->setWidth(w);
		pEnt->close();
	}
	refreshDisplay();
	ShowCarPos();
}
//-----------------------------------------------------------------------------
void CTEDialog::OnSelChange(NMHDR *pSel, LRESULT* result ) {
	::PostMessage(this->m_hWnd, WM_TESELCHANGE, 0, 0);
}
//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnTESelChange(WPARAM, LPARAM) {
	ShowCarPos();
	ShowTextParams(edStartLn, edEndLn);
	return (TRUE);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnFind()
{
	pFindDlg = new CFindReplaceDialog();
	pFindDlg->Create(TRUE, NULL, NULL, FR_DOWN, this);
	pFindDlg->SetActiveWindow();
	pFindDlg->ShowWindow(TRUE);
} 
//-----------------------------------------------------------------------------
void CTEDialog::OnReplace()
{
	pFindDlg = new CFindReplaceDialog();
	pFindDlg->Create(FALSE, NULL, NULL, FR_DOWN, this);
	pFindDlg->SetActiveWindow();
	pFindDlg->ShowWindow(TRUE);
} 
//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnFindReplaceMessage(WPARAM wParam, LPARAM lParam)
{
	pFindDlg = CFindReplaceDialog::GetNotifier(lParam);
    ASSERT(pFindDlg != NULL);
	if (pFindDlg->IsTerminating()) {
        pFindDlg = NULL;
		return 0;
	}
	FINDREPLACE &fr = pFindDlg->m_fr;
	if (pFindDlg->FindNext()) {
		if (!TryFoundMore(fr))
			::MessageBox((pFindDlg)?pFindDlg->m_hWnd:hwndDialog, CMsg(IDS_SEARCHFIN), CMsg(IDS_INFO), MB_OK);
	}
	else if (pFindDlg->ReplaceCurrent())
	{
		if (TryFoundMore(fr)) 
			::SendMessage(hwndEdit, EM_REPLACESEL, TRUE, LPARAM(fr.lpstrReplaceWith));
		else
			::MessageBox((pFindDlg)?pFindDlg->m_hWnd:hwndDialog, CMsg(IDS_SEARCHFIN), CMsg(IDS_INFO), MB_OK);
	}
	else if (fr.Flags & FR_REPLACEALL) {
		int i = 0;
		CString msg;
		while (TryFoundMore(fr)) {
			::SendMessage(hwndEdit, EM_REPLACESEL, TRUE, LPARAM(fr.lpstrReplaceWith));
			i++;
		}
		if (i==0)
			msg = CMsg(IDS_SEARCHFIN);
		else {
			msg = CFMsg(IDS_REPLACED, i);
		}
		::MessageBox((pFindDlg)?pFindDlg->m_hWnd:hwndDialog, msg, CMsg(IDS_INFO), MB_OK);
		
	}
	return 0;
} 
//-----------------------------------------------------------------------------
bool CTEDialog::TryFoundMore(FINDREPLACE &fr)
{
	FINDTEXT fntext;
	fntext.lpstrText = fr.lpstrFindWhat;
	GetSelParams();
	fntext.chrg.cpMax = -1;
	if (fr.Flags & FR_DOWN)
		fntext.chrg.cpMin = edSelEnd;
	else 
		fntext.chrg.cpMin = edSelStart;
	LRESULT FindPos = ::SendMessage(hwndEdit, EM_FINDTEXT, fr.Flags, (LPARAM)&fntext);
	if (-1 != FindPos)
	{
		::SendMessage(hwndEdit, EM_SETSEL, FindPos, FindPos+wcslen(fr.lpstrFindWhat));
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
//	HMENU hmenu;   // top-level menu 
	HMENU hPopup;  // shortcut menu 
//	if ((hmenu = MyLoadMenu(_hdllInstance, MAKEINTRESOURCE(MENU1))) == NULL) 
//		return 0;
//	hPopup = GetSubMenu(hmenu, 0);
	hPopup = ::GetSubMenu(::GetMenu(m_hWnd), 0);
//	int cmd = TrackPopupMenuEx(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, 
//		GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), wnd, NULL); 
	int cmd = TrackPopupMenuEx(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
		GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwndDialog, NULL); 
	//DestroyMenu(hPopup);
//	MenuCommand(cmd);
	::SetFocus(hwndEdit);
	return 0;	
}
//-----------------------------------------------------------------------------
void CTEDialog::OnEditMenuItems(UINT nID)
{
	UINT msg; 
	LPARAM lp = 0;
	switch (nID) {
	case ID_SELECTALL: msg = EM_SETSEL, lp = -1; break;
	case ID_CUT: msg = WM_CUT; break;
	case ID_COPY: msg = WM_COPY; break;
	case ID_PASTE: msg = WM_PASTE; break;
	case ID_UNDO: msg = EM_UNDO; break;
	case ID_REDO: msg = EM_REDO; break;
	}
	::SendMessage(hwndEdit, msg, 0, lp);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnAbout()
{
	DialogBox(_hdllInstance,MAKEINTRESOURCE(ABOUTDLG),hwndDialog,&AboutDlgProc);	
} 
//-----------------------------------------------------------------------------
void CTEDialog::OnFont()
{
	CHOOSEFONT cf;           
	LOGFONT lf;       

	ZeroMemory(&lf, sizeof(lf));
	lstrcpyn(lf.lfFaceName, EditorFontName, LF_FACESIZE);
	lf.lfCharSet = DEFAULT_CHARSET;

	HDC DC = ::GetDC(0);
	lf.lfHeight = -MulDiv (EditorFontSize, GetDeviceCaps(DC, LOGPIXELSY), 72); 
	::ReleaseDC(0,DC);

	ZeroMemory(&cf, sizeof(CHOOSEFONT));
	cf.lStructSize = sizeof (CHOOSEFONT);
	cf.hwndOwner = hwndDialog;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;

	if (ChooseFont(&cf))
	{
		EditorFontSize = cf.iPointSize/10;
		lstrcpyn(EditorFontName, lf.lfFaceName, LF_FACESIZE);
	
		HKEY prodKey;
		if (CreateKey(HKEY_CURRENT_USER, pAppInfoLocFull, prodKey) == ERROR_SUCCESS)
		{
			RegSetValueEx(prodKey, sFontSize, 0, REG_DWORD, 
				(LPBYTE)&EditorFontSize, sizeof(EditorFontSize));
			RegSetValueEx(prodKey, sFontName, 0, REG_SZ, 
				(LPBYTE)&EditorFontName, sizeof(EditorFontName));
		}
		SetFont(EditorFontName, EditorFontSize);
	}
}
//-----------------------------------------------------------------------------
void CTEDialog::OnOptionExitOnEnter() {
	SwitchItem(ExitOnEnter, sExitOnEnter);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnOptionExitOnCtrlEnter() {
	SwitchItem(ExitOnCtrlEnter, sExitOnCtrlEnter);
}
//-----------------------------------------------------------------------------
void CTEDialog::OnOptionFastYAlign() {
	SwitchItem(FastYAlign, sFastYAlign);
}
//-----------------------------------------------------------------------------
BOOL CTEDialog::PreTranslateMessage(MSG* pMsg)
{
	if (TranslateAccelerator(hwndDialog, accel, pMsg))
		return TRUE;
	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
	{
		if ((pMsg->hwnd == hwndHeight) || (pMsg->hwnd == hwndLineSpace) ||
			(pMsg->hwnd == hwndWidth)) {
			::SetFocus(hwndEdit);
			return TRUE;
		}
	}

	if (pMsg->hwnd == hwndEdit) {
		if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
			if (ExitOnEnter || (ExitOnCtrlEnter && (::GetKeyState(VK_CONTROL) < 0))) {
				OnClose();
				return TRUE;
			}
	}
#ifdef _BRX
	if (pMsg->message == WM_KEYDOWN) 
		return ::IsDialogMessage(CTEDialog::dlg->hwndDialog, pMsg);
	return FALSE;
#else
	return CAcUiDialog::PreTranslateMessage(pMsg);
#endif
}
//-----------------------------------------------------------------------------
