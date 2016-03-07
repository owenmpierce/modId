/* PARSER.C - Episode definition file parsing unit
 **
 ** Copyright (C) 2012 by Owen Pierce "Lemm". (owen.m.pierce@gmail.com)
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
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#include "parser.h"
#include "pconio.h"
#include "utils.h"

/*
 ** An episode definition file conists of multiple command lines.
 ** A command line has zero or one commands.
 ** A command takes the form of an alphanumeric string,
 **  followed by zero or more values on the same line of the command,
 **  followed by zero or more subcommands on subsequent lines.
 ** An attribute is either a scanf() readable string (double quote surrounded),
 **  or a scanf() "%i" readable number.
 ** Commands and Values are delimited by whitespace.
 ** All text between the # symbol and the newline is unprocessed comment text.
 */

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

#define MAXCOMMANDDEPTH 8
#define COMMANDDELIMITERS " \t\v\f\r\n"

typedef enum ParserState {
    prs_StartLine,
    prs_GetCommand,
    prs_DoCommand,
    prs_LineTooLong,
    prs_InvalidLine,
    prs_InvalidCommand,
    prs_MissingValue,
    prs_InvalidValue,
    prs_EndOfFile,
} ParserState;

typedef struct Parser {
    ParserState Status;
    FILE *InputFile;
    void **Buffer;
    unsigned Line;
    char LineBuffer[LINE_MAX];
    CommandNode *Command, *NextCommand;
    int CommandDepth;
    CommandNode * CommandStack[MAXCOMMANDDEPTH];
} Parser;



static int getnextline(Parser *parser);
static int getcommand(Parser *parser);
int docommand(Parser *parser);
int check_parser_status(Parser *parser);

static int getnextline(Parser *parser) {
    char *c;
    int n;

    assert(parser->Status == prs_StartLine);

    /* Read in a buffered line */
    if (fgets(parser->LineBuffer, LINE_MAX, parser->InputFile))
        parser->Line++;
    else if (feof(parser->InputFile)) {
        parser->Status = prs_EndOfFile;
        return 0;
    } else {
        parser->Status = prs_InvalidLine;
        return 0;
    }

    /* Ensure entire line was read */
    n = strlen(parser->LineBuffer);
    if (n == LINE_MAX - 1 && parser->LineBuffer[n] != '\n') {
        parser->Status = prs_LineTooLong;
        return 0;
    }

    /* Strip comments (to end of line) */
    c = strchr(parser->LineBuffer, '#');
    if (c)
        *c = '\0';

    if (DebugMode)
        do_output("Read line %i of parser file.\n", parser->Line);
    parser->Status = prs_GetCommand;
    return 1;
}

static int getcommand(Parser *parser) {
    char *cmdstr;
    CommandNode *cmd;

    assert(parser->Status == prs_GetCommand);
    assert(parser->CommandDepth >= 0 && parser->CommandDepth < MAXCOMMANDDEPTH);
    assert(parser->Command);

    /* Parse the first token on a line as a command */
    for (cmdstr = strtok(parser->LineBuffer, COMMANDDELIMITERS);
            cmdstr && !strcmp("", cmdstr); strtok(NULL, COMMANDDELIMITERS))
        continue;

    /* If the line is empty, get the next line */
    if (!cmdstr) {
        parser->Status = prs_StartLine;
        return 0;
    }

    if (DebugMode)
        do_output("Got directive token %s at line %i\n", cmdstr, parser->Line);

    /* If command is a subcommand of current menu, descend */
    for (cmd = parser->Command->SubCommands; cmd && cmd->CommandStr; cmd++) {
        if (!strcmp(cmd->CommandStr, cmdstr)) {
            parser->CommandStack[++parser->CommandDepth] = parser->Command->SubCommands;
            parser->Command = cmd;
            parser->Status = prs_DoCommand;
            if (DebugMode)
                do_output("Descending to level %i at line %i\n",
                    parser->CommandDepth, parser->Line);
            return 1;
        }
    }


    /* Search for command in current menu */
    do {
        for (cmd = parser->CommandStack[parser->CommandDepth];
                cmd->CommandStr; cmd++) {
            if (!strcmp(cmdstr, cmd->CommandStr)) {
                parser->Command = cmd;
                parser->Status = prs_DoCommand;
                if (DebugMode) {
                    do_output("Remaining on level %i at line %i\n", parser->CommandDepth, parser->Line);
                }
                return 1;
            }
        }

        /* Ascend a level and try again */
        if (DebugMode)
            do_output("Ascending to level %i at line %i.\n", parser->CommandDepth - 1, parser->Line);

    } while (--parser->CommandDepth >= 0);

    /* Couldn't find the supplied command */
    parser->Status = prs_InvalidCommand;
    return 0;
}

