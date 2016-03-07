/* HUFF.C - Huffman compression and decompression routines.
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

#include <stdio.h>
#include "pconio.h"

typedef struct
{
	unsigned short bit0;
	unsigned short bit1;
} nodestruct;

typedef struct {
	int num;
	unsigned long bits;
} compstruct;

static nodestruct nodes[255];
static compstruct comptable[256];


/* Expand huffman-compressed input file into output buffer */
void huff_expand(unsigned char *pin, unsigned char *pout, unsigned long inlen, unsigned long outlen)
{
	unsigned short curnode;
	unsigned long incnt = 0, outcnt = 0;
	unsigned char c, mask;
	unsigned short nextnode;

	curnode = 254; /* Head node */

	do
	{
		mask = 1;
		c = *(pin++);
		incnt++;

		do
		{
			if(c & mask)
				nextnode = nodes[curnode].bit1;
			else
				nextnode = nodes[curnode].bit0;


			if(nextnode < 256)
			{
				/* output a char and move back to the head node */
				*(pout++) = nextnode;
				outcnt++;
				curnode = 254;
			}
			else
			{
				/* move to the next node */
				curnode = nextnode & 0xFF;
			}
			/* Move to consider the next bit */
			mask <<= 1;
		}
		while(outcnt < outlen && mask != 0);

	}
	while(incnt < inlen && outcnt < outlen);
}


/* Read the huffman dictionary from a file */
void huff_read_dictionary(FILE *fin, unsigned long offset)
{
	fseek(fin, offset, SEEK_SET);
	fread(nodes, sizeof(nodestruct), 255, fin);
}

void trace_node(int curnode, int numbits, unsigned long curbits)
{
	int bit0, bit1;

	if(curnode < 256)
	{
		/* This is a character */
		comptable[curnode].num = numbits;
		comptable[curnode].bits = curbits;
		if( numbits > 32 )
			do_output("HUFF: Comptable only allows 32 bits max for node %d!\n", curnode);
	}
	else
	{
		/* This is another node */
		bit0 = nodes[curnode & 0xFF].bit0;
		bit1 = nodes[curnode & 0xFF].bit1;
		numbits++;
		
		trace_node(bit0, numbits, curbits);
		trace_node(bit1, numbits, (curbits | (1UL << (numbits - 1))));
	}
}

void huff_setup_compression()
{
	/* Trace down the Huffman tree, recording the bits into the relevant compstruct entry. */
	trace_node((254 | 256), 0, 0);
}

/* Compress data using huffman dictionary from input buffer into output buffer */
unsigned long huff_compress(unsigned char *pin, unsigned char *pout, unsigned long inlen, unsigned long outlen)
{
	unsigned long outcnt;
	unsigned long incnt;
	unsigned char cout;
	unsigned long bits;
	int numbitsin, numbitsout, numbits;
	unsigned char c;

	incnt = outcnt = 0;
	numbitsout = 0;
	cout = 0;
	do {
		c = *(pin++);
		incnt++;
      
		bits = comptable[c].bits;
		numbits = comptable[c].num;
		
		numbitsin = 0;
      
		do {
			/* Output a bit to the buffer */
			cout >>= 1;
			cout |= (unsigned char)((bits & 1) << 7);
			bits >>= 1;
			
			numbitsout++;
			numbitsin++;
			
			if(numbitsout == 8)
			{
				*(pout++) = cout;
				outcnt++;
				numbitsout = 0;
				cout = 0;
			}
		} while(numbitsin < numbits && outcnt < outlen);
	} while(incnt < inlen && outcnt < outlen);
   
	/* Output any remaining bits */
	if(numbitsout > 0 && outcnt < outlen)
	{
		cout >>= (8 - numbitsout);
		*(pout++) = cout;
		outcnt++;
	}
   
	return outcnt;
}

