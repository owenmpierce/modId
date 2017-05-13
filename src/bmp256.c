/* BMP256.C - 1-bit, 4-bit, and 8-bit BMP manipulation routines.
 ** Adapted from BMP16.C by Lemm, 2011 - 2016
 **
 ** Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
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

#include "pconio.h" // Include this first or Win32 API conflicts with bmp16.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>

#include "utils.h"
#include "bmp256.h"

#define BMP_SIG   ((uint16_t)(0x4D42))
#define BI_RGB    0

typedef struct __attribute__ ((__packed__)) BITMAPFILEHEADER {
	uint16_t bfType; /* Must be 0x4D42 = "BM" (spec) */
	uint32_t bfSize; /* Size in bytes of BMP file */
	uint16_t bfReserved1; /* Must be 0 (spec) */
	uint16_t bfReserved2; /* Must be 0 (spec) */
	uint32_t bfOffBits; /* Offset in bytes to data */
} BITMAPFILEHEADER;

typedef struct __attribute__ ((__packed__)) BITMAPINFOHEADER {
	uint32_t biSize; /* Size of structure: 40 */
	int32_t biWidth; /* Width of bitmap in pixels */
	int32_t biHeight; /* Negative heights not supported */
	uint16_t biPlanes; /* Must be 1 (spec) */
	uint16_t biBitCount; /* Only 4 and 1 supported */
	uint32_t biCompression; /* Only BI_RGB supported */
	uint32_t biSizeImage; /* Size in bytes of image */
	int32_t biXPelsPerMeter; /* Ignored - 0 */
	int32_t biYPelsPerMeter; /* Ignored - 0 */
	uint32_t biClrUsed; /* 2 ^ bpp (export) */
	uint32_t biClrImportant; /* Ignored - 0 */
} BITMAPINFOHEADER;

typedef struct __attribute__ ((__packed__)) RGBQUAD {
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
} RGBQUAD;

RGBQUAD Palette2[2] ={
	{0x00, 0x00, 0x00, 0x00}, // black
	{0xFF, 0xFF, 0xFF, 0x00} // bright white
};

RGBQUAD Palette4[4] ={
	{0x00, 0x00, 0x00, 0x00}, // black
	{ 0x55, 0x55, 0x55, 0x00}, // dark gray
	{ 0xAA, 0xAA, 0xAA, 0x00}, // white
	{0xFF, 0xFF, 0xFF, 0x00} // bright white
};

RGBQUAD Palette256[256] ={
	{ 0x00, 0x00, 0x00, 0x00}, // black
	{ 0xAA, 0x00, 0x00, 0x00}, // blue
	{ 0x00, 0xAA, 0x00, 0x00}, // green
	{ 0xAA, 0xAA, 0x00, 0x00}, // cyan
	{ 0x00, 0x00, 0xAA, 0x00}, // red
	{ 0xAA, 0x00, 0xAA, 0x00}, // magenta
	{ 0x00, 0x55, 0xAA, 0x00}, // brown
	{ 0xAA, 0xAA, 0xAA, 0x00}, // white
	{ 0x55, 0x55, 0x55, 0x00}, // dark gray
	{ 0xFF, 0x55, 0x55, 0x00}, // bright blue
	{ 0x55, 0xFF, 0x55, 0x00}, // bright green
	{ 0xFF, 0xFF, 0x55, 0x00}, // bright cyan
	{ 0x55, 0x55, 0xFF, 0x00}, // bright red
	{ 0xFF, 0x55, 0xFF, 0x00}, // bright magenta
	{ 0x55, 0xFF, 0xFF, 0x00}, // bright yellow
	{ 0xFF, 0xFF, 0xFF, 0x00}, // bright white
	{ 0x00, 0xCC, 0xFF, 0x00}, // ORANGE for transparent (no orange in default EGA)
	{ 0xE3, 0xAA, 0xAA, 0x00}, // blue
	{ 0xAA, 0xE3, 0xAA, 0x00}, // green
	{ 0xE3, 0xE3, 0xAA, 0x00}, // cyan
	{ 0xAA, 0xAA, 0xE3, 0x00}, // red
	{ 0xE3, 0xAA, 0xE3, 0x00}, // magenta
	{ 0xAA, 0xC7, 0xE3, 0x00}, // brown
	{ 0xE3, 0xE3, 0xE3, 0x00}, // white
	{ 0xC7, 0xC7, 0xC7, 0x00}, // dark gray
	{ 0xFF, 0xC7, 0xC7, 0x00}, // bright blue
	{ 0xC7, 0xFF, 0xC7, 0x00}, // bright green
	{ 0xFF, 0xFF, 0xC7, 0x00}, // bright cyan
	{ 0xC7, 0xC7, 0xFF, 0x00}, // bright red
	{ 0xFF, 0xC7, 0xFF, 0x00}, // bright magenta
	{ 0xC7, 0xFF, 0xFF, 0x00}, // bright yellow
	{ 0xFF, 0xFF, 0xFF, 0x00}, // bright white
	/* Colors 32-255 are never used.  Make them an awful color */
#ifdef __GNUC__
	[32 ... 255] =
	{0x00, 0x99, 0x99, 0x00}
#endif
};

