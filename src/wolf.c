/* wolf.c - Import and Export routines for Wolf3D VGAGRAPH files
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <memory.h>
#include <assert.h>

#include "bmp256.h"
#include "huff.h"
#include "parser.h"
#include "pconio.h"
#include "utils.h"
#include "switches.h"

#pragma pack(1)

void parse_k456_misc_ascent(void **);
void parse_k456_misc_descent(void **);
void parse_k456_b800_descent(void **);
void parse_k456_terminator_descent(void **);

typedef struct {
    uint16_t Width;
    uint16_t Height;
} BitmapHeadStruct;

typedef struct {
    uint16_t Width;
    uint16_t Height;
    int16_t OrgX;
    int16_t OrgY;
    int16_t Rx1, Ry1;
    int16_t Rx2, Ry2;
    uint16_t Shifts;
} SpriteHeadStruct;

typedef struct {
    uint16_t Height;
    uint16_t Offset[256];
    uint8_t Width[256];
} FontHeadStruct;

typedef struct {
    uint16_t Height;
    uint16_t Width;
    uint16_t LineStarts[];
} TerminatorHeadStruct;

#pragma pack()
// The ExeImageSize fields are values from the real start of exe image,
// i.e. KEEN4E.EXE + 0x2E00 to EndOfFile=0x3D740
// It seems that the standard shareware KDREAMS.EXE starts at 0x1C00

typedef struct {
    char ExeName[14]; /* 8.3 Exe Name added */
    char EgaGraphName[14]; /* Custom EGAGraph name (optional) */
    char GameExt[4]; /* Archive Extension added */
    unsigned long ExeImageSize;
    unsigned long ExeHeaderSize;
    unsigned long OffEgaHead;
    unsigned long OffEgaDict;
    unsigned int GrStarts;
    unsigned long NumChunks;
    unsigned int NumFonts, IndexFonts;
    unsigned int NumMaskedFonts, IndexMaskedFonts;
    unsigned int NumBitmaps, IndexBitmaps, IndexBitmapTable;
    unsigned int NumMaskedBitmaps, IndexMaskedBitmaps, IndexMaskedBitmapTable;
    unsigned int NumSprites, IndexSprites, IndexSpriteTable;
    unsigned int Num8Tiles, Index8Tiles;
    unsigned int Num8MaskedTiles, Index8MaskedTiles;
    unsigned int Num16Tiles, Index16Tiles;
    unsigned int Num16MaskedTiles, Index16MaskedTiles;
    unsigned int Num32Tiles, Index32Tiles;
    unsigned int Num32MaskedTiles, Index32MaskedTiles;
    unsigned int NumMisc, IndexMisc; /* Includes ANSI images */
    unsigned int NumTexts, IndexTexts;
    unsigned int NumDemos, IndexDemos;
} EpisodeInfoStruct;

/* Stores filenames of misc chunks (e.g. terminator text, b800 text, etc) */

typedef struct MiscInfoList_s {
    char Type[16]; // MISC code used in definition file
    char File[PATH_MAX]; // name of the export file (w/o file extension)
    unsigned int Chunk; // Game Archive Chunk number
    struct MiscInfoList_s *next; // Tail of list
} MiscInfoList;


#define HEADERLEN_KDREAMS		0x1C00
#define HEADERLEN_BMENACE1		0x3800
#define HEADERLEN_BMENACE2		0x3800
#define HEADERLEN_BMENACE3		0x3A00		/* strange */

#define HEADERLEN_DDAVE2		0x0

#define HEADERLEN_SHADOWK		0x0

#define NUM_VERSIONS	10

typedef struct {
    unsigned long len;
    uint8_t *data;
} ChunkStruct;

static int ExportInitialised = 0;
static int ImportInitialised = 0;
static ChunkStruct *EgaGraph = NULL;
static BitmapHeadStruct *BmpHead = NULL;
static BitmapHeadStruct *BmpMaskedHead = NULL;
static SpriteHeadStruct *SprHead = NULL;
static SwitchStruct *Switches = NULL;
static MiscInfoList *MiscInfos = NULL;

/* A buffer for file-defined episodes */
static EpisodeInfoStruct EpisodeInfo;
EpisodeInfoStruct *GalaxyEpisodeInfo = &EpisodeInfo;

/* Command tree for parsing galaxy definition files */
static ValueNode CV_GAMEEXT[] = {
    VALUENODE("%3s", EpisodeInfoStruct, GameExt),
    ENDVALUE
};

static ValueNode CV_EGAGRAPH[] = {
    VALUENODE("%13s", EpisodeInfoStruct, EgaGraphName),
    ENDVALUE
};

static ValueNode CV_EXEINFO[] = {
    VALUENODE("%13s", EpisodeInfoStruct, ExeName),
    VALUENODE("%li", EpisodeInfoStruct, ExeImageSize),
    VALUENODE("%li", EpisodeInfoStruct, OffEgaHead),
    VALUENODE("%li", EpisodeInfoStruct, OffEgaDict),
    VALUENODE("%li", EpisodeInfoStruct, ExeHeaderSize),
    ENDVALUE
};

static ValueNode CV_FONT[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumFonts),
    VALUENODE("%i", EpisodeInfoStruct, IndexFonts),
    ENDVALUE
};

/*
Apparently FONTMS "Masked?, Monochrome? Fonts" are defined in Wolf source, but I've yet to see one in use
 */
static ValueNode CV_FONTM[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumMaskedFonts),
    VALUENODE("%i", EpisodeInfoStruct, IndexMaskedFonts),
    ENDVALUE
};

static ValueNode CV_PICS[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumBitmaps),
    VALUENODE("%i", EpisodeInfoStruct, IndexBitmaps),
    VALUENODE("%i", EpisodeInfoStruct, IndexBitmapTable),
    ENDVALUE
};

static ValueNode CV_PICM[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumMaskedBitmaps),
    VALUENODE("%i", EpisodeInfoStruct, IndexMaskedBitmaps),
    VALUENODE("%i", EpisodeInfoStruct, IndexMaskedBitmapTable),
    ENDVALUE
};

static ValueNode CV_SPRITES[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumSprites),
    VALUENODE("%i", EpisodeInfoStruct, IndexSprites),
    VALUENODE("%i", EpisodeInfoStruct, IndexSpriteTable),
    ENDVALUE
};

static ValueNode CV_TILE8[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num8Tiles),
    VALUENODE("%i", EpisodeInfoStruct, Index8Tiles),
    ENDVALUE
};

static ValueNode CV_TILE8M[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num8MaskedTiles),
    VALUENODE("%i", EpisodeInfoStruct, Index8MaskedTiles),
    ENDVALUE
};

static ValueNode CV_TILE16[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num16Tiles),
    VALUENODE("%i", EpisodeInfoStruct, Index16Tiles),
    ENDVALUE
};

static ValueNode CV_TILE16M[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num16MaskedTiles),
    VALUENODE("%i", EpisodeInfoStruct, Index16MaskedTiles),
    ENDVALUE
};

static ValueNode CV_TILE32[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num32Tiles),
    VALUENODE("%i", EpisodeInfoStruct, Index32Tiles),
    ENDVALUE
};

static ValueNode CV_TILE32M[] = {
    VALUENODE("%i", EpisodeInfoStruct, Num32MaskedTiles),
    VALUENODE("%i", EpisodeInfoStruct, Index32MaskedTiles),
    ENDVALUE
};

static ValueNode CV_TEXT[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumTexts),
    VALUENODE("%i", EpisodeInfoStruct, IndexTexts),
    ENDVALUE
};
static ValueNode CV_DEMO[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumDemos),
    VALUENODE("%i", EpisodeInfoStruct, IndexDemos),
    ENDVALUE
};

static ValueNode CV_B800TEXT[] = {

    VALUENODE("%i", MiscInfoList, Chunk),
    VALUENODE("%s", MiscInfoList, File),
    ENDVALUE
};

static ValueNode CV_TERMINATOR[] = {

    VALUENODE("%i", MiscInfoList, Chunk),
    VALUENODE("%s", MiscInfoList, File),
    ENDVALUE
};

static ValueNode CV_MISC[] = {

    VALUENODE("%i", MiscInfoList, Chunk),
    VALUENODE("%s", MiscInfoList, File),
    ENDVALUE
};

static ValueNode CV_CHUNKS[] = {
    VALUENODE("%li", EpisodeInfoStruct, NumChunks),
    ENDVALUE
};

static CommandNode SC_CHUNKS[] = {
    { "FONT", NULL, NULL, CV_FONT, NULL},
    { "FONTM", NULL, NULL, CV_FONTM, NULL},
    { "PICS", NULL, NULL, CV_PICS, NULL},
    { "PICM", NULL, NULL, CV_PICM, NULL},
    { "SPRITES", NULL, NULL, CV_SPRITES, NULL},
    { "TILE8", NULL, NULL, CV_TILE8, NULL},
    { "TILE8M", NULL, NULL, CV_TILE8M, NULL},
    { "TILE16", NULL, NULL, CV_TILE16, NULL},
    { "TILE16M", NULL, NULL, CV_TILE16M, NULL},
    { "TILE32", NULL, NULL, CV_TILE32, NULL},
    { "TILE32M", NULL, NULL, CV_TILE32M, NULL},
    { "TEXT", NULL, NULL, CV_TEXT, NULL},
    { "DEMO", NULL, NULL, CV_DEMO, NULL},
    { "B800TEXT", parse_k456_b800_descent, parse_k456_misc_ascent, CV_B800TEXT, NULL},
    { "TERMINATOR", parse_k456_terminator_descent, parse_k456_misc_ascent, CV_TERMINATOR, NULL},
    { "MISC", parse_k456_misc_descent, parse_k456_misc_ascent, CV_MISC, NULL},
    ENDCOMMAND
};

static ValueNode CV_GRSTARTS[] = {
    VALUENODE("%i", EpisodeInfoStruct, GrStarts),
    ENDVALUE
};


ValueNode CV_GALAXY[] = {
    ENDVALUE
};

CommandNode SC_GALAXY[] = {
    COMMANDLEAF(GAMEEXT, NULL, NULL),
    COMMANDLEAF(EGAGRAPH, NULL, NULL),
    COMMANDLEAF(GRSTARTS, NULL, NULL),
    COMMANDLEAF(EXEINFO, NULL, NULL),
    COMMANDNODE(CHUNKS, NULL, NULL),
    ENDCOMMAND
};


