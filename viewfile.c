/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					   Display the Contents of an Archive					*
*						  VIEWFILE.C  Updated 21/05/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/*			Barmaglot
			---------

	Varkalos', hlivkie shor'ki
	Pyr'alis' po nave.
	I hr'ukotali zel'uki,
	Kak m'umziki v move.

	O, bojs'a Barmaglota, syn!
	On tak svirlep i dik!
	A v glushe rymit ispolin-
	Zlopastnyj Brandashmyg.

	No vz'al on mech i vz'al on shchit
	Vysokih polon dum,
	V glushchobu put' ego lezhit,
	Pod derevo Tumtum.

	On vstal pod derevo i zhd'ot,
	I tut graahnul grom -
	Letit uzhasnyj Barmaglot
	I pylkaet ogn'om.

	Raz-dva, raz-dva! Gorit trava!
	Vzy-vzy - strizhaet mech,
	Uva-uva - i golova
	Barabardaet s plech!

	O svetozarnyj mal'chik moj!
	Ty pobedil v boju!
	O, hrabroslavlennyj geroj,
	Hvalu tebe poju!

	Varkalos', hlivkie shor'ki
	Pyr'alis' po nave.
	I hr'ukotali zel'uki,
	Kak m'umziki v move */

#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "choice.h"
#include "error.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "sql.h"
#include "system.h"
#include "tags.h"
#include "wildcard.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
#else
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* Prototypes for functions in ARCHIVE.C */

int extractData( const WORD dataInfo, const BYTE extraInfo,
				 LONG dataLen, const LONG fileLen, const BOOLEAN isFile );

#ifdef __MSDOS16__

/* Prototypes for functions in MISC.ASM */

int getRatio( long dataLen, long compressedLen );
#endif /* __MSDOS16__ */

/****************************************************************************
*																			*
*							Display Contents of Archive						*
*																			*
****************************************************************************/

/* The names of the OS's. */

const char *systemName[] = { "MSDOS", " UNIX", "Amiga", " Mac",
							 " Arc", " OS/2", " IIgs", "Atari",
							 " VMS", "Prime", "?????" };

/* Data to control the printing of the directory fields.  The fields are
   NAME, DATE, TIME, SIZE, LENGTH, SYSTEM, and RATIO */

static int fieldWidths[] = { 0, 9, 9, 8, 8, 7, 6 };
static BOOLEAN hasBottomLine[] = { TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE };

/* What format to print the date in */

enum { DATE_MDY, DATE_DMY, DATE_YMD, DATE_MYD, DATE_DYM, DATE_YDM };

int dateFormat;		/* Which of the above date formats to use */

/* Determine the format to print the date in */

void setDateFormat( void )
	{
	dateFormat = getCountry();
	}

/* Extract the date fields from the time value */

#ifdef __MSDOS16__
  void extractDate( const LONG time, int *time1, int *time2, int *time3,
									 int *hours, int *minutes, int *seconds );
#else

void extractDate( const LONG time, int *time1, int *time2, int *time3,
								   int *hours, int *minutes, int *seconds )
	{
	struct tm *localTime;

	/* Extract time fields and get month count from 1..n not 0..n-1 */
	localTime = gmtime( ( time_t * ) &time );
	localTime->tm_mon++;

	*hours = localTime->tm_hour;
	*minutes = localTime->tm_min;
	*seconds = localTime->tm_sec;

	/* Set up fields depending on country */
	switch( dateFormat )
		{
		case DATE_DMY:
			/* Use DD/MM/YY format */
			*time1 = localTime->tm_mday;
			*time2 = localTime->tm_mon;
			*time3 = localTime->tm_year;
			break;

		case DATE_YMD:
			/* Use YY/MM/DD format */
			*time1 = localTime->tm_year;
			*time2 = localTime->tm_mon;
			*time3 = localTime->tm_mday;
			break;

		default:
			/* Use MM/DD/YY format */
			*time1 = localTime->tm_mon;
			*time2 = localTime->tm_mday;
			*time3 = localTime->tm_year;
		}
	}
