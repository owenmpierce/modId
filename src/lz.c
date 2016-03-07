/* LZ.C - Keen 1-compatible LZ compression and decompression routines.
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
#include <stdlib.h>

#include "pconio.h"

/* Helper functions for compression and decompression */
int lzd_first_char( int code );
int lzd_write_string( char **pout, int code, int ch );
int lzd_read_code( FILE *fin, int NumBits, int reset );
long lze_write_code( FILE *fout, int code, int NumBits, int reset );
int lze_add_key( int str, int ch, int code );
int lze_in_table( int str, int ch );

//Reads NumBits from file #inf, returning them in order in a Long
int lzd_read_code( FILE *fin, int NumBits, int reset ) {
    static int BitMask, BitByte;
    int BitCount = 0, BitBuffer = 0;

    if( reset ) {
        BitMask = BitByte = 0;
        return 0;
    }

    while( BitCount < NumBits ) {
        //Read 8 bits from the file when we need to
        if( !BitMask ) {
            BitByte = fgetc( fin );
            BitMask = 128;
        }

        //Take bits out of BitByte and shove them in BitBuffer
        if( BitByte & BitMask )
            BitBuffer |= 1 << (NumBits - BitCount - 1);
        //Increment the number of bits we've read
        BitCount++;
        //Move the mask to the next bit
        BitMask >>= 1;
    }
    return BitBuffer;
}

#define MAXBITS 12
#define TABLESIZE (1 << MAXBITS)
#define LZ_ERROR    256
#define LZ_EOF        257
#define LZ_START    258
static int lz_key_table[2*TABLESIZE];    /* Used only by lz_encode */
static int lz_code_table[TABLESIZE];
static int lz_char_table[TABLESIZE];

long lz_decompress( FILE *fin, char *pout ) {
    int i, LastCode, NewCode, NumBits;
    int CurEntry = 0;
    int CurStr = 0; /* Note that CurStr is an index into the tables */
    long OutLen = 0;
    int CurChar = 0;

    /* We start with 9-bit codes */
    NumBits = 9;

    /* Reset the reader */
    lzd_read_code( NULL, 0, 1 );

    /* Initialise the string tables */
    for( i = 0; i < 256; i++ ) {
        lz_code_table[i] = -1;
        lz_char_table[i] = i;
    }
    for( i = 256; i < TABLESIZE; i++ ) {
        lz_code_table[i] = -1;
        lz_char_table[i] = -1;
    }

    /* Start adding items to the table at index 258 */
    /* because 256 is reserved as ERROR (Guess), and */
    /* index 257 is reserved as EOF. */
    CurEntry = LZ_START;

    //cW := the first code word in the codestream (it denotes a root);
    NewCode = lzd_read_code( fin, NumBits, 0 );

    //output the string.cW to the charstream;
    OutLen += lzd_write_string( &pout, lz_code_table[ NewCode ],
        lz_char_table[ NewCode ] );

    do {
        //pW := cW;
        LastCode = NewCode;

        //cW := next code word in the codestream;
        NewCode = lzd_read_code( fin, NumBits, 0 );
        /* Check for error code (256) or EOF code (257) */
        if( NewCode == LZ_ERROR || NewCode == LZ_EOF )
            break;

        //Is the string.cW present in the dictionary?
        if( lz_char_table[ NewCode ] != -1 ) {
            //if it is, output the string.cW to the charstream;
            OutLen += lzd_write_string( &pout, lz_code_table[ NewCode ],
                lz_char_table[ NewCode ] );

            //P := string.pW;
            //C := the first character of the string.cW;
            CurStr = LastCode;
            CurChar = lzd_first_char( NewCode );
        } else {
            //if not,
            //P := string.pW;
            //C := the first character of the string.pW;
            CurStr = LastCode;
            CurChar = lzd_first_char( LastCode );
            //output the string P+C to the charstream
            //   (now it corresponds to the cW);
            OutLen += lzd_write_string( &pout, CurStr, CurChar );
        }

        if( CurEntry < TABLESIZE ) {
            //add the string P+C to the dictionary;
            lz_code_table[ CurEntry ] = CurStr;
            lz_char_table[ CurEntry ] = CurChar;

            CurEntry++;
            //Increase the bit size as necessary
            if( CurEntry == (1 << NumBits) - 1 && NumBits < MAXBITS )
                NumBits++;
        }

        /* Continue until we reach the EOF code (checked above) */
    } while( 1 );

    /* If there was an error, return a negative count */
    if( NewCode == LZ_ERROR )
        return -OutLen;
    else
        return OutLen;
}

int lzd_first_char( int icode ) {
    while( lz_code_table[ icode ] != -1 )
        icode = lz_code_table[ icode ];

    return lz_char_table[ icode ];
}

