/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								Main Archiver Code							*
*							FRONTEND.C  Updated 28/08/92					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "choice.h"
#include "error.h"
#include "filesys.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#include "tags.h"
#include "wildcard.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
  #include "store\store.h"
#elif defined( __MAC__ )
  #include "fastio.h"
  #include "hpackio.h"
  #include "store.h"
  #include "language.h"
#else
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
  #include "store/store.h"
#endif /* System-specific include directives */

/* Prototypes for functions in VIEWFILE.C */

void listFiles( void );

/* Prototypes for functions in ARCHIVE.C */

void addData( const char *filePath, const FILEINFO *fileInfoPtr, const WORD dirIndex, const FD dataFD );
void retrieveData( FILEHDRLIST *fileInfo );

BOOLEAN confirmSkip( const char *str1, const char *str2, const BOOLEAN str1fileName );

void initBadFileInfo( void );
void processBadFileInfo( void );

#ifdef __AMIGA__

/* Prototypes for functions defined in AMIGA.C */

LONG storeDiskObject( const char *pathName, const FD outFD );
void storeComment( const FD outFD );
#endif /* __AMIGA__ */
#ifdef __OS2__

/* Prototypes for functions in OS2.C */

LONG storeEAinfo( const BOOLEAN isFile, const char *pathName, const LONG eaSize, const FD archiveFD );

#endif /* __OS2__ */

/* The following are defined in ARCHIVE.C */

extern BOOLEAN overWriteEntry;	/* Whether we overwrite the existing file
								   entry or add a new one in addFileHeader()
								   - used by FRESHEN, REPLACE, UPDATE */

/* The following are defined in ARCDIRIO.C */

extern int cryptFileDataLength;	/* The size of the encryption header at the
								   start of the compressed data */

/* The extensions used for HPACK archives, and used to match HPACK archives */

#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  char HPAK_EXT[] = ".HPK";
#elif defined( __ARC__ )
  char HPAK_EXT[] = "";
#else
  char HPAK_EXT[] = ".hpk";
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ */
#ifdef __ARC__
  char HPAK_MATCH_EXT[] = "";
#else
  char HPAK_MATCH_EXT[] = ".\04Hh\06\04Pp\06\04Kk\06";	/* Compiled ".[Hh][Pp][Kk]" */
#endif /* __ARC__ */

/* The distance in bytes to the next piece of data to handle */

long skipDist;

/* The name of the archive currently being processed */

char archiveFileName[ MAX_PATH ];

/* Some general vars used throughout HPACK */

char choice;				/* The command to the archiver */
WORD flags = 0;				/* Various flags set by the user */
WORD dirFlags = 0;			/* Directory-handling flags set by the user */
WORD overwriteFlags = 0;	/* Overwrite-on-extract flags */
WORD viewFlags = 0;			/* Options for the View command */
WORD xlateFlags = 0;		/* Options for output translation */
WORD cryptFlags = 0;		/* Options for encryption/security */
WORD multipartFlags = 0;	/* Options for multipart read/write control */
WORD sysSpecFlags = 0;		/* System-specific options */
WORD commentType = TYPE_NORMAL;	/* The type of the archive comment */
BOOLEAN archiveChanged = FALSE;	/* Whether the archive has been changed */
BOOLEAN firstFile = TRUE;	/* Whether this is first file we're processing */
char basePath[ MAX_PATH ];	/* The output directory given by the -b option */
int basePathLen = 0;		/* The length of the basePath */
int screenHeight, screenWidth;	/* The screen size */
char signerID[ 256 ];		/* userID of archive signer */
char mainUserID[ 256 ];		/* userID of main PKE destination (needs to be init'd) */
char secUserID[ 256 ];		/* userID of secondary PKE destination */

/****************************************************************************
*																			*
*						Support Routines for the Main Code					*
*																			*
****************************************************************************/

/* Keep a record of every archive processed and don't do the same archive
   again.  This detail is necessary due to a combination of local OS file
   handling and HPACK file handling.  In the cases where HPACK works on
   a second file which it then renames as the original after deleting the
   original, the findNext() call may then match this newly-created archive
   and process it again, which results in either a "Nothing to do" error
   (for DELETE, FRESHEN, and UPDATE), or an endless loop as we keep matching
   the new archives (for REPLACE) */

typedef struct FL {
				  char *name;			/* This archive's name */
				  struct FL *next;		/* Pointer to next record */
				  } FILEDATA;

static FILEDATA *archiveNameListHead = NULL;

BOOLEAN addArchiveName( char *archiveName )
	{
	FILEDATA *namePtr, *prevPtr, *newRecord;

	for( namePtr = archiveNameListHead; namePtr != NULL;
				   prevPtr = namePtr, namePtr = namePtr->next )
		/* Check whether we've already processed an archive of this name */
		if( !strcmp( archiveName, namePtr->name ) )
			return( TRUE );

	/* We have reached the end of the list without a match, so we add this
	   archive name to the list */
	if( ( newRecord = ( FILEDATA * ) hmalloc( sizeof( FILEDATA ) ) ) == NULL ||
		( newRecord->name = ( char * ) hmalloc( strlen( archiveName ) + 1 ) ) == NULL )
		error( OUT_OF_MEMORY );
	strcpy( newRecord->name, archiveName );
	newRecord->next = NULL;
	if( archiveNameListHead == NULL )
		archiveNameListHead = newRecord;
	else
		prevPtr->next = newRecord;
	return( FALSE );
	}