#endif /* __MSDOS16__ */

/* Fill a field with a certain value */

static void fillField( int ch, int count )
	{
	while( count-- )
		hputchar( ch );
	}

/* Format a filename to fit the screen size */

static char *formatFilename( char *fileName, int fieldWidth, BOOLEAN isUnicode )
	{
	int fileNameLen, maxFileNameLen;
	static char formattedFileName[ 128 ];

	/* Adjust for very narrow fields */
	if( fieldWidth < 2 )
		fieldWidth = 2;

	/* If it's a Unicode filename, at the moment just return an indicator of
	   this fact (virtually no OS's support this yet) */
	if( isUnicode )
		return( "<Unicode>" );

	/* Get filename, truncate if overly long, and add end marker */
	fileNameLen = strlen( fileName );
	maxFileNameLen = ( fileNameLen > fieldWidth ) ? fieldWidth : fileNameLen;
	strncpy( formattedFileName, fileName, maxFileNameLen );
	if( fileNameLen > fieldWidth )
		strcpy( formattedFileName + fieldWidth - 2, ".." );
	else
		formattedFileName[ maxFileNameLen ] = '\0';

	return( formattedFileName );
	}

/* Display information on one file */

static void showFileInfo( const FILEHDRLIST *fileInfoPtr, int nameFieldWidth )
	{
	const FILEHDR *theHeader = &fileInfoPtr->data;
	int time1, time2, time3, hours, minutes, seconds;
	BYTE cryptInfo = ( theHeader->archiveInfo & ARCH_EXTRAINFO ) ?
			*fileInfoPtr->extraInfo & ( EXTRA_SECURED | EXTRA_ENCRYPTED ) : 0;
	int ratio, index;

	/* Handle file, data lengths and compression ratios.  The check for
	   fileLen > dataLen is necessary since we may end up expanding the
	   data when we append the checksum to the end */
#ifdef __MSDOS16__
	ratio = getRatio( theHeader->fileLen, theHeader->dataLen );
#else
	if( theHeader->fileLen && theHeader->fileLen > theHeader->dataLen )
		ratio = 100 - ( int ) ( ( 100 * theHeader->dataLen ) / theHeader->fileLen );
	else
		ratio = 0;
#endif /* __MSDOS16__ */

	/* Set up the date fields */
	extractDate( theHeader->fileTime, &time1, &time2, &time3,
									  &hours, &minutes, &seconds );

	/* Display the data fields */
	for( index = 0; selectOrder[ index ] != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		switch( selectOrder[ index ] )
			{
			case SELECT_NAME:
				hputchar( ( cryptInfo == ( EXTRA_ENCRYPTED | EXTRA_SECURED ) ) ? '#' :
						  ( cryptInfo == EXTRA_ENCRYPTED ) ? '*' :
						  ( cryptInfo == EXTRA_SECURED ) ? '-' : ' ' );
				hprintf( "%-*s", nameFieldWidth,
						 formatFilename( fileInfoPtr->fileName, nameFieldWidth,
										 ( fileInfoPtr->extraInfo != NULL ) &&
										 ( *fileInfoPtr->extraInfo & EXTRA_UNICODE ) ) );
				break;

			case SELECT_DATE:
				hprintf( "%02d/%02d/%02d", time1, time2, time3 );
				break;

			case SELECT_TIME:
				hprintf( "%02d:%02d:%02d", hours, minutes, seconds );
				break;

			case SELECT_SIZE:
				hprintf( "%7lu", theHeader->dataLen );
				break;

			case SELECT_LENGTH:
				hprintf( "%7lu", theHeader->fileLen );
				break;

			case SELECT_SYSTEM:
				hprintf( "%-6s",
						 ( ( theHeader->archiveInfo & ARCH_SYSTEM ) >= OS_UNKNOWN ) ?
							systemName[ OS_UNKNOWN ] :
							systemName[ theHeader->archiveInfo & ARCH_SYSTEM ] );
				break;

			case SELECT_RATIO:
				hprintf( " %02d%% ", ratio );
				break;
			}
		}
	hputchar( '\n' );
	}

