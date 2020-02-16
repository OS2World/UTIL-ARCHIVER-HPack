/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					Binary <-> RFC 1113 ASCII Encoding Routines				*
*							PEMCODE.C  Updated 13/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* Encode/decode data from binary to the printable format specified in
   RFC 1113 */

#include <string.h>
#include "defs.h"
#include "error.h"
#include "frontend.h"
#include "hpacklib.h"
#include "script.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
#elif defined( __MAC__ )
  #include "fastio.h"
#else
  #include "io/fastio.h"
#endif /* System-specific include directives */

/* The padding value used to pad odd output bytes */

#define PEM_PAD		'='

/* Binary <-> PEM translation tables.  Note that in the ASCII -> binary
   table the padding value is given a return code of 0x00 since it is
   handled by the decode routine.  In addition, we only need to check values
   not between ' ' and '~' since readLine() won't allow other character types */

static char binToAscii[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char asciiToBin[] = {
					ERROR, ERROR, ERROR, ERROR, ERROR, ERROR, ERROR, ERROR,
					ERROR, ERROR, ERROR, 0x3E,  ERROR, ERROR, ERROR, 0x3F,
					0x34,  0x35,  0x36,  0x37,  0x38,  0x39,  0x3A,  0x3B,
					0x3C,  0x3D,  ERROR, ERROR, ERROR, 0x00,  ERROR, ERROR,
					ERROR, 0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,
					0x07,  0x08,  0x09,  0x0A,  0x0B,  0x0C,  0x0D,  0x0E,
					0x0F,  0x10,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,
					0x17,  0x18,  0x19,  ERROR, ERROR, ERROR, ERROR, ERROR,
					ERROR, 0x1A,  0x1B,  0x1C,  0x1D,  0x1E,  0x1F,  0x20,
					0x21,  0x22,  0x23,  0x24,  0x25,  0x26,  0x27,  0x28,
					0x29,  0x2A,  0x2B,  0x2C,  0x2D,  0x2E,  0x2F,  0x30,
                    0x31,  0x32,  0x33,  ERROR, ERROR, ERROR, ERROR, ERROR
					};

/* Basic single-char en/decode functions */

#define encode(data)	binToAscii[ data ]
#define decode(data)	asciiToBin[ data ]

/****************************************************************************
*																			*
*								PEM Encoding Routines						*
*																			*
****************************************************************************/

/* Encode a block of binary data into the PEM format */

int pemEncode( BYTE *inBuffer, char *outBuffer, int count )
	{
	int srcIndex = 0, destIndex = 0;
	int remainder = count % 3;

	while( srcIndex < count )
		{
		outBuffer[ destIndex++ ] = encode( inBuffer[ srcIndex ] >> 2 );
		outBuffer[ destIndex++ ] = encode( ( ( inBuffer[ srcIndex ] << 4 ) & 0x30 ) |
										   ( ( inBuffer[ srcIndex + 1 ] >> 4 ) & 0x0F ) );
		srcIndex++;
		outBuffer[ destIndex++ ] = encode( ( ( inBuffer[ srcIndex ] << 2 ) & 0x3C ) |
										   ( ( inBuffer[ srcIndex + 1 ] >> 6 ) & 0x03 ) );
		srcIndex++;
		outBuffer[ destIndex++ ] = encode( inBuffer[ srcIndex++ ] & 0x3F );
		}

	/* Go back and add padding if we've encoded too many characters */
	if( remainder == 2 )
		/* There were only 2 bytes in the last group */
		outBuffer[ destIndex - 1 ] = PEM_PAD;
	else
		if( remainder == 1 )
			/* There was only 1 byte in the last group */
			outBuffer[ destIndex - 2 ] = outBuffer[ destIndex - 1 ] = PEM_PAD;

	outBuffer[ destIndex ] = '\0';

	/* Return count of encoded bytes */
	return( destIndex );
	}

/****************************************************************************
*																			*
*								PEM Decoding Routines						*
*																			*
****************************************************************************/

/* Decode a block of binary data from the PEM format */

int pemDecode( char *inBuffer, BYTE *outBuffer )
	{
	int srcIndex = 0, destIndex = 0;
	int byteCount, remainder = 0;
	BYTE c0, c1, c2, c3;

	/* Strip whitespace and find out how many bytes we have to decode and
	   how much padding there is */
	while( *inBuffer == ' ' || *inBuffer == '\t' )
		inBuffer++;
	byteCount = strlen( inBuffer );
	if( byteCount && inBuffer[ byteCount - 1 ] == PEM_PAD )
		remainder++;
	if( ( byteCount - 1 ) && ( inBuffer[ byteCount - 2 ] == PEM_PAD ) )
		remainder++;

	while( srcIndex < byteCount )
		{
		/* Decode a block of data from the input buffer */
		c0 = decode( inBuffer[ srcIndex++ ] - ' ' );
		c1 = decode( inBuffer[ srcIndex++ ] - ' ' );
		c2 = decode( inBuffer[ srcIndex++ ] - ' ' );
		c3 = decode( inBuffer[ srcIndex++ ] - ' ' );
		if( ( c0 | c1 | c2 | c3 ) == ( BYTE ) ERROR )
			return( ERROR );

		/* Copy the decoded data to the output buffer */
		outBuffer[ destIndex++ ] = ( c0 << 2 ) | ( c1 >> 4 );
		outBuffer[ destIndex++ ] = ( c1 << 4 ) | ( c2 >> 2);
		outBuffer[ destIndex++ ] = ( c2 << 6 ) | ( c3 );
		}

	/* Return count of decoded bytes */
	return( destIndex - remainder );
	}

/* Read a PEM-encoded field from a file.  Although readLine() does all sorts
   of fancy error processing, we just return a yes/no status */

int readPemField( BYTE *data )
	{
	char inBuffer[ 100 ];
	int count = 0, lineLength, errPos /*, line = 0*/;

	/* Keep reading fields as long as we find the space delimiter at
	   the start */
	while( fgetByte() == ' ' )
		{
		if( readLine( inBuffer, 100, &lineLength, &errPos ) != READLINE_NO_ERROR )
			return( ERROR );
		if( lineLength )
			count += pemDecode( inBuffer, data + count );
/*		line++;	*/
		}
	ungetByte();

	return( OK );
	}