#if !defined( __MSDOS16__ ) || defined( GUI )

/* Free up the memory used by the list of archive names */

void freeArchiveNames( void )
	{
	FILEDATA *namePtr = archiveNameListHead, *headerPtr;

	/* A simpler implementation of the following would be:
	   for( namePtr = archiveNameListHead; namePtr != NULL; namePtr = namePtr->next )
		   hfree( namePtr );
	   However this will fail on some systems since we will be trying to
	   read namePtr->next out of a record we have just free()'d */
	while( namePtr != NULL )
		{
		headerPtr = namePtr;
		namePtr = namePtr->next;
		hfree( headerPtr->name );
		hfree( headerPtr );
		}
	}
#endif /* !__MSDOS16__ || GUI */

/* Keep a record of every file added to an archive.  This is used by the
   move option, which is a two-pass operation: The first pass adds the files
   to the archive, the second pass goes through and deletes the added files */

static FILEDATA *filePathListHead = NULL, *prevPtr;

static void addFilePath( char *filePath )
	{
	FILEDATA *newRecord;

	/* Add the filepath to the list of filepaths */
	if( ( newRecord = ( FILEDATA * ) hmalloc( sizeof( FILEDATA ) ) ) == NULL ||
		( newRecord->name = ( char * ) hmalloc( strlen( filePath ) + 1 ) ) == NULL )
		error( OUT_OF_MEMORY );
	strcpy( newRecord->name, filePath );
	newRecord->next = NULL;
	if( filePathListHead == NULL )
		filePathListHead = newRecord;
	else
		prevPtr->next = newRecord;
	prevPtr = newRecord;
	}

/* Delete files and free up the memory used by the list of file paths */

void wipeFilePaths( void )
	{
	FILEDATA *namePtr = filePathListHead, *headerPtr;

	/* Step through the list of names freeing them */
	while( namePtr != NULL )
		{
		headerPtr = namePtr;
		namePtr = namePtr->next;

		/* Wipe the file */
		wipeFile( headerPtr->name );

		/* Free the memory */
		hfree( headerPtr->name );
		hfree( headerPtr );
		}
	}

/* Add a series of directories to an archive (used by processDir()) */

static void addDirPath( char *fullPath, char *internalPath, const int currPathLen, const WORD dirIndex )
	{
	FILEINFO fileInfo;
	int matchLen, count;
	char ch;

	/* We shouldn't get here if the DIR_NOCREATE flags is set to specify that
	   no directories should be created if they don't already exist */
	if( dirFlags & DIR_NOCREATE )
		{
		fullPath[ currPathLen ] = '\0';		/* Truncate to pure path */
		error( PATH_NOT_FOUND, internalPath );
		}

	setArchiveCwd( dirIndex );

	/* Grovel through all directories in the path, adding them to the
	   archive directory tree as we go */
	matchLen = strlen( getPath( dirIndex ) );
	while( matchLen < currPathLen )
		{
		/* Get the next directories name */
		for( count = matchLen + 1; internalPath[ count ] && internalPath[ count ] != SLASH;
			 count++ );
		ch = internalPath[ count ];
		internalPath[ count ] = '\0';

		/* Try and add the new directory */
		if( findFirst( fullPath, ALLFILES_DIRS, &fileInfo ) && isDirectory( fileInfo ) )
			{
#ifndef GUI
			hprintfs( MESG_ADDING_DIRECTORY_s, fileInfo.fName );
#endif /* !GUI */
			createDir( getArchiveCwd(), fileInfo.fName, fileInfo.fTime );
			if( flags & STORE_ATTR )
				{
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
				addDirData( TAG_MSDOS_ATTR, TAGFORMAT_STORED, LEN_MSDOS_ATTR, LEN_NONE );
				fputDirByte( fileInfo.fAttr );		/* Save attributes */
#elif defined( __AMIGA__ )
				addDirData( TAG_AMIGA_ATTR, TAGFORMAT_STORED, LEN_AMIGA_ATTR, LEN_NONE );
				fputDirByte( fileInfo.fAttr );		/* Save attributes */
				if( fileInfo.hasComment )
					storeComment( 0 );				/* Save comment */
				storeDiskObject( fullPath, 0 );
#elif defined( __ARC__ )
				addDirData( TAG_ARC_ATTR, TAGFORMAT_STORED, LEN_ARC_ATTR, LEN_NONE );
				fputDirByte( fileInfo.fAttr );		/* Save attributes */
#elif defined( __ATARI__ )
				addDirData( TAG_ATARI_ATTR, TAGFORMAT_STORED, LEN_ATARI_ATTR, LEN_NONE );
				fputDirByte( fileInfo.fAttr );		/* Save attributes */
#elif defined( __MAC__ )
				addDirData( TAG_MAC_ATTR, TAGFORMAT_STORED, LEN_MAC_ATTR, LEN_NONE );
				fputDirWord( fileInfo.fAttr );		/* Save attributes */
				addDirData( TAG_CREATION_TIME, TAGFORMAT_STORED, LEN_CREATION_TIME, LEN_NONE );
				fputDirLong( fileInfo.createTime );	/* Save creation time */
				if( fileInfo.backupTime )
					{
					/* Save backup time if there is one */
					addDirData( TAG_BACKUP_TIME, TAGFORMAT_STORED, LEN_BACKUP_TIME, LEN_NONE );
					fputDirLong( fileInfo.backupTime );
					}
				if( fileInfo.versionNumber )
					{
					/* Save version number if there is one */
					addDirData( TAG_VERSION_NUMBER, TAGFORMAT_STORED, LEN_VERSION_NUMBER, LEN_NONE );
					fputDirWord( ( WORD ) fileInfo.versionNumber );
					}
#elif defined( __OS2__ )
				if( fileInfo.aTime )
					{
					/* Save access time if there is one */
					addDirData( TAG_ACCESS_TIME, TAGFORMAT_STORED, LEN_ACCESS_TIME, LEN_NONE );
					fputDirLong( fileInfo.aTime );
					}
				if( fileInfo.cTime )
					{
					/* Save creation time if there is one */
					addDirData( TAG_CREATION_TIME, TAGFORMAT_STORED, LEN_CREATION_TIME, LEN_NONE );
					fputDirLong( fileInfo.cTime );
					}
				storeEAinfo( FALSE, fullPath, fileInfo.eaSize, archiveFD );
#elif defined( __UNIX__ )
				addDirData( TAG_UNIX_ATTR, TAGFORMAT_STORED, LEN_UNIX_ATTR, LEN_NONE );
				fputDirWord( fileInfo.statInfo.st_mode );	/* Save attributes */
				addDirData( TAG_ACCESS_TIME, TAGFORMAT_STORED, LEN_ACCESS_TIME, LEN_NONE );
				fputDirLong( fileInfo.statInfo.st_atime );	/* Save access time */
#endif /* Various OS-dependant attribute writes */
				}
			internalPath[ count ] = ch;
			matchLen = count;
			findEnd( &fileInfo );
			}
		else
			error( PATH_NOT_FOUND, internalPath );
		}
	}

