/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							Macintosh-Specific Routines						*
*							 MAC.C  Updated 10/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
* 			Copyright 1992  Peter C. Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* There are some special cases to do with handling the Mac filesystem
   interface.  Since all Mac system calls expect Pascal strings, we need to
   convert the strings first.  In addition, although the calls are pretty
   much identical to what the rest of the world uses, the parameter values
   and orders have been swapped around for no logical reason.

   All paths are passed around inside HPACK as SLASH-seperated path
   components for simplicity.  For the Mac, SLASH is a special non-character
   code which is used to delimit pathname components.  When an access to a
   directory is necessary, the individual components are extracted and the
   path parsed to return a dirID for the fileName component.  These dirID's
   should not be confused with HPACK dirID's (which work in the same manner
   but have different values)

   "You can always pick the DOS person in the Usenet Mac programming
	groups -- they're the ones who want to access the hardware directly".

	- Tim Hammett

   "You'll have to write the mappings yourself since Apple doesn't want you
	using a portable API ... OursIsBetterFasterSmallerAndImcompatibleAndWell
	LitigateToKeepItIncompatibleSinceWeDontWantYouWritingProgramsForOther
	Platforms, that's Apple".

	- Nigel Bree */

#include <MacHeaders>
#include <Packages.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <console.h>
#include "defs.h"
#include "flags.h"
#include "frontend.h"
#include "system.h"
#include "wildcard.h"
#include "hpackio.h"

/* The time difference between the Mac and Unix epochs, in seconds */

#define MAC_TIME_OFFSET		0x7C245F00L

/* The directory ID of the root and current directories */

#define MAC_ROOT_DIRID		2
#define MAC_CURRENT_DIRID	0

/* Whether we want OS calls to be executed asynchronously or not */

#define ASYNC_STATUS		FALSE

/* Define to translate from Mac to rest-of-the-world error codes */

#define xlatErr(errCode)	( ( errCode ) == OK ) ? OK : IO_ERROR

/* Default vRefNum and dirID */

static short defaultVRefArchive, defaultVRefFile;
static long defaultDirID;

/* The drive the archive disk is on (for multidisk archives) */

static int archiveDrive;

/* Pascalise a given filename */

static Str255 pFileName;

#define pDirName	pFileName			/* Alias for Pascal fileName */
#define pVolName	pFileName			/* Alias for Pascal volume name */

static void pascalise( const char *cString )
	{
	strcpy( ( char * ) pFileName, cString );
	CtoPstr( pFileName );
	}

/* Determine the default vRefNum and dirID */

void initMacInfo( void )
	{
#if 0
	WDPBRec wdInfo;

	/* Get vRefNum for current volume */
	if( GetVol( NULL, &defaultVRefArchive ) != OK )
		puts( "initMacInfo: Can't get current vRefNum" );

	/* Get dirID for WD on current volume */
	wdInfo.ioCompletion = NULL;	/* No callback routine */
	wdInfo.ioWDVRefNum = defaultVRefArchive;
	wdInfo.ioWDIndex = 0;		/* Use vRefNum for lookup */
	wdInfo.ioWDProcID = 0;		/* Index all WD's */
	if( PBGetWDInfo( &wdInfo, ASYNC_STATUS ) != OK )
		puts( "initMacInfo: Can't get dirID for WD" );
	defaultDirID = wdInfo.ioWDDirID;
#endif /* 0 */
	defaultVRefArchive = 0;
	defaultDirID = 0;
	}

/* Determine the vRefNum for a particular path.  This comprises several
   support routines since we have to sort out the volume an archive is on
   right at the start and then strip off the volume name component for
   multi-disk archive handling, since the volume ID will change with each
   new disk.  In addition, findFirst()/Next() bypasses this code and
   defaults to defaultVRefArchive, since by the time it's called the
   volume name component has been stripped */

