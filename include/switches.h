/* SWITCHES.H - Switch-handling routines - header file. 
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

#ifndef INC_SWITCHES_H__
#define INC_SWITCHES_H__

typedef struct
{
	char InputPath[PATH_MAX];
	char OutputPath[PATH_MAX];
	int Backup;
	int Episode;
	int Export;
	int Import;
	int Extract;
	int SeparateMask;
	int IgrabSig;
	int IgrabHuffTrailMode;
	int Patch;
	char PalettePath[PATH_MAX];
	char EpisodeDefPath[PATH_MAX];
} SwitchStruct;

SwitchStruct *getswitches(int argc, char *argv[]);

#endif /* !INC_SWITCHES_H__ */
