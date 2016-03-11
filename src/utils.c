/* UTILS.C - Miscellaneous utility functions.
**
 ** Copyright (c)2016 by Owen Pierce
 ** Based on LModkeen 2 Copyright (c)2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
** Based on ModKeen 2.0.1 Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#include <sys/stat.h>

#include "utils.h"
#include "pconio.h"

/* Flag that indicates if the console output was or not initialized properly */
short console_inited = 0;
/* Flag that indicates if we should output more info about procedures */
int DebugMode = 0;

void dbg_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void quit(char *message, ...)
{
	va_list args;
	va_start(args, message);
	
	if (console_inited) {
		beep();
		setcol_error;
		do_output("\nERROR: ");
#ifndef WIN32
		vw_printw(screen, message, args);
#else
		vfprintf(stderr, message, args);
#endif /* !WIN32 */
		do_output("\n");
		setcol_normal;
	}
	else {
		fprintf(stderr, "\nERROR: ");
		vfprintf(stderr, message, args);
		fprintf(stderr, "\n");
	}

	va_end(args);	
	exit(1);
}

/* Open a file, backing it up if necessary (never overwriting an old
** backup), and returning the value returned from fopen().
*/
FILE *openfile(char *filename, char *access, int backup)
{
	char backupname[PATH_MAX];

	/* Check if we need to make a backup of the original file */
	if((strchr(access, 'w') || strchr(access, 'a')) && backup)
	{
		int i = 0;
		do sprintf(backupname, "%s.bak%d", filename, i++);
		while (fileexists(backupname));
		
		rename(filename, backupname);
	}
	
	return fopen(filename, access);
}


int fileexists(char *filename) // Version by Shadow Master
{
// Here I replaced the original code with some ugly code I found in the SuperTux
// 0.1.3 source. It should do the same thing anyways. Return 1 on success.
	struct stat filestat;
	if (stat(filename, &filestat) == -1) {
		return 0;
	} else {
		if (S_ISREG(filestat.st_mode)) return -1;
	}
	return 0;
}

typedef struct
{
	unsigned short mzid;
	unsigned short image_l;
	unsigned short image_h;
	unsigned short num_relocs;
	unsigned short header_size;
	unsigned short min_paras;
	unsigned short max_paras;
	unsigned short init_ss;
	unsigned short init_sp;
	unsigned short checksum;
	unsigned short init_ip;
	unsigned short init_cs;
	unsigned short reloc_offset;
	unsigned short overlay_num;
} EXE_HEADER;

#define EXEMZ	0x5A4D
#define EXEZM	0x4D5A

/* Returns the size of the loaded image of the program, or 0 on error */
/* SM: Modified so we can give a value of "headerlen" we're expecting... this way
 * we might support any exe file in the future */
int get_exe_image_size(FILE *f, unsigned long *imglen, unsigned long *headerlen)
{
	EXE_HEADER head;

	rewind(f);
	/* Read the header from the file if we can */
	if(fread(&head, sizeof(EXE_HEADER), 1, f) == 1)
	{
		/* Check that the 'MZ' id is present */
		if(head.mzid == EXEMZ || head.mzid == EXEZM)
		{
			/* Calculate the image size */
			if (!*headerlen) {
				*imglen = ((unsigned long)head.image_h - 1) * 512L + head.image_l - (unsigned long)head.header_size * 16L;
				*headerlen = (unsigned long)head.header_size * 16L;
			}
			else *imglen = ((unsigned long)head.image_h - 1) * 512L + head.image_l - *headerlen;
			return 1;
		}
	}

	// If we got here, something failed
	*imglen = *headerlen = 0;
	return 0;
}

/* Show that something is complete */
void completemsg()
{
	gotoxy(30, wherey());
	setcol_success;
	do_output("100%%\n");
	setcol_normal;
}

/* Show that "something is happening" (i.e. progress counter) */
void showprogress(float param)
{
	gotoxy(30, wherey());
	setcol(COL_PROGRESS, true);
	do_output("%3d%%", param);
	setcol_normal;
}

char *strlwr(char *str)
{
	unsigned char *p = (unsigned char  *)str;

	while (*p) {
		*p = tolower(*p);
		p++;
	}

	return str;
}
