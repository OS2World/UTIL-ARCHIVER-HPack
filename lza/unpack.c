/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  HPACK Arithmetic Decode Routines					*
*							UNPACK.C  Updated 03/06/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990, 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#include "defs.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "io\fastio.h"
  #include "lza\model.h"
#elif defined( __MAC__ )
  #include "fastio.h"
  #include "model.h"
#else
  #include "crc/crc16.h"
  #include "io/fastio.h"
  #include "lza/model.h"
#endif /* System-specific include directives */

/* The following are defined in LZA.C */

extern LONG dataLen;	/* The data byte count */

/****************************************************************************
*																			*
*						Arithmetic Decoding Routine							*
*																			*
****************************************************************************/

/* Current state of the decoding */

static WORD value;		/* Currently seen code value */
static WORD low, high;	/* Ends of the current code region */

/* The bit buffer */

BYTE inBuffer;			/* Bit buffer for input */
int inBitsToGo;			/* Number of bits available in buffer */

/* Start decoding a stream of symbols */

void startDecode( void )
	{
	/* Initialize the bit input routines.  Since this is dual-use code
	   we can only read in 8 bits of a possible 16.  Note that the read of
	   the 16-bit value must be spread over two lines since both sides of
	   a single-line '|' have side effects */
	value = ( ( WORD ) fgetByte() ) << 8;
	value |= fgetByte();
	inBuffer = fgetByte();
	inBitsToGo = 8;
	dataLen -= 3;				/* We've read in 3 bytes so far */

	low = 0;					/* The full code range */
	high = 0xFFFF;
	}

/* Loop to get rid of bits */

static void discardBits( void )
	{
	/* Loop to get rid of bits */
	while( TRUE )
		{
		/* If the MS digits match, shift out the bits */
		if( !( ( high ^ low ) & 0x8000 ) )
			;
		else
			/* If there is a danger of underflow, shift out the 2nd MS digit */
			if( ( low & ~high ) & 0x4000 )
				{
				value ^= 0x4000;
				low &= 0x3FFF;
				high |= 0x4000;
				}
			else
				/* We can't shift anything out, so we return */
				break;

		/* Scale up code range */
		low <<= 1;
		high <<= 1;
		high++;

		/* Move in next input bit */
		if( !inBitsToGo )
			{
			if( !dataLen )
				/* Out of input data, extend last bit in buffer into all of
				   inBuffer */
				inBuffer = ( value & 0x01 ) ? 0xFF : 0;
			else
				{
				inBuffer = fgetByte();
				dataLen--;
				}
			inBitsToGo = 8;
			}
		value = ( value << 1 ) | ( ( inBuffer & 0x80 ) ? 1 : 0 );
		inBuffer <<= 1;
		inBitsToGo--;
		}
	}

/* Decode the next symbol (order 1) */

int decodeOrder1( void )
	{
	int cum;				/* Cum.freq.calculated */
	int symbol;				/* Symbol decoded */
	long h427;
	WORD cumFreq0, h318, h172, h628;
	int h198, h027;

	h387 = h246; if( h217[ h387 ] != -1 )
		{
		h198 = h217[ h387 ]; h628 = h528[ h198 ].h506;
		h427 = ( long ) ( high - low ) + 1L;
		h027 = ( ( ( long ) ( value - low ) + 1L ) * h628 - 1 ) / h427;
		h198 = h528[ h198 ].h261; while( h528[ h198 ].h506 > h027 )
		h198 = h528[ h198 ].h261; h318 = h528[ h528[ h198 ].h704 ].h506;
		h172 = h528[ h198 ].h506; high = low + ( h427 * h318 ) / h628 - 1;
		low += ( h427 * h172 ) / h628;

		/* Get rid of input bits */
		discardBits();

		if( h528[ h528[ h198 ].h704 ].h462 ) {
		h246 = h027 = h648[ h198 ].h506; h508( h198 );
		h797( h198, PACK_MASK ); return( h027 ); } else h508( h198 );
		}

	/* Get total cumFreq0 for order 0 model */
	cumFreq0 = h313[ 0 ];

	/* Check for special case of range = 0x10000L */
	if( !low && high == 0xFFFF )
		{
		/* Find cumulative frequency for value */
		cum = ( ( ( long ) ( value - low ) + 1 ) * cumFreq0 - 1 ) >> 16;

		/* Then find the symbol */
		for( symbol = 1; h313[ symbol ] > cum; symbol++ );

		/* Narrow the code region to that allotted to this symbol */
		high = low + ( 0x10000L * h313[ symbol - 1 ] ) / cumFreq0 - 1;
		low += ( 0x10000L * h313[ symbol ] ) / cumFreq0;
		}
	else
		{
		/* Find cumulative frequency for value */
		WORD range = ( high - low ) + 1;

		cum = ( ( ( long ) ( value - low ) + 1 ) * cumFreq0 - 1 ) / range;

		/* Then find the symbol */
		for( symbol = 1; h313[ symbol ] > cum; symbol++ );

		/* Narrow the code region to that allotted to this symbol */
		high = low + ( ( long ) range * h313[ symbol - 1 ] ) / cumFreq0 - 1;
		low += ( ( long ) range * h313[ symbol ] ) / cumFreq0;
		}

	/* Get rid of the extra bits */
	discardBits();

	/* Translate symbol to actual value */
	h246 = h027 = h284[ symbol ]; h762( ( WORD ) ( ( ( h387 << 4 ) & 0x0FF0 ) ^ h246 ) );
	return( h027 );
	}

/* Decode the next symbol (order 0) */

int decodeOrder0( const int *cumFreq )
	{
	long range;				/* Size of current code region */
	int cum;				/* Cum.freq.calculated */
	int symbol;				/* Symbol decoded */

	/* Find cum.freq. for value */
	range = ( long ) ( high - low ) + 1;
	cum = ( ( ( long ) ( value - low ) + 1 ) * cumFreq[ 0 ] - 1 ) / range;

	/* Then find the symbol */
	for( symbol = 1; cumFreq[ symbol ] > cum; symbol++ );

	/* Narrow the code region to that allotted to this symbol */
	high = low + ( range * cumFreq[ symbol - 1 ] ) / cumFreq[ 0 ] - 1;
	low = low + ( range * cumFreq[ symbol ] ) / cumFreq[ 0 ];

	/* Get rid if input bits */
	discardBits();

	return( symbol );
	}