void bmp256_free(BITMAP256 *bmp) {
	if (bmp) {
		if (bmp->lines)
			free(bmp->lines);
		if (bmp->bits)
			free(bmp->bits);
		free(bmp);
	}
}

BITMAP256 *bmp256_load(char *fname) {
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	BITMAP256 *bmp;
	FILE *fin;
	int y;

	/* Open the input picture */
	fin = fopen(fname, "rb");
	if (!fin) {
		return NULL;
	}

	/* Read the header and make sure it's a real BMP */
	fread(&bfh, sizeof (BITMAPFILEHEADER), 1, fin);
	if (bfh.bfType != BMP_SIG) {
		fclose(fin);
		return NULL;
	}

	/* Make sure it's a format we can handle */
	fread(&bih, sizeof (BITMAPINFOHEADER), 1, fin);
	if (bih.biPlanes != 1 ||
			(bih.biBitCount != 8 && bih.biBitCount != 4 &&
			 bih.biBitCount != 2 && bih.biBitCount != 1) ||
			bih.biCompression != BI_RGB || bih.biHeight < 0) {
		fclose(fin);
		return NULL;
	}

	/* Create a memory bitmap */
	bmp = bmp256_create(bih.biWidth, bih.biHeight, bih.biBitCount);
	if (!bmp) {
		fclose(fin);
		return NULL;
	}

	/* Now load the data into the buffer, one line at a time */
	fseek(fin, bfh.bfOffBits, SEEK_SET);
	for (y = bmp->height - 1; y >= 0; y--)
		fread(bmp->lines[y], bmp->linewidth, 1, fin);

	/* Close the input file and return the bitmap pointer */
	fclose(fin);
	return bmp;
}

int bmp256_save(BITMAP256 *bmp, char *fname, int backup) {
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	RGBQUAD *palette;
	FILE *fout;
	int y;
	int colors;

	if (!bmp)
		return 0;

	/* Allow saving only 1bpp, 4bpp, and 8bpp bitmaps (2bpp isn't widely supported) */
	assert((bmp->bpp == 1) || (bmp->bpp == 4) || (bmp->bpp == 8));

	colors = (1 << bmp->bpp);

	/* Open the output picture */
	fout = openfile(fname, "wb", backup);
	if (!fout) {
		return 0;
	}

	/* Initialise the BITMAPFILEHEADER */
	bfh.bfType = BMP_SIG;
	bfh.bfSize = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + colors * sizeof (RGBQUAD) + bmp->linewidth * bmp->height;
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;
	bfh.bfOffBits = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + colors * sizeof (RGBQUAD);

	/* Initialise the BITMAPINFOHEADER */
	bih.biSize = sizeof (BITMAPINFOHEADER);
	bih.biWidth = bmp->width;
	bih.biHeight = bmp->height;
	bih.biPlanes = 1;
	bih.biBitCount = bmp->bpp;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = bmp->linewidth * bmp->height;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = colors;
	bih.biClrImportant = 0;

	/* Select the palette */
	switch (colors) {
		case 2:
			palette = Palette2;
			break;

		case 4:
			palette = Palette4;
			break;

		case 16:
		case 256:
		default:
			palette = Palette256;
			break;
	}

	/* Write the headers and palette */
	fwrite(&bfh, sizeof (BITMAPFILEHEADER), 1, fout);
	fwrite(&bih, sizeof (BITMAPINFOHEADER), 1, fout);
	fwrite(palette, sizeof (RGBQUAD), colors, fout);

	/* Now save the data from the buffer, one line at a time */
	for (y = bmp->height - 1; y >= 0; y--)
		fwrite(bmp->lines[y], bmp->linewidth, 1, fout);

	/* Close the output file and return success */
	fclose(fout);
	return 1;
}

