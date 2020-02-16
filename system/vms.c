/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  VMS-Specific Routines							*
*							 VMS.C  Updated 30/06/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
* 		Copyright 1992  Some poor sucker :-) and Peter C. Gutmann.			*
*							All rights reserved								*
*																			*
****************************************************************************/

/* VAXORCIST:  Everything looks okay to me.

   SYSMGR:  Maybe it's hibernating.

   VAXORCIST:  Unlikely.  It's probably trying to lure us into a false sense
			   of security.

   SYSMGR:  Sounds like VMS alright.

   - Christopher Russell */

#include <curses.h>
#include <descrip.h>
#include <iodef.h>
#include <ssdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stsdef.h>
#include "defs.h"
#include "error.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#include "tags.h"
#include "io/hpackio.h"

/* Translate HPACK return codes into a VMS-compatible format */

int translateVMSretVal( const int retVal )
	{
	/* Return success */
	if( retVal == OK )
		return( STS$K_CONTROL_INHIB_MSG | STS$K_SUCCESS );

	/* Return severity level along with error encoded into customer-defined
	   facility number field */
	return( STS$K_CONTROL_INHIB_MSG | STS$K_FAC_NO_CUST_DEF | \
			( retVal << STS$V_FAC_NO ) | ( retVal < 100 ) ? STS$K_FATAL : \
			( retVal < 200 ) ? STS$K_ERROR : STS$K_WARNING );
	}

/* Translate a filespec from canonical to VMS format */

char *translateToVMS( char *pathName, const BOOLEAN isDirPath )
	{
	static char vmsName[ MAX_PATH ];
	int pathNameLen = strlen( pathName );
	int i, startPos = 0, lastSlash = ERROR;

	/* First, look for a node/device component */
	for( i = 0; i < pathNameLen; i++ )
		if( pathName[ i ] == ':' )
			startPos = i + 1;			/* Include ':' */

	/* Copy across the node/device component if it exists */
	if( startPos )
		{
		strcpy( vmsName, pathName );
		pathName += startPos;
		pathNameLen -= startPos;
		}

	/* Now scan for directory delimiters */
	for( i = startPos; i < pathNameLen; i++ )
		if( pathName[ i ] == SLASH )
			lastSlash = i;

	/* If it's a directory path, there is no filename component */
	if( isDirPath )
		lastSlash = pathNameLen;

	/* If there is a path, translate it to the VMS format */
	if( lastSlash != ERROR )
		{
		vmsName[ startPos++ ] = '[';
		for( i = 0; i < lastSlash; i++ )
			vmsName[ startPos++ ] = ( pathName[ i ] == SLASH ) ? '.' : pathName[ i ];
		vmsName[ startPos++ ] = ']';
		pathName += lastSlash + !isDirPath;	/* + 1 to skip SLASH if there is one */
		}

	/* Finally, add the filename component */
	strcpy( vmsName + startPos, pathName );

	return( vmsName );
	}

/****************************************************************************
*																			*
*								HPACKLIB Functions							*
*																			*
****************************************************************************/

/* Get an input character, no echo */

#include <ssdef.h>
#include <iodef.h>

int hgetch( void )
	{
	int	tt[ 2 ];
	int	channel;
	int	status;
	char ch[ 4 ];
	struct
		{
		short int status;
		short int trans_cnt;
		long int dev_dependant;
		} ioBlock;

	tt[ 0 ] = strlen( tt[ 1 ] = ( int ) "tt:" );
	if( ~sys$assign( tt, &channel, 0, 0 ) & 1 )
		return( ERROR );

	status = sys$qiow( 0,			/* Event Flag number */
					   channel,		/* Channel to device */
					   IO$_READVBLK,	/* Function code */
					   ioBlock,		/* I/O Status Block */
					   0,			/* AST routine address */
					   0,			/* AST parameter address */
					   &ch,			/* P1 - address of buffer for bytes */
					   1,			/* P2 - number of bytes to read */
					   0, 0, 0, 0 );	/* Parameter 3-6 not used */
	if( ( ( ~status ) & 1 ) || ( ( ~ioBlock.status ) & 1 ) )
		return( ERROR );
	}

