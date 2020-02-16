/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								HPACK CLI Interface							*
*							  CLI.C  Updated 06/05/92						*
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
#include "script.h"
#include "sql.h"
#include "system.h"
#include "wildcard.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "fastio.h"
  #include "language.h"
#else
  #include "io/fastio.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* Prototypes for functions in VIEWFILE.C */

void setDateFormat( void );

/* Prototypes for compression functions */

void initPack( const BOOLEAN initAllBuffers );
void endPack( void );

/* Prototypes for functions in VIEWFILE.C */

void showTotals( void );

/* Prototypes for functions in KEYMGMT.C */

char *getFirstKeyPath( const char *keyRingPath, const char *keyringName );
char *getNextKeyPath( void );

/* The default match string, which matches all files.  This must be done
   as a static array since if given as a constant string some compilers
   may put it into the code segment which may cause problems when FILESYS.C
   attempts any conversion on it */

char WILD_MATCH_ALL[] = "*";

/* The following are defined in ARCHIVE.C */

extern BOOLEAN overWriteEntry;	/* Whether we overwrite the existing file
								   entry or add a new one in addFileHeader()
								   - used by FRESHEN, REPLACE, UPDATE */
#ifdef __MSDOS16__
  void ndoc( const char **strPtr );
#endif /* __MSDOS16__ */
#ifdef __OS2__
  BOOLEAN queryFilesystemType( char *path );
#endif /* __OS2__ */
#ifdef __MAC__
  #define ARCHIVE_PATH	TRUE
  #define FILE_PATH		FALSE

  void setVolumeRef( char *path, const BOOLEAN isArchivePath );
  int stripVolumeName( char *path );
#endif /* __MAC__ */
#ifdef __VMS__
  int translateVMSretVal( const int retVal );
#endif /* __VMS__ */

#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )

/* The drive the archive is on (needed for multidisk archives) */

extern BYTE archiveDrive;
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */

/* The default path and name of HPACK's configuration file */

const char HPACKPATH[] = "HPACKPATH";

#if defined( __ARC__ )
  const char CONFIGFILENAME[] = "hpack_cfg";
#elif defined( __MAC__ )
  const char CONFIGFILENAME[] = "HPACK Configuration File";
#else
  const char CONFIGFILENAME[] = "hpack.cfg";
#endif /* Various system-specific config file names */

/****************************************************************************
*																			*
*							Handle Command-line Switches					*
*																			*
****************************************************************************/

/* Handle the command for the archive.  This can also be done in a switch
   statement; however the compiler probably produces code equivalent to the
   following, and it is far more compact in source form */

#define NO_CMDS	9

static void doCommand( const char *theCommand )
	{
	int i;

	choice = toupper( *theCommand );
	for( i = 0; ( i < NO_CMDS ) && ( "ADFPVXRUT"[ i ] != choice ); i++ );
	if( i == NO_CMDS || theCommand[ 1 ] )
		error( UNKNOWN_COMMAND, theCommand );
	}

/* Get a byte as two hex digits - used by several sections of doArg() */

#ifdef __MSDOS16__
  BYTE getHexByte( char **strPtr );
#else
BYTE getHexByte( char **strPtr )
	{
	char ch;
	BYTE retVal;

	retVal = toupper( **strPtr );
	retVal -= ( retVal >= 'A' ) ? 'A' - 10 : '0';
	*strPtr++;
	if( isxdigit( **strPtr ) )
		{
		retVal <<= 4;
		ch = toupper( **strPtr );
		ch -= ( ch >= 'A' ) ? ( 'A' - 10 ) : '0';
		retVal |= ch;
		}
	return( retVal );
	}
#endif /* __MSDOS16__ */

/* Set the base path */

int setBasePath( char *argString )
	{
	int length = 0, ch;

	while( *argString )
		{
		basePath[ length ] = caseConvert( *argString );
#if defined( __ATARI__ ) || defined( __OS2__ )
		if( basePath[ length ] == '\\' )
			basePath[ length ] = SLASH;
#endif /* __ATARI__ || __OS2__ */
		argString++;
		if( ++length >= MAX_PATH - 1 )
			{
			/* Path is too long, truncate it, set error status and return
			   actual length */
			basePath[ length - 1 ] = '\0';
			basePathLen = ERROR;
			while( basePath[ length ] )
				length++;
			return( length );
			}
		}
	basePath[ length ] = '\0';

	/* Append SLASH if necessary */
#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __ATARI__ ) || \
	defined( __MAC__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ ) || defined( __VMS__ )
	if( length && ( ch = basePath[ length - 1 ] ) != SLASH && ch != ':' )
#else
	if( length && ( ch = basePath[ length - 1 ] ) != SLASH )
#endif /* __AMIGA__ || __ARC__ || __ATARI__ || __MAC__ || __MSDOS16__ || __MSDOS32__ || __OS2__ || __VMS__ */
		{
		strcat( basePath, SLASH_STR );
		length++;
		}
	basePathLen = length;

	return( length );
	}

/* Set a userID */

