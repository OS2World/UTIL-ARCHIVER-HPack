/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   Archimedes-Specific Routines						*
*							 ARC.C  Updated 21/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992  Peter Gutmann, Jason Williams, and Edouard Poor.	*
*								All rights reserved							*
*																			*
****************************************************************************/

/* The Archimedes makes all system calls via software interrupts (SWI's).
   Some SWI's have 16 different versions of the same call, depending on
   which way the wind is blowing at the time, but with 32-bit registers to
   specify the call type you can probably get away with this.

   In addition, Risc-OS handles all data in terms of 32-bit unsigned values.
   This means a lot of the following code is pretty Vaxocentric (well,
   Arcocentric), ie assuming type int = type char * and so on.

   "The determined programmer can write BBC Basic V in *any* language"
											- Rastaman Ja */

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "arcdir.h"
#include "flags.h"
#include "frontend.h"
#include "system.h"
#include "wildcard.h"
#include "io/fastio.h"

#include "kernel.h"

/* The time difference between the Archimedes and Unix epochs, in seconds */

#define ARC_TIME_OFFSET		0x83AA7E80

/* Function numbers for SWI's (SWI number).  Make these 0x200xx to repress
   errors */

#define OS_Byte			0x06
#define OS_File			0x08
#define OS_Args			0x09
#define OS_Find			0x0D
#define OS_GBPB			0x0C		/* The famous heebeegeebee call */
#define OS_FSControl	0x29
#define OS_ReadModeVar	0x35
#define OS_WriteN		0x46
#define OS_ReadSysInfo	0x58

/* Subfunctions for the above (R0 value).  The main thing to watch out for
   is that there are about 20 ways of doing anything.  Some of them are
   interchangeable */

#define SET_CATINFO		0x01		/* Set catalogue info (used for SetTime) */
#define SET_LOADADDR	0x02		/* Set load address */
#define SET_EXECADDR	0x03		/* Set execute address */
#define SET_ATTRIBUTE	0x04		/* Set file/dir attribute */
#define GET_CATINFO		0x05		/* Get catalogue info (used for SetTime) */
#define DELETE_FILE		0x06		/* Delete a file/directory */
#define CREATE_DIR		0x08		/* Create a directory */
#define SET_FILETYPE	0x12		/* Set a file's type */

#define READ_POINTER	0x00		/* Read current file pointer */
#define WRITE_POINTER	0x01		/* Write current file pointer */
#define READ_EXTENT		0x02		/* Read size of file */
#define WRITE_EXTENT	0x03		/* Write size of file */
#define ENSURE_FILE		0xFF		/* Ensure all data is written to file */

#define CLOSE_FILE		0x00		/* Close a file (one of several variants) */
#define CREATE_FILE		0x83		/* Create a file (one of many variants) */
#define OPEN_READ		0x43		/* Open file for read access */
#define OPEN_UPDATE		0xC3		/* Open file for R/W access */

#define WRITE_BYTES		0x02		/* Write data (one of several variants) */
#define READ_BYTES		0x04		/* Read data (one of several variants) */
#define READ_DIRINFO	0x0A		/* Read directory info (one of many) */

#define RENAME			0x19		/* Rename file/directory */

#define READ_MAGIC_ID	0x02		/* Read system magic numbers */

/* Various file types */

#define FILETYPE_ARCHIVE	0x00000DDCL		/* Sparc data file */
#define FILETYPE_DOS		0x00000FE4L		/* MSDOS data file */
#define FILETYPE_DATA		0x00000FFDL		/* Generic data file */
#define FILETYPE_TEXT		0x00000FFFL		/*Text file */

/* File object types */

#define FILEOBJ_FILE	1
#define FILEOBJ_DIR		2

/* The routine to call SWI's from C */

int SWI( int inRegCount, int outRegCount, int swiCode, ... )
	{
	va_list argPtr;
	_kernel_swi_regs regsIn, regsOut;
	_kernel_oserror *errorInfo;
	int count;
	int *temp;

	va_start( argPtr, swiCode );

	/* Pull the input registers off the stack */
	for( count = 0; count < inRegCount; count++ )
		regsIn.r[ count ] = va_arg( argPtr, int );

	if( ( errorInfo = _kernel_swi( swiCode, &regsIn, &regsOut ) ) != NULL )
		{
		printf( "Argh! SWI error! %s\n", errorInfo->errmess );
		va_end( argPtr );
		return( ERROR );
		}

	/* Put the returned values into the output vars */
	for( count = 0; count < outRegCount; count++ )
		{
		temp = va_arg( argPtr, int * );
		if( temp != NULL )
			*temp = regsOut.r[ count ];
		}

	va_end( argPtr );
	return( OK );
	}

