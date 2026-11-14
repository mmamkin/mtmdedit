//-----------------------------------------------------------------------------
//----- TEDialog.cpp : Implementation of CTEDialog
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "TEDialog.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

namespace {

const LPCTSTR kTextTypeNames[] = { _T("UNKNOWN"), _T("DTEXT"), _T("MTEXT"), _T("DIM") };
const LPCTSTR kRegKey = _T("Software\\Mtmlab\\MtmdEdit");
//- A monospaced default makes control codes (\P, %%d, {\fArial;...}) easier
//- to edit than the old "MS Sans Serif" default.
const LPCTSTR kDefaultFontName = _T("Consolas");

//- AutoCAD's TEXT command places the next line 5/3 of the text height below
//- the previous one; used when only one DTEXT is selected and there is no
//- second line to measure the spacing from.
constexpr double kDefaultLineSpaceFactor = 5.0 / 3.0;

//- Rich edit code page for UTF-16 text (EM_GETTEXTEX / EM_SETTEXTEX)
constexpr UINT kCodePageUtf16 = 1200;

//-----------------------------------------------------------------------------
HMENU LoadLocalizedMenu(HINSTANCE hInst, LPCTSTR menuName)
{
	HRSRC hrsrc = ::FindResourceEx(hInst, RT_MENU, menuName, GetUserDefaultLangID());
	if (!hrsrc)
		hrsrc = ::FindResourceEx(hInst, RT_MENU, menuName, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	if (!hrsrc)
		return nullptr;
	return ::LoadMenuIndirect(::LockResource(::LoadResource(hInst, hrsrc)));
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK AboutDlgProc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	case WM_INITDIALOG: {
		HWND hwndText = ::GetDlgItem(wnd, IDC_TEXT);
		::SendMessage(hwndText, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
		const LRESULT evmask = ::SendMessage(hwndText, EM_GETEVENTMASK, 0, 0);
		::SendMessage(hwndText, EM_SETEVENTMASK, 0, evmask | ENM_LINK);
		::SendMessage(hwndText, EM_AUTOURLDETECT, TRUE, 0);
		::SetDlgItemText(wnd, IDC_TEXT, LoadMsg(IDS_ABOUT));
		return TRUE;
	}
	case WM_COMMAND:
		//- The old code fell through into WM_NOTIFY here and read lparam as an ENLINK.
		if (LOWORD(wparam) == IDCANCEL || LOWORD(wparam) == IDOK) {
			::EndDialog(wnd, 0);
			return TRUE;
		}
		break;
	case WM_NOTIFY: {
		auto *pLink = reinterpret_cast<ENLINK *>(lparam);
		if (wparam == IDC_TEXT && pLink->nmhdr.code == EN_LINK && pLink->msg == WM_LBUTTONUP) {
			CString url;
			const LONG len = pLink->chrg.cpMax - pLink->chrg.cpMin;
			TEXTRANGE rg { pLink->chrg, url.GetBuffer(len + 1) };
			::SendMessage(pLink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&rg));
			url.ReleaseBuffer();
			::ShellExecute(wnd, _T("open"), url, nullptr, nullptr, SW_SHOWNORMAL);
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
void RefreshDisplay()
{
	actrTransactionManager->queueForGraphicsFlush();
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}

//-----------------------------------------------------------------------------
//- Returns the editable text of a TEXT/MTEXT/DIMENSION. When the text holds a
//- field, the raw field code is returned so the user can edit it.
CString ReadEntityText(AcDbEntity *pEnt)
{
	AcString s;
	if (pEnt->hasFields()) {
		AcDbField *pField = nullptr;
		if (pEnt->getField(_T("TEXT"), pField, AcDb::kForRead) == Acad::eOk && pField != nullptr) {
			pField->getFieldCode(s, AcDbField::kAddMarkers);
			pField->close();
			return CString(s.kwszPtr());
		}
	}
	if (auto *pText = AcDbText::cast(pEnt))
		pText->textString(s);
	else if (auto *pMText = AcDbMText::cast(pEnt))
		pMText->contents(s);
	else if (auto *pDim = AcDbDimension::cast(pEnt))
		pDim->dimensionText(s);
	return CString(s.kwszPtr());
}

//-----------------------------------------------------------------------------
CString ReadEntityText(AcDbObjectId id)
{
	AcDbEntityPointer pEnt(id, AcDb::kForRead);
	return pEnt.openStatus() == Acad::eOk ? ReadEntityText(pEnt.object()) : CString();
}

//-----------------------------------------------------------------------------
AcDbObjectId FindTextStyle(const CString &name)
{
	AcDbObjectId id;
	AcDbTextStyleTable *pTable = nullptr;
	if (acdbHostApplicationServices()->workingDatabase()->getTextStyleTable(pTable, AcDb::kForRead) == Acad::eOk) {
		pTable->getAt(name, id);
		pTable->close();
	}
	return id;
}

//-----------------------------------------------------------------------------
//- Copies a DTEXT next to the original. deepCloneObjects is used instead of
//- clone(): clone() copies the extension dictionary id as is, so a source line
//- with fields or annotative scales would share its dictionary with the copy.
AcDbObjectId CloneDText(AcDbObjectId srcId)
{
	AcDbObjectId ownerId;
	{
		AcDbEntityPointer pSrc(srcId, AcDb::kForRead);
		if (pSrc.openStatus() != Acad::eOk)
			return AcDbObjectId::kNull;
		ownerId = pSrc->ownerId();
	}
	AcDbObjectIdArray ids;
	ids.append(srcId);
	AcDbIdMapping idMap;
	if (srcId.database()->deepCloneObjects(ids, ownerId, idMap) != Acad::eOk)
		return AcDbObjectId::kNull;
	AcDbIdPair pair(srcId, AcDbObjectId::kNull, true);
	if (!idMap.compute(pair) || !pair.isCloned())
		return AcDbObjectId::kNull;
	return pair.value();
}

//-----------------------------------------------------------------------------
void AFXAPI DDX_Distance(CDataExchange *pDX, int nIDC, double &value)
{
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (pDX->m_bSaveAndValidate) {
		CString s;
		CWnd::FromHandle(hWndCtrl)->GetWindowText(s);
		acdbDisToF(s, -1, &value);
	} else {
		ACHAR buf[64];
		acdbRToS(value, -1, -1, buf);
		AfxSetWindowText(hWndCtrl, buf);
	}
}

} // namespace

//-----------------------------------------------------------------------------
TESettings g_settings;

CString LoadMsg(UINT id)
{
	CString s;
	if (id && !s.LoadString(_hdllInstance, id, GetUserDefaultLangID()))
		s.LoadString(_hdllInstance, id, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	return s;
}

//-----------------------------------------------------------------------------
void TESettings::Load()
{
	fontName = kDefaultFontName;
	CRegKey key;
	if (key.Open(HKEY_CURRENT_USER, kRegKey, KEY_READ) != ERROR_SUCCESS)
		return;
	key.QueryDWORDValue(_T("ExitOnEnter"), exitOnEnter);
	key.QueryDWORDValue(_T("ExitOnCtrlEnter"), exitOnCtrlEnter);
	key.QueryDWORDValue(_T("FastYAlign"), fastYAlign);
	key.QueryDWORDValue(_T("FontSize"), fontSize);
	TCHAR buf[LF_FACESIZE] = {};
	ULONG chars = LF_FACESIZE;
	if (key.QueryStringValue(_T("FontName"), buf, &chars) == ERROR_SUCCESS && buf[0])
		fontName = buf;
	//- Guard against a hand-edited registry making the editor unusable
	if (fontSize < 4 || fontSize > 72)
		fontSize = 10;
}

//-----------------------------------------------------------------------------
void TESettings::Save() const
{
	CRegKey key;
	if (key.Create(HKEY_CURRENT_USER, kRegKey) != ERROR_SUCCESS)
		return;
	key.SetDWORDValue(_T("ExitOnEnter"), exitOnEnter);
	key.SetDWORDValue(_T("ExitOnCtrlEnter"), exitOnCtrlEnter);
	key.SetDWORDValue(_T("FastYAlign"), fastYAlign);
	key.SetDWORDValue(_T("FontSize"), fontSize);
	key.SetStringValue(_T("FontName"), fontName);
}

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC (CTEDialog, CAcUiDialog)

const UINT CTEDialog::s_findReplaceMsg = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CTEDialog, CAcUiDialog)
	ON_MESSAGE(WM_KICKIDLE, &CTEDialog::OnKickIdle)
	ON_MESSAGE(WM_TESELCHANGE, &CTEDialog::OnTESelChange)
	ON_WM_SETCURSOR()
	ON_MESSAGE(WM_CONTEXTMENU, &CTEDialog::OnContextMenu)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_EDIT, &CTEDialog::OnEnChangeEdit)
	ON_CBN_SELCHANGE(IDC_STYLE, &CTEDialog::OnEnChangeStyle)
	ON_EN_CHANGE(IDC_HEIGHT, &CTEDialog::OnEnChangeHeight)
	ON_EN_CHANGE(IDC_LINESPACE, &CTEDialog::OnEnChangeLineSpace)
	ON_EN_CHANGE(IDC_WIDTH, &CTEDialog::OnEnChangeWidth)
	ON_NOTIFY(EN_SELCHANGE, IDC_EDIT, &CTEDialog::OnSelChange)
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
	ON_COMMAND_RANGE(ID_UNDO, ID_SELECTALL, &CTEDialog::OnEditMenuItems)
	ON_REGISTERED_MESSAGE(s_findReplaceMsg, &CTEDialog::OnFindReplaceMessage)
	ON_COMMAND(ID_FONT, &CTEDialog::OnFont)
	ON_COMMAND(ID_OPTIONS_EXITONENTER, &CTEDialog::OnOptionExitOnEnter)
	ON_COMMAND(ID_OPTIONS_EXITONCTRLENTER, &CTEDialog::OnOptionExitOnCtrlEnter)
	ON_COMMAND(ID_OPTIONS_FASTYALIGN, &CTEDialog::OnOptionFastYAlign)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
CTEDialog::CTEDialog (CWnd *pParent, HINSTANCE hInstance)
	: CAcUiDialog (CTEDialog::IDD, pParent, hInstance)
{
	//- Lets AutoCAD remember the dialog size and position between sessions
	SetDialogName(LoadMsg(IDS_DLGNAME));
}

//-----------------------------------------------------------------------------
bool CTEDialog::LoadSelection(const ads_name sset)
{
	m_dtexts.setLogicalLength(0);
	m_textType = TETextType::Unknown;

	Adesk::Int32 len = 0;
	if (acedSSLength(sset, &len) != RTNORM || len == 0)
		return false;

	//- If at least one DTEXT is picked, edit all picked DTEXTs as lines and
	//- ignore MTEXT/DIMENSION; otherwise edit the first picked entity.
	AcDbObjectId firstId;
	for (Adesk::Int32 i = 0; i < len; i++) {
		ads_name ent;
		AcDbObjectId id;
		if (acedSSName(sset, i, ent) != RTNORM || acdbGetObjectId(id, ent) != Acad::eOk)
			continue;
		if (firstId.isNull())
			firstId = id;
		if (id.objectClass()->isDerivedFrom(AcDbText::desc()))
			m_dtexts.append(id);
	}

	if (!m_dtexts.isEmpty()) {
		m_textType = TETextType::DText;
		ReadDTextLineSpace();
		SortDText();
		//- Line 0 may have changed after sorting; the alignment reference
		//- point and the default height/width must come from the top line.
		ReadDTextLineSpace();
		return true;
	}

	AcDbEntityPointer pEnt(firstId, AcDb::kForRead);
	if (pEnt.openStatus() != Acad::eOk)
		return false;
	if (auto *pMText = AcDbMText::cast(pEnt.object())) {
		m_textType = TETextType::MText;
		m_entId = firstId;
		m_textHeight = pMText->textHeight();
		m_widthFactor = pMText->width();
		m_lineSpace = pMText->lineSpacingFactor();
	} else if (auto *pDim = AcDbDimension::cast(pEnt.object())) {
		m_textType = TETextType::Dimension;
		m_entId = firstId;
		m_textHeight = pDim->dimtxt();
	} else {
		acutPrintf(LoadMsg(IDS_ALLOWEDENT));
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
BOOL CTEDialog::OnInitDialog()
{
	CAcUiDialog::OnInitDialog();

	m_accel = ::LoadAccelerators(_hdllInstance, MAKEINTRESOURCE(IDR_ACCEL));
	if (HMENU hMenu = LoadLocalizedMenu(_hdllInstance, MAKEINTRESOURCE(MENU1)))
		::SetMenu(m_hWnd, hMenu);

	m_edit.SetTextMode(TM_PLAINTEXT | TM_MULTILEVELUNDO);
	ApplyEditorFont();
	m_edit.SetEventMask(ENM_CHANGE | ENM_SELCHANGE | ENM_KEYEVENTS);
	FillStyles();

	if (m_textType == TETextType::Dimension) {
		GetDlgItem(IDC_LINESPACE)->EnableWindow(FALSE);
		GetDlgItem(IDC_WIDTH)->EnableWindow(FALSE);
	}
	if (!IsDText()) {
		m_style.SelectString(-1, GetTextStyleName(0));
		GetDlgItem(IDC_ALIGNX)->EnableWindow(FALSE);
		GetDlgItem(IDC_ALIGNY)->EnableWindow(FALSE);
	}

	LoadTextFromDb();
	UpdateMenuItems();
	m_useEvents = true;
	m_edit.SetFocus();
	return FALSE; // focus was set explicitly
}

//-----------------------------------------------------------------------------
void CTEDialog::OnInitDialogFinish()
{
	//- Resize behaviour: the OK button sticks to the right edge, the status
	//- bar to the bottom, and the editor takes all remaining space.
	static DLGCTLINFO layout[] = {
		{ IDCANCEL, MOVEX, 100 },
		{ IDC_STATUSBAR, ELASTICX | MOVEY, 100 },
		{ IDC_EDIT, ELASTICXY, 100 },
	};
	SetControlProperty(layout, static_cast<DWORD>(std::size(layout)));
	UpdateStatusLayout();
}

//-----------------------------------------------------------------------------
void CTEDialog::DoDataExchange (CDataExchange *pDX)
{
	CAcUiDialog::DoDataExchange (pDX);
	DDX_Control(pDX, IDC_EDIT, m_edit);
	DDX_Control(pDX, IDC_STYLE, m_style);
	DDX_Control(pDX, IDC_STATUSBAR, m_statusBar);
	DDX_Distance(pDX, IDC_HEIGHT, m_textHeight);
	DDX_Distance(pDX, IDC_LINESPACE, m_lineSpace);
	DDX_Distance(pDX, IDC_WIDTH, m_widthFactor);
}

//-----------------------------------------------------------------------------
void CTEDialog::OnSize(UINT nType, int cx, int cy)
{
	CAcUiDialog::OnSize(nType, cx, cy);
	if (m_statusBar.GetSafeHwnd())
		UpdateStatusLayout();
}

//-----------------------------------------------------------------------------
void CTEDialog::UpdateStatusLayout()
{
	CRect rect;
	m_statusBar.GetClientRect(&rect);
	const int r = rect.right;
	int widths[4] = { r - 190, r - 140, r - 90, r };
	m_statusBar.SetParts(4, widths);
	m_statusBar.SetText(kTextTypeNames[static_cast<int>(m_textType)], 2, 0);
}

//-----------------------------------------------------------------------------
//- The hint in the status bar is updated on idle, not directly in
//- OnSetCursor, which fires on every mouse move.
LRESULT CTEDialog::OnKickIdle (WPARAM, LPARAM)
{
	if (!m_needStatusUpdate)
		return FALSE;
	m_needStatusUpdate = false;
	UINT txt = 0;
	switch (m_hoverCtrl ? ::GetDlgCtrlID(m_hoverCtrl) : 0) {
	case IDC_STYLE:     txt = IDS_TEXTSTYLE; break;
	case IDC_HEIGHT:    txt = IDS_TEXTHEIGHT; break;
	case IDC_LINESPACE: txt = IDS_LINESPACE; break;
	case IDC_WIDTH:     txt = m_textType == TETextType::MText ? IDS_MTEXTWIDTH : IDS_WIDTHFACTOR; break;
	case IDC_ALIGNX:    txt = IDS_ALIGNX; break;
	case IDC_ALIGNY:    txt = IDS_ALIGNY; break;
	}
	m_statusBar.SetText(LoadMsg(txt), 0, 0);
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CTEDialog::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message)
{
	if (m_hoverCtrl != pWnd->GetSafeHwnd()) {
		m_hoverCtrl = pWnd->GetSafeHwnd();
		m_needStatusUpdate = true;
	}
	return CAcUiDialog::OnSetCursor(pWnd, nHitTest, message);
}

//-----------------------------------------------------------------------------
void CTEDialog::ApplyEditorFont()
{
	CHARFORMAT2 chf {};
	chf.cbSize = sizeof(chf);
	chf.dwMask = CFM_SIZE | CFM_FACE;
	chf.yHeight = static_cast<LONG>(g_settings.fontSize) * 20; // points -> twips
	lstrcpyn(chf.szFaceName, g_settings.fontName, LF_FACESIZE);
	m_edit.SetDefaultCharFormat(chf);
}

//-----------------------------------------------------------------------------
void CTEDialog::OnUCase()
{
	CString s;
	EditorGetText(s, GT_SELECTION);
	s.MakeUpper();
	EditorSetText(s, ST_SELECTION);
	PushTextToDb();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnLCase()
{
	CString s;
	EditorGetText(s, GT_SELECTION);
	s.MakeLower();
	EditorSetText(s, ST_SELECTION);
	PushTextToDb();
}

//-----------------------------------------------------------------------------
CString CTEDialog::GetEditorLine(int i)
{
	const int len = m_edit.LineLength(m_edit.LineIndex(i));
	CString s;
	//- EM_GETLINE stores the buffer size in the first WORD of the buffer, so
	//- it needs room for at least 2 characters even for empty lines.
	const int bufLen = (std::max)(len, 2) + 1;
	const int got = m_edit.GetLine(i, s.GetBuffer(bufLen), bufLen);
	s.ReleaseBufferSetLength((std::min)(got, len));
	return s;
}

//-----------------------------------------------------------------------------
void CTEDialog::SetInsPoint(AcDbObjectId id, const AcGePoint3d &insPoint) const
{
	AcDbObjectPointer<AcDbText> pText(id, AcDb::kForWrite);
	if (pText.openStatus() != Acad::eOk)
		return;
	const AcDb::TextHorzMode hor = pText->horizontalMode();
	//- Left-baseline, Aligned and Fit texts are anchored by position(); all
	//- other justifications by alignmentPoint(). Aligned and Fit also need the
	//- second point moved by the same offset to keep the text length.
	if (pText->verticalMode() == AcDb::kTextBase &&
		(hor == AcDb::kTextLeft || hor == AcDb::kTextAlign || hor == AcDb::kTextFit)) {
		const AcGeVector3d offset = insPoint - pText->position();
		pText->setPosition(insPoint);
		if (hor == AcDb::kTextAlign || hor == AcDb::kTextFit)
			pText->setAlignmentPoint(pText->alignmentPoint() + offset);
	} else {
		pText->setAlignmentPoint(insPoint);
	}
	pText->adjustAlignment(pText->database());
}

//-----------------------------------------------------------------------------
AcGePoint3d CTEDialog::GetInsPoint(AcDbObjectId id) const
{
	AcDbObjectPointer<AcDbText> pText(id, AcDb::kForRead);
	if (pText.openStatus() != Acad::eOk)
		return AcGePoint3d::kOrigin;
	const AcDb::TextHorzMode hor = pText->horizontalMode();
	if (pText->verticalMode() == AcDb::kTextBase &&
		(hor == AcDb::kTextLeft || hor == AcDb::kTextAlign || hor == AcDb::kTextFit))
		return pText->position();
	return pText->alignmentPoint();
}

//-----------------------------------------------------------------------------
void CTEDialog::ReadDTextLineSpace()
{
	if (m_dtexts.isEmpty())
		return;
	const AcGePoint3d p1 = GetInsPoint(m_dtexts[0]);
	m_x1 = p1.x;
	m_y1 = p1.y;
	{
		AcDbObjectPointer<AcDbText> pText(m_dtexts[0], AcDb::kForRead);
		if (pText.openStatus() == Acad::eOk) {
			m_textHeight = pText->height();
			m_widthFactor = pText->widthFactor();
			const double angle = pText->rotation();
			m_sinA = sin(angle);
			m_cosA = cos(angle);
		}
	}
	if (m_dtexts.length() > 1) {
		//- Distance between the first two lines, measured perpendicular to
		//- the text direction.
		const AcGePoint3d p2 = GetInsPoint(m_dtexts[1]);
		m_lineSpace = (p1.y - p2.y) * m_cosA - (p1.x - p2.x) * m_sinA;
	} else {
		//- The old code left the line space uninitialised in this case.
		m_lineSpace = m_textHeight * kDefaultLineSpaceFactor;
	}
	m_dx = m_lineSpace * m_sinA;
	m_dy = m_lineSpace * m_cosA;
}

//-----------------------------------------------------------------------------
//- Sorts DTEXT lines top to bottom in the rotated frame of the first line.
//- Each insertion point is read once instead of on every comparison, which
//- the old bubble sort did (O(n^2) entity opens).
void CTEDialog::SortDText()
{
	const int n = m_dtexts.length();
	std::vector<double> key(n);
	for (int i = 0; i < n; i++) {
		const AcGePoint3d p = GetInsPoint(m_dtexts[i]);
		key[i] = p.y * m_cosA - p.x * m_sinA;
	}
	std::vector<int> order(n);
	std::iota(order.begin(), order.end(), 0);
	std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return key[a] > key[b]; });
	AcDbObjectIdArray sorted;
	for (int i : order)
		sorted.append(m_dtexts[i]);
	m_dtexts = sorted;
}

//-----------------------------------------------------------------------------
void CTEDialog::FillStyles()
{
	m_style.ResetContent();
	AcDbTextStyleTable *pTable = nullptr;
	if (acdbHostApplicationServices()->workingDatabase()->getTextStyleTable(pTable, AcDb::kForRead) != Acad::eOk)
		return;
	AcDbTextStyleTableIterator *pIter = nullptr;
	if (pTable->newIterator(pIter) == Acad::eOk) {
		for (; !pIter->done(); pIter->step()) {
			AcDbTextStyleTableRecord *pStyle = nullptr;
			if (pIter->getRecord(pStyle, AcDb::kForRead) != Acad::eOk)
				continue;
			//- Shape files loaded with LOAD create unnamed style records;
			//- they cannot be assigned to text.
			AcString name;
			if (pStyle->getName(name) == Acad::eOk && !name.isEmpty() && !pStyle->isShapeFile())
				m_style.AddString(name.kwszPtr());
			pStyle->close();
		}
		delete pIter;
	}
	pTable->close();
}

//-----------------------------------------------------------------------------
CString CTEDialog::GetTextStyleName(int index) const
{
	AcDbObjectId styleId;
	{
		AcDbEntityPointer pEnt(IsDText() ? m_dtexts[index] : m_entId, AcDb::kForRead);
		if (pEnt.openStatus() != Acad::eOk)
			return CString();
		if (auto *pText = AcDbText::cast(pEnt.object()))
			styleId = pText->textStyle();
		else if (auto *pMText = AcDbMText::cast(pEnt.object()))
			styleId = pMText->textStyle();
		else if (auto *pDim = AcDbDimension::cast(pEnt.object()))
			styleId = pDim->dimtxsty();
	}
	//- Copy the name before the record is closed; the old code returned a
	//- pointer into the already closed record.
	AcDbObjectPointer<AcDbTextStyleTableRecord> pStyle(styleId, AcDb::kForRead);
	AcString name;
	if (pStyle.openStatus() == Acad::eOk)
		pStyle->getName(name);
	return CString(name.kwszPtr());
}

//-----------------------------------------------------------------------------
void CTEDialog::SetTextStyle(int index, const CString &name)
{
	const AcDbObjectId styleId = FindTextStyle(name);
	if (styleId.isNull())
		return;
	AcDbEntityPointer pEnt(IsDText() ? m_dtexts[index] : m_entId, AcDb::kForWrite);
	if (pEnt.openStatus() != Acad::eOk)
		return;
	if (auto *pText = AcDbText::cast(pEnt.object()))
		pText->setTextStyle(styleId);
	else if (auto *pMText = AcDbMText::cast(pEnt.object()))
		pMText->setTextStyle(styleId);
	else if (auto *pDim = AcDbDimension::cast(pEnt.object()))
		pDim->setDimtxsty(styleId);
}

//-----------------------------------------------------------------------------
void CTEDialog::UpdateMenuItems()
{
	CMenu *pMenu = GetMenu();
	if (!pMenu)
		return;
	auto check = [pMenu](UINT id, DWORD on) {
		pMenu->CheckMenuItem(id, MF_BYCOMMAND | (on ? MF_CHECKED : MF_UNCHECKED));
	};
	check(ID_OPTIONS_EXITONENTER, g_settings.exitOnEnter);
	check(ID_OPTIONS_EXITONCTRLENTER, g_settings.exitOnCtrlEnter);
	check(ID_OPTIONS_FASTYALIGN, g_settings.fastYAlign);
	if (!IsDText()) {
		pMenu->EnableMenuItem(ID_ALIGNX, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(ID_ALIGNY, MF_BYCOMMAND | MF_GRAYED);
	}
}

//-----------------------------------------------------------------------------
void CTEDialog::EditorGetText(CString &s, DWORD flags)
{
	LONG len = 0;
	if (flags & GT_SELECTION) {
		long start = 0, end = 0;
		m_edit.GetSel(start, end);
		len = end - start;
	} else {
		GETTEXTLENGTHEX gtl { GTL_DEFAULT | GTL_NUMCHARS, kCodePageUtf16 };
		len = static_cast<LONG>(m_edit.SendMessage(EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&gtl), 0));
	}
	GETTEXTEX gt { static_cast<DWORD>((len + 1) * sizeof(WCHAR)), flags, kCodePageUtf16, nullptr, nullptr };
	m_edit.SendMessage(EM_GETTEXTEX, reinterpret_cast<WPARAM>(&gt), reinterpret_cast<LPARAM>(s.GetBuffer(len + 1)));
	s.ReleaseBuffer();
}

//-----------------------------------------------------------------------------
//- Sets editor text without firing EN_CHANGE, so programmatic updates do not
//- loop back into the database.
void CTEDialog::EditorSetText(const CString &s, DWORD flags)
{
	long selStart = 0, selEnd = 0;
	m_edit.GetSel(selStart, selEnd);
	SETTEXTEX st { flags | ST_KEEPUNDO, kCodePageUtf16 };
	const DWORD oldMask = m_edit.GetEventMask();
	m_edit.SetEventMask(oldMask & ~ENM_CHANGE);
	m_edit.SendMessage(EM_SETTEXTEX, reinterpret_cast<WPARAM>(&st), reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(s)));
	m_edit.SetSel(selStart, selEnd);
	m_edit.SetEventMask(oldMask);
}

//-----------------------------------------------------------------------------
//- Writes the editor contents back to the drawing. Called on every change so
//- the drawing previews the edit live.
void CTEDialog::PushTextToDb()
{
	AcAxDocLock lock(acdbCurDwg());

	if (!IsDText()) {
		CString s;
		EditorGetText(s, GT_DEFAULT);
		//- The rich edit control stores paragraphs as a bare \r
		s.Replace(_T("\r"), _T("\\P"));
		{
			AcDbEntityPointer pEnt(m_entId, AcDb::kForWrite);
			if (pEnt.openStatus() != Acad::eOk)
				return;
			if (auto *pMText = AcDbMText::cast(pEnt.object()))
				pMText->setContents(s);
			else if (auto *pDim = AcDbDimension::cast(pEnt.object()))
				pDim->setDimensionText(s);
		}
		acdbEvaluateFields(m_entId, AcDbField::kPreview);
		RefreshDisplay();
		return;
	}

	const int lineCount = m_edit.GetLineCount();
	const int dtextCount = m_dtexts.length();
	for (int i = 0; i < (std::min)(dtextCount, lineCount); i++) {
		{
			AcDbObjectPointer<AcDbText> pText(m_dtexts[i], AcDb::kForWrite);
			if (pText.openStatus() != Acad::eOk)
				continue;
			pText->setTextString(GetEditorLine(i));
		}
		acdbEvaluateFields(m_dtexts[i], AcDbField::kPreview);
	}

	if (dtextCount > lineCount) {
		//- Lines were deleted in the editor: erase the surplus DTEXTs
		for (int i = dtextCount - 1; i >= lineCount; i--) {
			AcDbEntityPointer pEnt(m_dtexts[i], AcDb::kForWrite);
			if (pEnt.openStatus() == Acad::eOk)
				pEnt->erase();
			m_dtexts.removeAt(i);
		}
	} else {
		//- Lines were added: copy the last DTEXT one line space further down
		for (int i = dtextCount; i < lineCount; i++) {
			const AcDbObjectId prevId = m_dtexts[i - 1];
			const AcDbObjectId newId = CloneDText(prevId);
			if (newId.isNull())
				break;
			{
				AcDbObjectPointer<AcDbText> pText(newId, AcDb::kForWrite);
				if (pText.openStatus() == Acad::eOk) {
					//- A field copied from the previous line would overwrite
					//- the text typed for this one on the next evaluation.
					if (pText->hasFields())
						pText->removeField(_T("TEXT"));
					pText->setTextString(GetEditorLine(i));
				}
			}
			AcGePoint3d pos = GetInsPoint(prevId);
			pos.x += m_dx;
			pos.y -= m_dy;
			SetInsPoint(newId, pos);
			m_dtexts.append(newId);
		}
	}
	RefreshDisplay();
}

//-----------------------------------------------------------------------------
void CTEDialog::LoadTextFromDb()
{
	CString s;
	if (IsDText()) {
		for (int i = 0; i < m_dtexts.length(); i++) {
			if (i > 0)
				s += _T("\r\n");
			s += ReadEntityText(m_dtexts[i]);
		}
	} else {
		s = ReadEntityText(m_entId);
		s.Replace(_T("\\P"), _T("\r\n"));
	}
	EditorSetText(s, ST_DEFAULT);
	//- Loading the text is not a user edit; start with an empty undo stack.
	m_edit.EmptyUndoBuffer();
}

//-----------------------------------------------------------------------------
//- Shows height, width factor and style of the DTEXT lines touched by the
//- editor selection; a field is left empty when the lines differ.
void CTEDialog::ShowTextParams()
{
	if (!IsDText())
		return;
	const long last = (std::min<long>)(m_endLn, m_dtexts.length() - 1);
	if (m_startLn > last)
		return;

	m_useEvents = false;
	bool heightEqual = true, wfEqual = true, styleEqual = true;
	double height = 0.0, wf = 0.0;
	AcDbObjectId styleId;
	for (long i = m_startLn; i <= last; i++) {
		AcDbObjectPointer<AcDbText> pText(m_dtexts[i], AcDb::kForRead);
		if (pText.openStatus() != Acad::eOk)
			continue;
		if (i == m_startLn) {
			height = pText->height();
			wf = pText->widthFactor();
			styleId = pText->textStyle();
			continue;
		}
		heightEqual = heightEqual && pText->height() == height;
		wfEqual = wfEqual && pText->widthFactor() == wf;
		styleEqual = styleEqual && pText->textStyle() == styleId;
	}
	if (heightEqual)
		SetDlgItemDouble(IDC_HEIGHT, height);
	else
		SetDlgItemText(IDC_HEIGHT, _T(""));
	if (wfEqual)
		SetDlgItemDouble(IDC_WIDTH, wf);
	else
		SetDlgItemText(IDC_WIDTH, _T(""));
	if (styleEqual)
		m_style.SelectString(-1, GetTextStyleName(m_startLn));
	else
		m_style.SetCurSel(-1);
	m_useEvents = true;
}

//-----------------------------------------------------------------------------
void CTEDialog::ReadSelection()
{
	m_edit.GetSel(m_selStart, m_selEnd);
	m_endLn = m_edit.LineFromChar(m_selEnd);
	m_startLn = m_edit.LineFromChar(m_selStart);
	m_col = m_selEnd - m_edit.LineIndex(m_endLn);
}

//-----------------------------------------------------------------------------
void CTEDialog::ShowCaretPos()
{
	ReadSelection();
	CString s;
	s.Format(LoadMsg(IDS_CURSORPOS), m_endLn + 1, m_col + 1);
	m_statusBar.SetText(s, 3, 0);
}

//-----------------------------------------------------------------------------
bool CTEDialog::GetDlgItemDouble(int item, double &value) const
{
	CString s;
	GetDlgItemText(item, s);
	double v = 0.0;
	if (s.IsEmpty() || acdbDisToF(s, -1, &v) != RTNORM)
		return false;
	value = v;
	return true;
}

//-----------------------------------------------------------------------------
void CTEDialog::SetDlgItemDouble(int item, double value)
{
	ACHAR buf[64];
	acdbRToS(value, -1, -1, buf);
	SetDlgItemText(item, buf);
}

//-----------------------------------------------------------------------------
void CTEDialog::OnClose()
{
	EndDialog(IDOK);
}

//-----------------------------------------------------------------------------
void CTEDialog::OnAlignX()
{
	if (!IsDText())
		return;
	AcAxDocLock lock(acdbCurDwg());
	//- Work in the rotated frame of the first line: keep each line's offset
	//- along the line normal and give it the first line's coordinate along
	//- the text direction.
	const double x1 = m_x1 * m_cosA + m_y1 * m_sinA;
	for (int i = 1; i < m_dtexts.length(); i++) {
		AcGePoint3d p = GetInsPoint(m_dtexts[i]);
		const double y2 = p.x * m_sinA - p.y * m_cosA;
		p.x = x1 * m_cosA + y2 * m_sinA;
		p.y = x1 * m_sinA - y2 * m_cosA;
		SetInsPoint(m_dtexts[i], p);
	}
	RefreshDisplay();
	ShowCaretPos();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnAlignY()
{
	if (!IsDText())
		return;
	AcAxDocLock lock(acdbCurDwg());
	//- Keep each line's coordinate along the text direction and stack the
	//- lines one line space apart below the first one.
	const double y1 = m_x1 * m_sinA - m_y1 * m_cosA;
	for (int i = 1; i < m_dtexts.length(); i++) {
		AcGePoint3d p = GetInsPoint(m_dtexts[i]);
		const double x2 = p.x * m_cosA + p.y * m_sinA;
		p.x = x2 * m_cosA + (y1 + m_lineSpace * i) * m_sinA;
		p.y = x2 * m_sinA - (y1 + m_lineSpace * i) * m_cosA;
		SetInsPoint(m_dtexts[i], p);
	}
	RefreshDisplay();
	ShowCaretPos();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeEdit()
{
	if (m_useEvents)
		PushTextToDb();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeStyle()
{
	if (!m_useEvents)
		return;
	AcAxDocLock lock(acdbCurDwg());
	CString name;
	m_style.GetWindowText(name);
	ReadSelection();
	if (IsDText()) {
		for (long i = m_startLn; i <= m_endLn && i < m_dtexts.length(); i++)
			SetTextStyle(i, name);
	} else {
		SetTextStyle(0, name);
	}
	RefreshDisplay();
	ShowCaretPos();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeHeight()
{
	double height = 0.0;
	if (!m_useEvents || !GetDlgItemDouble(IDC_HEIGHT, height) || height <= 0.0)
		return;
	AcAxDocLock lock(acdbCurDwg());
	ReadSelection();
	if (IsDText()) {
		for (long i = m_startLn; i <= m_endLn && i < m_dtexts.length(); i++) {
			AcDbObjectPointer<AcDbText> pText(m_dtexts[i], AcDb::kForWrite);
			if (pText.openStatus() == Acad::eOk)
				pText->setHeight(height);
		}
	} else {
		AcDbEntityPointer pEnt(m_entId, AcDb::kForWrite);
		if (pEnt.openStatus() == Acad::eOk) {
			if (auto *pMText = AcDbMText::cast(pEnt.object()))
				pMText->setTextHeight(height);
			else if (auto *pDim = AcDbDimension::cast(pEnt.object()))
				pDim->setDimtxt(height);
		}
	}
	RefreshDisplay();
	ShowCaretPos();
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeLineSpace()
{
	double lineSpace = 0.0;
	if (!m_useEvents || !GetDlgItemDouble(IDC_LINESPACE, lineSpace))
		return;
	m_lineSpace = lineSpace;
	if (IsDText()) {
		m_dx = m_lineSpace * m_sinA;
		m_dy = m_lineSpace * m_cosA;
		if (g_settings.fastYAlign)
			OnAlignY();
	} else if (m_textType == TETextType::MText) {
		//- AutoCAD rejects MTEXT line spacing factors outside 0.25..4.0; the
		//- error status is enough to ignore such intermediate input.
		AcAxDocLock lock(acdbCurDwg());
		Acad::ErrorStatus es = Acad::eInvalidInput;
		{
			AcDbObjectPointer<AcDbMText> pMText(m_entId, AcDb::kForWrite);
			if (pMText.openStatus() == Acad::eOk)
				es = pMText->setLineSpacingFactor(m_lineSpace);
		}
		if (es == Acad::eOk)
			RefreshDisplay();
	}
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEnChangeWidth()
{
	double w = 0.0;
	if (!m_useEvents || !GetDlgItemDouble(IDC_WIDTH, w) || w <= 0.0)
		return;
	AcAxDocLock lock(acdbCurDwg());
	ReadSelection();
	if (IsDText()) {
		for (long i = m_startLn; i <= m_endLn && i < m_dtexts.length(); i++) {
			AcDbObjectPointer<AcDbText> pText(m_dtexts[i], AcDb::kForWrite);
			if (pText.openStatus() == Acad::eOk)
				pText->setWidthFactor(w);
		}
	} else if (m_textType == TETextType::MText) {
		AcDbObjectPointer<AcDbMText> pMText(m_entId, AcDb::kForWrite);
		if (pMText.openStatus() == Acad::eOk)
			pMText->setWidth(w);
	}
	RefreshDisplay();
	ShowCaretPos();
}

//-----------------------------------------------------------------------------
//- EN_SELCHANGE arrives before the control finished updating its selection,
//- so the real work is posted to run afterwards.
void CTEDialog::OnSelChange(NMHDR *, LRESULT *result)
{
	PostMessage(WM_TESELCHANGE);
	*result = 0;
}

//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnTESelChange(WPARAM, LPARAM)
{
	ShowCaretPos();
	ShowTextParams();
	return TRUE;
}

//-----------------------------------------------------------------------------
void CTEDialog::ShowFindDialog(bool findOnly)
{
	//- Only one find/replace window at a time; the old code opened a new one
	//- (and leaked the previous) on every Ctrl+F. CFindReplaceDialog deletes
	//- itself when its window is destroyed.
	if (m_pFindDlg) {
		m_pFindDlg->DestroyWindow();
		m_pFindDlg = nullptr;
	}
	//- Prefill with the selected text, unless it spans several lines
	CString selected;
	EditorGetText(selected, GT_SELECTION);
	if (selected.Find(_T('\r')) >= 0)
		selected.Empty();
	m_pFindDlg = new CFindReplaceDialog();
	m_pFindDlg->Create(findOnly ? TRUE : FALSE, selected, nullptr, FR_DOWN, this);
	m_pFindDlg->ShowWindow(SW_SHOW);
}

void CTEDialog::OnFind() { ShowFindDialog(true); }
void CTEDialog::OnReplace() { ShowFindDialog(false); }

//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnFindReplaceMessage(WPARAM, LPARAM lParam)
{
	CFindReplaceDialog *pDlg = CFindReplaceDialog::GetNotifier(lParam);
	if (!pDlg)
		return 0;
	if (pDlg->IsTerminating()) {
		m_pFindDlg = nullptr;
		return 0;
	}
	const FINDREPLACE &fr = pDlg->m_fr;
	const HWND owner = pDlg->GetSafeHwnd();

	if (pDlg->FindNext()) {
		if (!FindNext(fr))
			::MessageBox(owner, LoadMsg(IDS_SEARCHFIN), LoadMsg(IDS_INFO), MB_OK);
	} else if (pDlg->ReplaceCurrent()) {
		//- Replace the current match (if the selection is one), then move to
		//- the next one, like the standard Windows replace dialog. The old
		//- code replaced the *next* match without showing it first.
		CString sel;
		EditorGetText(sel, GT_SELECTION);
		const bool matchCase = (fr.Flags & FR_MATCHCASE) != 0;
		if (!sel.IsEmpty() && (matchCase ? sel.Compare(fr.lpstrFindWhat) : sel.CompareNoCase(fr.lpstrFindWhat)) == 0)
			m_edit.ReplaceSel(fr.lpstrReplaceWith, TRUE);
		if (!FindNext(fr))
			::MessageBox(owner, LoadMsg(IDS_SEARCHFIN), LoadMsg(IDS_INFO), MB_OK);
	} else if (pDlg->ReplaceAll()) {
		//- Replace All covers the whole text, not just the part after the caret
		m_edit.SetSel(0, 0);
		FINDREPLACE down = fr;
		down.Flags |= FR_DOWN;
		int count = 0;
		while (FindNext(down)) {
			m_edit.ReplaceSel(fr.lpstrReplaceWith, TRUE);
			count++;
		}
		CString msg;
		if (count == 0)
			msg = LoadMsg(IDS_SEARCHFIN);
		else
			msg.FormatMessage(LoadMsg(IDS_REPLACED), count);
		::MessageBox(owner, msg, LoadMsg(IDS_INFO), MB_OK);
	}
	return 0;
}

//-----------------------------------------------------------------------------
bool CTEDialog::FindNext(const FINDREPLACE &fr)
{
	ReadSelection();
	FINDTEXTEX ft {};
	ft.lpstrText = fr.lpstrFindWhat;
	//- Searching up needs cpMax = 0; -1 means "to the end" only when searching down.
	if (fr.Flags & FR_DOWN) {
		ft.chrg.cpMin = m_selEnd;
		ft.chrg.cpMax = -1;
	} else {
		ft.chrg.cpMin = m_selStart;
		ft.chrg.cpMax = 0;
	}
	const WPARAM flags = fr.Flags & (FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD);
	if (m_edit.SendMessage(EM_FINDTEXTEX, flags, reinterpret_cast<LPARAM>(&ft)) == -1)
		return false;
	m_edit.SetSel(ft.chrgText);
	return true;
}

//-----------------------------------------------------------------------------
LRESULT CTEDialog::OnContextMenu(WPARAM, LPARAM lParam)
{
	CMenu *pMenu = GetMenu();
	CMenu *pPopup = pMenu ? pMenu->GetSubMenu(0) : nullptr;
	if (!pPopup)
		return 0;
	//- Signed: coordinates can be negative on multi-monitor setups
	CPoint pt(static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)));
	//- Shift+F10 and the menu key send (-1, -1): open the menu at the caret
	if (lParam == -1) {
		pt = m_edit.GetCaretPos();
		m_edit.ClientToScreen(&pt);
	}
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
	m_edit.SetFocus();
	return 0;
}

//-----------------------------------------------------------------------------
void CTEDialog::OnEditMenuItems(UINT nID)
{
	switch (nID) {
	case ID_SELECTALL: m_edit.SetSel(0, -1); break;
	case ID_CUT:       m_edit.Cut(); break;
	case ID_COPY:      m_edit.Copy(); break;
	case ID_PASTE:     m_edit.Paste(); break;
	case ID_UNDO:      m_edit.Undo(); break;
	case ID_REDO:      m_edit.Redo(); break;
	}
}

//-----------------------------------------------------------------------------
void CTEDialog::OnAbout()
{
	::DialogBox(_hdllInstance, MAKEINTRESOURCE(ABOUTDLG), m_hWnd, &AboutDlgProc);
}

//-----------------------------------------------------------------------------
void CTEDialog::OnFont()
{
	LOGFONT lf {};
	lstrcpyn(lf.lfFaceName, g_settings.fontName, LF_FACESIZE);
	lf.lfCharSet = DEFAULT_CHARSET;
	{
		CClientDC dc(this);
		lf.lfHeight = -MulDiv(static_cast<int>(g_settings.fontSize), dc.GetDeviceCaps(LOGPIXELSY), 72);
	}
	CFontDialog dlg(&lf, CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT, nullptr, this);
	if (dlg.DoModal() != IDOK)
		return;
	g_settings.fontSize = dlg.GetSize() / 10;
	g_settings.fontName = dlg.GetFaceName();
	g_settings.Save();
	ApplyEditorFont();
}

//-----------------------------------------------------------------------------
void CTEDialog::ToggleOption(DWORD &option)
{
	option = !option;
	UpdateMenuItems();
	g_settings.Save();
}

void CTEDialog::OnOptionExitOnEnter() { ToggleOption(g_settings.exitOnEnter); }
void CTEDialog::OnOptionExitOnCtrlEnter() { ToggleOption(g_settings.exitOnCtrlEnter); }
void CTEDialog::OnOptionFastYAlign() { ToggleOption(g_settings.fastYAlign); }

//-----------------------------------------------------------------------------
BOOL CTEDialog::PreTranslateMessage(MSG *pMsg)
{
	if (m_accel && ::TranslateAccelerator(m_hWnd, m_accel, pMsg))
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		const int id = ::GetDlgCtrlID(pMsg->hwnd);
		//- Enter in a numeric field confirms it and returns to the text
		if (id == IDC_HEIGHT || id == IDC_LINESPACE || id == IDC_WIDTH) {
			m_edit.SetFocus();
			return TRUE;
		}
		if (pMsg->hwnd == m_edit.GetSafeHwnd()) {
			const bool ctrl = ::GetKeyState(VK_CONTROL) < 0;
			if (g_settings.exitOnEnter || (g_settings.exitOnCtrlEnter && ctrl)) {
				OnClose();
				return TRUE;
			}
		}
	}
	return CAcUiDialog::PreTranslateMessage(pMsg);
}
