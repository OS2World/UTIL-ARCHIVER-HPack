/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  	  Fast File I/O Routines						*
*							FASTIO.C  Updated 10/07/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "error.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "data\ebcdic.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "ebcdic.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
#else
  #include "crc/crc16.h"
  #include "data/ebcdic.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* Prototypes for functions in ARCHIVE.C */

void blankLine( int length );

#ifdef __MAC__

/* Prototypes for functions in MAC.C */

void bleuurgghh( void );
void waitForDisk( void );

#endif /* __MAC__ */

#ifdef __ARC__

/* Prototypes for functions in ARC.C */

void writeToStdout( BYTE *buffer, int bufSize );

#endif /* __ARC__ */


/* Some vars used to keep track of the fast I/O routines.  OS's which support
   asynchronous I/O can allocate a shadow buffer for independant reads/writes */

BYTE *_inBuffer, *_outBuffer;
#if defined( __MSDOS16__ )
  WORD _inBufferBase, _outBufferBase;
#endif /* __MSDOS16__ */
FD _inFD, _outFD;
int _inByteCount, _outByteCount;
int _inBytesRead;

/* Flags to control checksumming of input/output, the count of output bytes
   to checksum, and the point in the buffer to start checksumming */

BOOLEAN doChecksumIn, doChecksumOut, doChecksumDirOut;
long checksumLength;		/* Number of bytes to checksum */
int checksumMark, dirChecksumMark;	/* Start point in buffers for checksumming data */

/* Flags to control encryption/decryption of input/output, the count of
   output bytes to encrypt, and the point in the buffer to start encrypting */

BOOLEAN doCryptIn, doCryptOut;
long cryptLength;			/* Number of bytes to decrypt */
int cryptMark;				/* Start point in buffer for encrypting data */

/* Variables to handle the kludging of the skipping of checksumming of the
   encryption header */

BOOLEAN preemptDecrypt;		/* Whether we need to preempt the checksumming */
long preemptedChecksumLength;	/* Length of data to checksum */

/* The number of bytes written so far for this segment, the total number of
   bytes written before this segment (needed for multipart archive writes),
   and the current position in the archive (needed for multipart archive
   reads) */

static long currLength, totalLength;
static long currentPosition;

/* The output data intercept type */

static OUTINTERCEPT_TYPE outputIntercept = OUT_DATA;

/* Whether we need to pad the output data writtten to allow room for a
   multipart archive trailer */

static BOOLEAN padOutput;

/* Prototypes for the output routines used by the output intercept code */

void initOutFormatText( void );
void outFormatChar( const char ch );
void endOutFormatText( void );			/* Formatted text output */

/* The value to use as line terminator when doing translation to MSDOS CRLF
   format */

static BYTE lineEndChar;

/* Vars used to handle the general-purpose buffer.  This buffer is used
   during extraction for textfile translation and during changes to an
   archive for a general-purpose buffer for assembling data.  When necessary
   the upper half of the mrglBuffer is sacred to the dirInfo handling code
   for buffering of dirInfo */

#define DIRBUF_OFFSET	( _BUFSIZE / 2 )	/* Start of dir.info buffer */

BYTE *mrglBuffer, *dirBuffer;
int mrglBufCount, dirBufCount;
int dirBytesRead;

/* Macro to output a byte to the mrglBuffer */

#define putMrglByte(ch)	do \
							{ \
							if( mrglBufCount == _BUFSIZE ) \
								writeMrglBuffer( _BUFSIZE ); \
							mrglBuffer[ mrglBufCount++ ] = ch; \
							} \
						while( 0 )

/* Defines to control the size of archive segments when creating a multipart
   archive */

#define MULTIPART_TRAILER_SIZE	( HPACK_ID_SIZE + sizeof( WORD ) + sizeof( BYTE ) )
#define MIN_PARTSIZE			500		/* Min.data size for which we bother */
#define MIN_ARCHIVE_SIZE		( HPACK_ID_SIZE + MIN_PARTSIZE + MULTIPART_TRAILER_SIZE )

static int minPartSize;			/* The min.size of an archive segment */
static BOOLEAN atomicWrite;		/* Whether we are in atomic write mode */
BOOLEAN atomicWriteOK;			/* Whether the atomic write succeeded */

/****************************************************************************
*																			*
*							Initialize fast I/O functions 					*
*																			*
****************************************************************************/

/* Allocate buffers for the fast I/O routines, the GP buffer, and the shadow
   buffers if used.  Note that as these routines are called before the string
   table has been set up we can't display a normal error message, so we
   return an error status instead */

int initFastIO( void )
	{
#if defined( __MSDOS16__ )
	if( ( _inBuffer = ( BYTE * ) hmallocSeg( _BUFSIZE ) ) == NULL ||
		( _outBuffer = ( BYTE * ) hmallocSeg( _BUFSIZE ) ) == NULL ||
		( mrglBuffer = ( BYTE * ) hmalloc( _BUFSIZE ) ) == NULL )
#else
	if( ( _inBuffer = ( BYTE * ) hmalloc( _BUFSIZE ) ) == NULL ||
		( _outBuffer = ( BYTE * ) hmalloc( _BUFSIZE ) ) == NULL ||
		( mrglBuffer = ( BYTE * ) hmalloc( _BUFSIZE ) ) == NULL )
#endif /* __MSDOS16__ */
		return( ERROR );
	dirBuffer = mrglBuffer + DIRBUF_OFFSET;

#if defined( __MSDOS16__ )
	_inBufferBase = FP_SEG( _inBuffer );
	_outBufferBase = FP_SEG( _outBuffer );
#endif /* __MSDOS16__ */

	/* Set up CRC16 lookup table */
	initCRC16();

	return( OK );
	}

#if !defined( __MSDOS16__ ) || defined( GUI )

/* Free the fast I/O buffers and the GP buffer */

void endFastIO( void )
	{
	sfree( _inBuffer, _BUFSIZE );
	sfree( _outBuffer, _BUFSIZE );
	sfree( mrglBuffer, _BUFSIZE );
	}
#endif /* !__MSDOS16__ || GUI */

/* Reset input system */

