MtmdEdit is a small ARX application for easy editing 
of DTEXT, MTEXT objects and dimension text.

###History

v4.0 - 2026/11/14

* Ported to AutoCAD 2027 (ObjectARX 2027, Visual Studio 2026 / v145, C++20, x64)
* Dropped build configurations for AutoCAD 2012-2021 and BricsCAD (their toolsets are no longer available)
* Added MtmdEdit-Setup.exe: installs the VC++ runtime prerequisite and the autoloader bundle, uninstalls from Windows settings, silent mode /q
* No more AutoCAD crash on a failed entity open; errors are skipped instead
* New DTEXT lines are copied with deepCloneObjects, so fields and annotative scales are no longer shared with the source line
* Fixed: line space was undefined when a single DTEXT was edited
* Fixed: style name read from an already closed record, memory freed with the wrong operator
* Fixed: Find/Replace opened a new window on every Ctrl+F; "Replace" now replaces the current match; searching up works
* Fixed: About dialog message handling fall-through; context menu from the keyboard
* Editor: monospaced default font (Consolas), multi-level undo/redo, find box prefilled with the selection

v3.5 - 2021/01/03

* Added support for AutoCAD 2018, 2019, 2020
* Dropped support for doubleclick editing

v3.3 - 2013/06/17

+ Sources uploaded to <http://code.google.com/p/mtmdedit/>
+ Solution structure refactored, now there is one project for all platforms and CAD-versions

v3.3 beta - 2010/09/10

* Bug fixes
+ Russian interface added (it's used automatically on russian Windows)
+ Now you can click on link in "About" window

v3.2 - 2010/08/31

* Fixed bug with AcdbField handling
* Double click handler rewritten
+ Options added: Exit on Enter, Ctrl-Enter, Fast Y Align 
(text is aligned as you change line space)  

v3.1 - 2010/08/27

MtmdEdit was rewritten to use MFC framework

###Main features

- You can select several DTEXT objects and then edit them at the same time (it looks like AutoCAD MTEXT editor)
- Change line spacing between DTEXT objects
- Align DTEXT objects
- Change style, height (in case of DTEXT you can change separately each DTEXT object or selected lines)
- Make selection UPPERCASE(Ctrl+U) or lowercase(Ctrl+L)
- Unlike standard editor, you can edit control codes and field codes

###Terms of Use

MtmdEdit is freeware.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications.
You may distribute this program freely in its original unaltered
form. However, you may not distribute this program with or as
part of a commercial release without our express written consent.

###Installing (AutoCAD 2027)

Run `MtmdEdit-Setup.exe`. It:

- checks that AutoCAD 2027 is installed (MtmdEdit works in no other release),
- installs the Microsoft Visual C++ 2015-2026 runtime (x64, incl. MFC) when
  the computer has an older one; the installer carries it, nothing is downloaded,
- installs the autoloader bundle to `%APPDATA%\Autodesk\ApplicationPlugins\MtmdEdit.bundle`
  (per user, no administrator rights except for the runtime),
- adds "MtmdEdit for AutoCAD 2027" to Windows "Installed apps" for uninstalling.

Restart AutoCAD 2027 afterwards; the command is TE. `MtmdEdit-Setup.exe /q`
installs silently (for deployment to many computers).

###Building

Requirements (developer machine only, not needed by users):

- Visual Studio 2026 (or 2022 17.14) with "Desktop development with C++" and the
  "C++ MFC for latest build tools (x86 & x64)" component
- ObjectARX 2027 SDK from <https://aps.autodesk.com/developer/overview/autocad-objectarx-sdk>

Steps:

    set "OBJECTARX2027=C:\ObjectARX 2027"
    build.cmd

`build.cmd` produces `Out\MtmdEdit2027.arx` and the installer `Out\MtmdEdit-Setup.exe`.
The installer stub is compiled with the C# compiler that ships with Windows
(.NET Framework 4.x), so no installer tool is needed. To test without
installing, load `Out\MtmdEdit2027.arx` with APPLOAD.

The .arx works only in AutoCAD 2027: AutoCAD 2027 is binary compatible with
itself only, so every AutoCAD release needs its own build.

###Using MtmdEdit

Enter command "TE".

If even one DTEXT object is in selection, MtmdEdit will start in DTEXT mode.

To align DTEXT objects in X or Y direction click "X" or "Y" button correspondingly

To change linespace (DTEXT mode) change value of corresponding edit box and click "Y" button

To exit press ESC or click "Ok" (See also options "Exit on Enter", "Exit on Ctrl-Enter")

###Reporting Problems

Please send all suggestions and bug reports to:
	Mansur MAMKIN <mmamkin@mail.ru>

###Updates

For the latest version see <http://mtmlab.ru/download>