/* Process one directories worth of files */

#define MAX_LEVELS		15		/* Max.directory depth we will recurse to */

static void processDir( FILENAMEINFO *fileNameList, const char *filePath,
						const int internalPathStart )
	{
	FILEINFO fileInfo, currentDir[ MAX_LEVELS ];
	FILENAMEINFO *fileNamePtr;
	int currentLevel = 0, i;
	int pathEnd[ MAX_LEVELS ];
	char fullPath[ MAX_PATH ], *internalPath, ch;
	int currPathLen = strlen( filePath ), internalPathLen, nameLen;
	BOOLEAN foundFile, skipFile, skipFind;
/*	BOOLEAN recurseFlags[ MAX_LEVELS ], forceRecurseFlags[ MAX_LEVELS ]; */
/*	BOOLEAN doRecurse = ( flags & RECURSE_SUBDIR ) ? TRUE : FALSE; */
/*	BOOLEAN recurseOnce = FALSE, forceRecurse = FALSE; */
	WORD dirIndex = ROOT_DIR;	/* Default dir for DIR_FLATTEN option */
	ATTR matchAttr = ( flags & STORE_ATTR ) ? ALLFILES_DIRS : FILES_DIRS;
	FD workFD, savedInFD;

	/* Perform a depth-first search of all directories below the current
	   one.  Note that there is little advantage to be gained from an
	   explicit match rather than MATCH_ALL since we will at some point need
	   to match MATCH_ALL anyway to find directories; this also lets us use
	   extended wildcards when doing the string comparisons */
	strcpy( fullPath, filePath );
	internalPath = fullPath + internalPathStart;
	internalPathLen = currPathLen - internalPathStart;
#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __ATARI__ ) || \
	defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ )
	if( ( currPathLen && fullPath[ currPathLen - 1 ] != SLASH ) &&
		fullPath[ currPathLen - 1 ] != ':' )
#else
	if( currPathLen && fullPath[ currPathLen - 1 ] != SLASH )
