/****************************************************************************
*																			*
*						  HPACK Multi-System Archiver						*
*						  ===========================						*
*																			*
*					 File Archive/Dearchive Handling Routines				*
*						  ARCHIVE.C  Updated 27/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  				  and if you do so there will be....trubble.				*
* 			  And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* "One archiver to rule them all; one archiver to find them.
	One archiver to bring them all and in the darkness bind them.
	In the land of madness where the HPACK lies"
					-- Apologies to JRRT */

#include <ctype.h>
#include <stdio.h>
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
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
  #include "store\store.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
  #include "store.h"
#else
  #include "crc/crc16.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
  #include "store/store.h"
#endif /* System-specific include directives */

/* Prototypes for compression/decompression functions */

long pack( BOOLEAN *isText, long noBytes );
BOOLEAN unpack( const BOOLEAN isText, const long coprLen, const long uncoprLen );

/* Prototypes for functions in GUI.C */

void initProgressReport( const BOOLEAN fileNameTruncated, const char *oldFileName,
						 const char *fileName, const LONG fileLen );
void endProgressReport( void );
BOOLEAN confirmSkip( const char *str1, const char *str2, const BOOLEAN str1Filename );

#ifdef __AMIGA__

/* Prototypes for functions in AMIGA.C */

LONG storeDiskObject( const char *pathName, const FD outFD );
void setIcon( const char *filePath, const TAGINFO *tagInfo, const FD srcFD );
int storeComment( const FILEINFO *fileInfo, const FD outFD );
void setComment( const char *filePath, const TAGINFO *tagInfo, const FD srcFD );

#endif /* __AMIGA__ */
#ifdef __ARC__

/* Prototypes for functions in ARC.C */

void setFileType( const char *filePath, const WORD type );

#endif /* __ARC__ */
#ifdef __MAC__

/* Prototypes for functions in MAC.C */

extern BOOLEAN doCryptOut;		/* Defined in FASTIO.C */

void setFileType( const char *filePath, const LONG type, const LONG creator );
void setBackupTime( const char *filePath, const LONG timeStamp );
void setCreationTime( const char *filePath, const LONG timeStamp );
void setVersionNumber( const char *filePath, const BYTE versionNumber );
FD openResourceFork( const char *filePath, const int mode );
void closeResourceFork( const FD theFD );

#endif /* __MAC__ */
#ifdef __MSDOS16__

/* Prototypes for functions in MISC.ASM */

void printFilePath( const char *filePath, int padding );
void getFileName( char *fileName );

/* Prototype for isDevice() routine */

BOOLEAN isDevice( const FD theFD );

#endif /* __MSDOS16__ */
#ifdef __OS2__

/* Prototypes for functions in OS2.C */

void setAccessTime( const char *filePath, const LONG timeStamp );
void setCreationTime( const char *filePath, const LONG timeStamp );
void addLongName( const char *filePath, const char *longName );
LONG storeEAinfo( const BOOLEAN isFile, const char *pathName, const LONG eaSize, const FD outFD );
void setEAinfo( const char *filePath, const TAGINFO *tagInfo, const FD srcFD );

#endif /* __OS2__ */

/* The following is defined in FASTIO.C */

extern int cryptMark;

/* Various defines */

#define MIN_FILESIZE	50	/* Any files of below this length are stored,
							   since it is not worth trying to compress
							   them */

/* A flag to indicate whether we overwrite the existing file entry at currPos
   or whether we add a new one via addFileHeader().  This is set for the
   FRESHEN, REPLACE, and UPDATE commands */

BOOLEAN overWriteEntry;

/* The suffix to use for a filename when we use smart overwrite */

#ifdef __ARC__
  char AUTO_SUFFIX[] = "_000";
#else
  char AUTO_SUFFIX[] = ".000";
#endif /* __ARC__ */

/****************************************************************************
*																			*
*								General Work Functions						*
*																			*
****************************************************************************/

/* Symbolic defines used when skipping a file */

#define STR1_FILENAME	TRUE
#define STR2_FILENAME	FALSE

#ifndef GUI

/* Blank out a line of text */

void blankLine( int length )
	{
	hputchar( '\r' );
	while( length-- )
		hputchar( ' ' );
	hputchar( '\r' );
	}

/* Blank out a block of characters */

void blankChars( int length )
	{
	int i;

	for( i = 0; i < length; i++ )
		hputchars( '\b' );
	for( i = 0; i < length; i++ )
		hputchars( ' ' );
	for( i = 0; i < length; i++ )
		hputchars( '\b' );
	}

/* Handle an idiot box - either return TRUE or FALSE to the caller, or
   exit if response is FALSE */

BOOLEAN confirmAction( const char *message )
	{
	char ch;

	hprintf( MESG_s_YN, message );
	hflush( stdout );
	while( ( ch = hgetch(), ch = toupper( ch ) ) != RESPONSE_YES &&
											  ch != RESPONSE_NO );
	hputchar( ch );
	hputchar( '\n' );
	return( ch == RESPONSE_YES );
	}

void idiotBox( const char *message )
	{
	char ch;

	hprintf( WARN_s_CONTINUE_YN, message );
	hflush( stdout );
	while( ( ch = hgetch(), ch = toupper( ch ) ) != RESPONSE_YES &&
											  ch != RESPONSE_NO );
	hputchar( ch );
	hputchar( '\n' );
	if( ch == RESPONSE_NO )
		{
		errorFD = IO_ERROR;	/* Make sure we don't delete archive by mistake */
		error( STOPPED_AT_USER_REQUEST );
		}
	hputchar( '\n' );
	}

/* Print dots to show the length of a file.  The most elegant way to
   backspace over the dots would be to use ANSI codes, but not all systems
   have ANSI drivers.  Filenames are padded out with spaces to make them
   align nicely */

#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  #define PADDING		12			/* 8.3 */
#elif defined( __AMIGA__ ) || defined( __MAC__ ) || defined( __OS2__ ) || defined( __UNIX__ )
  #define PADDING		16			/* Pad out to at least 16 chars */
#elif defined( __ARC__ )
  #define PADDING		10			/* 10-char filenames */
#else
  #error "Need to define filename length padding in ARCHIVE.C"
#endif /* Various OS-specific defines */

static void printFileName( const char *fileName, int charsUsed,
						   const long fileSize, const BOOLEAN printDots )
	{
	int i, length, noDots;

	/* Quick check for no printing */
	if( flags & STEALTH_MODE )
		return;

	hputchar( ' ' );

	/* Determine statistics for filename.  The amount added to charsUsed
	   represents the length of ' ' + path + SLASH (if used) + filename with
	   any necessary padding added + ' '.  noDots is calculated as one dot
	   for every 4K (2^12) bytes of data */
	for( i = strlen( fileName ); i && fileName[ i ] != SLASH; i-- );
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ )
	charsUsed += ( i ? i + 1 : 0 ) + PADDING + 2;
