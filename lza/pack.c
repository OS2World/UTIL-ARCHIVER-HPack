/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  HPACK Arithmetic Encoder						*
*							 PACKF.C  Updated 06/11/91						*
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
  #include "io\fastio.h"
#elif defined( __MAC__ )
  #include "fastio.h"
#else
  #include "io/fastio.h"
#endif /* System-specific include directives */

/* Count of no.bytes output */

extern long outByteCount;

/****************************************************************************
*																			*
*								Bit Output Routines							*
*																			*
****************************************************************************/

/* The bit buffer */

static BYTE outBuffer;			/* Bit buffer for output */
static int outBitsToGo;			/* Number of bits free in buffer */

/* Output a bit */

static void outBit( const int bit )
	{
	/* Put bit in buffer */
	outBuffer <<= 1;
	outBuffer |= bit;

	/* Output buffer if it is now full */
	if( !--outBitsToGo )
		{
		fputByte( outBuffer );
		outByteCount++;
		outBitsToGo = 8;
		}
	}

/****************************************************************************
*																			*
*							Arithmetic Coding Routines						*
*																			*
****************************************************************************/

/* The current state of the encoding */

WORD low, high;		/* Ends of the current code region */
int bitsToFollow;	/* No.opposing bits to output after the next bit */

/* Initialise variables used in the arithmetic coding */

void startEncode( void )
	{
	low = 0;
	high = 0xFFFF;			/* Full code range */
	bitsToFollow = 0;		/* No bits to follow next */

	/* Set the bit buffer to empty */
	outBitsToGo = 8;
	}

/* Encode a symbol */

void encode( const int cumFreq0,
			 const int cumFreqLower, const int cumFreqUpper )
	{
	BYTE highBit;
	LONG range = ( LONG ) ( high - low ) + 1L;	/* Size of curr.code region */

	/* Narrow the code region to that allotted to this symbol */
	high = low + ( WORD ) ( ( range * cumFreqLower ) / cumFreq0 - 1 );
	low += ( WORD ) ( ( range * cumFreqUpper ) / cumFreq0 );

	while( TRUE )
		{
		if( ( high & 0x8000 ) == ( low & 0x8000 ) )
			{
			highBit = ( high & 0x8000 ) ? 1 : 0;
			outBuffer <<= 1;
			outBuffer |= highBit;
			if( !--outBitsToGo )
				{
				fputByte( outBuffer );
				outByteCount++;
				outBitsToGo = 8;
				}
			while( bitsToFollow )
				{
				outBuffer <<= 1;
				outBuffer |= !highBit;
				if( !--outBitsToGo )
					{
					fputByte( outBuffer );
					outByteCount++;
					outBitsToGo = 8;
					}
				bitsToFollow--;
				}
			}
		else
			if( ( low & 0x4000 ) && !( high & 0x4000 ) )
					{
					/* Output an opposite bit later if in middle half */
					bitsToFollow++;
					low &= 0x3FFF;
					high |= 0x4000;
					}
				else
					break;

		/* Now scale up code range */
		low <<= 1;			/* low = 2 * low */
		high <<= 1;			/* 2 * high + 1 */
		high++;
		}
	}

/* Finish encoding the stream by outputting two bits that select the quarter
   that the current code range contains */

void endEncode( void )
	{
	BYTE bit = ( low & 0x4000 ) ? 0 : 1, padding;
	static BYTE padBits[] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

	bitsToFollow++;
	outBit( bit );
	while( bitsToFollow-- )
		outBit( !bit );

	/* Flush out the last bits, padding with ones or zeroes depending on
	   the value of the last bit */
	if( outBitsToGo < 8 )
		{
		padding = ( outBuffer & 1 ) ? padBits[ outBitsToGo ] : 0;
		fputByte( ( BYTE ) ( ( outBuffer << outBitsToGo ) | padding ) );
		outByteCount++;
		}
	}