static BOOLEAN isArchiveSearchPath( const char *path )
	{
	int pathLen = strlen( path ), extLen = strlen( HPAK_MATCH_EXT );

	/* Check whether this is a path to an archive or a path to a normal file */
	return( ( pathLen > extLen && \
			!strcmp( path + pathLen - extLen, HPAK_MATCH_EXT ) ) ? TRUE : FALSE );
	}

static BOOLEAN isArchivePath( const char *path )
	{
	int pathLen = strlen( path ), extLen = strlen( HPAK_EXT );

	/* Check whether this is a path to an archive or a path to a normal file */
	return( ( pathLen > extLen && \
			!strcmp( path + pathLen - extLen, HPAK_EXT ) ) ? TRUE : FALSE );
	}

int stripVolumeName( char *path )
	{
	int i;
	
	/* Overwrite the volume name at the start of the string if there is one.
	   This is necessary since for multidisk archives the volume name may
	   change for each disk */
	for( i = 0; path[ i ] && path[ i ] != ':'; i++ );
	if( path[ i ] )
		{
		/* Note the fencepost error in the length arg so we move the '\0' as well */
		memmove( path, path + i + 1, strlen( path + i ) );
		return( i + 1 );
		}

	/* No volume name */
	return( 0 );
	}

int currentDrive;

static short getVRefNum( const char *path )
	{
	char pVolumeName[ MAX_FILENAME ];
	short vRefNum = defaultVRefArchive;
	HVolumeParam volumePB;
	Str255 volName;
	OSErr status;
	long dirID;
	int i;

	/* Extract the volume name from the start of the string */
	for( i = 0; path[ i ] && path[ i ] != ':'; i++ );
	if( !path[ i ] )
		{
		/* No volume name, return default vRefNum.  We can have two different
		   types of vRefNums, one for the archive path and one for the file
		   path (set by the basePath option), so we have to check whether
		   we're handling an archive or a normal file */
		if( isArchivePath( path ) || isArchiveSearchPath( path ) )
			return( defaultVRefArchive );
		else
			return( defaultVRefFile );
		}

	/* Set up the fields in the parameter block */
	volumePB.ioCompletion = NULL;	/* No callback routine */
	volumePB.ioNamePtr = volName;
	volumePB.ioVolIndex = 0;		/* Index through entries */

	/* Search through all mounted volumes looking for a matching volume name */
	do
		{
		volumePB.ioVRefNum = 0;		/* Use ioVolIndex field */
		volumePB.ioVolIndex++;		/* Move to next field */
		status = PBHGetVInfo( &volumePB, ASYNC_STATUS );
		}
	while( ( status != nsvErr ) && ( strnicmp( path, ( char * ) volName + 1, ( int ) *volName ) ) );
	currentDrive = volumePB.ioVDrvInfo;	/* Remember what drive we're on */

	return( ( status == OK ) ? volumePB.ioVRefNum : \
			( isArchivePath( path ) ) ? defaultVRefArchive : defaultVRefFile );
	}

void setVolumeRef( char *path, const BOOLEAN isArchivePath )
	{
	short vRefNum = getVRefNum( path );
	int i;

	if( isArchivePath )
		{
		/* Set vRefNum from archive path */
		if( ( defaultVRefArchive = vRefNum ) == 0 )
			/* If we're using a default directory, use a default drive as well */
			archiveDrive = 0;
		else
			/* Remember what drive we're on */
			archiveDrive = currentDrive;
		}
	else
		/* Set vRefNum from basePath */
		defaultVRefFile = vRefNum;
	}

/* Extract the filename component out of a pathname */

static char *getFilenameComponent( const char *pathName )
	{
	char *lastSlashPtr, *pathPtr = ( char * ) pathName;
	int i;

	/* Skip the volume name if there is one */
	for( i = 0; pathName[ i ] && pathName[ i ] != ':'; i++ );
	if( pathName[ i ] == ':' )
		pathPtr += i + 1;	/* Point past volume name */

	/* Now find the last directory seperator */
	for( lastSlashPtr = pathPtr + strlen( pathPtr ); \
		 lastSlashPtr >= pathPtr && *lastSlashPtr != SLASH; \
		 lastSlashPtr-- );
	lastSlashPtr++;		/* Fix fencepost error */

	return( lastSlashPtr );
	}