void resetFastIn( void )
	{
	_inByteCount = 0;
	doChecksumIn = doCryptIn = preemptDecrypt = FALSE;
	cryptLength = checksumLength = 0L;
	if( ( _inBytesRead = vread( _inBuffer, _BUFSIZE ) ) == ERROR )
		fileError();
	}

void resetFastOut( void )
	{
	_outByteCount = mrglBufCount = dirBufCount = 0;
	currLength = 0L;	/* Note: totalLength is set independantly */
	doChecksumOut = doCryptOut = doChecksumDirOut = FALSE;
	minPartSize = MIN_PARTSIZE;
	atomicWrite = padOutput = FALSE;
	}

#ifdef __MAC__

/* Save and restore the state of the output routines if they need to be used
   for another output stream (used when writing to the resource fork file) */

static FD savedOutFD;
static WORD savedMultipartFlags;
static LONG savedCurrLength, savedTotalLength;

void saveOutputState( void )
	{
	/* Save output FD */
	savedOutFD = getOutputFD();

	/* Save various output flags */
	savedMultipartFlags = multipartFlags;
	multipartFlags &= ~MULTIPART_WRITE;		/* Turn off multipart writes */
	savedCurrLength = currLength;
	savedTotalLength = totalLength;
	}

void restoreOutputState( void )
	{
	/* Restore output FD */
	setOutputFD( savedOutFD );

	/* Restore output flags */
	multipartFlags = savedMultipartFlags;
	currLength = savedCurrLength;
	totalLength = savedTotalLength;
	}

#endif /* __MAC__ */

/* Save and restore the state of the input routines in case we need to use
   them for another input stream */

static FD savedInFD;
static int savedInByteCount, savedInBytesRead;
static BOOLEAN savedPreemptDecrypt;
static WORD savedMultipartFlags;
static LONG savedCurrentPosition;

void saveInputState( void )
	{
	/* Save input FD */
	savedInFD = getInputFD();

	/* Save data in input buffer */
	memcpy( mrglBuffer, _inBuffer, _BUFSIZE );
	savedInByteCount = _inByteCount;
	savedInBytesRead = _inBytesRead;
	if( !( multipartFlags & MULTIPART_READ ) )
		savedCurrentPosition = htell( savedInFD );
	else
		savedCurrentPosition = currentPosition;

	/* Save various input flags */
	savedPreemptDecrypt = preemptDecrypt;
	savedMultipartFlags = multipartFlags;
	multipartFlags &= ~MULTIPART_READ;		/* Turn off multipart reads */
	}

void restoreInputState( void )
	{
	/* Restore input FD */
	setInputFD( savedInFD );

	/* Restore data to input buffer */
	memcpy( _inBuffer, mrglBuffer, _BUFSIZE );
	_inByteCount = savedInByteCount;
	_inBytesRead = savedInBytesRead;
	currentPosition = savedCurrentPosition;
	if( !( multipartFlags & MULTIPART_READ ) )
		hlseek( savedInFD, savedCurrentPosition, SEEK_SET );
	else
		vlseek( currentPosition, SEEK_SET );

	/* Restore various input flags */
	preemptDecrypt = savedPreemptDecrypt;
	multipartFlags = savedMultipartFlags;
	}

inline void initTranslationSystem( const BYTE theChar )
	{
	/* Set up the line termination character */
	lineEndChar = theChar;
	}

/* Handle checksumming of input/output.  We use checksumBegin()/checksumEnd()
   for output, and checksumSetInput() for input since the checksumming must
   be done before decryption takes place, not afterwards */

/* Turn checksumming of output on/off */

void checksumBegin( const BOOLEAN resetChecksum )
	{
	doChecksumOut = TRUE;

	/* Reset checksum if necessary and mark start point in buffer */
	if( resetChecksum )
		crc16 = 0;
	checksumMark = _outByteCount;
	}

void checksumEnd( void )
	{
	if( doChecksumOut )
		/* Checksum rest of data in outBuffer */
		crc16buffer( _outBuffer + checksumMark, _outByteCount - checksumMark );

	doChecksumOut = FALSE;
	}

void checksumSetInput( const long dataLength, const BOOLEAN resetChecksum )
	{
	int dataLeft = _BUFSIZE - _inByteCount;

	/* Reset the checksum value if necessary */
	if( resetChecksum )
		crc16 = 0;

	/* CRC any data already in the buffer at this point */
	crc16buffer( _inBuffer + _inByteCount, ( dataLength < dataLeft ) ?
										   ( int ) dataLength : dataLeft );
	if( dataLength > dataLeft )
		{
		/* Calculate amount left to checksum and indicate that there is more */
		checksumLength = dataLength - dataLeft;
		doChecksumIn = TRUE;
		}
	}

/* Handle checksumming of the directory buffer output stream */

void checksumDirBegin( const BOOLEAN resetChecksum )
	{
	doChecksumDirOut = TRUE;

	/* Reset checksum if necessary and mark start point in buffer */
	if( resetChecksum )
		crc16 = 0;
	dirChecksumMark = dirBufCount;
	}

void checksumDirEnd( void )
	{
	if( doChecksumDirOut )
		/* Checksum rest of data in outBuffer */
		crc16buffer( dirBuffer + dirChecksumMark, dirBufCount - dirChecksumMark );

	doChecksumDirOut = FALSE;
	}

/* Handle encryption of input/output, much as for checksumming */

int cryptBegin( const BOOLEAN isMainKey )
	{
	int cryptInfoLength;

	/* Write encryption information packet and initialise encryption system */
	cryptInfoLength = putEncryptionInfo( isMainKey );
	doCryptOut = TRUE;

	/* Mark start point in buffer */
	cryptMark = _outByteCount;

	return( cryptInfoLength );
	}

void cryptEnd( void )
	{
	if( doCryptOut )
		/* Encrypt rest of data in outBuffer */
		encryptCFB( _outBuffer + cryptMark, _outByteCount - cryptMark );

	doCryptOut = FALSE;
	}

