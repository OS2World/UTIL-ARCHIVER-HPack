/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  I/O Stream Header File						*
*							 STREAM.H  Updated 05/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _STREAM_DEFINED

#define _STREAM_DEFINED

#include "defs.h"
#if defined( __ATARI__ )
  #include "crc/crc16.h"
  #include "crypt/crypt.h"
  #include "io/hpackio.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "crypt.h"
  #include "hpackio.h"
#else
  #include "crc/crc16.h"
  #include "crypt/crypt.h"
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* All I/O is done on generalized I/O streams, which may represent files,
   blocks of memory, virtual files, a console, etc.  Lower-level FASTIO
   routines are responsible for mapping these streams to actual devices.
   Each stream has associated with it methods for checksumming, encrypting,
   and performing error correction on the stream.  Currently these are a
   straight CRC and generic block cipher - a more sophisticated
   implementation would include a CHECKSUM_INFO and CRYPT_INFO struct with
   functions pointers to the checksumming and encryption code */

/* The structure for keeping track of what each stream corresponds to */

typedef struct {
			   /* The general-purpose stream fields */
			   BYTE *buffer;		/* Buffer for data */
			   int bufferSize;		/* Total size of buffer */
			   int readPointer;		/* Current read position in buffer */
			   int writePointer;	/* Current write position in buffer */
			   WORD status;			/* Stream status information */
			   long position;		/* Where we are in a seekable stream */

			   /* Fields used when the stream is attached to a file */
			   FD theFD;			/* File stream is associated with */

			   /* Fields used for checksumming */
			   WORD checksum;		/* Checksum for stream */

			   /* Pointer to encryption information for stream */
			   CRYPT_INFO *cryptInfo;

			   /* Pointer to ECC information for stream */
/*			   ECC_INFO *eccInfo; */
			   } STREAM;

/* Error values returned by stream I/O functions */ 

enum { STREAM_OK,				/* No error */
	   STREAM_EMPTY = -1,		/* Stream empty (no more data available) */
	   STREAM_FULL = -2,		/* Stream full (no room for more data) */
	   STREAM_NO_WRITE = -3,	/* Attempt to write to input-only stream */
	   STREAM_NO_READ = -4,		/* Attempt to read from output-only stream */
	   STREAM_BAD_PARAM = -5,	/* Bad parameter */
	   STREAM_NO_MEM = -6,		/* Out of memory for operation */
	   STREAM_NO_SEEK = -7		/* Attempt to seek past start/end of stream */
	 };

/* The modes a stream can have */

enum { STREAM_MODE_INPUT, STREAM_MODE_OUTPUT, STREAM_MODE_INPUT_OUTPUT };

/* The attributes a stream can have */

enum { STREAM_ATTR_CHECKSUM, STREAM_ATTR_CRYPT, STREAM_ATTR_ECC };

/* Where to seek to in a stream */

enum { STREAM_POS_FROMSTART, STREAM_POS_FROMCURRENT, STREAM_POS_FROMEND };

/* Create and delete streams */

int newStream( STREAM *stream, const int bufferSize );
int deleteStream( STREAM *stream );

/* Change the properties of a stream */

int setStreamMode( STREAM *stream, const int mode );
int setStreamFileAssociation( STREAM *stream, const FD theFD );
int flushStream( STREAM *stream );

/* Change the way data in a stream is handled */

int setStreamPosition( STREAM *stream, long position, int whence );

/* Input/output data from/to a stream */

int sputc( STREAM *stream, const BYTE data );
int swrite( STREAM *stream, const BYTE *data, int count );

int sgetc( STREAM *stream );
int sread( STREAM *stream, BYTE *data, int count );

#endif /* !_STREAM_DEFINED */
