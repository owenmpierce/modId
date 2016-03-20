/* HUFF.C - Huffman compression and decompression routines.
**
** Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
** printf parameters modified for LModKeen 2 integration.
** huffmanize function imported and modified from TED5.
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
#include "utils.h"

typedef struct
{
	unsigned short bit0;
	unsigned short bit1;
} nodestruct;

typedef struct {
	int num;
	unsigned long bits;
} compstruct;

static nodestruct nodes[256]; /* Only 255 should be used, but xGADICT generally has an additional zero node */
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

/* Write the huffman dictionary to a file */
void huff_write_dictionary(FILE *fout)
{
	fwrite(nodes, sizeof(nodestruct), 256, fout); // Includes last zero node
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

/* Takes the counts array and builds a huffman tree at nodes array. */

void huffmanize(int counts[])
{
	/* codes are either bytes if <256 or nodearray numbers+256 if >=256 */
	unsigned short value[256],code0,code1;
	/* probablilities are the number of times the code is hit or $ffffffff if
	it is allready part of a higher node */
	unsigned int prob[256],low/*,workprob*/;

	short i,worknode/*,bitlength*/;
	/*unsigned int bitstring;*/


	/* all possible leaves start out as bytes */
	for (i=0; i<256; i++)
	{
		value[i] = i;
		prob[i] = counts[i];
	}

	/* start selecting the lowest probable leaves for the ends of the tree */

	worknode = 0;
	while (1)	/* break out of when all codes have been used */
	{
		/* find the two lowest probability codes */

		code0=0xffff;
		low = 0x7fffffff;
		for (i=0;i<256;i++)
			if (prob[i]<low)
			{
				code0 = i;
				low = prob[i];
			}

		code1=0xffff;
		low = 0x7fffffff;
		for (i=0;i<256;i++)
			if (prob[i]<low && i != code0)
			{
				code1 = i;
				low = prob[i];
			}

		if (code1 == 0xffff)
		{
			if (value[code0]<256)
				quit("Wierdo huffman error: last code wasn't a node!");
			if (value[code0]-256 != 254)
				quit("Wierdo huffman error: headnode wasn't 254!");
			break;
		}

		/* make code0 into a pointer to work
		remove code1 (make 0xffffffff prob) */
		nodes[worknode].bit0 = value[code0];
		nodes[worknode].bit1 = value[code1];

		value[code0] = 256 + worknode;
		prob[code0] += prob[code1];
		prob[code1] = 0xffffffff;
		worknode++;
	}
}

void huff_setup_compression()
{
	/* Trace down the Huffman tree, recording the bits into the relevant compstruct entry. */
	trace_node((254 | 256), 0, 0);
}

/* Compress data using huffman dictionary from input buffer into output buffer */
unsigned long huff_compress(unsigned char *pin, unsigned char *pout, unsigned long inlen, unsigned long outlen, int igrabhufftrailmode)
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
   
	/* Output any remaining bits, or even just an additional    */
	/* trailing zero byte, based on value of igrabhufftrailmode */
	/* (used for emulating a few variants of an IGRAB quirk)    */
	if (((numbitsout > 0) ||
	     (igrabhufftrailmode == 1) ||
	     ((igrabhufftrailmode == 2) && (inlen < 60000))
	    ) && (outcnt < outlen)
	) {
		cout >>= (8 - numbitsout);
		*(pout++) = cout;
		outcnt++;
	}
   
	return outcnt;
}

