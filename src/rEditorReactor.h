/////////////////////////////////////////////
// AcEditorReactor reactors.

#if !defined(ARX__REDITORREACTOR_H__20011221_165545)
#define ARX__REDITORREACTOR_H__20011221_165545

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dbidmap.h"
#include "aced.h"
#include "AcDblClkEdit.h"

/*class MyAcDbDoubleClickEdit: public AcDbDoubleClickEdit
{
public:
	void startEdit(AcDbEntity *pEnt,AcGePoint3d pt);
	void finishEdit(void);
};*/ 

class MTMeditorreactor : public AcEditorReactor
{
public:

	// Constructor / Destructor
	MTMeditorreactor(const bool autoInitAndRelease = true);
	virtual ~MTMeditorreactor();

	static MTMeditorreactor * reac;
	//{{AFX_ARX_METHODS(MTMeditorreactor)
	virtual void commandWillStart(const ACHAR* cmdStr);
	virtual void beginDoubleClick(const AcGePoint3d& clickPoint);
	//}}AFX_ARX_METHODS

private:
	// Auto initialization and release flag.
	bool m_autoInitAndRelease;
};



#endif // !defined(ARX__REDITORREACTOR_H__20011221_165545)