/************************************************************************************************************/
/** KEEN 4, 5, 6 EXPORTING ROUTINES *************************************************************************/

/************************************************************************************************************/


void wolf_export_begin(SwitchStruct *switches) {
    char filename[PATH_MAX];
    FILE *exefile, *graphfile, *headfile, *dictfile, *filetoread;
    unsigned long exeimglen, exeheaderlen;
    uint32_t offset;
    uint8_t *CompEgaGraphData;
    uint32_t *EgaHead = NULL;
    uint32_t egagraphlen, inlen, outlen;
    int i, j;
    uint32_t grstart_mask;


    /* Never allow the export start to occur more than once */
    if (ExportInitialised)
        quit("Tried to initialise Galaxy Engine files a second time!");

    /* Save the switches */
    Switches = switches;

    /* Adjust for 3 or 4 byte GRSTARTS */
    if (EpisodeInfo.GrStarts != 3 && EpisodeInfo.GrStarts != 4)
        quit("GRSTARTS must be 3 or 4! (Defined as %i).\n", EpisodeInfo.GrStarts);

    grstart_mask = 0xFFFFFFFF >> (8 * (4 - EpisodeInfo.GrStarts));

    /* Read Game archive data */
    exefile = graphfile = headfile = dictfile = filetoread = NULL;

    /* Check for EGADICT and EGAHEAD*/
    sprintf(filename, "%s/egadict.%s", Switches->InputPath, EpisodeInfo.GameExt);
    dictfile = fopen(filename, "rb");
    sprintf(filename, "%s/egahead.%s", Switches->InputPath, EpisodeInfo.GameExt);
    headfile = fopen(filename, "rb");

    /* If either one is not found externally, then check in the exe */
    if (!dictfile || !headfile) {
        /* Open the EXE */
        sprintf(filename, "%s/%s", Switches->InputPath, EpisodeInfo.ExeName);
        exefile = fopen(filename, "rb");
        if (!exefile)
            quit("Can't open %s for reading!", filename);

        // Due to my modification to get_exe_image_size(), I MUST initialize exeheaderlen with 0
        // or random data might be extracted from it, which screws up the resultant exeimglen value.
        // TODO: bug with this
        exeheaderlen = 0;

        /* Check that it's the right version (and it's unpacked) */
        if (!get_exe_image_size(exefile, &exeimglen, &exeheaderlen))
            quit("%s is not a valid exe file!", filename);
        if (DebugMode) {
            do_output("Exe Image Length is: 0x%08lX\n", exeimglen);
            do_output("Header length is: 0x%08lX\n", exeheaderlen);
        }
        if (exeimglen != EpisodeInfo.ExeImageSize)
            quit("Incorrect .exe image length for %s.  (Expected %X, found %X). "
                "Ensure that you have the proper game version and that the executable has been UNLZEXE'd!\n", filename, EpisodeInfo.ExeImageSize,
                exeimglen);
        if (exeheaderlen != EpisodeInfo.ExeHeaderSize)
            quit("Incorrect .exe header length for %s.  (Expected %X, found %X). "
                " Check your game version!\n", filename, EpisodeInfo.ExeHeaderSize,
                exeheaderlen);
    }

    /* Get the EGADICT data*/
    if (dictfile) {
        filetoread = dictfile;
        offset = 0;
    } else {
        setcol_warning;
        do_output("Cannot find EGADICT.%s, Extracting EGADICT from the exe.\n", EpisodeInfo.GameExt);
        setcol_normal;
        filetoread = exefile;
        offset = exeheaderlen + EpisodeInfo.OffEgaDict;
    }
    huff_read_dictionary(filetoread, offset);

    /* Get the EGAHEAD Data */
    EgaHead = malloc(EpisodeInfo.NumChunks * sizeof (uint32_t));
    if (!EgaHead)
        quit("Not enough memory to read EGAHEAD!");

    if (headfile) {
        filetoread = headfile;
        offset = 0;
    } else if (exefile) {
        setcol_warning;
        do_output("Cannot find EGAHEAD.%s, Extracting EGAHEAD data from the exe.\n", EpisodeInfo.GameExt);
        setcol_normal;
        filetoread = exefile;
        offset = exeheaderlen + EpisodeInfo.OffEgaHead, SEEK_SET;
    } else {
        quit("Could not find header in egahead.%s or in %s!", EpisodeInfo.GameExt, EpisodeInfo.ExeName);
    }

    /* Read the EGAHEAD */
    fseek(filetoread, offset, SEEK_SET);
    for (i = 0; i < EpisodeInfo.NumChunks; i++) {
        fread(&offset, EpisodeInfo.GrStarts, 1, filetoread);
        offset &= grstart_mask;
        EgaHead[i] = offset;
    }

    /* Close the files */
    if (dictfile)
        fclose(dictfile);
    if (headfile)
        fclose(headfile);
    if (exefile)
        fclose(exefile);

    /* Now read the EGAGRAPH */
    sprintf(filename, "%s/%s", Switches->InputPath, EpisodeInfo.EgaGraphName);
    if (!fileexists(filename))
        sprintf(filename, "%s/egagraph.%s", Switches->InputPath, EpisodeInfo.GameExt);

    graphfile = fopen(filename, "rb");
    if (!graphfile)
        quit("Can't open %s!", filename);
    fseek(graphfile, 1, SEEK_END);
    egagraphlen = ftell(graphfile) - 1;
    fseek(graphfile, 0, SEEK_SET);

    CompEgaGraphData = (uint8_t *) malloc(egagraphlen);
    if (!CompEgaGraphData)
        quit("Not enough memory to read EGAGRAPH!");

    fread(CompEgaGraphData, egagraphlen, 1, graphfile);

    fclose(graphfile);

    /* Now decompress the EGAGRAPH */
    do_output("Decompressing: ");
    EgaGraph = (ChunkStruct *) malloc(EpisodeInfo.NumChunks * sizeof (ChunkStruct));
    if (!EgaGraph)
        quit("Not enough memory to decompress EGAGRAPH!");
    for (i = 0; i < EpisodeInfo.NumChunks; i++) {
        /* Show that something is happening */
        showprogress((int) ((i * 100) / EpisodeInfo.NumChunks));

        offset = EgaHead[i];

        /* Make sure the chunk is valid */
        if (offset != grstart_mask) {
            /* Get the expanded length of the chunk */
            if (i >= EpisodeInfo.Index8Tiles && i < EpisodeInfo.Index32MaskedTiles + EpisodeInfo.Num32MaskedTiles) {
                /* Expanded sizes of 8, 16,and 32 tiles are implicit */
                if (i >= EpisodeInfo.Index16MaskedTiles) /* 16x16 tiles are one/chunk */
                    outlen = 2 * 16 * 5;
                else if (i >= EpisodeInfo.Index16Tiles)
                    outlen = 2 * 16 * 4;
                else if (i >= EpisodeInfo.Index8MaskedTiles) /* 8x8 tiles are all in one chunk! */
                    outlen = EpisodeInfo.Num8MaskedTiles * 8 * 5;
                else if (i >= EpisodeInfo.Index8Tiles)
                    outlen = EpisodeInfo.Num8Tiles * 8 * 4;
            } else {
                memcpy(&outlen, CompEgaGraphData + offset, sizeof (uint32_t));
                offset += sizeof (uint32_t);
            }

            /* Allocate memory and decompress the chunk */
            EgaGraph[i].len = outlen;
            EgaGraph[i].data = (uint8_t *) malloc(outlen);
            if (!EgaGraph[i].data)
                quit("Not enough memory to decompress EGAGRAPH chunk %d (size %X at %X)!", i, outlen, offset);

            inlen = 0;
            /* Find out the input length */
            for (j = i + 1; j < EpisodeInfo.NumChunks; j++) {
                if (EgaHead[j] != grstart_mask) {
                    inlen = EgaHead[j] - offset;
                    break;
                }
            }
	    /* Technically, there should be an extra header entry proceeding the final grstart,
	     * as this is how the ID Caching manager determines the compressed chunk length */
            if (j == EpisodeInfo.NumChunks)
                inlen = egagraphlen - offset;
            if (DebugMode) {
                gotoxy(0, wherey() + 1);
                do_output("Expanding chunk:");
                gotoxy(30, wherey());
                setcol(COL_PROGRESS, true);
                do_output("  EgaHead[%d] = 0x%08lX", i, (unsigned long)EgaHead[i]);
                setcol_normal;
                gotoxy(0, wherey() - 1);
            }
            huff_expand(CompEgaGraphData + offset, EgaGraph[i].data, inlen, outlen);

        } else {
            EgaGraph[i].len = 0;
            EgaGraph[i].data = NULL;
        }

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
    if (DebugMode) {
        gotoxy(30, wherey());
        setcol_success;
        do_output("done                        ");
        setcol_normal;
        gotoxy(0, wherey() + 1);
    }

    /* Set up pointers to bitmap and sprite tables if said data type exists */
    if (EpisodeInfo.NumBitmaps > 0)
        BmpHead = (BitmapHeadStruct *) EgaGraph[EpisodeInfo.IndexBitmapTable].data;

    if (EpisodeInfo.NumMaskedBitmaps > 0)
        BmpMaskedHead = (BitmapHeadStruct *) EgaGraph[EpisodeInfo.IndexMaskedBitmapTable].data;
    if (EpisodeInfo.NumSprites > 0)
        SprHead = (SpriteHeadStruct *) EgaGraph[EpisodeInfo.IndexSpriteTable].data;

    free(EgaHead);
    free(CompEgaGraphData);

    ExportInitialised = 1;
}

void wolf_export_end() {
    int i;

    if (!ExportInitialised)
        quit("Tried to end export before beginning!");

    for (i = 0; i < EpisodeInfo.NumChunks; i++)
        if (EgaGraph[i].data)
            free(EgaGraph[i].data);

    free(EgaGraph);

    ExportInitialised = 0;
}

void k456_export_bitmaps() {
    BITMAP256 *bmp, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export bitmaps before initialisation!");

    if (EpisodeInfo.NumBitmaps == 0)
        return;

    /* Export all the bitmaps */
    do_output("Exporting bitmaps: ");

    for (i = 0; i < EpisodeInfo.NumBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumBitmaps);

        if (EgaGraph[EpisodeInfo.IndexBitmaps + i].data) {
            /* Decode the bitmap data */
            for (p = 0; p < 4; p++) {
                /* Create a 1bpp bitmap for each plane */
                planes[p] = bmp256_create(BmpHead[i].Width * 8, BmpHead[i].Height, 1);
                if (!planes[p])
                    quit("Not enough memory to create bitmaps!");

                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.IndexBitmaps + i].data + p * BmpHead[i].Width * BmpHead[i].Height;
                for (y = 0; y < BmpHead[i].Height; y++)
                    memcpy(planes[p]->lines[y], pointer + y * BmpHead[i].Width, BmpHead[i].Width);
            }

            /* Create the bitmap file */
            sprintf(filename, "%s/%s_pic_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
            bmp = bmp256_merge_ex(planes, 4, 4);
            if (!bmp)
                quit("Not enough memory to create bitmap!");
            if (!bmp256_save(bmp, filename, Switches->Backup))
                quit("Can't open bitmap file %s!", filename);

            /* Free the memory used */
            for (p = 0; p < 4; p++) {
                bmp256_free(planes[p]);
            }
            bmp256_free(bmp);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_export_masked_bitmaps() {
    BITMAP256 *bmp, *mbmp, *planes[5];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export masked bitmaps before initialisation!");

    if (EpisodeInfo.NumMaskedBitmaps == 0)
        return;

    /* Export all the bitmaps */
    do_output("Exporting masked bitmaps: ");

    for (i = 0; i < EpisodeInfo.NumMaskedBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumMaskedBitmaps);

        if (EgaGraph[EpisodeInfo.IndexMaskedBitmaps + i].data) {
            /* Decode the mask and color plane data */
            for (p = 0; p < 5; p++) {
                /* Create a 1bpp bitmap for each plane */
                planes[p] = bmp256_create(BmpMaskedHead[i].Width * 8, BmpMaskedHead[i].Height, 1);

                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.IndexMaskedBitmaps + i].data + ((p + 1) % 5) * BmpMaskedHead[i].Width * BmpMaskedHead[i].Height;
                for (y = 0; y < BmpMaskedHead[i].Height; y++)
                    memcpy(planes[p]->lines[y], pointer + y * BmpMaskedHead[i].Width, BmpMaskedHead[i].Width);
            }

            if (Switches->SeparateMask) {
                /* Draw the color planes and mask separately */
                mbmp = bmp256_create(BmpMaskedHead[i].Width * 8 * 2, BmpMaskedHead[i].Height, 4);
                bmp256_blit(planes[4], 0, 0, mbmp, BmpMaskedHead[i].Width * 8, 0, BmpMaskedHead[i].Width * 8, BmpMaskedHead[i].Height);
                bmp = bmp256_merge_ex(planes, 4, 4);
                bmp256_blit(bmp, 0, 0, mbmp, 0, 0, BmpMaskedHead[i].Width * 8, BmpMaskedHead[i].Height);
                bmp256_free(bmp);
            } else {
                /* Incorporate the mask information with the color planes */
                mbmp = bmp256_merge_ex(planes, 5, 8);
            }

            /* Create the bitmap file */
            sprintf(filename, "%s/%s_picm_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
            if (!bmp256_save(mbmp, filename, Switches->Backup))
                quit("Can't open bitmap file %s!", filename);

            /* Free the memory used */
            for (p = 0; p < 5; p++)
                bmp256_free(planes[p]);
            bmp256_free(mbmp);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_export_tiles() {
    BITMAP256 *tiles, *bmp, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export tiles before initialisation!");

    /* Export all the tiles into one bitmap*/
    do_output("Exporting tiles: ");

    tiles = bmp256_create(16 * 18, 16 * ((EpisodeInfo.Num16Tiles + 17) / 18), 4);

    /* Create a 1bpp bitmap for each plane */
    for (p = 0; p < 4; p++)
        planes[p] = bmp256_create(16, 16, 1);

    for (i = 0; i < EpisodeInfo.Num16Tiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num16Tiles);

        if (EgaGraph[EpisodeInfo.Index16Tiles + i].data) {
            /* Decode the image data */
            for (p = 0; p < 4; p++) {
                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.Index16Tiles + i].data + p * 2 * 16;
                for (y = 0; y < 16; y++)
                    memcpy(planes[p]->lines[y], pointer + y * 2, 2);
            }

            bmp = bmp256_merge_ex(planes, 4, 4);
            bmp256_blit(bmp, 0, 0, tiles, 16 * (i % 18), 16 * (i / 18), 16, 16);
            bmp256_free(bmp);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    /* Create the bitmap file */
    sprintf(filename, "%s/%s_tile16.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(tiles, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);

    /* Free the memory used */
    for (p = 0; p < 4; p++)
        bmp256_free(planes[p]);
    bmp256_free(tiles);
}

void k456_export_masked_tiles() {
    BITMAP256 *tiles, *bmp, *planes[5];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export masked tiles before initialisation!");

    /* Export all the masked tiles into one bitmap*/
    do_output("Exporting masked tiles: ");

    if (Switches->SeparateMask)
        tiles = bmp256_create(16 * 18 * 2, 16 * ((EpisodeInfo.Num16MaskedTiles + 17) / 18), 4);
    else
        tiles = bmp256_create(16 * 18, 16 * ((EpisodeInfo.Num16MaskedTiles + 17) / 18), 8);

    /* Create a 1bpp bitmap for each plane */
    for (p = 0; p < 5; p++)
        planes[p] = bmp256_create(16, 16, 1);

    for (i = 0; i < EpisodeInfo.Num16MaskedTiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num16MaskedTiles);

        if (EgaGraph[EpisodeInfo.Index16MaskedTiles + i].data) {
            /* Decode the mask and color plane data */
            for (p = 0; p < 5; p++) {
                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.Index16MaskedTiles + i].data + ((p + 1) % 5) * 2 * 16;
                for (y = 0; y < 16; y++)
                    memcpy(planes[p]->lines[y], pointer + y * 2, 2);
            }

            /* Draw the tile to the master tilesheet */
            if (Switches->SeparateMask) {
                bmp256_blit(planes[4], 0, 0, tiles, 16 * 18 + 16 * (i % 18), 16 * (i / 18), 16, 16);
                bmp = bmp256_merge_ex(planes, 4, 4);
            } else {
                bmp = bmp256_merge_ex(planes, 5, 8);
            }

            bmp256_blit(bmp, 0, 0, tiles, 16 * (i % 18), 16 * (i / 18), 16, 16);
            bmp256_free(bmp);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    /* Create the bitmap file */
    sprintf(filename, "%s/%s_tile16m.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(tiles, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);

    /* Free the memory used */
    for (p = 0; p < 5; p++)
        bmp256_free(planes[p]);
    bmp256_free(tiles);
}

void k456_export_8_tiles() {
    BITMAP256 *tiles, *bmp, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export 8x8 tiles before initialisation!");

    /* Export all the 8x8 tiles into one bitmap*/
    do_output("Exporting 8x8 tiles: ");

    tiles = bmp256_create(8, 8 * EpisodeInfo.Num8Tiles, 4);

    /* Create a 1bpp bitmap for each plane */
    for (p = 0; p < 4; p++)
        planes[p] = bmp256_create(8, 8, 1);

    if (EgaGraph[EpisodeInfo.Index8Tiles].data) {
        for (i = 0; i < EpisodeInfo.Num8Tiles; i++) {
            /* Show that something is happening */
            showprogress((i * 100) / EpisodeInfo.Num8Tiles);

            /* Decode the image data */
            for (p = 0; p < 4; p++) {
                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.Index8Tiles].data + (i * 4 * 8) + p * 8;
                for (y = 0; y < 8; y++)
                    memcpy(planes[p]->lines[y], pointer + y, 1);
            }

            bmp = bmp256_merge_ex(planes, 4, 4);
            bmp256_blit(bmp, 0, 0, tiles, 0, 8 * i, 8, 8);
            bmp256_free(bmp);
            //do_output("\x8\x8\x8\x8");
        }
        completemsg();
    }

    /* Create the bitmap file */
    sprintf(filename, "%s/%s_tile8.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(tiles, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);

    /* Free the memory used */
    for (p = 0; p < 4; p++)
        bmp256_free(planes[p]);
    bmp256_free(tiles);
}

void k456_export_8_masked_tiles() {
    BITMAP256 *tiles, *bmp, *planes[5];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export 8x8 masked tiles before initialisation!");

    /* Export all the 8x8 masked tiles into one bitmap*/
    do_output("Exporting 8x8 masked tiles: ");

    if (Switches->SeparateMask)
        tiles = bmp256_create(8 * 2, 8 * EpisodeInfo.Num8MaskedTiles, 4);
    else
        tiles = bmp256_create(8, 8 * EpisodeInfo.Num8MaskedTiles, 8);

    /* Create a 1bpp bitmap for each plane */
    for (p = 0; p < 5; p++)
        planes[p] = bmp256_create(8, 8, 1);

    if (EgaGraph[EpisodeInfo.Index8MaskedTiles].data) {
        for (i = 0; i < EpisodeInfo.Num8MaskedTiles; i++) {
            /* Show that something is happening */
            showprogress((i * 100) / EpisodeInfo.Num8MaskedTiles);

            /* Decode the mask data */
            pointer = EgaGraph[EpisodeInfo.Index8MaskedTiles].data + (i * 5 * 8);

            /* Decode the mask and color plane data */
            for (p = 0; p < 5; p++) {
                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.Index8MaskedTiles].data + (i * 5 * 8) + ((p + 1) % 5) * 8;
                for (y = 0; y < 8; y++)
                    memcpy(planes[p]->lines[y], pointer + y, 1);
            }

            if (Switches->SeparateMask) {
                bmp256_blit(planes[4], 0, 0, tiles, 8, 8 * i, 8, 8);
                bmp = bmp256_merge_ex(planes, 4, 4);
            } else {
                bmp = bmp256_merge_ex(planes, 5, 8);
            }

            bmp256_blit(bmp, 0, 0, tiles, 0, 8 * i, 8, 8);
            bmp256_free(bmp);
            //do_output("\x8\x8\x8\x8");
        }
        completemsg();
    }

    /* Create the bitmap file */
    sprintf(filename, "%s/%s_tile8m.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(tiles, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);

    /* Free the memory used */
    for (p = 0; p < 5; p++)
        bmp256_free(planes[p]);
    bmp256_free(tiles);
}

void k456_export_sprites() {
    BITMAP256 *bmp, *spr, *planes[5];
    FILE *f;
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export sprites before initialisation!");

    if (EpisodeInfo.NumSprites == 0)
        return;

    /* Export all the sprites */
    do_output("Exporting sprites: ");

    /* Open a text file for the clipping and origin info */
    sprintf(filename, "%s/%s_sprites.txt", Switches->OutputPath, EpisodeInfo.GameExt);
    f = openfile(filename, "w", Switches->Backup);

    for (i = 0; i < EpisodeInfo.NumSprites; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumSprites);

        if (EgaGraph[EpisodeInfo.IndexSprites + i].data) {
            /* Output the collision rectangle and origin information */
            fprintf(f, "%d: [%d, %d, %d, %d], [%d, %d], %d\n", i, (SprHead[i].Rx1 - SprHead[i].OrgX) >> 4,
                    (SprHead[i].Ry1 - SprHead[i].OrgY) >> 4, (SprHead[i].Rx2 - SprHead[i].OrgX) >> 4,
                    (SprHead[i].Ry2 - SprHead[i].OrgY) >> 4, SprHead[i].OrgX >> 4, SprHead[i].OrgY >> 4,
                    SprHead[i].Shifts);

            /* Now create the sprite bitmap */
            if (Switches->SeparateMask)
                spr = bmp256_create(SprHead[i].Width * 8 * 3, SprHead[i].Height, 4);
            else
                spr = bmp256_create(SprHead[i].Width * 8 * 2, SprHead[i].Height, 8);

            if (!spr)
                quit("Couldn't create bitmap for sprite %i!\n");

            /* Decode the sprite mask and color plane data */
            for (p = 0; p < 5; p++) {
                /* Create a 1bpp bitmap for each plane */
                planes[p] = bmp256_create(SprHead[i].Width * 8, SprHead[i].Height, 1);

                /* Decode the lines of the bitmap data */
                pointer = EgaGraph[EpisodeInfo.IndexSprites + i].data + ((p + 1) % 5) * SprHead[i].Width * SprHead[i].Height;
                for (y = 0; y < SprHead[i].Height; y++)
                    memcpy(planes[p]->lines[y], pointer + y * SprHead[i].Width, SprHead[i].Width);
            }

            /* Draw the Color planes and mask */
            if (Switches->SeparateMask) {
                bmp256_blit(planes[4], 0, 0, spr, SprHead[i].Width * 8, 0, SprHead[i].Width * 8, SprHead[i].Height);
                bmp = bmp256_merge_ex(&planes[0], 4, 4);
            } else {
                bmp = bmp256_merge_ex(planes, 5, 8);
            }

            bmp256_blit(bmp, 0, 0, spr, 0, 0, SprHead[i].Width * 8, SprHead[i].Height);

            /* Draw the collision rectangle */
            bmp256_rect(spr, spr->width - (SprHead[i].Width * 8), 0,
                    spr->width - 1, spr->height - 1, 8);
            bmp256_rect(spr,
                    spr->width - (SprHead[i].Width * 8) + ((SprHead[i].Rx1 - SprHead[i].OrgX) >> 4),
                    ((SprHead[i].Ry1 - SprHead[i].OrgY) >> 4),
                    spr->width - (SprHead[i].Width * 8) + ((SprHead[i].Rx2 - SprHead[i].OrgX) >> 4),
                    ((SprHead[i].Ry2 - SprHead[i].OrgY) >> 4), 12);

            /* Create the bitmap file */
            sprintf(filename, "%s/%s_sprite_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
            if (!bmp256_save(spr, filename, Switches->Backup))
                quit("Can't open bitmap file %s!", filename);

            /* Free the memory used */
            for (p = 0; p < 5; p++)
                bmp256_free(planes[p]);
            bmp256_free(bmp);
            bmp256_free(spr);

        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
    fclose(f);
}

void k456_export_texts() {
    char filename[PATH_MAX];
    FILE *f;
    int i;

    if (!ExportInitialised)
        quit("Trying to export texts before initialisation!");

    if (EpisodeInfo.NumTexts == 0)
        return;

    /* Export all the texts */
    do_output("Exporting texts: ");

    for (i = 0; i < EpisodeInfo.NumTexts; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumTexts);

        if (EgaGraph[EpisodeInfo.IndexTexts + i].data) {
            /* Create the text file */
            sprintf(filename, "%s/%s_txt_%04d.txt", Switches->OutputPath, EpisodeInfo.GameExt, i);
            f = openfile(filename, "wb", Switches->Backup);
            if (!f)
                quit("Can't open text file %s!", filename);
            fwrite(EgaGraph[EpisodeInfo.IndexTexts + i].data, EgaGraph[EpisodeInfo.IndexTexts + i].len, 1, f);
            fclose(f);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_export_misc() {

    MiscInfoList *mp;
    FILE *f;
    char filename[PATH_MAX];

    if (!ExportInitialised)
        quit("Trying to export misc chunks before initialisation!");

    do_output("Exporting misc chunks: ");

    /* Search misc chunk list for a terminator text chunk */
    for (mp = MiscInfos; mp; mp = mp->next) {
        if (strcmp(mp->Type, "MISC"))
            continue;

        if (EgaGraph[mp->Chunk].data) {
            /* Create the text file */
            sprintf(filename, "%s/%s_misc_%s.bin", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
            f = openfile(filename, "wb", Switches->Backup);
            if (!f)
                quit("Can't open file %s!", filename);
            fwrite(EgaGraph[mp->Chunk].data, EgaGraph[mp->Chunk].len, 1, f);
            fclose(f);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_export_demos() {
    char filename[PATH_MAX];
    FILE *f;
    int i;

    if (!ExportInitialised)
        quit("Trying to export demos before initialisation!");

    /* Export all the demos */
    do_output("Exporting demos: ");

    for (i = 0; i < EpisodeInfo.NumDemos; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumDemos);

        if (EgaGraph[EpisodeInfo.IndexDemos + i].data) {
            /* Create the demo file */
            sprintf(filename, "%s/demo%d.%s", Switches->OutputPath, i, EpisodeInfo.GameExt);
            f = openfile(filename, "wb", Switches->Backup);
            if (!f)
                quit("Can't open file %s!", filename);
            fwrite(EgaGraph[EpisodeInfo.IndexDemos + i].data, EgaGraph[EpisodeInfo.IndexDemos + i].len, 1, f);
            fclose(f);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_export_fonts() {
    BITMAP256 *font, *bmp;
    FontHeadStruct *FontHead;
    char filename[PATH_MAX];
    int i, j, w, bw, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export fonts before initialisation!");

    /* Export all the fonts into separate bitmaps*/
    do_output("Exporting fonts: ");

    for (i = 0; i < EpisodeInfo.NumFonts; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumFonts);

        if (EgaGraph[EpisodeInfo.IndexFonts + i].data) {
            FontHead = (FontHeadStruct *) EgaGraph[EpisodeInfo.IndexFonts + i].data;

            /* Find out the maximum character width */
            w = 0;
            for (j = 0; j < 256; j++)
                if (FontHead->Width[j] > w)
                    w = FontHead->Width[j];

            font = bmp256_create(w * 16, FontHead->Height * 16, 4);

            /* Create a 1bpp bitmap for the character */
            bmp = bmp256_create(w, FontHead->Height, 1);

            /* Now decode the characters */
            pointer = EgaGraph[EpisodeInfo.IndexFonts + i].data;
            for (j = 0; j < 256; j++) {
                /* Get the width of the character in bytes */
                bw = (FontHead->Width[j] + 7) / 8;

                /* Clear the bitmap */
                bmp256_rect(bmp, 0, 0, bmp->width - 1, bmp->height - 1, 8);

                /* Decode the lines of the character data */
                if (FontHead->Width[j] > 0)
                    for (y = 0; y < FontHead->Height; y++)
                        memcpy(bmp->lines[y], pointer + FontHead->Offset[j] + (y * bw), bw);

                /* Copy the character into the grid */
                bmp256_blit(bmp, 0, 0, font, (j % 16) * w, (j / 16) * FontHead->Height, w, FontHead->Height);

                /* Fill the remainder of the bitmap with Grey */
                bmp256_rect(font, (j % 16) * w + FontHead->Width[j], (j / 16) * FontHead->Height,
                        (j % 16) * w + w - 1, (j / 16) * FontHead->Height + FontHead->Height - 1, 8);
            }

            /* Create the bitmap file */
            sprintf(filename, "%s/%s_fon_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
            if (!bmp256_save(font, filename, Switches->Backup))
                quit("Can't open bitmap file %s!", filename);

            /* Free the memory used */
            bmp256_free(font);
            bmp256_free(bmp);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

/*
 Export full screen ANSI art displayed at the dos prompt
 */


// The seven header bytes don't seem to control anything, 

typedef struct {
    uint8_t byte1; // 0xFD ????
    uint16_t seg; // B800 dest seg
    uint32_t len; // 0x00000FA0 length of ANSI Arrt (80x25x2)
    uint8_t data[];
} ANSIStruct;

void k456_export_ansi() {

    MiscInfoList *mp;
    FILE *f;
    char filename[PATH_MAX];

    if (!ExportInitialised)
        quit("Trying to export ANSI art screens before initialisation!");

    do_output("Exporting ANSI art screens: ");

    /* Search misc chunk list for a terminator text chunk */
    for (mp = MiscInfos; mp; mp = mp->next) {
        if (strcmp(mp->Type, "B800TEXT"))
            continue;

        if (EgaGraph[mp->Chunk].data) {
            /* Create the text file */
            sprintf(filename, "%s/%s_ansi_%s.bin", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
            f = openfile(filename, "wb", Switches->Backup);
            if (!f)
                quit("Can't open file %s!", filename);
            fwrite(EgaGraph[mp->Chunk].data, EgaGraph[mp->Chunk].len, 1, f);
            fclose(f);
        }
        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

/* 
 * Export the scrolling Terminator text as a monochrome bitmap
 */
void k456_export_terminator_text() {

    MiscInfoList *mp;
    int x, y, color;
    uint16_t *rleptr;
    BITMAP256 *bmp;
    char filename[PATH_MAX];
    TerminatorHeadStruct *TerminatorHead;

    if (!ExportInitialised)
        quit("Trying to export terminator text before initialisation!");

    do_output("Exporting terminator text: ");

    /* Search misc chunk list for a terminator text chunk */
    for (mp = MiscInfos; mp; mp = mp->next) {
        if (strcmp(mp->Type, "TERMINATOR"))
            continue;

        /* Get the height and width of the bitmap */
        if (EgaGraph[mp->Chunk].data) {
            TerminatorHead = (TerminatorHeadStruct *) EgaGraph[mp->Chunk].data;

            /* Create a 1bpp bitmap */
            bmp = bmp256_create(TerminatorHead->Width, TerminatorHead->Height, 1);

            /* Decode RLE image one line at a time */
            for (y = 0; y < TerminatorHead->Height; y++) {
                rleptr = (uint16_t*) (EgaGraph[mp->Chunk].data + TerminatorHead->LineStarts[y]);
                x = 0;
                color = 0;

                while (*rleptr != 0xFFFF) {
                    int p;

                    /* Write the run of pixels */
                    for (p = 0; p < *rleptr; p++) {
                        bmp256_putpixel(bmp, x + p, y, color);
                    }

                    /* Update color and x position */
                    x += *(rleptr++);
                    color ^= 1;

                }
            }
        }

        /* Create the bitmap file */
        sprintf(filename, "%s/%s_terminator_%s.bmp", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
        if (!bmp256_save(bmp, filename, Switches->Backup))
            quit("Can't open bitmap file %s!", filename);
        bmp256_free(bmp);
    }
    //do_output("\x8\x8\x8\x8");
    completemsg();

}

void do_k456_export(SwitchStruct *switches) {
    k456_export_begin(switches);
    k456_export_fonts();
    k456_export_bitmaps();
    k456_export_masked_bitmaps();
    k456_export_8_tiles();
    k456_export_8_masked_tiles();
    k456_export_tiles();
    k456_export_masked_tiles();
    k456_export_sprites();
    k456_export_texts();
    k456_export_terminator_text();
    k456_export_ansi();
    k456_export_demos();
    k456_export_misc();
    k456_export_end();
}


/************************************************************************************************************/
/** KEEN 4, 5, 6 IMPORTING ROUTINES *************************************************************************/

/************************************************************************************************************/

void k456_import_begin(SwitchStruct *switches) {
    char filename[PATH_MAX];
    FILE *exefile, *dictfile, *filetoread;
    uint32_t offset;
	 	unsigned long	exeimglen, exeheaderlen;
    int i;

    /* Never allow the import start to occur more than once */
    if (ImportInitialised)
        quit("Tried to initialise Keen 4 files a second time!");

    /* Save the switches */
    Switches = switches;

    /* Read Game archive data */
    exefile = dictfile = filetoread = NULL;

    /* Check for EGADICT */
    sprintf(filename, "%s/egadict.%s", Switches->InputPath, EpisodeInfo.GameExt);
    dictfile = fopen(filename, "rb");

    /* If it is not found externally, then check in the exe */
    if (!dictfile) {
        /* Open the EXE */
        sprintf(filename, "%s/%s", Switches->InputPath, EpisodeInfo.ExeName);
        exefile = fopen(filename, "rb");
        if (!exefile)
            quit("Can't open %s for reading!", filename);

        // Due to my modification to get_exe_image_size(), I MUST initialize exeheaderlen with 0
        // or random data might be extracted from it, which screws up the resultant exeimglen value.
        // TODO: bug with this
        exeheaderlen = 0;

        /* Check that it's the right version (and it's unpacked) */
        if (!get_exe_image_size(exefile, &exeimglen, &exeheaderlen))
            quit("%s is not a valid exe file!", filename);
        if (DebugMode) {
            do_output("Exe Image Length is: 0x%08lX\n", exeimglen);
            do_output("Header length is: 0x%08lX\n", exeheaderlen);
        }
        if (exeimglen != EpisodeInfo.ExeImageSize)
            quit("Incorrect .exe image length for %s.  (Expected %X, found %X). "
                " Check your game version!\n", filename, EpisodeInfo.ExeImageSize,
                exeimglen);
        if (exeimglen != EpisodeInfo.ExeImageSize)
            quit("Incorrect .exe header length for %s.  (Expected %X, found %X). "
                " Check your game version!\n", filename, EpisodeInfo.ExeHeaderSize,
                exeheaderlen);
    }

    /* Get the EGADICT data*/
    if (dictfile) {
        filetoread = dictfile;
        offset = 0;
    } else {
        setcol_warning;
        do_output("Cannot find %s, Extracting EGADICT from the exe.\n", filename);
        setcol_normal;
        filetoread = exefile;
        offset = exeheaderlen + EpisodeInfo.OffEgaDict;
    }
    huff_read_dictionary(filetoread, offset);
    huff_setup_compression();

    /* Close the files */
    if (dictfile)
        fclose(dictfile);
    if (exefile)
        fclose(exefile);

    /* Initialise the EGAGRAPH */
    EgaGraph = (ChunkStruct *) malloc(EpisodeInfo.NumChunks * sizeof (ChunkStruct));
    if (!EgaGraph)
        quit("Not enough memory to initialise EGAGRAPH!");
    for (i = 0; i < EpisodeInfo.NumChunks; i++) {
        EgaGraph[i].data = NULL;
        EgaGraph[i].len = 0;
    }

    /* Set up pointers to bitmap and sprite tables */
    BmpHead = (BitmapHeadStruct *) malloc(sizeof (BitmapHeadStruct) * EpisodeInfo.NumBitmaps);
    BmpMaskedHead = (BitmapHeadStruct *) malloc(sizeof (BitmapHeadStruct) * EpisodeInfo.NumMaskedBitmaps);
    SprHead = (SpriteHeadStruct *) malloc(sizeof (SpriteHeadStruct) * EpisodeInfo.NumSprites);
    if (!BmpHead || !BmpMaskedHead || !SprHead)
        quit("Not enough memory to initialise structures!");

    if (EpisodeInfo.NumBitmaps > 0) {
        EgaGraph[EpisodeInfo.IndexBitmapTable].data = (uint8_t *) BmpHead;
        EgaGraph[EpisodeInfo.IndexBitmapTable].len = sizeof (BitmapHeadStruct) * EpisodeInfo.NumBitmaps;
    }
    if (EpisodeInfo.NumMaskedBitmaps > 0) {
        EgaGraph[EpisodeInfo.IndexMaskedBitmapTable].data = (uint8_t *) BmpMaskedHead;
        EgaGraph[EpisodeInfo.IndexMaskedBitmapTable].len = sizeof (BitmapHeadStruct) * EpisodeInfo.NumMaskedBitmaps;
    }
    if (EpisodeInfo.NumSprites > 0) {
        EgaGraph[EpisodeInfo.IndexSpriteTable].data = (uint8_t *) SprHead;
        EgaGraph[EpisodeInfo.IndexSpriteTable].len = sizeof (SpriteHeadStruct) * EpisodeInfo.NumSprites;
    }

    ImportInitialised = 1;
}

void k456_import_end() {
    char filename[PATH_MAX];
    int i;
    uint8_t *compdata;
    FILE *headfile, *graphfile, *patchfile;
    uint32_t offset, len, grstart_mask, ptr;

    if (!ImportInitialised)
        quit("Tried to end import without beginning it!");

    /* Get the GrStart width */

    if (EpisodeInfo.GrStarts != 3 && EpisodeInfo.GrStarts != 4)
        quit("GrStarts must be 3 or 4! (Set to %i)", EpisodeInfo.GrStarts);

    grstart_mask = 0xFFFFFFFF >> (8 * (4 - EpisodeInfo.GrStarts));

    /* Open the EGAHEAD and EGAGRAPH files for writing */
    sprintf(filename, "%s/egahead.%s", Switches->InputPath, EpisodeInfo.GameExt);
    headfile = openfile(filename, "wb", Switches->Backup);
    if (!headfile)
        quit("Unable to open %s for writing!", filename);

    if (strcmp(EpisodeInfo.EgaGraphName, ""))
        sprintf(filename, "%s/%s", Switches->InputPath, EpisodeInfo.EgaGraphName);
    else
        sprintf(filename, "%s/egagraph.%s", Switches->InputPath, EpisodeInfo.GameExt);
    graphfile = openfile(filename, "wb", Switches->Backup);
    if (!graphfile)
        quit("Unable to open %s for writing!", filename);


    /* Compress data and output the EGAHEAD and EGAGRAPH */
    do_output("Compressing: ");
    offset = 0;
    for (i = 0; i < EpisodeInfo.NumChunks; i++) {
        /* Show that something is happening */
        showprogress((int) ((i * 100) / EpisodeInfo.NumChunks));

        if (EgaGraph[i].data && EgaGraph[i].len > 0) {
            /* Save the current offset */
            ptr = offset;

            /* Give some extra room for compressed data (as it's occasionally larger) */
            compdata = malloc(EgaGraph[i].len * 2);
            if (!compdata)
                quit("Not enough memory for compression buffer!");

            len = huff_compress(EgaGraph[i].data, compdata, EgaGraph[i].len, EgaGraph[i].len * 2);

            /* If the chunk is not a tile chunk then we need to output the length first */
            if (i < EpisodeInfo.Index8Tiles || i >= EpisodeInfo.Index32MaskedTiles +
                    EpisodeInfo.Num32MaskedTiles) {
                fwrite(&EgaGraph[i].len, sizeof (uint32_t), 1, graphfile);
                offset += sizeof (uint32_t);
            }
            fwrite(compdata, len, 1, graphfile);
            free(compdata);

            /* Calculate the next offset (taking t*/
            offset += len;
        } else {
            // The same for all Keens according to KDR edition source
            // Uncanny, isn't it?
            ptr = grstart_mask;
        }
        fwrite(&ptr, EpisodeInfo.GrStarts, 1, headfile);

        //do_output("\x8\x8\x8\x8");
    }

    /* Write the final header entry, which is where the n+1'th chunk would start */
    ptr = offset;
    fwrite(&ptr, EpisodeInfo.GrStarts, 1, headfile);

		/*
    if (ptr != ftell(graphfile) - 1)
	    quit("EGAGRAPH improperly sized!");
			*/

    completemsg();

    /* Close files */
    fclose(headfile);
    fclose(graphfile);

    /* Free the memory used */
    free(EgaGraph);
    free(BmpHead);
    free(BmpMaskedHead);
    free(SprHead);

    /* Create the Patch File */
    if (Switches->Patch) {
        sprintf(filename, "%s/%s.pat", Switches->InputPath, EpisodeInfo.GameExt);
        patchfile = openfile(filename, "w", Switches->Backup);

        fprintf(patchfile, "%%ext %s\n"
                "%%version 1.4\n" // TODO: Put this in definition file and episode info
                "# Load the modified graphics\n"
                "%%egahead egahead.%s\n"
                "%%end\n", EpisodeInfo.GameExt, EpisodeInfo.GameExt);
        fclose(patchfile);
    }
}

void k456_import_bitmaps() {
    BITMAP256 *bmp, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import bitmaps before initialisation!");

    if (EpisodeInfo.NumBitmaps == 0)
        return;

    /* Import all the bitmaps */
    do_output("Importing bitmaps: ");

    for (i = 0; i < EpisodeInfo.NumBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumBitmaps);

        /* Open the bitmap file */
        sprintf(filename, "%s/%s_pic_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        bmp = bmp256_load(filename);
        if (!bmp)
            quit("Can't open bitmap file %s!", filename);
        if (bmp->width % 8 != 0)
            quit("Bitmap %s is not a multiple of 8 pixels wide!", filename);
        if (bmp->bpp != 4 && bmp->bpp != 8)
            quit("Bitmap %s has neither 16 nor 256 colors!", filename);

        /* Decode the bmp file */
        if (!bmp256_split_ex(bmp, planes, 0, 4))
            quit("Not enough memory to import bitmap %s!", filename);

        /* Set up the BmpHead structures */
        BmpHead[i].Width = bmp->width / 8;
        BmpHead[i].Height = bmp->height;

        /* Allocate memory for the data */
        EgaGraph[EpisodeInfo.IndexBitmaps + i].len = BmpHead[i].Width * BmpHead[i].Height * 4;
        pointer = malloc(BmpHead[i].Width * BmpHead[i].Height * 4);
        if (!pointer)
            quit("Not enough memory for bitmap %d!", i);
        EgaGraph[EpisodeInfo.IndexBitmaps + i].data = pointer;

        /* Decode the bitmap data */
        for (p = 0; p < 4; p++) {
            /* Decode the lines of the bitmap data */
            pointer = EgaGraph[EpisodeInfo.IndexBitmaps + i].data + p * BmpHead[i].Width * BmpHead[i].Height;
            for (y = 0; y < BmpHead[i].Height; y++)
                memcpy(pointer + y * BmpHead[i].Width, planes[p]->lines[y], BmpHead[i].Width);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(bmp);

        //do_output("\x8\x8\x8\x8");
    }

    completemsg();
}

void k456_import_masked_bitmaps() {
    BITMAP256 *bmp, *mbmp, *planes[6];
    char filename[PATH_MAX];
    int i, p, y;
    unsigned granularity;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import masked bitmaps before initialisation!");

    if (EpisodeInfo.NumMaskedBitmaps == 0)
        return;

    /* Import all the masked bitmaps */
    do_output("Importing masked bitmaps: ");

    granularity = Switches->SeparateMask ? 2 : 1;

    for (i = 0; i < EpisodeInfo.NumMaskedBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumMaskedBitmaps);

        /* Open the bitmap file and validate it */
        sprintf(filename, "%s/%s_picm_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        mbmp = bmp256_load(filename);
        if (!mbmp)
            quit("Can't open bitmap file %s!", filename);
        if (mbmp->width % (8 * granularity) != 0)
            quit("Masked bitmap %s is not a multiple of %d pixels wide!",
                filename, granularity * 8);
        if (!Switches->SeparateMask && mbmp->bpp != 8)
            quit("Masked bitmap %s is not a 256-color bitmap!", filename);

        if (Switches->SeparateMask) {
            /* Get the mask */
            planes[0] = bmp256_create(mbmp->width / 2, mbmp->height, 1);
            if (!planes[0])
                quit("Not enough memory to create bitmap!");
            bmp256_blit(mbmp, planes[0]->width, 0, planes[0], 0, 0, planes[0]->width, planes[0]->height);

            /* Create a copy of the image for splitting */
            bmp = bmp256_create(mbmp->width / granularity, mbmp->height, 4);
            if (!bmp)
                quit("Not enough memory to create bitmap!");
            bmp256_blit(mbmp, 0, 0, bmp, 0, 0, bmp->width, bmp->height);

            /* Split the color data into component planes */
            bmp256_split_ex(bmp, &planes[1], 0, 4);
            bmp256_free(bmp);
        } else {
            /* Split the image into 5 planes */
            bmp256_split_ex(mbmp, &planes[1], 0, 5);
            planes[0] = planes[5];
            planes[5] = NULL;
        }

        /* Set up the BmpHead structures */
        BmpMaskedHead[i].Width = mbmp->width / (granularity * 8);
        BmpMaskedHead[i].Height = mbmp->height;

        /* Allocate memory for the data */
        EgaGraph[EpisodeInfo.IndexMaskedBitmaps + i].len = BmpMaskedHead[i].Width * BmpMaskedHead[i].Height * 5;
        pointer = malloc(BmpMaskedHead[i].Width * BmpMaskedHead[i].Height * 5);
        if (!pointer)
            quit("Not enough memory for bitmap %d!", i);
        EgaGraph[EpisodeInfo.IndexMaskedBitmaps + i].data = pointer;

        /* Encode the mask and color plane data */
        for (p = 0; p < 5; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            pointer = EgaGraph[EpisodeInfo.IndexMaskedBitmaps + i].data + p * BmpMaskedHead[i].Width * BmpMaskedHead[i].Height;
            for (y = 0; y < BmpMaskedHead[i].Height; y++)
                memcpy(pointer + y * BmpMaskedHead[i].Width, planes[p]->lines[y], BmpMaskedHead[i].Width);
        }

        /* Free the memory used */
        for (p = 0; p < 5; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(mbmp);

        //do_output("\x8\x8\x8\x8");
    }

    completemsg();
}

void k456_import_tiles() {
    BITMAP256 *bmp, *tile, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import tiles before initialisation!");

    /* Import all the tiles */
    do_output("Importing tiles: ");

    /* Open the bitmap file */
    sprintf(filename, "%s/%s_tile16.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    bmp = bmp256_load(filename);
    if (!bmp)
        quit("Can't open bitmap file %s!", filename);
    if (bmp->width != 18 * 16)
        quit("Tile bitmap %s is not 288 pixels wide!", filename);
    if (bmp->bpp != 4 && bmp->bpp != 8)
        quit("Tile bitmap %s is neither 16 nor 256 colors!", filename);

    for (i = 0; i < EpisodeInfo.Num16Tiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num16Tiles);

        /* Extract the tile we want */
        tile = bmp256_create(16, 16, 4);
        if (!tile)
            quit("Not enough memory to create bitmap!");
        bmp256_blit(bmp, (i % 18) * 16, (i / 18) * 16, tile, 0, 0, 16, 16);

        /* Decode the tile */
        bmp256_split_ex(tile, planes, 0, 4);

        /* Allocate memory for the data */
        EgaGraph[EpisodeInfo.Index16Tiles + i].len = 2 * 16 * 4;
        pointer = malloc(2 * 16 * 4);
        if (!pointer)
            quit("Not enough memory for tile %d!", i);
        EgaGraph[EpisodeInfo.Index16Tiles + i].data = pointer;

        /* Encode the bitmap data */
        for (p = 0; p < 4; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            /* Encode the lines of the image data */
            pointer = EgaGraph[EpisodeInfo.Index16Tiles + i].data + p * 2 * 16;
            for (y = 0; y < 16; y++)
                memcpy(pointer + y * 2, planes[p]->lines[y], 2);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(tile);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    bmp256_free(bmp);
}

void k456_import_masked_tiles() {
    BITMAP256 *bmp, *tile, *planes[6];
    char filename[PATH_MAX];
    int i, p, y;
    unsigned granularity;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import masked tiles before initialisation!");

    /* Import all the masked tiles */
    do_output("Importing masked tiles: ");

    granularity = Switches->SeparateMask ? 2 : 1;

    /* Open the bitmap file */
    sprintf(filename, "%s/%s_tile16m.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    bmp = bmp256_load(filename);
    if (!bmp)
        quit("Can't open bitmap file %s!", filename);
    if (bmp->width != 18 * 16 * granularity)
        quit("Masked tile bitmap %s is not %d pixels wide!", filename,
            granularity * 18 * 16);
    if (!Switches->SeparateMask && bmp->bpp != 8)
        quit("Masked tile bitmap %s is not a 256-color bitmap!", filename);

    /* Skip the first tile, as it should always be transparent */
    for (i = 1; i < EpisodeInfo.Num16MaskedTiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num16MaskedTiles);

        /* Extract the color data of the tile we want */
        tile = bmp256_create(16, 16, 8);
        if (!tile)
            quit("Not enough memory to create bitmap!");
        bmp256_blit(bmp, (i % 18) * 16, (i / 18) * 16, tile, 0, 0, 16, 16);


        /* Extract the tile mask */
        if (Switches->SeparateMask) {
            /* Get the color planes */
            if (!bmp256_split_ex(tile, &planes[1], 0, 4))
                quit("Not enough memory to create bitmap!");

            /* Get the mask */
            planes[0] = bmp256_create(16, 16, 1);
            if (!planes[0])
                quit("Not enough memory to create bitmap!");
            bmp256_blit(bmp, 16 * 18 + (i % 18) * 16, (i / 18) * 16, planes[0], 0, 0, 16, 16);
        } else {
            /* Get mask and color plane, and shuffle planes into order */
            if (!bmp256_split_ex(tile, &planes[1], 0, 5))
                quit("Not enough memory to create bitmap!");
            planes[0] = planes[5];
            planes[5] = NULL;
        }

        /* Allocate memory for the data */
        EgaGraph[EpisodeInfo.Index16MaskedTiles + i].len = 2 * 16 * 5;
        pointer = malloc(2 * 16 * 5);
        if (!pointer)
            quit("Not enough memory for masked tile %d!", i);
        EgaGraph[EpisodeInfo.Index16MaskedTiles + i].data = pointer;

        /* Encode the bitmap data */
        for (p = 0; p < 5; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            /* Encode the lines of the image data */
            pointer = EgaGraph[EpisodeInfo.Index16MaskedTiles + i].data + p * 2 * 16;
            for (y = 0; y < 16; y++)
                memcpy(pointer + y * 2, planes[p]->lines[y], 2);
        }

        /* Free the memory used */
        for (p = 0; p < 5; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(tile);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    bmp256_free(bmp);
}

void k456_import_8_tiles() {
    BITMAP256 *bmp, *tile, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import 8x8 tiles before initialisation!");

    /* Import all the 8x8 tiles */
    do_output("Importing 8x8 tiles: ");

    /* Open the bitmap file */
    sprintf(filename, "%s/%s_tile8.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    bmp = bmp256_load(filename);
    if (!bmp)
        quit("Can't open bitmap file %s!", filename);
    if (bmp->width != 8)
        quit("8x8 Tile bitmap %s is not 8 pixels wide!", filename);

    /* Allocate memory for the all the tiles */
    EgaGraph[EpisodeInfo.Index8Tiles].len = EpisodeInfo.Num8Tiles * 8 * 4;
    pointer = malloc(EpisodeInfo.Num8Tiles * 8 * 4);
    if (!pointer)
        quit("Not enough memory for 8x8 tiles!");
    EgaGraph[EpisodeInfo.Index8Tiles].data = pointer;


    for (i = 0; i < EpisodeInfo.Num8Tiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num8Tiles);

        /* Extract the tile we want */
        tile = bmp256_create(8, 8, 4);
        if (!tile)
            quit("Not enough memory to create bitmap!");
        bmp256_blit(bmp, 0, i * 8, tile, 0, 0, 8, 8);

        /* Decode the tile */
        bmp256_split_ex(tile, planes, 0, 4);


        /* Decode the bitmap data */
        for (p = 0; p < 4; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            /* Decode the lines of the image data */
            pointer = EgaGraph[EpisodeInfo.Index8Tiles].data + i * 8 * 4 + p * 8;
            for (y = 0; y < 8; y++)
                memcpy(pointer + y, planes[p]->lines[y], 1);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(tile);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    bmp256_free(bmp);
}

void k456_import_8_masked_tiles() {
    BITMAP256 *bmp, *tile, *planes[6];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;
    unsigned granularity;

    if (!ImportInitialised)
        quit("Trying to import 8x8 masked tiles before initialisation!");

    /* Import all the 8x8 masked tiles */
    do_output("Importing 8x8 masked tiles: ");

    granularity = Switches->SeparateMask ? 2 : 1;

    /* Open the bitmap file */
    sprintf(filename, "%s/%s_tile8m.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    bmp = bmp256_load(filename);
    if (!bmp)
        quit("Can't open bitmap file %s!", filename);
    if (bmp->width != 8 * granularity)
        quit("Masked 8x8 tile bitmap %s is not %d pixels wide!", filename,
            granularity * 8);
    if (!Switches->SeparateMask && bmp->bpp != 8)
        quit("Masked 8x8 tile bitmap %s is not a 256-color bitmap!", filename);


    /* Allocate memory for the data */
    EgaGraph[EpisodeInfo.Index8MaskedTiles].len = EpisodeInfo.Num8MaskedTiles * 8 * 5;
    pointer = malloc(EpisodeInfo.Num8MaskedTiles * 8 * 5);
    if (!pointer)
        quit("Not enough memory for 8x8 masked tiles!");
    EgaGraph[EpisodeInfo.Index8MaskedTiles].data = pointer;


    for (i = 0; i < EpisodeInfo.Num8MaskedTiles; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.Num8MaskedTiles);

        /* Extract the color planes from the tile we want */
        tile = bmp256_create(8, 8, 8);
        if (!tile)
            quit("Not enough memory to create bitmap!");
        bmp256_blit(bmp, 0, i * 8, tile, 0, 0, 8, 8);


        /* Extract the tile mask*/
        if (Switches->SeparateMask) {
            /* Grab the color info */
            bmp256_split_ex(tile, &planes[1], 0, 4);

            /* Copy the mask from the right half of the bitmap */
            planes[0] = bmp256_create(8, 8, 1);
            if (!planes[0])
                quit("Not enough memory to create bitmap!");
            bmp256_blit(bmp, 8, i * 8, planes[0], 0, 0, 8, 8);
        } else {
            /* Grab all planes and move them into the correct order */
            bmp256_split_ex(tile, &planes[1], 0, 5);
            planes[0] = planes[5];
            planes[5] = NULL;
        }


        /* Encode the bitmap data */
        for (p = 0; p < 5; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            /* Encode the lines of the image data */
            pointer = EgaGraph[EpisodeInfo.Index8MaskedTiles].data + (i * 8 * 5) + p * 8;
            for (y = 0; y < 8; y++)
                memcpy(pointer + y, planes[p]->lines[y], 1);
        }

        /* Free the memory used */
        for (p = 0; p < 5; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(tile);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    bmp256_free(bmp);
}

void k456_import_texts() {
    char filename[PATH_MAX];
    int i;
    FILE *f;
    unsigned long len;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import texts before initialisation!");

    /* Import all the texts */
    do_output("Importing texts chunks: ");

    for (i = 0; i < EpisodeInfo.NumTexts; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumTexts);

        /* Open the demo file */
        sprintf(filename, "%s/%s_txt_%04d.txt", Switches->OutputPath, EpisodeInfo.GameExt, i);
        f = openfile(filename, "rb", Switches->Backup);
        if (!f)
            quit("Can't open %s!", filename);

        /* Get the file length */
        fseek(f, 1, SEEK_END);
        len = ftell(f) - 1;
        fseek(f, 0, SEEK_SET);

        /* Allocate memory */
        pointer = malloc(len);
        if (!pointer)
            quit("Not enough memory to import misc chunks!");
        EgaGraph[EpisodeInfo.IndexTexts + i].len = len;
        EgaGraph[EpisodeInfo.IndexTexts + i].data = pointer;

        /* Read the data and close the file */
        fread(pointer, len, 1, f);
        fclose(f);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_import_misc() {
    char filename[PATH_MAX];
    FILE *f;
    unsigned long len;
    uint8_t *pointer;
    MiscInfoList *mp;

    if (!ImportInitialised)
        quit("Trying to import misc chunks before initialisation!");

    /* Import all the misc chunks */
    do_output("Importing misc chunks: ");

    for (mp = MiscInfos; mp; mp = mp->next) {

        if (strcmp(mp->Type, "MISC"))
            continue;

        /* Open the demo file */
        sprintf(filename, "%s/%s_misc_%s.bin", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
        f = openfile(filename, "rb", Switches->Backup);
        if (!f)
            quit("Can't open %s!", filename);

        /* Get the file length */
        fseek(f, 1, SEEK_END);
        len = ftell(f) - 1;
        fseek(f, 0, SEEK_SET);

        /* Allocate memory */
        pointer = malloc(len);
        if (!pointer)
            quit("Not enough memory to import misc chunks!");
        EgaGraph[mp->Chunk].len = len;
        EgaGraph[mp->Chunk].data = pointer;

        /* Read the data and close the file */
        fread(pointer, len, 1, f);
        fclose(f);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_import_demos() {
    char filename[PATH_MAX];
    int i;
    FILE *f;
    unsigned long len;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import demos before initialisation!");

    /* Import all the demos */
    do_output("Importing demos: ");

    for (i = 0; i < EpisodeInfo.NumDemos; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumDemos);

        /* Open the demo file */
        sprintf(filename, "%s/demo%d.%s", Switches->OutputPath, i, EpisodeInfo.GameExt);
        f = openfile(filename, "rb", Switches->Backup);
        if (!f)
            quit("Can't open %s!", filename);

        /* Get the file length */
        fseek(f, 1, SEEK_END);
        len = ftell(f) - 1;
        fseek(f, 0, SEEK_SET);

        /* Allocate memory */
        pointer = malloc(len);
        if (!pointer)
            quit("Not enough memory to import demos!");
        EgaGraph[EpisodeInfo.IndexDemos + i].len = len;
        EgaGraph[EpisodeInfo.IndexDemos + i].data = pointer;

        /* Read the data and close the file */
        fread(pointer, len, 1, f);
        fclose(f);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_import_fonts() {
    BITMAP256 *font, *bmp;
    FontHeadStruct FontHead;
    char filename[PATH_MAX];
    int i, j, mw, w, bw, y, x;
    uint8_t *pointer;
    unsigned long offset;

    if (!ImportInitialised)
        quit("Trying to import fonts before initialisation!");

    /* Import all the fonts from separate bitmaps*/
    do_output("Importing fonts: ");

    for (i = 0; i < EpisodeInfo.NumFonts; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumFonts);

        /* Open the bitmap */
        sprintf(filename, "%s/%s_fon_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        font = bmp256_load(filename);
        if (!font)
            quit("Can't open bitmap file %s!", filename);
        if (font->width % 16 != 0)
            quit("Font bitmap %s is not a multiple of 16 pixels wide!", filename);

        /* Calculate the height and width of the cells */
        mw = font->width / 16;
        FontHead.Height = font->height / 16;

        /* Get the size and offsets of the characters */
        offset = sizeof (FontHeadStruct);
        for (j = 0; j < 256; j++) {
            /* Get the width of the character in pixels */
            for (x = 0; x < mw; x++)
                if (bmp256_getpixel(font, mw * (j % 16) + x, FontHead.Height * (j / 16)) == 8)
                    break;
            w = x;
            FontHead.Width[j] = w;

            /* Get the width of the character in bytes */
            if (FontHead.Width[j] > 0) {
                bw = (FontHead.Width[j] + 7) / 8;
                FontHead.Offset[j] = offset;
                offset += bw * FontHead.Height;
            } else {
                FontHead.Offset[j] = 0;
            }
        }

        /* Allocate memory */
        pointer = malloc(offset);
        if (!pointer)
            quit("Not enough memory to import fonts!");
        EgaGraph[EpisodeInfo.IndexFonts + i].len = offset;
        EgaGraph[EpisodeInfo.IndexFonts + i].data = pointer;

        /* Save the FontHead table */
        memcpy(pointer, &FontHead, sizeof (FontHeadStruct));
        pointer += sizeof (FontHeadStruct);

        /* Export the characters */
        for (j = 0; j < 256; j++) {
            if (FontHead.Width[j] > 0) {
                /* Copy the character into a 1-bit bitmap */
                bmp = bmp256_create(FontHead.Width[j], FontHead.Height, 1);
                if (!bmp)
                    quit("Not enough memory to export font!");
                bmp256_blit(font, mw * (j % 16), FontHead.Height * (j / 16), bmp, 0, 0, FontHead.Width[j], FontHead.Height);

                /* Now encode the lines of the character into the output */
                bw = (FontHead.Width[j] + 7) / 8;
                pointer = EgaGraph[EpisodeInfo.IndexFonts + i].data + FontHead.Offset[j];
                for (y = 0; y < FontHead.Height; y++)
                    memcpy(pointer + y * bw, bmp->lines[y], bw);

                /* And free the bitmap */
                bmp256_free(bmp);
            }
        }

        /* Free memory */
        bmp256_free(font);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void k456_import_sprites() {
    BITMAP256 *bmp, *spr, *planes[6];
    char filename[PATH_MAX];
    FILE *f;
    int i, p, y, j;
    int rx1, ry1, rx2, ry2, ox, oy, sh;
    unsigned granularity;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Trying to import sprites before initialisation!");

    if (EpisodeInfo.NumSprites == 0)
        return;

    /* Import all the sprites  */
    do_output("Importing sprites: ");

    granularity = Switches->SeparateMask ? 3 : 2;

    /* Open a text file for the clipping and origin info */
    sprintf(filename, "%s/%s_sprites.txt", Switches->OutputPath, EpisodeInfo.GameExt);
    f = fopen(filename, "rb");
    if (!f)
        quit("Unable to open %s!");

    for (i = 0; i < EpisodeInfo.NumSprites; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EpisodeInfo.NumSprites);

        /* Output the collision rectangle and origin information */
        if (fscanf(f, "%d: [%d, %d, %d, %d], [%d, %d], %d\n", &j, &rx1, &ry1, &rx2, &ry2, &ox, &oy, &sh) != 8)
            quit("Error reading data for sprite %d from sprite text file!", i);
        if (j != i)
            quit("Sprite text file has entry for %d when %d expected!", j, i);
        if (sh != 1 && sh != 2 && sh != 4)
            quit("Sprite %d has an illegal shift value!", i);

        SprHead[i].OrgX = ox << 4;
        SprHead[i].OrgY = oy << 4;
        SprHead[i].Rx1 = (rx1 + ox) << 4;
        SprHead[i].Ry1 = (ry1 + oy) << 4;
        SprHead[i].Rx2 = (rx2 + ox) << 4;
        SprHead[i].Ry2 = (ry2 + oy) << 4;
        SprHead[i].Shifts = sh;

        /* Open the bitmap file */
        sprintf(filename, "%s/%s_sprite_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        spr = bmp256_load(filename);
        if (!spr)
            quit("Can't open bitmap file %s!", filename);
        if (spr->width % (granularity * 8) != 0)
            quit("Sprite bitmap %s is not a multiple of %d pixels wide!",
                filename, granularity * 8);
        if (!Switches->SeparateMask && spr->bpp != 8)
            quit("Sprite bitmap %s is not a 256-color bitmap!", filename);

        /* Grab the color plane data from the bitmap */
        bmp = bmp256_create(spr->width / granularity, spr->height, 8);
        if (!bmp)
            quit("Not enough memory to create bitmap!");
        bmp256_blit(spr, 0, 0, bmp, 0, 0, bmp->width, bmp->height);

        if (Switches->SeparateMask) {
            /* Get the color planes */
            bmp256_split_ex(bmp, &planes[1], 0, 4);

            /* Get the mask */
            planes[0] = bmp256_create(spr->width / granularity, spr->height, 1);
            if (!planes[0])
                quit("Not enough memory to create bitmap!");
            bmp256_blit(spr, planes[0]->width, 0, planes[0], 0, 0, planes[0]->width, planes[0]->height);
        } else {
            /* Split color planes and mask, and move them into order */
            bmp256_split_ex(bmp, &planes[1], 0, 5);
            planes[0] = planes[5];
            planes[5] = NULL;
        }

        /* Set up the SprHead structures */
        SprHead[i].Width = bmp->width / 8;
        SprHead[i].Height = bmp->height;

        /* Allocate memory for the data */
        EgaGraph[EpisodeInfo.IndexSprites + i].len = SprHead[i].Width * SprHead[i].Height * 5;
        pointer = malloc(SprHead[i].Width * SprHead[i].Height * 5);
        if (!pointer)
            quit("Not enough memory for bitmap %d!", i);
        EgaGraph[EpisodeInfo.IndexSprites + i].data = pointer;


        /* Encode the bitmap data */
        for (p = 0; p < 5; p++) {
            if (!planes[p])
                quit("Not enough memory to create bitmap!");

            /* Encode the lines of the bitmap data */
            pointer = EgaGraph[EpisodeInfo.IndexSprites + i].data + p * SprHead[i].Width * SprHead[i].Height;
            for (y = 0; y < SprHead[i].Height; y++)
                memcpy(pointer + y * SprHead[i].Width, planes[p]->lines[y], SprHead[i].Width);
        }

        /* Free the memory used */
        for (p = 0; p < 5; p++) {
            bmp256_free(planes[p]);
        }
        bmp256_free(spr);
        bmp256_free(bmp);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();

    fclose(f);
}

void k456_import_terminator_text() {

    MiscInfoList *mp;
    int y, len;
    BITMAP256 *bmp;
    char filename[PATH_MAX];
    char *pointer;
    uint16_t *rleptr;

    if (!ImportInitialised)
        quit("Trying to export terminator text before initialisation!");

    do_output("Importing terminator text: ");

    /* Open the bitmap file */
    for (mp = MiscInfos; mp; mp = mp->next) {

        if (strcmp(mp->Type, "TERMINATOR"))
            continue;

        /* Open the bitmap */
        sprintf(filename, "%s/%s_terminator_%s.bmp", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
        bmp = bmp256_load(filename);
        if (!bmp)
            quit("Can't open bitmap file %s!", filename);

        /* Allocate enough memory for the chunk */
        pointer = (char*) malloc(sizeof (TerminatorHeadStruct) + 2 * bmp->height + bmp->height * bmp->width * 2);
        if (!pointer)
            quit("Not enough memory to import terminator text!");
        rleptr = (uint16_t*)&(((TerminatorHeadStruct*) pointer)->LineStarts[bmp->height]);
        ((TerminatorHeadStruct*) pointer)->Height = bmp->height;
        ((TerminatorHeadStruct*) pointer)->Width = bmp->width;

        for (y = 0; y < bmp->height; y++) {
            int color, x, runlength;

            /* Ensure leftmost column is black */
            if (bmp256_getpixel(bmp, 0, y) != 0)
                quit("Leftmost column of %s isn't black!", filename);

            /* Record the pointer to the RLE data for this line in the header */
            ((TerminatorHeadStruct*) pointer)->LineStarts[y] = (uint16_t) ((uintptr_t) rleptr - (uintptr_t) pointer);

            /* Encode the RLE data */
            color = 0;
            x = 0;
            runlength = 0;
            while (x < bmp->width) {
                if (bmp256_getpixel(bmp, x, y) == color) {
                    runlength++;
                    x++;
                } else {
                    *(rleptr++) = runlength;
                    color ^= 1;
                    runlength = 0;
                }
            }
            *(rleptr++) = runlength;
            *(rleptr++) = (uint16_t) 0xFFFF;
        }

        /* Free Memory */
        bmp256_free(bmp);
        len = ((char*) rleptr - pointer);
        pointer = realloc(pointer, len);
        EgaGraph[mp->Chunk].len = len;
        EgaGraph[mp->Chunk].data = (uint8_t *)pointer;
    }

    //do_output("\x8\x8\x8\x8");
    completemsg();

}

void k456_import_ansi() {
    char filename[PATH_MAX];
    FILE *f;
    unsigned long len;
    uint8_t *pointer;
    MiscInfoList *mp;

    if (!ImportInitialised)
        quit("Trying to import ANSI art before initialisation!");

    /* Import all the Ansi Art */
    do_output("Importing ANSI art: ");

    for (mp = MiscInfos; mp; mp = mp->next) {

        if (strcmp(mp->Type, "B800TEXT"))
            continue;

        /* Open the demo file */
        sprintf(filename, "%s/%s_ansi_%s.bin", Switches->OutputPath, EpisodeInfo.GameExt, mp->File);
        f = openfile(filename, "rb", Switches->Backup);
        if (!f)
            quit("Can't open %s!", filename);

        /* Get the file length */
        fseek(f, 1, SEEK_END);
        len = ftell(f) - 1;
        fseek(f, 0, SEEK_SET);

        /* Allocate memory */
        pointer = malloc(len);
        if (!pointer)
            quit("Not enough memory to import ANSI art!");
        EgaGraph[mp->Chunk].len = len;
        EgaGraph[mp->Chunk].data = pointer;

        /* Read the data and close the file */
        fread(pointer, len, 1, f);
        fclose(f);

        //do_output("\x8\x8\x8\x8");
    }
    completemsg();
}

void do_k456_import(SwitchStruct *switches) {
    k456_import_begin(switches);
    k456_import_fonts();
    k456_import_bitmaps();
    k456_import_masked_bitmaps();
    k456_import_tiles();
    k456_import_masked_tiles();
    k456_import_8_tiles();
    k456_import_8_masked_tiles();
    k456_import_sprites();
    k456_import_texts();
    k456_import_terminator_text();
    k456_import_ansi();
    k456_import_demos();
    k456_import_misc();
    k456_import_end();
}

/************************************************************************************************************/
/** KEEN 4, 5, 6 PARSING ROUTINES *************************************************************************/
/************************************************************************************************************/

static void *OldParseBuffer = NULL;

void parse_k456_misc_descent(void **buf) {
    MiscInfoList *ei;

    /* Add a new ExternInfoList to the list */
    ei = malloc(sizeof (MiscInfoList));
    ei->next = MiscInfos;
    MiscInfos = ei;

    /* Set the parser buffer to the new ExternInfoList */

    OldParseBuffer = *buf;
    *buf = ei;
    strcpy(((MiscInfoList*) * buf)->Type, "MISC");
}

void parse_k456_misc_ascent(void **buf) {
    /* Restore the parser buffer */
    *buf = OldParseBuffer;
}

void parse_k456_b800_descent(void **buf) {
    parse_k456_misc_descent(buf);
    strcpy(((MiscInfoList*) * buf)->Type, "B800TEXT");
}

void parse_k456_terminator_descent(void **buf) {
    parse_k456_misc_descent(buf);
    strcpy(((MiscInfoList*) * buf)->Type, "TERMINATOR");
}