/* Set the global 256-color palette for exporting bitmaps */
int bmp256_setpalette(char *fname) {
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	FILE *fin;

	/* Open the input picture */
	fin = fopen(fname, "rb");
	if (!fin) {
		return 0;
	}

	/* Read the header and make sure it's a real BMP */
	fread(&bfh, sizeof (BITMAPFILEHEADER), 1, fin);
	if (bfh.bfType != BMP_SIG) {
		fclose(fin);
		return 0;
	}

	/* Make sure it's a format we can handle */
	fread(&bih, sizeof (BITMAPINFOHEADER), 1, fin);
	if (bih.biPlanes != 1 || bih.biBitCount != 8 ||
			bih.biCompression != BI_RGB || bih.biHeight < 0) {
		fclose(fin);
		return 0;
	}

	/* Seek to the bitmap color table and read it in to the global palette */
	fseek(fin, sizeof (BITMAPFILEHEADER) + bih.biSize, SEEK_SET);
	fread(Palette256, sizeof (RGBQUAD),
			bih.biClrUsed ? bih.biClrUsed : 1 << bih.biBitCount, fin);

	/* Close the input file and return success */
	fclose(fin);
	return 1;
}

BITMAP256 *bmp256_create(int width, int height, int bpp) {
	BITMAP256 *bmp;
	unsigned int y;

	assert(width >= 0);
	assert(height >= 0);

	/* Allocate the BITMAP structure */
	bmp = (BITMAP256 *) malloc(sizeof (BITMAP256));
	if (!bmp) {
		return NULL;
	}
	bmp->width = width;
	bmp->height = height;

	/* Allow only 1bpp, 2bpp, 4bpp, and 8bpp bitmaps */
	assert((bpp == 1) || (bpp == 2) || (bpp == 4) || (bpp == 8));
	bmp->bpp = bpp;

	/* Calculate line width in bytes, with dword padding */
	bmp->linewidth = (bmp->width * bmp->bpp + 31) >> 3 & ~3;
	bmp->lines = NULL;
	bmp->bits = NULL;

	/* Allocate the pixel buffer */
	bmp->bits = (uint8_t *) malloc(bmp->height * bmp->linewidth *
			sizeof (uint8_t));
	if (!bmp->bits) {
		bmp256_free(bmp);
		return NULL;
	}

	/* Allocate the lines array */
	bmp->lines = (uint8_t **) malloc(bmp->height *
			sizeof (uint8_t *));
	if (!bmp->lines) {
		bmp256_free(bmp);
		return NULL;
	}
	for (y = 0; y < bmp->height; y++)
		bmp->lines[y] = bmp->bits + bmp->linewidth * y;

	/* Clear the bitmap */
	memset(bmp->bits, 0, bmp->linewidth * bmp->height);

	return bmp;
}

int bmp256_getpixel(BITMAP256 *bmp, int x, int y) {
	register int shift;
	if (x < 0 || x >= bmp->width || y < 0 || y >= bmp->height)
		return -1;

	if (bmp->bpp == 1) {
		shift = (~x & 7);
		return (bmp->lines[y][x / 8] >> shift) & 0x01;
	} else if (bmp->bpp == 2) {
		shift = (~x & 3) * 2;
		return (bmp->lines[y][x / 4] >> shift) & 0x03;
	} else if (bmp->bpp == 4) {
		shift = (~x & 1) * 4;
		return (bmp->lines[y][x / 2] >> shift) & 0x0F;
	} else if (bmp->bpp == 8) {
		return bmp->lines[y][x] & 0xFF;
	}

	return 0;
}