#elif defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __MAC__ ) || defined( __UNIX__ )
	charsUsed += ( i ? i + 1 : 0 ) + 3 +
				 ( ( strlen( fileName + i ) > PADDING ) ? strlen( fileName + i ) : PADDING );
#elif defined( __OS2__ )
	charsUsed += ( i ? i + 1 : 0 ) + 2 +
				 ( ( strlen( fileName + i ) > PADDING ) ? strlen( fileName + i ) : PADDING );
#else
	hprintf( "Need to set up handling of filename padding in ARCHIVE.C, line %d\n", __LINE__ );
#endif /* OS-specific padding handling */
	length = ( screenWidth - charsUsed ) > 0 ? screenWidth - charsUsed : 0;
	noDots = ( int ) ( fileSize >> 12 ) > length ?
			 length : ( int ) ( ( fileSize + 2047 ) >> 12 );

#ifdef __MSDOS16__
	printFilePath( fileName, i );
#else
	hprintf( "%s", fileName );	/* Filename may contain '%' char */
	for( i = strlen( fileName ); i < 16; i++ )
		hputchar( ' ' );
	hputchar( ' ' );		/* Always print at least one space */
#endif /* __MSDOS16__ */

	/* Print dots corresponding to the file length if required, otherwise
	   print a CRLF */
	if( printDots )
		{
		for( i = 0; i < noDots; i++ )
			hputchar( '.' );
		while( i-- > 0 )
			hputchar( '\b' );
		}
	else
		hputchar( '\n' );
	}

/* Confirm skipping a file */

BOOLEAN confirmSkip( const char *str1, const char *str2, const BOOLEAN str1Filename )
	{
	char ch;

	/* Ask user for skip choice.  Note that we use a hgetch() and not a
	   hgetchar(), since hgetchar() requires a CR which is spuriously echoed */
	hprintf( MESG_s_s_YNA, str1, str2 );
	hflush( stdout );
	while( ( ch = hgetch(), ch = toupper( ch ) ) != RESPONSE_YES &&
							ch != RESPONSE_NO && ch != RESPONSE_ALL );
	hputchar( ch );

	/* Blank the current line */
	blankLine( screenWidth - 1 );

	if( ch == RESPONSE_NO )
		{
		/* Indicate file is being skipped */
		hprintf( MESG_SKIPPING_s, ( str1Filename ) ? str1 : str2 );
		return( TRUE );
		}

	if( ch == RESPONSE_ALL )
		{
		/* Do an automatic "Confirm" for all following files */
		if( flags & INTERACTIVE )
			flags &= ~INTERACTIVE;
		else
			overwriteFlags |= OVERWRITE_ALL;
		}

	return( FALSE );
	}

#ifndef __MSDOS16__

/* Get a filename from the luser */

void getFileName( char *fileName )
	{
	hprintf( MESG_s_ALREADY_EXISTS_ENTER_NEW_NAME, fileName );

	/* Nasty input routine - should check for illegal chars and suchlike.
	   Will also overflow if anyone enters more than 16K chars */
	hflush( stdout );
	hgets( ( char * ) mrglBuffer );
	mrglBuffer[ MAX_PATH - 1 ] = '\0';
	strcpy( fileName, ( char * ) mrglBuffer );

	/* Clean up screen */
	blankLine( screenWidth - 1 );
	}
#endif /* !__MSDOS16__ */
#endif /* !GUI */

/* Functions to handle files which didn't extract/test properly.  At the
   moment we just print a summary of any problems, we could do more
   sophisticated handling such as tagging them for non-deletion on move etc.
   This would entail keeping track of skipped files as well as corrupted ones */

int badFileCount;

void initBadFileInfo( void )
	{
	/* Reset count of bad files */
	badFileCount = 0;
	}

void processBadFileInfo( void )
	{
	if( badFileCount )
		{
#ifdef GUI
		char string[ 10 ];

		itoa( badFileCount, string, 10 );
		alert( ALERT_FILES_CORRUPTED, string );
#else
		if( badFileCount == 1 )
			hputs( WARN_FILE_CORRUPTED );
		else
			hprintf( WARN_d_s_CORRUPTED, badFileCount, getCase( badFileCount ) );
#endif /* GUI */
		}
	}

/****************************************************************************
*																			*
*							Extract Data from an Archive					*
*																			*
****************************************************************************/

/* The following variables are visible to both extractData() and retrieveData().
   This is necessary since they are useful only for retrieveData() calls to
   extractData() */

BOOLEAN fileNameTruncated;		/* Whether filename has been munged for OS */
char *oldFileName, *fileName;	/* Original and munged filename */

/* The following variables are needed for the create-on-write facility.  This
   is necessary because the output file must be opened at the last possible
   moment, when there is no more chance of it being skipped.  This avoids the
   case where the output file is unnecessarily overwritten when the data is
   skipped.  Note that we need to statically set this to FALSE since
   extractData() may be accessed by external procedures which don't know about
   the createOnWrite facility */

BOOLEAN createOnWrite = FALSE;	/* Whether file needs to be created first */
char *filePath;					/* Name of output file */

/* Possible return types after data has been extracted:  Corrupted data,
   OK data, data which has been skipped since the storage/encryption mode is
   unknown, and data which must be resynchronised since there was some non-
   data corruption (eg in the encryption information) without the usual
   "Bad data" warnings */

typedef enum { DATA_BAD, DATA_OK, DATA_SKIPPED, DATA_RESYNC } DATA_STATUS;

/* General-purpose data extraction handler */