int setUserID( char *idString, char *argString )
	{
	int length = strlen( argString ) & 0xFF;

	/* Copy argString across if there is one */
	if( length )
		{
		strncpy( idString, argString, length );
		idString[ length ] = '\0';
		}

	return( length );
	}

/* Get all switches and point at the next arg to process

   Flags used/not used:
   Used:     ABCDEF..I.KLMNO..RSTUVWX.Z
   Not used: ......GH.J.....PQ.......Y. */

static void doArg( char *argv[], int *count )
	{
	char *strPtr, ch;
	BYTE lineEndChar;
	BOOLEAN longArg;

	/* Set up various options */
	*basePath = '\0';	/* Default is no output directory */
	initSQL();

	while( TRUE )
		{
		/* Get next arg */
		strPtr = ( char * ) argv[ *count ];
		longArg = *strPtr == '+';
		if( ( *strPtr != '-' ) && !longArg )
			break;

		/* Check for '--' as end of arg list */
		if( !strcmp( strPtr++, "--" ) )
			{
			/* This was the last arg */
			( *count )++;
			break;
			}

		/* Handle long argument format */
		if( longArg )
			{
			/* Universal long options */
			if( !strnicmp( "select", strPtr, 6 ) )
				{
				if( processSqlCommand( strPtr ) == ERROR )
					hputchar( '\n' );	/* Get err.msg away from main text */
				}
			else
			if( !strnicmp( "noext", strPtr, 5 ) )
				/* Don't force '.HPK' extension on archive name */
				sysSpecFlags |= SYSPEC_NOEXT;
			else
			if( !strnicmp( "gopher", strPtr, 5 ) )
				{
				int length;

				/* Treat rest of string as a gopher query */
				strPtr += 5;
				for( length = strlen( strPtr ); length; length-- )
					if( strPtr[ length ] == '\t' )
						{
						/* Truncate the date argument */
						strPtr[ length ] = '\0';
						break;
						}

				/* Add the query as a filespec and turn on gopher mode */
				addFilespec( strPtr );
				sysSpecFlags |= SYSPEC_GOPHER;
				}
			else
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
			if( !stricmp( strPtr, "devcheck" ) )
				/* Check for device filenames on extract */
				sysSpecFlags |= SYSPEC_CHECKSAFE;
			else
				error( UNKNOWN_OPTION, *--strPtr );
#elif defined( __AMIGA__ ) || defined( __OS2__ )
			if( !stricmp( strPtr, "lower" ) )
				{
				/* Force lower case on file and dir names if it would make
				   sense */
				if( choice == VIEW || choice == EXTRACT ||
					choice == TEST || choice == DISPLAY )
					sysSpecFlags |= SYSPEC_FORCELOWER;
				}
			else
				error( UNKNOWN_OPTION, *--strPtr );
#elif defined( __ARC__ )
			if( !stricmp( strPtr, "invert" ) )
				{
				/* Invert filename extensions into directories if it would
				   make sense */
				if( choice == EXTRACT || choice == VIEW || choice == TEST )
					sysSpecFlags |= SYSPEC_INVERTDIR;
				}
			else
			if( !stricmp( strPtr, "lower" ) )
				{
				/* Force lower case on file and dir names if it would make
				   sense */
				if( choice == VIEW || choice == EXTRACT ||
					choice == TEST || choice == DISPLAY )
					sysSpecFlags |= SYSPEC_FORCELOWER;
				}
			else
			if( !stricmp( strPtr, "type" ) )
				/* Add file-type association data */
				addTypeAssociation( strPtr + 5 );
			else
				error( UNKNOWN_OPTION, *--strPtr );
#elif defined( __IIGS__ ) || defined( __MAC__ )
			if( !stricmp( strPtr, "lower" ) )
				{
				/* Force lower case on file and dir names if it would make
				   sense */
				if( choice == VIEW || choice == EXTRACT ||
					choice == TEST || choice == DISPLAY )
					sysSpecFlags |= SYSPEC_FORCELOWER;
				}
			else
			if( !stricmp( strPtr, "type" ) )
				/* Add file-type association data */
				addTypeAssociation( strPtr + 5 );
			else
				error( UNKNOWN_OPTION, *--strPtr );
#elif defined( __UNIX__ )
			if( !stricmp( strPtr, "lower" ) )
				{
				/* Force lower case on file and dir names if it would make
				   sense */
				if( choice == VIEW || choice == EXTRACT ||
					choice == TEST || choice == DISPLAY )
					sysSpecFlags |= SYSPEC_FORCELOWER;
				}
			else
			if( !stricmp( strPtr, "noumask" ) )
				/* Ignore umask for file and dir attributes */
				sysSpecFlags |= SYSPEC_NOUMASK;
			else
			if( !stricmp( strPtr, "device" ) )
				{
				/* Treat archive name as device for multipart archive
				   to device */
				sysSpecFlags |= SYSPEC_DEVICE;
				flags |= OVERWRITE_SRC;
				}
			else
				error( UNKNOWN_OPTION, *--strPtr );
#elif defined( __VMS__ )
			if( !stricmp( strPtr, "rsx" ) )
				/* Translate file/dir names to RSX-11 type format */
				sysSpecFlags |= SYSPEC_RSX11;
			else
				error( UNKNOWN_OPTION, *--strPtr );
#else
			error( LONG_ARG_NOT_SUPPORTED );
#endif /* Various system-specific options */

			/* Skip to end of arg */
			strPtr += strlen( strPtr );
			}

		/* Process individual args */
		while( *strPtr )
			{
			ch = *strPtr++;				/* Avoid toupper() side-effects */
			switch( toupper( ch ) )
				{
				case '0':
					flags |= STORE_ONLY;
					break;

				case 'A':
					flags |= STORE_ATTR;
					break;

				case 'B':
					/* Copy the argument across */
					strPtr += setBasePath( strPtr );
					break;

				case 'C':
					flags |= CRYPT;

					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'A':
							cryptFlags |= CRYPT_CKE_ALL;
							break;

						case 'I':
							cryptFlags |= CRYPT_CKE;
							break;

						case 'P':
							ch = *strPtr++;
							ch = toupper( ch );
							if( ch == 'S' )
								{
								cryptFlags |= CRYPT_SEC | CRYPT_PKE_ALL;
								strPtr += setUserID( secUserID, strPtr );
								}
							else
								{
								if( ch == 'A' )
									cryptFlags |= CRYPT_PKE_ALL;
								else
									{
									cryptFlags |= CRYPT_PKE;
									if( ch != 'I' )
										strPtr--;	/* Move back to '\0' */
									}
								strPtr += setUserID( mainUserID, strPtr );
								}
							break;

						case 'S':
							cryptFlags |= CRYPT_SEC | CRYPT_CKE_ALL;
							break;

						default:
							cryptFlags |= CRYPT_CKE_ALL;
							strPtr--;
						}
					break;

				case 'D':
					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'A':
							dirFlags |= DIR_ALLPATHS;
							break;

						case 'C':
							dirFlags |= ( DIR_CONTAIN | DIR_FLATTEN );
							break;

						case 'F':
							dirFlags |= DIR_FLATTEN;
							break;

						case 'M':
							dirFlags |= DIR_MKDIR;
							break;

						case 'N':
							dirFlags |= DIR_NOCREATE;
							break;

						case 'R':
							dirFlags |= DIR_RMDIR;
							break;

						case 'V':
							dirFlags |= DIR_MVDIR;
							break;

						default:
							error( UNKNOWN_DIR_OPTION, *--strPtr  );
						}
					break;

				case 'E':
					flags |= ERROR_RECOVER;
					break;

				case 'F':
					flags |= MOVE_FILES;
					break;

				case 'I':
					flags |= INTERACTIVE;
					break;

				case 'K':
					flags |= OVERWRITE_SRC;
					break;

				case 'L':
					ch = *strPtr++;
					if( toupper( ch ) == 'I' )
						cryptFlags |= CRYPT_SIGN;
					else
						{
						strPtr--;	/* Back up arg.pointer */
						cryptFlags |= CRYPT_SIGN_ALL;
						}
					strPtr += setUserID( signerID, strPtr );
					break;

#ifndef __MSDOS16__
				case 'M':
					flags |= MULTIPART_ARCH;
					break;
#endif /* __MSDOS16__ */

				case 'N':
#ifdef __MSDOS16__
					ndoc( &strPtr );
#endif /* __MSDOS16__ */
					break;

				case 'O':
					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'A':
							overwriteFlags |= OVERWRITE_ALL;
							break;

						case 'N':
							overwriteFlags |= OVERWRITE_NONE;
							break;

						case 'P':
							overwriteFlags |= OVERWRITE_PROMPT;
							break;

						case 'S':
							overwriteFlags |= OVERWRITE_SMART;
							break;

						default:
							error( UNKNOWN_OVERWRITE_OPTION, *--strPtr  );
						}
					break;

				case 'R':
					flags |= RECURSE_SUBDIR;
					break;

				case 'S':
					/* Stuart if you fclose( STDOUT ) to implement this I'll
					   dump ten thousand frogs on you */
					flags |= STEALTH_MODE;
					break;

				case 'T':
					flags |= TOUCH_FILES;
					break;

				case 'U':
					flags |= BLOCK_MODE;
					break;

				case 'V':
					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'D':
							viewFlags |= VIEW_DIRS;
							break;

						case 'F':
							viewFlags |= VIEW_FILES;
							break;

						default:
							error( UNKNOWN_VIEW_OPTION, *--strPtr  );
						}
					break;

				case 'W':
					flags |= ARCH_COMMENT;
					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'A':
							/* ANSI text comment */
							commentType = TYPE_COMMENT_ANSI;
							break;

						case 'G':
							/* GIF graphics comment */
							commentType = TYPE_COMMENT_GIF;
							break;

						default:
							/* Default: Text comment */
							commentType = TYPE_COMMENT_TEXT;
							strPtr--;	/* Correct strPtr value */
						}
					break;

				case 'X':
					/* Only apply outupt translation if it would make sense */
					if( choice == EXTRACT || choice == DISPLAY )
						flags |= XLATE_OUTPUT;

					/* Determine which kind of translation is required */
					ch = *strPtr++;
					switch( toupper( ch ) )
						{
						case 'A':
							xlateFlags |= XLATE_EOL;
							lineEndChar = '\0';
							break;

#if !( defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	   defined( __MSDOS32__ ) || defined( __OS2__ ) )
						case 'C':
							xlateFlags |= XLATE_EOL;
							lineEndChar = '\r' | 0x80;
							break;
#endif /* !( __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

						case 'E':
							xlateFlags |= XLATE_EBCDIC;
							break;

#if !defined( __AMIGA__ ) && !defined( __ARC__ ) && !defined( __UNIX__ )
						case 'L':
							xlateFlags |= XLATE_EOL;
							lineEndChar = '\n';
							break;
#endif /* !( __AMIGA__ || __ARC__ || __UNIX__ ) */

						case 'P':
							xlateFlags |= XLATE_PRIME;
							break;

#ifndef __MAC__
						case 'R':
							xlateFlags |= XLATE_EOL;
							lineEndChar = '\r';
							break;
#endif /* !__MAC__ */

						case 'S':
							xlateFlags = XLATE_SMART;
							break;

						case 'X':
							/* Get line-end-char in hex */
							xlateFlags |= XLATE_EOL;
							lineEndChar = getHexByte( &strPtr );
							break;

						default:
							/* Default: Smart translate - ignore
							   lineEndChar setting */
							xlateFlags = XLATE_SMART;
							strPtr--;	/* Correct strPtr value */
						}

					/* Set up the translation system if necessary */
					initTranslationSystem( lineEndChar );

					break;

				default:
					error( UNKNOWN_OPTION, *--strPtr );
				}
			}
		( *count )++;
		}