/****************************************************************************
*																			*
*								HPACKIO Functions							*
*																			*
****************************************************************************/

/* Create a new file */

FD hcreat( const char *fileName, const int attr )
	{
	LONG r0;

	if( attr );			/* Get rid of unused parameter warning */
	if( SWI( 2, 1, OS_Find, CREATE_FILE, ( LONG ) fileName, &r0 ) == ERROR )
		return( IO_ERROR );
	else
		return( r0 ? ( FD ) r0 : IO_ERROR );
	}

/* Open an existing file.  We have to be careful here since even if the file
   doesn't exist the call will still return a FD of 0, and performing a
   subsequent courtesy close on this FD has the cute side-effect of closing
   *all* open FD's */

FD hopen( const char *fileName, const int mode )
	{
	LONG r0;
	int status;

	if( mode == O_RDONLY )
		status = SWI( 2, 1, OS_Find, OPEN_READ, fileName, &r0 );
	else
		status = SWI( 2, 1, OS_Find, OPEN_UPDATE, fileName, &r0 );
	return( ( status == ERROR || !r0 ) ? IO_ERROR : ( FD ) r0 );
	}

/* Close a file */

int hclose( const FD theFD )
	{
	if( SWI( 3, 0, OS_Find, CLOSE_FILE, ( LONG ) theFD, 0L ) == ERROR )
		return( IO_ERROR );
	else
		return( OK );
	}

/* Read data from a file */

int hread( const FD theFD, void *buffer, const unsigned int bufSize )
	{
	LONG r0, r1, r2, r3;

	if( SWI( 4, 4, OS_GBPB, READ_BYTES, ( LONG ) theFD, buffer, ( LONG ) bufSize, \
			 &r0, &r1, &r2, &r3 ) == ERROR )
		return( IO_ERROR );
	else
		return( ( int ) ( bufSize - r3 ) );
	}

/* Write data to a file */

int hwrite( const FD theFD, void *buffer, const unsigned int bufSize )
	{
	LONG r0, r1, r2;

	if( SWI( 4, 3, OS_GBPB, WRITE_BYTES, ( LONG ) theFD, buffer, ( LONG ) bufSize, \
			 &r0, &r1, &r2 ) == ERROR )
		return( 0 );
	else
		return( ( int ) ( r2 - ( LONG ) buffer ) );
	}

/* Seek to a position in a file.  The Arc can only do an absolute seek so
   we need to figure out where to move the pointer ourselves */

long hlseek( const FD theFD, const long position, const int whence )
	{
	LONG r0, r1, r2;

	if( whence == SEEK_SET )
		/* Just move the pointer to where we want to go */
		SWI( 3, 0, OS_Args, WRITE_POINTER, ( LONG ) theFD, position );
	else
		if( whence == SEEK_CUR )
			{
			/* Find out where we are, add the offset, and move there */
			SWI(3, 3, OS_Args, READ_POINTER, ( LONG ) theFD, 0L, &r0, &r1, &r2 );
			SWI(3, 0, OS_Args, WRITE_POINTER, ( LONG ) theFD, r2 + position );
			}
		else
			{
			/* Force all data for the file handle to disk, find out how big
			   the file is now, then seek back from there */
			SWI( 2, 0, OS_Args, ENSURE_FILE, ( LONG ) theFD );
			SWI( 2, 3, OS_Args, READ_EXTENT, ( LONG ) theFD, &r0, &r1, &r2 );
			SWI( 3, 0, OS_Args, WRITE_POINTER, ( LONG ) theFD, r2 + position );
			}

	if( SWI( 3, 3, OS_Args, READ_POINTER, ( LONG ) theFD, 0L, &r0, &r1, &r2 ) == ERROR )
		return( IO_ERROR );
	else
		return( ( long ) r2 );
	}

/* Return the current position in the file */

