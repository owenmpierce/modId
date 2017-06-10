/* KEEN123.C - Keen 1, 2, and 3 import and export functions.
 **
 ** Copyright (c)2016-2017 by Owen Pierce
 ** Based on LModkeen 2 Copyright (c)2007 by Ignacio R. Morelle "Shadow Master". (shadowm2006@gmail.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include <memory.h>
#include "pconio.h"

#include "utils.h"
#include "lz.h"
#include "bmp256.h"
#include "switches.h"
#include "parser.h"

#pragma pack(1)

// Local function prototypes
int fin_to_bmp(char *finfile, char *bmpfile);
int bmp_to_fin(char *bmpfile, char *finfile);

void parse_k123_extern_ascent(void **);
void parse_k123_extern_descent(void **);

/* Define the structures for EGAHEAD */
typedef struct {
    uint32_t LatchPlaneSize; // Size of one plane of latch data
    uint32_t SpritePlaneSize; // Size of one plane of sprite data
    uint32_t OffBitmapTable; // Offset in EGAHEAD to bitmap table
    uint32_t OffSpriteTable; // Offset in EGAHEAD to sprite table
    uint16_t Num8Tiles; // Number of 8x8 tiles
    uint32_t Off8Tiles; // Offset of 8x8 tiles (relative to plane data)
    uint16_t Num32Tiles; // Number of 32x32 tiles (always 0)
    uint32_t Off32Tiles; // Offset of 32x32 tiles (relative to plane data)
    uint16_t Num16Tiles; // Number of 16x16 tiles
    uint32_t Off16Tiles; // Offset of 16x16 tiles (relative to plane data)
    uint16_t NumBitmaps; // Number of bitmaps in table
    uint32_t OffBitmaps; // Offset of bitmaps (relative to plane data)
    uint16_t NumSprites; // Number of sprites
    uint32_t OffSprites; // Offset of sprites (relative to plane data)
    uint16_t Compressed; // (Keen 1 only) Nonzero: LZ compressed data
} EgaHeadStruct;

typedef struct {
    uint16_t Width; // Width of the bitmap
    uint16_t Height; // Height of the bitmap
    uint32_t Offset; // Offset of bitmap in EGALATCH
    uint8_t Name[8]; // Name of the bitmap
} BitmapHeadStruct;

typedef struct {
    uint16_t Width; // Width of the sprite
    uint16_t Height; // Height of the bitmap
    uint16_t OffsetDelta; // Remaining offset to the bitmap in bytes
    uint16_t OffsetParas; // Offset to the bitmap in paragraphs
    uint16_t Rx1, Ry1; // Top-left corner of clipping rectangle
    uint16_t Rx2, Ry2; // Bottom-right corner of clipping rectangle
    uint8_t Name[16]; // Name of the sprite
} SpriteHeadStruct;

typedef struct {
    uint16_t OffSet;
    uint8_t Priority;
    uint8_t Exist;
    uint8_t Name[12];
} SndTypeStruct;

#pragma pack()

typedef struct {
    char ExeName[14];
    char GameExt[4];
    unsigned int NumTiles;
    unsigned int NumFonts;
    unsigned int NumBitmaps;
    unsigned int NumSprites;
    unsigned int NumExternals; // TODO: expand this
    unsigned int NumSounds;
    unsigned long OffsetSounds;
} EpisodeInfoStruct;

/* Stores filenames of external files (e.g. keen1 previews) */
typedef struct ExternInfoList_s {
    char Name[PATH_MAX]; // Filename to read data from
    struct ExternInfoList_s *next; // Tail of list
} ExternInfoList;


static int ExportInitialised = 0;
static int ImportInitialised = 0;
static EgaHeadStruct *EgaHead = NULL;
static BitmapHeadStruct *BmpHead = NULL;
static SpriteHeadStruct *SprHead = NULL;
static uint8_t *LatchData = NULL;
static uint8_t *SpriteData = NULL;
static BITMAP256 *FontBmp = NULL;
static BITMAP256 *TileBmp = NULL;
static BITMAP256 **SpriteBmp = NULL;
static BITMAP256 **BitmapBmp = NULL;
static SwitchStruct *Switches = NULL;

static ExternInfoList *ExternInfos = NULL;


#define PREVIEW_COUNTSTART			2
/* A buffer for file-defined episodes */
static EpisodeInfoStruct EpisodeInfo;
//EpisodeInfoStruct *GalaxyEpisodeInfo = &EpisodeInfo;
// TODO: This
EpisodeInfoStruct *VorticonsEpisodeInfo = &EpisodeInfo;

static ValueNode CV_GAMEEXT[] = {
    VALUENODE("%3s", EpisodeInfoStruct, GameExt),
    ENDVALUE
};

static ValueNode CV_EXEINFO[] = {
    VALUENODE("%13s", EpisodeInfoStruct, ExeName),
    ENDVALUE
};

static ValueNode CV_TILES[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumTiles),
    ENDVALUE
};

static ValueNode CV_FONTS[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumFonts),
    ENDVALUE
};

static ValueNode CV_PICS[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumBitmaps),
    ENDVALUE
};

