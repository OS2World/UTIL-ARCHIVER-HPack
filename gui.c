/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								HPACK GUI Interface							*
*							  GUI.C  Updated 06/05/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1992  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* "Another Brick Through the Window, Part 2"

	{\lead}
	We don't need no pull-down-menus
	We don't need no rescaled fonts
	No dark icons in the corner
	Hackers, leave those Macs alone.
	Hey! Hackers! Leave them Macs alone!
	All in all its just another WIMP up for sale
	All in all you're just another WIMP up for the sale.

	{\kids}
	We don't need no fancy windows
	We don't need no title bars
	No MultiFinder in the startup
	Hackers leave them Macs alone
	Hey! Hackers! Leave them Macs alone!
	All in all its just another WIMP up for sale
	All in all you're just another WIMP up for the sale.

	{\guitar}

	"Another Brick Through the Window, Part 3"

	I don't need no mice around me
	And I don't need no fonts to calm me.
	I have seen the writing on the wall.
	Don't think I need any WIMP at all.
	No! Don't think I need any WIMP at all.
	No! Don't think I'll need any WIMP at all.
	All in all it was all just bricks through the window.
	All in all you were all just bricks through the window.

		- Nathan Torkington */

#include <ctype.h>
#include <string.h>
#include <time.h>
#ifdef __MAC__
  #include "defs.h"
  #include "arcdir.h"
  #include "choice.h"
  #include "error.h"
  #include "filesys.h"
  #include "flags.h"
  #include "frontend.h"
  #include "hpacklib.h"
  #include "script.h"
  #include "system.h"
  #include "wildcard.h"
  #include "crypt.h"
  #include "fastio.h"
#else
  #include "defs.h"
  #include "arcdir.h"
  #include "choice.h"
  #include "error.h"
  #include "filesys.h"
  #include "flags.h"
  #include "frontend.h"
  #include "hpacklib.h"
  #include "script.h"
  #include "system.h"
  #include "wildcard.h"
  #include "crypt/crypt.h"
  #include "io/fastio.h"
#endif /* __MAC__ */

/* Prototypes for functions in SCRIPT.C */

void addFilespec( char *fileSpec );
void freeFilespecs( void );
void processListFile( const char *listFileName );

/* Prototypes for compression functions */

void initPack( const BOOLEAN initAllBuffers );
void endPack( void );

/* The following are defined in ARCHIVE.C */

extern BOOLEAN overWriteEntry;	/* Whether we overwrite the existing file
								   entry or add a new one in addFileHeader()
								   - used by FRESHEN, REPLACE, UPDATE */

/* The default match string, which matches all files.  This must be done
   as a static array since if given as a constant string some compilers
   may put it into the code segment which may cause problems when FILESYS.C
   attempts any conversion on it */

char WILD_MATCH_ALL[] = "*";

/* The following is normally declared in VIEWFILE.C, but is used somewhat
   differently by the GUI code */

int dateFormat = 0;

#ifdef __MSDOS__
/* The drive the archive is on (needed for multidisk archives) */

extern BYTE archiveDrive;
#endif /* __MSDOS__ */

/****************************************************************************
*																			*
*							Serious Error Handler							*
*																			*
****************************************************************************/

#if 0

void error( ERROR_INFO *errInfo, ... )
	{
	/*
	** Handle any error recovery as in error.c (in the Mac's case, just
	** crashing would probably be appropriate).  Depending on the severity
	** you would exit back to the OS or just complain via an idiot box.
	*/
	}
#endif /* 0 */

/****************************************************************************
*																			*
*								Idiot Boxes									*
*																			*
****************************************************************************/

