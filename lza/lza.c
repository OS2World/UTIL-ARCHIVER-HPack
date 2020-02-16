/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					   		  LZA Main Compressor Code						*
*							  LZA.C  Updated 08/04/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* "That is not dead which can eternal run
	And over strange eons even HPACK may finish compressing"

		- Apologies to H.P.Lovecraft */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "choice.h"
#include "error.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "crypt\crypt.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
  #include "lza\model.h"
  #include "lza\model2.h"
  #include "lza\model3.h"
  #include "lza\model4.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "crypt.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
  #include "model.h"
  #include "model2.h"
  #include "model3.h"
  #include "model4.h"
#else
  #include "crc/crc16.h"
  #include "crypt/crypt.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
  #include "lza/model.h"
  #include "lza/model2.h"
  #include "lza/model3.h"
  #include "lza/model4.h"
#endif /* System-specific include directives */

/* Prototypes for functions in GUI.C */

void updateProgressReport( void );

/* Prototypes for the arithmetic coder */

void startEncode( void );
void endEncode( void );
void encode( const int cumFreq0, const int cumFreqLower, const int cumFreqUpper );

void startDecode( void );
int decodeOrder1( void );
int decodeOrder0( const int *cumFreq );

/* Prototypes for the models for various encodings */

void startLiteralModel( void );

/* Prototypes for the order-1 arithcoder */

void haveAGoAt( const int value );

#ifdef __MAC__

/* Prototypes for Mac-specific routines */

void yieldTimeSlice( void );

#endif /* __MAC__ */

/* LZSS Coding Parameters */

#define MAX_POSITION		32768L	/* Window size - must be a power of 2 */
#define MAX_LENGTH			64		/* Max. match length - must be pwr.of 2 */

#define STR_BUF_SIZE		MAX_POSITION	/* Size of string buffer */
#define STR_BUF_MASK		( STR_BUF_SIZE - 1 )	/* For fast "% STR_BUF_SIZE" */
#define LOOKAHEAD_BUF_SIZE	MAX_LENGTH		/* Size of look-ahead buffer */
#define LOOKAHEAD_BUF_MASK	( LOOKAHEAD_BUF_SIZE - 1 )	/* For fast "% LOOKAHEAD_BUF_SIZE" */
#define MATCH_THRESHOLD		5		/* When changing this value, remember to
									   change NO_SYMBOLS in model.h, and the
									   hashTable size in model.c */
#define LZSS_ESC			256		/* Escape value for LZSS code */

#define LIST_SIZE			8192	/* The number of hashed list headers */
#define LIST_END			-1		/* Marker for end of list */

static BYTE *textBuffer, *lookAheadBuffer;	/* The string and lookahead buffers */
static int *linkBuffer, *tailList;	/* The list1 data structure */
int *headList;						/* Must be externally visible for findMatch() */

/* The output byte count (used by PACK.C, UNPACK.C) */

LONG outByteCount;

/* The count of the number of bytes processed for the variable-adaptivity
   count scaling */

unsigned int modelByteCount;

/* The byte count for which we bother updating modelByteCount.  Some
   compilers can't handle 'U' so we declare it as 'L' instead.  If possible
   it should be declared as 'U' for 16-bit systems */

#ifdef IRIX
  #define SCALE_RANGE		40000L
#else
  #define SCALE_RANGE		40000U
#endif /* IRIX */

/****************************************************************************
*																			*
*								Encoding Routines							*
*																			*
****************************************************************************/

WORD isText;	/* Actually a BOOLEAN but WORD makes it faster to handle */
int windowSize;	/* Whether we're using an 8K, 16K or 32K window */

/* Set up the models for the various encoders */

void startModels( void )
	{
	/* Set up the models for the encoder */
	h291();
	startHighPosModel();
	startLowPosModel();
	startLengthModel();

	/* Reset the byte count for the count scaling */
	modelByteCount = 0;
	}

/* Encode a literal */

#define encodeLiteral(value)	haveAGoAt( value )

/* Encode a high position */

void encodeHighPos( const int theCode )
	{
	const int symbol = highPosToIndex[ theCode ];	/* Translate code to an index */

	encode( highPosCumFreq[ 0 ], highPosCumFreq[ symbol - 1 ], highPosCumFreq[ symbol ] );

	/* Now update the model */
	updateHighPosModel( symbol );
	}