#if 0

/* Alternative, simpler method which requires curses */

int hgetch( void )
	{
	char ch;

	noecho();
	ch = getch();
	echo();
	return( ch );
	}

#endif /* 0 */

/****************************************************************************
*																			*
*								HPACKIO Functions							*
*																			*
****************************************************************************/

/* Set a file's timestamp */

void setFileTime( const char *fileName, const LONG fileTime )
	{
	puts( "Need to implement setFileTime" );
	}

/* Rename a file */

void rename( const char *srcFile, const char *destFile )
	{
	char vmsSrcFile[ MAX_PATH ], vmsDestFile[ MAX_PATH ];

	translateToVMS( srcFile, vmsSrcFile );
	translateToVMS( destFile, vmsDestFile );

	puts( "Need to implement rename" );
	}

/* Create a directory.  This is like the usual mkdir() but we translate the
   path into a VMS format (Question:  How far can *you* throw VMS?) */

int hmkdir( const char *dirName, const int attr )
	{
	char vmsDirName[ MAX_PATH ];

	translateToVMS( dirName, vmsDirName );
	mkdir( vmsDirName, attr );
	}

/* opendir(), readdir(), closedir() for Unix.  Presumably the VMS findFirst()
   can be based on these + the Unix findFirst() code? */

#if 0
#include <fcntl.h>
#include <sys/types.h>

#include <dir.h>

#define DIRBUFSIZE		512

typedef struct {
			   struct stat d_stat;		/* stat() info for dir */
			   char d_name[ MAX_FILENAME ];
			   } DIRECT;

typedef struct {
			   int dd_fd;				/* This directories' FD */
			   int dd_loc;				/* Index into buffer of dir info */
			   int dd_size;				/* No. of entries in buffer */
			   char dd_buf[ DIRBUFSIZE ];	/* Buffer of dir entries */
			   } DIR;

DIR *opendir( const char *dirName )
	{
	DIR *dirPtr;
	FD dirFD;

	/* Try and open directory corresponding to dirName */
	if( ( dirFD = hopen( dirName, O_RDONLY ) ) != FILE_ERROR )
		{
		/* Allocate room for directory information */
		if( ( dirPtr = ( DIR * ) hmalloc( sizeof( DIR ) ) ) != NULL )
			{
			/* Set up directory information */
			dirPtr->dd_fd = dirFD;
			dirPtr->dd_loc = 0;
			dirPtr->dd_size = 0;
			return( dirPtr );
			}
		hclose( dirFD );
		}
	return( NULL );
	}

/* Read out the next directory entry */

DIRECT *readdir( DIR *dirPtr )
	{
	static DIRECT dirInfo;			/* Dir.info as we want it set out */
	struct direct *diskDirInfo;		/* Dir.info as stored on disk */

	/* Grovel through dir.entries until we find a non-deleted file */
	do
		{
		/* Check if we need to read in more of dir */
		if( dirPtr->dd_loc >= dirPtr->dd_size )
			{
			/* Yes, read in next load of info */
			dirPtr->dd_loc = 0;
			if( ( dirPtr->dd_size = hread( dirPtr->dd_fd, dirPtr->dd_buf, DIRBUFSIZE ) ) == FILE_ERROR )
				/* Out of directory, return NULL dirinfo */
				return( NULL );
			}

		/* Extract this directory entry and point to next location in buffer */
		diskDirInfo = ( struct direct * ) ( dirPtr->dd_buf + dirPtr->dd_loc );
		dirPtr->dd_loc += sizeof( struct direct );
		}
	while( diskDirInfo->d_ino == 0 );

	/* Move info across to struct as we want it */
	strncpy( dirInfo.d_name, diskDirInfo->d_name, NAMEINDIR_LEN );
	dirInfo.d_name[ NAMEINDIR_LEN ] = '\0';
	stat( dirInfo.d_name, &dirInfo.d_stat );

	return( &dirInfo );
	}

/* End opendir() functions */