#ifdef __OS2__
	/* Set the flag which controls filename truncation depending on whether
	   we are extracting to an HPFS or FAT filesystem */
	destIsHPFS = queryFilesystemType( basePath );
#endif /* __OS2__ */
#ifdef __MAC__
	/* Remember the vRefNum for the path from/to which we are munging files */
	setVolumeRef( basePath, FILE_PATH );
#endif /* __MAC__ */
	}

/* Shortcut check for stealth mode.  This is necessary for proper operation
   in stealth mode before we've had time to check command-line args */

static BOOLEAN checkStealthMode( char *argv[], int argc )
	{
	int argCount, i;
	char ch, *argPtr;

	/* Loop through all args checking for stealth-mode option */
	for( argCount = 0; argCount < argc; argCount++ )
		{
		argPtr = argv[ argCount ];

		/* Continue if we hit an extended option */
		if( *argPtr == '+' )
			continue;

		/* Exit if we run out of switches or hit a '--' switch */
		if( *argPtr != '-' )
			break;
		if( argPtr[ 1 ] == '-' )
			break;

		/* Check each option for the stealth-mode switch.  If we find an
		   option which allows sub-options, skip it and continue (since the
		   suboptions can look like a stealth-mode switch) */
		for( i = 1; argPtr[ i ]; i++ )
			{
			ch = toupper( argPtr[ i ] );
			if( ch == 'B' || ch == 'C' || ch == 'D' || ch == 'L' || ch == 'O' ||
				ch == 'V' || ch == 'W' || ch == 'X' )
				break;
			if( ch == 'S' )
				return( TRUE );
			}
		}

	/* No stealth-mode flag found */
	return( FALSE );
	}