/* Determine the dirID for a particular path */

static long getDirID( short vRefNum, const char *path )
	{
	long dirID = MAC_CURRENT_DIRID;
	int pathLen = strlen( path ), startIndex = 0, endIndex, dirNameLen, i;
	char *pathPtr = ( char * ) path;
	Str255 pDirName;
	CInfoPBRec pb;

	/* Skip the volume name if there is one */
	for( i = 0; pathPtr[ i ] && pathPtr[ i ] != ':'; i++ );
	if( pathPtr[ i ] == ':' )
		pathPtr += i + 1;	/* Point past volume name */
	if( *pathPtr == SLASH )
		pathPtr++;			/* Skip slash after volume name */

	/* Find the start of the filename component, return if there is no pathname */
	if( !( pathLen = ( getFilenameComponent( pathPtr ) - pathPtr ) ) )
		/* Default to cwd for now */
		return( defaultDirID );

	/* Walk down the path getting dirID's for each component as we go */
	endIndex = startIndex;
	pb.dirInfo.ioCompletion = NULL;		/* No callback routine */
	while( endIndex != pathLen )
		{
		/* Extract a directory name out of the pathname */
		while( endIndex < pathLen && pathPtr[ endIndex ] != SLASH )
			endIndex++;
		dirNameLen = endIndex - startIndex;
		strncpy( ( char * ) pDirName + 1, pathPtr + startIndex, dirNameLen );
		pDirName[ 0 ] = dirNameLen;

		/* Set up relevant fields in the pb struct */
		pb.dirInfo.ioNamePtr = pDirName;
		pb.dirInfo.ioVRefNum = vRefNum;
		pb.dirInfo.ioFDirIndex = 0;		/* Access directory by name */
		pb.dirInfo.ioDrDirID = dirID;

		if( PBGetCatInfo( &pb, ASYNC_STATUS ) != OK )
			printf( "getDirID: Can't stat %s\n", pathPtr + startIndex );

		dirID = pb.dirInfo.ioDrDirID;
		startIndex = ++endIndex;	/* Skip SLASH */
		}

	/* Default to cwd for now */
	return( dirID );
	}

/* Stat a file/directory.  It would be nice to use PBHGetFInfo() for this
   but that only works for files, not directories.  In addition the
   companion PBHSetFInfo() can't set backup times */

int stat( const char *fileName, CInfoPBRec *pbPtr )
	{
	short vRefNum = getVRefNum( fileName );
	long dirID = getDirID( vRefNum, fileName );
	OSErr status;

	pascalise( getFilenameComponent( fileName ) );

	/* Set up file parameter block */
	pbPtr->hFileInfo.ioCompletion = NULL;	/* No callback */
	pbPtr->hFileInfo.ioVRefNum = vRefNum;
	pbPtr->hFileInfo.ioFDirIndex = 0;		/* Get info on given filename */
	pbPtr->hFileInfo.ioFVersNum = 0;		/* Ignore version no */
	pbPtr->hFileInfo.ioDirID = dirID;
	pbPtr->hFileInfo.ioNamePtr = pFileName;

	/* Get the file info and reset the dirID field to its proper value */
	status = PBGetCatInfo( pbPtr, ASYNC_STATUS );
	pbPtr->hFileInfo.ioDirID = dirID;
	return( xlatErr( status ) );
	}

#ifdef NO_RIFFRAFF

/* Keep out all the LC owners ('020 code already keeps the Classics out) */

void noRiffRaff( void )
	{
	double x = sin( 0.671279 );
	}
#endif /* NO_RIFFRAFF */