void alert( ALERT_TYPE alertType, const char *message )
	{
	puts( "vvvvvvvv" );
	switch( alertType )
		{
		case ALERT_FILE_IN_ARCH:
			printf( "File %s already in archive - skipping\n", message );
			break;

		case ALERT_TRUNCATED_PADDING:
			printf( "Truncated %s bytes of EOF padding\n", message );
			break;

		case ALERT_DATA_ENCRYPTED:
			printf( "Data is encrypted - skipping %s\n", message );
			break;

		case ALERT_DATA_CORRUPTED:
			puts( "Warning: Data probably corrupted" );
			break;

		case ALERT_PATH_TOO_LONG:
			printf( "Path %s... too long\n", message );
			break;

		case ALERT_CANNOT_PROCESS_ENCR_INFO:
			printf( "Cannot process encryption information - skipping %s\n", message );
			break;

		case ALERT_CANNOT_OPEN_DATAFILE:
			printf( "Cannot open data file %s - skipping\n", message );
			break;

		case ALERT_CANNOT_OPEN_SCRIPTFILE:
			printf( "Cannot open script file %s\n", message );
			break;

		case ALERT_UNKNOWN_ARCH_METHOD:
			printf( "Unknown archiving method - skipping %s\n", message );
			break;

		case ALERT_NO_OVERWRITE_EXISTING:
			printf( "Won't overwrite existing file %s\n", message );
			break;

		case ALERT_FILE_IS_DEVICEDRVR:
			puts( "File corresponds to device driver" );
			break;

		case ALERT_ARCHIVE_UPTODATE:
			puts( "Archive is uptodate" );
			break;

		case ALERT_ERROR_DURING_ERROR_REC:
			puts( "Communist plot detected.  Archiver suiciding" );
			break;

		case ALERT_UNKNOWN_SCRIPT_COMMAND:
			printf( "Unknown script command %s\n", message );
			break;

		case ALERT_PATH_TOO_LONG_LINE:
			printf( "Path too long in line %s\n", message );
			break;

		case ALERT_BAD_CHAR_IN_FILENAME_LINE:
			printf( "Bad char in filename on line %s\n", message );
			break;

		case ALERT_CANT_FIND_PUBLIC_KEY:
			puts( "Cannot find public key" );
			break;

		case ALERT_SEC_INFO_CORRUPTED:
			puts( "Security information corrupted" );
			break;

		case ALERT_SECRET_KEY_COMPROMISED:
			printf( "Secret key compromised for userID %s\n", message );
			puts( "This public key cannot be used" );
			break;

		case ALERT_ARCH_SECTION_TOO_SHORT:
			puts( "Archive section too short, skipping..." );
			break;

		case ALERT_DIRNAME_CONFLICTS_WITH_FILE:
			printf( "Directory name %s conflicts with existing file - skipping\n", message );
			break;

		case ALERT_PASSWORD_INCORRECT:
			puts( "Password is incorrect" );
			break;

		case ALERT_FILES_CORRUPTED:
			if( !strcmp( message, "1" ) )
				puts( "One file was corrupted" );
			else
				printf( "%d files were corrupted\n", message );
			break;
		}

	puts( "^^^^^^^^" );
	}

/****************************************************************************
*																			*
*								Various Dialog Boxes						*
*																			*
****************************************************************************/

/* Conio function prototypes */

int getch( void );		/* getchar() is brain-damaged */
void clrscr( void );	/* Clear screen */

/* Handle an idiot box - either return TRUE or FALSE to the caller, or
   exit if response is FALSE */

BOOLEAN confirmAction( const char *message )
	{
	/*
	** Return TRUE or FALSE depending on whether the luser selects TRUE or
	** FALSE based on the message prompt.  See CLI equivalent in ARCHIVE.C
	*/
	char ch;

	puts( "vvvvvvvv" );
	printf( "%s: y/n\n", message );
	ch = getch();
	puts( "^^^^^^^^" );
	return( ( ch == 'y' ) ? TRUE : FALSE );
	}

void idiotBox( const char *message )
	{
	/*
	** Either return or exit depending on whether the luser selects TRUE or
	** FALSE based on the message prompt.  See CLI equivalent in ARCHIVE.C
	*/
	char ch;

	puts( "vvvvvvvv" );
	printf( "Warning: %s. Continue y/n\n", message );
	ch = getch();
	puts( "^^^^^^^^" );

	if( ch == 'n' )
		{
		errorFD = IO_ERROR;	/* Make sure we don't delete archive by mistake */
		error( STOPPED_AT_USER_REQUEST );
		}
	}

/* Confirm skipping a file */

BOOLEAN confirmSkip( const char *str1, const char *str2, const BOOLEAN str1Filename )
	{
	/*
	** Confirm skipping/overwriting an existing file.  See CLI equivalent
	** in ARCHIVE.C
	*/
	char ch;

	if( str1Filename );		/* Get rid of warning */
	puts( "vvvvvvvv" );
	printf( "%s %s, y/n/a\n", str1, str2 );
	ch = getch();
	puts( "^^^^^^^^" );

	if( ch == 'n' )
		return( TRUE );

	if( ch == 'a' )
		{
		/* Do an automatic "Confirm" for all following files */
		if( flags & INTERACTIVE )
			flags &= ~INTERACTIVE;
		else
			overwriteFlags |= OVERWRITE_ALL;
		}

	return( FALSE );
	}