/* Process the default HPACK configuration file */

void doConfigFile( void )
	{
	char *configFileName;
	FD configFileFD;
	WORD oldFlags = flags;

	/* Build path to config file and try and process it */
	configFileName = getFirstKeyPath( getenv( HPACKPATH ), CONFIGFILENAME );
	while( configFileName != NULL )
		{
		/* Try and read the seed from the seedfile */
		if( ( configFileFD = hopen( configFileName, O_RDONLY | S_DENYRDWR ) ) != ERROR )
			{
			hclose( configFileFD );

			/* Turn on stealth mode to suppress informative burble, handle
			   the config file, and exit */
			flags |= STEALTH_MODE;
			processListFile( configFileName );
			flags = oldFlags;
			break;
			}

		/* No luck, try the next search path */
		configFileName = getNextKeyPath();
		}
	}

/****************************************************************************
*																			*
*							Argument Validity Checks						*
*																			*
****************************************************************************/

void checkArgs( void )
	{
	int i;

	/* Check the basePath length */
	if( basePathLen == ERROR )
		error( PATH_s__TOO_LONG, basePath );

	/* Make sure we don't try and use wildcards in the base path */
	if( basePathLen && strHasWildcards( basePath ) )
		error( CANNOT_USE_WILDCARDS_s, basePath );

	/* Make sure there's a signerID given if we need to authenticate data */
	if( ( cryptFlags & ( CRYPT_SIGN | CRYPT_SIGN_ALL ) ) && !*signerID )
		error( MISSING_USERID );

	/* If we're performing public-key encryption with a secondary userID,
	   make sure the user has also entered a primary userID */
	if( !( cryptFlags ^ ( CRYPT_SEC | CRYPT_PKE_ALL ) ) && !*mainUserID )
		error( MISSING_USERID );

	/* We can't delete the archive if we're meant to be updating it */
	if( ( flags & OVERWRITE_SRC ) &&
		choice == FRESHEN || choice == REPLACE || choice == UPDATE )
		error( CANNOT_CHANGE_DEL_ARCH );

	/* We can't perform update operations on multipart archives */
	if( ( flags & MULTIPART_ARCH ) &&
		( choice == DELETE || choice == FRESHEN || choice == REPLACE ||
		  choice == UPDATE || ( choice == EXTRACT && ( flags & MOVE_FILES ) ) ) )
		error( CANNOT_CHANGE_MULTIPART_ARCH );

	/* If we're encrypting, make sure we're not trying to encrypt using two
	   different methods */
	if( flags & CRYPT )
		{
		i = cryptFlags & ( CRYPT_CKE | CRYPT_PKE | CRYPT_CKE_ALL | CRYPT_PKE_ALL );
		if( i != CRYPT_CKE && i != CRYPT_PKE &&
			i != CRYPT_CKE_ALL && i != CRYPT_PKE_ALL )
			error( CANNOT_USE_BOTH_CKE_PKE );
		}

	/* Only apply the move command if it would make sense (protect the user
	   from performing a move on Display or Test) */
	if( ( flags & MOVE_FILES ) &&
		choice != ADD && choice != FRESHEN && choice != EXTRACT &&
		choice != REPLACE && choice != UPDATE )
		flags &= ~MOVE_FILES;

	/* Turn on multipart writes if we're adding to an archive in multipart
	   mode */
	if( ( flags & MULTIPART_ARCH ) && ( choice == ADD ) )
		multipartFlags |= MULTIPART_WRITE;
	}