/****************************************************************************
*																			*
*								HPACKIO Functions							*
*																			*
****************************************************************************/

/* Create a new file.  It would be nice to be able to specify at this point
   what type of file to create, but we may not know until much later on after
   the type has been translated from a foreign OS */

FD hcreat( const char *fileName, const int attr )
	{
	short vRefNum = getVRefNum( fileName );
	long dirID = getDirID( vRefNum, fileName );
	FD fRefNum;
	OSErr status;

	pascalise( getFilenameComponent( fileName ) );

	if( ( status = HCreate( vRefNum, dirID, pFileName, 'TEXT', 'TEXT' ) ) == dupFNErr )
		/* If the file already exists, delete it (yes I know that creat() by
		   definition creates a new file, but the Mac version doesn't), then re-
		   re-create it */
		if( ( status = HDelete( vRefNum, dirID, pFileName ) ) == OK )
			status = HCreate( vRefNum, dirID, pFileName, 'TEXT', 'TEXT' );
	if( status == OK )
		status = HOpen( vRefNum, dirID, pFileName, ( char ) attr, &fRefNum );

	return( ( status == OK ) ? fRefNum : IO_ERROR );
	}

/* Open an existing file */

FD hopen( const char *fileName, const int mode )
	{
	short vRefNum = getVRefNum( fileName );
	long dirID = getDirID( vRefNum, fileName );
	FD fRefNum;
	OSErr status;

	pascalise( getFilenameComponent( fileName ) );
	status = HOpen( vRefNum, dirID, pFileName, ( char ) mode, &fRefNum );

	return( ( status == OK ) ? fRefNum : IO_ERROR );
	}

/* Close a file */

int hclose( const FD theFile )
	{
	return( xlatErr( FSClose( theFile ) ) );
	}

/* Read data from a file.  Note that hitting EOF isn't an error since we always
   try and read as much as we can into a buffer */

int hread( const FD theFile, void *buffer, const unsigned int count )
	{
	long readCount = ( long ) count;
	OSErr status;

	if( ( status = FSRead( theFile, &readCount, buffer ) ) != OK && status != eofErr )
		fileError();

	return( ( int ) readCount );
	}

/* Write data to a file.  Note that running out of disk space isn't an error
   since we may be doing a multidisk archive */

int hwrite( const FD theFile, void *buffer, const unsigned int count )
	{
	long writeCount = ( long ) count;
	Str255 volName;
	int vRefNum;
	long freeBytes;
	OSErr status;

	if( ( status = FSWrite( theFile, &writeCount, buffer ) ) != OK && status != dskFulErr )
		fileError();

	/* If we've run out of room, find out how much there really is an write that much */
	if( status == dskFulErr && \
		GetVInfo( archiveDrive, &volName, &vRefNum, &freeBytes ) == OK )
		{
		/* Retry the write.  Getting disk full is now an error */
		if( ( status = FSWrite( theFile, &writeCount, buffer ) ) != OK )
			fileError();
		}

	return( ( int ) writeCount );
	}

/* Seek to a position in a file */

long hlseek( const FD theFile, const long offset, const int origin )
	{
	/* The Mac has SEEK_SET = 1, SEEK_CUR = 3, SEEK_END = 2 */
	static short originTbl[] = { 1, 3, 2 };

	if( SetFPos( theFile, originTbl[ origin ], offset ) != OK )
		return( -1L );
	else
		return( htell( theFile ) );
	}

/* Return the current position in a file */

long htell( const FD theFile )
	{
	long filePos;

	if( GetFPos( theFile, &filePos ) == OK )
		return( filePos );
	else
		return( -1L );
	}

/* Truncate a file at the current position */

int htruncate( const FD theFile )
	{
	return( xlatErr( SetEOF( theFile, htell( theFile ) ) ) );
	}

/* Remove a file */