static ValueNode CV_SPRITES[] = {
    VALUENODE("%i", EpisodeInfoStruct, NumSprites),
    ENDVALUE
};

static ValueNode CV_EXTERN[] = {
    VALUENODE("%s", ExternInfoList, Name),
    ENDVALUE
};

ValueNode CV_VORTICONS[] = {
    ENDVALUE
};

CommandNode SC_VORTICONS[] = {
    COMMANDLEAF(GAMEEXT, NULL, NULL),
    COMMANDLEAF(EXEINFO, NULL, NULL),
    COMMANDLEAF(TILES, NULL, NULL),
    COMMANDLEAF(FONTS, NULL, NULL),
    COMMANDLEAF(PICS, NULL, NULL),
    COMMANDLEAF(SPRITES, NULL, NULL),
    //	COMMANDLEAF (EXTERN, NULL, NULL),
    COMMANDLEAF(EXTERN, parse_k123_extern_descent, parse_k123_extern_ascent),
};

/************************************************************************************************************/
/** KEEN 1, 2, 3 EXPORTING ROUTINES *************************************************************************/

/************************************************************************************************************/

void k123_export_begin(SwitchStruct *switches) {
    char filename[PATH_MAX];
    FILE *headfile = NULL;
    FILE *latchfile = NULL;
    FILE *spritefile = NULL;

    /* Never allow the export start to occur more than once */
    if (ExportInitialised)
        quit("Tried to initialise Keen 1 files a second time!");

    /* Save the switches */
    Switches = switches;

    /* Open EGAHEAD */
    sprintf(filename, "%s/egahead.%s", Switches->InputPath, EpisodeInfo.GameExt);
    if (!(headfile = openfile(filename, "rb", Switches->Backup)))
        quit("Can't open %s!\n", filename);

    /* Read all the header data */
    EgaHead = (EgaHeadStruct *) malloc(sizeof (EgaHeadStruct));
    if (!EgaHead)
        quit("Not enough memory to read header");

    fread(EgaHead, sizeof (EgaHeadStruct), 1, headfile);

    /* Now check that the egahead appears correct */
    if ((EgaHead->Num8Tiles / 256) != EpisodeInfo.NumFonts)
        quit("EgaHead should have only %d font! Check your version!", EpisodeInfo.NumFonts);
    if (EgaHead->Num16Tiles != EpisodeInfo.NumTiles)
        quit("EgaHead should have only %d tiles! Check your version!", EpisodeInfo.NumTiles);
    if (EgaHead->NumBitmaps != EpisodeInfo.NumBitmaps)
        quit("EgaHead should have only %d bitmaps! Check your version!", EpisodeInfo.NumBitmaps);
    if (EgaHead->NumSprites != EpisodeInfo.NumSprites)
        quit("EgaHead should have only %d sprites! Check your version!", EpisodeInfo.NumSprites);

    /* Allocate space for bitmap and sprite tables */
    BmpHead = (BitmapHeadStruct *) malloc(EgaHead->NumBitmaps * sizeof (BitmapHeadStruct));
    SprHead = (SpriteHeadStruct *) malloc(EgaHead->NumSprites * 4 * sizeof (SpriteHeadStruct));
    if (!BmpHead || !SprHead)
        quit("Not enough memory to create tables!\n");

    /* Read the bitmap and sprite tables */
    fseek(headfile, EgaHead->OffBitmapTable, SEEK_SET);
    fread(BmpHead, sizeof (BitmapHeadStruct), EgaHead->NumBitmaps, headfile);
    fseek(headfile, EgaHead->OffSpriteTable, SEEK_SET);
    fread(SprHead, 4 * sizeof (SpriteHeadStruct), EgaHead->NumSprites, headfile);

    /* Open the latch and sprite files and read them into memory */
    sprintf(filename, "%s/egalatch.%s", Switches->InputPath, EpisodeInfo.GameExt);
    latchfile = openfile(filename, "rb", Switches->Backup);
    if (!latchfile)
        quit("Cannot open %s!\n", filename);
    sprintf(filename, "%s/egasprit.%s", Switches->InputPath, EpisodeInfo.GameExt);
    spritefile = openfile(filename, "rb", Switches->Backup);
    if (!spritefile)
        quit("Cannot open %s!\n", filename);
    LatchData = (uint8_t *) malloc(EgaHead->LatchPlaneSize * 4);
    SpriteData = (uint8_t *) malloc(EgaHead->SpritePlaneSize * 5);
    if (!LatchData || !SpriteData)
        quit("Not enough memory to load latch and sprite data!\n");

    /* Read the latch and sprite data, decompressing it if necessary for Keen 1 */
    if (EgaHead->Compressed) {
        fseek(latchfile, 6, SEEK_SET);
        fseek(spritefile, 6, SEEK_SET);
        lz_decompress(spritefile, (char *) SpriteData);
        lz_decompress(latchfile, (char *) LatchData);
    } else {
        fread(LatchData, EgaHead->LatchPlaneSize * 4, 1, latchfile);
        fread(SpriteData, EgaHead->SpritePlaneSize * 5, 1, spritefile);
    }

    fclose(headfile);
    fclose(latchfile);
    fclose(spritefile);

    ExportInitialised = 1;
}