/* Get a filename from the luser */

void getFileName( char *fileName )
	{
	printf( "%s already exists:  Enter new filename\n", fileName );
	gets( fileName );
	}

/* Highly complex dialog box used to handle multidisk archive I/O */

void multipartWait( const WORD promptType, const int partNo )
	{
	/*
	** See the equivalent function in FASTIO.C for more details
	*/

	puts( "vvvvvvvv" );
	if( promptType & WAIT_PARTNO )
		printf( "This is part %d of a multipart archive\n", partNo + 1 );
	else
		putchar( '\n' );
	printf( "Please insert the " );
	if( promptType & WAIT_NEXTDISK )
		printf( "next disk" );
	else
		if( promptType & WAIT_PREVDISK )
			printf( "previous disk" );
		else
			{
			printf( "disk containing " );
			if( promptType & WAIT_LASTPART )
				printf( "the last part" );
			else
				printf( "part %d", partNo + 1 );
			printf( " of this archive" );
			}
	printf( " and press a key\n" );
	getch();
	puts( "^^^^^^^^" );
	}

/* Get a password from the luser, no echo */

char *getPassword( char *passWord, const char *prompt )
	{
	puts( "vvvvvvvv" );
	printf( prompt );
	gets( passWord );
	putchar( '\n' );
	puts( "^^^^^^^^" );
	return( passWord );
	}

/* Display the status of a signature on a piece of data.  The good signature
   message is an information-type message, the bad signature message is an
   alert()-type message */

void showSig( const BOOLEAN isGoodSig, const char *userID, const LONG timeStamp )
	{
	puts( "vvvvvvvv" );
	if( isGoodSig )
		printf( "Good signature from %s\n", userID );
	else
		printf( "Bad signature from %s\n", userID );
	printf( "Signature made on %s\n", ctime( ( time_t * ) &timeStamp ) );
	puts( "^^^^^^^^" );
	}

/****************************************************************************
*																			*
*							Compression Progress Report						*
*																			*
****************************************************************************/

static long totalLength, currentLength;

/* Display the initial compression progress bar */

void initProgressReport( const BOOLEAN fileNameTruncated, const char *oldFileName, \
						 const char *fileName, const LONG fileLen )
	{
	/*
	** Initialise a progress indicator for the file fileName of length
	** fileSize.  If fileNameTrunc is TRUE then the filename has been
	** truncated from oldFileName
	*/

	/* Save total data length */
	totalLength = fileLen;
	currentLength = 0L;

	puts( "vvvvvvvv" );
	printf( "Initing progress bar: %ld bytes\n", fileLen );
	printf( "Processing " );
	if( fileNameTruncated )
		printf( "%s as ", oldFileName );
	printf( "%s\n", fileName );
	puts( "^^^^^^^^" );
	}

/* Update progress bar */

void updateProgressReport( void )
	{
	/*
	** Update progress bar.  Called for every 2K between 0...totalLength
	*/

	/* Update data length */
	currentLength += 2048;

	puts( "vvvvvvvv" );
	printf( "Updating progress bar: %d%% complete\n", \
			 ( currentLength * 100 ) / totalLength );
	puts( "^^^^^^^^" );
	}

/* Erase progress bar */

void endProgressReport( void )
	{
	/*
	** Erase progress bar
	*/

	puts( "vvvvvvvv" );
	puts( "Ending progress bar" );
	puts( "^^^^^^^^" );
	}

/****************************************************************************
*																			*
*								Main Program Code							*
*																			*
****************************************************************************/

#ifdef __MSDOS__

/* Kludge code for DOS */

void *hmalloc( const unsigned long noBytes )
	{
	return( malloc( ( size_t ) noBytes ) );
	}

void *hmallocSeg( const WORD noBytes )
	{
	/* TurboC uses 8 bytes for the MCB, so we just add 1 to the segment
	   to get a paragraph-aligned address */
	return( MK_FP( FP_SEG( malloc( noBytes + 8 ) ) + 1, 0 ) );
	}

void hfree( void *pointer )
	{
	/* Check if it's a normalised pointer */
	if( !( FP_OFF( pointer ) & 0x000F ) )
		/* Yes, call free with true address of MCB */
		free( MK_FP( FP_SEG( pointer ) - 1, 8 ) );
	else
		/* No, just free it */
		free( pointer );
	}