DATA_STATUS extractData( const WORD dataInfo, const BYTE extraInfo,
						 LONG dataLen, const LONG fileLen, const BOOLEAN isFile )
	{
	WORD oldFlags = flags, oldXlateFlags = xlateFlags;
#ifndef GUI
	int charsUsed = 10;		/* Length of "Extracting" */
#endif /* !GUI */
	BOOLEAN dataOK = TRUE, isText, dataSkipped = FALSE;
	int cryptInfoLength;
	FD outputFD = IO_ERROR;

	/* Set/clear encryption status depending on whether the data needs to be
	   decrypted or not, or complain if no password is given for private-key
	   encrypted data */
	if( extraInfo & EXTRA_ENCRYPTED )
		if( ( dataInfo & ARCH_EXTRAINFO ) && !( flags & CRYPT ) )
			{
			if( isFile )
#ifdef GUI
				alert( ALERT_DATA_ENCRYPTED, fileName );
#else
				hprintf( MESG_DATA_IS_ENCRYPTED );
#endif /* GUI */
			goto skipData;
			}
		else
			/* Reset encryption system for this block of data */
			if( extraInfo & EXTRA_ENCRYPTED )
				{
				if( !cryptSetInput( dataLen, &cryptInfoLength ) )
					{
					if( isFile )
#ifdef GUI
						alert( ALERT_CANNOT_PROCESS_ENCR_INFO, NULL );
#else
						hprintf( MESG_CANNOT_PROCESS_ENCR_INFO );
#endif /* GUI */

					if( cryptInfoLength == 0 )
						/* Encryption info corrupted, force error recovery */
						dataOK = FALSE;
					else
						/* Can't process encryption info, skip rest of data */
						dataLen -= cryptInfoLength;

					goto skipData;
					}

				/* Adjust data length by length of encryption information packet */
				dataLen -= cryptInfoLength;
				}

	/* Set the appropriate translation mode if necessary.  These are
	   different for each OS */
	if( ( flags & XLATE_OUTPUT ) && ( xlateFlags & XLATE_SMART ) )
		if( dataInfo & ARCH_ISTEXT )
			switch( dataInfo & ARCH_SYSTEM )
				{
#if !defined( __AMIGA__ ) && !defined( __ARC__ ) && !defined( __UNIX__ )
				case OS_AMIGA:
				case OS_ARCHIMEDES:
				case OS_UNIX:
					xlateFlags = XLATE_EOL;
					initTranslationSystem( '\n' );
					break;
#endif /* !( __AMIGA__ || __ARC__ || __UNIX__ ) */

/*				case OS_IBM:
					xlateFlags = XLATE_EBCDIC;
					break; */

#if !defined( __MAC__ )
				case OS_MAC:
					xlateFlags = XLATE_EOL;
					initTranslationSystem( '\r' );
					break;
#endif /* !__MAC__ */

#if !( defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	   defined( __MSDOS32__ ) || defined( __OS2__ ) )
				case OS_ATARI:
				case OS_MSDOS:
				case OS_OS2:
					xlateFlags = XLATE_EOL;
					initTranslationSystem( '\r' | 0x80 );
					break;
#endif /* !( __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

				case OS_PRIMOS:
					xlateFlags = XLATE_PRIME;
					break;
				}
		else
			/* No translation for non-text files */
			xlateFlags = 0;

	if( ( dataInfo & ARCH_STORAGE ) < FORMAT_UNKNOWN )
		{
		if( isFile )
			{
			/* Create the output file if necessary.  We do this at the last
			   possible moment since if the data is skipped for any reason
			   we might (unnecessarily) overwrite an existing file */
			if( createOnWrite )
				{
				if( ( outputFD = hcreat( filePath, CREAT_ATTR ) ) == IO_ERROR )
					{
#ifdef GUI
					alert( ALERT_CANNOT_OPEN_DATAFILE, fileName );
#else
					hprintf( MESG_CANNOT_OPEN_DATAFILE );
#endif /* GUI */
					goto skipData;
					}
				errorFD = outputFD;
				setOutputFD( outputFD );
				}

#ifdef GUI
			initProgressReport( fileNameTruncated, oldFileName, fileName, fileLen );
#else
			hprintfs( MESG_EXTRACTING );
			if( fileNameTruncated )
				{
				hprintfs( MESG_s_AS, oldFileName );
				charsUsed += 4 + strlen( oldFileName );
				}
			printFileName( fileName, charsUsed, fileLen, choice != DISPLAY );
#endif /* GUI */
			}

		/* Make sure we don't print any dits during the extraction by
		   turning on stealth mode */
		if( choice == DISPLAY || !isFile )
			flags |= STEALTH_MODE;

		isText = ( dataInfo & ARCH_ISTEXT ) ? TRUE : FALSE;

		switch( dataInfo & ARCH_STORAGE )
			{
			case FORMAT_STORED:
				/* Don't bother with zero-length files */
				if( dataLen )
					dataOK = unstore( dataLen );
				else
					/* Can't corrupt a zero-length file */
					dataOK = TRUE;
				break;

			case FORMAT_PACKED:
				dataOK = unpack( isText, dataLen, fileLen );
				firstFile = FALSE;
				break;
			}
		}
	else
		{
		if( isFile )
#ifdef GUI
			alert( ALERT_UNKNOWN_ARCH_METHOD, fileName );
#else
			hprintf( MESG_UNKNOWN_ARCHIVING_METHOD );
#endif /* GUI */
skipData:
#ifndef GUI
		if( isFile )
			hprintf( MESG__SKIPPING_s, fileName );
#endif /* GUI */
		skipDist += dataLen;
		dataSkipped = TRUE;
		}

	/* Reset various options */
	createOnWrite = FALSE;
	flags = oldFlags;
	xlateFlags = oldXlateFlags;

	/* Close the output FD if necessary */
	if( outputFD != IO_ERROR )	/* outputFD may not have been created */
		hclose( outputFD );
	errorFD = IO_ERROR;	/* We no longer need to delete the extracted file on error */

	return( dataSkipped ? ( !dataOK ? DATA_RESYNC : DATA_SKIPPED ) :
						  ( DATA_STATUS ) dataOK );
	}

/* Retrieve data from the archive file to a data file */

