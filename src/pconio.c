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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "pconio.h"
#include "utils.h"

/* Shared prototypes and variables */
int NoWait = 0;
void pconio_exit();

/* ==================== NCURSES CONSOLE I/O ==================== */
#ifdef CONIO_UNIX

WINDOW	*screen;
int		oldcurstat;

int		NoGUI = 0;

void do_output(char* format, ...)
{
	char buf[512];
	va_list msg;
	va_start(msg, format);
	vsprintf(buf, format, msg);
	va_end(msg);
	
	wprintw(screen, "%s", buf);
	wrefresh(screen);
}

void setcol(short pair, bool isbold)
{
	if (isbold) attron(A_BOLD); else attroff(A_BOLD);
	color_set(pair, NULL);
}

int pconio_init()
{
	screen = initscr();
	if (!screen) return -1;

	cbreak();
	noecho();
	nonl();
	touchwin(screen);
	scrollok(screen, TRUE);
	oldcurstat=curs_set(0);
	console_inited = 1;
	atexit(&pconio_exit);
	// Miscellaneous init calls
	start_color();
	init_pair(COL_SUCCESS, COLOR_GREEN, COLOR_BLACK);
	init_pair(COL_WARNING, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COL_ERROR, COLOR_RED, COLOR_BLACK);
	init_pair(COL_PROGRESS, COLOR_YELLOW, COLOR_BLACK);
	return 0;
}

void pconio_exit()
{
	if (!console_inited) return;
	if (!NoWait) do_output("Press any key to exit...");
	getch();
	curs_set(oldcurstat);
	endwin();
	console_inited = 0;
}

void gotoxy(int x, int y)
{
	wmove(screen, y, x);
}

int wherex()
{
	int __attribute__ ((unused)) y;
	int x;
	getyx(screen, y, x);
	return x;
}

int wherey()
{
	int __attribute__ ((unused)) x;
	int y;
	getyx(screen, y, x);
	return y;
}
#endif /* CONIO_UNIX */

/* ==================== WIN32 CONSOLE I/O ==================== */
#ifdef CONIO_WIN32

HANDLE	hCon;
WORD	wDefaultConAttr;

int pconio_init()
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!hCon) return 1;
	GetConsoleScreenBufferInfo(hCon, &info);
	wDefaultConAttr = info.wAttributes;
	console_inited = 1;
	return 0;
}

void do_output(char* format, ...)
{
	char	buf[512];
	DWORD	dwRes;
	va_list msg;
	va_start(msg, format);
	vsprintf(buf, format, msg);
	va_end(msg);
	// Windows Kernel doesn't like printf() messing with consoles that have
	// text attributes applied, so we have to use Windows WriteConsole() here.
	// Curiously something similar happens with UNIX - ncurses
	WriteConsole(hCon, (LPSTR)buf, strlen(buf), &dwRes, NULL);
}

int wherex()
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hCon, &info);
	return info.dwCursorPosition.X;
}

int wherey()
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hCon, &info);
	return info.dwCursorPosition.Y;
}

void gotoxy(int x, int y)
{
	COORD newpos;
	newpos.X = x;
	newpos.Y = y;
	SetConsoleCursorPosition(hCon, newpos);
}

void setcol(short pair, bool isbold)
{
	// too bad Windows doesn't provide a more intuitive way of console coloring...
	COORD curpos;
	WORD newattr;
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hCon, &info);

	curpos.X = wherex();
	curpos.Y = wherey();

	switch (pair)
	{
		case -1:
			newattr = info.wAttributes;
			break;
		case COL_SUCCESS:
			newattr = FOREGROUND_GREEN;
			break;
		case COL_WARNING:
			newattr = FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		case COL_ERROR:
			newattr = FOREGROUND_RED;
			break;
		case COL_PROGRESS:
			newattr = FOREGROUND_BLUE | FOREGROUND_GREEN;
			break;			
		default:
			newattr = wDefaultConAttr;
			break;
	}
	if (pair != 0) {
		if (isbold) newattr |= FOREGROUND_INTENSITY;
		else newattr ^= FOREGROUND_INTENSITY;
	}
	SetConsoleTextAttribute(hCon, newattr);

	SetConsoleCursorPosition(hCon, curpos);
}

void beep()
{
	// Empty. I don't remember the API that caused a system beep from user32 XD
	;
}


#endif /* CONIO_WIN32 */


