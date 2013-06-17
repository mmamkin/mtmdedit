MtmdEdit v3.3 beta for AutoCAD 2007-2010
Copyright (©) 2003-2010 Mansur MAMKIN 


MtmdEdit is a small ARX application for easy editing 
of DTEXT, MTEXT objects and dimension text.

History
------------------
v3.3 - 2013/06/17
+ Sources uploaded to code.google.com/p/mtmdedit/
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

Main features
------------------
1. You can select several DTEXT objects and then edit them 
   at the same time (it looks like AutoCAD MTEXT editor)

2. Change line spacing between DTEXT objects

3. Align DTEXT objects

4. Change style, height (in case of DTEXT you can change 
separately each DTEXT object or selected lines)

5. Make selection UPPERCASE(Ctrl+U) or lowercase(Ctrl+L)

6. Unlike standard editor, you can edit control codes and field codes

7. Double click event supported (press Ctrl+K in MtmdEdit window
   to switch double click activation)

Terms of Use
------------------
Copyright (©) 2003-2010 Mansur MAMKIN

MtmdEdit is freeware.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications.
You may distribute this program freely in its original unaltered
form. However, you may not distribute this program with or as
part of a commercial release without our express written consent.

Using MtmdEdit
------------------
Enter command "TE" or simply double click text object.

Note: to switch (on/off) double click activation, 
      press Ctrl+K in MtmdEdit. 

If even one DTEXT object is in selection, MtmdEdit will start 
in DTEXT mode.

To align DTEXT objects in X or Y direction click "X" or "Y"
button correspondingly

To change linespace (DTEXT mode) change value of corresponding
edit box and click "Y" button

To exit press ESC or click "Ok" (See also options "Exit on Enter", "Exit on Ctrl-Enter")

Reporting Problems
------------------
Please send all suggestions and bug reports to:
	Mansur MAMKIN <mmamkin@mail.ru>

Updates
------------------
For the latest version see http://mtmlab.ru/download 