/* Encode a low position */

void encodeLowPos( const int theCode )
	{
	const int symbol = lowPosToIndex[ theCode ];	/* Translate code to an index */

	encode( lowPosCumFreq[ 0 ], lowPosCumFreq[ symbol - 1 ], lowPosCumFreq[ symbol ] );

	/* Now update the model */
	if( !isText )
		updateLowPosModel( symbol );
	}

/* Encode a length */

void encodeLength( const int theCode )
	{
	const int symbol = lengthToIndex[ theCode ];	/* Translate code to an index */

	encode( lengthCumFreq[ 0 ], lengthCumFreq[ symbol - 1 ], lengthCumFreq[ symbol ] );

	/* Now update the model */
	updateLengthModel( symbol );
	}

/* Decode a literal */

#define decodeLiteral()	decodeOrder1()

/* Decode a high position */

int decodeHighPos( void )
	{
	const int symbol = decodeOrder0( highPosCumFreq );
	const int retVal = indexToHighPos[ symbol ];

	/* Update the model */
	updateHighPosModel( symbol );

	/* Return translated value */
	return( retVal );
	}

/* Decode a low position */

int decodeLowPos( void )
	{
	const int symbol = decodeOrder0( lowPosCumFreq );
	const int retVal = indexToLowPos[ symbol ];

	/* Update the model */
	if( !isText )
		updateLowPosModel( symbol );

	/* Return translated value */
	return( retVal );
	}

/* Decode a length */

int decodeLength( void )
	{
	const int symbol = decodeOrder0( lengthCumFreq );
	const int retVal = indexToLength[ symbol ];

	/* Update the model */
	updateLengthModel( symbol );

	/* Return translated value */
	return( retVal );
	}

/****************************************************************************
*																			*
*					LZSS Compression/Decompression Routines					*
*																			*
****************************************************************************/

/* Various variables used by the assembly language findMatch() routine */

int listIndex, lookAheadIndex, matchPos, lastMatchLen, currBufPos;
long noBytes;

/* Set up buffers for LZSS compression and order-1 arithcoder */

void initPack( const BOOLEAN initAllBuffers )
	{
	if( ( textBuffer = ( BYTE * ) hmalloc( STR_BUF_SIZE + MAX_LENGTH ) ) == NULL )
		error( OUT_OF_MEMORY );

	/* Only allocate the following buffers if we need them - saves 64K */
	if( initAllBuffers )
		{
		if( ( lookAheadBuffer = ( BYTE * ) hmalloc( LOOKAHEAD_BUF_SIZE * 2 ) ) == NULL ||
			( linkBuffer = ( int * ) hmalloc( ( STR_BUF_SIZE * sizeof( int ) ) ) ) == NULL ||
			( headList = ( int * ) hmalloc( LIST_SIZE * sizeof( int ) ) ) == NULL ||
			( tailList = ( int * ) hmalloc( LIST_SIZE * sizeof( int ) ) ) == NULL )
			error( OUT_OF_MEMORY );
		}

	initModels();
	}

/* Free LZSS buffers and order-1 arithcoder data */

void endPack( void )
	{
	sfree( textBuffer, STR_BUF_SIZE + MAX_LENGTH );
	if( choice != EXTRACT && choice != TEST && choice != DISPLAY )
		{
		sfree( lookAheadBuffer, LOOKAHEAD_BUF_SIZE * 2 );
		sfree( linkBuffer, STR_BUF_SIZE * sizeof( int ) );
		sfree( headList, LIST_SIZE * sizeof( int ) );
		sfree( tailList, LIST_SIZE * sizeof( int ) );
		}
	endModels();
	}

/* Perform the compression */

