/****************************************************************************
*                                                                           *
*                          HPACK Multi-System Archiver                 		*
*                          ===========================                      *
*                                                                           *
*                       High-Speed Data Movement Routines                   *
*							STORE.C  Updated 12/07/91						*
*                                                                           *
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*                                                                           *
* 		Copyright 1989 - 1991  Peter C.Gutmann.  All rights reserved       	*
*                                                                           *
****************************************************************************/

#include <ctype.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#include "hpacklib.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "crypt\crypt.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "store\store.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "crypt.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "store.h"
#else
  #include "crc/crc16.h"
  #include "crypt/crypt.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "store/store.h"
#endif /* System-specific include directives */

/* Prototypes for functions in GUI.C */

void updateProgressReport( void );

/* The following are defined in FASTIO.C */

extern BOOLEAN doCryptOut;
extern int cryptMark;

/****************************************************************************
*																			*
*					Store Data in an Archive without Compression			*
*																			*
****************************************************************************/

/* Store data in an archive with no compression.  This routine must take
   care to correctly handle any data already in the buffer, unlike
   unstore() which assumes the buffer is empty when first called */

LONG store( BOOLEAN *isText )
	{
	LONG byteCount = 0L;
	int bytesRead, bytesToRead, printCount = 0;
	int sampleCount = 0, textByteCount = 0;
	BOOLEAN moreData = TRUE;
	BYTE ch, buffer[ sizeof( WORD ) ];

	crc16 = 0;

	while( moreData )
		{
		/* Read as much as we can into the remaining buffer space */
		bytesToRead = _BUFSIZE - _outByteCount;
		byteCount += bytesRead = hread( _inFD, _outBuffer + _outByteCount, bytesToRead );

		/* Update CRC for the data read and encrypt it and any other data
		   still in the buffer if necessary */
		crc16buffer( _outBuffer + _outByteCount, bytesRead );
		if( doCryptOut )
			encryptCFB( _outBuffer + _outByteCount, bytesRead );

		/* See if the file is text or binary */
		while( sampleCount < 50 && sampleCount <= bytesRead )
			{
			if( isprint( ch = _outBuffer[ _outByteCount + sampleCount ] ) ||
						 ch == '\r' || ch == '\n' || ch == '\t' )
				textByteCount++;
			sampleCount++;		/* isprint() has side effects - eek! */
			}

		if( _outByteCount + bytesRead == _BUFSIZE )
			{
			/* We've filled the buffer, so flush it */
			writeBuffer( _BUFSIZE );

			/* Check for special case of file size being exactly what we've
			   just read in */
			if( !bytesRead )
				moreData = FALSE;
			}
		else
			{
			/* There's still room for more in the buffer.  Since this is the
			   last pass through the loop we don't need to update cryptMark -
			   this will be done automatically when the CRC is written */
			_outByteCount += bytesRead;
			moreData = FALSE;
			}

		/* Print a dit if necessary */
		printCount += bytesRead;
#ifdef GUI
		while( printCount >= 2048 )
			{
			updateProgressReport();
			printCount -= 2048;
			}
#else
		while( printCount >= 4096 )
			{
			hputchars( 'O' );
			hflush( stdout );
			printCount -= 4096;
			}
#endif /* GUI */
		}

	/* Output last dit due to rounding if necessary */
	if( printCount >= 2048 )
		{
		hputchars( 'O' );
		hflush( stdout );
		}

	if( doCryptOut )
		{
		/* Encrypt crc16 before writing it */
		mputWord( buffer, crc16 );
		encryptCFB( buffer, sizeof( WORD ) );
		fputWord( mgetWord( buffer ) );
		cryptMark = _outByteCount;
		}
	else
		fputWord( crc16 );

	/* If at least 95% of the file's chars are text, we assume it's text.
	   This test uses absolute values rather than percentages since for
	   files smaller than 50 bytes it may be hard to reliably determine
	   whether it is text or not */
	*isText = ( textByteCount > 47 ) ? TRUE : FALSE;

	return( byteCount + sizeof( WORD ) );
	}

/****************************************************************************
*																			*
*					Unstore Uncompressed Data from an Archive				*
*																			*
****************************************************************************/

