/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  HPACK Messages Symbolic Defines					*
*							LANGUAGE.H  Updated 28/04/93					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*					and if you do so there will be....trubble.				*
*			 And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1991 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#include "filehdr.h"
#include "filesys.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
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

void blankLine( int count );

/* Prototypes for functions in KEYMGMT.C */

char *getFirstKeyPath( const char *keyRingPath, const char *keyringName );
char *getNextKeyPath( void );

/* The escape code used to indicate a special condition in a string */

#define ESCAPE_CODE		0xFF

/* The message buffer and pointers to it */

#define MAX_MSGS		260		/* Shouldn't be more than this... */

static char *mesgBuffer;
char *mesg[ MAX_MSGS ];

/* The maximum possible number of macro characters */

#define NO_MACROS	128

/* States for the conditional-string parser.  The state machine isn't 100%
   correct but it works, and the preprocessor will never output an incorrect
   series of input states anyway */

typedef enum { STATE_NORMAL, STATE_IF_TRUE, STATE_IF_FALSE,
			   STATE_ELSE_TRUE, STATE_ELSE_FALSE, STATE_CASE_BEGIN,
			   STATE_CASE } STATE;

/* Characters for the response from the user */

char RESPONSE_YES, RESPONSE_NO, RESPONSE_ALL, RESPONSE_QUIT;

/* The default language and charset type, which are adapted to the system
   we're running on */

#define DEFAULT_LANGUAGE	"english"

#if defined( __AMIGA__ )
  #define DEFAULT_CHARSET	"iso8859-1"
#elif defined( __ARC__ )
  #define DEFAULT_CHARSET	"iso8859-1"
#elif defined( __ATARI__ )
  #define DEFAULT_CHARSET	"ibmpc"
#elif defined( __MAC__ )
  #define DEFAULT_CHARSET	"macintosh"
#elif defined( __MSDOS__ ) || defined( __OS2__ )
  #define DEFAULT_CHARSET	"ibmpc"
#elif defined( __UNIX__ )
  #define DEFAULT_CHARSET	"ascii"
#elif defined( __VMS__ )
  #define DEFAULT_CHARSET	"ascii"
#else
  #define DEFAULT_CHARSET	"ascii"
#endif /* Various system-specific charset types */

/* The name of the HPACK data file */

#if defined( __ARC__ )
  #define DATA_FILENAME		"hpack_lang"
#elif defined( __MAC__ )
  #define DATA_FILENAME		"HPACK Language Data"
#elif defined( __AMIGA__ )
  #define DATA_FILENAME		"s:language.dat"
#else
  #define DATA_FILENAME		"language.dat"
#endif /* Various system-specific filenames */

/* Get a filename string which varies depending on the count of files */

char *getCase( const int count )
	{
	char *strPtr = CASE_FILE_STRING;

	while( ( BYTE ) *strPtr++ == ESCAPE_CODE )
		{
		/* Check for individual cases.  Ideally these should be coded into
		   the strings (in fact the preprocessor can do this) but it makes
		   the interpreter too complex.  Since there are only three cases its
		   easier to hard-code it */
		if( ( *strPtr == 0x40 ) && ( count == 1 ) )
			return( ++strPtr );
		if( ( *strPtr == 0x41 ) &&
			( ( count < 10 || count > 20 ) &&
				( ( count % 10 ) >= 2 && ( count % 10 ) <= 4 ) ) )
			return( ++strPtr );
		if( *strPtr == 0x4E )
			return( ++strPtr );

		/* Skip to next code */
		while( *strPtr++ );
		}

	return( "" );	/* Should never happen */
	}

/* Display the title screen */

void showTitle( void )
	{
	hputs( TITLE_LINE1 );
	hputs( TITLE_LINE2 );
	hputs( TITLE_LINE3 );
	hputchar( '\n' );
	}

/* Display the help screen */