#endif /* __AMIGA__ || __ARC__ || __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
		/* Append SLASH if necessary */
		fullPath[ currPathLen++ ] = SLASH;
	strcpy( fullPath + currPathLen, MATCH_ALL );
	foundFile = skipFind = findFirst( fullPath, matchAttr, &fileInfo );

	while( TRUE )
		if( foundFile && ( skipFind || findNext( &fileInfo ) ) )
			{
			foundFile = TRUE;
			skipFind = FALSE;

			/* See if the file we have found is a directory */
			if( isDirectory( fileInfo ) )
				{
#if defined( __AMIGA__ ) || defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ ) || defined( __UNIX__ )
				/* Continue if we've hit "." or ".." */
				if( *fileInfo.fName == '.' && ( !fileInfo.fName[ 1 ] ||
							( fileInfo.fName[ 1 ] == '.' && !fileInfo.fName[ 2 ] ) ) )
					continue;
#endif /* __AMIGA__ || __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ || __UNIX__ */

/*				if( recurseOnce || doRecurse ) */
				if( flags & RECURSE_SUBDIR )
					{
					/* Check the path length (the +5 is for the later
					   addition of SLASH between the name and the path, and
					   a SLASH_MATCH_ALL at the end) */
					if( ( ( nameLen = strlen( fileInfo.fName ) ) + currPathLen + 4 ) >= MAX_PATH )
						error( PATH_s_s_TOO_LONG, fullPath, fileInfo.fName );

					/* Set up the information for the new directory */
/*					recurseFlags[ currentLevel ] = doRecurse; */
/*					recurseOnce = FALSE; */
					pathEnd[ currentLevel ] = currPathLen;
					currentDir[ currentLevel++ ] = fileInfo;
					strcpy( fullPath + currPathLen, fileInfo.fName );
					currPathLen += nameLen;
					if( !( dirFlags & DIR_FLATTEN ) )
						{
						/* Add the new directory if it isn't already in the archive */
						if( matchPath( internalPath, currPathLen - internalPathStart,
									   &dirIndex ) != PATH_FULLMATCH )
							{
							/* Only add this path if we've been asked to add
							   all paths anyway regardless of whether they contain
							   any files */
							if( dirFlags & DIR_ALLPATHS )
								{
								addDirPath( fullPath, internalPath,
											currPathLen - internalPathStart, dirIndex );
								archiveChanged = TRUE;
								}
							}
						else
							{
							/* Go to that directory and update its timestamp */
							setArchiveCwd( dirIndex );
							setDirTime( dirIndex, fileInfo.fTime );
							}
						}

#ifndef GUI
					hprintfs( MESG_CHECKING_DIRECTORY_s, internalPath );
#endif /* !GUI */
					if( currentLevel >= MAX_LEVELS )
						error( TOO_MANY_LEVELS_NESTING );
					strcpy( fullPath + currPathLen++, SLASH_MATCH_ALL );
					foundFile = skipFind = findFirst( fullPath, matchAttr, &fileInfo );
					continue;
					}
				else
					/* Make sure we don't mistake directories for filenames */
					continue;
				}

			/* Try and match the filename */
			internalPathLen = currPathLen - internalPathStart;
			for( fileNamePtr = fileNameList; fileNamePtr != NULL;
				 fileNamePtr = fileNamePtr->next )
/*				if( ( forceRecurse && !isDirectory( fileInfo ) ) || \ */
				if( matchString( fileNamePtr->fileName, fileInfo.fName, fileNamePtr->hasWildcards ) )
					if( ( dirFlags & DIR_FLATTEN ) ||
						( matchPath( internalPath, ( internalPathLen ) ? internalPathLen - 1 : 0,
												   &dirIndex ) ) == PATH_FULLMATCH )
						{
/*						\* Check whether what we've found is a directory */
/*						if( isDirectory( fileInfo ) ) */
/*							{ */
/*							\* We've matched a directory, add the directory */
/*							   info and recurse through it */
/*							skipFind = TRUE; */
/*							recurseOnce = forceRecurse = TRUE; */
/*							break;		\* Exit for loop */
/*							} */

retry:
						strcpy( fullPath + currPathLen, fileInfo.fName );

						/* Make sure we don't try to archive the archive */
						skipFile = FALSE;
#ifdef __ARC__
						/* Determining whether two files are the same on the
						   Arc is quite difficult since there is no unique
						   extension to look for.  A better method may be to
						   see whether we can open the file - if we can't,
						   HPACK (or something else) is using it */
						if( ( fileInfo.type == 0x0FFD ) ||
							( ( ( i = strlen( fileInfo.fName ) - 2 ) > 1 ) &&
							  ( fileInfo.fName[ i ] == '?' ) ) )
							switch( fileInfo.fName[ i + 1 ] )
								{
								case '1':
									/* Ends in '?1' */
									if( errorFD != IO_ERROR )
										skipFile = isSameFile( errorFileName, fullPath );
									break;

								case '2':
									/* Ends in '?2' */
									if( dirFileFD != IO_ERROR )
										skipFile = isSameFile( dirFileName, fullPath );
									break;

								case '3':
									/* Ends in '?3' */
									if( secFileFD != IO_ERROR )
										skipFile = isSameFile( secFileName, fullPath );
									break;

								default:
									/* Data file */
									skipFile = isSameFile( archiveFileName, fullPath );
									ch = 0;		/* Get rid of warning */
								}
#else
						if( ( ( i = strlen( fileInfo.fName ) - 3 ) > 1 ) &&
							( ( ch = fileInfo.fName[ i ] ) == '$' ||
								ch == 'H' || ch == 'h' ) )
							{
							/* First we perform a relatively cheap search
							   for the HPAK_MATCH_EXT or "$$x" suffix to see
							   if this is actually an archive file.  Only
							   then will it be necessary to perform the
							   expensive call to check if the files are identical */
							if( matchString( HPAK_MATCH_EXT, fileInfo.fName + i - 1, HAS_WILDCARDS ) ||
								!strncmp( fileInfo.fName + i, "$$", 2 ) )
								{
								/* We've found an HPACK-related file; check
								   them to see if they are identical. The
								   following [ i + 2 ] is safe since we can
								   only arrive at this point if there is a
								   valid suffix at [ i + 2 ] */
								switch( fileInfo.fName[ i + 2 ] )
									{
									case '1':
										/* Extension '.$$1' */
										if( errorFD != IO_ERROR )
											skipFile = isSameFile( errorFileName, fullPath );
										break;

									case '2':
										/* Extension '.$$2' */
										if( dirFileFD != IO_ERROR )
											skipFile = isSameFile( dirFileName, fullPath );
										break;

									case '3':
										/* Extension '.$$3' */
										if( secFileFD != IO_ERROR )
											skipFile = isSameFile( secFileName, fullPath );
										break;

									default:
										/* Extension '.HPK' */
										skipFile = isSameFile( archiveFileName, fullPath );
									}
								}
							}
#endif /* __ARC__ */

						if( skipFile )
							/* The file is an HPACK-related one, skip it */
							break;

						/* Complain if the file is already in the archive and we
						   try to ADD it.  Don't complain otherwise, since it will
						   be replaced during pass 2 of the UPDATE command */
						if( addFileName( ( const WORD ) ( ( dirFlags & DIR_FLATTEN ) ?
											ROOT_DIR : dirIndex ),
										 commentType, fileInfo.fName ) )
							{
							if( choice == ADD )
#ifdef GUI
								alert( ALERT_FILE_IN_ARCH, fileInfo.fName );
#else
								hprintf( MESG_FILE_s_ALREADY_IN_ARCH__SKIPPING,
												fileInfo.fName );
#endif /* GUI */
							break;	/* Exit the for loop */
							}

						/* Ask the user whether they want to handle this file */
						if( ( flags & INTERACTIVE ) &&
							  confirmSkip( MESG_ADD, fileInfo.fName, FALSE ) )
							{
							/* Remove the last filename entered from the name
							   table since we since we won't be adding this file */
							deleteLastFileName();
							break;	/* Exit the for loop */
							}

						if( ( workFD = hopen( fullPath, O_RDONLY | S_DENYWR | A_SEQ ) ) == IO_ERROR )
#ifdef GUI
							alert( ALERT_CANNOT_OPEN_DATAFILE, internalPath );
#else
							hprintf( WARN_CANNOT_OPEN_DATAFILE_s, internalPath );
#endif /* GUI */
						else
							{
							savedInFD = getInputFD();
							setInputFD( workFD );
							addData( fullPath, &fileInfo, dirIndex, workFD );
							hclose( workFD );
							setInputFD( savedInFD );

							/* If we're moving file, remember the name so we
							   can go back and delete it later */
							if( flags & MOVE_FILES )
								addFilePath( fullPath );
							}

						break;	/* Exit the for loop */
						}
					else
						{
						/* Build the path to the file inside the archive.  The
						   currPathLen check is in case of a trailing SLASH */
						addDirPath( fullPath, internalPath, ( internalPathLen ) ?
									internalPathLen - 1 : 0, dirIndex );

						/* Restore the directory index and continue */
						dirIndex = currEntry;
						goto retry;
						}
			}
		else
			/* Go back up one level if we can and try again */
			if( currentLevel )
				{
				fullPath[ currPathLen ? currPathLen - 1 : 0 ] = '\0';
#ifndef GUI
				hprintfs( MESG_LEAVING_DIRECTORY_s, internalPath );
#endif /* !GUI */
				findEnd( &fileInfo );
				fileInfo = currentDir[ --currentLevel ];
				currPathLen = pathEnd[ currentLevel ];
/*				forceRecurse = FALSE; */
/*				doRecurse = recurseFlags[ currentLevel ]; */
				foundFile = TRUE;
				if( !( dirFlags & DIR_FLATTEN ) )
					popDir();
				}
			else
				break;
	}