#endif /* __MSDOS__ */

/* Global vars used by the frontend code */

BOOLEAN hasFileSpec = FALSE, createNew = FALSE;
char archivePath[ MAX_PATH ], fileCode[ MATCH_DEST_LEN ];

/* Get and validate the archive name */

void getArchiveName( void )
	{
	char archiveName[ 80 ];
	int lastSlash, i;

	puts( "vvvvvvvv" );
	printf( "Enter archive name " );
	gets( archiveName );
	putchar( '\n' );
	puts( "^^^^^^^^" );

	/* Check archive name */
	if( strlen( archiveName ) > MAX_PATH - 5 )
		error( PATH_s_TOO_LONG, archiveName );
	else
		strcpy( archiveFileName, archiveName );

	/* Extract the pathname and convert it into an OS-compatible format */
	lastSlash = extractPath( archiveFileName, archivePath );
#ifdef __MSDOS__
	archiveDrive = ( archiveFileName[ 1 ] == ':' ) ? toupper( *archiveFileName ) - 'A' + 1 : 0;
#endif /* __MSDOS__ */

	/* Either append archive file suffix to the filespec or force the
	   existing suffix to HPAK_EXT */
	for( i = lastSlash; archiveFileName[ i ] && archiveFileName[ i ] != '.'; i++ );
	if( archiveFileName[ i ] != '.' )
		strcat( archiveFileName, HPAK_EXT );
	else
		strcpy( archiveFileName + i, HPAK_EXT );
	}

/* Get and process the filename */

void getArchFilename( void )
	{
	char fileName[ 80 ];
	FILEINFO fileInfo;
	int lastSlash;
	char ch;
	BOOLEAN fileHasWildcards;

	puts( "vvvvvvvv" );
	printf( "Enter file name " );
	gets( fileName );
	putchar( '\n' );
	puts( "^^^^^^^^" );

	/* Add the name of the file to be processed to the fileName list */
	if( *fileName == '@' )
		{
		/* Assemble the listFile name + path into the dirBuffer (we can't
		   use the mrglBuffer since processListFile() ovrewrites it) */
		strcpy( ( char * ) dirBuffer, fileName + 1 );
		lastSlash = extractPath( ( char * ) dirBuffer, ( char * ) dirBuffer + 80 );
		compileString( ( char * ) dirBuffer + lastSlash, ( char * ) dirBuffer + 80 );
		fileHasWildcards = strHasWildcards( ( char * ) dirBuffer + 80 );
#if defined( __MSDOS__ ) || defined( __OS2__ )
		if( lastSlash && ( ch = dirBuffer[ lastSlash - 1 ] ) != SLASH && \
														  ch != ':' )
#else
		if( lastSlash && ( ch = dirBuffer[ lastSlash - 1 ] ) != SLASH )
#endif /* __MSDOS__ || __OS2__ */
			/* Add a slash if necessary */
			dirBuffer[ lastSlash - 1 ] = SLASH;
		strcpy( ( char * ) dirBuffer + lastSlash, MATCH_ALL );

		/* Check each file in the directory, passing those that match to
		   processListFile() */
		if( findFirst( ( char * ) dirBuffer, FILES, &fileInfo ) )
			{
			do
				{
				strcpy( ( char * ) dirBuffer + lastSlash, fileInfo.fName );
				if( matchString( ( char * ) dirBuffer + 80, fileInfo.fName, fileHasWildcards ) )
					processListFile( ( char * ) dirBuffer );
				}
			while( findNext( &fileInfo ) );
			findEnd( &fileInfo );
			}
		}
	else
		addFilespec( fileName );
	hasFileSpec = TRUE;
	}

/* Display the archive directory, allowing manipulation of the files/dirs */

typedef enum { SEL_PARENTDIR, SEL_SUBDIR, SEL_FILE } SELECTTYPE;