int hunlink( const char *fileName )
	{
	short vRefNum = getVRefNum( fileName );
	long dirID = getDirID( vRefNum, fileName );

	pascalise( getFilenameComponent( fileName ) );
	return( xlatErr( HDelete( vRefNum, dirID, pFileName ) ) );
	}

/* Create a directory */

int hmkdir( const char *dirName, const int attr )
	{
	short vRefNum = getVRefNum( dirName );
	long dirID = getDirID( vRefNum, dirName );
	long newDirID;

	pascalise( getFilenameComponent( dirName ) );
	return( xlatErr( DirCreate( vRefNum, dirID, pDirName, &newDirID ) ) );
	}

/* Rename a file */

int hrename( const char *oldName, const char *newName )
	{
	short vRefNum = getVRefNum( oldName );
	long dirID = getDirID( vRefNum, oldName );
	char pNewName[ MAX_FILENAME + 1 ];	/* +1 for Pascal string */

	pascalise( getFilenameComponent( newName ) );
	memcpy( pNewName, pFileName, MAX_FILENAME );
	pascalise( getFilenameComponent( oldName ) );
	return( xlatErr( HRename( vRefNum, dirID, pFileName, pNewName ) ) );
	}

/* Set/change a file/dir's attributes */

int hchmod( const char *fileName, const int attr )
	{
	CInfoPBRec filePB;
	OSErr status;

	/* Get the file info, change the attribute field, then write it back out */
	if( ( status = stat( fileName, &filePB ) ) != ERROR )
		{
		filePB.hFileInfo.ioFlAttrib = attr;
		status = PBSetCatInfo( &filePB, ASYNC_STATUS );
		}

	return( xlatErr( status ) );
	}

/****************************************************************************
*																			*
*								HPACKLIB Functions							*
*																			*
****************************************************************************/

/* Get an input character, no echo */

int hgetch( void )
	{
	int ch;

	/* Turn off echo */
	csetmode( C_RAW, stdin );

	/* Skip any garbage and get input char */
	while( ( ch = getchar() ) == EOF );

	/* Turn echo back on */
	csetmode( C_ECHO, stdin );

	return( ch );
	}

/* Allocate/reallocate/free memory (the library malloc() is somewhat flaky) */

void *hmalloc( const unsigned long size )
	{
	return( ( void * ) NewPtr( size ) );
	}

void hfree( void *buffer )
	{
	DisposPtr( ( Ptr ) buffer );
	}

void *hrealloc( void *buffer, const unsigned long size )
	{
	DisposPtr( ( Ptr ) buffer );
	return( ( void * ) NewPtr( size ) );
	}

/****************************************************************************
*																			*
*								SYSTEM Functions							*
*																			*
****************************************************************************/

/* Set a file's timestamp */

void setFileTime( const char *fileName, const LONG fileTime )
	{
	CInfoPBRec filePB;

	/* Get the file info, change the time fields, then write it back out */
	if( stat( fileName, &filePB ) != ERROR )
		{
		filePB.hFileInfo.ioFlCrDat = filePB.hFileInfo.ioFlMdDat = \
									 fileTime + MAC_TIME_OFFSET;
		PBSetCatInfo( &filePB, ASYNC_STATUS );
		}
	}

void setCreationTime( const char *fileName, const LONG creationTime )
	{
	CInfoPBRec filePB;

	/* Get the file info, change the creation time field, then write it back out */
	if( stat( fileName, &filePB ) != ERROR )
		{
		filePB.hFileInfo.ioFlCrDat = creationTime + MAC_TIME_OFFSET;
		PBHSetFInfo( &filePB, ASYNC_STATUS );
		}
	}

void setBackupTime( const char *fileName, const LONG creationTime )
	{
	CInfoPBRec filePB;

	/* Get the file info, change the backup time field, then write it back out */
	if( stat( fileName, &filePB ) != ERROR )
		{
		filePB.hFileInfo.ioFlBkDat = creationTime + MAC_TIME_OFFSET;
		PBHSetFInfo( &filePB, ASYNC_STATUS );
		}
	}

