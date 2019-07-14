/* UTILS.H - Miscellaneous utility functions - header file.
**
** Copyright (c)2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
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

#ifndef INC_UTILS_H__
#define INC_UTILS_H__


#define TRACE(x) do { if (DEBUG) dbg_printf x; } while (0)

void quit(char *message, ...);
void dbg_printf(const char *fmt, ...);
FILE *openfile(char *filename, char *access, int backup);
int fileexists(char *filename);
int get_exe_image_size(FILE *f, unsigned long *imglen, unsigned long *headerlen);
char *strlwr (char *str);

void completemsg();
void showprogress(float param);

/* Flag that indicates if the console output was or not initialized properly */
extern short console_inited;
/* Flag that indicates if we should output more info about procedures */
extern int DebugMode;

#endif /* !INC_UTILS_H__ */