/* Display information on one directory */

static void showDirInfo( DIRHDRLIST *dirInfoPtr, int nameFieldWidth )
	{
	int time1, time2, time3, hours, minutes, seconds;
	int index, fieldType;

	extractDate( dirInfoPtr->data.dirTime, &time1, &time2, &time3, &hours, &minutes, &seconds );
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		switch( fieldType )
			{
			case SELECT_NAME:
				hprintf( "/%-*s", nameFieldWidth,
						 formatFilename( dirInfoPtr->dirName, nameFieldWidth,
										 dirInfoPtr->data.dirInfo & DIR_UNICODE ) );
				break;

			case SELECT_DATE:
				hprintf( "%02d/%02d/%02d", time1, time2, time3 );
				break;

			case SELECT_TIME:
				hprintf( "%02d:%02d:%02d", hours, minutes, seconds );
				break;

			default:
				fillField( ' ', fieldWidths[ fieldType ] - 1 );
			}
		}
	hputchar( '\n' );
	}

/* The size of the directory stack.  We should never have more than 50
   directories in one pathname */

#define DIRSTACK_SIZE	50

/* Display the path to a directory */

void showDirPath( WORD dirNo )
	{
	WORD dirStack[ DIRSTACK_SIZE ];
	int time1, time2, time3, hours, minutes, seconds;
	int stackIndex = 0;
	char *dirName;

	/* Print the directory info if necessary */
	if( dirNo )
		{
		hprintf( MESG_DIRECTORY );
		extractDate( getDirTime( dirNo ), &time1, &time2, &time3, &hours, &minutes, &seconds );

		/* Get chain of directories from root to current directory.  Some of
		   this code is duplicated in getPath(), but we can't use getPath()
		   since it munges the pathName into an OS-compatible format as it goes */
		do
			dirStack[ stackIndex++ ] = dirNo;
#pragma warn -pia
		while( ( dirNo = getParent( dirNo ) ) && stackIndex <= DIRSTACK_SIZE );
#pragma warn +pia

		/* Now print full path to current directory */
		while( stackIndex )
			{
			dirName = getDirName( dirStack[ --stackIndex ] );
			hprintf( "/%s", dirName );
			}

		/* Finally, print the directory date */
		hprintf( MESG_DIRECTORY_TIME, time1, time2, time3, hours, minutes, seconds );
		}

	/* Make sure we don't get a "Nothing to do" error if we elect to view
	   only (empty) directories */
	if( viewFlags & VIEW_DIRS )
		archiveChanged = TRUE;
	}

/* Static vars for tracking totals when multiple archives are used */

static LONG grandTotalLength = 0L, grandTotalSize = 0L;
static int grandTotalFiles = 0, totalArchives = 0;

/* Display a directory of files within an archive */