/* Set a file's version number */

void setVersionNumber( const char *fileName, const BYTE versionNumber )
	{
	CInfoPBRec filePB;

	/* Get the file info, change the version number field, then write it back out */
	if( stat( fileName, &filePB ) != ERROR )
		{
		filePB.hFileInfo.ioFVersNum = versionNumber;
		PBHSetFInfo( &filePB, ASYNC_STATUS );
		}
	}

/* Set a file's type */

void setFileType( const char *fileName, const LONG type, const LONG creator )
	{
	CInfoPBRec filePB;

	/* Get the file info, change the type/creator fields, then write it back out */
	if( stat( fileName, &filePB ) != ERROR )
		{
		filePB.hFileInfo.ioFlFndrInfo.fdType = type;
		filePB.hFileInfo.ioFlFndrInfo.fdCreator = creator;
		PBHSetFInfo( &filePB, ASYNC_STATUS );
		}
	}

/* Set any extra attribute/type/access information needed by an archive */

void setExtraInfo( const char *fileName )
	{
	switch( cryptFlags & ( CRYPT_PKE_ALL | CRYPT_CKE_ALL | CRYPT_SIGN_ALL ) )
		{
		case CRYPT_PKE_ALL:
		case CRYPT_CKE_ALL:
			/* Encrypted archive */
			setFileType( fileName, 'HPKC', 'HPAK' );
			break;

		case CRYPT_SIGN_ALL:
			/* Secured archive */
			setFileType( fileName, 'HPKS', 'HPAK' );
			break;

		case CRYPT_PKE_ALL | CRYPT_SIGN_ALL:
		case CRYPT_CKE_ALL | CRYPT_SIGN_ALL:
			/* Encrypted + secured archive */
			setFileType( fileName, 'HPKB', 'HPAK' );
			break;

		default:
			/* Plain archive */
			setFileType( fileName, 'HPAK', 'HPAK' );
		}
	}

#ifndef GUI

/* The following two functions aren't needed for the GUI version */

#define DEFAULT_ROWS		24
#define DEFAULT_COLS		80

void getScreenSize( void )
	{
	/* Not really appropriate so just default to 24x80 */
	screenWidth = DEFAULT_COLS;
	screenHeight = DEFAULT_ROWS;
	}

int getCountry( void )
	{
	Intl0Hndl panHandle;
	BYTE dateFormat;

	/* Extract the date format from the Intl 0 resource info */
	panHandle = ( Intl0Hndl ) IUGetIntl( 0 );
	dateFormat = ( *panHandle )->dateOrder;

	return( ( dateFormat < 3 ) ? dateFormat : 0 );
	}
#endif /* GUI */

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

/* Copy extra info (type, icons, etc) from one file to another */

void copyExtraInfo( const char *srcFileName, const char *destFileName )
	{
	/* Copy extra info from srcFD to destFD... */
	}

/* Find the first/next file in a directory.  Note we have to take special
   care if we're trying to match archives since the volume name component
   has been stripped and we don't want to default to the data file volume */