long htell( const FD theFD )
	{
	LONG r0, r1, r2;

	if( SWI( 3, 3, OS_Args, READ_POINTER, ( LONG ) theFD, 0L, &r0, &r1, &r2 ) == ERROR )
		return( IO_ERROR );
	else
		return( ( long ) r2 );
	}

/* Truncate a file at the current position by finding where we are and then
   setting this as the new extent */

int htruncate( const FD theFD )
	{
	LONG r0, r1, r2;

	if( SWI( 3, 3, OS_Args, READ_POINTER, ( LONG ) theFD, 0L, &r0, &r1, &r2 ) != ERROR && \
		SWI( 3, 0, OS_Args, WRITE_EXTENT, ( LONG ) theFD, r2 ) != ERROR )
		return( OK );
	else
		return( IO_ERROR );
	}

/* Remove a file */

int hunlink( const char *fileName )
	{
	if( SWI( 2, 0, OS_File, DELETE_FILE, fileName ) == ERROR )
		return( IO_ERROR );
	else
		return( OK );
	}

/* Create a directory */

int hmkdir( const char *dirName, const int attr )
	{
	if( attr );			/* Get rid of unused parameter warning */
	if( SWI( 5, 0, OS_File, CREATE_DIR, dirName, 0L, 0L, 0L ) == ERROR )
		return( ERROR );
	else
		return( OK );
	}

/* Rename a file */

int hrename( const char *srcName, const char *destName )
	{
	if( SWI( 3, 0, OS_FSControl, RENAME, ( LONG ) srcName, ( LONG ) destName ) == ERROR )
		return( ERROR );
	else
		return( OK );
	}

/* Set/change a file/dirs attributes */

int hchmod( const char *fileName, const int attr )
	{
	if( SWI( 6, 0, OS_File, SET_ATTRIBUTE, fileName, 0L, 0L, 0L, ( LONG ) attr ) == ERROR )
		return( ERROR );
	else
		return( OK );
	}

/****************************************************************************
*																			*
*								HPACKLIB Functions							*
*																			*
****************************************************************************/

/* Get an input character, no echo */

int hgetch( void )
	{
	LONG r0, r1, r2 = 255;

	while( r2 != 0 )
		SWI( 3, 3, OS_Byte, 129, 0, 100, &r0, &r1, &r2 );

	return( ( int ) r1 );
	}

/****************************************************************************
*																			*
*								SYSTEM Functions							*
*																			*
****************************************************************************/

/* Set file time */

void setFileTime( const char *fileName, const LONG time )
	{
	LONG r0, r1, r2, r3, r4, r5;
	LONG low, high;

	SWI( 2, 6, OS_File, GET_CATINFO, fileName, &r0, &r1, &r2, &r3, &r4, &r5 );
	low = time + ARC_TIME_OFFSET;
	high = ( low >> 16 ) * 100;
	low = ( low & 0xFFFF ) * 100;
	r3 = ( high << 16 ) + low;
	r2 = ( r2 & 0xFFFFFF00 ) + ( high >> 16 );
	SWI( 6, 0, OS_File, SET_CATINFO, fileName, r2, r3, r4, r5 );
	}

/* Set file type */

void setFileType( const char *fileName, const WORD type )
	{
	SWI( 3, 0, OS_File, SET_FILETYPE, fileName, ( LONG ) type );
	}

/* Set file load, execute address */

void setLoadAddress( const char *fileName, const LONG loadAddr )
	{
	SWI( 3, 0, OS_File, SET_LOADADDR, fileName, loadAddr );
	}

void setExecAddress( const char *fileName, const LONG execAddr )
	{
	SWI( 4, 0,  OS_File, SET_EXECADDR, fileName, 0L, execAddr );
	}

/* Find the first/next file in a directory.  Since Risc-OS doesn't keep
   track of where we are via some sort of magic number, we need to do a
   certain amount of work to remember the path and filespec we're processing.
   In addition there is a bug in the heebeegeebee routines in that only a
   fileSpec of '*' will work, so we need to keep track of the true fileSpec
   and perform the matching ourselves.  Finally, in order to improve
   performance we need to buffer multiple entries in memory, and handle
   access to variable-length records aligned to longword boundaries.  Change
   the following code at your own risk! */