void showHelp( void )
	{
	int totalLines = HELP_STRINGS_OFS, lineNo = 4;
	int strIndex = HELP_STRINGS_OFS;
	char *strPtr, response;

	/* Find the total number of lines, used for printing the % complete
	   indicator */
	while( mesg[ totalLines++ ] != NULL );
	totalLines -= HELP_STRINGS_OFS;

	do
		{
		/* Point to next string */
		strPtr = mesg[ strIndex++ ];
		if( strPtr == NULL )
			{
			hprintf( MESG_HIT_ANY_KEY );
			hgetch();
			blankLine( screenWidth - 1 );
			return;
			}
		else
			{
			/* Display the string */
			hprintf( "%s\n", strPtr );

			/* Update the prompt if we've filled a screen, provided we're not
			   on the last line */
			if( ( ++lineNo >= screenHeight - 1 ) && ( mesg[ strIndex ] != NULL ) )
				{
				hprintf( MESG_HIT_SPACE_FOR_NEXT_SCREEN,
						( 100 * ( strIndex - HELP_STRINGS_OFS ) ) / totalLines );

				/* Process user response */
				response = hgetch();
				response = toupper( response );	/* Damn macros */
				blankLine( screenWidth );
				if( response == RESPONSE_QUIT )
					return;
				lineNo -= ( response == ' ' ) ? screenHeight : 1;
				}
			}
		}
	while( strPtr != NULL );
	}

/* Read in a string - self-limiting at 40 chars (it's assumed no language or
   charset name is longer than this) */

static void readString( BYTE *buffer )
	{
	int i = 0;

#pragma warn -pia
	while( ( buffer[ i++ ] = fgetByte() ) && ( i < 40 ) );
#pragma warn +pia
	}

/* Set up and shut down the message base */