/****************************************************************************
*																			*
*								Main Program Code							*
*																			*
****************************************************************************/

#if !( defined( __IIGS__ ) || defined( __MAC__ ) || defined( __MSDOS16__ ) )

/* Handle program interrupt */

#include <signal.h>

static void progBreak( int dummyValue )
	{
#ifndef __AMIGA__
	dummyValue = dummyValue;	/* Get rid of used but not defined warning */
#endif /* !__AMIGA__ */
	error( STOPPED_AT_USER_REQUEST );
	}
#endif /* !( __IIGS__ || __MAC__ || __MSDOS16__ ) */

#ifdef __MAC__					/* Needed for command-line processing */
  #include <console.h>
#endif /* __MAC__ */

/* The main program */

int main( int argc, char *argv[] )
	{
	int no, count = 1, lastSlash;
	FILEINFO archiveInfo;
	char archivePath[ MAX_PATH ], fileCode[ MATCH_DEST_LEN ], ch;
	BOOLEAN createNew, nothingDone = TRUE, hasFileSpec = FALSE;
	BOOLEAN gotPassword = FALSE, hasWildCards;

#ifdef __MAC__
	/* Get command-line parameters */
	argc = ccommand( &argv );
#endif /* __MAC__ */

	/* Perform general initialization.  The following two/three calls must be
	   made pretty promptly since they enable the handling of various
	   resources like memory buffers and text messages */
#if defined( __MSDOS16__ )
	initMem();		/* Init mem.mgr - must come before all other inits */
#endif /* __MSDOS16__ */
	if( ( initFastIO() == ERROR ) ||
		( initMessages( argv[ 0 ], argc < 3 ) == ERROR ) )
		/* The following message must be hardwired into the code since the
		   message system hasn't been initialised yet */
		{
		hputs( "Panic: Cannot read internationalization information.  Please ensure that" );
#if defined( __AMIGA__ )
		hputs( "       the language data file 'language.dat' is in the 's:' directory." );
#elif defined( __ARC__ )
		hputs( "       the language data file 'hpack_lang' is in the same directory as" );
		hputs( "       the HPACK executable." );
#elif defined( __MAC__ )
		hputs( "       the language data file 'HPACK Language Data' is in the same place" );
		hputs( "       as the HPACK executable." );
#elif defined( __UNIX__ )
		hputs( "       the language data file 'language.dat' is in your $PATH along with" );
		hputs( "       the HPACK executable." );
#else
		hputs( "       the language data file 'language.dat' is in the same directory as" );
		hputs( "       the HPACK executable." );
#endif /* System-specific language.dat error messages */
		exit( 1 );		/* Status level 1 = Internal error */
		}

	/* Spew forth some propaganda */
	if( !checkStealthMode( argv + 2, argc - 2 ) )
		showTitle();
#if !( defined( __IIGS__ ) || defined( __MAC__ ) || defined( __MSDOS16__ ) )
	/* Trap program break */
  #ifdef __UNIX__
	if( !isatty( 0 ) )
		/* If we're running in the background, turn on stealth mode so we
		   don't swamp the (l)user with noise */
		flags |= STEALTH_MODE;
	else
  #endif /* __UNIX__ */
	signal( SIGINT, progBreak );
#endif /* !( __IIGS__ || __MAC__ || __MSDOS16__ ) */

	/* Handle various environment-dependant configuration options */
#ifndef __OS2__
	initExtraInfo();
#endif /* __OS2__ */
	setDateFormat();
	getScreenSize();

	/* Only initialise the compressor/decompressor if we have to.
	   Unfortunately we need to initialise the decompressor for the VIEW
	   command since there may be compressed archive comments in the data */
	if( ( argc > 1 ) && ( ch = toupper( *argv[ 1 ] ) ) != DELETE )
		/* Only allocate the memory for the compressor if we have to */
		initPack( ch != EXTRACT && ch != TEST && ch != DISPLAY && ch != VIEW );

	initCrypt();
	initArcDir();	/* Must be last init in MSDOS version */

	/* If there are not enough command-line args, exit with a help message */
	if( argc < 3 )
		{
		showHelp();
		exit( OK );
		}

	/* First process command.  The order is: Main command, configuration
	   file, options (which can override config file options) */
	doCommand( argv[ 1 ] );
	count++;
	doConfigFile();
	doArg( argv, &count );
	createNew = ( choice == ADD && ( flags & OVERWRITE_SRC ) ) ? TRUE : FALSE;
										/* Common subexpr.elimination */

	/* Allow room in path for '.xxx\0' suffix */
	if( argv[ count ] == NULL )
		strcpy( archiveFileName, "*" );
	else
		if( strlen( argv[ count ] ) > MAX_PATH - 5 )
			error( PATH_s_TOO_LONG, argv[ count ] );
		else
			strcpy( archiveFileName, argv[ count++ ] );

	/* Extract the pathname and convert it into an OS-compatible format */
	lastSlash = extractPath( archiveFileName, archivePath );
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	archiveDrive = ( archiveFileName[ 1 ] == ':' ) ? toupper( *archiveFileName ) - 'A' + 1 : 0;
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */

	/* Either append archive file suffix to the filespec or force the
	   existing suffix to HPAK_MATCH_EXT, and point the variable no to the
	   end of the filename component */
	for( no = strlen( archiveFileName );
		 no >= lastSlash && archiveFileName[ no ] != '.';
		 no-- );
#ifdef __ARC__
	/* No extensions on the Archimedes, don't try anything clever */
	no = strlen( archiveFileName );
#else
  #ifdef __UNIX__
	if( !( sysSpecFlags & SYSPEC_DEVICE ) )
		/* Only force the HPACK extension if it's a normal file */
  #endif /* __UNIX__ */
	if( sysSpecFlags & SYSPEC_NOEXT )
		/* No extension required, simply don't try anything clever */
		no = strlen( archiveFileName );
	else
		if( archiveFileName[ no ] != '.' )
			{
			no = strlen( archiveFileName );
			strcat( archiveFileName, HPAK_MATCH_EXT );
			}
		else
			strcpy( archiveFileName + no, HPAK_MATCH_EXT );
#endif /* __ARC__ */

#ifdef __OS2__
	/* Make sure we get the case-mangling right */
	isHPFS = queryFilesystemType( archiveFileName );
#endif /* __OS2__ */
#ifdef __MAC__
	/* Remember the vRefNum for the archive path.  In addition we strip off
	   the volume name, since if we're creating a multidisk archive the
	   volume name may change with each disk */
	setVolumeRef( archiveFileName, ARCHIVE_PATH );
	no -= stripVolumeName( archivePath );
	lastSlash -= stripVolumeName( archiveFileName );
#endif /* __MAC__ */

	/* Compile the filespec into a form acceptable by the wildcard-matching
	   finite-state machine */
	if( ( ch = compileString( archiveFileName + lastSlash, fileCode ) ) < 0 )
		error( ( ch == WILDCARD_FORMAT_ERROR ) ?
			   BAD_WILDCARD_FORMAT : WILDCARD_TOO_COMPLEX, archiveFileName + lastSlash );

	/* Now add names of files to be processed to the fileName list */
	while( count < argc )
		{
		if( *argv[ count ] == '@' )
			{
			/* Assemble the listFile name + path into the dirBuffer (we can't
			   use the mrglBuffer since processListFile() ovrewrites it) */
			strcpy( ( char * ) dirBuffer, argv[ count++ ] + 1 );
			lastSlash = extractPath( ( char * ) dirBuffer, ( char * ) dirBuffer + 80 );
			if( ( ch = compileString( ( char * ) dirBuffer + lastSlash, ( char * ) dirBuffer + 80 ) ) < 0 )
				error( ( ch == WILDCARD_FORMAT_ERROR ) ?
					   BAD_WILDCARD_FORMAT : WILDCARD_TOO_COMPLEX, dirBuffer + lastSlash );
			hasWildCards = strHasWildcards( ( char * ) dirBuffer + 80 );
#if defined( __AMIGA__ ) || defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __MAC__ ) || defined( __OS2__ )
			if( lastSlash && ( ch = dirBuffer[ lastSlash - 1 ] ) != SLASH &&
															  ch != ':' )
#else
			if( lastSlash && ( ch = dirBuffer[ lastSlash - 1 ] ) != SLASH )
#endif /* __AMIGA__ || __ATARI__ || __MAC__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
				/* Add a slash if necessary */
				dirBuffer[ lastSlash - 1 ] = SLASH;
			strcpy( ( char * ) dirBuffer + lastSlash, MATCH_ALL );

			/* Check each file in the directory, passing those that match to
			   processListFile() */
			if( findFirst( ( char * ) dirBuffer, FILES, &archiveInfo ) )
				{
				do
					{
					strcpy( ( char * ) dirBuffer + lastSlash, archiveInfo.fName );
					if( matchString( ( char * ) dirBuffer + 80, archiveInfo.fName, hasWildCards ) )
						processListFile( ( char * ) dirBuffer );
					}
				while( findNext( &archiveInfo ) );
				findEnd( &archiveInfo );
				}
			}
		else
			addFilespec( ( char * ) argv[ count++ ] );
		hasFileSpec = TRUE;
		}

	/* Now that we've handled all command-line args and scripts, perform
	   some general arg checking and set SQL defaults if they're not
	   already set */
	checkArgs();
	setDefaultSql();

	/* Default is all files if no name is given */
	if( !( hasFileSpec || ( sysSpecFlags & SYSPEC_GOPHER ) ) )
		addFilespec( WILD_MATCH_ALL );

	/* See if we can find a matching archive.  The only case where a non-
	   match is not an error is if we are using the ADD command:  In this
	   case we create a new archive */
#if defined( __AMIGA__ ) || defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	if( lastSlash && ( ch = archivePath[ lastSlash - 1 ] ) != SLASH &&
														ch != ':' )