BOOLEAN cryptSetInput( long dataLength, int *cryptInfoLength )
	{
	int dataLeft;

	/* Read in encryption information packet and initialise encryption system */
	if( !getEncryptionInfo( cryptInfoLength ) )
		return( FALSE );

	dataLength -= *cryptInfoLength;
	dataLeft = _BUFSIZE - _inByteCount;

	/* Perform any necessary checksumming */
	if( preemptDecrypt )
		{
		checksumSetInput( preemptedChecksumLength - *cryptInfoLength, RESET_CHECKSUM );
		preemptDecrypt = FALSE;
		}

	/* Decrypt any data already in the buffer at this point */
	decryptCFB( _inBuffer + _inByteCount, ( dataLength < dataLeft ) ?
										  ( int ) dataLength : dataLeft );
	if( dataLength > dataLeft )
		{
		/* Evaluate the number of bytes still left to decrypt and turn on the
		   decryptIn flag */
		cryptLength = dataLength - dataLeft;
		doCryptIn = TRUE;
		}

	return( TRUE );
	}

/* A kludge function which allows the checksumming of input data after the
   encryption header has been skipped */

void preemptCryptChecksum( const long length )
	{
	preemptDecrypt = TRUE;
	preemptedChecksumLength = length;
	}

/* Get and set the current absolute position in the data */

long getCurrPosition( void )
	{
	return( totalLength + currLength + _outByteCount - HPACK_ID_SIZE );
	}

void setCurrPosition( const long position )
	{
	totalLength = currentPosition = position;
	currLength = 0L;
	}

/* Turn atomic write mode on and off */

void setWriteType( const WRITE_TYPE writeType )
	{
	switch( writeType )
		{
		case STD_WRITE:
			/* Set standard write mode */
			minPartSize = MIN_PARTSIZE;
			atomicWrite = padOutput = FALSE;
			break;

		case ATOMIC_WRITE:
			/* All data must be written onto same disk */
			minPartSize = _outByteCount;

			atomicWrite = TRUE;
			atomicWriteOK = FALSE;
			padOutput = FALSE;
			break;

		case SAFE_WRITE:
			/* Pad the output data written to allow room for a multipart
			   archive trailer.  This is necessary for the end of the data
			   in a multipart archive, since after the next hwrite() we will
			   be leaving multipart-write mode so we need to ensure there is
			   always room for the trailer */
			padOutput = TRUE;
			break;
		}
	}

/****************************************************************************
*																			*
*							Data Fast I/O Functions							*
*																			*
****************************************************************************/

/* Normally HPACK reads/writes values in big-endian format.  If there is
   ever any need to change this to little-endian format, it can be done
   here by defining RW_LITTLE_ENDIAN */

#if !defined( __MSDOS16__ )

/* These should functions should really be implemented in assembly language
   for efficiency */

#ifdef RW_LITTLE_ENDIAN

void fputLong( const LONG value )
	{
	fputByte( ( BYTE ) ( value & 0xFF ) );
	fputByte( ( BYTE ) ( ( value >> 8 ) & 0xFF ) );
	fputByte( ( BYTE ) ( ( value >> 16 ) & 0xFF ) );
	fputByte( ( BYTE ) ( ( value >> 24 ) & 0xFF ) );
	}

void fputWord( const WORD data )
	{
	fputByte( ( BYTE ) ( data & 0xFF ) );
	fputByte( ( BYTE ) ( data >> BITS_PER_BYTE ) );
	}

#else

void fputLong( const LONG value )
	{
	fputByte( ( BYTE ) ( ( value >> 24 ) & 0xFF ) );
	fputByte( ( BYTE ) ( ( value >> 16 ) & 0xFF ) );
	fputByte( ( BYTE ) ( ( value >> 8 ) & 0xFF ) );
	fputByte( ( BYTE ) ( value & 0xFF ) );
	}

void fputWord( const WORD data )
	{
	fputByte( ( BYTE ) ( data >> BITS_PER_BYTE ) );
	fputByte( ( BYTE ) ( data & 0xFF ) );
	}

#endif /* RW_LITTLE_ENDIAN */

void fputByte( const BYTE data )
	{
	/* Write the buffer if necessary */
	if( _outByteCount == _BUFSIZE )
		{
		/* Encrypt the data if required */
		if( doCryptOut )
			{
			encryptCFB( _outBuffer + cryptMark, _BUFSIZE - cryptMark );
			cryptMark = 0;
			}

		/* Checksum the data if required */
		if( doChecksumOut )
			{
			crc16buffer( _outBuffer + checksumMark, _BUFSIZE - checksumMark );
			checksumMark = 0;
			}

		writeBuffer( _BUFSIZE );
		}

	_outBuffer[ _outByteCount++ ] = data;
	}

/* The function versions of getByte(), getWord(), and getLong() */

#ifdef RW_LITTLE_ENDIAN

LONG fgetLong( void )
	{
	LONG value;

	value = ( LONG ) fgetByte();
	value |= ( LONG ) fgetByte() << 8;
	value |= ( LONG ) fgetByte() << 16;
	value |= ( LONG ) fgetByte() << 24;
	return( value );
	}

WORD fgetWord( void )
	{
	WORD value;

	value = ( WORD ) fgetByte();
	value |= ( WORD ) fgetByte() << BITS_PER_BYTE;
	return( value );
	}

#else

LONG fgetLong( void )
	{
	LONG value;

	value = ( LONG ) fgetByte() << 24;
	value |= ( LONG ) fgetByte() << 16;
	value |= ( LONG ) fgetByte() << 8;
	value |= ( LONG ) fgetByte();
	return( value );
	}

WORD fgetWord( void )
	{
	WORD value;

	value = ( WORD ) fgetByte() << BITS_PER_BYTE;
	value |= ( WORD ) fgetByte();
	return( value );
	}

#endif /* RW_LITTLE_ENDIAN */

int fgetByte( void )
	{
	/* Refill the input buffer if necessary */
	if( _inByteCount == _BUFSIZE )
		{
		/* Read in a new bufferful of data */
		_inByteCount = 0;
		if( ( _inBytesRead = vread( _inBuffer, _BUFSIZE ) ) == ERROR )
			fileError();
		}

	return( ( _inByteCount == _inBytesRead ) ? FEOF : _inBuffer[ _inByteCount++ ] );
	}