int initMessages( const char *path, const BOOLEAN doShowHelp )
	{
	LONG presentFlags[ NO_MACROS ], presentFlag;
	int count, length, macroLength, helpLength;
	char *languageName, *charsetName;
	char xlatTbl[ NO_MACROS ][ 3 ];
	int language = 0, charset = 0, i, j;
	BYTE buffer[ MAX_PATH ], *mesgPtr;
	STATE state = STATE_NORMAL;
	BOOLEAN stateLock, isErrorSection = TRUE;
#ifndef __AMIGA__
	char *fileNamePtr;
#endif /* !__AMIGA__ */
#ifdef __ARC__
	char languageBuffer[ 100 ], charsetBuffer[ 100 ];
#endif /* __ARC__ */
	FD inFD;
	BYTE ch;

	/* Build the path to the data file and try and open it */
#if defined( __AMIGA__ )
	/* Just open the config file in the standard directory */
	if( ( inFD = hopen( DATA_FILENAME, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) == ERROR )
		return( ERROR );
#elif defined( __ARC__ )
	/* First, try to find the file on argv[ 0 ], if that fails look on
	   HPACKPATH for it */
	strcpy( ( char * ) buffer, path );
	fileNamePtr = findFilenameStart( ( char * ) buffer );
	strcpy( fileNamePtr, DATA_FILENAME );
	fileNamePtr = ( char * ) buffer;
	if( ( inFD = hopen( fileNamePtr, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) == ERROR )
		{
		/* Build path to language file and try and process it */
		fileNamePtr = getFirstKeyPath( getenv( "HPACKPATH" ), DATA_FILENAME );
		while( fileNamePtr != NULL )
			{
			/* Try and read the seed from the seedfile */
			if( ( inFD = hopen( fileNamePtr, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) != ERROR )
				break;

			/* No luck, try the next search path */
			fileNamePtr = getNextKeyPath();
			}
		if( fileNamePtr == NULL )
			return( ERROR );		/* Couldn't find/open input file */
		}
#elif defined( __ATARI__ ) || defined( __MAC__ ) || \
	  defined( __MSDOS__ ) || defined( __OS2__ )
	strcpy( ( char * ) buffer, path );
	fileNamePtr = findFilenameStart( ( char * ) buffer );
	strcpy( fileNamePtr, DATA_FILENAME );
	fileNamePtr = ( char * ) buffer;
	if( ( inFD = hopen( fileNamePtr, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) == ERROR )
		return( ERROR );
#elif defined( __UNIX__ )
	/* Build path to language file and try and process it */
	fileNamePtr = getFirstKeyPath( getenv( "PATH" ), DATA_FILENAME );
	while( fileNamePtr != NULL )
		{
		/* Try and read the seed from the seedfile */
		if( ( inFD = hopen( fileNamePtr, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) != ERROR )
			break;

		/* No luck, try the next search path */
		fileNamePtr = getNextKeyPath();
		}
	if( fileNamePtr == NULL )
		{
		/* Try to find it from argv[ 0 ] */
		strcpy( ( char * ) buffer, path );
		fileNamePtr = findFilenameStart( ( char * ) buffer );
		strcpy( fileNamePtr, DATA_FILENAME );
		fileNamePtr = ( char * ) buffer;
		if( ( inFD = hopen( fileNamePtr, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) == ERROR )
			return( ERROR );		/* Couldn't find/open input file */
		}
#endif /* System-specific handling of how to find language defn.file */
	setInputFD( inFD );
	resetFastIn();

	/* Check for magic value 'LANG' at start */
	if( fgetLong() != 0x4C414E47L )
		return( ERROR );

	/* Find out which language and character set the user wants */
#ifdef __ARC__
	if( ( languageName = getenv( "LANGUAGE" ) ) != NULL )
		strcpy( languageBuffer, languageName );
	else
		*languageBuffer = '\0';
	if( ( charsetName = getenv( "CHARSET" ) ) != NULL )
		strcpy( charsetBuffer, charsetName );
	else
		*charsetBuffer = '\0';
	languageName = languageBuffer;
	charsetName = charsetBuffer;
		/* Don't you just love implementation-dependant parts of standards? */
#else
	if( ( languageName = getenv( "LANGUAGE" ) ) == NULL )
		languageName = DEFAULT_LANGUAGE;
	if( ( charsetName = getenv( "CHARSET" ) ) == NULL )
		charsetName = DEFAULT_CHARSET;
#endif /* __ARC__ */

	/* Find the language we're looking for */
	while( TRUE )
		{
		/* Read in the next language name, default to English if no
		   match found */
		readString( buffer );
		if( *buffer == 0xFF )
			{
			ungetByte();	/* Undo last byte read */
			languageName = DEFAULT_LANGUAGE;
			language = 0;
			break;
			}

		if( !stricmp( ( char * ) buffer, languageName ) )
			{
			/* We've found the language we want, skip to end marker */
			while( fgetByte() != 0xFF );
			break;
			}

		/* Go to next language */
		language++;
		}

	/* Read in the macro present flags */
	count = fgetWord();
	for( i = 0; i < count; i++ )
		presentFlags[ i ] = fgetLong();

	/* Search for the character set translation table */
	while( TRUE )
		{
		/* Read in the next character set name and make sure we haven't
		   reached the end of the macro tables */
		readString( buffer );
		if( !*buffer )
			{
			ungetByte();	/* Undo last byte read */

			/* Unknown character set, simply clear the macro table */
			for( i = 0; i < count; i++ )
				{
				xlatTbl[ i ][ 0 ] = 1;
				xlatTbl[ i ][ 1 ] = '?';
				}
			break;
			}
		length = fgetWord();

		/* Check for a known character set */
		if( !stricmp( ( char * ) buffer, charsetName ) )
			{
			presentFlag = 1 << charset;
			for( i = 0; i < count; i++ )
				if( presentFlags[ i ] & presentFlag )
					{
					/* The macro is present, read it in */
					macroLength = fgetByte();
					xlatTbl[ i ][ 0 ] = macroLength;
					for( j = 0; j < macroLength; j++ )
						xlatTbl[ i ][ j + 1 ] = fgetByte();
					}
				else
					{
					/* Macro doesn't exist, add placeholder */
					xlatTbl[ i ][ 0 ] = 1;
					xlatTbl[ i ][ 1 ] = '?';
					}
			break;
			}
		else
			/* Skip over this macro table */
			skipSeek( ( long ) length );

		/* Go to next character set */
		charset++;
		}

	/* Skip all remaining macro tables */
	readString( buffer );
	while( *buffer )
		{
		skipSeek( ( long ) fgetWord() );
		readString( buffer );
		}

	/* Skip to the start of the actual text data */
	for( i = 0; i < language; i++ )
		fgetLong();
	skipSeek( ( long ) fgetLong() );

	/* Read in length of total block (not counting the size of the user-
	   response chars) and the length of the help block */
	length = fgetWord() - 4;
	helpLength = fgetWord();
	if( !doShowHelp )
		/* Only read in the help data if we need to */
		length -= helpLength;

	/* Read in the text data block.  Note that we need to use an fgetByte()
	   loop rather than a direct read to go via the fastio buffering
	   system */
	if( ( mesgBuffer = hmalloc( length + 200 ) ) == NULL )
		return( ERROR );
	for( i = 0; i < length; i++ )
		mesgBuffer[ i ] = fgetByte();
	mesgBuffer[ i ] = '\0';		/* Add final terminator */

	/* Build up the array of pointers to the messages */
	i = count = 0;
	while( count < length )
		{
		/* Check if it has any special significance */
		mesgPtr = ( BYTE * ) mesgBuffer + count;
		if( *mesgPtr == ESCAPE_CODE )
			if( mesgPtr[ 1 ] == 0x20 )
				{
				/* Handle for special end-of-error-section code */
				count += 2;
				mesgPtr += 2;
				isErrorSection = FALSE;
				}
			else
				{
				stateLock = FALSE;
				while( *mesgPtr == ESCAPE_CODE )
					{
					/* Skip escape code */
					mesgPtr++;
					count++;

					/* Check for an 'if' code */
					if( !stateLock && ( *mesgPtr & 0x80 ) )
						{
						/* Check for a string matching the current OS type */
						if( ( *mesgPtr & 0x0F ) == ARCHIVE_OS )
							state = STATE_IF_TRUE;
						else
							if( ( *mesgPtr & 0x0F ) == 0x0F )
								state = ( ( state == STATE_ELSE_FALSE ) ||
										  ( state == STATE_IF_TRUE ) ) ?
										STATE_ELSE_FALSE : STATE_ELSE_TRUE;
							else
								state = ( state != STATE_NORMAL ) ?
										STATE_ELSE_FALSE : STATE_IF_FALSE;

						/* Make sure a state is locking (ie it can't later be
						   negated by a false conditional in a multiple-case
						   if statement) */
						if( ( state == STATE_IF_TRUE ) ||
							( state == STATE_ELSE_TRUE ) )
							stateLock = TRUE;

						/* Check for the end of the current conditional
						   string context */
						if( ( *mesgPtr & 0x0F ) == 0x0E )
							state = STATE_NORMAL;
						}

					/* Check for a 'case' control code */
					if( *mesgPtr & 0x40 )
						state = ( *mesgPtr == 0x4F ) ? STATE_NORMAL :
								( ( state == STATE_CASE_BEGIN ) ||
								  ( state == STATE_CASE ) ) ?
									STATE_CASE : STATE_CASE_BEGIN;

					/* Skip escape control code */
					mesgPtr++;
					count++;
					}
				}

		/* Add the message if necessary and point to the next message.  If
		   it's a case code, we must take care not to skip the control code
		   which is needed later when the string is parsed */
		if( ( state == STATE_NORMAL ) || ( state == STATE_IF_TRUE ) ||
			( state == STATE_ELSE_TRUE ) || ( state == STATE_CASE_BEGIN ) )
			mesg[ i++ ] = ( state == STATE_CASE_BEGIN ) ?
						  ( char * ) mesgPtr - 2 : ( char * ) mesgPtr;

		/* Now skip the string, checking for macros as we go */
		if( isErrorSection )
			count++;	/* Skip over error code */
#pragma warn -pia
		while( ch = mesgBuffer[ count++ ] )
#pragma warn +pia
			if( ( ch >= 0x80 ) && ( ch != 0xFF ) )
				{
				/* Adjust to base 0 */
				ch -= 0x80;

				/* Check for a one-char substitution (simplest case) */
				if( ( macroLength = xlatTbl[ ch ][ 0 ] ) == 1 )
					mesgBuffer[ count - 1 ] = xlatTbl[ ch ][ 1 ];
				else
					{
					/* Open up room for a multichar macro and adjust all counts */
					count--;				/* Fiddle count to undo ++ */
					memmove( mesgBuffer + count + macroLength - 1,
							 mesgBuffer + count, length - count );
					memcpy( mesgBuffer + count, &xlatTbl[ ch ][ 1 ], macroLength );
					count += macroLength;	/* Lack of -1 re-adds ++ */
					length += macroLength - 1;
					}
				}
		}

	/* Skip the help data if necessary and add the end marker */
	if( !doShowHelp )
		skipSeek( ( long ) helpLength );
	mesg[ i ] = NULL;

	/* Finally, read the user-response chars */
	RESPONSE_YES = fgetByte();
	RESPONSE_NO = fgetByte();
	RESPONSE_ALL = fgetByte();
	RESPONSE_QUIT = fgetByte();

	hclose( inFD );

	return( OK );
	}

#ifndef __MSDOS16__

void endMessages( void )
	{
	hfree( mesgBuffer );
	}
#endif /* __MSDOS16__ */
