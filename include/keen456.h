/* KEEN456.H - Keen 4, 5, and 6 import and export routines - header file.
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

#ifndef INC_KEEN456_H__
#define INC_KEEN456_H__

#include "switches.h"

/* Export routines */
void do_k456_export (SwitchStruct *switches);
/*
void k456_export_begin(SwitchStruct *switches);
void k456_export_tiles();
void k456_export_masked_tiles();
void k456_export_8_tiles();
void k456_export_8_masked_tiles();
void k456_export_bitmaps();
void k456_export_masked_bitmaps();
void k456_export_sprites();
void k456_export_fonts();
void k456_export_texts();
void k456_export_demos();
void k456_export_misc();
void k456_export_end();
*/

/* Import routines */
void do_k456_import (SwitchStruct *switches);
/*
void k456_import_begin(SwitchStruct *switches);
void k456_import_tiles();
void k456_import_masked_tiles();
void k456_import_8_tiles();
void k456_import_8_masked_tiles();
void k456_import_bitmaps();
void k456_import_masked_bitmaps();
void k456_import_sprites();
void k456_import_fonts();
void k456_import_texts();
void k456_import_demos();
void k456_import_misc();
void k456_import_end();
*/

/* General info routines */
/*
char* k456_getexefilename(char *buf);
char* k456_getegadictfile(char* buf);
char* k456_getegaheadfile(char* buf);
char* k456_getegagraphfile(char* buf);
char* k456_getgfxinfoefile(char* buf);
char* k456_getaudiotfile(char* buf);
char* k456_getaudiohedfile(char* buf);
char* k456_getgamemapsfile(char* buf);
char* k456_getmapheadfile(char* buf);
*/
#endif /* ! INC_KEEN456_H__ */