int bmp256_putpixel(BITMAP256 *bmp, int x, int y, int c) {
	register int mask, shift;
	if (x < 0 || x >= bmp->width || y < 0 || y >= bmp->height)
		return -1;

	if (bmp->bpp == 1) {
		shift = (~x & 7);
		mask = 0x01 << shift;
		bmp->lines[y][x / 8] &= ~mask;
		bmp->lines[y][x / 8] |= ((c & 1) << shift);
	} else if (bmp->bpp == 2) {
		shift = (~x & 3) * 2;
		mask = 0x03 << shift;
		bmp->lines[y][x / 4] &= ~mask;
		bmp->lines[y][x / 4] |= (c << shift) & mask;
	} else if (bmp->bpp == 4) {
		shift = (~x & 1) * 4;
		mask = 0x0F << shift;
		bmp->lines[y][x / 2] &= ~mask;
		bmp->lines[y][x / 2] |= (c << shift) & mask;
	} else if (bmp->bpp == 8) {
		mask = 0xFF;
		bmp->lines[y][x] = c & mask;
	}

	return c;
}

void bmp256_rect(BITMAP256 *bmp, int x1, int y1, int x2, int y2, int c) {
	unsigned int x, y;

	/* Make some sanity checks */
	if (x1 > x2 || y1 > y2)
		return;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;

	if (x2 >= bmp->width)
		x2 = bmp->width - 1;

	if (y2 >= bmp->height)
		y2 = bmp->height - 1;

	/* Color pixel by pixel */
	for (y = y1; y <= y2; y++)
		for (x = x1; x <= x2; x++)
			bmp256_putpixel(bmp, x, y, c);
}

BITMAP256 *bmp256_duplicate(BITMAP256 *bmp) {
	BITMAP256 *bmp2 = bmp256_create(bmp->width, bmp->height, bmp->bpp);
	if (bmp2) {
		memcpy(bmp2->bits, bmp->bits, bmp->linewidth * bmp->height);
	}

	return bmp2;
}

int bmp256_split(BITMAP256 *bmp, BITMAP256 **red, BITMAP256 **green, BITMAP256 **blue, BITMAP256 **bright) {
	if (bmp->bpp == 4) {
		BITMAP256 * planes[4];
		unsigned int x, y;
		int p, c;

		for (p = 0; p < 4; p++) {
			planes[p] = bmp256_create(bmp->width, bmp->height, 1);

			for (y = 0; y < bmp->height; y++) {
				for (x = 0; x < bmp->width; x++) {
					c = bmp256_getpixel(bmp, x, y);
					c = (c & (1 << p)) ? 1 : 0;
					bmp256_putpixel(planes[p], x, y, c);
				}
			}
		}
		*blue = planes[0];
		*green = planes[1];
		*red = planes[2];
		*bright = planes[3];
		return 1;
	}

	return 0;
}

/* Split variable number of planes into 1bpp bitmaps */
int bmp256_split_ex(BITMAP256 *bmp, BITMAP256 *planes[],
		unsigned planestart, unsigned planecount) {
	return bmp256_split_ex2(bmp, planes, planestart, planecount, 1);
}

/* Split variable number of planes into bitmaps of variable bpp */
int bmp256_split_ex2(BITMAP256 *bmp, BITMAP256 *planes[],
		unsigned planestart, unsigned planecount, unsigned planebpp) {
	unsigned int x, y;
	int p, c, mask, shift;

	/* Check the arguments */
	assert(bmp);
	assert(planebpp >= 1 && planebpp <= 8);
	assert(planestart >= 0);
	assert(planecount*planebpp + planestart <= bmp->bpp);

	/* Split into bitmaps */
	mask = (1<<planebpp)-1;
	for (p = 0; p < planecount; p++) {
		planes[p] = bmp256_create(bmp->width, bmp->height, planebpp);
		if (!planes[p])
			return 0;

		shift = p*planebpp + planestart;

		for (y = 0; y < bmp->height; y++) {
			for (x = 0; x < bmp->width; x++) {
				c = bmp256_getpixel(bmp, x, y);
				c = (c >> shift) & mask;
				bmp256_putpixel(planes[p], x, y, c);
			}
		}
	}

	return 1;
}

