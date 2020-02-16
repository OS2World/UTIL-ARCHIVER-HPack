/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							I/O Stream Handling Routines						*
*							 STREAM.C  Updated 05/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "io\stream.h"
#elif defined( __MAC__ )
  #include "stream.h"
#else
  #include "io/stream.h"
#endif /* System-specific include directives */

/* The stream status information.  A stream can be connected to a device,
   or simply be a block of memory */

#define STREAM_MASK_IO		0xFFFC	/* Mask for I/O type flags */
#define STREAM_MASK_INPUT	0xFFFE	/* Mask for input flag */
#define STREAM_MASK_OUTPUT	0xFFFD	/* Mask for output flag */
#define STREAM_INPUT		0x0001	/* Stream is for input */
#define STREAM_OUTPUT		0x0002	/* Stream is for output */

#define STREAM_MASK_FILE	0xFFFB	/* Mask for file flag */
#define STREAM_MEM			0x0000	/* Stream corr.to mem.block (default) */
#define STREAM_FILE			0x0004	/* Stream corresponds to file/device */

#define STREAM_MASK_CHECKSUM	0xFFF8	/* Mask for checksum flag */
#define STREAM_CHECKSUM		0x0008	/* Checksum input/output */

#define STREAM_MASK_CRYPT	0xFFEF	/* Mask for en/decrypt flag */
#define STREAM_CRYPT		0x0010	/* En/decrypt input/output */

#define STREAM_MASK_DIRTY	0xFFDF	/* Mask for dirty status */
#define STREAM_DIRTY		0x0020	/* Position updated since last read */

/****************************************************************************
*																			*
*							Stream new/delete Operators						*
*																			*
****************************************************************************/

int newStream( STREAM *stream, const int bufferSize )
	{
	/* Set up general-purpose fields */
	if( !bufferSize )
		/* No length, set stream buffer to NULL */
		stream->buffer = NULL;
	else
		/* Allocate room for stream buffer (be nice to have default
		   parameters here) */
		if( ( stream->buffer = ( BYTE * ) hmalloc( bufferSize ) ) == NULL )
			return( STREAM_NO_MEM );
	stream->bufferSize = bufferSize;
	stream->readPointer = stream->writePointer = 0;
	stream->status = 0;
	stream->position = 0;

	/* Set up file, checksum, encryption, and ecc-related fields */
	stream->theFD = ERROR;
	stream->checksum = 0;
	stream->cryptInfo = NULL;
/*	stream->eccInfo = NULL; */

	return( STREAM_OK );
	}

int deleteStream( STREAM *stream )
	{
	if( stream->theFD != ERROR )
		hclose( stream->theFD );
	memset( stream->buffer, 0, stream->bufferSize );
	hfree( stream->buffer );
	newStream( stream, 0 );

	return( STREAM_OK );
	}

/****************************************************************************
*																			*
*						Stream Property Management Functions				*
*																			*
****************************************************************************/

/* Set the mode of a stream: input, output, or both */

int setStreamMode( STREAM *stream, const int mode )
	{
	switch( mode )
		{
		case STREAM_MODE_INPUT:
			stream->status |= STREAM_INPUT;
			break;

		case STREAM_MODE_OUTPUT:
			stream->status |= STREAM_OUTPUT;
			break;

		case STREAM_MODE_INPUT_OUTPUT:
			stream->status |= STREAM_INPUT | STREAM_OUTPUT;
			break;

		default:
			return( STREAM_BAD_PARAM );
		}

	return( STREAM_OK );
	}

/* Associate a stream with a file */

int setStreamFileAssociation( STREAM *stream, const FD theFD )
	{
	/* Simply attach the stream to the FD */
	stream->theFD = theFD;

	return( STREAM_OK );
	}

/* Flush a stream */