/* Process a data file (with wildcards allowed) */

void handleArchive( void )
	{
	FD workFD, freshenFD;
	FILEHDRLIST *endPtr = NULL;
	DIRHDRLIST *dirInfoPtr;
	FILEINFO fileInfo;
	FILEPATHINFO *pathInfoPtr;
	FILENAMEINFO *fileInfoPtr;
	char externalPath[ MAX_PATH ], *filePath, *fileName, *path, oldChoice;
	FILEHDRLIST *list2currPtr = NULL, *list2startPtr = NULL;
	int dummy;
	BOOLEAN falseBasePath = FALSE, hasMatch, processedFile;
	WORD dirNo;

	/* Reset the fast I/O subsystem */
	resetFastOut();

	/* Make sure the base path is valid */
	if( *basePath && !dirExists( basePath ) )
		error( CANNOT_ACCESS_BASEDIR, basePath );

	/* Options which add files to an archive */
	if( choice == ADD || choice == UPDATE )
		{
		/* Write ID string at start if necessary */
		if( !htell( archiveFD ) )
			{
			memcpy( _outBuffer, HPACK_ID, HPACK_ID_SIZE );
			_outByteCount = HPACK_ID_SIZE;
			}

		/* Write encryption information */
		if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
			cryptFileDataLength = cryptBegin( ( cryptFlags & CRYPT_SEC ) ?
											  SECONDARY_KEY : MAIN_KEY );
		else
			cryptFileDataLength = 0;

		/* Add the external path component if there is one.  Note that
		   basePath already ends in a SLASH */
		if( *basePath )
			strcpy( externalPath, basePath );

		/* Process each group of specified file in subdirectories seperately.
		   We need to do things in this somewhat messy manner instead of
		   just passing <path>/<name> to findFirst in case we are asked to
		   do subdirectories as well */
		for( pathInfoPtr = filePathListStart; pathInfoPtr != NULL;
			 pathInfoPtr = pathInfoPtr->next )
			{
			/* Add the internal path component */
			strcpy( externalPath + basePathLen, pathInfoPtr->filePath );

#ifdef __VMS__
			/* If there is a node name attached to the path, make sure we
			   don't treat it as part of the file path */
			if( pathInfoPtr->node != NULL )
				processDir( pathInfoPtr->fileNames, externalPath,
							basePathLen + strlen( pathInfoPtr->node ) +
							( ( pathInfoPtr->device != NULL ) ?
							strlen( pathInfoPtr->device ) : 0 ) );
			else
#endif /* __VMS__ */
			/* If there is a device attached to the path, make sure we
			   don't treat it as part of the file path */
			processDir( pathInfoPtr->fileNames, externalPath,
						basePathLen + ( ( pathInfoPtr->device != NULL ) ?
						strlen( pathInfoPtr->device ) : 0 ) );
			}
		if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
			cryptEnd();

		/* Write the archive directory and reset the error status if necessary */
		if( archiveChanged )
			{
			writeArcDir();

			errorFD = IO_ERROR;
			oldArcEnd = 0;
			}

		return;
		}

	/* Options which extract/change files in an archive */
	if( choice == DELETE || choice == FRESHEN || choice == REPLACE )
		{
		/* Open a destination archive file.  Note that we can't use tmpnam()
		   since that would open a file in the current directory, which may
		   or may not be the same as the archive directory.  For the same
		   reason we can't make use of a preset directory for storing temporary
		   files (such as /tmp or getenv( "TMP" ) ).  So we open a file of the
		   form "archiveFileName." + TEMP_EXT */
		strcpy( errorFileName, archiveFileName );
		strcpy( errorFileName + strlen( errorFileName ) - 3, TEMP_EXT );
		if( ( workFD = hcreat( errorFileName, CREAT_ATTR ) ) == IO_ERROR )
			error( CANNOT_OPEN_TEMPFILE );
		memcpy( _outBuffer, HPACK_ID, HPACK_ID_SIZE );	/* Put ID at start */
		_outByteCount += HPACK_ID_SIZE;
		errorFD = workFD;
		setOutputFD( workFD );
		}
	else
		if( choice == EXTRACT || choice == TEST )
			/* Set up info for summary of files which didn't extract/test
			   properly */
			initBadFileInfo();

	/* Select files and directories to process */
	for( fileHdrCurrPtr = getFirstFile( fileHdrStartPtr );
		 fileHdrCurrPtr != NULL;
		 fileHdrCurrPtr = getNextFile() )
		{
		/* Get all the info we need about the filename and path */
		filePath = getPath( fileHdrCurrPtr->data.dirIndex );
		fileName = fileHdrCurrPtr->fileName;
		dirNo = fileHdrCurrPtr->data.dirIndex;
		if( dirNo != ROOT_DIR )
			/* Stomp final SLASH on non-root dir. */
			filePath[ strlen( filePath ) - 1 ] = '\0';
		hasMatch = FALSE;

		/* Try and match the filename and path against a command-line arg */
		for( pathInfoPtr = filePathListStart;
			 pathInfoPtr != NULL && !hasMatch;
			 pathInfoPtr = pathInfoPtr->next )
			{
			/* Get path information, skipping the initial SLASH if necessary */
			path = pathInfoPtr->filePath;
			if( *path == SLASH )
				path++;

			/* Try and match the filename and path against a given filespec */
			for( fileInfoPtr = pathInfoPtr->fileNames;
				 fileInfoPtr != NULL && !hasMatch;
				 fileInfoPtr = fileInfoPtr->next )
				{
				if( matchString( fileInfoPtr->fileName, fileName, fileInfoPtr->hasWildcards ) &&
					( ( flags & RECURSE_SUBDIR ) ||
					  matchString( path, filePath, pathInfoPtr->hasWildcards ) ) &&
					( fileHdrCurrPtr->hType ) == commentType )
					{
					/* Match on fileName/path, select it */
					fileHdrCurrPtr->tagged = TRUE;

					/* Select the directories leading to it */
					if( ( dirNo != ROOT_DIR ) && !( dirFlags & DIR_FLATTEN ) )
						{
						/* Mark the encompassing directory for later creation */
						setDirTaggedStatus( dirNo );

						/* Now walk up the tree to the root directory marking
						   every directory we pass through unless we're creating
						   only the containing directory */
						if( !( dirFlags & DIR_CONTAIN ) )
							while( ( dirNo = getParent( dirNo ) ) != ROOT_DIR &&
								   !getDirTaggedStatus( dirNo ) )
								setDirTaggedStatus( dirNo );
						}

					/* We've matched the name, force an exit from the match loop */
					hasMatch = TRUE;
					}
				}
			}
		}

	/* Now make a second pass selecting directories.  Unfortunately we
	   can't use the same early-out match scheme as before since there
	   may be multiple path spec to dir path matches, so we have to
	   check all path specs against the dir path */
	for( dirNo = getFirstDir(); dirNo != END_MARKER; dirNo = getNextDir() )
		{
		/* Try and match the path to the directory */
		filePath = getPath( dirNo );
		if( dirNo != ROOT_DIR )
			/* Stomp final SLASH on non-root dir */
			filePath[ strlen( filePath ) - 1 ] = '\0';

		for( pathInfoPtr = filePathListStart; pathInfoPtr != NULL;
			 pathInfoPtr = pathInfoPtr->next )
			{
			/* Get path information, skipping the initial SLASH if necessary */
			path = pathInfoPtr->filePath;
			if( *path == SLASH )
				path++;

			hasMatch = FALSE;
			if( matchString( path, filePath, pathInfoPtr->hasWildcards ) )
				{
				/* Try and match the dir.name and path against a given filespec */
				for( fileInfoPtr = pathInfoPtr->fileNames; fileInfoPtr != NULL;
					 fileInfoPtr = fileInfoPtr->next )
					if( ( dirInfoPtr = getFirstDirEntry( dirNo ) ) != NULL )
						do
							if( matchString( fileInfoPtr->fileName, dirInfoPtr->dirName,
								fileInfoPtr->hasWildcards ) )
								{
								hasMatch = TRUE;
								if( flags & RECURSE_SUBDIR )
									/* Select all nested info */
									selectDir( dirInfoPtr, TRUE );
								else
									if( fileInfoPtr->hasWildcards )
										/* Select the directory itself */
										setDirTaggedStatus( dirInfoPtr->dirIndex );
									else
										/* Select only immediate contents */
										selectDirNoContents( dirInfoPtr->dirIndex, TRUE );
								}
						while( ( dirInfoPtr = getNextDirEntry() ) != NULL );

				/* The directory matched but there were no matching files
				   within it, select the directory itself.  This occurs if
				   the form /dir/ is used instead of /dir, to override the
				   file selection */
				if( !hasMatch )
					setDirTaggedStatus( dirNo );
				}
			}
		}

	/* If we're asked to create all paths, mark all directories as selected */
	if( dirFlags & DIR_ALLPATHS )
		{
		getFirstDir();					/* Skip archive root directory */
		for( dirNo = getNextDir(); dirNo != END_MARKER; dirNo = getNextDir() )
			setDirTaggedStatus( dirNo );
		}

	/* If we're in unified compression mode, remember the last tagged file
	   header.  This is useful since it allows an early exit from the block
	   decompression when individual files are being extracted */
	if( flags & BLOCK_MODE )
		for( fileHdrCurrPtr = fileHdrStartPtr; fileHdrCurrPtr != NULL;
			 fileHdrCurrPtr = fileHdrCurrPtr->next )
			if( fileHdrCurrPtr->tagged )
				endPtr = fileHdrCurrPtr;

#ifndef GUI
	/* Simple case: Display selected files/directories and exit */
	if( choice == VIEW )
		{
		listFiles();
		return;
		}
#endif /* !GUI */

	/* Recreate the directory tree in the archive if necessary.  At the
	   moment we only create directories, we don't restore any date stamps,
	   attributesm icons etc since these may change due to file extraction
	   into the directories */
	if( ( choice == EXTRACT ) && !( dirFlags & DIR_FLATTEN ) )
		extractDirTree( EXTRACTDIR_CREATE_ONLY );

	/* Flag the fact that we want to overwrite existing files headers rather
	   than add new ones when we call addData() */
	overWriteEntry = TRUE;

	/* Force a read the first time we call getByte() */
	forceRead();

	/* Process the encryption header if there is one */
	if( ( choice == EXTRACT || choice == TEST || choice == DISPLAY ) &&
		cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
		{
/*!!!!!!!!!*/
/*hputs( "Attempting to adjust for incorrect encryption header..." ); */
/*hputs( "  Added padding bytes, didn't skip dummy header" ); */
		if( !cryptSetInput( getCoprDataLength(), &dummy ) )
/*		if( !cryptSetInput( getCoprDataLength() + 25000L, &dummy ) ) */
/*		if( !cryptSetInput( 245760L, &dummy ) ) */
			error( CANNOT_PROCESS_CRYPT_ARCH );

		/* Skip the dummy header which covers the encryption info */
/*!!!!!!!!*/
		fileHdrCurrPtr = fileHdrStartPtr->next;
/*		fileHdrCurrPtr = fileHdrStartPtr; */
		}
	else
		fileHdrCurrPtr = fileHdrStartPtr;

	skipDist = 0L;
	for( fileHdrCurrPtr = getFirstFile( fileHdrCurrPtr );
		 fileHdrCurrPtr != NULL; fileHdrCurrPtr = getNextFile() )
		{
		processedFile = FALSE;
		filePath = getPath( fileHdrCurrPtr->data.dirIndex );
		if( fileHdrCurrPtr->tagged && fileHdrCurrPtr->hType == commentType )
			{
			if( choice == DELETE )
				{
				/* Ask for confirmation if necessary */
				if( flags & INTERACTIVE )
					if( confirmSkip( MESG_DELETE, buildInternalPath( fileHdrCurrPtr ),
									 FALSE ) )
						goto skipData;

				archiveChanged = TRUE;
#ifndef GUI
				hprintfs( MESG_DELETING_s_FROM_ARCHIVE,
						  buildInternalPath( fileHdrCurrPtr ) );
#endif /* GUI */
				skipDist += fileHdrCurrPtr->data.dataLen +
							fileHdrCurrPtr->data.auxDataLen;

				/* Unlink and free the file header */
				unlinkFileHeader( fileHdrCurrPtr,
								  UNLINK_HDRLIST | UNLINK_ARCDIR | UNLINK_DELETE );

				/* Flag the fact that we've processed a file in this pass */
				processedFile = TRUE;
				}
			else
				if( choice == FRESHEN || choice == REPLACE )
					{
					/* Make sure the matched file is in the archive, that
					   there is another copy on disk to freshen from, and
					   that it's newer than the one in the archive */
					if( addFileName( fileHdrCurrPtr->data.dirIndex,
									 fileHdrCurrPtr->hType,
									 fileHdrCurrPtr->fileName ) == FALSE )
						goto endProcessFile;
					if( ( dirFlags & DIR_FLATTEN ) && !*basePath && *filePath )
						{
						/* File has a path within the archive but DIR_FLATTEN
						   or explicit base path were given, create a false
						   base path corresponding to the internal path */
						strcpy( basePath, filePath );
						basePathLen = strlen( basePath );
						falseBasePath = TRUE;
						}
					if( !findFirst( ( filePath = buildExternalPath( fileHdrCurrPtr ) ),
									( const ATTR ) ( ( flags & STORE_ATTR ) ?
										ALLFILES : FILES ), &fileInfo ) )
						{
						findEnd( &fileInfo );
						goto endProcessFile;
						}
					findEnd( &fileInfo );
					if( ( choice == FRESHEN ) &&
						( fileInfo.fTime <= fileHdrCurrPtr->data.fileTime ) )
						goto endProcessFile;

					/* Ask for confirmation if necessary */
					if( flags & INTERACTIVE )
						if( confirmSkip( ( choice == FRESHEN ) ? MESG_FRESHEN :
																 MESG_REPLACE,
										 filePath, FALSE ) )
							goto endProcessFile;

					archiveChanged = TRUE;

					/* Add the new header data onto the end of list2.  Note
					   that we set list2currPtr to NULL when it was declared
					   even though this isn't necessary since it's presence
					   here trips up many compilers and they see a use of an
					   uninitialized pointer */
					if( list2startPtr == NULL )
						/* Set up list2 if it's not already set up */
						list2startPtr = fileHdrCurrPtr;
					else
						list2currPtr->next = fileHdrCurrPtr;
					list2currPtr = fileHdrCurrPtr;

					/* Add the new data to the archive */
					freshenFD = hopen( filePath, O_RDONLY | S_DENYWR | A_SEQ );
					setInputFD( freshenFD );
					addData( filePath, &fileInfo, fileHdrCurrPtr->data.dirIndex, freshenFD );
					hclose( freshenFD );
					setInputFD( archiveFD );

					/* If we're moving file, remember the name so we can go
					   back and delete it later */
					if( flags & MOVE_FILES )
						addFilePath( filePath );

					/* Unlink the header for the file to be updated */
					unlinkFileHeader( fileHdrCurrPtr, UNLINK_HDRLIST );

					/* Flag the fact that we've processed a file in this pass */
					processedFile = TRUE;
					}
				else
					{
					/* Extract data */
					skipToData();
					retrieveData( fileHdrCurrPtr );

					/* If there are no more files to decompress, exit now
					   (useful if BLOCK_MODE is set since we don't need to
					   process any data after this point) */
					if( fileHdrCurrPtr == endPtr )
						break;
					}
			}

endProcessFile:
		/* Reset the false base path if necessary */
		if( falseBasePath )
			{
			*basePath = '\0';
			basePathLen = 0;
			falseBasePath = FALSE;
			}

		/* Check if there was a match for this file, and if we haven't
		   already processed a file.  The second check is necessary since
		   we may have deleted a file header as part of the operation
		   performed, and fileHdrCurrPtr now points at another file header */
		if( !processedFile && !fileHdrCurrPtr->tagged  )
			if( choice == DELETE )
				{
skipData:
				skipToData();
				moveData( fileHdrCurrPtr->data.dataLen + fileHdrCurrPtr->data.auxDataLen );
				}
			else
				if( flags & BLOCK_MODE )
					{
					/* We're decompressing in block mode, we have to decompress
					   this file to keep the decompressor in sync, so we turn
					   the extract into a test */
					oldChoice = choice;
					choice = TEST;
					skipToData();
					retrieveData( fileHdrCurrPtr );
					choice = oldChoice;
					}
				else
					/* Skip this archived file */
					skipDist += fileHdrCurrPtr->data.dataLen +
								fileHdrCurrPtr->data.auxDataLen;
		}

	/* Handle the archive update commands by moving any remaining data across
	   to the new archive, concatenating the header list for this data onto
	   the end of the one for the new archive, and then calling writeArcDir() */
	if( choice == FRESHEN || choice == REPLACE )
		{
		/* Step through the list of headers copying the data across as we go */
		for( fileHdrCurrPtr = fileHdrStartPtr; fileHdrCurrPtr != NULL;
			 fileHdrCurrPtr = fileHdrCurrPtr->next )
			{
			hlseek( archiveFD, fileHdrCurrPtr->offset + HPACK_ID_SIZE, SEEK_SET );
			moveData( fileHdrCurrPtr->data.dataLen + fileHdrCurrPtr->data.auxDataLen );
			}

		/* Concatenate the header list onto the end of the header list for the
		   new archive if there is one */
		if( list2startPtr != NULL )
			{
			list2currPtr->next = fileHdrStartPtr;
			fileHdrStartPtr = list2startPtr;
			}
		}

	/* Perform cleanup for those functions that need it */
	if( choice == DELETE || choice == FRESHEN || choice == REPLACE )
		{
		/* Flush the data buffer */
		flushBuffer();

		/* Delete the old archive and rename the temp file to the archive
		   name unless it's been deleted (which can happen if we delete all
		   the files in the archive) */
		copyExtraInfo( archiveFileName, errorFileName );
		hclose( archiveFD );
		archiveFD = workFD;
		writeArcDir();
		hclose( archiveFD );
		hunlink( archiveFileName );
		if( errorFD != ERROR )
			hrename( errorFileName, archiveFileName );

		/* Flag the fact that we no longer need to attempt any recovery in
		   case of an error */
		archiveFD = errorFD = dirFileFD = IO_ERROR;
		}
	else
		if( choice == EXTRACT || choice == TEST )
			{
			/* If we were extracting or testing files, print a summary of
			   corrupted files (since these often scroll off the screen too
			   quickly to see) */
			processBadFileInfo();

			/* Now that all files have been extracted, make a second pass
			   setting attributes, dates, and other information which might
			   have been affected by the writing of files into the created
			   directories */
			if( ( choice == EXTRACT ) && !( dirFlags & DIR_FLATTEN ) )
				extractDirTree( EXTRACTDIR_SET_ATTR );
			}

	}	/* "Real programmers can write five-page-long do loops
			without difficulty" - _Real_Men_Don't_Program_Pascal_ */
