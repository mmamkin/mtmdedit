//-----------------------------------------------------------------------------
//----- TEDialog.h : Declaration of the CTEDialog
//-----------------------------------------------------------------------------
#pragma once

#include "acui.h"

//- Posted from EN_SELCHANGE, see CTEDialog::OnSelChange
#define WM_TESELCHANGE (WM_USER + 10)

enum class TETextType { Unknown, DText, MText, Dimension };

//-----------------------------------------------------------------------------
//- Loads a string resource in the user's UI language, falling back to English
//- (the .rc only carries Russian and English string tables).
CString LoadMsg(UINT id);

//-----------------------------------------------------------------------------
//- User options persisted in HKCU\Software\Mtmlab\MtmdEdit.
struct TESettings {
	DWORD exitOnEnter = 0;
	DWORD exitOnCtrlEnter = 0;
	DWORD fastYAlign = 0;
	CString fontName;
	DWORD fontSize = 10;

	void Load();
	void Save() const;
};
extern TESettings g_settings;

//-----------------------------------------------------------------------------
class CTEDialog : public CAcUiDialog {
	DECLARE_DYNAMIC (CTEDialog)
public:
	enum { IDD = EDITDLG };

	explicit CTEDialog (CWnd *pParent = nullptr, HINSTANCE hInstance = nullptr);

	//- Reads the picked entities. Returns false when nothing editable was picked.
	bool LoadSelection(const ads_name sset);

protected:
	void DoDataExchange (CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	//- CAdUiBaseDialog hook run at the end of OnInitDialog; the resize layout
	//- has to be registered there, after the base dialog captured control sizes.
	virtual void OnInitDialogFinish();
	BOOL PreTranslateMessage(MSG*) override;

	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg LRESULT OnTESelChange (WPARAM, LPARAM);
	afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT);
	afx_msg LRESULT OnContextMenu(WPARAM, LPARAM);
	afx_msg void OnClose();
	afx_msg void OnEnChangeEdit();
	afx_msg void OnEnChangeStyle();
	afx_msg void OnEnChangeHeight();
	afx_msg void OnEnChangeLineSpace();
	afx_msg void OnEnChangeWidth();
	afx_msg void OnSelChange(NMHDR *pSel, LRESULT* result);
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
	afx_msg void OnOptionExitOnEnter();
	afx_msg void OnOptionExitOnCtrlEnter();
	afx_msg void OnOptionFastYAlign();
	afx_msg LRESULT OnFindReplaceMessage(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	static const UINT s_findReplaceMsg;

	//- Entities being edited: one MTEXT/DIMENSION, or a list of DTEXT lines
	//- sorted top to bottom along the text direction.
	TETextType m_textType = TETextType::Unknown;
	AcDbObjectId m_entId;
	AcDbObjectIdArray m_dtexts;

	//- Values shown in the toolbar edits
	double m_lineSpace = 0.0, m_textHeight = 0.0, m_widthFactor = 0.0;
	//- Rotation of the first DTEXT and its insertion point; all DTEXT
	//- alignment maths runs in that rotated frame.
	double m_cosA = 1.0, m_sinA = 0.0, m_x1 = 0.0, m_y1 = 0.0;
	//- Offset between successive DTEXT lines in WCS
	double m_dx = 0.0, m_dy = 0.0;

	long m_selStart = 0, m_selEnd = 0, m_startLn = 0, m_endLn = 0, m_col = 0;

	//- Suppresses EN_CHANGE handlers while the dialog fills the controls itself
	bool m_useEvents = false;
	bool m_needStatusUpdate = false;
	HACCEL m_accel = nullptr;
	HWND m_hoverCtrl = nullptr;
	CFindReplaceDialog *m_pFindDlg = nullptr;

	CRichEditCtrl m_edit;
	CComboBox m_style;
	CStatusBarCtrl m_statusBar;

	bool IsDText() const { return m_textType == TETextType::DText; }

	// Selection / geometry helpers
	void ReadDTextLineSpace();
	void SortDText();
	AcGePoint3d GetInsPoint(AcDbObjectId id) const;
	void SetInsPoint(AcDbObjectId id, const AcGePoint3d &insPoint) const;

	// Text style helpers
	void FillStyles();
	CString GetTextStyleName(int index) const;
	void SetTextStyle(int index, const CString &name);

	// Database <-> editor transfer
	void PushTextToDb();
	void LoadTextFromDb();
	void EditorGetText(CString &s, DWORD flags);
	void EditorSetText(const CString &s, DWORD flags);
	CString GetEditorLine(int i);

	// UI helpers
	bool GetDlgItemDouble(int item, double &value) const;
	void SetDlgItemDouble(int item, double value);
	void ApplyEditorFont();
	void UpdateMenuItems();
	void UpdateStatusLayout();
	void ReadSelection();
	void ShowCaretPos();
	void ShowTextParams();
	void ToggleOption(DWORD &option);
	void ShowFindDialog(bool findOnly);
	bool FindNext(const FINDREPLACE &fr);
};