void closedir( DIR *dirPtr )
	{
	hclose( dirPtr->dd_fd );
	hfree( dirPtr );
	}
#endif /* 0 */

/* Find the first/next file in a directory */

BOOLEAN findFirst( const char *pathName, const ATTR fileAttr, FILEINFO *fileInfo )
	{
	char dirPath[ MAX_PATH ];
	int pos;

	/* Start at first entry in this directory */
	fileInfo->matchAttr = fileAttr;

	strcpy( dirPath, pathName );
	if( !*dirPath )
		{
		/* No pathname, search current directory */
		*dirPath = '.' ;
		dirPath[ 1 ] = '\0';
		}

	/* Check whether we want to match all files in a directory or one
	   particular file */
	if( dirPath[ strlen( dirPath ) - 1 ] == '.' )
		{
		/* Try and open directory */
		if( ( fileInfo->dirBuf = opendir( dirPath ) ) == NULL )
			return( FALSE );
		dirPath[ strlen( dirPath ) - 1 ] = '\0';	/* Zap '.' */
		strcpy( fileInfo->dName, dirPath );

		/* Pull file information out of directory */
		return( findNext( fileInfo ) );
		}
	else
		{
		/* Handling one particular file, just stat it */
		fileInfo->dirBuf=NULL;
		for( pos = strlen( dirPath ) - 1; pos != 0 && dirPath[ pos ] != SLASH; \
			 pos-- );
		strcpy( fileInfo->fName, dirPath + pos );
		if( stat( dirPath, &fileInfo->statInfo ) != ERROR )
			{
			fileInfo->fTime = fileInfo->statInfo.st_mtime;
			fileInfo->fSize = fileInfo->statInfo.st_size;
			return TRUE;
			}
		else
			return FALSE;
		}
	}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
	struct dirent *dirinfo;
	char dirPath[ MAX_PATH ];

	do
		{
		/* Grovel through the directory skipping deleted and non-matching files */
		if( ( dirinfo = readdir( fileInfo->dirBuf ) ) == NULL )
			return( FALSE );

		/* Fill out fileInfo fields.  Some systems may have the stat info
		   available after the readdir() call, but we have to assume the
		   worst for the portable version */
		strncpy( fileInfo->fName, dirinfo->d_name, MAX_FILENAME );
		fileInfo->fName[ MAX_FILENAME ] = 0;
		if( strlen( fileInfo->dName ) + strlen( fileInfo->fName ) > MAX_PATH )
			error( PATH_s__TOO_LONG, dirPath );
		strcpy( dirPath, fileInfo->dName );			/* Copy dirpath */
		strcat( dirPath, fileInfo->fName );			/* Add filename */
		stat( dirPath, &fileInfo->statInfo );
		fileInfo->fTime = fileInfo->statInfo.st_mtime;
		fileInfo->fSize = fileInfo->statInfo.st_size;
		}
	/* Sometimes we only want to match files, not directories */
	while( ( fileInfo->matchAttr == ALLFILES ) ? \
		   ( ( fileInfo->statInfo.st_mode & S_IFMT ) == S_IFDIR ) : 0 );

	return( TRUE );
	}

/* End findFirst/Next() for this directory */

void findEnd( const FILEINFO *fileInfo )
	{
	if( fileInfo->dirBuf != NULL )
		closedir( fileInfo->dirBuf );
	}

/* Get the screen size */

#define DEFAULT_ROWS		24
#define DEFAULT_COLS		80

void getScreenSize( void )
	{
	int charlen = 8, channel;
	struct {
		   short dummy1, cols;
		   char dummy2, dummy3, dummy4, rows;
		   } termchar;

	/* Get screen info from system */
	$DESCRIPTOR( devnam,"SYS$COMMAND" );
	sys$assign( &devnam, &channel, 0, 0 );
	sys$qiow( 0, channel, IO$_SENSEMODE, 0, 0, 0, &termchar, &charlen, 0, 0, 0, 0 );
	sys$dassgn( channel );

	screenHeight = ( termchar.rows > 0 ) ? ( int ) termchar.rows : DEFAULT_ROWS;
	screenWidth = ( termchar.cols > 0 ) ? ( int ) termchar.cols : DEFAULT_COLS;
	}