int flushStream( STREAM *stream )
	{
	/* If the stream is attached to a file and is writeable and there is data
	   in it, write the data */
	if( ( stream->status & STREAM_OUTPUT ) &&
		( stream->theFD != ERROR ) && stream->writePointer )
		hwrite( stream->theFD, stream->buffer, stream->writePointer );

	/* Empty the buffer */
	stream->writePointer = 0;

	return( STREAM_OK );
	}

/****************************************************************************
*																			*
*							Stream Data Handling Functions					*
*																			*
****************************************************************************/

int setStreamPosition( STREAM *stream, long position, int whence )
	{
	long streamPos;

	/* Turn all seeks into an absolute seek */
	if( whence == STREAM_POS_FROMCURRENT )
		/* Seek relative to current position in stream */
		position += stream->position;
	else
		if( whence == STREAM_POS_FROMEND )
			/* Seek relative to end of stream */
			puts( "Can't seek relative to end of stream" );
/*			position += stream->endPosition; */

	/* Check if we've gone past the start of the stream */
	if( stream->position + stream->readPointer + position < 0 )
		{
		/* If it's a non-seekable memory stream, return error code */
		if( ( stream->status & STREAM_MASK_FILE ) == STREAM_MEM )
			return( STREAM_NO_SEEK );

		/* Flush the buffer if necessary */
		if( stream->status && STREAM_DIRTY )
			hwrite( stream->theFD, stream->buffer, stream->writePointer );

		/* Try and seek to the required position */
		streamPos = stream->readPointer + position;
		if( hlseek( stream->theFD, streamPos, SEEK_CUR ) == ERROR )
			return( STREAM_NO_SEEK );
		stream->readPointer = stream->writePointer = 0;
		stream->status &= STREAM_DIRTY;
		return( STREAM_OK );
		}

	/* Check if we've gone past the end of the stream */
	if( stream->readPointer + position > stream->bufferSize )
		{
		/* If it's a non-seekable memory stream, return error code */
		if( ( stream->status & STREAM_MASK_FILE ) == STREAM_MEM )
			return( STREAM_NO_SEEK );

		/* Flush the buffer if necessary */
		if( stream->status && STREAM_DIRTY )
			hwrite( stream->theFD, stream->buffer, stream->writePointer );

		/* Try and seek to the required position */
		streamPos = stream->readPointer + position;
		if( hlseek( stream->theFD, streamPos, SEEK_CUR ) == ERROR )
			return( STREAM_NO_SEEK );
		stream->readPointer = stream->writePointer = 0;
		stream->status &= STREAM_DIRTY;
		return( STREAM_OK );
		}

	/* Simply update the in-memory pointer */
	stream->readPointer = ( int ) ( position - stream->position );
	stream->writePointer = ( int ) ( position - stream->position );
	return( STREAM_OK );
	}

/****************************************************************************
*																			*
*							Stream read/write Functions						*
*																			*
****************************************************************************/

/* Write a block of data to a stream */

int swrite( STREAM *stream, const BYTE *data, int count )
	{
	int bytesToMove;

	/* Make sure we're allowed to write to it */
	if( !( stream->status & STREAM_OUTPUT ) )
		return( STREAM_NO_WRITE );

	while( count )
		{
		/* Copy as much as we can into the remaining buffer space */
		bytesToMove = stream->bufferSize - stream->writePointer;
		if( count < bytesToMove )
			bytesToMove = count;
		memcpy( stream->buffer + stream->writePointer, data, bytesToMove );
		count -= bytesToMove;

#if 0
		/* Update data checksum if necessary */
		if( stream->status & STREAM_CHECKSUM )
			stream->checksum = crc16buffer( stream->checksum,
											stream->buffer + stream->writePointer,
											bytesToMove );

		/* Encrypt data in buffer if necessary */
		if( stream->status & STREAM_CRYPT )
			encryptCFB( stream->cryptInfo,
						stream->buffer + stream->writePointer,
						bytesToMove );
#endif /* 0 */

		/* Update count of bytes in buffer */
		stream->writePointer += bytesToMove;
		if( stream->writePointer == stream->bufferSize )
			{
			/* If there is more data and nowhere to put it, complain */
			if( count && !( stream->status & STREAM_FILE ) )
				return( STREAM_FULL );

			hwrite( stream->theFD, stream->buffer, stream->bufferSize );
			stream->writePointer = 0;
			}
		}

	return( STREAM_OK );
	}

