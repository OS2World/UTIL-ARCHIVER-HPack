/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						 	  Error Handling Routines						*
*							 ERROR.C  Updated 01/08/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1991  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef __MSDOS16__
  #include <stdarg.h>
  #include <stdio.h>
#endif /* !__MSDOS16__ */
#include <stdlib.h>
#include "defs.h"
#include "arcdir.h"
#include "error.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "io\hpackio.h"
  #include "io\fastio.h"
  #include "language\language.h"
  #include "store\store.h"
#elif defined( __MAC__ )
  #include "hpackio.h"
  #include "fastio.h"
  #include "language.h"
  #include "store.h"
#else
  #include "io/hpackio.h"
  #include "io/fastio.h"
  #include "language/language.h"
  #include "store/store.h"
#endif /* System-specific include directives */

/* Note that some of the following variables must be initialized to 'empty'
   values initially since we may call error() before they are set to a
   sensible value at runtime.  These variables are: errorFD, dirFileFD,
   workDrive, and oldArcEnd */

char errorFileName[ MAX_PATH ];	/* The name of the current output file (used
								   to delete it in case of error) */
FD errorFD = IO_ERROR;	/* The FD of the current output file.  This MUST be
						   closed before the corresponding errorFileName is
						   deleted.  Not closing it results in loss of disk
						   space since it will still be allocated, but the
						   directory entry for it will have been wiped */

char dirFileName[ MAX_PATH ];	/* The name of the directory special info file
								   (if used) */
FD dirFileFD = IO_ERROR;		/* The FD of the directory special info file */

char secFileName[ MAX_PATH ];	/* The name of the security info file (if used */
FD secFileFD = IO_ERROR;		/* The FD of the security infor file */
int secInfoLen;					/* The length of the security info */

#ifdef __MAC__
  FD resourceTmpFD = IO_ERROR;	/* The FD of the temp file for resource forks */
#endif /* __MAC__ */

/* The original state of the archive */

long oldArcEnd = 0L;	/* The end of the archive before files were [A]dded */
void *oldHdrlistEnd;	/* End of list of file headers before files were [A]dded */
int oldDirEnd;
unsigned int oldDirNameEnd;

#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  extern int fileErrno;	/* I/O error code */
#endif /* __MSDOS16__ || __MSDOS32__ */
#ifdef __VMS__
  int translateVMSretVal( const int retVal );
#endif /* __VMS__ */
#ifdef __AMIGA__
  void clearLocks( void );
#endif /* __AMIGA__ */

/* The archiveFD.  This must be made global to support multipart archives
   since the low-level read and write routines change it whenever they move
   to another section of a multipart archive */

FD archiveFD;

/****************************************************************************
*																			*
*							Exit with an Error Message						*
*																			*
****************************************************************************/

static BOOLEAN recurseCheck = FALSE;	/* Used to check for recursive calls to error() */

/* Exit with an error message.  Note that it may be necessary to call the
   functions endPack(), endArcDir(), endFastIO(), and endMem()/freeFileSpecs().
   However if an error occurs during these functions we will end up doing
   things like dereferencing null pointers unless a lot of extra code is added
   to check for this sort of thing, thus we rely on the .crt0 code to handle
   this */