void showArchiveDirectory( void )
	{
	FILEHDRLIST *fileInfoPtr, *currFileInfoPtr;
	DIRHDRLIST *dirInfoPtr, *currDirInfoPtr;
	WORD count, nextDir;
	int line, currLine = 0, ch;
	SELECTTYPE currentSelection;
	LONG theTime;

	/* First, sort the files in each directory */
	for( count = getFirstDir(); count != END_MARKER; count = getNextDir() )
		sortFiles( count );

	count = getFirstDir();
	while( TRUE )
		{
		currentSelection = 0;
		clrscr();
		line = 0;

		/* Display a parent direcotory if we're not on the root dir */
		if( count != ROOT_DIR )
			{
			if( line == currLine )
				currentSelection = SEL_PARENTDIR;

			printf( ( line == currLine ) ? "-> " : "   " );
#pragma warn -def
			printf( ( getDirTaggedStatus( count ) ) ? "#" : " " );
#pragma warn +def
			printf( " %15s ", ".." );
			theTime = getDirTime( count );
			printf( ctime( ( time_t * ) &theTime ) );
			line++;
			}

		/* First display directories */
		if( ( dirInfoPtr = getFirstDirEntry( count ) ) != NULL )
			do
				{
				if( line == currLine )
					{
					currentSelection = SEL_SUBDIR;
					currDirInfoPtr = dirInfoPtr;
					}

				printf( ( line == currLine ) ? "-> " : "   " );
				printf( ( dirInfoPtr->tagged ) ? "#" : " " );
				printf( " %15s ", dirInfoPtr->dirName );
				printf( ctime( ( time_t * ) &dirInfoPtr->data.dirTime ) );
				line++;
				}
			while( ( dirInfoPtr = getNextDirEntry() ) != NULL );

		/* Then display files */
		if( ( fileInfoPtr = getFirstFileEntry( count ) ) != NULL )
			do
				/* Exclude special-format files */
				if( !( fileInfoPtr->data.archiveInfo & ARCH_SPECIAL ) )
					{
					if( line == currLine )
						{
						currentSelection = SEL_FILE;
						currFileInfoPtr = fileInfoPtr;
						}

					printf( ( line == currLine ) ? "-> " : "   " );
					printf( ( fileInfoPtr->tagged ) ? "#" : " " );
					printf( " %15s ", fileInfoPtr->fileName );
					printf( ctime( ( time_t * ) &fileInfoPtr->data.fileTime ) );
					line++;
					}
			while( ( fileInfoPtr = getNextFileEntry() ) != NULL );

		ch = toupper( getch() );
		if( !ch )
			ch = getch();
		if( ch == 0x48 && currLine )
			currLine--;
		else
			if( ch == 0x50 && currLine < line - 1 )
				currLine++;
			else
				if( ch == '\r' )
					{
					/* Go up/down a directory level */
					if( currentSelection == SEL_PARENTDIR )
						{
						count = getParent( count );
						currLine = 0;
						}
					if( currentSelection == SEL_SUBDIR && \
						( nextDir = getNextDir() ) != END_MARKER )
						{
						count = nextDir;
						currLine = 0;
						}
					setArchiveCwd( count );
					}
				else
					if( ch == ' ' )
						{
						if( currentSelection == SEL_SUBDIR )
							selectDir( currDirInfoPtr, !currDirInfoPtr->tagged );
						if( currentSelection == SEL_FILE )
							selectFile( currFileInfoPtr, !currFileInfoPtr->tagged );
						}
					else
						if( ch == 'Q' )
							break;
		}
	}

/* The main program */