void listFiles( void )
	{
	FILEHDRLIST *fileInfoPtr;
	DIRHDRLIST *dirInfoPtr;
	FILEHDR theHeader;
	LONG totalLength = 0L, totalSize = 0L;
	int fileCount = 0, totalRatio;
	WORD oldFlags, hType, count;
	BOOLEAN showFile, dirNameShown;
	int nameFieldWidth = screenWidth - 1, index, fieldType;
	int maxFilenameLen = 0;

	/* Find the longest filename field */
	for( fileInfoPtr = getFirstFile( fileHdrStartPtr ); fileInfoPtr != NULL;
		 fileInfoPtr = getNextFile() )
		if( fileInfoPtr->tagged )
			{
			/* Count this file header and check whether we've got a new
			   longest name */
			fileCount++;
			if( strlen( fileInfoPtr->fileName ) > maxFilenameLen )
				maxFilenameLen = strlen( fileInfoPtr->fileName );
			}

	/* Find out how much space we can give the filename field */
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		if( fieldType != SELECT_NAME )
			nameFieldWidth -= fieldWidths[ fieldType ];
	index = strlen( getCase( fileCount ) ) + ( ( fileCount > 99 ) ? 4 :
											   ( fileCount > 9 ) ? 3 : 2 );
	if( ( index < maxFilenameLen ) && ( maxFilenameLen < nameFieldWidth ) )
		nameFieldWidth = maxFilenameLen;
	else
		if( index < nameFieldWidth )
			nameFieldWidth = index;

	/* Turn on stealth mode to disable printing of any extraneous noise
	   while extracting data.  This is safe since the view options don't
	   check for stealth mode */
	flags |= STEALTH_MODE;

	/* Print a newline if there is an archive listing preceding this one */
	if( totalArchives )
		hputchar( '\n' );

	/* Display the title fields */
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		if( fieldType == SELECT_NAME )
			hputchar( ' ' );	/* Add space for file type char */
		hprintf( "%-*s", ( fieldType == SELECT_NAME ) ?
						 nameFieldWidth : fieldWidths[ fieldType ] - 1,
						 *( &VIEWFILE_FIELD1 + fieldType ) );
		}
	hputchar( '\n' );

	/* Display the upper underscore fields */
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		if( fieldType == SELECT_NAME )
			hputchar( ' ' );	/* Add space for file type char */
		fillField( '-', ( fieldType == SELECT_NAME ) ?
						nameFieldWidth : fieldWidths[ fieldType ] - 1 );
		}
	hputchar( '\n' );

	for( count = getFirstDir(); count != END_MARKER; count = getNextDir() )
		{
		/* First handle any special entries (if there are any and provided
		   it's not a multipart archive) */
		if( ( fileInfoPtr = getFirstFileEntry( count ) ) != NULL &&
			!( flags & MULTIPART_ARCH ) )
			do
				{
				/* Check whether this is an archive comment file, unless
				   we've been asked to display comment files only */
				if( !( flags & ARCH_COMMENT ) &&
					( ( hType = fileInfoPtr->hType ) == TYPE_COMMENT_TEXT ||
						hType == TYPE_COMMENT_ANSI ) )
					{
					/* Move to the comment data */
					theHeader = fileInfoPtr->data;
					vlseek( fileInfoPtr->offset, SEEK_SET );
					resetFastIn();
					oldFlags = flags;

					/* Set up any handlers we need for it */
					switch( hType )
						{
						case TYPE_COMMENT_TEXT:
							/* Set up system for text output */
							setOutputIntercept( OUT_FMT_TEXT );
							if( !( flags & XLATE_OUTPUT ) )
								{
								/* Turn on smart translation if none is
								   specified */
								flags |= XLATE_OUTPUT;
								xlateFlags = XLATE_SMART;
								}
							break;

						case TYPE_COMMENT_ANSI:
							/* Output it raw (assumes ANSI driver) */
							setOutputFD( STDOUT );
							break;

						default:
							/* Don't know what to do with it, send it to the
							   bit bucket */
							setOutputIntercept( OUT_NONE );
						}

					/* Call extractData() to move the comment to wherever
					   it's meant to go */
					extractData( theHeader.archiveInfo, ( const BYTE )
								 ( ( fileInfoPtr->extraInfo == NULL ) ? 0 :
										*fileInfoPtr->extraInfo ),
								 theHeader.dataLen, theHeader.fileLen, FALSE );

					/* Reset status of output handlers */
					flags = oldFlags;
					resetOutputIntercept();
					archiveChanged = TRUE;
					}
				}
			while( ( fileInfoPtr = getNextFileEntry() ) != NULL );

		dirNameShown = FALSE;	/* Haven't printed dir.name yet */

		/* First print any subdirectories */
		if( !( viewFlags & VIEW_FILES ) )
			if( ( dirInfoPtr = getFirstDirEntry( count ) ) != NULL )
				do
					if( getDirTaggedStatus( dirInfoPtr->dirIndex ) )
						{
						/* Print the path to the directory if necessary */
						if( !dirNameShown )
							{
							showDirPath( count );
							dirNameShown = TRUE;
							}

						showDirInfo( dirInfoPtr, nameFieldWidth );
						archiveChanged = TRUE;
						}
				while( ( dirInfoPtr = getNextDirEntry() ) != NULL );

		/* Then print the files in the directory */
		if( !( viewFlags & VIEW_DIRS ) )
			{
			/* Sort the files if necessary */
			if( viewFlags & VIEW_SORTED )
				sortFiles( count );

			if( ( fileInfoPtr = getFirstFileEntry( count ) ) != NULL )
				do
					{
					/* Exclude either special-format files by default, or non-comment
					   files if asked to display only comment files */
					if( flags & ARCH_COMMENT )
						showFile = ( hType = fileInfoPtr->hType ) == TYPE_COMMENT_TEXT ||
									 hType == TYPE_COMMENT_ANSI;
					else
						showFile = ( !( fileInfoPtr->data.archiveInfo & ARCH_SPECIAL ) );

					if( showFile && fileInfoPtr->tagged )
						{
						/* Print the path to the directory if necessary */
						if( !dirNameShown )
							{
							showDirPath( count );
							dirNameShown = TRUE;
							}

						archiveChanged = TRUE;	/* We've accessed the archive */
						showFileInfo( fileInfoPtr, nameFieldWidth );
						totalLength += fileInfoPtr->data.fileLen;
						totalSize += fileInfoPtr->data.dataLen;
						}
					}
				while( ( fileInfoPtr = getNextFileEntry() ) != NULL );
			}
		}

	/* Print summary of file info */
