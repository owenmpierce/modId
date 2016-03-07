/* PARSER.H - Episode definition file parsing unit
**
** Copyright (c)2011 by Owen Pierce "Lemm" (owen.m.pierce@gmail.com)
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

#ifndef INC_PARSER_H__
#define INC_PARSER_H__

#include <stddef.h>

typedef struct ValueNode
{
	char *Format;													// scanf() format string to be parsed for this value
	int Offset;														// offset in the parser buffer that this value should be written to
} ValueNode;

typedef struct CommandNode
{
	char *CommandStr;											// Name of command recognized by parser
	void (*PreCommand)  (void **);				// Function to call before doing command
	void (*PostCommand) (void **);				// Function to call after doing command
	ValueNode *Values;										// List of Values this command accepts
	struct CommandNode *SubCommands;			// List of Subcommands parented by this command
} CommandNode;

#define VALUENODE(s, t, o) { s, offsetof (t, o) }
#define ENDVALUE { NULL, 0 }

#define COMMANDLEAF(n,pre,post) { #n, pre, post, CV_##n, NULL   }
#define COMMANDNODE(n,pre,post) { #n, pre, post, CV_##n, SC_##n }
#define ENDCOMMAND		   { NULL, NULL, NULL, NULL, NULL }


int	parse_definition_file (char *file, void **buf, CommandNode *cmd);


#endif /* !INC_PARSER_H__ */