void main( void )
	{
	BOOLEAN doProcess = FALSE;
	char ch;

	/* Perform general initialization */
	initFastIO();
#ifndef __OS2__
	initExtraInfo();
#endif /* __OS2__ */
	getScreenSize();
	initPack( TRUE );
	initArcDir();

	/* Reset general status vars */
	hasFileSpec = FALSE;

	/* Process commands */
	while( TRUE )
		{
		clrscr();
		puts( "Main menu: Choose [C]ommand, [O]ption, [A]rchive name, [F]ile name, [G]o" );
		ch = getch();
		switch( toupper( ch ) )
			{
			case 'C':
				puts( "Enter command [A]dd, [T]est, [N]ew" );
				choice = toupper( getch() );
				if( choice == 'N' )
					{
					choice = 'A';
					createNew = TRUE;
					}
				break;

			case 'O':
				puts( "Setting option to OVERWRITE_SRC" );
				flags |= OVERWRITE_SRC;
				break;

			case 'A':
				getArchiveName();
				break;

			case 'F':
				getArchFilename();
				break;

			case 'G':
				doProcess = TRUE;
			}

		/* Check if user has selected 'Go' */
		if( doProcess )
			{
			doProcess = FALSE;

			/* Default is all files if no name is given */
			if( !hasFileSpec )
				addFilespec( WILD_MATCH_ALL );

			/* Set up the encryption system */
			initCrypt();

			/* Reset error stats and arcdir system */
			archiveFD = errorFD = dirFileFD = secFileFD = IO_ERROR;
			oldArcEnd = 0L;
			resetArcDir();
			overWriteEntry = FALSE;
			if( flags & BLOCK_MODE )
				firstFile = TRUE;

			/* Open archive file with various types of mungeing depending on the
			   command type */
			if( choice == ADD || choice == UPDATE )
				{
				/* Create a new archive if none exists or overwrite an existing
				   one if asked for */
				if( createNew )
					{
					errorFD = archiveFD = hcreat( archiveFileName, CREAT_ATTR );
					strcpy( errorFileName, archiveFileName );
					setOutputFD( archiveFD );
					}
				else
					if( ( archiveFD = hopen( archiveFileName, O_RDWR | S_DENYRDWR | A_SEQ ) ) != IO_ERROR )
						{
						setInputFD( archiveFD );
						setOutputFD( archiveFD );
						readArcDir( SAVE_DIR_DATA );

						/* If we are adding to an existing archive and run out
						   of disk space, we must cut back to the size of the
						   old archive.  We do this by remembering the old state
						   of the archive and restoring it to this state if
						   necessary */
						if( fileHdrStartPtr != NULL )
							/* Only set oldArcEnd if there are files in the archive */
							oldArcEnd = fileHdrCurrPtr->offset + fileHdrCurrPtr->data.dataLen + \
										fileHdrCurrPtr->data.auxDataLen;
						getArcdirState( ( FILEHDRLIST ** ) &oldHdrlistEnd, &oldDirEnd );

						/* Move past existing data */
						hlseek( archiveFD, oldArcEnd + HPACK_ID_SIZE, SEEK_SET );
						}
				}
			else
				{
				/* Try to open the archive for read/write (in case we need to
				   truncate X/Ymodem padding bytes.  If that fails, open it
				   for read-only */
				if( ( archiveFD = hopen( archiveFileName, O_RDWR | S_DENYWR | A_RANDSEQ ) ) == IO_ERROR )
					archiveFD = hopen( archiveFileName, O_RDONLY | S_DENYWR | A_RANDSEQ );
				setInputFD( archiveFD );
				if( archiveFD != IO_ERROR )
					readArcDir( choice != VIEW && choice != EXTRACT && \
								choice != TEST && choice != DISPLAY  );
				}

			/* Make sure the open was successful */
			if( archiveFD == IO_ERROR )
				error( CANNOT_OPEN_ARCHFILE, archiveFileName );

			/* Show the directory and allow file tagging if there is an
			   archive directory */
			if( !createNew )
				showArchiveDirectory();

			/* Now munge files to/from archive */
			handleArchive();

			/* The UPDATE command is just a combination of the ADD and FRESHEN
			   commands.  We handle it by first adding new files in one pass
			   (but not complaining when we strike duplicates like ADD does),
			   and then using the freshen command to update duplicates in a
			   second pass.  This leads to a slight problem in that when files
			   are added they will then subsequently match on the freshen pass;
			   hopefully the extra overhead from checking whether they should
			   be freshened will not be too great */
			if( choice == UPDATE )
				{
				choice = FRESHEN;
				oldArcEnd = 0L;	/* Make sure we don't try any ADD-style recovery
								   since errorFD is now the temp file FD */
				vlseek( 0L, SEEK_SET );	/* Go back to start of archive */
				handleArchive();
				}

			/* Perform any updating of extra file information on the archive
			   unless it's a multipart archive, which leads to all sorts of
			   complications */
			if( archiveChanged && !( flags & MULTIPART_ARCH ) )
				setExtraInfo( archiveFD );

			/* Perform cleanup functions for this file */
			if( choice != DELETE && choice != FRESHEN && choice != REPLACE )
				/* If DELETE, FRESHEN, or REPLACE, archive file is already closed */
				{
				hclose( archiveFD );
				archiveFD = IO_ERROR;	/* Mark it as invalid */
				}

			/* Only go through the loop once if we are either creating a new
			   archive or processing a multipart archive */
			if( createNew || flags & MULTIPART_ARCH )
				break;
			}
		}

	/* Clean up before exiting */
	if( choice != DELETE )
		endPack();
	endCrypt();
	endArcDir();
	endExtraInfo();
	endFastIO();
	freeFilespecs();
	freeArchiveNames();
	}
