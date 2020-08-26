/*_________
 /         \ hello.c v3.6.3 [May 11, 2020] zlib licence
 |tiny file| Hello World file created [November 9, 2014]
 | dialogs | Copyright (c) 2014 - 2020 Guillaume Vareille http://ysengrin.com
 \____  ___/ http://tinyfiledialogs.sourceforge.net
      \|     git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd
              ____________________________________________
             |                                            |
             |   email: tinyfiledialogs at ysengrin.com   |
             |____________________________________________|
  _________________________________________________________________________________
 |                                                                                 |
 | the windows only wchar_t UTF-16 prototypes are at the bottom of the header file |
 |_________________________________________________________________________________|
  _________________________________________________________
 |                                                         |
 | on windows: - since v3.6 char is UTF-8 by default       |
 |             - if you want MBCS set tinyfd_winUtf8 to 0  |
 |             - functions like fopen expect MBCS          |
 |_________________________________________________________|

If you like tinyfiledialogs, please upvote my stackoverflow answer
https://stackoverflow.com/a/47651444

tiny file dialogs (cross-platform C C++)
InputBox PasswordBox MessageBox ColorPicker
OpenFileDialog SaveFileDialog SelectFolderDialog
Native dialog library for WINDOWS MAC OSX GTK+ QT CONSOLE & more
SSH supported via automatic switch to console mode or X11 forwarding

one C file + a header (add them to your C or C++ project) with 8 functions:
- beep
- notify popup (tray)
- message & question
- input & password
- save file
- open file(s)
- select folder
- color picker

complements OpenGL Vulkan GLFW GLUT GLUI VTK SFML TGUI
SDL Ogre Unity3d ION OpenCV CEGUI MathGL GLM CPW GLOW
Open3D IMGUI MyGUI GLT NGL STB & GUI less programs

NO INIT
NO MAIN LOOP
NO LINKING
NO INCLUDE

the dialogs can be forced into console mode

Windows (XP to 10) ASCII MBCS UTF-8 UTF-16
- native code & vbs create the graphic dialogs
- enhanced console mode can use dialog.exe from
http://andrear.altervista.org/home/cdialog.php
- basic console input

Unix (command line calls) ASCII UTF-8
- applescript, kdialog, zenity
- python (2 or 3) + tkinter + python-dbus (optional)
- dialog (opens a console if needed)
- basic console input
the same executable can run across desktops & distributions

C89/C18 & C++98/C++20 compliant: tested with C & C++ compilers
VisualStudio MinGW-gcc GCC Clang TinyCC OpenWatcom-v2 BorlandC SunCC ZapCC
on Windows Mac Linux Bsd Solaris Minix Raspbian
using Gnome Kde Enlightenment Mate Cinnamon Budgie Unity Lxde Lxqt Xfce
WindowMaker IceWm Cde Jds OpenBox Awesome Jwm Xdm Cwm

Bindings for LUA and C# dll, Haskell, Fortran
Included in LWJGL(java), Rust, Allegrobasic

- License -

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


/*
- Here is the Hello World:
    if a console is missing, it will use graphic dialogs
    if a graphical display is absent, it will use console dialogs
		(on windows the input box may take some time to open the first time)
*/


#include <stdio.h>
#include <string.h>
#include "tinyfiledialogs.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) /* silences warnings about strcpy strcat fopen*/
#endif