/*
 ** Process the command
 **
 */

int docommand(Parser *parser) {
    ValueNode *v;
    char *valstr;

    assert(parser->Status == prs_DoCommand);
    assert(*parser->Buffer != NULL);

    /* Create the format string */
    if (DebugMode)
        do_output("Performing directive %s at line %i (depth: %i)\n",
            parser->Command->CommandStr, parser->Line, parser->CommandDepth);

    /* Call the descent function */
    if (parser->Command->PreCommand)
        parser->Command->PreCommand(parser->Buffer);

    /* Assign each expected value to some offset in the episode struct */
    for (v = parser->Command->Values; v->Format; v++) {
        char formatbuf[PATH_MAX];

        /* Get the next token as a value */
        do {
            valstr = strtok(NULL, COMMANDDELIMITERS);
        } while (valstr && !strcmp("", valstr));

        /* No value found, so die */
        if (!valstr) {
            parser->Status = prs_MissingValue;
            return 0;
        }

        /* Scan the value in to its expected location */
        strcpy(formatbuf, v->Format);
        if (!sscanf(valstr, formatbuf, *parser->Buffer + v->Offset)) {
            parser->Status = prs_InvalidValue;
            return 0;
        }

        if (DebugMode) {
            do_output("Loaded value %s at line %i.\n", valstr, parser->Line);
        }
    }

    /* Call the PostCommand Function*/
    if (parser->Command->PostCommand)
        parser->Command->PostCommand(parser->Buffer);

    if (DebugMode)
        do_output("Finished with command %s at line %i.\n",
            parser->Command->CommandStr, parser->Line);

    /* Done with this command */
    parser->Status = prs_StartLine;
    return 1;
}

/* Parse episode definition information from a file and store it in
 ** an episode information structure
 */
int check_parser_status(Parser *parser) {

    /* Handle parser errors */
    switch (parser->Status) {
        case prs_InvalidLine:
            quit("Error reading line %d in definition file!", parser->Line);
            break;

        case prs_LineTooLong:
            quit("Line %d too long! (Max %d characters).", parser->Line, LINE_MAX);
            break;

        case prs_InvalidCommand:
            quit("Invalid command at line %d!", parser->Line);
            break;

        case prs_MissingValue:
            quit("Missing value at line %d", parser->Line);
            break;

        case prs_InvalidValue:
            quit("Invalid value at line %d!", parser->Line);
            break;

            /* Parser OK */
        case prs_StartLine:
        case prs_GetCommand:
        case prs_DoCommand:
        case prs_EndOfFile:
            break;

    }

    return 1;

}

/*
 Parses an episode definition file to fill the EpisodeInfoStruct
 */
int parse_definition_file(char *file, void **buf, CommandNode *cmd) {
    Parser parser;

    /* Setup the Parser machine */
    parser.InputFile = fopen(file, "r");
    if (!parser.InputFile)
        quit("Couldn't open parser input file %s!", file);
    else
        do_output("Opened parser definition file %s\n", file);

    fseek(parser.InputFile, 0, SEEK_SET);

    parser.Status = prs_DoCommand;
    parser.Buffer = buf;
    parser.Line = 0;
    strcpy(parser.LineBuffer, "   ");
    strtok(parser.LineBuffer, COMMANDDELIMITERS);
    parser.CommandDepth = 0;
    parser.CommandStack[0] = parser.Command = cmd;

    do {
        /* Do Current Command */
        if (parser.Status == prs_DoCommand)
            docommand(&parser);

        /* Get Next Line */
        if (parser.Status == prs_StartLine)
            getnextline(&parser);

        /* Get Next Command */
        if (parser.Status == prs_GetCommand)
            getcommand(&parser);

        /* Break if funny stuff happens */
        check_parser_status(&parser);

    } while (parser.Status != prs_EndOfFile);

    do_output("Definition file %s read.\n", file);

    /* Close the parser machine */
    fclose(parser.InputFile);

    return 1;
}

