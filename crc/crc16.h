/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*									CRC16 Macros							*
*							 CRC16.H  Updated 01/07/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#ifndef _CRC16_DEFINED

#define _CRC16_DEFINED

#include "defs.h"

/****************************************************************************
*																			*
*									CRC16 Defines							*
*																			*
****************************************************************************/

/* This macro computes the 16-bit CRC of a byte of data */

void updateCRC16( BYTE data );

#define updateCRC16(data) crc16 = ( crc16tbl[ ( BYTE ) crc16 ^ ( data ) ] ^ ( crc16 >> 8 ) )

extern WORD crc16tbl[], crc16;		/* Defined in CRC16.C */

/* To use the above, first initialize the crc16 value to zero:

   crc16 = 0;

   Then for all data bytes x do:

		updateCRC16( x );
*/

/* The following routine performs a high-speed crc16 calculations on a block
   of data.  Ideally it should be written in assembly language would be much
   faster than the above macro */

void crc16buffer( BYTE *buffer, int length );

/* Prototype for function in CRC16.C */

void initCRC16( void );

/****************************************************************************
*																			*
*							Compressor Checksum Defines						*
*																			*
****************************************************************************/

/* Defines to handle the data checksum.  The following two checksums were
   chosen to minimize the length of the encoded checksum:  In English,
   the most common digrams are 'e ', ' t', 'th', 'he', the most common
   trigrams are ' th', 'the', 'he ', and the most common tetragrams are
   ' the' and 'the ', thus we use these as the checksum.  For binaries we
   use four consecutive zeroes since these seem to be present in most files */

#define CHECKSUM_TEXT	" the"
#define CHECKSUM_BIN	"\0\0\0\0"

#define CHECKSUM_LEN	( sizeof( CHECKSUM_TEXT ) - 1 )	/* -1 for '\0' */

#endif /* !_CRC16_DEFINED */