int main( int argc , char * argv[] )
{
	int lIntValue;
	char const * lPassword;
	char const * lTheSaveFileName;
	char const * lTheOpenFileName;
	char const * lTheSelectFolderName;
	char const * lTheHexColor;
	char const * lWillBeGraphicMode;
	unsigned char lRgbColor[3];
	FILE * lIn;
	char lBuffer[1024];
	char const * lFilterPatterns[2] = { "*.txt", "*.text" };

	/*tinyfd_forceConsole = 1;*/ /* default is 0 */
	tinyfd_verbose = argc - 1;
	tinyfd_silent = 1;

#ifdef _WIN32
	/* tinyfd_winUtf8 = 1; */
/* On windows, you decide if char holds 1:UTF-8(default) or 0:MBCS */
/* Windows is not ready to handle UTF-8 as many char functions like fopen() expect MBCS filenames.*/	   
/* This hello.c file has been prepared, on windows, to convert the filenames from UTF-8 to UTF-16
   and pass them passed to _wfopen() instead of fopen() */
#endif

	lWillBeGraphicMode = tinyfd_inputBox("tinyfd_query", NULL, NULL);

	strcpy(lBuffer, "v");
	strcat(lBuffer, tinyfd_version);
	if (lWillBeGraphicMode)
	{
		strcat(lBuffer, "\ngraphic mode: ");
	}
	else
	{
		strcat(lBuffer, "\nconsole mode: ");
	}
	strcat(lBuffer, tinyfd_response);
	strcat(lBuffer, "\n");
	strcat(lBuffer, tinyfd_needs+78);
	tinyfd_messageBox("hello", lBuffer, "ok", "info", 0);

	tinyfd_notifyPopup("the title", "the message\n\tfrom outer-space", "info");

	if ( lWillBeGraphicMode && ! tinyfd_forceConsole )
	{
		lIntValue = tinyfd_messageBox("Hello World",
			"graphic dialogs [yes] / console mode [no]?",
			"yesno", "question", 1);
		tinyfd_forceConsole = ! lIntValue ;	
	}

	lPassword = tinyfd_inputBox(
		"a password box", "your password will be revealed later", NULL);

	if (!lPassword) return 1;

	tinyfd_messageBox("your password as read", lPassword, "ok", "info", 1);

	lTheSaveFileName = tinyfd_saveFileDialog(
		"let us save this password",
		"passwordFile.txt",
		2,
		lFilterPatterns,
		NULL);

	if (! lTheSaveFileName)
	{
		tinyfd_messageBox(
			"Error",
			"Save file name is NULL",
			"ok",
			"error",
			1);
		return 1 ;
	}

#ifdef _WIN32
	if (tinyfd_winUtf8)
		lIn = _wfopen(tinyfd_utf8to16(lTheSaveFileName), L"w"); /* the UTF-8 filename is converted to UTF-16 to open the file*/
	else
#endif
	lIn = fopen(lTheSaveFileName, "w");

	if (!lIn)
	{
		tinyfd_messageBox(
			"Error",
			"Can not open this file in write mode",
			"ok",
			"error",
			1);
		return 1 ;
	}
	fputs(lPassword, lIn);
	fclose(lIn);

	lTheOpenFileName = tinyfd_openFileDialog(
		"let us read the password back",
		"",
		2,
		lFilterPatterns,
		NULL,
		0);

	if (! lTheOpenFileName)
	{
		tinyfd_messageBox(
			"Error",
			"Open file name is NULL",
			"ok",
			"error",
			0);
		return 1 ;
	}

#ifdef _WIN32
	if (tinyfd_winUtf8)
		lIn = _wfopen(tinyfd_utf8to16(lTheOpenFileName), L"r"); /* the UTF-8 filename is converted to UTF-16 */
	else
#endif
	lIn = fopen(lTheOpenFileName, "r");

	if (!lIn)
	{
		tinyfd_messageBox(
			"Error",
			"Can not open this file in read mode",
			"ok",
			"error",
			1);
		return(1);
	}

	lBuffer[0] = '\0';
	fgets(lBuffer, sizeof(lBuffer), lIn);
	fclose(lIn);

	tinyfd_messageBox("your password as it was saved", lBuffer, "ok", "info", 1);

	lTheSelectFolderName = tinyfd_selectFolderDialog(
		"let us just select a directory", NULL);

	if (!lTheSelectFolderName)
	{
		tinyfd_messageBox(
			"Error",
			"Select folder name is NULL",
			"ok",
			"error",
			1);
		return 1;
	}

	tinyfd_messageBox("The selected folder is", lTheSelectFolderName, "ok", "info", 1);

	lTheHexColor = tinyfd_colorChooser(
		"choose a nice color",
		"#FF0077",
		lRgbColor,
		lRgbColor);

	if (!lTheHexColor)
	{
		tinyfd_messageBox(
			"Error",
			"hexcolor is NULL",
			"ok",
			"error",
			1);
		return 1;
	}

	tinyfd_messageBox("The selected hexcolor is", lTheHexColor, "ok", "info", 1);

	tinyfd_messageBox("your read password was", lPassword, "ok", "info", 1);

	tinyfd_beep();

	return 0;
}

#ifdef _MSC_VER
#pragma warning(default:4996)
#endif

/*
OSX :
$ clang -o hello.app hello.c tinyfiledialogs.c
( or gcc )

UNIX :
$ gcc -o hello hello.c tinyfiledialogs.c
( or clang tcc owcc cc CC  )

Windows :
	MinGW needs gcc >= v4.9 otherwise some headers are incomplete:
	> gcc -o hello.exe hello.c tinyfiledialogs.c -LC:/mingw/lib -lcomdlg32 -lole32

	TinyCC needs >= v0.9.27 (+ tweaks - contact me) otherwise some headers are missing
	> tcc -o hello.exe hello.c tinyfiledialogs.c
	    -isystem C:\tcc\winapi-full-for-0.9.27\include\winapi
	    -lcomdlg32 -lole32 -luser32 -lshell32

	Borland C: > bcc32c -o hello.exe hello.c tinyfiledialogs.c

	OpenWatcom v2: create a character-mode executable project.

	VisualStudio :
	  Create a console application project,
	  it links against Comdlg32.lib & Ole32.lib.
*/
