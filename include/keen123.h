/* KEEN123.H - Keen 1, 2, and 3 import and export functions - header file.
**
** Copyright (c)2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
** Based on ModKeen 2.0.1 Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
**
** Parts of this file based on fin2bmp.c (FIN2BMP original source code
** Copyright (C) 2002 by Andrew Durdin).
** Also contains parts from CKSounds.bas v1.0 by the CK Guy
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

#ifndef INC_KEEN123_H__
#define INC_KEEN123_H__

#include "switches.h"

/* Export routines */
void do_k123_export (SwitchStruct *switches);
/*
void k123_export_begin(SwitchStruct *switches);
void k123_export_bitmaps();
void k123_export_sprites();
void k123_export_tiles();
void k123_export_fonts();
void k123_export_external();
void k123_export_end();
*/

void do_k123_import (SwitchStruct *switches);
/*
void k123_import_begin(SwitchStruct *switches);
void k123_import_bitmaps();
void k123_import_sprites();
void k123_import_tiles();
void k123_import_fonts();
void k123_import_external();
void k123_import_end();
*/

#endif /* !INC_KEEN123_H__ */