#ifdef __TSC__
  #ifdef inline		/* Undo nulling-out of inline for TopSpeed C */
	#undef inline
  #endif

  #pragma save
  #pragma call( inline=>on, reg_param => (si, dx, di, es, cx), \
				reg_return => (cx), reg_saved => (bx, dx, ds, st1, st2) )
  static int doloop( BYTE *srcPtr, BYTE *destPtr, int Count ) =
		{
		0x1E,				/* push ds   */
		0x8E, 0xDA,			/* mov ds,dx */
		0xF3, 0xA6,			/* rep ; cmpsb */
		0x83, 0xF9, 0x00,	/* cmp   cx,0	NB Close comment was missing here */
		0x75, 0x09,			/* jne   $0 */
		0x4E,				/* dec   si */
		0x4F,				/* dec   di */
		0x26, 0x8A, 0x05,	/* mov   ax,es:[di] */
		0x3A, 0x04,			/* cmp   ax,[si] */
		0x74, 0x01, 		/* je    $1 */
		0x41,				/* $0: inc   cx */
		0x1F,				/* $1: pop ds */
		0xC3				/* ret */
		};
  #pragma restore
#endif

LONG pack( BOOLEAN *localIsText, long localNoBytes )
	{
	int ch, count, currMatchLen;
	long printCount;
	static BOOLEAN textBufferFull;
	static unsigned int textBufferCount;
	BOOLEAN flipFlop = FALSE;
	int offset, endByte = 0;
	int tailIndex, headIndex;
	BYTE *srcPtr, *destPtr;
#ifdef __TSC__
	int CountDown;
#endif /* __TSC__ */
#ifdef LATTICE
	/* Yet another Lattice C bug - nothing in the world will convince it that
	   a value >32767 is unsigned when used in an address expression so we
	   just set a constant variable to this value and use that instead of the
	   literal constant */
	const int latticeFix = STR_BUF_SIZE;
#endif /* LATTICE */

	/* Copy various local parameters to global parameters */
	noBytes = localNoBytes;

	/* Evaluate size of input file */
	noBytes += CHECKSUM_LEN;
#ifdef LATTICE
	/* Wanna see a compiler really lose it?  Then replace the following code
	   with the #else stuff.  For the following code, Lattice C 5.10 produces:

	   move.l <offset>(a4), d1
	   subi.l #00000800, d1

	   With everything on one line, you get:

	   <F-line trap> */
	printCount = noBytes;
	printCount -= 2048L;			/* Make sure we don't print a spurious dit */
#else
	printCount = noBytes - 2048L;	/* Make sure we don't print a spurious dit */
#endif /* LATTICE */
	if( flags & BLOCK_MODE )
		/* Always use largest window in block mode */
		windowSize = 0;
	else
		/* Select window size depending on data file size */
		windowSize = ( noBytes > 16384 ) ? 0 : ( noBytes > 8192 ) ? 1 : 2;

	/* Initialise the I/O system */
	resetFastIn();

	/* Initialise the arithmetic encoder and models */
	if( firstFile || !( flags & BLOCK_MODE ) )
		{
		textBufferFull = FALSE;
		textBufferCount = 0;
		startModels();
		}
	else
		/* Always reset the lowPos model in block mode in case we go from
		   text <-> binary mode */
		startLowPosModel();
	startEncode();
	outByteCount = 0L;

	/* See if the file is a text file or binary and use the appropriate
	   model.  If at least 95% of the file's chars are in the following
	   category, we assume it's text */
	for( count = 0, currMatchLen = 0; count < 50; count++ )
		if( ( ( ( ( ch = _inBuffer[ count ] ) >= 0x20 ) && ( ch <= 0x7E ) ) ) ||
			ch == '\r' || ch == '\n' || ch == '\t' )
			currMatchLen++;
	*localIsText = isText = ( currMatchLen > 47 ) ? TRUE : FALSE;

	/* Initialize various data structures if necessary.  The memset() of the
	   linkBuffer is done in two halves for use on 16-bit systems */
	if( firstFile || !( flags & BLOCK_MODE ) )
		{
		memset( headList, LIST_END, LIST_SIZE * sizeof( int ) );
		memset( tailList, LIST_END, LIST_SIZE * sizeof( int ) );
		currBufPos = 0;
		}

	/* Now fill the lookahead buffer and sigma buffer */
	for( lookAheadIndex = 0;
		 lookAheadIndex < LOOKAHEAD_BUF_SIZE && ( ch = fgetByte() ) != FEOF;
		 lookAheadBuffer[ lookAheadIndex ] =
			lookAheadBuffer[ lookAheadIndex + LOOKAHEAD_BUF_SIZE ] = ch,
		 lookAheadIndex++ );

	/* If the file is < LOOKAHEAD_BUF_SIZE bytes long we append the checksum
	   value now */
	if( lookAheadIndex < LOOKAHEAD_BUF_SIZE )
		for( count = 0; count < CHECKSUM_LEN && lookAheadIndex < LOOKAHEAD_BUF_SIZE;
			 count++, lookAheadIndex++ )
				lookAheadBuffer[ lookAheadIndex ] =
				lookAheadBuffer[ lookAheadIndex + LOOKAHEAD_BUF_SIZE ] =
							( isText ) ? CHECKSUM_TEXT[ endByte++ ] :
										 CHECKSUM_BIN[ endByte++ ];
	lookAheadIndex = 0;

	do
		{
		/* Find the longest match of the lookahead buffer in the text buffer */
		lastMatchLen = 1;
		listIndex = lookAheadBuffer[ lookAheadIndex ] << 5;
		ch = lookAheadBuffer[ ( lookAheadIndex + 1 ) & LOOKAHEAD_BUF_MASK ];
		ch ^= lookAheadBuffer[ ( lookAheadIndex + 2 ) & LOOKAHEAD_BUF_MASK ];
		listIndex |= ch & 0x1F;
		headIndex = headList[ listIndex ];
		while( headIndex != LIST_END )
			{
			/* Match ( position + 1 ) (since we already know there is a match
			   at ( position )) with the text in the lookahead buffer */
			currMatchLen = 1;

			/* Make sure we don't try and match past the current position in
			   the buffer.  Use an offset of -1 since we've already matched
			   the first char */
			count = currBufPos - headIndex - 1;
			if( count < 0 )
				count += STR_BUF_SIZE;
			if( count >= LOOKAHEAD_BUF_SIZE )
				count = LOOKAHEAD_BUF_SIZE - 1;

			/* Perform the actual string match */
			srcPtr = textBuffer + headIndex + 1;
			destPtr = lookAheadBuffer + ( ( lookAheadIndex + 1 ) & LOOKAHEAD_BUF_MASK );
#ifdef __TSC__
			if( count )
				{
				CountDown = count;
				count = doloop( srcPtr, destPtr, count );
				currMatchLen += CountDown - count;
				}
#else
			while( *srcPtr++ == *destPtr++ && count-- )
				currMatchLen++;
#endif /* __TSC__ */

			/* See if the new match is longer than an older match */
			if( currMatchLen >= lastMatchLen )
				{
				lastMatchLen = currMatchLen;
				matchPos = headIndex;
				}

			/* Move to next match in list */
			headIndex = linkBuffer[ headIndex ];
			}

		/* Special case:  At the very end of the file we may have a match
		   longer than the number of chars left.  In this case we decrease
		   the match length, and compress as single chars if necessary */
		if( noBytes < lastMatchLen )
			lastMatchLen = ( int ) noBytes;

		/* See if we have a match long enough to make it worthwhile ouputting
		   it as an LZSS code */
		if( lastMatchLen <= MATCH_THRESHOLD )
			{
			/* Output literal character */
			lastMatchLen = 1;
			encodeLiteral( lookAheadBuffer[ lookAheadIndex ] );
			}
		else
			{
			/* Output ( length, position ) pair */
			encodeLiteral( LZSS_ESC );	/* Encode escape for LZSS code */
			encodeLength( lastMatchLen - MATCH_THRESHOLD );
			if( isText )
				/* Take position from end of match not start */
				matchPos += lastMatchLen;
			offset = ( currBufPos - matchPos ) & STR_BUF_MASK;
			encodeHighPos( offset >> 7 );
			encodeLowPos( offset & 0x7F );
			}

		/* Now move the matched string from the lookahead buffer to the text
		   buffer, updating the head and tail lists as we go */
		for( count = 0; count < lastMatchLen; count++ )
			{
			/* See if we need to start deleting chars from the text buffer */
			if( textBufferFull )
				{
				/* Unlink the old char at currBufPos from lists */
				listIndex = textBuffer[ currBufPos ] << 5;
				ch = textBuffer[ ( currBufPos + 1 ) & STR_BUF_MASK ];
				ch ^= textBuffer[ ( currBufPos + 2 ) & STR_BUF_MASK ];
				listIndex |= ch & 0x1F;
				tailIndex = tailList[ listIndex ];
				if( tailIndex == currBufPos )
					/* Last char deleted */
					headList[ listIndex ] = tailList[ listIndex ] = LIST_END;
				else
					/* Point to next char in list */
					headList[ listIndex ] = linkBuffer[ currBufPos ];
				}
			else
				{
				textBufferCount++;

				/* Check whether the buffer is now full */
				if( textBufferCount >= STR_BUF_SIZE )
					textBufferFull = TRUE;
				}

			/* Move a char from the lookahead buffer into the string buffer
			   and read in a new char to replace it */
			textBuffer[ currBufPos ] = ch = lookAheadBuffer[ lookAheadIndex ];
			if( currBufPos < MAX_LENGTH )
				/* Add to sigma buffer as well */
#ifdef LATTICE
				textBuffer[ latticeFix + currBufPos ] = ch;
#else
				textBuffer[ STR_BUF_SIZE + currBufPos ] = ch;
#endif /* LATTICE */
			if( noBytes <= CHECKSUM_LEN + LOOKAHEAD_BUF_SIZE )
				lookAheadBuffer[ lookAheadIndex ] =
				lookAheadBuffer[ lookAheadIndex + LOOKAHEAD_BUF_SIZE ] =
							( isText ) ? CHECKSUM_TEXT[ endByte++ ] :
										 CHECKSUM_BIN[ endByte++ ];
			else
				lookAheadBuffer[ lookAheadIndex ] =
				lookAheadBuffer[ lookAheadIndex + LOOKAHEAD_BUF_SIZE ] =
													fgetByte();
			lookAheadIndex = ++lookAheadIndex & LOOKAHEAD_BUF_MASK;

			/* Add it to the link buffer/lists at currBufPos */
			listIndex = ch << 5;
			ch = lookAheadBuffer[ lookAheadIndex ]; /* lAI has been inc'd in the meantime */
			ch ^= lookAheadBuffer[ ( lookAheadIndex + 1 ) & LOOKAHEAD_BUF_MASK ];
			listIndex |= ch & 0x1F;
			tailIndex = tailList[ listIndex ];
			if( tailIndex == LIST_END )
				/* No chars in list */
				headList[ listIndex ] = currBufPos;
			else
				/* Add to end of list */
				linkBuffer[ tailIndex ] = currBufPos;
			tailList[ listIndex ] = currBufPos;
			linkBuffer[ currBufPos ] = LIST_END;

			/* Increment currBufPos */
			currBufPos = ++currBufPos & STR_BUF_MASK;

			/* Decrement bytes seen */
			noBytes--;
			if( modelByteCount < SCALE_RANGE )
				modelByteCount++;
			}

		/* Output a dit for every 2K processed */
		if( noBytes < printCount )
			{
#ifdef GUI
			updateProgressReport();
#else
			if( flipFlop )
				{
				hputchars( '\b' );
				hputchars( 'O' );
				}
			else
				hputchars( 'o' );
			hflush( stdout );
			flipFlop = !flipFlop;
			printCount -= 2048L;
#endif /* GUI */
#ifdef __MAC__
			yieldTimeSlice();
#endif /* __MAC__ */
			}
		}
	while( noBytes > 0 );

	/* Clean up any loose ends */
	endEncode();
#ifndef GUI
	if( flipFlop )
		{
		hputchars( '\b' );
		hputchars( 'O' );
		hflush( stdout );
		}
#endif /* GUI */

	return( outByteCount );
	}

