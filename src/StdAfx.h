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
//- StdAfx.h : precompiled header (Windows, MFC and ObjectARX headers)
//-----------------------------------------------------------------------------
#pragma once

#define _AFX_NO_DB_SUPPORT
#define _AFX_NO_DAO_SUPPORT

//- AutoCAD is a release build, so its MFC/CRT must not be mixed with the debug
//- ones. Hide _DEBUG while the MFC/ATL/STL headers are pulled in, otherwise
//- they force the debug CRT through #pragma comment(lib, ...).
#if defined(_DEBUG) && !defined(AC_FULL_DEBUG)
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#endif

#pragma pack (push, 8)
#pragma warning(disable: 4996)

#define STRICT
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

//- AutoCAD 2027 runs only on Windows 10/11, and current MFC refuses the old
//- Windows XP values this project used to define.
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

//- ObjectARX and OMF headers need this
#include <map>

#include <afxwin.h>
#include <afxext.h>
#include <afxole.h>
#include <afxodlgs.h>
#include <afxdisp.h>
#include <afxdtctl.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxpriv.h>          //- WM_KICKIDLE
#include <atlbase.h>
#include <richedit.h>

#include "arxHeaders.h"
#include "axlock.h"
#include "dbobjptr.h"

#include "resource.h"

#pragma pack (pop)

#ifdef _DEBUG_WAS_DEFINED
#define _DEBUG
#undef _DEBUG_WAS_DEFINED
#endif