/* Retrieve uncompressed data from an archive.  In order to handle the stream
   buffering we can't just read data into the _outBuffer, but have to read it
   into the _inBuffer, copy it to the _outBuffer, and then write the
   _outBuffer */

BOOLEAN unstore( long noBytes )
	{
	int bytesToProcess = _BUFSIZE - _inByteCount, printCount;
	WORD checkSum;

	crc16 = 0;

	noBytes -= sizeof( WORD );	/* Don't move the checksum at the end */
	_outByteCount = 0;
	if( bytesToProcess )
		{
		/* Don't move more data than necessary */
		if( bytesToProcess > noBytes )
			bytesToProcess = ( int ) noBytes;

		/* Write out any data still in the buffer */
		memcpy( _outBuffer, _inBuffer + _inByteCount, bytesToProcess );
		writeBuffer( bytesToProcess );
		noBytes -= bytesToProcess;
		_inByteCount += bytesToProcess;

		/* Update CRC for data in buffer */
		crc16buffer( _outBuffer, bytesToProcess );
		}

	/* Print a dit if necessary */
	printCount = bytesToProcess;
#ifdef GUI
	while( printCount >= 2048 )
		{
		updateProgressReport();
		printCount -= 2048;
		}
#else
	while( printCount >= 4096 )
		{
		hputchars( 'O' );
		hflush( stdout );
		printCount -= 4096;
		}
#endif /* GUI */

	while( noBytes )
		{
		_inBytesRead = vread( _inBuffer, _BUFSIZE );
		bytesToProcess = ( noBytes < _inBytesRead ) ?
						 ( int ) noBytes : _inBytesRead;
		memcpy( _outBuffer, _inBuffer, bytesToProcess );
		writeBuffer( bytesToProcess);
		noBytes -= bytesToProcess;
		_inByteCount = bytesToProcess;

		/* Update CRC for data in buffer */
		crc16buffer( _outBuffer, bytesToProcess );

		/* Print a dit if necessary */
		printCount += bytesToProcess;
#ifdef GUI
		while( printCount >= 2048 )
			{
			updateProgressReport();
			printCount -= 2048;
			}
#else
		while( printCount >= 4096 )
			{
			hputchars( 'O' );
			hflush( stdout );
			printCount -= 4096;
			}
#endif /* GUI */

		/* If we run out of input, make sure we don't go into an endless loop */
		if( !_inBytesRead )
			return( FALSE );			/* Data corrupted */
		}

	/* Output last dit due to rounding if necessary */
	if( printCount >= 2048 )
		{
		hputchars( 'O' );
		hflush( stdout );
		}

	flushBuffer();
	checkSum = fgetWord();
	return( crc16 == checkSum );
	}

/****************************************************************************
*																			*
*						High-speed Data Move Routine						*
*																			*
****************************************************************************/

/* Move data from one file to another without doing a CRC check.  This
   routine must take care to correctly handle any data already in the buffer.
   Note that this is a pure data moving routine and doesn't handle things
   like encryption */

void moveData( long noBytes )
	{
	int bytesRead, bytesToRead = _BUFSIZE - _outByteCount;

	/* Read as much as we can into the remaining buffer space */
	bytesRead = vread( _outBuffer + _outByteCount, ( noBytes < bytesToRead ) ?
												   ( int ) noBytes : bytesToRead );
	if( _outByteCount + bytesRead == _BUFSIZE )
		{
		/* We've filled the buffer, so flush it */
		writeBuffer( _BUFSIZE );
		noBytes -= bytesRead;
		}
	else
		{
		/* There's still room for more in the buffer */
		_outByteCount += bytesRead;
		noBytes = 0L;
		}

	/* Now blast the data through the buffer */
	while( noBytes )
		{
		bytesToRead = ( noBytes < _BUFSIZE ) ? ( int ) noBytes : _BUFSIZE;
		_outByteCount = vread( _outBuffer, bytesToRead );
		if( _outByteCount == _BUFSIZE )
			/* We've filled the buffer, so flush it */
			writeBuffer( bytesToRead );
		noBytes -= bytesToRead;
		}
	}