typedef struct {
			   LONG loadAddr;		/* Program load address */
			   LONG execAddr;		/* Program execute address */
			   LONG length;			/* File length */
			   LONG attr;			/* File attributes */
			   LONG objType;		/* File object type */
			   BYTE name[ 1 ];		/* File name */
			   } ARC_FILEINFO;

BOOLEAN findFirst( const char *filePath, const ATTR fileAttr, FILEINFO *fileInfo )
	{
	char *fileNamePtr, *pathPtr = fileInfo->dirPath;
	int filePathLen = strlen( filePath );

	/* Set up Risc-OS bookkeeping information */
	fileInfo->dirPos = 0L;
	fileInfo->currEntry = DIRBUF_ENTRIES;	/* Force a read */
	fileInfo->matchAttr = fileAttr;
	fileInfo->wantArchive = FALSE;
	strcpy( fileInfo->dirPath, filePath );

	/* Now find the last directory seperator */
	for( fileNamePtr = pathPtr + strlen( pathPtr ); \
		 fileNamePtr >= pathPtr && *fileNamePtr != SLASH; \
		 fileNamePtr-- );
	if( ( fileNamePtr >= pathPtr ) && ( *fileNamePtr == SLASH ) )
		*fileNamePtr = '\0';	/* Truncate to directory path only */
	fileNamePtr++;				/* Fix fencepost error */
	if( hasWildcards( fileNamePtr, strlen( fileNamePtr ) ) )
		/* Let the main code sort it out */
		strcpy( fileInfo->fileSpec, MATCH_ALL );
	else
		/* Need literal match, need to do special check in findNext() */
		strcpy( fileInfo->fileSpec, fileNamePtr );
	if( fileNamePtr == pathPtr )
		*fileInfo->dirPath = '\0';	/* Make sure we don't get fileSpec == dirPath */

	/* If we're looking for an HPACK archive, indicate that we need to
	   perform a special match since we can't check for the extension */
	if( !strcmp( filePath + filePathLen - 1, MATCH_ARCHIVE ) )
		{
		fileInfo->wantArchive = TRUE;
		fileInfo->dirPathLen = strlen( fileInfo->dirPath );
		strcpy( fileInfo->fileSpec, MATCH_ALL );
		}

	return( findNext( fileInfo ) );
	}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
	LONG r0, r1, r2;
	BOOLEAN doContinue = TRUE, hasType = TRUE;
	BYTE idBuffer[ HPACK_ID_SIZE ];
	ARC_FILEINFO *arcInfo;
	FD archiveFileFD;

	/* Try and read the directory information */
	do
		{
		/* Read in a bufferful of info if need be */
		if( fileInfo->currEntry == DIRBUF_ENTRIES )
			{
			if( SWI( 7, 5, OS_GBPB, READ_DIRINFO, fileInfo->dirPath, \
					 fileInfo->dirBuffer, DIRBUF_ENTRIES, fileInfo->dirPos, \
						DIRINFO_SIZE * DIRBUF_ENTRIES, "*", \
					 &r0, &r1, &r2, &fileInfo->totalEntries, &fileInfo->dirPos ) == ERROR )
				return( FALSE );
			fileInfo->currEntry = 0;	/* Reset buffer index */
			fileInfo->nextRecordPtr = ( void * ) fileInfo->dirBuffer;
			}

		if( ( fileInfo->totalEntries == 0 ) && ( fileInfo->dirPos < 0 ) )
			/* Check for exactly 0 entries read */
			return( FALSE );

		/* Move to the current record and set up a pointer to the next record */
		arcInfo = ( ARC_FILEINFO * ) fileInfo->nextRecordPtr;
		fileInfo->nextRecordPtr = ( char * ) arcInfo->name + strlen( ( char * ) arcInfo->name ) + 1;
		fileInfo->nextRecordPtr = ( char * ) ( ( ( int ) fileInfo->nextRecordPtr + 3 ) & ~3 );

		/* Extract the file's type */
		if( ( arcInfo->loadAddr & 0xFFF00000L ) == 0xFFF00000L )
			fileInfo->type = ( WORD ) ( arcInfo->loadAddr >> 8 ) & 0x0FFF;
		else
			{
			fileInfo->type = ( WORD ) FILETYPE_DATA;
			hasType = FALSE;
			}
		fileInfo->isDirectory = arcInfo->objType == FILEOBJ_DIR;

		/* If we're specifically looking for an archive file, make sure we
		   only match archives.  Specifically we look for files of type
		   ARCHIVE or DATA, and untyped files, which aren't directories */
		if( fileInfo->wantArchive )
			{
			if( !fileInfo->isDirectory && \
				( ( fileInfo->type == FILETYPE_ARCHIVE ) || \
				  ( fileInfo->type == FILETYPE_DATA ) || \
				  ( fileInfo->type == FILETYPE_DOS ) || \
				  ( fileInfo->type == FILETYPE_TEXT ) ) )
				{
				/* Try and open the file to look for the HPACK ID */
				if( *fileInfo->dirPath )
					{
					/* Append a SLASH and the filename */
					fileInfo->dirPath[ fileInfo->dirPathLen ] = SLASH;
					strcpy( fileInfo->dirPath + fileInfo->dirPathLen + 1, \
							( char * ) arcInfo->name );
					}
				else
					strcpy( fileInfo->dirPath, ( char * ) arcInfo->name );
				if( ( archiveFileFD = hopen( fileInfo->dirPath, O_RDONLY ) ) != ERROR )
					{
					/* Check for the HPACK ID bytes at the start of the archive */
					if( ( hread( archiveFileFD, idBuffer, HPACK_ID_SIZE ) == HPACK_ID_SIZE ) && \
						( !memcmp( idBuffer, "HPAK", 4 ) ) )
						doContinue = FALSE;
					hclose( archiveFileFD );
					}
				fileInfo->dirPath[ fileInfo->dirPathLen ] = '\0';
				}
			}
		else
			{
			/* Check whether we've found a directory */
			if( fileInfo->isDirectory )
				{
				/* If we want directories, indicate a match */
				if( fileInfo->matchAttr == ALLFILES_DIRS )
					doContinue = FALSE;
				}
			else
				/* We've found a file, assume a match unless told otherwise */
				doContinue = FALSE;

			/* Check for a literal match if we want one */
			if( !doContinue && strcmp( fileInfo->fileSpec, MATCH_ALL ) )
				doContinue = stricmp( fileInfo->fileSpec, ( char * ) arcInfo->name );
			}

		/* Move to next entry in buffer */
		fileInfo->currEntry++;
		if( fileInfo->currEntry >= fileInfo->totalEntries && fileInfo->dirPos < 0 )
			if( !doContinue )
				/* We've matched the last entry in the buffer, make sure we
				   exit the next time around */
				fileInfo->totalEntries = 0;
			else
				/* Out of entries and no more to read, return */
				return( FALSE );
		}
	while( doContinue );

	/* Copy the information into the fileInfo struct */
	if( hasType )
		{
#ifndef PETER_TIME
		LONG temp;

		/* Once every two years for approximately 3/4 second, adding files
		   will result in the date being wildly wrong.  Like Mel's reversed
		   loop test, this is a neat feature and not worth removing */
		fileInfo->fTime = ( arcInfo->execAddr + 72 ) / 100;
		temp = arcInfo->loadAddr << 24;
		fileInfo->fTime += ( ( temp % 100 ) << 8 ) / 100;
		fileInfo->fTime += ( temp / 100 ) << 8;
#else
		/* Divide five-byte time value by 100 using only shifts (17 cycles
		   total) */
		fileInfo->fTime = ( arcInfo->loadAddr << 25 ) | \
						  ( arcInfo->execAddr >> 7 );
		fileInfo->fTime += ( fileInfo->fTime >> 2 ) + \
						   ( fileInfo->fTime >> 6 ) + \
						   ( fileInfo->fTime >> 7 ) + \
						   ( fileInfo->fTime >> 8 ) + \
						   ( fileInfo->fTime >> 9 ) + \
						   ( fileInfo->fTime >> 11 ) + \
						   ( fileInfo->fTime >> 13 ) + \
						   ( fileInfo->fTime >> 15 ) + \
						   ( fileInfo->fTime >> 20 ) + \
						   ( fileInfo->fTime >> 22 ) + \
						   ( fileInfo->fTime >> 26 ) + \
						   ( fileInfo->fTime >> 27 ) + \
						   ( fileInfo->fTime >> 28 ) + \
						   ( fileInfo->fTime >> 29 ) + \
						   ( fileInfo->fTime >> 31 );
#endif /* PETER_TIME */
		fileInfo->fTime -= ARC_TIME_OFFSET;
		fileInfo->loadAddr = fileInfo->execAddr = ERROR;
		}
	else
		{
		fileInfo->type = ERROR;
		fileInfo->fTime = time( NULL );		/* Fake it with current time */
		fileInfo->loadAddr = arcInfo->loadAddr;
		fileInfo->execAddr = arcInfo->execAddr;
		}
	fileInfo->fSize = arcInfo->length;
	fileInfo->fAttr = ( WORD ) arcInfo->attr;
	strcpy( fileInfo->fName, ( char * ) arcInfo->name );

	return( TRUE );
	}

