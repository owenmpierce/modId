/* PCONIO.C - Multiplatform DOS-like console I/O library
**
** Copyright (C) 2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
**
** This software is provided 'as-is', without any express or implied warranty.
** In no event will the authors be held liable for any damages arising from
** the use of this software.
** Permission is granted to anyone to use this software for any purpose, including
** commercial applications, and to alter it and redistribute it freely, subject
** to the following restrictions:
**    1. The origin of this software must not be misrepresented; you must not
**       claim that you wrote the original software. If you use this software in
**       a product, an acknowledgment in the product documentation would be
**       appreciated but is not required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
** win32conio-dependent parts based on META engine v0.1.2 source code (src/console.cpp)
**   by Ignacio R. Morelle "Shadow Master" (GNU GPL v2), and yeah, I authorize the use
**   of that code here ;)
*/

#ifndef LINCONIO_H__
#define LINCONIO_H__

#ifdef DJGPP
#include <conio.h>
#else /* DJGPP */

#if defined ( __WIN32__ ) && !defined ( WIN32 )
#define WIN32
#endif /* __WIN32__ */

#ifdef WIN32
#define CONIO_WIN32
#define WIN32_LEAN_AND_MEAN

#define NOGDI

#include <windows.h>

#define true TRUE
#define false FALSE
#define bool BOOL

#else /* UNIX */

#define CONIO_UNIX
#include <ncurses.h>
#endif /* WIN32 */

// We'll just wrap the functions wherex(), wherey(), and gotoxy() here.

#ifdef __cplusplus
extern "C" {
#endif

#define COL_SUCCESS 1
#define COL_WARNING 2
#define COL_ERROR 3
#define COL_PROGRESS 4

#define IS_X 0
#define	IS_Y 1

#define setcol_normal setcol(0, false);
#define setcol_success setcol(COL_SUCCESS, true);
#define setcol_warning setcol(COL_WARNING, true);
#define setcol_error setcol(COL_ERROR, true);

void setcol(short pair, bool isbold);

#ifdef CONIO_UNIX
#define bold attron(A_BOLD);
#define unbold attroff(A_BOLD);
#else
#define bold setcol(-1, true);
#define unbold setcol(-1, false);
#endif /* CONIO_UNIX */


// I don't really know what these do, but I'll try to work based on their names.
void    gotoxy(int _x, int _y);
int     wherex();
int     wherey();

// Saves some values required by linconio calls at start
int     pconio_init(void);

void    do_output(char* format, ...);

#ifdef CONIO_UNIX
extern int NoGUI;
#endif /* CONIO_UNIX */


extern int NoWait;
#ifdef CONIO_UNIX
extern WINDOW *screen;
#endif /* CONIO_UNIX */

#ifdef CONIO_WIN32
void beep();
#endif /* CONIO_WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* DJGPP */
#endif /*!LINCONIO_H__*/