BITMAP256 *bmp256_merge_ex(BITMAP256 *planes[], unsigned planecount, unsigned bpp) {
	BITMAP256 *bmp;
	int p, c;
	unsigned x, y, planebpp, planebppmask;

	/* Ensure that the output planecount and bpp make sense,
		 and that the input planes are monochrome and the same size
		 */
	assert(planecount > 0 && planecount <= 8);
	assert(bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8);
	assert(bpp >= planecount);


	for (p = 0; p < planecount; p++) {
		assert(planes[p] && (planes[p]->bpp == 1 || planes[p]->bpp == 2 || (planecount == 1 && planes[p]->bpp == 8)) &&
				planes[p]->bpp == planes[0]->bpp &&
				planes[p]->width == planes[0]->width &&
				planes[p]->height == planes[0]->height);
	}

	bmp = bmp256_create(planes[0]->width, planes[0]->height, bpp);
	if (!bmp)
		return NULL;

	planebpp = planes[0]->bpp;
	planebppmask = (1 << planebpp) - 1;
	for (y = 0; y < bmp->height; y++) {
		for (x = 0; x < bmp->width; x++) {
			for (c = 0, p = 0; p < planecount; p++) {
				c |= (bmp256_getpixel(planes[p], x, y) & planebppmask) << (p * planebpp);
			}
			bmp256_putpixel(bmp, x, y, c);
		}
	}

	return bmp;
}

BITMAP256 *bmp256_merge(BITMAP256 *red, BITMAP256 *green, BITMAP256 *blue, BITMAP256 *bright) {
	BITMAP256 *bmp;
	int p, c;
	unsigned int x, y;
	BITMAP256 * planes[4];

	planes[0] = blue;
	planes[1] = green;
	planes[2] = red;
	planes[3] = bright;

	for (p = 0; p < 4; p++)
		if (!planes[p] || planes[p]->bpp != 1)
			return NULL;

	bmp = bmp256_create(planes[0]->width, planes[1]->height, 8);
	if (!bmp)
		return NULL;

	for (y = 0; y < bmp->height; y++) {
		for (x = 0; x < bmp->width; x++) {
			c = bmp256_getpixel(planes[0], x, y) & 1;
			c |= (bmp256_getpixel(planes[1], x, y) & 1) << 1;
			c |= (bmp256_getpixel(planes[2], x, y) & 1) << 2;
			c |= (bmp256_getpixel(planes[3], x, y) & 1) << 3;
			bmp256_putpixel(bmp, x, y, c);
		}
	}

	return bmp;
}

void bmp256_blit(BITMAP256 *src, unsigned int srcx, unsigned int srcy, BITMAP256 *dest, unsigned int destx, unsigned int desty, unsigned int width, unsigned int height) {
	unsigned int x, y;
	int c;

	/* Sanitise arguments */
	if (srcx > src->width) srcx = src->width - 1;
	if (srcx + width > src->width) width = src->width - srcx;
	if (srcy > src->height) srcy = src->height - 1;
	if (srcy + height > src->height) height = src->height - srcy;

	if (destx > dest->width) destx = dest->width - 1;
	if (destx + width > dest->width) width = dest->width - destx;
	if (desty > dest->height) desty = dest->height - 1;
	if (desty + height > dest->height) height = dest->height - desty;

	/* Copy the data */
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			/* Read a pixel, and convert it between bpp */
			c = bmp256_getpixel(src, srcx + x, srcy + y);
			if (src->bpp == 1 && dest->bpp == 2)
				c = (c == 1 ? 3 : 0);
			else if (src->bpp == 1 && dest->bpp == 4)
				c = (c == 1 ? 15 : 0);
			else if (src->bpp == 1 && dest->bpp == 8)
				c = (c == 1 ? 15 : 0);

			else if (src->bpp == 4 && dest->bpp == 1)
				c = (c > 7 ? 1 : 0);
			else if (src->bpp == 4 && dest->bpp == 8)
				c = c;
			else if (src->bpp == 8 && dest->bpp == 1)
				c = (c > 7 ? 1 : 0);
			else if (src->bpp == 8 && dest->bpp == 4)
				c = (c % 16);

			bmp256_putpixel(dest, destx + x, desty + y, c);
		}
	}
}

