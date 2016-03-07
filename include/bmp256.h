/* BMP256.H - 1-bit, 4-bit, and 8-bit BMP manipulation routines - header file.
**
** Original code Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
** FIN2BMP compatibility code added by Ignacio R. Morelle "Shadow Master"
** (shadowm2006@gmail.com), the code is Copyright (c)2002 Andrew Durdin
** 8-bpp support added by Owen Pierce (owen.m.pierce@gmail.com)
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

#ifndef INC_BMP256_H__
#define INC_BMP256_H__
struct BITMAP256
{
    unsigned int width;
    unsigned int height;
    unsigned long linewidth;    /* width of line in bytes (includes padding) */
    unsigned int bpp;
    unsigned int colours;
    unsigned char **lines;
    unsigned char *bits;
};
typedef struct BITMAP256 BITMAP256;


void bmp256_free(BITMAP256 *bmp);
BITMAP256 *bmp256_create(int width, int height, int bpp);
BITMAP256 *bmp256_load(char *fname);
int bmp256_save(BITMAP256 *bmp, char *fname, int backup);
int bmp256_setpalette (char *fname);
int bmp256_getpixel(BITMAP256 *bmp, int x, int y);
int bmp256_putpixel(BITMAP256 *bmp, int x, int y, int c);
void bmp256_rect(BITMAP256 *bmp, int x1, int y1, int x2, int y2, int c);
BITMAP256 *bmp256_duplicate(BITMAP256 *bmp);
int bmp256_split(BITMAP256 *bmp, BITMAP256 **red, BITMAP256 **green, BITMAP256 **blue, BITMAP256 **bright);
int bmp256_split_ex (BITMAP256 *bmp, BITMAP256 *planes[], unsigned planestart, unsigned planecount);
int bmp256_split_ex2 (BITMAP256 *bmp, BITMAP256 *planes[], unsigned planestart, unsigned planecount, unsigned planebpp);
BITMAP256 *bmp256_merge(BITMAP256 *red, BITMAP256 *green, BITMAP256 *blue, BITMAP256 *bright);
BITMAP256 *bmp256_merge_ex(BITMAP256 *planes[], unsigned planecount, unsigned bpp);
void bmp256_blit(BITMAP256 *src, unsigned int srcx, unsigned int srcy, BITMAP256 *dest, unsigned int destx, unsigned int desty, unsigned int width, unsigned int height);
void bmp256_unpack( BITMAP256 *bmp );
BITMAP256 *bmp256_demunge(BITMAP256 *planes[], unsigned planecount, int bpp);
int bmp256_munge(BITMAP256 *bmp, BITMAP256 *planes[], unsigned planecount);

#endif /* !INC_BMP256_H__ */