/* Perform the decompression */

LONG dataLen;	/* Used in UNPACK.C */

BOOLEAN unpack( const BOOLEAN localIsText, const long dataSize, const long localNoBytes )
	{
	static int bufIndex;	/* Must be static for block mode */
	int position, length, ch;
	int endByte = 0, printCount = 0, checkBytes = 0;
	BYTE checkSum[ CHECKSUM_LEN ];
	BOOLEAN flipFlop = FALSE;

	/* Copy the local parameters to their global equivalents */
	noBytes = localNoBytes;

	/* Initialise the I/O system */
	dataLen = dataSize;
	resetFastOut();
	noBytes += CHECKSUM_LEN;	/* Non-test version stores only orig.file size */

	/* Determine which model was used for compression */
	isText = localIsText;
	if( flags & BLOCK_MODE )
		/* Always use largest window in block mode */
		windowSize = 0;
	else
		/* Select window size depending on data file size */
		windowSize = ( noBytes > 16384 ) ? 0 : ( noBytes > 8192 ) ? 1 : 2;

	/* Initialise various data structures */
	if( firstFile || !( flags & BLOCK_MODE ) )
		startModels();
	else
		/* Always reset the lowPos model in block mode in case we go from
		   text <-> binary mode */
		startLowPosModel();
	startDecode();
	memset( checkSum, 0xFF, CHECKSUM_LEN );	/* Clear checksum */

	if( firstFile || !( flags & BLOCK_MODE ) )
		bufIndex = 0;
	while( noBytes > 0 )
		{
		/* Decode a literal char or length byte */
		if( ( ch = decodeLiteral() ) != LZSS_ESC )
			{
			/* Output byte and add to circular buffer */
			if( noBytes <= CHECKSUM_LEN )
				{
				/* If the data is corrupted we may end up decoding a random
				   string, so we insert a sanity check here */
				if( endByte >= CHECKSUM_LEN )
					{
					flushBuffer();		/* Flush any remaining data */
					return( FALSE );
					}

				checkSum[ endByte++ ] = ch;
				}
			else
				fputByte( ( BYTE ) ch );

			textBuffer[ bufIndex++ ] = ch;
			bufIndex &= STR_BUF_MASK;
			noBytes--;
			printCount++;
			if( modelByteCount < SCALE_RANGE )
				modelByteCount++;
			}
		else
			{
			/* Decode ( length, position ) pair */
			length = decodeLength() + MATCH_THRESHOLD;
			position = decodeHighPos() << 7;
			position |= decodeLowPos();
			position = bufIndex - position;
			if( isText )
				position -= length;
			position &= STR_BUF_MASK;

			/* Adjust byte count and printCount for string */
			noBytes -= length;
			if( noBytes <= CHECKSUM_LEN )
				checkBytes = CHECKSUM_LEN - ( int ) noBytes;
			printCount += length;
			if( modelByteCount < SCALE_RANGE )
				modelByteCount += length;

			/* Copy length bytes, starting at position, to the output */
			while( length-- )
				{
				/* Read bytes of the string out of the circular buffer */
				ch = textBuffer[ position++ & STR_BUF_MASK ];
				if( checkBytes && length < checkBytes )
					{
					/* If the data is corrupted we may end up decoding a
					   random string, so we insert a sanity check here */
					if( endByte >= CHECKSUM_LEN )
						{
						flushBuffer();		/* Flush any remaining data */
						return( FALSE );
						}

					checkSum[ endByte++ ] = ch;
					}
				else
					fputByte( ( BYTE ) ch );

				/* Add them to the tail end of the buffer */
				textBuffer[ bufIndex++ ] = ch;
				bufIndex &= STR_BUF_MASK;
				}
			}

		/* Output a dit for every 2K processed */
		if( printCount >= 2048 )
			{
#ifdef GUI
			updateProgressReport();
#else
			if( flipFlop )
				{
				hputchars( '\b' );
				hputchars( 'O' );
				}
			else
				hputchars( 'o' );
			hflush( stdout );
			flipFlop = !flipFlop;
			printCount -= 2048;
#endif /* GUI */
#ifdef __MAC__
			yieldTimeSlice();
#endif /* __MAC__ */
			}
		}

	/* Flush the output buffer and print the last dit if necessary */
	flushBuffer();
#ifndef GUI
	if( flipFlop )
		{
		hputchars( '\b' );
		hputchars( 'O' );
		hflush( stdout );
		}
#endif /* GUI */

	/* Compare the final CHECKSUM_LEN decoded bytes against the checksum as
	   a data integrity check */
	return( !memcmp( checkSum, ( isText ) ? CHECKSUM_TEXT : CHECKSUM_BIN,
					 CHECKSUM_LEN ) );
	}