int lzd_write_string( char **pout, int icode, int ichar ) {
    int count = 0, i;
    static int stack[128];
    int sp = 0;

    /* Put the string onto the stack */
    do {
        /* Make sure we're not about to overrun the stack */
        if( sp >= 128 ) {
            do_output("LZ: output stack overflow\n");
            exit( 1 );
        }

        /* Shove it onto the stack */
        stack[sp++] = ichar;
        count++;

        /* See if we've reached the end of the string */
        if( icode != -1 ) {
            ichar = lz_char_table[ icode ];
            icode = lz_code_table[ icode ];
        } else
            break;
    } while( 1 );

    /* Pull the string back off the stack and output it */
    for( i = count; i > 0; i-- ) {
        ichar = stack[--sp];
        *(*pout)++ = ichar;
   }

    /* Return the number of characters output */
    return count;
}

long lz_compress( FILE *fin, FILE *fout ) {
    int i, NumBits;
    int CurEntry = 0;
    int CurStr = -1; /* Note that CurStr is an index into the tables */
    long OutLen = 0;
    int CurChar = 0;
    int Key;

    /* We start with 9-bit codes */
    NumBits = 9;

    /* Reset the writer */
    lze_write_code( NULL, 0, 0, 1 );

    /* Initialise the string tables */
    for( i = 0; i < 2*TABLESIZE; i++ )
        lz_key_table[i] = -1;
    for( i = 0; i < 256; i++ ) {
        lz_code_table[i] = -1;
        lz_char_table[i] = i;
    }
    for( i = 256; i < TABLESIZE; i++ ) {
        lz_code_table[i] = -1;
        lz_char_table[i] = -1;
    }

    /* Start adding items to the table at index 258 */
    /* because 256 is reserved as ERROR (Guess), and */
    /* index 257 is reserved as EOF. */
    CurEntry = LZ_START;

    //C := next character in the charstream;
    CurChar = fgetc( fin );
    CurStr = -1;
    while( CurChar != EOF ) {

        //Is the string P+C present in the dictionary?
        Key = lze_in_table( CurStr, CurChar );
        if( Key != -1 ) {
            //P := P+C (extend P with C);
            CurStr = Key;
        } else {
            if( OutLen > 0x120 )
                CurStr = CurStr;

            //i.output the code word which denotes P to the codestream;
            OutLen += lze_write_code( fout, CurStr, NumBits, 0 );
            //ii.add the string P+C to the dictionary;
            if( CurEntry < TABLESIZE ) {
                //add the string P+C to the dictionary;
                lze_add_key( CurStr, CurChar, CurEntry );
                lz_code_table[ CurEntry ] = CurStr;
                lz_char_table[ CurEntry ] = CurChar;
                CurEntry++;
                //Increase the bit size as necessary
                if( CurEntry == (1 << NumBits) && NumBits < MAXBITS )
                    NumBits++;
            }
            //iii.P := C (P now contains only the character C);
            CurStr = CurChar;
        }

        //C := next character in the charstream;
        CurChar = fgetc( fin );
    }
    //i.output the code word which denotes P to the codestream;
    OutLen += lze_write_code( fout, CurStr, NumBits, 0 );
    /* Output the end-of-file marker */
    OutLen += lze_write_code( fout, LZ_EOF, NumBits, 0 );

    return OutLen;
}

long lze_write_code( FILE *fout, int code, int NumBits, int reset ) {
    static int BitMask = 128, BitByte;
    int BitCount = 0;
    int BytesWritten = 0;

    if( reset ) {
        BitMask = 128;
        BitByte = 0;
        return 0;
    }

    //Cache the bits from the code into a byte
    //and output it when it gets full

    while( BitCount < NumBits ) {
        //Take bits out of NumBits and shove them in BitByte
        if( code & (1 << (NumBits - BitCount - 1)) )
            BitByte |= BitMask;
        //Increment the number of bits we've read
        BitCount++;
        //Move the mask to the next bit
        BitMask >>= 1;

        //Write 8 bits to the file when we need to
        if( BitMask == 0 ) {
            fputc( BitByte, fout );
            BytesWritten++;
            BitByte = 0;
            BitMask = 128;
        }
    }
    /* If we're writing the EOF code, make sure the whole thing gets out */
    if( code == LZ_EOF && BitMask != 128 ) {
        fputc( BitByte, fout );
        BytesWritten++;
    }
    return BytesWritten;
}


#define HASH(x,y) ((x)|((y)<<4))

int lze_in_table( int str, int ch ) {
    int hash, index;

    /* If there's no preceding str, then we have a single char */
    if( str == -1 )
        return ch;

    hash = HASH( str, ch );
    index = lz_key_table[ hash ];
    while( index != -1 && (str != lz_code_table[index] ||
            ch != lz_char_table[index]) ) {
        hash = (hash+1) & (2*TABLESIZE-1);
        index = lz_key_table[ hash ];
    }
    return index;
}

int lze_add_key( int str, int ch, int code ) {
    int hash, index;

    hash = HASH( str, ch );
    index = lz_key_table[ hash ];
    while( index != -1 ) {
        hash++;
        index = lz_key_table[ hash ];
    }
    lz_key_table[ hash ] = code;
    lz_code_table[ code ] = str;
    lz_char_table[ code ] = ch;

    return hash;
}
