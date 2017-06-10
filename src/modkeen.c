/* modkeen.c - source for a graphics importer/exporter for 16 bit ID software games
 ** 
 ** Copyright (c)2016-2017 by Owen Pierce, based on LModkeen
 ** Thanks to NY00123 for assistance
 **
 ** Greetings and thanks from Andrew Durdin to Anders Gavare and 
 ** Daniel Olson for their assistance.
 **
 ** Copyright (c)2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
 ** Original code Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
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
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include "bmp256.h"
#include "keen123.h"
#include "keen456.h"
#include "parser.h"
#include "pconio.h"
#include "switches.h"
#include "utils.h"


static char* txt_signature0 =
"ModId 0.1.2 - Copyright (c)2016-2017 Owen Pierce\n";
static char* txt_signature1 =
"Based on LMODKEEN 2 release 1 - Windows/Linux ModKeen port, Copyright (c) 2007 by Shadow Master\n";
static char* txt_signature2 =
"Based on ModKeen 2.0.1 source code, Copyright (c) 2002-2004 Andrew Durdin\n"
"Based on Fin2BMP source code, Copyright (c) 2002 Andrew Durdin\n"
"\n";

typedef enum {
	engine_None,
	engine_Vorticons,
	engine_Galaxy,
} EngineType;

typedef struct {
	EngineType Engine;
} EpisodeInfoStruct;



extern CommandNode SC_VORTICONS[];
extern ValueNode CV_VORTICONS[];
extern EpisodeInfoStruct *VorticonsEpisodeInfo;

extern CommandNode SC_GALAXY[];
extern ValueNode CV_GALAXY[];
extern EpisodeInfoStruct *GalaxyEpisodeInfo;

static EpisodeInfoStruct EpisodeInfo;
static EpisodeInfoStruct *DefinitionBufferPtr = &EpisodeInfo;

void read_galaxy_definition(void **);
void read_vorticons_definition(void **);

/* Command Tree Root */
ValueNode CV_MAIN[] = {
	ENDVALUE
};

CommandNode SC_MAIN[] = {
	COMMANDNODE(GALAXY, read_galaxy_definition, NULL),
	COMMANDNODE(VORTICONS, read_vorticons_definition, NULL),
	ENDCOMMAND
};

CommandNode CommandRoot[] = {
	COMMANDNODE(MAIN, NULL, NULL),
	ENDCOMMAND
};


// Local function prototypes

void read_vorticons_definition(void **buf) {
	if (EpisodeInfo.Engine != engine_None)
		quit("Only one engine type can be specified!\n");

	*buf = VorticonsEpisodeInfo;
	EpisodeInfo.Engine = engine_Vorticons;
}

void read_galaxy_definition(void **buf) {
	if (EpisodeInfo.Engine != engine_None)
		quit("Only one engine type can be specified!\n");
	DefinitionBufferPtr = GalaxyEpisodeInfo;
	EpisodeInfo.Engine = engine_Galaxy;
}

int main(int argc, char *argv[]) {
	SwitchStruct *switches;

	/* Display the signature */
	//fprintf(stdout, txt_signature1);
	//fprintf(stdout, txt_signature2);

	/* Parse first, ask questions later :) */
	/* Get the options */
	switches = getswitches(argc, argv);

	/* Setup terminal now */
	if (pconio_init()) {
		quit("CONIO: failed to setup terminal.");
	}

	/* Display the signature (again)*/
	bold;
	do_output(txt_signature0);
	unbold;
	do_output(txt_signature1);
	do_output(txt_signature2);

	if (switches->Export) {
		/* Set exporting palette */
		if (strcmp("", switches->PalettePath)) {
			if (!bmp256_setpalette(switches->PalettePath))
				quit("Could not open palette bitmap %s\n",
						switches->PalettePath);
		}

		/* Export all data */
		if (switches->EpisodeDefPath) {
			if (parse_definition_file(switches->EpisodeDefPath,
						(void **) &DefinitionBufferPtr, CommandRoot)) {
				switch (EpisodeInfo.Engine) {
					case engine_Vorticons:
						do_k123_export(switches);
						break;

					case engine_Galaxy:
						do_k456_export(switches);
						break;

					default:
					case engine_None:
						quit("Unknown engine declared in episode definition file!");
						break;
				}
			} else {
				quit("Episode definition file %s is improperly formatted.\n",
						switches->EpisodeDefPath);
			}

		}
	} else if (switches->Import) {
		/* Import all data */
		if (switches->EpisodeDefPath) {
			if (parse_definition_file(switches->EpisodeDefPath,
						(void **) &DefinitionBufferPtr, CommandRoot)) {
				switch (EpisodeInfo.Engine) {
					case engine_Vorticons:
						do_k123_import(switches);
						break;

					case engine_Galaxy:
						do_k456_import(switches);
						break;

					default:
					case engine_None:
						quit("Invalid engine defined in definition file %s!",
								switches->EpisodeDefPath);
						break;
				}
			} else {
				quit("Definition file %s improperly formatted.\n",
						switches->EpisodeDefPath);
			}
		}
	}

	do_output("Done!\n\n");

	/* Quit, indicating success */
	return 0;
}