#else
	if( lastSlash && archivePath[ lastSlash - 1 ] != SLASH )
#endif /* __AMIGA__ || __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
		/* Add a slash if necessary */
		archivePath[ lastSlash - 1 ] = SLASH;
	strcpy( archivePath + lastSlash, ( sysSpecFlags & SYSPEC_NOEXT ) ?
									 MATCH_ALL : MATCH_ARCHIVE );
	if( !findFirst( archivePath, FILES, &archiveInfo ) )
#ifdef __ARC__
		/* The Arc findFirst() is very picky about matching only archives, so
		   we can't set createNew because we may have found a matching file
		   which isn't an archive */
		if( choice != ADD )
			error( NO_ARCHIVES );
#else
		if( choice == ADD && !hasWildcards( archiveFileName + lastSlash, no - lastSlash ) )
			/* No existing archive found, force the creation of a new one */
			createNew = TRUE;
		else
			error( NO_ARCHIVES );
#endif /* __ARC__ */

	/* Now process each archive which matches the given filespec */
again:
	do
		if( createNew || matchString( fileCode, archiveInfo.fName, TRUE ) )
			{
			if( createNew )
				{
#ifdef __UNIX__
				if( !( sysSpecFlags & SYSPEC_DEVICE ) )
					/* Only force the HPACK extension if it's a normal file */
#endif /* __UNIX__ */
				/* Set the extension to the proper form */
				if( !( sysSpecFlags & SYSPEC_NOEXT ) )
					strcpy( archiveFileName + no, HPAK_EXT );
				strcpy( errorFileName, archiveFileName );
				}
			else
				{
				/* Set archivePath as well as archiveFileName to the archive's
				   name, since when processing multiple archives archiveFileName
				   will be stomped */
				strcpy( archivePath + lastSlash, archiveInfo.fName );
				strcpy( archiveFileName, archivePath );
				if( addArchiveName( archiveInfo.fName ) )
					/* Make sure we don't try to process the same archive
					   twice (see the comment for addArchiveName) */
					continue;
				}

			hprintfs( MESG_ARCHIVE_IS_s, archiveFileName );

			/* Reset error stats and arcdir system */
			archiveFD = errorFD = dirFileFD = secFileFD = IO_ERROR;
			oldArcEnd = 0L;
			resetArcDir();
			overWriteEntry = FALSE;
			if( flags & BLOCK_MODE )
				firstFile = TRUE;
			setCurrPosition( 0L );

			/* Open archive file with various types of mungeing depending on the
			   command type */
			if( choice == ADD || choice == UPDATE )
				{
				/* Create a new archive if none exists or overwrite an existing
				   one if asked for */
				if( createNew )
					{
					errorFD = archiveFD = hcreat( archiveFileName, CREAT_ATTR );
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
							oldArcEnd = fileHdrCurrPtr->offset + fileHdrCurrPtr->data.dataLen +
										fileHdrCurrPtr->data.auxDataLen;
						getArcdirState( ( FILEHDRLIST ** ) &oldHdrlistEnd, &oldDirEnd );

						/* Move past existing data and update current
						   position pointer in archive */
						vlseek( oldArcEnd, SEEK_SET );
						setCurrPosition( oldArcEnd + HPACK_ID_SIZE );
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
					readArcDir( choice != VIEW && choice != EXTRACT &&
								choice != TEST && choice != DISPLAY  );

				/* The password will have been obtained as part the process
				   of reading the archive directory */
				gotPassword = TRUE;
				}

			/* Make sure the open was successful */
			if( archiveFD == IO_ERROR )
				error( CANNOT_OPEN_ARCHFILE, archiveFileName );

			/* We've found an archive to ADD things to, so signal the fact
			   that we don't need to make a second pass with the createNew
			   flag set */
			nothingDone = FALSE;

			/* Set up the password(s) if necessary */
			if( !gotPassword && ( cryptFlags & ( CRYPT_CKE | CRYPT_CKE_ALL ) ) )
				{
				initPassword( MAIN_KEY );
				if( cryptFlags & CRYPT_SEC )
					initPassword( SECONDARY_KEY );

				/* Flag the fact that we no longer need to get the password
				   later on if there are more archives */
				gotPassword = TRUE;
				}

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

			/* The EXTRACT command with MOVE_FILES option is just a
			   combination of the standard EXTRACT and DELETE commands.  We
			   handle this by making a second pass which deletes the files.
			   This could be done in one step by combining the EXTRACT and
			   DELETE, but doing it this way is much easier since the two
			   normally involve several mutually exclusive alternatives */
			if( choice == EXTRACT && ( flags & MOVE_FILES ) )
				{
				choice = DELETE;
				oldArcEnd = 0L;			/* Don't try any recovery */
				vlseek( 0L, SEEK_SET );	/* Go back to start of archive */
				handleArchive();
				}

			/* Perform cleanup functions for this file */
			if( choice != DELETE && choice != FRESHEN && choice != REPLACE &&
				!( choice == EXTRACT && ( flags & MOVE_FILES ) ) )
				/* If DELETE, FRESHEN, or REPLACE, archive file is already closed */
				{
				hclose( archiveFD );
				archiveFD = IO_ERROR;	/* Mark it as invalid */
				}

			/* Perform any updating of extra file information on the archive
			   if we've changed it, unless it's a multipart archive which
			   leads to all sorts of complications */
			if( archiveChanged && !( flags & MULTIPART_ARCH ) &&
				!( choice == VIEW || choice == TEST || choice == EXTRACT || choice == DISPLAY ) )
				setExtraInfo( archiveFileName );

			/* Now that we've finished all processing, delete any files we've
			   added if the MOVE_FILES option was used */
			if( archiveChanged && ( flags & MOVE_FILES ) )
				wipeFilePaths();

#if defined( __MSDOS16__ )
			/* Reset arcDir filesystem in memory */
			resetArcDirMem();
#endif /* __MSDOS16__ */

			/* Only go through the loop once if we are either creating a new
			   archive or processing a multipart archive */
			if( createNew || flags & MULTIPART_ARCH )
				break;
			}
	while( findNext( &archiveInfo ) );

	/* There were no files found to ADD to, so we execute the above loop one
	   more time and create a new archive.  We have to be very careful on
	   the Archimedes since we may have found something which isn't an
	   archive but which matches the given filespec, so we only try again
	   with createNew set if there isn't anything there which could be
	   overwritten */
#ifdef __ARC__
	if( choice == ADD && nothingDone &&
		!hasWildcards( archiveFileName + lastSlash, no - lastSlash ) &&
		!findFirst( archiveFileName, FILES_DIRS, &archiveInfo ) )
#else
	if( choice == ADD && nothingDone )
#endif /* __ARC__ */
		{
		createNew = TRUE;
		goto again;
		}

	/* Finish findFirst() functions */
	findEnd( &archiveInfo );

	/* Complain if we didn't end up doing anything */
	if( !archiveChanged )
		if( choice == FRESHEN )
			{
			/* If we were freshening an archive then the nonexistance of
			   files to change isn't necessarily a problem */
			hputs( MESG_ARCH_IS_UPTODATE );
			closeFiles();
			}
		else
			/* There was nothing to do to the archive, treat as an error
			   condition */
			if( nothingDone )
				/* Couldn't find a matching archive */
				error( NO_ARCHIVES );
			else
				if( choice == ADD || choice == REPLACE )
					/* Couldn't find a matching file to add */
					error( NO_FILES_ON_DISK );
				else
					/* Couldn't find a matching file in the archive */
					error( NO_FILES_IN_ARCHIVE );

	/* Clean up before exiting.  Note that the final message must be printed
	   before most of the cleanup functions are called as these will delete
	   the message database */
	if( choice == VIEW )
		showTotals();
	hprintfs( MESG_DONE );
	endCrypt();
#if defined( __MSDOS16__ )
	endMem();		/* The mem.mgr.takes care of all memory freeing */
#else
	if( choice != DELETE )
		endPack();
	endArcDir();
	endExtraInfo();
	endFastIO();
	endMessages();
	freeFilespecs();
	freeArchiveNames();
#endif /* __MSDOS16__ */

	/* Obesa Cantavit */
#ifdef __VMS__
	return( translateVMSretVal( OK ) );
#else
	return( OK );
#endif /* __VMS__ */
	}