void k123_export_bitmaps() {
    BITMAP256 *bmp, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export bitmaps before initialisation!");

    /* Export all the bitmaps */
    do_output("Exporting bitmaps");

    for (i = 0; i < EgaHead->NumBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EgaHead->NumBitmaps);

        /* Decode the bitmap data */
        for (p = 0; p < 4; p++) {
            /* Create a 1bpp bitmap for each plane */
            planes[p] = bmp256_create(BmpHead[i].Width * 8, BmpHead[i].Height, 1);

            /* Decode the lines of the bitmap data */
            pointer = LatchData + EgaHead->OffBitmaps + BmpHead[i].Offset + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < BmpHead[i].Height; y++)
                memcpy(planes[p]->lines[y], pointer + y * BmpHead[i].Width, BmpHead[i].Width);
        }

        /* Create the bitmap file */
        sprintf(filename, "%s/%s_pic_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        bmp = bmp256_merge_ex(planes, 4, 4);
        if (!bmp256_save(bmp, filename, Switches->Backup))
            quit("Can't open bitmap file %s!", filename);

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_export_sprites() {
    BITMAP256 *spr, *bmp, *planes[5];
    char filename[PATH_MAX];
    int i, p, y;
    unsigned granularity;
    uint8_t *pointer;
    SpriteHeadStruct *sprhead;

    if (!ExportInitialised)
        quit("Trying to export sprites before initialisation!");

    /* Export all the sprites */
    do_output("Exporting sprites");

    granularity = Switches->SeparateMask ? 3 : 2;

    for (i = 0; i < EgaHead->NumSprites; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EgaHead->NumSprites);

        /* Construct the sprite bitmap */
        sprhead = &SprHead[i * 4];
        spr = bmp256_create(sprhead->Width * 8 * granularity, sprhead->Height,
                Switches->SeparateMask ? 4 : 8);

        /* Decode the sprite color plane and mask data */
        for (p = 0; p < 5; p++) {
            /* Create a 1bpp bitmap for each plane */
            planes[p] = bmp256_create(sprhead->Width * 8, sprhead->Height, 1);

            /* Decode the lines of the image data */
            pointer = SpriteData + EgaHead->OffSprites + sprhead->OffsetParas * 16 + sprhead->OffsetDelta + p * EgaHead->SpritePlaneSize;
            for (y = 0; y < sprhead->Height; y++)
                memcpy(planes[p]->lines[y], pointer + y * sprhead->Width, sprhead->Width);
        }

        /* Draw the Color planes and mask */
        if (Switches->SeparateMask) {
            bmp = bmp256_merge_ex(planes, 4, 4);
            bmp256_blit(planes[4], 0, 0, spr, bmp->width, 0, bmp->width, bmp->height);
        } else {
            bmp = bmp256_merge_ex(planes, 5, 8);
        }

        bmp256_blit(bmp, 0, 0, spr, 0, 0, bmp->width, bmp->height);

        /* Draw the clipping rectangle, red within dark grey */
        bmp256_rect(spr, bmp->width * (granularity - 1), 0, bmp->width * granularity - 1, bmp->height - 1, 8);
        bmp256_rect(spr, bmp->width * (granularity - 1) + (sprhead->Rx1 >> 8), (sprhead->Ry1 >> 8),
                bmp->width * (granularity - 1) + (sprhead->Rx2 >> 8), (sprhead->Ry2 >> 8), 12);

        /* Create the bitmap file */
        sprintf(filename, "%s/%s_sprite_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        if (!bmp256_save(spr, filename, Switches->Backup))
            quit("Can't open bitmap file %s!", filename);

        /* Free the memory used */
        for (p = 0; p < 5; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);
        bmp256_free(spr);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_export_tiles() {
    BITMAP256 *bmp, *tiles, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export tiles before initialisation!");

    /* Export all the 16x16 tiles into one bitmap */
    do_output("Exporting tiles");

    /* Create a bitmap large enough to hold all the tiles */
    tiles = bmp256_create(13 * 16, (EgaHead->Num16Tiles + 12) / 13 * 16, 4);

    for (i = 0; i < EgaHead->Num16Tiles; i++) {
        showprogress((i * 100) / EgaHead->Num16Tiles);

        for (p = 0; p < 4; p++) {
            /* Create a 1bpp bitmap for each plane */
            planes[p] = bmp256_create(16, 16, 1);

            pointer = LatchData + EgaHead->Off16Tiles + i * 32 + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < 16; y++)
                memcpy(planes[p]->lines[y], pointer + y * 2, 2);
        }

        /* Merge the tile and put it in the large bitmap */
        bmp = bmp256_merge_ex(planes, 4, 4);
        bmp256_blit(bmp, 0, 0, tiles, (i % 13) * 16, (i / 13) * 16, 16, 16);

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();

    /* Save the bitmap */
    sprintf(filename, "%s/%s_tile16.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(tiles, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);
    bmp256_free(tiles);
}

void k123_export_fonts() {
    BITMAP256 *bmp, *font, *planes[4];
    char filename[PATH_MAX];
    int i, p, y;
    uint8_t *pointer;

    if (!ExportInitialised)
        quit("Trying to export font before initialisation!");

    /* Export all the 8x8 tiles into one bitmap */
    do_output("Exporting font");

    /* Create a bitmap large enough to hold all the tiles */
    font = bmp256_create(16 * 8, (EgaHead->Num8Tiles + 15) / 16 * 8, 4);

    for (i = 0; i < EgaHead->Num8Tiles; i++) {
        showprogress((i * 100) / EgaHead->Num8Tiles);

        for (p = 0; p < 4; p++) {
            /* Create a 1bpp bitmap for each plane */
            planes[p] = bmp256_create(8, 8, 1);

            pointer = LatchData + EgaHead->Off8Tiles + i * 8 + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < 8; y++)
                memcpy(planes[p]->lines[y], pointer + y, 1);
        }

        /* Merge the tile and put it in the large bitmap */
        bmp = bmp256_merge_ex(planes, 4, 4);
        bmp256_blit(bmp, 0, 0, font, (i % 16) * 8, (i / 16) * 8, 8, 8);

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();

    /* Save the bitmap */
    sprintf(filename, "%s/%s_font.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    if (!bmp256_save(font, filename, Switches->Backup))
        quit("Can't open bitmap file %s!", filename);
    bmp256_free(font);
}

/*
 * Export Full Screen bitmaps (stored in external files)
 */
void k123_export_external() {
    char *in_filename;
    char *out_filename;
    ExternInfoList *ep;

    do_output("Exporting external graphics");

    /* Export previews first (if any) */
    for (ep = ExternInfos; ep; ep = ep->next) {

        in_filename = (char*) malloc(sizeof (char) * PATH_MAX);
        out_filename = (char*) malloc(sizeof (char) * PATH_MAX);

        sprintf(in_filename, "%s/%s.%s", Switches->InputPath, ep->Name, EpisodeInfo.GameExt);
        sprintf(out_filename, "%s/%s_extern_%s.bmp", Switches->OutputPath, EpisodeInfo.GameExt, ep->Name);

        if (fin_to_bmp(in_filename, out_filename))
            quit("\nCouldn't convert %s to %s!", in_filename, out_filename);

        free(in_filename);
        free(out_filename);
    }
    completemsg();
}

void k123_export_end() {
    /* Clean up */
    if (!ExportInitialised)
        quit("Tried to end export before beginning it!");

    free(LatchData);
    free(SpriteData);
    free(BmpHead);
    free(SprHead);
    free(EgaHead);

    ExportInitialised = 0;
}

void do_k123_export(SwitchStruct *switches) {
    k123_export_begin(switches);
    k123_export_bitmaps();
    k123_export_sprites();
    k123_export_tiles();
    k123_export_fonts();
    k123_export_external();
    k123_export_end();
}

/************************************************************************************************************/
/** KEEN 1, 2, 3 IMPORTING ROUTINES *************************************************************************/

/************************************************************************************************************/


void k123_import_begin(SwitchStruct *switches) {
    char filename[PATH_MAX];
    int i;
    unsigned sprgranularity;
    uint32_t size;

    /* Never allow the import start to occur more than once */
    if (ImportInitialised)
        quit("Tried to initialise Keen 1 files a second time!");

    /* Save the switches */
    Switches = switches;

    /* Create the header structure */
    EgaHead = (EgaHeadStruct *) malloc(sizeof (EgaHeadStruct));
    if (!EgaHead)
        quit("Not enough memory to create header");

    /* Read the font bitmap */
    sprintf(filename, "%s/%s_font.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    FontBmp = bmp256_load(filename);
    if (!FontBmp)
        quit("Can't open font bitmap %s!", filename);
    if (FontBmp->width != 128)
        quit("Font bitmap %s is not 128 pixels wide!", filename);

    /* Read the tile bitmap */
    sprintf(filename, "%s/%s_tile16.bmp", Switches->OutputPath, EpisodeInfo.GameExt);
    TileBmp = bmp256_load(filename);
    if (!TileBmp)
        quit("Can't open tile bitmap %s!", filename);
    if (TileBmp->width != 13 * 16)
        quit("Tile bitmap %s is not 208 pixels wide!", filename);

    /* Read the bitmaps */
    BitmapBmp = (BITMAP256 **) malloc(EpisodeInfo.NumBitmaps * sizeof (BITMAP256 *));
    if (!BitmapBmp)
        quit("Not enough memory to create bitmaps!");
    for (i = 0; i < EpisodeInfo.NumBitmaps; i++) {
        sprintf(filename, "%s/%s_pic_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        BitmapBmp[i] = bmp256_load(filename);
        if (!BitmapBmp[i])
            quit("Can't open bitmap %s!", filename);
        if (BitmapBmp[i]->width % 8 != 0)
            quit("Bitmap %s is not a multiple of 8 pixels wide!", filename);
    }

    /* Read the sprites */
    sprgranularity = Switches->SeparateMask ? 3 : 2;
    SpriteBmp = (BITMAP256 **) malloc(EpisodeInfo.NumSprites * sizeof (BITMAP256 *));
    if (!SpriteBmp)
        quit("Not enough memory to create sprites!");
    for (i = 0; i < EpisodeInfo.NumSprites; i++) {
        sprintf(filename, "%s/%s_sprite_%04d.bmp", Switches->OutputPath, EpisodeInfo.GameExt, i);
        SpriteBmp[i] = bmp256_load(filename);
        if (!SpriteBmp[i])
            quit("Can't open sprite bitmap %s!", filename);
        if (SpriteBmp[i]->width % (sprgranularity * 8) != 0)
            quit("Sprite bitmap %s is not a multiple of %d pixels wide!",
                filename, sprgranularity * 8);
    }

    /* Now allocate the bmphead and sprhead */
    BmpHead = (BitmapHeadStruct *) malloc(EpisodeInfo.NumBitmaps * sizeof (BitmapHeadStruct));
    SprHead = (SpriteHeadStruct *) malloc(EpisodeInfo.NumSprites * 4 * sizeof (SpriteHeadStruct));
    if (!BmpHead || !SprHead)
        quit("Not enough memory to create tables!\n");

    /* Calculate the latch and sprite plane sizes */
    size = 0;
    EgaHead->OffBitmaps = size;
    for (i = 0; i < EpisodeInfo.NumBitmaps; i++)
        size += BitmapBmp[i]->width * BitmapBmp[i]->height / 8;
    size += (size % 16 == 0) ? 0 : (16 - size % 16); /* Convert size to paragraph multiple */
    EgaHead->Off8Tiles = size;
    size += EpisodeInfo.NumFonts * 256 * 8;
    size += (size % 16 == 0) ? 0 : (16 - size % 16); /* Convert size to paragraph multiple */
    EgaHead->Off16Tiles = size;
    size += EpisodeInfo.NumTiles * 32;
    size += (size % 16 == 0) ? 0 : (16 - size % 16); /* Convert size to paragraph multiple */
    EgaHead->LatchPlaneSize = size;

    size = 0;
    EgaHead->OffSprites = size;
    for (i = 0; i < EpisodeInfo.NumSprites; i++)
        size += SpriteBmp[i]->width * SpriteBmp[i]->height / sprgranularity / 8;
    /* The final size must be padded to a paragraph */
    EgaHead->SpritePlaneSize = size + (16 - size % 16);

    /* Set up the EgaHead structure */
    EgaHead->Num8Tiles = EpisodeInfo.NumFonts * 256;
    EgaHead->Num32Tiles = 0;
    EgaHead->Off32Tiles = 0;
    EgaHead->Num16Tiles = EpisodeInfo.NumTiles;
    EgaHead->NumBitmaps = EpisodeInfo.NumBitmaps;
    EgaHead->OffBitmapTable = sizeof (EgaHeadStruct);
    EgaHead->NumSprites = EpisodeInfo.NumSprites;
    EgaHead->OffSpriteTable = EgaHead->OffBitmapTable + EgaHead->NumBitmaps * sizeof (BitmapHeadStruct);
    EgaHead->Compressed = 0;

    LatchData = (uint8_t *) malloc(EgaHead->LatchPlaneSize * 4);
    SpriteData = (uint8_t *) malloc(EgaHead->SpritePlaneSize * 5);
    if (!LatchData || !SpriteData)
        quit("Not enough memory to load latch and sprite data!\n");

    ImportInitialised = 1;
}

void k123_import_bitmaps() {
    BITMAP256 * planes[4];
    int i, p, y;
    uint8_t *pointer;
    uint32_t offset;

    if (!ImportInitialised)
        quit("Tried to import bitmaps without initialising!");

    /* Import all the bitmaps */
    do_output("Importing bitmaps");

    offset = 0;
    for (i = 0; i < EgaHead->NumBitmaps; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EgaHead->NumBitmaps);

        /* Set up the BmpHead for this bitmap */
        BmpHead[i].Width = BitmapBmp[i]->width / 8;
        BmpHead[i].Height = BitmapBmp[i]->height;
        BmpHead[i].Offset = offset;
        strncpy((char *)BmpHead[i].Name, "", 8);
        offset += BmpHead[i].Width * BmpHead[i].Height;

        /* Split the bitmap up into planes */
        bmp256_split_ex(BitmapBmp[i], planes, 0, 4);

        for (p = 0; p < 4; p++) {
            /* Copy the lines of the bitmap data */
            pointer = LatchData + EgaHead->OffBitmaps + BmpHead[i].Offset + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < BmpHead[i].Height; y++)
                memcpy(pointer + y * BmpHead[i].Width, planes[p]->lines[y], BmpHead[i].Width);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_import_sprites() {
    BITMAP256 *bmp, *planes[5];
    int i, j, p, y, x;
    uint8_t *pointer;
    unsigned granularity;
    uint32_t offset;
    SpriteHeadStruct *sprhead;

    if (!ImportInitialised)
        quit("Tried to import sprites without initialising!");

    /* Import all the sprites */
    do_output("Importing sprites");

    granularity = Switches->SeparateMask ? 3 : 2;

    offset = 0;
    for (i = 0; i < EgaHead->NumSprites; i++) {
        /* Show that something is happening */
        showprogress((i * 100) / EgaHead->NumSprites);

        /* Set up the SprHead for this bitmap */
        sprhead = &SprHead[i * 4];
        sprhead->Width = SpriteBmp[i]->width / granularity / 8;
        sprhead->Height = SpriteBmp[i]->height;
        sprhead->OffsetDelta = offset % 16;
        sprhead->OffsetParas = offset / 16;

        /* Work out top-left corner of the clipping rectangle */
        x = y = 0;
        for (y = 0; y < SpriteBmp[i]->height; y++)
            for (x = 0; x < SpriteBmp[i]->width / granularity; x++)
                if (bmp256_getpixel(SpriteBmp[i],
                        SpriteBmp[i]->width / granularity * (granularity - 1) + x, y) == 12)
                    goto foundtl;
foundtl:
        sprhead->Rx1 = x << 8;
        sprhead->Ry1 = y << 8;

        /* Work out bottom-right corner of the clipping rectangle */
        x = y = 0;
        for (y = SpriteBmp[i]->height - 1; y >= 0; y--)
            for (x = SpriteBmp[i]->width / granularity - 1; x >= 0; x--)
                if (bmp256_getpixel(SpriteBmp[i],
                        SpriteBmp[i]->width / granularity * (granularity - 1) + x, y) == 12)
                    goto foundbr;
foundbr:
        sprhead->Rx2 = x << 8;
        sprhead->Ry2 = y << 8;

        strncpy((char *)sprhead->Name, "", 16);

        /* Copy this into the other three SprHead structures for this sprite */
        for (j = 1; j < 4; j++) {
            /* The extra width allows for shifts */
            SprHead[i * 4 + j].Width = sprhead->Width + 1;
            SprHead[i * 4 + j].Height = sprhead->Height;
            SprHead[i * 4 + j].OffsetDelta = sprhead->OffsetDelta;
            SprHead[i * 4 + j].OffsetParas = sprhead->OffsetParas;
            SprHead[i * 4 + j].Rx1 = sprhead->Rx1;
            SprHead[i * 4 + j].Ry1 = sprhead->Ry1;
            SprHead[i * 4 + j].Rx2 = sprhead->Rx2;
            SprHead[i * 4 + j].Ry2 = sprhead->Ry2;
            strncpy((char *)SprHead[i * 4 + j].Name, (char *)sprhead->Name, 16);
        }
        offset += sprhead->Width * sprhead->Height;

        /* Copy the sprite image and split it up into planes */
        bmp = bmp256_create(SpriteBmp[i]->width / granularity,
                SpriteBmp[i]->height, 8);
        bmp256_blit(SpriteBmp[i], 0, 0, bmp, 0, 0, bmp->width, bmp->height);

        /* Get the mask data from the bitmap */
        if (Switches->SeparateMask) {
            bmp256_split_ex(bmp, planes, 0, 4);
            planes[4] = bmp256_create(bmp->width, bmp->height, 1);
            bmp256_blit(SpriteBmp[i], bmp->width, 0, planes[4], 0, 0, bmp->width, bmp->height);
        } else {
            bmp256_split_ex(bmp, planes, 0, 5);
            for (j = 0; j < 5; j++) {
                if (!planes[j])
                    quit("No plane %d!\n", j);
            }
        }

        /* Decode the sprite image data */
        for (p = 0; p < 5; p++) {
            /* Copy the lines of the image data */
            pointer = SpriteData + EgaHead->OffSprites + sprhead->OffsetParas * 16 + sprhead->OffsetDelta + p * EgaHead->SpritePlaneSize;
            for (y = 0; y < sprhead->Height; y++)
                memcpy(pointer + y * sprhead->Width, planes[p]->lines[y], sprhead->Width);
        }

        /* Free the memory used */
        for (p = 0; p < 5; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_import_tiles() {
    BITMAP256 *bmp, *planes[4];
    int i, p, y;
    uint8_t *pointer;

    if (!ImportInitialised)
        quit("Tried to import tiles without initialising!");

    /* Import all the 16x16 tiles from one bitmap */
    do_output("Importing tiles");

    for (i = 0; i < EgaHead->Num16Tiles; i++) {
        showprogress((i * 100) / EgaHead->Num16Tiles);

        /* Copy the tile into the small bitmap and split it up into planes */
        bmp = bmp256_create(16, 16, 4);
        bmp256_blit(TileBmp, (i % 13) * 16, (i / 13) * 16, bmp, 0, 0, 16, 16);
        bmp256_split_ex(bmp, planes, 0, 4);

        for (p = 0; p < 4; p++) {
            pointer = LatchData + EgaHead->Off16Tiles + i * 32 + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < 16; y++)
                memcpy(pointer + y * 2, planes[p]->lines[y], 2);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_import_fonts() {
    BITMAP256 *bmp, *planes[4];
    int i, p, y;
    uint8_t *offset;

    if (!ImportInitialised)
        quit("Tried to import fonts without initialising!");

    /* Import all the 8x8 tiles from one bitmap */
    do_output("Importing font");

    for (i = 0; i < EgaHead->Num8Tiles; i++) {
        /* Copy the character into the small bitmap and split it up into planes */
        bmp = bmp256_create(8, 8, 4);
        bmp256_blit(FontBmp, (i % 16) * 8, (i / 16) * 8, bmp, 0, 0, 8, 8);
        bmp256_split_ex(bmp, planes, 0, 4);

        showprogress((i * 100) / EgaHead->Num8Tiles);

        for (p = 0; p < 4; p++) {
            offset = LatchData + EgaHead->Off8Tiles + i * 8 + p * EgaHead->LatchPlaneSize;
            for (y = 0; y < 8; y++)
                memcpy(offset + y, planes[p]->lines[y], 1);
        }

        /* Free the memory used */
        for (p = 0; p < 4; p++)
            bmp256_free(planes[p]);
        bmp256_free(bmp);

        //printf("\x8\x8\x8\x8");
    }
    completemsg();
}

void k123_import_external() {
    char *in_filename;
    char *out_filename;
    ExternInfoList *ep;

    do_output("Importing external graphics");
    /* Import previews first (if any) */
    /* Export previews first (if any) */
    for (ep = ExternInfos; ep; ep = ep->next) {

        in_filename = (char*) malloc(sizeof (char) * PATH_MAX);
        out_filename = (char*) malloc(sizeof (char) * PATH_MAX);

        sprintf(in_filename, "%s/%s_extern_%s.bmp", Switches->OutputPath, EpisodeInfo.GameExt, ep->Name);
        sprintf(out_filename, "%s/%s.%s", Switches->InputPath, ep->Name, EpisodeInfo.GameExt);

        if (bmp_to_fin(in_filename, out_filename))
            quit("\nCouldn't convert %s to %s!", in_filename, out_filename);

        free(in_filename);
        free(out_filename);
    }

    completemsg();
}

void k123_import_end() {
    int i;
    FILE *headfile, *latchfile, *spritefile;
    char filename[PATH_MAX];

    if (!ImportInitialised)
        quit("Tried to end import without initialising!");

    /* Save EGAHEAD data */
    sprintf(filename, "%s/egahead.%s", Switches->InputPath, EpisodeInfo.GameExt);
    headfile = openfile(filename, "wb", Switches->Backup);
    if (!headfile)
        quit("Can't open %s for writing!", filename);

    /* Write the egahead, bitmap and sprite tables */
    fwrite(EgaHead, sizeof (EgaHeadStruct), 1, headfile);
    fseek(headfile, EgaHead->OffBitmapTable, SEEK_SET);
    fwrite(BmpHead, sizeof (BitmapHeadStruct), EgaHead->NumBitmaps, headfile);
    fseek(headfile, EgaHead->OffSpriteTable, SEEK_SET);
    fwrite(SprHead, sizeof (SpriteHeadStruct), EgaHead->NumSprites * 4, headfile);

    /* Close the file */
    fclose(headfile);


    /* Write the latch file */
    sprintf(filename, "%s/egalatch.%s", Switches->InputPath, EpisodeInfo.GameExt);
    latchfile = openfile(filename, "wb", Switches->Backup);
    if (!latchfile)
        quit("Can't open %s for writing!", filename);
    fwrite(LatchData, EgaHead->LatchPlaneSize * 4, 1, latchfile);
    fclose(latchfile);

    /* Write the sprite file */
    sprintf(filename, "%s/egasprit.%s", Switches->InputPath, EpisodeInfo.GameExt);
    spritefile = openfile(filename, "wb", Switches->Backup);
    if (!spritefile)
        quit("Can't open %s for writing!", filename);
    fwrite(SpriteData, EgaHead->SpritePlaneSize * 5, 1, spritefile);
    fclose(spritefile);


    /* Free all the memory used */
    free(SpriteData);
    free(LatchData);
    for (i = EpisodeInfo.NumSprites - 1; i >= 0; i--)
        bmp256_free(SpriteBmp[i]);
    for (i = EpisodeInfo.NumBitmaps - 1; i >= 0; i--)
        bmp256_free(BitmapBmp[i]);
    bmp256_free(TileBmp);
    bmp256_free(FontBmp);
    free(EgaHead);

    ImportInitialised = 0;
}

/****************************************************************************/
/*                    FINALE 2 BMP CONVERSION CODE                          */
/****************************************************************************/
/* Added by: Ignacio "Shadow Master" R. Morelle <shadowm2006@gmail.com>
 * Original code by: Andew Durdin <andy@durdin.net>
 * Using it is simple enough fortunately ;)
 */

/* fin_to_bmp: exports to BMP */
int fin_to_bmp(char *finfile, char *bmpfile) {
    FILE *fin;
    BITMAP256 *bmp;
    int result = 0;
    int x, y, plane;
    int b, bitmask, i;
    long count, maxcount;
    uint8_t bytes[130];
    uint8_t bytecount;
    fin = fopen(finfile, "rb");
    if (!fin) {
        do_output("FIN2BMP: Cannot open %s for reading!\n", finfile);
        return 1;
    }
    bmp = bmp256_create(320, 200, 4);
    if (!bmp) {
        do_output("FIN2BMP: Can't create %s!\n", bmpfile);
        fclose(fin);
        return 1;
    }
    /* Clear the bitmap */
    bmp256_rect(bmp, 0, 0, 319, 199, 0);
    /* Read the plane data size from the file */
    fread(&maxcount, sizeof ( long), 1, fin);
    /*printf( "Decompressing...\n" );*/
    count = x = y = 0;
    plane = 1;
    while ((plane <= 8) && (b = fgetc(fin)) != EOF) {
        if (b & 0x80) {
            //N + 1 bytes of data follows
            bytecount = (b & 0x7F) + 1;
            fread(bytes, bytecount, 1, fin);
        } else {
            //Repeat N + 3 of following byte
            bytecount = b + 3;
            b = fgetc(fin);
            memset(bytes, b, bytecount);
        }
        /* Draw buffered bits */
        for (i = 0; i < bytecount; i++) {
            for (bitmask = 0x80; bitmask > 0; bitmask >>= 1) {
                if (bytes[i] & bitmask)
                    bmp256_putpixel(bmp, x, y, bmp256_getpixel(bmp, x, y) | plane);
                x++;
                if (x >= 320) {
                    x = 0;
                    y++;
                }
                count++;
                if (count >= maxcount * 2) {
                    count = 0;
                    x = 0;
                    y = 0;
                    plane <<= 1;
                }
            }
        }
    }
    /* The BMP now contains the picture--save it and return */
    if (!bmp256_save(bmp, bmpfile, Switches->Backup)) {
        result = 1;
        do_output("FIN2BMP: Can't save %s!\n", bmpfile);
    } /*else
		printf( "Done!\n" );*/
    bmp256_free(bmp);
    fclose(fin);
    return result;
}

/* bmp_to_fin: imports from BMP */
int bmp_to_fin(char *bmpfile, char *finfile) {
    uint8_t *datstart, *runstart, *probe;
    long count;
    int plane, len;
    FILE *fout;
    BITMAP256 *bmp;
    /* Load the bmp, and make sure it's the right size */
    bmp = bmp256_load(bmpfile);
    if (!bmp) {
        do_output("BMP2FIN: Can't find input bitmap, or not 16 colours!\n");
        return 1;
    }
    if (bmp->width != 320 || bmp->height != 200) {
        do_output("BMP2FIN: %s should be 320x200 16 colours!\n", bmpfile);
        bmp256_free(bmp);
        return 1;
    }
    /* Load the input data file */
    fout = fopen(finfile, "wb");
    if (!fout) {
        do_output("BMP2FIN: Can't open %s for writing!\n", finfile);
        bmp256_free(bmp);
        return 1;
    }

    /* Unpack the bmp */
    bmp256_unpack(bmp);

    /* Write the plane data size from the file */
    count = 0x8000L;
    fwrite(&count, sizeof ( long), 1, fout);
    /*printf("Compressing...\n");*/

    /* Compress the bits in each plane into the file */
    for (plane = 0; plane < 4; plane++) {
        count = 8000;
        datstart = bmp->bits + plane * bmp->linewidth * bmp->height;
        runstart = probe = datstart;
        while (count) {
            /* Probe for the start of a run */
            while (count && (probe - runstart) < 3) {
                probe++;
                count--;
                if (*probe != *runstart)
                    runstart = probe;
            }
            /* Probe for the end of the run */
            while (count && *probe == *runstart) {
                probe++;
                count--;
            }
            /* Now we should have datstart the start of the data, */
            /* runstart the start of the run, and probe 1 past the */
            /* end of the run */
            /* Save the data */
            while ((runstart - datstart) > 0) {
                len = (runstart - datstart);
                if (len > 128) len = 128;
                /* Write a block of data */
                fputc((len - 1) | 0x80, fout);
                fwrite(datstart, len, 1, fout);
                datstart += len;
            }
            /* Save the run */
            while ((probe - runstart) > 0) {
                len = (probe - runstart);
                if (len > 130) len = 130;
                /* Write a run code */
                fputc((len - 3), fout);
                fputc(*runstart, fout);
                runstart += len;
            }
            /* And continue from where the probe is */
            datstart = runstart = probe;
        }
        /* output 192 extra padding bytes at the end of each plane */
        fputc(0x7D, fout);
        fputc(0, fout);
        fputc(0x3D, fout);
        fputc(0, fout);
    }
    fclose(fout);
    bmp256_free(bmp);
    /*printf( "Done.\n" );*/
    return 0;
}

void do_k123_import(SwitchStruct *switches) {
    k123_import_begin(switches);
    k123_import_bitmaps();
    k123_import_sprites();
    k123_import_tiles();
    k123_import_fonts();
    k123_import_external();
    k123_import_end();
}

/************************************************************************************************************/
/** KEEN 1, 2, 3 PARSING ROUTINES *************************************************************************/
/************************************************************************************************************/

static void *OldParseBuffer = NULL;

/* parse_k123_extern: adds k123 extern bmp to ExternInfoList list*/
void parse_k123_extern_descent(void **buf) {
    ExternInfoList *ei;

    /* Add a new ExternInfoList to the list */
    ei = malloc(sizeof (ExternInfoList));
    ei->next = ExternInfos;
    ExternInfos = ei;

    /* Set the parser buffer to the new ExternInfoList */

    OldParseBuffer = *buf;
    *buf = ei;

}

void parse_k123_extern_ascent(void **buf) {
    /* Restore the parser buffer */
    *buf = OldParseBuffer;
}