#ifdef __MSDOS16__
	totalRatio = getRatio( totalLength, totalSize );
#else
	if( totalLength && totalLength > totalSize )
		totalRatio = 100 - ( int ) ( ( 100 * totalSize ) / totalLength );
	else
		totalRatio = 0;
#endif /* __MSDOS16__ */

	/* Display the lower underscore fields */
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		if( fieldType == SELECT_NAME )
			hputchar( ' ' );	/* Add space for file type char */
		fillField( hasBottomLine[ fieldType ] ? '-' : ' ',
				   ( fieldType == SELECT_NAME ) ?
						nameFieldWidth : fieldWidths[ fieldType ] - 1 );
		}
	hputchar( '\n' );

	/* Display the totals fields */
	for( index = 0; ( fieldType = selectOrder[ index ] ) != SELECT_END; index++ )
		{
		if( index )
			hputchar( ' ' );
		if( !hasBottomLine[ fieldType ] )
			fillField( ' ', fieldWidths[ fieldType ] - 1 );
		else
			switch( fieldType )
				{
				case SELECT_NAME:
					hprintf( " %u %s", fileCount,
							 formatFilename( getCase( fileCount ),
											 nameFieldWidth, FALSE ) );
					break;

				case SELECT_SIZE:
					hprintf( "%7lu", totalSize );
					break;

				case SELECT_LENGTH:
					hprintf( "%7lu", totalLength );
					break;

				case SELECT_RATIO:
					hprintf( " %02d%% ", totalRatio );
					break;
				}
		}
	hputchar( '\n' );

	/* Update grand totals */
	grandTotalLength += totalLength;
	grandTotalSize += totalSize;
	grandTotalFiles += fileCount;
	totalArchives++;

	/* Turn stealth mode off again */
	flags &= ~STEALTH_MODE;
	}

/* Print summary of archives viewed if necessary */

void showTotals( void )
	{
	int grandTotalRatio;

	/* Print totals if there was more than one archive */
	if( totalArchives > 1 )
		{
		/* Evaluate total compression ratio for all archives */
#ifdef __MSDOS16__
		grandTotalRatio = getRatio( grandTotalLength, grandTotalSize );
#else
		if( grandTotalLength && grandTotalLength > grandTotalSize )
			grandTotalRatio = 100 - ( int )
							  ( ( 100 * grandTotalSize ) / grandTotalLength );
		else
			grandTotalRatio = 0;
#endif /* __MSDOS16__ */

		hprintf( VIEWFILE_TOTAL_LINE1, totalArchives, grandTotalFiles,
				 getCase( grandTotalFiles ), grandTotalRatio );
		hprintf( VIEWFILE_TOTAL_LINE2, grandTotalSize, grandTotalLength );
		}
	}