void retrieveData( FILEHDRLIST *fileInfo )
	{
	FILEHDR *theHeader = &fileInfo->data;
	BYTE *extraInfoPtr = fileInfo->extraInfo;
	char *filePtr, *suffixPtr, ch;
	int iteration, i;
	long auxDataLen = theHeader->auxDataLen;
	DATA_STATUS dataStatus;
#ifndef GUI
	BOOLEAN printNL = TRUE;
#endif /* !GUI */
	FD dataFD;
	TAGINFO tagInfo;
#ifdef __UNIX__
	struct utimbuf timeStamp;
#endif /* __UNIX__ */

	/* Flag the fact that we've accessed the archive */
	archiveChanged = TRUE;

	/* Don't create the file by default when we try to extract it */
	createOnWrite = FALSE;

	fileNameTruncated = FALSE;
	fileName = buildInternalPath( fileInfo );
	if( choice == EXTRACT )
		{
		filePath = buildExternalPath( fileInfo );
		fileNameTruncated = isTruncated();

		/*  Make sure we don't overwrite an existing file unless the
			OVERWRITE_ALL option has been specified */
		if( ( dataFD = hopen( filePath, O_RDONLY | S_DENYNONE ) ) != IO_ERROR )
			{
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
			/* Check if the file corresponds to a device name */
			if( sysSpecFlags & SYSPEC_CHECKSAFE )
				if( isDevice( dataFD ) )
					{
#ifdef GUI
					alert( ALERT_FILE_IS_DEVICEDRVR, NULL );
#else
					hprintf( OSMESG_FILE_IS_DEVICEDRVR );
#endif /* GUI */
					if( !( overwriteFlags & OVERWRITE_PROMPT ) )
						{
						/* Don't extract without user intervention */
#ifndef GUI
						hprintf( MESG__SKIPPING_s, fileName );
#endif /* !GUI */
						skipDist += theHeader->dataLen + auxDataLen;
						hclose( dataFD );
						return;
						}
#ifndef GUI
					hputchar( '\n' );
#endif /* !GUI */
					}
#endif /* __MSDOS16__ || __MSDOS32__ */

			hclose( dataFD );

			if( !( overwriteFlags & OVERWRITE_ALL ) )
				{
				/* Check whether we want a forced skip of this file */
				if( overwriteFlags & OVERWRITE_NONE )
					{
					ch = RESPONSE_NO;
#ifdef GUI
					alert( ALERT_NO_OVERWRITE_EXISTING, fileName );
#else
					hprintf( MESG_WONT_OVERWRITE_EXISTING_s, fileName );
#endif /* GUI */
					}
				else
					{
					/* Find the filename component of the filePath */
					filePtr = findFilenameStart( filePath );

					if( overwriteFlags & OVERWRITE_SMART )
						{
						/* Find either the '.' in the filename or the end of
						   the filename.  In either case we stop after
						   the MAX_FILENAME - 5th char to allow room for the
						   extension */
						for( i = 0; *filePtr && *filePtr != '.' && i < MAX_FILENAME - 5;
							 i++, filePtr++ );
#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __ATARI__ ) || \
	defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ )
						for( iteration = strlen( fileName ) + 1; iteration &&
							 fileName[ iteration - 1 ] != SLASH &&
							 fileName[ iteration - 1 ] != ':'; iteration-- );
#else
						for( iteration = strlen( fileName ) + 1; iteration &&
							 fileName[ iteration - 1 ] != SLASH; iteration-- );
#endif /* OS-specific pathname parsing */
						suffixPtr = fileName + iteration + i;
						if( filePtr - filePath > MAX_PATH - 5 )
							/* No room for extension, panic */
							error( PATH_s_TOO_LONG, filePath );

						/* Now try and open a new file with a numerical
						   suffix.  Increment the suffix until we can open
						   the file */
						iteration = 0;
						strcpy( filePtr, AUTO_SUFFIX );
						while( ( dataFD = hopen( filePath, O_RDONLY | S_DENYNONE ) ) != IO_ERROR )
							{
							hclose( dataFD );

							/* Set the suffix to the iteration count */
							strcpy( filePtr, AUTO_SUFFIX );
							i = iteration++;
							filePtr[ 1 ] += i / 100;
							i %= 100;
							filePtr[ 2 ] += i / 10;
							i %= 10;
							filePtr[ 3 ] += i;
							}

						/* Finally, update the fileName to reflect the changes
						   to the filePath */
						strcpy( suffixPtr, filePtr );

						/* We have now found a unique filename which we can use */
						fileNameTruncated = TRUE;	/* Force printing of original name */
						ch = RESPONSE_YES;
						}
					else
						if( overwriteFlags & OVERWRITE_PROMPT )
							{
							while( ( dataFD = hopen( filePath, O_RDONLY | S_DENYNONE ) ) != IO_ERROR )
								{
								hclose( dataFD );

								/* Call B&D filename input routine */
								getFileName( fileName );

								/* Do a path length check */
								i = MAX_PATH - strlen( filePath ) - 1;	/* -1 is for '\0' */
								if( strlen( fileName ) > i )
									{
									/* Path too long, truncate and issue warning */
									strncpy( filePtr, fileName, i );
									filePtr[ i ] = '\0';
#ifdef GUI
									alert( ALERT_PATH_TOO_LONG, filePath );
#else
									hprintf( MESG_PATH_s__TOO_LONG, filePath );
#endif /* GUI */
									}
								else
									/* Copy name across */
									strcpy( filePtr, fileName );
								}
							fileNameTruncated = TRUE;	/* Force printing of original name */
							ch = RESPONSE_YES;
							}
						else
							ch = ( confirmSkip( filePath, MESG_ALREADY_EXISTS__OVERWRITE,
												STR1_FILENAME ) ) ? RESPONSE_NO : RESPONSE_YES;
					}

				if( ch == RESPONSE_NO )
					{
					skipDist += theHeader->dataLen + auxDataLen;
					return;
					}
				}
			}
		else
			if( flags & INTERACTIVE )
				/* Ask the user whether they want to extract this file */
				if( confirmSkip( MESG_EXTRACT, fileName, STR2_FILENAME ) )
					{
					skipDist += theHeader->dataLen + auxDataLen;
					return;
					}

		/* Flag the fact that the file must be created when we're ready to
		   write to it */
		createOnWrite = TRUE;

		if( fileNameTruncated )
			oldFileName = fileInfo->fileName;

		/* Set the file to delete in case of error to the unarchived file */
		strcpy( errorFileName, filePath );
		}
	else
		{
		if( flags & INTERACTIVE )
			/* Ask the user whether they want to handle this file */
			if( confirmSkip( ( choice == DISPLAY ) ? MESG_DISPLAY : MESG_TEST,
							 fileName, STR2_FILENAME ) )
				{
				skipDist += theHeader->dataLen + auxDataLen;
				return;
				}

		setOutputFD( STDOUT );
		if( choice == TEST )
			setOutputIntercept( OUT_NONE );
		}

	/* Extract the data */
	dataStatus = extractData( theHeader->archiveInfo, ( const BYTE )
							  ( ( extraInfoPtr == NULL ) ? 0 : *extraInfoPtr ),
							  theHeader->dataLen, theHeader->fileLen, TRUE );

	/* Check the file has not been corrupted */
	if( dataStatus == DATA_BAD )
		{
		/* A klane korruption.... */
#ifdef GUI
		alert( ALERT_DATA_CORRUPTED, NULL );
#else
		hprintf( WARN_FILE_PROBABLY_CORRUPTED );
#endif /* GUI */
		badFileCount++;
		}

	if( choice == EXTRACT || choice == TEST )
		{
#ifndef GUI
		if( choice == TEST && dataStatus == DATA_OK )
			hprintf( MESG_FILE_TESTED_OK );
#endif /* GUI */

		/* Perform a data authentication check if necessary */
		if( ( theHeader->archiveInfo & ARCH_EXTRAINFO ) &&
			( *extraInfoPtr & EXTRA_SECURED ) && ( dataStatus == DATA_OK ) )
			{
#ifndef GUI
			hputchar( '\n' );
#endif /* !GUI */
			readTag( &auxDataLen, &tagInfo );
			checkSignature( fileInfo->offset, theHeader->dataLen );
#ifndef GUI
			printNL = FALSE;
#endif /* !GUI */
			}

		if( choice == TEST )
			{
			resetOutputIntercept();
			skipDist += auxDataLen;
			}
		else
			{
			/* Give files their real date if necessary */
			if( !( flags & TOUCH_FILES ) )
				setFileTime( filePath, theHeader->fileTime );

			if( dataStatus == DATA_SKIPPED || dataStatus == DATA_RESYNC )
				{
				/* File was never extracted, remove it */
				hunlink( filePath );

				if( dataStatus == DATA_SKIPPED )
					{
					/* Skip auxData and exit */
					skipDist += auxDataLen;
#ifndef GUI
					hputchar( '\n' );
#endif /* !GUI */
					return;
					}
				}

#ifdef __MAC__
			/* If we're extracting a file and don't chose to restore attribute
			   information, we still need to grovel through the auxData field
			   in case a resource fork is present */
			if( ( dataStatus != DATA_RESYNC ) && !( flags & STORE_ATTR ) )
				{
				/* Grovel through the auxData field looking for a RESOURCE_FORK tag */
				while( auxDataLen > 0 )
					if( ( readTag( &auxDataLen, &tagInfo ) == TAGCLASS_MISC ) &&
						( tagInfo.tagID == TAG_RESOURCE_FORK ) )
						/* Extract the resource data.  If we get an error,
						   move to DATA_RESYNC state.  Maybe we should remove
						   the file as well */
						switch( tagInfo.dataFormat )
							{
							case TAGFORMAT_STORED:
								if( !unstore( tagInfo.dataLength ) )
									auxDataLen = ERROR;
								break;

							case TAGFORMAT_PACKED:
								if( !unpack( FALSE, tagInfo.dataLength, tagInfo.uncoprLength ) )
									auxDataLen = ERROR;
								break;

							default:
								skipSeek( tagInfo.dataLength );
							}
						else
							skipSeek( tagInfo.dataLength );
				}
			else
#endif /* __MAC__ */

			/* Set the files' attributes etc properly if there is any extra
			   information recorded */
			if( ( dataStatus != DATA_RESYNC ) && ( flags & STORE_ATTR ) )
				{
				/* Grovel through the auxData field looking for tags we
				   can use */
				while( auxDataLen > 0 )
					switch( readTag( &auxDataLen, &tagInfo ) )
						{
						case TAGCLASS_ATTRIBUTE:
							/* Read in the attributes and set them */
							hchmod( filePath, readAttributeData( tagInfo.tagID ) );
							break;

#if defined( __MSDOS16__ )
						/* Fix compiler bug */
#elif defined( __AMIGA__ )
						case TAGCLASS_ICON:
							/* Check if it's an AMIGA_ICON tag */
							if( tagInfo.tagID == TAG_AMIGA_ICON )
								/* Set icon for file */
								setIcon( filePath, &tagInfo, archiveFD );
							else
								skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_COMMENT:
							setComment( filePath, &tagInfo, archiveFD );
							break;
#elif defined( __MAC__ )
						case TAGCLASS_TIME:
							/* Read in the file times and set them */
							if( tagInfo.tagID == TAG_BACKUP_TIME )
								/* Set file's backup time */
								setBackupTime( filePath, fgetLong() );
							else
								if( tagInfo.tagID == TAG_CREATION_TIME )
									/* Set file's creation time */
									setCreationTime( filePath, fgetLong() );
								else
									skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_MISC:
							if( tagInfo.tagID == TAG_RESOURCE_FORK )
								/* Extract the resource data.  If we get an
								   error, move to DATA_RESYNC state.  Maybe
								   we should remove the file as well */
								switch( tagInfo.dataFormat )
									{
									case TAGFORMAT_STORED:
										if( !unstore( tagInfo.dataLength ) )
											auxDataLen = ERROR;
										break;

									case TAGFORMAT_PACKED:
										if( !unpack( FALSE, tagInfo.dataLength, tagInfo.uncoprLength ) )
											auxDataLen = ERROR;
										break;

									default:
										skipSeek( tagInfo.dataLength );
									}
							else
								if( tagInfo.tagID == TAG_VERSION_NUMBER )
									setVersionNumber( filePath, ( BYTE ) fgetWord() );
								else
									skipSeek( tagInfo.dataLength );
							break;
#elif defined( __OS2__ )
						case TAGCLASS_TIME:
							/* Read in the file times and set them */
							if( tagInfo.tagID == TAG_ACCESS_TIME )
								/* Set file's access time */
								setAccessTime( filePath, fgetLong() );
							else
								if( tagInfo.tagID == TAG_CREATION_TIME )
									/* Set file's creation time */
									setCreationTime( filePath, fgetLong() );
								else
									skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_ICON:
							/* Set the icon EA if possible */
							if( tagInfo.tagID == TAG_OS2_ICON )
								setEAinfo( filePath, &tagInfo, archiveFD );
							else
								skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_MISC:
							/* Set the longname EA if possible */
							if( tagInfo.tagID  == TAG_LONGNAME ||
								tagInfo.tagID == TAG_OS2_MISC_EA )
								{
								setEAinfo( filePath, &tagInfo, archiveFD );
								break;
								}
							else
								skipSeek( tagInfo.dataLength );
							break;
#elif defined( __UNIX__ )
						case TAGCLASS_TIME:
							/* Read in the file times and set them */
							if( tagInfo.tagID == TAG_CREATION_TIME )
								{
								timeStamp.actime = fgetLong();
								timeStamp.modtime = theHeader->fileTime;
								utime( filePath, &timeStamp );
								}
							else
								skipSeek( tagInfo.dataLength );
							break;
#endif /* Various OS-dependant attribute reads */

						case TAGCLASS_NONE:
							/* Couldn't find a tag we could do anything with,
							   and we've run out of tags */
							break;

						default:
							/* Can't do anything with this tag, skip it */
							skipSeek( tagInfo.dataLength );
							break;
						}

				/* If the auxDataLen count has gone negative, the data has
				   been corrupted */
				if( auxDataLen < 0 )
					dataStatus = DATA_RESYNC;
				}
			else
				skipDist += auxDataLen;

			/* Set the file type for those OS's which store this */
#if defined( __MSDOS16__ )
			/* Fix compiler bug */
#elif defined( __ARC__ )
			findFileType( fileInfo );
			if( fileInfo->type )
				setFileType( filePath, fileInfo->type );
#elif defined( __IIGS__ )
			findFileType( fileInfo );
			if( fileInfo->type )
				setFileType( filePath, fileInfo->type, fileInfo->auxType );
#elif defined( __MAC__ )
			findFileType( fileInfo );
			if( fileInfo->type )
				setFileType( filePath, fileInfo->type, fileInfo->creator );
#endif /* Various OS-dependant file-type settings */

#ifdef __OS2__
			/* If we've truncated the filename, add the full name as an EA */
			if( fileNameTruncated )
				addLongName( filePath, oldFileName );
#endif /* __OS2__ */
			}
		}
	else
		{
#ifndef GUI
		hprintf( MESG_HIT_A_KEY );
		hflush( stdout );
		hgetch();	/* getchar() only terminates on CR */
#endif /* GUI */
		skipDist += auxDataLen;
		}

