/* HUFF.H - Huffman compression and decompression routines - header file.
**
** Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
** printf parameters modified for LModKeen 2 integration.
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

#ifndef INC_HUFF_H__
#define INC_HUFF_H__

void huff_expand(unsigned char *pin, unsigned char *pout, unsigned long inlen, unsigned long outlen);
unsigned long huff_compress(unsigned char *pin, unsigned char *pout, unsigned long inlen, unsigned long outlen);
void huff_read_dictionary(FILE *fin, unsigned long offset);
void huff_setup_compression();

#endif /* !INC_HUFF_H__ */