/* Put a byte to a stream - optimized for speed, so has a minimum of error
   checking */

int sputc( STREAM *stream, const BYTE data )
	{
	/* Write the buffer if necessary */
	if( stream->writePointer == stream->bufferSize )
		{
#if 0
		/* Update data checksum if necessary */
		if( stream->status & STREAM_CHECKSUM )
			stream->checksum = crc16buffer( stream->checksum,
											stream->buffer + stream->writePointer,
											bytesToMove );

		/* Encrypt data in buffer if necessary */
		if( stream->status & STREAM_CRYPT )
			encryptCFB( stream->cryptInfo,
						stream->buffer + stream->writePointer,
						bytesToMove );
#endif /* 0 */

		/* If there is nowhere to put the data, complain */
		if( !( stream->status & STREAM_FILE ) )
			return( STREAM_FULL );

		hwrite( stream->theFD, stream->buffer, stream->bufferSize );
		}

	/* Put data in buffer */
	stream->buffer[ stream->writePointer++ ] = data;

	return( STREAM_OK );
	}

/* Read a block of data from a stream */

int sread( STREAM *stream, BYTE *data, int count )
	{
	int bufIndex = 0, bytesToRead;

	while( count )
		{
		/* Refill the buffer if necessary */
		if( stream->readPointer == stream->bufferSize )
			{
			/* If there is more data and nowhere to get it from, complain */
			if( count && !( stream->status & STREAM_FILE ) )
				return( STREAM_EMPTY );

/*			vread( stream->bufferSize ); */

#if 0
			/* Update data checksum if necessary */
			if( stream->status & STREAM_CHECKSUM )
				stream->checksum = crc16buffer( stream->checksum,
												stream->buffer + stream->readPointer,
												bytesToMove );

			/* Encrypt data in buffer if necessary */
			if( stream->status & STREAM_CRYPT )
				encryptCFB( stream->cryptInfo,
							stream->buffer + stream->readPointer,
							bytesToMove );
#endif /* 0 */
			stream->readPointer = 0;
			}

		/* Determine how much data we can move out of the buffer */
		bytesToRead = stream->bufferSize - stream->readPointer;
		if( bytesToRead > count )
			bytesToRead = count;

		/* Move data from the stream to the read buffer */
		memcpy( data + bufIndex, stream->buffer + stream->readPointer, bytesToRead );
		stream->readPointer += bytesToRead;
		bufIndex += bytesToRead;
		count -= bytesToRead;
		}

	return( STREAM_OK );
	}

/* Get a byte from a stream */

int sgetc( STREAM *stream )
	{
	/* Refill the buffer if necessary */
	if( stream->readPointer == stream->bufferSize )
		{
		/* If there is more data and nowhere to get it from, complain */
		if( !( stream->status & STREAM_FILE ) )
			return( STREAM_EMPTY );

/*		vread( stream->bufferSize ); */

#if 0
		/* Update data checksum if necessary */
		if( stream->status & STREAM_CHECKSUM )
			stream->checksum = crc16buffer( stream->checksum,
											stream->buffer + stream->readPointer,
											bytesToMove );

		/* Encrypt data in buffer if necessary */
		if( stream->status & STREAM_CRYPT )
			encryptCFB( stream->cryptInfo,
						stream->buffer + stream->readPointer,
						bytesToMove );
#endif /* 0 */
		}

	/* Get data from buffer */
	return( ( stream->readPointer == stream->bufferSize ) ?
			STREAM_EMPTY : stream->buffer[ stream->readPointer++ ] );
	}
