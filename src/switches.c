/* SWITCHES.C - Switch-handling routines.
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
#include <limits.h>
#include <string.h>

#include "utils.h"
#include "switches.h"

#include "pconio.h"

// GNU C compatibility:
#ifndef stricmp
#define stricmp strcasecmp
#endif /* !stricmp */

static SwitchStruct switches;

static void showswitches(void);
static void defaultswitches();
static int getswitch(char *string, char **option, char **value);

SwitchStruct *getswitches(int argc, char *argv[])
{
	char *option = NULL, *value = NULL;
	int i;
	
	defaultswitches();
	
	if(argc <= 1)
	{
		quit ("Run \"modid -help\" for information on usage for information on "
		      "usage.\n");
	}

	for(i = 1; i < argc; i++)
	{
		if(!getswitch(argv[i], &option, &value))
			quit("Invalid switch '%s'!", argv[i]);

		if(stricmp(option, "nowait") == 0)
		{
			NoWait = 1;
		}
		if(stricmp(option, "debug") == 0)
		{
			DebugMode = 1;
		}			
		else if(stricmp(option, "export") == 0)
		{
			if(switches.Import)
				quit("Cannot both import and export!");
			switches.Export = 1;
		}
		else if(stricmp(option, "import") == 0)
		{
			if(switches.Export)
				quit("Cannot both import and export!");
			switches.Import = 1;
		}
		else if(stricmp(option, "gamedef") == 0)
		{
			FILE *def;
			if(!value)
				quit("No game definition file given!");

			/* First check for a valid path to a readable definition file */
			if ((def = fopen(value, "r")) != NULL)
			{
				fclose (def);
				strncpy (switches.EpisodeDefPath, value, PATH_MAX);
				continue;
			}
		}
		else if(stricmp(option, "gamedir") == 0)
		{
			if(!value)
				quit("No directory for game files given!");

			strncpy(switches.InputPath, value, PATH_MAX);
		}
		else if(stricmp(option, "bmpdir") == 0)
		{
			if(!value)
				quit("No directory for BMP files given!");

			strncpy(switches.OutputPath, value, PATH_MAX);
		}
		else if(stricmp(option, "palette") == 0)
		{
			if(!value)
				quit("No path to palette BMP given!");

			strncpy(switches.PalettePath, value, PATH_MAX);
		}
		else if(strcmp(option, "16color") == 0)
		{
			switches.SeparateMask = 1;
		}
		else if(strcmp(option, "igrabsig") == 0)
		{
			switches.IgrabSig = 1;
		}
		else if(strcmp(option, "igrabhufftrail1") == 0)
		{
			switches.IgrabHuffTrailMode = 1;
		}
		else if(strcmp(option, "igrabhufftrail2") == 0)
		{
			switches.IgrabHuffTrailMode = 2;
		}
		else if(strcmp(option, "nosparse") == 0)
		{
			switches.SparseTiles = 0;
		}
		else if(stricmp(option, "backup") == 0)
		{
			switches.Backup = 1;
		}
		else if(stricmp(option, "nopatch") == 0)
		{
			switches.Patch = 0;
		}
		else if(stricmp(option, "help") == 0 || stricmp(option, "?") == 0)
		{
			showswitches();
			exit(0);
		}
		else
		{
			showswitches();
			if (strlen(option) > 0) quit("Unknown switch '%s'!", option);
			quit("No parameters given!");
		}
	}
	
	if(!switches.Import && !switches.Export)
		quit("Either -import or -export must be given!");
	if(strlen(switches.EpisodeDefPath) == 0)
		quit("The game definition path must be given!");
	
	return &switches;
}

static void defaultswitches()
{
	strncpy(switches.InputPath, ".", PATH_MAX);
	strncpy(switches.OutputPath, ".", PATH_MAX);
	strncpy(switches.PalettePath, "", PATH_MAX);
	strncpy(switches.EpisodeDefPath, "", PATH_MAX);
	
	switches.Backup = 0;
	switches.Export = 0;
	switches.Import = 0;
	switches.SeparateMask = 0;
	switches.IgrabSig = 0;
	switches.IgrabHuffTrailMode = 0;
	switches.SparseTiles = 1;
	switches.Patch = 1;
}

/* Switch format: -option="value string" -option -option=value */
static int getswitch(char *string, char **option, char **value)
{
	unsigned int len = strlen(string);
	char *p;
	
	/* An empty string is not a valid switch */
	if(len == 0)
		return 0;
		
	/* A switch must begin with '-' or '/' */
	p = string;
	if(*p != '-' && *p != '/')
		return 0;
		
	p++;
	/* For UNIX switches style compatibility */
	if (*p == '-') p++;
	if (!p) return 1;
	
	*option = p;
	*value = NULL;
	while(*p)
	{
		if(*p == '=' || *p == ':')
		{
			*p = 0;
			p++;
			
			if(*p != '"')
			{
				*value = p;
			}
			else
			{
				*value = p++;
				while(*p && *p != '"')
					p++;
				*p = 0;
			}
			
			/* We've found a valid switch */
			return 1;
		}
		p++;
	}
	/* We've found a valid switch -- but it has no value*/
	return 1;
}

static void showswitches (void)
{
	fprintf(stdout,
			"  Valid options for ModId are:\n"
			"    -nowait             [Doesn't ask for a keypress at exit; useful for scripts]\n"
			"    -gamedef=FILEPATH   [Path to the game definition file (required)]\n"
			"    -export             [Export game data to BMP files (and more)]\n"
			"    -import             [Import game data from BMP files (and more)]\n"
			"    -gamedir=DIRECTORY  [Game files are in DIRECTORY (defaults to current)]\n"
			"    -bmpdir=DIRECTORY   [BMP files are in DIRECTORY (defaults to current)]\n"
			"    -palette=FILEPATH   [Set BMP palette for export (defaults to EGA colors)]\n"
			"    -16color            [Masked BMP files have 16 colors, separate masks]\n"
			"    -igrabsig           [Add !ID! signature before certains chunks as in IGRAB]\n"
			"    -igrabhufftrail1    [Add trailing byte in compression as in IGRAB]\n"
			"    -igrabhufftrail2    [Add trailing byte in <60000 chunk compression as in IGRAB]\n"
			"    -nosparse           [Export sparse Keen 4-6 tiles as black tiles, import as-is]\n"
			"    -backup             [Create backups of changed files]\n"
			"    -debug              [Show debug information for developers and testers]\n"
			"    -help               [Shows the valid options for ModId]\n"
			"\n"
			"For more information on ModId, read the README file provided with the\n"
			"program's source code or binaries.\n"
			"\n");
}