/* Set extra info for the archive */

void setExtraInfo( const char *archivePath )
	{
	setFileType( archivePath, ( WORD ) FILETYPE_ARCHIVE );
	}

/* Get any randomish information the system can give us (not terribly good,
   but every extra bit helps - if it's there we may as well use it) */

void getRandomInfo( BYTE *buffer, int bufSize )
	{
	LONG r0, r1, r2, r3, r4;

	/* Read system-dependant magic numbers: IOEB, peripheral info, LCD status,
	   and minor and major system serial numbers (UID1 and UID2) */
	SWI( 1, 5, OS_ReadSysInfo, READ_MAGIC_ID, &r0, &r1, &r2, &r3, &r4 );

	mputLong( buffer, r0 ^ r1 ^ r2 );
	mputLong( buffer + sizeof( LONG ), r3 );
	mputLong( buffer + ( 2 * sizeof( LONG ) ), r4 );
	}

/* Case-insensitive string comparisons */

int strnicmp( const char *src, const char *dest, int length )
	{
	char srcCh, destCh;
	char *srcPtr = ( char * ) src, *destPtr = ( char * ) dest;

	while( length-- )
		{
		/* Need to be careful with toupper() side-effects */
		srcCh = *srcPtr++;
		srcCh = toupper( srcCh );
		destCh = *destPtr++;
		destCh = toupper( destCh );

		if( srcCh != destCh )
			return( srcCh - destCh );
		}

	return( 0 );
	}