#ifdef GUI
	endProgressReport();
#else
	if( printNL )
		hputchars( '\n' );
#endif /* GUI */

	/* Attempt recovery if the data was corrupted */
	if( ( dataStatus == DATA_BAD || dataStatus == DATA_RESYNC ) &&
		  fileInfo->next != NULL )
		{
		/* Do an absolute seek to the start of the next data block, since we
		   may currently be anywhere in the current block, and reset input
		   data counters */
		vlseek( fileInfo->next->offset, SEEK_SET );
		forceRead();
		skipDist = 0L;
		}
	}

/****************************************************************************
*																			*
*								Add Data to an Archive						*
*																			*
****************************************************************************/

/* Add data to the archive file from a data file */

void addData( const char *filePath, const FILEINFO *fileInfoPtr,
			  const WORD dirIndex, const FD dataFD )
	{
	FILEHDR theHeader;
	long dataLength, auxDataLen = 0L, errorTagLen, startPosition;
	int comprMethod, i, cryptInfoLength = 0, secInfoLength;
	WORD hType = TYPE_NORMAL;
	BOOLEAN isText = FALSE;
	BYTE extraInfo;
#ifdef __UNIX__
	LONG linkID = ( ( LONG ) fileInfoPtr->statInfo.st_dev << 24 ) ^ fileInfoPtr->statInfo.st_ino;
/*	BOOLEAN isLink = checkForLink( linkID );	Not used yet */
#endif /* __UNIX__ */
#ifdef __MAC__
	FD savedInFD, resourceForkFD;
	int resourceComprMethod;
	BOOLEAN cryptOutSave, dummy;
	WORD cryptFlagSave;
	LONG byteCount;

	/* If there's a resource fork, try and establish a FD to it */
	if( fileInfoPtr->resSize &&
		( resourceForkFD = openResourceFork( filePath, O_RDONLY | S_DENYWR | A_SEQ ) ) == IO_ERROR )
		{
#ifdef GUI
		alert( ALERT_CANNOT_OPEN_DATAFILE, filePath );
#else
		hprintf( WARN_CANNOT_OPEN_DATAFILE_s, filePath );
#endif /* GUI */
		return;
		}
#endif /* __MAC__ */

	/* Flag the fact that we've made changes to the archive */
	archiveChanged = TRUE;

#ifdef GUI
	initProgressReport( FALSE, NULL, fileInfoPtr->fName, fileInfoPtr->fSize );
#else
	hprintfs( MESG_ADDING );
	printFileName( fileInfoPtr->fName, 6, fileInfoPtr->fSize, TRUE );
#endif /* GUI */
#ifdef __UNIX__
	/* If it's a link, don't add the data twice */
/*	if( !isLink )
		Don't do anything at the moment - need to work on ergonomics of link
		handling before this code is enabled
		{ */
#endif /* __UNIX__ */
	if( cryptFlags & ( CRYPT_CKE | CRYPT_PKE ) )
		cryptInfoLength = cryptBegin( MAIN_KEY );

	if( fileInfoPtr->fSize )
		if( fileInfoPtr->fSize > MIN_FILESIZE && !( flags & STORE_ONLY ) )
			{
			/* Encrypt any data still in the buffer.  This is necessary since
			   we may need to backtrack later on if the data is uncompressible */
			if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
				{
				encryptCFB( _outBuffer + cryptMark, _outByteCount - cryptMark );
				cryptMark = _outByteCount;
				}

			saveCryptState();
			comprMethod = FORMAT_PACKED;
			dataLength = pack( &isText, fileInfoPtr->fSize );
			firstFile = FALSE;

			/* If we've ended up expanding the data then store it instead.
			   Undoing the call to pack() is particularly nasty since there
			   may still be data in the buffer which is unwritten; there may
			   in fact still be data in the buffer from the *previous* call to
			   pack(), so we must flush the buffer before we do any seeking.
			   If this is a multipart archive we don't bother since the
			   complications involved if the data is spread over 15 different
			   disks makes recovery distrinctly nontrivial.  Similary if we
			   are in block mode we can't unwind the compressor to recover */
			if( dataLength >= fileInfoPtr->fSize &&
				!( flags & MULTIPART_ARCH ) &&
				!( flags & BLOCK_MODE ) )
				{
#ifdef GUI
				/* Redraw the progress bar */
				endProgressReport();
				initProgressReport( FALSE, NULL, fileInfoPtr->fName, fileInfoPtr->fSize );
#else
				/* Go to start of line and overprint "Adding" message */
				hputchars( '\r' );
				hprintfs( MESG_ADDING );
				printFileName( fileInfoPtr->fName, 6, fileInfoPtr->fSize, TRUE );
#endif /* GUI */

				flushBuffer();
				hlseek( archiveFD, -dataLength, SEEK_CUR );
				htruncate( archiveFD );	/* Kill everything after this point */
				hlseek( dataFD, 0L, SEEK_SET );
				setCurrPosition( getCurrPosition() - dataLength + HPACK_ID_SIZE );
				restoreCryptState();
				goto storeData;
				}
			}
		else
			{
storeData:
			comprMethod = FORMAT_STORED;
			dataLength = store( &isText );
			}
	else
		{
		/* Don't bother handling zero-length files */
		comprMethod = FORMAT_STORED;
		dataLength = 0L;
		}
	if( cryptFlags & ( CRYPT_CKE | CRYPT_PKE ) )
		cryptEnd();

	/* Sign the file if required */
	if( cryptFlags & CRYPT_SIGN )
		{
        /* Get all info to sign out of buffers */
		flushBuffer();

		/* Write the signature as a tag.  Since we don't know the tag length
		   in advance, we use a somewhat nasty trick of relying on writeTag()
		   to store the length as a byte and adding it later by or-ing it into
		   the data in the output buffer.  This is safe since we've just
		   flushed it */
		startPosition = getCurrPosition() - dataLength;
		writeTag( TAG_SECURITY_INFO, TAGFORMAT_STORED, 0L, 0L );
		secInfoLength = createSignature( startPosition, dataLength, signerID );
		_outBuffer[ LONG_TAGSIZE ] = secInfoLength;
		auxDataLen += LONG_TAGSIZE + sizeof( BYTE ) + secInfoLength;
		}

	/* Add type info for archive comments */
	if( flags & ARCH_COMMENT )
		hType = commentType;

#ifdef __MAC__
	/* We've stored the data fork, now store the resource fork if there is
	   one.  We've already opened a FD to it at the start of the function */
	if( fileInfoPtr->resSize )
		{
		/* Encrypt any data still in output buffer and force it out to disk */
		flushBuffer();

		/* Turn off output encryption while we write to the temp file */
		cryptFlagSave = cryptFlags;
		cryptFlags = 0;
		cryptOutSave = doCryptOut;
		doCryptOut = FALSE;

		/* Open the resource fork of the file */
		savedInFD = getInputFD();
		saveOutputState();
		if( ( resourceTmpFD = hcreat( RESOURCE_TMPNAME, O_RDWR ) ) == ERROR )
			error( CANNOT_OPEN_TEMPFILE );
		setInputFD( resourceForkFD );
		setOutputFD( resourceTmpFD );

		if( ( fileInfoPtr->resSize > MIN_FILESIZE ) &&
			!( flags & BLOCK_MODE ) && !( flags & STORE_ONLY ) )
			{
			resourceComprMethod = TAGFORMAT_PACKED;
			byteCount = pack( &dummy, fileInfoPtr->resSize );

			/* If we've ended up expanding the data then store it as before */
			if( byteCount >= fileInfoPtr->resSize )
				{
#ifdef GUI
				/* Redraw the progress bar */
				endProgressReport();
				initProgressReport( FALSE, NULL, fileInfoPtr->fName, fileInfoPtr->resSize );
#else
				/* Go to start of line and overprint "Adding" message */
				hputchars( '\r' );
				hprintfs( MESG_ADDING );
				printFileName( fileInfoPtr->fName, 6, fileInfoPtr->resSize, TRUE );
#endif /* GUI */

				flushBuffer();
				hlseek( resourceTmpFD, 0L, SEEK_SET );
				htruncate( resourceTmpFD );	/* Kill everything after this point */
				hlseek( resourceForkFD, 0L, SEEK_SET );
				goto storeResourceData;
				}
			}
		else
			{
storeResourceData:
			resourceComprMethod = TAGFORMAT_STORED;
			byteCount = store( &dummy );
			}
		flushBuffer();		/* Force data out to disk */

		/* Reset encryption status */
		cryptFlags = cryptFlagSave;
		doCryptOut = cryptOutSave;

		/* Now move the data from the temp file to the archive itself */
		closeResourceFork( resourceForkFD );
		hlseek( resourceTmpFD, 0L, SEEK_SET );
		setInputFD( resourceTmpFD );
		forceRead();
		restoreOutputState();
		auxDataLen += writeTag( TAG_RESOURCE_FORK, resourceComprMethod,
								byteCount, fileInfoPtr->resSize ) +
					  byteCount;

		/* Copy the data across the slow way.  This is necessary to handle encryption */
		while( byteCount-- )
			fputByte( fgetByte() );

		/* Delete temp file and restore input stream */
		hclose( resourceTmpFD );
		resourceTmpFD = IO_ERROR;		/* Mark it as invalid */
		hunlink( RESOURCE_TMPNAME );
		setInputFD( savedInFD );
		}
#endif /* __MAC__ */

	/* Write the file's attributes if necessary */
	if( flags & STORE_ATTR )
		{
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
		writeTag( TAG_MSDOS_ATTR, TAGFORMAT_STORED, LEN_MSDOS_ATTR, LEN_NONE );
		fputByte( fileInfoPtr->fAttr );			/* Save attributes */
		auxDataLen += SHORT_TAGSIZE + sizeof( BYTE );
#elif defined( __AMIGA__ )
		writeTag( TAG_AMIGA_ATTR, TAGFORMAT_STORED, LEN_AMIGA_ATTR, LEN_NONE );
		fputByte( fileInfoPtr->fAttr );			/* Save attributes */
		auxDataLen += SHORT_TAGSIZE + sizeof( BYTE );
		if( fileInfoPtr->hasComment )
			auxDataLen += storeComment( fileInfoPtr, archiveFD );
		auxDataLen += storeDiskObject( filePath, archiveFD );
#elif defined( __ARC__ )
		writeTag( TAG_ARC_ATTR, TAGFORMAT_STORED, LEN_ARC_ATTR, LEN_NONE );
		fputByte( fileInfoPtr->fAttr );			/* Save attributes */
		auxDataLen += SHORT_TAGSIZE + sizeof( BYTE );
#elif defined( __ATARI__ )
		writeTag( TAG_ATARI_ATTR, TAGFORMAT_STORED, LEN_ATARI_ATTR, LEN_NONE );
		fputByte( fileInfoPtr->fAttr );			/* Save attributes */
		auxDataLen += SHORT_TAGSIZE + sizeof( BYTE );
#elif defined( __MAC__ )
		writeTag( TAG_MAC_ATTR, TAGFORMAT_STORED, LEN_MAC_ATTR, LEN_NONE );
		fputWord( fileInfoPtr->fAttr );			/* Save attributes */
		writeTag( TAG_CREATION_TIME, TAGFORMAT_STORED, LEN_CREATION_TIME, LEN_NONE );
		fputLong( fileInfoPtr->createTime );	/* Save creation time */
		writeTag( TAG_MAC_TYPE, TAGFORMAT_STORED, LEN_MAC_TYPE, LEN_NONE );
		fputLong( fileInfoPtr->type );			/* Save file type */
		writeTag( TAG_MAC_CREATOR, TAGFORMAT_STORED, LEN_MAC_CREATOR, LEN_NONE );
		fputLong( fileInfoPtr->creator );		/* Save file creator */
		if( fileInfoPtr->backupTime )
			{
			/* Save backup time if there is one */
			writeTag( TAG_BACKUP_TIME, TAGFORMAT_STORED, LEN_BACKUP_TIME, LEN_NONE );
			fputLong( fileInfoPtr->backupTime );
			auxDataLen += SHORT_TAGSIZE + sizeof( LONG );
			}
		if( fileInfoPtr->versionNumber )
			{
			/* Save version number if there is one */
			writeTag( TAG_VERSION_NUMBER, TAGFORMAT_STORED, LEN_VERSION_NUMBER, LEN_NONE );
			fputWord( ( WORD ) fileInfoPtr->versionNumber );
			auxDataLen += SHORT_TAGSIZE + sizeof( WORD );
			}
		auxDataLen += SHORT_TAGSIZE + sizeof( WORD ) + SHORT_TAGSIZE + sizeof( LONG ) +
					  SHORT_TAGSIZE + sizeof( LONG ) + SHORT_TAGSIZE + sizeof( LONG );
#elif defined( __OS2__ )
		if( fileInfoPtr->aTime )
			{
			/* Save access time if there is one */
			writeTag( TAG_ACCESS_TIME, TAGFORMAT_STORED, LEN_ACCESS_TIME, LEN_NONE );
			fputLong( fileInfoPtr->aTime );
			}
		if( fileInfoPtr->cTime )
			{
			/* Save creation time if there is one */
			writeTag( TAG_CREATION_TIME, TAGFORMAT_STORED, LEN_CREATION_TIME, LEN_NONE );
			fputLong( fileInfoPtr->cTime );			
			}
		auxDataLen += SHORT_TAGSIZE + sizeof( LONG ) + SHORT_TAGSIZE + sizeof( LONG );
		auxDataLen += storeEAinfo( TRUE, filePath, fileInfoPtr->eaSize, archiveFD );
#elif defined( __UNIX__ )
		writeTag( TAG_UNIX_ATTR, TAGFORMAT_STORED, LEN_UNIX_ATTR, LEN_NONE );
		fputWord( fileInfoPtr->statInfo.st_mode );	/* Save attributes */
		writeTag( TAG_ACCESS_TIME, TAGFORMAT_STORED, LEN_ACCESS_TIME, LEN_NONE );
		fputLong( fileInfoPtr->statInfo.st_atime );	/* Save access time */
		auxDataLen += SHORT_TAGSIZE + sizeof( WORD ) + SHORT_TAGSIZE + sizeof( LONG );
#endif /* Various OS-dependant attribute writes */
		}
#ifdef __UNIX__
/*		}
	else
		{
		/ It's a link, set to zero-length stored file /
		fileInfoPtr->fSize = 0L;
		dataLength = 0L;
		comprMethod = FORMAT_STORED;
		} */
#endif /* __UNIX__ */

	/* Build the header from the file info and add it to the header list */
	theHeader.fileTime = fileInfoPtr->fTime;
	theHeader.fileLen = fileInfoPtr->fSize;
	theHeader.auxDataLen = auxDataLen;
	theHeader.dirIndex = dirIndex;
	theHeader.dataLen = dataLength + cryptInfoLength;
	theHeader.archiveInfo = ( flags & ARCH_COMMENT ) ? ARCH_SPECIAL : 0;
	theHeader.archiveInfo |= comprMethod;
	theHeader.archiveInfo |= ARCHIVE_OS;
	if( isText )
		theHeader.archiveInfo |= ARCH_ISTEXT;

	/* Build the extraInfo byte */
	extraInfo = ( cryptFlags & CRYPT_PKE || cryptFlags & CRYPT_CKE ) ?
				EXTRA_ENCRYPTED : 0;
	extraInfo |= ( cryptFlags & CRYPT_SIGN ) ? EXTRA_SECURED : 0;
#if defined( __MSDOS16__ )
	/* Fix compiler bug */
#elif defined( __ARC__ )
	extraInfo |= sizeof( WORD );
#elif defined( __IIGS__ )
	extraInfo |= sizeof( WORD ) + sizeof( LONG );
#elif defined( __MAC__ )
	extraInfo |= sizeof( LONG ) + sizeof( LONG );
#endif /* Various OS-dependant extraInfo length defines */
#ifdef __UNIX__
/*	if( isLink )
		extraInfo = EXTRA_LINKED; */
#endif /* __UNIX__ */
	if( extraInfo )
		theHeader.archiveInfo |= ARCH_EXTRAINFO;

	/* Include the size of the error recovery tag if necessary */
	if( flags & ERROR_RECOVER )
		{
		/* Length is error ID + file header + ASCIZ filename + crc16.  Note
		   that we have to be very careful here since adding the auxDataLen
		   may change the length encoding of the header, making it larger and
		   changing the length encoding, etc */
		theHeader.auxDataLen += ERROR_ID_SIZE + strlen( fileInfoPtr->fName ) +
								1 + sizeof( WORD );
		errorTagLen = computeHeaderSize( &theHeader, extraInfo );
		theHeader.auxDataLen += errorTagLen;
		if( ( i = computeHeaderSize( &theHeader, extraInfo ) ) != errorTagLen )
			/* Adjust in case the header size has grown */
			theHeader.auxDataLen += i - errorTagLen;

		/* Nasty chicken-and-egg problem: We don't know the tag size until
		   after we write it, but we need to know the auxDataLen before we
		   write it, so we kludge it here */
		if( errorTagLen <= SHORT_TAGLEN )
			theHeader.auxDataLen += SHORT_TAGSIZE;
		else
			/* Have you ever wondered why the HPACK file permissions are '666'? */
			theHeader.auxDataLen += LONG_TAGSIZE + ( ( errorTagLen <= 0xFF ) ?
										sizeof( BYTE ) : sizeof( WORD ) ) +
									sizeof( WORD );
		}

	if( !overWriteEntry )
		{
		/* Add the new header and any supplementary information to the list */
		addFileHeader( &theHeader, hType, extraInfo, NO_LINK );
		}
	else
		{
		/* Overwrite the existing header at this position and update its
		   extraInfo */
		fileHdrCurrPtr->data = theHeader;
		if( extraInfo )
			{
			/* Get rid of any existing extraInfo if it exists */
			if( fileHdrCurrPtr->extraInfo != NULL )
				hfree( fileHdrCurrPtr->extraInfo );

			/* Now add the new extraInfo */
			if( ( fileHdrCurrPtr->extraInfo =
					( BYTE * ) hmalloc( getExtraInfoLen( extraInfo ) + 1 ) ) == NULL )
				error( OUT_OF_MEMORY );
			*fileHdrCurrPtr->extraInfo = extraInfo;
			}
		}
	fileHdrCurrPtr->tagged = TRUE;

	/* Add any extraInfo to the header if necessary */
#if defined( __MSDOS16__ )
	/* Fix compiler bug */
#elif defined( __ARC__ )
	/* Add file type */
	fileHdrCurrPtr->type = fileInfoPtr->type;
#elif defined( __IIGS__ )
	/* Add file type and auxiliary type */
	fileHdrCurrPtr->type = fileInfoPtr->type;
	fileHdrCurrPtr->auxType = fileInfoPtr->auxType;
#elif defined( __MAC__ )
	/* Add file type and creator */
	fileHdrCurrPtr->type = fileInfoPtr->type;
	fileHdrCurrPtr->creator = fileInfoPtr->creator;
#elif defined( __UNIX__ )
	/* Add link ID (only used internally by HPACK) */
	fileHdrCurrPtr->fileLinkID = linkID;
#endif /* Various OS-dependant information additions */

	/* Write the error recovery tag if necessary - if used this must be the
	   last tag, following the file data and auxiliary data */
	if( flags & ERROR_RECOVER )
		{
		/* First, write the error-recovery ID info */
		writeTag( TAG_ERROR, TAGFORMAT_STORED, errorTagLen, LEN_NONE );
		for( i = 0; i < ERROR_ID_SIZE; i++ )
			fputByte( ERROR_ID[ i ] );

		/* Then write the error-recovery info itself: file header, fileName,
		   and crc16 for tag info.  Note that we can't use writeTag() for
		   the file header since we have to write the individual fields for
		   portability */
		checksumBegin( RESET_CHECKSUM );
		writeFileHeader( fileHdrCurrPtr );
		auxDataLen = strlen( fileInfoPtr->fName );
		for( i = 0; i <= auxDataLen; i++ )
			fputByte( fileInfoPtr->fName[ i ] );
		checksumEnd();
		fputWord( crc16 );
		}


#ifdef GUI
	endProgressReport();
#else
	hputchars( '\n' );
#endif /* GUI */
#pragma warn -par
	}
#pragma warn +par