static BOOLEAN getFileInfo( CInfoPBRec *pbPtr, FILEINFO *fileInfo )
	{
	int filenameLength;
	Str255 ioName;
	FInfo *finderInfo = &pbPtr->hFileInfo.ioFlFndrInfo;
	DInfo *dirInfo = &pbPtr->dirInfo.ioDrUsrWds;
	BOOLEAN matched;
	OSErr status;

	/* Set up PBRec fields */
	pbPtr->hFileInfo.ioCompletion = NULL;		/* No callback */
	pbPtr->hFileInfo.ioVRefNum = fileInfo->vRefNum;
	if( pbPtr->hFileInfo.ioNamePtr == NULL )
		pbPtr->hFileInfo.ioNamePtr = ioName;	/* Allocate space to store name */

	do
		{
		/* Get the information on the directory entry */
		pbPtr->hFileInfo.ioDirID = fileInfo->dirID;	/* Needs to be reset each time */
		matched = FALSE;
		status = IO_ERROR;
		while( status != OK )
			{
			if( ( status = PBGetCatInfo( pbPtr, ASYNC_STATUS ) ) == fnfErr )
				/* We've run out of files, return */
				return( FALSE );

			/* Move to the next file */
			pbPtr->hFileInfo.ioFDirIndex++;
			}

		/* Extract various useful pieces of information */
		fileInfo->isDirectory = ( pbPtr->hFileInfo.ioFlAttrib & 0x10 ) ? TRUE : FALSE;
		filenameLength = pbPtr->hFileInfo.ioNamePtr[ 0 ];

		/* Perform an explicit match for the .HPK extension if necessary */
		if( fileInfo->matchArchive )
			{
			/* Look for an extension */
			if( ( filenameLength > strlen( HPAK_EXT ) ) && \
				( !stricmp( ( char * ) pbPtr->hFileInfo.ioNamePtr + \
							filenameLength - strlen( HPAK_EXT ), HPAK_EXT ) ) )
				matched = !fileInfo->isDirectory;	/* Don't match directories */
				break;
			}
		else
			/* Sometimes we only want to match files, not directories */
			matched = ( fileInfo->matchAttr == ALLFILES ) ? \
					  !fileInfo->isDirectory : TRUE;
		}
	while( !matched );

	/* Copy the info from PBRec into the FILEINFO struct */
	strncpy( fileInfo->fName, ( char * ) pbPtr->hFileInfo.ioNamePtr + 1, filenameLength );
	fileInfo->fName[ filenameLength ] = '\0';
	fileInfo->fTime = pbPtr->hFileInfo.ioFlMdDat - MAC_TIME_OFFSET;
	fileInfo->createTime = pbPtr->hFileInfo.ioFlCrDat - MAC_TIME_OFFSET;
	fileInfo->backupTime = ( pbPtr->hFileInfo.ioFlBkDat ) ? \
						   pbPtr->hFileInfo.ioFlBkDat - MAC_TIME_OFFSET : 0L;
	fileInfo->versionNumber = pbPtr->hFileInfo.ioFVersNum;
	if( fileInfo->isDirectory )
		{
		fileInfo->fAttr = finderInfo->fdFlags;
		fileInfo->type = fileInfo->creator = 0L;
		fileInfo->fSize = fileInfo->resSize = 0L;
		}
	else
		{
		fileInfo->fAttr = finderInfo->fdFlags;
		fileInfo->type = finderInfo->fdType;
		fileInfo->creator = finderInfo->fdCreator;
		fileInfo->resSize = pbPtr->hFileInfo.ioFlRLgLen;
		fileInfo->fSize = pbPtr->hFileInfo.ioFlLgLen;
		}
	fileInfo->entryNo = pbPtr->hFileInfo.ioFDirIndex;

	return( TRUE );
	}

BOOLEAN findFirst( const char *pathName, const ATTR fileAttr, FILEINFO *fileInfo )
	{
	char *fileName = getFilenameComponent( pathName );
	short vRefNum = ( fileInfo->matchArchive = !strcmp( fileName, MATCH_ARCHIVE ) ) ? \
					defaultVRefArchive : getVRefNum( pathName );
	long dirID = getDirID( vRefNum, pathName );
	CInfoPBRec pb;

	/* Set up vRefNum, dirID, and file match type fields */
	fileInfo->vRefNum = vRefNum;
	fileInfo->dirID = dirID;
	fileInfo->matchAttr = fileAttr;

	/* Check if we want to match a particular file or a whole directory */
	if( strcmp( fileName, MATCH_ALL ) && !fileInfo->matchArchive )
		{
		/* Set up PBRec for individual file match */
		pascalise( fileName );
		pb.hFileInfo.ioNamePtr = pFileName;
		pb.hFileInfo.ioFDirIndex = 0;	/* Match individual file */
		}
	else
		{
		/* Set up PBRec for whole directory match */
		pb.hFileInfo.ioNamePtr = NULL;
		pb.hFileInfo.ioFDirIndex = 1;	/* Match first directory entry */
		}

	return( getFileInfo( &pb, fileInfo ) );
	}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
	CInfoPBRec pb;

	/* Set up PBRec fields */
	pb.hFileInfo.ioNamePtr = NULL;
	pb.hFileInfo.ioFDirIndex = fileInfo->entryNo;

	return( getFileInfo( &pb, fileInfo ) );
	}