/****************************************************************************
*																			*
*						Directory Data Fast I/O Functions					*
*																			*
****************************************************************************/

/* Write data to the temporary directory data file.  These all write to the
   directory data file so there is no need to specify a file descriptor.  We
   can't use use fputByte(), fputWord(), or fputLong() for this since these
   write to the outBuffer, so we use these versions which use the dirBuffer */

#ifdef RW_LITTLE_ENDIAN

void fputDirLong( const LONG data )
	{
	fputDirByte( ( BYTE ) ( data & 0xFF ) );
	fputDirByte( ( BYTE ) ( ( data >> 8 ) & 0xFF ) );
	fputDirByte( ( BYTE ) ( ( data >> 16 ) & 0xFF ) );
	fputDirByte( ( BYTE ) ( ( data >> 24 ) & 0xFF ) );
	}

void fputDirWord( const WORD data )
	{
	fputDirByte( ( BYTE ) ( data & 0xFF ) );
	fputDirByte( ( BYTE ) ( data >> BITS_PER_BYTE ) );
	}

#else

void fputDirLong( const LONG data )
	{
	fputDirByte( ( BYTE ) ( ( data >> 24 ) & 0xFF ) );
	fputDirByte( ( BYTE ) ( ( data >> 16 ) & 0xFF ) );
	fputDirByte( ( BYTE ) ( ( data >> 8 ) & 0xFF ) );
	fputDirByte( ( BYTE ) ( data & 0xFF ) );
	}

void fputDirWord( const WORD data )
	{
	fputDirByte( ( BYTE ) ( data >> BITS_PER_BYTE ) );
	fputDirByte( ( BYTE ) ( data & 0xFF ) );
	}

#endif /* RW_LITTLE_ENDIAN */

void fputDirByte( const BYTE data )
	{
	if( dirBufCount == DIRBUFSIZE )
		writeDirBuffer( dirBufCount );
	dirBuffer[ dirBufCount++ ] = data;
	}

#endif /* !__MSDOS16__ */

/****************************************************************************
*																			*
*							Memory Fast I/O Functions							*
*																			*
****************************************************************************/

/* Get or put WORD's and LONG's from and to memory */

#if !defined( __MSDOS16__ )

/* The memory equivalents of fgetWord() and fgetLong() */

#ifdef RW_LITTLE_ENDIAN

void mputLong( BYTE *memPtr, const LONG data )
	{
	memPtr[ 0 ] = ( BYTE ) ( data & 0xFF );
	memPtr[ 1 ] = ( BYTE ) ( ( data >> 8 ) & 0xFF );
	memPtr[ 2 ] = ( BYTE ) ( ( data >> 16 ) & 0xFF );
	memPtr[ 3 ] = ( BYTE ) ( ( data >> 24 ) & 0xFF );
	}

void mputWord( BYTE *memPtr, const WORD data )
	{
	memPtr[ 0 ] = ( BYTE ) ( data & 0xFF );
	memPtr[ 1 ] = ( BYTE ) ( ( data >> 8 ) & 0xFF );
	}

#else

void mputLong( BYTE *memPtr, const LONG data )
	{
	memPtr[ 0 ] = ( BYTE ) ( ( data >> 24 ) & 0xFF );
	memPtr[ 1 ] = ( BYTE ) ( ( data >> 16 ) & 0xFF );
	memPtr[ 2 ] = ( BYTE ) ( ( data >> 8 ) & 0xFF );
	memPtr[ 3 ] = ( BYTE ) ( data & 0xFF );
	}

void mputWord( BYTE *memPtr, const WORD data )
	{
	memPtr[ 0 ] = ( BYTE ) ( ( data >> 8 ) & 0xFF );
	memPtr[ 1 ] = ( BYTE ) ( data & 0xFF );
	}

#endif /* RW_LITTLE_ENDIAN */

/* The memory equivalents of fgetWord() and fgetLong() */

#ifdef RW_LITTLE_ENDIAN

LONG mgetLong( BYTE *memPtr )
	{
	return( ( LONG ) memPtr[ 0 ] |
			( ( LONG ) memPtr[ 1 ] << 8 ) |
			( ( LONG ) memPtr[ 2 ] << 16 ) |
			( ( LONG ) memPtr[ 3 ] << 24 ) );
	}

WORD mgetWord( BYTE *memPtr )
	{
	return( ( WORD ) ( ( memPtr[ 0 ] << 8 ) | memPtr[ 1 ] ) );
	}

#else

LONG mgetLong( BYTE *memPtr )
	{
	return( ( ( LONG ) memPtr[ 0 ] << 24 ) |
			( ( LONG ) memPtr[ 1 ] << 16 ) |
			( ( LONG ) memPtr[ 2 ] << 8 ) |
			( LONG ) memPtr[ 3 ] );
	}

WORD mgetWord( BYTE *memPtr )
	{
	return( ( WORD ) ( ( memPtr[ 0 ] << 8 ) | memPtr[ 1 ] ) );
	}

#endif /* RW_LITTLE_ENDIAN */

#endif /* !__MSDOS16__ */

/****************************************************************************
*																			*
*							Fast I/O Support Functions						*
*																			*
****************************************************************************/

#ifndef GUI

/* Wait for the luser to continue when handling a multipart archive */

void multipartWait( const WORD promptType, const int partNo )
	{
#ifdef __MAC__
	bleuurgghh();
#endif /* __MAC__ */

	if( promptType & WAIT_PARTNO )
		hprintf( MESG_PART_d_OF_MULTIPART_ARCHIVE, partNo + 1 );
	else
		hputchar( '\n' );
	hprintf( MESG_PLEASE_INSERT_THE );
	if( promptType & WAIT_NEXTDISK )
		hprintf( MESG_NEXT_DISK );
	else
		if( promptType & WAIT_PREVDISK )
			hprintf( MESG_PREV_DISK );
		else
			{
			hprintf( MESG_DISK_CONTAINING );
			if( promptType & WAIT_LASTPART )
				hprintf( MESG_THE_LAST_PART );
			else
				hprintf( MESG_PART_d, partNo + 1 );
			hprintf( MESG_OF_THIS_ARCHIVE );
			}
#ifdef __MAC__
	waitForDisk();
#else
	hprintf( MESG_AND_PRESS_A_KEY );
	hgetch();
#endif /* __MAC__ */
	blankLine( screenWidth - 1 );	/* Blank out prompt message */
	if( promptType & WAIT_NEXTDISK )
		hprintf( MESG_CONTINUING );
	}