/* Get the country we're running in */

int getCountry( void )
	{
	puts( "Need to implement getCountry()" );
	return( 0 );	/* Default to US - see viewfile.c for country codes */
	}

/* The worst-case htruncate() - open a new file, copy everything across to
   the truncation point, and delete the old file.  This only works because
   htruncate is almost never called and because the drive access speed is
   assumed to be fast */

void moveData( const FD _inFD, const FD destFD, const long noBytes );

int htruncate( const FD theFD )
	{
	FD newArchiveFD;
	char newArchiveName[ MAX_PATH ];
	long truncatePoint;
	int retVal;

	puts( "Need to implement htruncate()" );

	/* Set up temporary filename to be the archive name with a ".TMP" suffix */
	strcpy( newArchiveName, archiveFileName );
	strcpy( newArchiveName, strlen( newArchiveName ) - 3, "TMP" );

	if( ( newArchiveFD = hopen( newArchiveName, O_RDWR ) ) == FILE_ERROR )
		error( INTERNAL_ERROR );
	setOutputFD( newArchiveFD );
	setInputFD( archiveFD );

	/* Use the moveData() routine to move all data up to the truncation point
	   to the new archive */
	truncatePoint = htell( archiveFD );
	hlseek( archiveFD, 0L, SEEK_SET );
	_outByteCount = 0;
	moveData( truncatePoint );

	/* Close and delete the old archive, and make the new archive the active
	   one */
	retVal = hclose( archiveFD );
	archiveFD = newArchiveFD;
	retVal |= hunlink( archiveFileName );
	return( retVal | hrename( newArchiveName, archiveName ) );
	}
#endif /* Various mutation-dependant truncate()'s */

/* Check whether two pathnames refer to the same file */

BOOLEAN isSameFile( const char *pathName1, const char *pathName2 )
	{
	struct stat statInfo1, statInfo2;

	puts( "Need to implement isSameFile()" );
	stat( pathName1, &statInfo1 );	/* This will work? but is nasty */
	stat( pathName2, &statInfo2 );
	return( statInfo1.st_ino == statInfo2.st_ino && \
			statInfo1.st_dev == statInfo2.st_dev );
	}

/****************************************************************************
*																			*
*								SYSTEM Functions							*
*																			*
****************************************************************************/

/* Lowercase a string */

void strlwr( char *string )
	{
	while( *string )
		{
		*string = tolower( *string );
		string++;
		}
	}

/* Move a block of memory, handles overlapping blocks */

void memmove( void *dest, void *src, int length )
	{
	int itmp = ( length + 7 ) >> 3;

	if( dest > src )
		{
		dest += length;
		src += length;
		switch( length & 3 )
			{
			case 0:	do {	*--dest = *--src;
			case 7:			*--dest = *--src;
			case 6:			*--dest = *--src;
			case 5:			*--dest = *--src;
			case 4:			*--dest = *--src;
			case 3:			*--dest = *--src;
			case 2:			*--dest = *--src;
			case 1:			*--dest = *--src;
			 		} while( --itmp > 0 );
			}
		}
	else
		{
		switch( length & 3 )
			{
			case 0:	do {	*dest++ = *src++;
			case 7:			*dest++ = *src++;
			case 6:			*dest++ = *src++;
			case 5:			*dest++ = *src++;
			case 4:			*dest++ = *src++;
			case 3:			*dest++ = *src++;
			case 2:			*dest++ = *src++;
			case 1:			*dest++ = *src++;
				 	} while( --itmp > 0 );
			}
		}
	/* This was added mainly to frighten you */
	}

/* Non-case-sensitive string compare */

int strnicmp( char *src, char *dest, int length )
	{
	char srcCh, destCh;

	while( length-- )
		{
		/* Need to be careful with toupper() side-effects */
		srcCh = *src++;
		srcCh = toupper( srcCh );
		destCh = *dest++;
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