void error( char *errMsg, ... )
	{
#if !defined( __MSDOS16__ ) || defined( GUI )
  #ifdef NO_STDARG
	char buffer[ 20 ];				/* Buffer for string conversions */
	char *format = errMsg + 1;		/* Pointer to format string */
  #endif /* NO_STDARG */
	va_list argPtr;					/* Argument list pointer */
#endif /* !__MSDOS16__ || GUI */

#ifndef GUI
	hprintf( MESG_ERROR );			/* Print initial part of error message */
#endif /* !GUI */
#if defined( __MSDOS16__ ) && !defined( GUI )
	hvprintf();	/* Print with args off current stack frame + other nastiness */
#elif defined( NO_STDARG )
	/* vprintf() for systems which don't have it.  This only handles the
	   very few cases which are present in error messages ('%s', '%c'),
	   with two example cases ('%%' and '%d') being included to show how
	   these variations would be handled */
	va_start( argPtr, format );
	while( *format )
		{
		if( *format == '%' )
			switch( *++format )
				{
				/* String */
				case 's':
					fputs( va_arg( argPtr, ( char * ), stdout );
					format++;
					break;

				/* Single character */
				case 'c':
					putchar( va_arg( argPtr, char ) );
					format++;
					break;

  #if 0	/* Not needed - included only as an example */
				/* Percent sign */
				case '%':
					putchar( '%' );
					format++;
					break;

				/* Decimal integer */
				case 'd':
					fputs( itoa( va_arg( argPtr, int ), buffer, 10 ), stdout );
					format++;
					break;
  #endif /* 0 */
				/* Unknown option */
				default:
					putchar( *format );
					format++;
				}
		else
			{
			putchar( *format );
			format++;
			}
		}
	va_end( argPtr );
#else
	va_start( argPtr, errMsg );		/* Initialize va_ functions	*/
	vprintf( errMsg + 1, argPtr );	/* Print string */
	va_end( argPtr );				/* Close va_ functions */
#endif /* NO_STDARG */
#ifndef GUI
	hputchar( '\n' );
#endif /* !GUI */

#ifdef __AMIGA__
	/* Clear any directory locks still in place, before we do any mungeing
	   of files */
	clearLocks();
#endif /* __AMIGA__ */

	/* Shut down the encryption system */
	endCrypt();

	/* Shut down extraInfo handling */
	endExtraInfo();

	/* Do a nasty check for recursive calls to error() via writeArcDir() */
	if( recurseCheck )
		{
		/* Moth detected in relay bank #6 */
#ifdef GUI
		alert( ALERT_ERROR_DURING_ERROR_REC, NULL );
#else
		hputs( MESG_ERROR_DURING_ERROR_RECOVERY );
#endif /* GUI */
#ifdef __VMS__
		exit( translateVMSretVal( ( int ) *INTERNAL_ERROR ) );
#else
		exit( ( int ) *INTERNAL_ERROR );
#endif /* __VMS__ */
		}
	recurseCheck = TRUE;

	/* If we had an error during [A]dd, restore the old archive end exit */
	if( oldArcEnd )
		{
		/* Only attempt recovery if we've changed the archive */
		if( archiveChanged )
			{
			hlseek( archiveFD, oldArcEnd + HPACK_ID_SIZE, SEEK_SET );
			setArcdirState( oldHdrlistEnd, oldDirEnd );
			resetFastOut();			/* Clear output buffer */
			writeArcDir();

			/* Append the old security and trailer info if necessary */
			if( secFileFD != IO_ERROR )
				{
				hlseek( secFileFD, 0, SEEK_SET );
				hlseek( archiveFD, -( long ) ( sizeof( BYTE ) + HPACK_ID_SIZE ), SEEK_CUR );
				setInputFD( secFileFD );
				moveData( secInfoLen + sizeof( WORD ) + sizeof( BYTE ) + HPACK_ID_SIZE );
				flushBuffer();

				/* Zap the security info file */
				hclose( secFileFD );
				hunlink( secFileName );
				secFileFD = IO_ERROR;
				}

			htruncate( archiveFD );
			hclose( archiveFD );
			}

		/* Don't do anything more to archiveFile, secFile or errorFile */
		archiveFD = errorFD = IO_ERROR;
		}

	/* Close and delete any necessary files */
	closeFiles();

	/* Exeunt with divers alarums */
#ifdef __VMS__
	exit( translateVMSretVal( ( int ) *errMsg ) );
#else
	exit( ( int ) *errMsg );
#endif /* __VMS__ */
#pragma warn -par					/* Suppress 'Arg not used' warning */
	}
#pragma warn +par

/* Print an OS-specific error message for a file I/O error.  This routine
   is called if an error occurs during a file R/W operation, and thus should
   only be expected to handle problems that will not already have been
   caught in the hopen() call */

void fileError( void )
	{
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
	char *errMsg;

	switch( fileErrno )
		{
		case 0x20:
			errMsg = OSMESG_FILE_SHARING_VIOLATION;
			break;

		case 0x21:
			errMsg = OSMESG_FILE_LOCKING_VIOLATION;
			break;

		default:
			error( FILE_ERROR, fileErrno );
		}
	error( errMsg );
#else
	error( FILE_ERROR );
#endif /* __MSDOS16__ || __MSDOS32__ */
	}

/* Close files (and delete incorrect one if need be).  We have to be very
   finicky here since some OS's have rather poor resource management and
   can't keep track of open/closed files themselves */

void closeFiles( void )
	{
	if( archiveFD != IO_ERROR )
		{
		hclose( archiveFD );
		archiveFD = IO_ERROR;
		}
	if( errorFD != IO_ERROR )
		{
		hclose( errorFD );
		hunlink( errorFileName );
		errorFD = IO_ERROR;
		}
	if( dirFileFD != IO_ERROR )
		{
		hclose( dirFileFD );
		hunlink( dirFileName );
		dirFileFD = IO_ERROR;
		}
	if( secFileFD != IO_ERROR )
		{
		hclose( secFileFD );
		hunlink( secFileName );
		secFileFD = IO_ERROR;
		}
#ifdef __MAC__
	if( resourceTmpFD != IO_ERROR )
		{
		hclose( resourceTmpFD );
		hunlink( RESOURCE_TMPNAME );
		resourceTmpFD = IO_ERROR;
		}
#endif /* __MAC__ */
	}