/* Functions for opening and closing a file's resource fork */

FD openResourceFork( const char *fileName, const int mode )
	{
	short vRefNum = getVRefNum( fileName );
	long dirID = getDirID( vRefNum, fileName );
	FD resourceForkFD;

	pascalise( getFilenameComponent( fileName ) );
	return( ( HOpenRF( vRefNum, dirID, pFileName, ( char ) mode, \
					   &resourceForkFD ) == OK ) ? resourceForkFD : IO_ERROR );
	}

void closeResourceFork( const FD resourceForkFD )
	{
	FSClose( resourceForkFD );
	}

/* Grudgingly yield a smidgeon of CPU time for other processes every now and then */

#define FGND_YIELD_TIME	1		/* Yield 1/60 seconds */
#define BGND_YIELD_TIME	5		/* Yield 5/60 seconds */

void yieldTimeSlice( void )
	{
	static long yieldTime = 1;
	EventRecord	theEvent;

	if( WaitNextEvent( 0xFFFF, &theEvent, yieldTime, NULL ) )
		if( ( theEvent.what == app4Evt ) && ( theEvent.message & 0x01000000 ) )
			yieldTime = ( theEvent.message & 0x00000001 ) ? \
						FGND_YIELD_TIME : BGND_YIELD_TIME;
	}

/* Eject a disk/wait for a new disk to be inserted */

void bleuurgghh( void )
	{
	Eject( NULL, defaultVRefArchive );
	}

void waitForDisk( void )
	{
	EventRecord	theEvent;
	Point thePoint;
	long freeBytes;

	while( TRUE )
		if( WaitNextEvent( 0xFFFF, &theEvent, 0xFFFFFFFFL, NULL ) && ( theEvent.what == diskEvt ) )
			if( HiWord( theEvent.message ) )
				/* Some sort of error, give the user the option of ejecting
				   or initializing the disk.  If we're running in the BG
				   the FG program will receive the message, otherwise we have
				   to process it */
				{
				SetPt( &thePoint, -1, -1 );	/* Centre the dialog on the screen */
				if( DIBadMount( thePoint, theEvent.message ) == OK )
					break;
				}
			else
				/* Disk was mounted OK, continue */
				break;

	/* We've mounted a new disk, get its volume name and vRefNum */
	GetVInfo( ( int ) LoWord( theEvent.message ), pVolName, \
			  &defaultVRefArchive, &freeBytes );
	}

/* Get any randomish information the system can give us (not terribly good,
   but every extra bit helps - if it's there we may as well use it) */

void getRandomInfo( BYTE *buffer, int bufSize )
	{
	mputLong( buffer, clock() );
	mputLong( buffer + sizeof( LONG ), time( NULL ) );
	mputLong( buffer + ( 2 * sizeof( LONG ) ), FreeMem() );
	}

/* Lowercase a string */

void strlwr( const char *string )
	{
	char *strPtr = ( char * ) string;
	
	while( *strPtr )
		{
		*strPtr = tolower( *strPtr );
		strPtr++;
		}
	}

/* Compare string, non-case-sensitive */

int strnicmp( const char *src, const char *dest, int length )
	{
	char *srcPtr = ( char * ) src, *destPtr = ( char * ) dest;
	char srcCh, destCh;

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