int stricmp( const char *src, const char *dest )
	{
	int srcLen = strlen( src ), destLen = strlen( dest );

	if( srcLen != destLen )
		return( srcLen - destLen );
	return( strnicmp( src, dest, ( srcLen > destLen ) ? srcLen : destLen ) );
	}

/* Lowercase a string */

void strlwr( char *string )
	{
	while( *string )
		{
		*string = tolower( *string );
		string++;
		}
	}

static int bbcModeVar( int varNo )
	{
	LONG r0, r1, r2;

	SWI( 3, 3, OS_ReadModeVar, -1, ( LONG ) varNo, 0, &r0, &r1, &r2 );
	return( ( int ) r2 );
	}

void getScreenSize( void )
	{
	screenWidth  = bbcModeVar( 1 ) - 1;
	screenHeight = bbcModeVar( 2 );	/* They tell me the -1 isn't needed here */
	}

/* "Well we can do this under Risc-OS 3...." */

int getCountry( void )
	{
	return( 1 );		/* Default to UK */
	}

/* Determine whether two filenames refer to the same file.  We cheat a
   little by taking advantage of the fact that the second filename probably
   refers to a file which we know is open in exclusive access mode; if
   another hopen() call to it fails, we can assume they're the same */

BOOLEAN isSameFile( const char *pathName1, const char *pathName2 )
	{
	FD theFD;

	if( ( theFD = hopen( pathName2, O_RDONLY | S_DENYWR ) ) == ERROR )
		return( TRUE );
	hclose( theFD );
	return( FALSE );
	}

/* Write out a single char.  Used to fix stdio's GSTrans-ing of
   control characters */

void hputchar( const int ch )
	{
	if( ch == '\n' )
		SWI( 0, 0, 256 + '\r' );
	SWI( 0, 0, 256 + ch );
	}

/* Write a block of data to what would would be STDOUT on other machines */

void writeToStdout( BYTE *buffer, int bufSize )
	{
	SWI( 2, 0, OS_WriteN, buffer, bufSize );
	}