// TODO: make this bit indepedent

void bmp256_unpack(BITMAP256 *bmp) {
	/* Unpack a 4bpp bitmap into planes b, g, r, h */
	int unpackw;
	uint8_t **uplines;
	uint8_t *upbits;
	uint8_t *pix;
	int p, x, y, count;
	uint8_t byte;

	unpackw = (bmp->width + 7) / 8;

	upbits = malloc(unpackw * bmp->height * 4 * sizeof ( uint8_t));
	uplines = malloc(bmp->height * 4 * sizeof ( uint8_t *));

	for (p = 0; p < 4; p++) {
		for (y = 0; y < bmp->height; y++) {
			uplines[p * bmp->height + y] = upbits + (p * unpackw * bmp->height) +
				(y * unpackw);

			pix = uplines[p * bmp->height + y];
			count = 0;
			byte = 0;
			for (x = 0; x < bmp->width; x++) {
				byte <<= 1;
				byte |= ((bmp256_getpixel(bmp, x, y) & (1 << p)) ? 1 : 0);
				count++;
				if (count == 8) {
					/* output the byte */
					*pix++ = byte;
					count = 0;
					byte = 0;
				}
			}
			if (count != 0) {
				/* output the byte */
				*pix++ = byte;
			}
		}
	}

	free(bmp->lines);
	free(bmp->bits);

	bmp->bits = upbits;
	bmp->lines = uplines;

	bmp->linewidth = unpackw;
	//bmp->planar = 1;
}

// Split one bitmap into several planes, pixel-by-pixel
int bmp256_munge(BITMAP256 *bmp, BITMAP256 *planes[], unsigned planecount) {

	uint8_t *srcline, *dstline;
	int x,y,p;

	assert (planecount > 0);

	// Ensure that the width of demunged bitmap is multiple of plane number
	assert (bmp->width % planecount == 0);

	for (p = 0; p < planecount; p++) {
		planes[p] = bmp256_create(bmp->width/planecount, bmp->height, bmp->bpp);

		for (y = 0; y < planes[p]->height; y++) {
			srcline = bmp->lines[y] + p;
			dstline = planes[p]->lines[y];

			for (x = 0; x < planes[p]->width; x++) {
				*dstline = *srcline;
				dstline++;
				srcline += planecount;
			}
		}
	}

	return 1;
}

// Merge several planes into one bitmap, pixel-by-pixel
BITMAP256 *bmp256_demunge(BITMAP256 *planes[], unsigned planecount, int bpp) {

	BITMAP256 *bmp;
	int pwidth, height, width;
	int x,y,p;
	uint8_t *dstline, *srcline;

	assert (planecount > 0);
	
	// Ensure planes are of equal dimension and bpp
	pwidth = planes[0]->width;
	height = planes[0]->height;
	for (p = 0; p < planecount; p++) {
		assert(planes[p]->bpp == bpp);
		assert(planes[p]->width == pwidth);
		assert(planes[p]->height == height);
	}

	width = pwidth * planecount;

	bmp = bmp256_create(width, height, bpp);

	for (p = 0; p < planecount; p++) {
		for (y = 0; y<height; y++) {
			dstline = bmp->lines[y] + p;
			srcline = planes[p]->lines[y];

			for (x = 0; x<pwidth; x++) {
				*dstline = *srcline;
				dstline += planecount;
				srcline++;
			}
		}
	}

	return bmp;

}