#endif /* GUI */

/* Skip ahead a certain number of bytes.  Doing this as a direct hlseek() is
   not possible since some of the data may already be in the buffer or may be
   on a different disk */

void skipSeek( LONG skipLength )
	{
	int bytesInBuffer = _inBytesRead - _inByteCount;

	if( skipLength < bytesInBuffer )
		/* We can satisfy the request with what is still in the buffer */
		_inByteCount += ( WORD ) skipLength;
	else
		{
		/* First use all the bytes still in the buffer */
		skipLength -= bytesInBuffer;
		_inByteCount += bytesInBuffer;

		if( doCryptIn )
			/* If there is more encrypted data left after the data we are
			   skipping over, we have to read in all the data we would
			   normally seek over for decryption purposes */
			if( cryptLength > skipLength )
				{
				while( skipLength-- )
					fgetByte();
				return;
				}
			else
				/* We've exhausted the encrypted data */
				cryptLength = 0;

		/* Now we can do an hlseek() if necessary */
		if( skipLength )
			/* Check if we can satisfy the seek request in the current
			   archive part */
			if( !( flags & MULTIPART_READ ) ||
				currPart == getPartNumber( skipLength ) )
				hlseek( _inFD, skipLength, SEEK_CUR );
			else
				/* Seek destination is outside the current archive section,
				   do a full vlseek() */
				vlseek( skipLength, SEEK_CUR );

		/* Force a read at the next getByte() */
		forceRead();
		}
	}

/* Skip to the next piece of input data */

void skipToData( void )
	{
	if( skipDist )
		{
		/* Move past unwanted data and reset skip distance */
		skipSeek( skipDist );
		skipDist = 0L;
		}
	}

/* Set the output intercept */

void setOutputIntercept( OUTINTERCEPT_TYPE interceptType )
	{
	outputIntercept = interceptType;

	switch( outputIntercept )
		{
		case OUT_FMT_TEXT:
			/* Formatted text output */
			initOutFormatText();
			break;

		default:
			/* Default: No action */
			;
		}
	}

/* Reset the output intercept to the default setting */

void resetOutputIntercept( void )
	{
	switch( outputIntercept )
		{
		case OUT_FMT_TEXT:
			/* Formatted text output */
			endOutFormatText();
			break;

		default:
			/* Default: No action */
			;
		}

	/* Reset output intercept to default type */
	outputIntercept = OUT_DATA;
	}

/* Perform a seek, moving over multiple disks if necessary */

long vlseek( const long offset, const int whence )
	{
	long position = offset;		/* Default is SEEK_SET */
	int thePart;

	/* Do a quick check for whether we can do a normal seek */
	if( !( multipartFlags & MULTIPART_READ ) )
		{
		/* Adjust for virtual start of archive if necessary */
		if( whence == SEEK_SET )
			position += HPACK_ID_SIZE;

		return( hlseek( _inFD, position, whence ) );
		}

	/* Turn all seeks into an absolute seek */
	switch( whence )
		{
		case SEEK_CUR:
			/* Seek relative to current position */
			position += currentPosition;
			break;

		case SEEK_END:
			/* Seek relative to end of archive */
			position += endPosition;
		}

	/* Sanity check in case the offset info is from a corrupt archive - don't
	   try to move to nonexistant archive segment */
	if( position < 0L )
		return( position );

	/* Get the disk the correct part of the archive is on if necessary,
	   then move to the offset in that segment */
	if( ( thePart = getPartNumber( position ) ) != currPart )
		getPart( thePart );
	if( thePart )
		hlseek( _inFD, ( position - getPartSize( thePart - 1 ) ) + HPACK_ID_SIZE, SEEK_SET );
	else
		hlseek( _inFD, position + HPACK_ID_SIZE, SEEK_SET );
	currentPosition = position;

	/* Update count of bytes left to read */
	segmentEnd = getPartSize( thePart ) - currentPosition;

	return( position );
	}

/* Return the current position, taking multi-disk archives into account */

long vtell( void )
	{
	/* Do a quick check for whether we can do a normal seek */
	if( !( multipartFlags & MULTIPART_READ ) )
		return( htell( _inFD ) );

	/* Return the current position in the archive */
	return( currentPosition );
	}

/* Read in a bufferful of data, skipping to the next disk in the case of a
   multipart archive */

int vread( BYTE *buffer, int bufSize )
	{
	int startOffset = 0;	/* Remember start position of read */
	int bytesRead = 0, retVal;

	/* Do a quick check for whether we can do a normal read */
	if( !( multipartFlags & MULTIPART_READ ) )
		{
		if( ( bytesRead = hread( _inFD, buffer, bufSize ) ) == IO_ERROR )
			fileError();
		}
	else
		{
		/* Perform a read, checking for end of data */
retryRead:
		if( segmentEnd > bufSize )
			{
			if( ( retVal = hread( _inFD, buffer + startOffset, bufSize ) ) == IO_ERROR )
				fileError();
			bytesRead += retVal;
			segmentEnd -= bufSize;
			}
		else
			{
			/* Read in what's left into the buffer */
			if( segmentEnd )
				{
				if( hread( _inFD, buffer + startOffset, ( int ) segmentEnd ) == IO_ERROR )
					fileError();
				startOffset += ( int ) segmentEnd;
				bufSize -= ( int ) segmentEnd;
				bytesRead += ( int ) segmentEnd;
				}

			/* More parts to go, get the next disk */
			if( currPart < lastPart )
				{
				currPart++;
				getPart( currPart );
				segmentEnd = getPartSize( currPart ) - getPartSize( currPart - 1 ) ;

				/* Read in the rest of _inBuffer */
				hlseek( _inFD, HPACK_ID_SIZE, SEEK_SET );
				goto retryRead;
				}
			}
		}

	/* Checksum the data if required */
	if( doChecksumIn )
		{
		crc16buffer( _inBuffer, ( bufSize < checksumLength ) ? bufSize : ( int ) checksumLength );
		if( bufSize < checksumLength )
			checksumLength -= bufSize;
		else
			{
			checksumLength = 0;
			doChecksumIn = FALSE;
			}
		}

	/* Decrypt the data if necessary */
	if( doCryptIn )
		{
		decryptCFB( _inBuffer, ( bufSize < cryptLength ) ? bufSize : ( int ) cryptLength );
		if( bufSize < cryptLength )
			cryptLength -= bufSize;
		else
			{
			cryptLength = 0;
			doCryptIn = FALSE;
			}
		}

	/* Update position in archive */
	currentPosition += bytesRead;

	return( bytesRead );
	}

