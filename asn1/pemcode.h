/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					Binary <-> RFC 1113 ASCII Encoding Header				*
*							PEMCODE.H  Updated 13/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* En/decode a buffer full of PEM-encoded data */

int pemEncode( BYTE *inBuffer, char *outBuffer, int count );
int pemDecode( char *inBuffer, BYTE *outBuffer );

/* Read a PEM-encoded field from a file */

int readPemField( BYTE *data );
