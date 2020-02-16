/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								  CRC16 Routines							*
*							 CRC16.C  Updated 16/07/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#include "defs.h"

/* The crc16 table and crc16 variable itself */

WORD crc16tbl[ 256 ];
WORD crc16;

/* The block crc16 calculation routine.  Ideally this should be done in
   assembly language for speed */

void crc16buffer( BYTE *bufPtr, int length )
	{
	while( length-- )
		crc16 = crc16tbl[ ( BYTE ) crc16 ^ *bufPtr++ ] ^ ( crc16 >> 8 );
	}

/* The initialisation routine for the crc16 table */

void initCRC16( void )
	{
	int bitCount, tblIndex;
	WORD crcVal;

	for( tblIndex = 0; tblIndex < 256; tblIndex++ )
		{
		crcVal = tblIndex;
		for( bitCount = 0; bitCount < 8; bitCount++ )
			if( crcVal & 0x01 )
				crcVal = ( crcVal >> 1 ) ^ 0xA001;
			else
				crcVal >>=  1;
		crc16tbl[ tblIndex ] = crcVal;
		}
	}