/* Write out a certain amount of data.  This is a special version which checks
   for the disk being full by making sure the no.of bytes written are the same
   as the number of bytes requested to be written (as well as the normal
   ERROR return value, since hwrite() does not treat disk full as an ERROR),
   and also handles multi-part archives by asking for another disk.

   Note that once a write fails in a multipart archive we don't need to bother
   retrying the write on the new disk since we've already cleared some of the
   buffer on the current disk and a write will be forced as soon as the buffer
   is full again anyway.  The only problem this causes is that writeBuffer()
   may need to be called multiple times if the buffer is being flushed.

   0 = Head in free space
   1 = Head placed firmly against wall

   Multidisk archives = 10101010101010101010101010 */

BOOLEAN outputAdjusted;

void vwrite( BYTE *buffer, int bufSize )
	{
	int i, bytesWritten;
#ifndef __MSDOS16__
	BOOLEAN retryWrite = FALSE;
#endif /* !__MSDOS16__ */

	if( outputIntercept == OUT_FMT_TEXT )
		{
		/* Formatted text output */
		for( i = 0; i < bufSize; i++ )
			outFormatChar( buffer[ i ] );
		}

	/* If we're done with the data, leave now */
	if( outputIntercept != OUT_DATA )
		return;

#ifdef __ARC__
	/* The Archimedes doesn't have standard streams so we use a custom
	   output routine */
	if( _outFD == STDOUT )
		{
		writeToStdout( buffer, bufSize );
		return;
		}
#endif /* __ARC__ */

	/* Output raw data to FD */
	if( ( bytesWritten = hwrite( _outFD, buffer, bufSize ) ) == ERROR )
		fileError();

	/* Now update the count of bytes written unless we're writing the final
	   info on a multipart archive.  Note that this does not include the
	   header or trailer information, only the raw data */
	if( !atomicWrite )
		currLength += bytesWritten;
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
	/* Wonderful things to tell your kids #27: The check for the identity of
	   _outFD is necessary under MSDOS since there is a ^Z (CPM EOF) on the
	   end of the file which is not written, and which ends up causing an
	   "Out of disk space" error on STDOUT if it isn't caught */
	if( ( bytesWritten != bufSize ) && ( _outFD > STDPRN ) )
#else
	if( bytesWritten != bufSize )
#endif /* __MSDOS16__ || __MSDOS32__ */
		{
		/* Complain if its not a multidisk archive and we run out of disk
		   space.  Note that once we've locked the disk space by writing as
		   much as possible we don't need to check space availability or the
		   return status of hwrite() any more since the disk space has already
		   been allocated/written to */
#ifdef __MSDOS16__
		error( OUT_OF_DISK_SPACE );
#else
		if( !( multipartFlags & MULTIPART_WRITE ) )
			error( OUT_OF_DISK_SPACE );

		/* Check whether the archive is big enough to make it worthwhile */
		currLength -= HPACK_ID_SIZE + MULTIPART_TRAILER_SIZE;
					/* Don't count info at start and end */
		i = 0;
		if( currLength > minPartSize )
			{
			/* Check if there's enough room to append the trailer information */
			if( bytesWritten < MULTIPART_TRAILER_SIZE )
				{
				if( atomicWrite )
					{
					/* Since we wrote the previous block in SAFE_WRITE mode,
					   we know there is enough room for the trailer, so we
					   just seek back and add it */
					hlseek( archiveFD, -( long ) MULTIPART_TRAILER_SIZE, SEEK_END );
					htruncate( archiveFD );
					}
				else
					{
					/* Not enough room to append trailer information, backtrack
					   and read in data at end of archive which is about to be
					   overwritten */
					i = MULTIPART_TRAILER_SIZE - bytesWritten;
					hlseek( archiveFD, -( long ) MULTIPART_TRAILER_SIZE, SEEK_END );
					hread( archiveFD, mrglBuffer + HPACK_ID_SIZE, i );

					/* Move data which will be overwritten by the trailer info.
					   into mrglBuffer as well */
					memcpy( mrglBuffer + HPACK_ID_SIZE + i, _outBuffer, MULTIPART_TRAILER_SIZE );
					i += MULTIPART_TRAILER_SIZE;
					bytesWritten = MULTIPART_TRAILER_SIZE;
					}
				}
			else
				/* Under most OS's this is the only case which will ever occur
				   since writes are rounded to the disk sector size */
				if( atomicWrite )
					{
					/* We need to get everything on one disk, go back and zap
					   anything we've written (including the previously-written
					   padding bytes) */
					bytesWritten += MULTIPART_TRAILER_SIZE;
					hlseek( archiveFD, -( long ) bytesWritten, SEEK_END );
					htruncate( archiveFD );
					}
				else
					/* Just go back a bit and add the trailer */
					bytesWritten -= MULTIPART_TRAILER_SIZE;

			if( atomicWrite )
				{
				/* Save output buffer info for later */
				memcpy( mrglBuffer, _outBuffer, MULTIPART_TRAILER_SIZE );
				bytesWritten = _outByteCount;
				_outByteCount = 0;
				}
			else
				/* Backtrack MULTIPART_TRAILER_SIZE bytes if necessary */
				hlseek( archiveFD, -( long ) MULTIPART_TRAILER_SIZE, SEEK_END );

			/* Append trailer information */
			fputWord( ( WORD ) currPart++ );
			fputByte( SPECIAL_MULTIPART );
			fputByte( HPACK_ID[ 0 ] );
			fputByte( HPACK_ID[ 1 ] );
			fputByte( HPACK_ID[ 2 ] );
			fputByte( HPACK_ID[ 3 ] );
			hwrite( archiveFD, _outBuffer, MULTIPART_TRAILER_SIZE );
			hclose( archiveFD );
			if( atomicWrite )
				{
				/* Restore output buffer info and retry on another disk */
				memcpy( _outBuffer, mrglBuffer, MULTIPART_TRAILER_SIZE );
				_outByteCount = bytesWritten;
				retryWrite = TRUE;	/* Retry write later on */
				}
			else
				{
				_outByteCount = 0;	/* Reset count from fput()'s */
				addPartSize( totalLength + currLength );
				lastPart++;
				}
			}
		else
			{
			/* Archive is too short to bother with, delete it */
			if( currLength > bytesWritten )
				{
				/* Read back data already written */
				currLength -= bytesWritten;		/* Don't reread data in _outBuf */
				currLength += MULTIPART_TRAILER_SIZE;	/* Adj.len.to proper size */
				i = ( int ) currLength;
				hlseek( archiveFD, HPACK_ID_SIZE, SEEK_SET );
				hread( archiveFD, mrglBuffer + HPACK_ID_SIZE, i );
				}
			bytesWritten = 0;
			hclose( archiveFD );
			hunlink( errorFileName );
			errorFD = 0;	/* No need to delete arch.on err.any more */
#ifdef GUI
			alert( ALERT_ARCH_SECTION_TOO_SHORT, NULL );
#else
			hprintf( WARN_ARCHIVE_SECTION_TOO_SHORT );
#endif /* GUI */
			}

		/* Go to next disk */
		multipartWait( WAIT_NEXTDISK, 0 );
		if( !( flags & OVERWRITE_SRC ) &&
			( archiveFD = hopen( errorFileName, O_RDONLY ) ) != ERROR )
			{
			/* Make sure we don't overwrite an existing archive unless explicity
			   asked to */
			hclose( archiveFD );
			error( CANNOT_OPEN_ARCHFILE, errorFileName );
			}
		if( ( errorFD = archiveFD = hcreat( errorFileName, CREAT_ATTR ) ) == ERROR )
			error( CANNOT_OPEN_ARCHFILE, errorFileName );
		setOutputFD( archiveFD );

		/* If we need to retry the write later on, add the header for a new
		   archive and exit now */
		if( retryWrite )
			{
			memmove( _outBuffer + HPACK_ID_SIZE, _outBuffer, _outByteCount );
			memcpy( _outBuffer, HPACK_ID, HPACK_ID_SIZE );
			return;
			}

		/* Try and write out the number of bytes we need for a minimum-size archive */
		memcpy( mrglBuffer, HPACK_ID, HPACK_ID_SIZE );
		i += HPACK_ID_SIZE;
		while( hwrite( archiveFD, mrglBuffer, MIN_ARCHIVE_SIZE ) < MIN_ARCHIVE_SIZE )
			{
			hclose( archiveFD );
			hunlink( errorFileName );
			errorFD = 0;
#ifdef GUI
			alert( ALERT_ARCH_SECTION_TOO_SHORT, NULL );
#else
			hprintf( WARN_ARCHIVE_SECTION_TOO_SHORT );
#endif /* GUI */
			multipartWait( WAIT_NEXTDISK, 0 );
			if( !( flags & OVERWRITE_SRC ) &&
				( archiveFD = hopen( errorFileName, O_RDONLY ) ) != ERROR )
				{
				/* Make sure we don't overwrite an existing archive unless
				   explicity asked to */
				hclose( archiveFD );
				error( CANNOT_OPEN_ARCHFILE, errorFileName );
				}
			if( ( errorFD = archiveFD = hcreat( errorFileName, CREAT_ATTR ) ) == ERROR )
				error( CANNOT_OPEN_ARCHFILE, errorFileName );
			setOutputFD( archiveFD );
			}
		hlseek( archiveFD, i, SEEK_SET );	/* Seek back to last valid byte */
		totalLength += currLength;	/* Add new segment size */
		currLength = ( LONG ) i;

		/* Now we have room for an archive of at least MIN_ARCHIVE_SIZE bytes
		   as well as having written out any backread data.  Move data down
		   to start of _outBuffer if necessary and adjust the new start position
		   of data in the buffer */
		if( padOutput )		/* !!! Don't write it on *last* write !!! */
			{
			bufSize -= MULTIPART_TRAILER_SIZE;	/* Don't write padding */
			outputAdjusted = TRUE;
			}
		if( bytesWritten )
			memcpy( _outBuffer, _outBuffer + bytesWritten, bufSize - bytesWritten );
		bufSize -= bytesWritten;
		checksumMark = _outByteCount = bufSize;
#endif /* !__MSDOS16__ */
		}

	/* If we got this far, the atomic write succeeded */
	if( atomicWrite )
		atomicWriteOK = TRUE;
	}

/* High-level writeBuffer() routine with character set translation */

#define DC1		( 0x11 ^ 0x80 )	/* Prime ASCII run count escape code in
								   high-bit set format */
#define CPM_EOF	0x1A			/* ^Z which turns up in some MSDOS files */

void writeBuffer( const int bufSize )
	{
	static BOOLEAN isRunCount = FALSE;
#if !( defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	   defined( __MSDOS32__ ) || defined( __OS2__ ) )
	static BOOLEAN crSeen = FALSE;
#endif /* !( __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
	BYTE ch;

	/* Reset output byte counter unless we need to remember the number of
	   output bytes for backtracking purposes */
	if( !atomicWrite )
		_outByteCount = 0;

	/* We may have more work to do if we are using output translation.
	   The order of translation is to translate char sets, then translate
	   EOF chars */
	if( flags & XLATE_OUTPUT )
		{
		if( xlateFlags & XLATE_PRIME )
			{
			/* Translate buffer from Prime ASCII -> normal ASCII */
			while( _outByteCount < bufSize )
				{
				/* Get char, treat as run count if necessary */
				ch = _outBuffer[ _outByteCount++ ];
				if( isRunCount )
					{
					/* Output RLE'd spaces */
					while( ch-- )
						putMrglByte( ' ' );
					isRunCount = FALSE;
					continue;
					}

				/* Don't output special chars */
				if( ch == DC1 || !ch )
					{
					isRunCount = ( ch == DC1 );
					continue;
					}

				/* Output char with high bits toggled; if it's a NL, prepend
				   a CR */
				if( ( ch ^ 0x80 ) == '\n' )
					putMrglByte( '\r' );
				putMrglByte( ch ^ 0x80 );
				}

			_outByteCount = 0;		/* Re-reset byte counter */
			return;					/* Leave now */
			}
		else
			{
			if( xlateFlags & XLATE_EBCDIC )
				/* Translate buffer from EBCDIC -> ASCII */
				while( _outByteCount < bufSize )
					{
					_outBuffer[ _outByteCount ] = xlateTbl[ _outBuffer[ _outByteCount ] ];
					_outByteCount++;	/* Can't do above - used on LHS and RHS */
					}

			_outByteCount = 0;		/* Reset byte counter */

			if( xlateFlags & XLATE_EOL )
				{
				/* Perform EOL translation */
				while( _outByteCount < bufSize )
					{
					/* Check whether we need to translate this char */
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
					if( ( ch = _outBuffer[ _outByteCount++ ] ) == lineEndChar )
						{
						/* Output CRLF in place of existing line terminator */
						putMrglByte( '\r' );
						putMrglByte( '\n' );
						}
#else
					if( ( ch = _outBuffer[ _outByteCount++ ] ) == ( lineEndChar & 0x7F ) ||
						( crSeen && ch == '\n' ) )
						{
						/* Check for CR of CRLF pair if necessary */
						if( ( lineEndChar & 0x80 ) && ch == '\r' )
							{
							/* Seen CR of CRLF pair */
							crSeen = TRUE;
							continue;
							}

						if( !crSeen || ( crSeen && ch == '\n' ) )
							/* See either CR alone, LF alone, or CRLF */
  #if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __UNIX__ )
							putMrglByte( '\n' );
  #elif defined( __MAC__ )
							putMrglByte( '\r' );
  #endif /* Various OS-specific defines */
						else
							/* Must have been a spurious CR-only */
							if( ch != CPM_EOF )
								putMrglByte( ch );
						crSeen = FALSE;
						}
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
					else
						/* Copy the value across directly */
						if( ch != CPM_EOF )
							putMrglByte( ch );
					}

				_outByteCount = 0;		/* Re-reset byte counter */
				return;					/* Leave now */
				}
			}
		}

	/* Call internal writeBuffer routine to do the actual write */
	vwrite( _outBuffer, bufSize );
	}

void writeMrglBuffer( const int bufSize )
	{
	mrglBufCount = 0;	/* Reset byte counter */

	/* Call internal writeBuffer routine to do the actual write */
	vwrite( mrglBuffer, bufSize );
	}

void writeDirBuffer( const int bufSize )
	{
	int bytesWritten;

	/* Checksum the data if required.  We don't bother with encryption since
	   eventually this data will be copied to the main archive file, and will
	   be encrypted as it goes through the main buffers */
	if( doChecksumDirOut )
		{
		crc16buffer( dirBuffer + dirChecksumMark, bufSize - dirChecksumMark );
		dirChecksumMark = 0;
		}

	dirBufCount = 0;	/* Reset byte counter */
	if( ( bytesWritten = hwrite( dirFileFD, dirBuffer, bufSize ) ) == ERROR )
		fileError();
	else
		/* Since the dirBuffer is only ever written to disk we don't need
		   to check for ^Z irregularities under MSDOS */
		if( bytesWritten != bufSize )
			error( OUT_OF_DISK_SPACE );
	}

/* Flush the output buffers */

void flushBuffer( void )
	{
	int i;

	/* We need to check for both the translate-output option and whether any
	   of the translate-flags are set, since translation may be temporarily
	   turned off if the file is non-text */
	if( flags & XLATE_OUTPUT && xlateFlags )
		{
		/* If we are translating the output, first force the translation of
		   the rest of the output buffer with a call to writeBuffer, and then
		   flush the mrgl buffer if necessary */
		if( _outByteCount )
			writeBuffer( _outByteCount );
		flushMrglBuffer();
		}
	else
		/* Otherwise flush the std.output buffer */
		if( _outByteCount )
			{
			/* Encrypt the data if required */
			if( doCryptOut )
				{
				encryptCFB( _outBuffer + cryptMark, _outByteCount - cryptMark );
				cryptMark = 0;
				}

			/* Checksum the data if required */
			if( doChecksumOut )
				{
				crc16buffer( _outBuffer + checksumMark, _outByteCount - checksumMark );
				checksumMark = 0;
				}

			/* If we need to pad the output for a multipart archive, add
			   another MULTIPART_TRAILER_SIZE bytes of data */
			if( padOutput )
				{
				for( i = 0; i < MULTIPART_TRAILER_SIZE; i++ )
					fputByte( 0 );
				outputAdjusted = FALSE;		/* Remember to fiddle things later */

				/* Keep calling writeBuffer() until all data is flushed out
				   of the output buffer (we may need to call writeBuffer()
				   several times if the data is split over multiple disks) */
				while( _outByteCount )
					writeBuffer( _outByteCount );
				}
			else
				writeBuffer( _outByteCount );

			/* Adjust data size for padding if it hasn't already been done */
			if( padOutput && !outputAdjusted )
				currLength -= MULTIPART_TRAILER_SIZE;
			}
	}

void flushMrglBuffer( void )
	{
	if( mrglBufCount )
		writeMrglBuffer( mrglBufCount );
	}

void flushDirBuffer( void )
	{
	if( dirBufCount )
		writeDirBuffer( dirBufCount );
	}
