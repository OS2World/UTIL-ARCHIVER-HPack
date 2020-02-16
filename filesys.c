/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						Filesystem-Specific Support Routines				*
*							FILESYS.C  Updated 03/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "error.h"
#include "filesys.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#include "tags.h"
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

#ifdef __OS2__

/* Prototypes for functions in OS2.C */

void setAccessTime( const char *filePath, const LONG timeStamp );
void setCreationTime( const char *filePath, const LONG timeStamp );
void addLongName( const char *filePath, const char *longName );
void setEAinfo( const char *filePath, const TAGINFO *tagInfo, const FD srcFD );

#endif /* __OS2__ */

/* Take a full pathname, convert it to an OS-compatible format, and extract
   out the path component of the name */

int extractPath( char *fullPath, char *pathName )
	{
#if defined( __AMIGA__ ) || defined( __ATARI__ ) || defined( __MAC__ ) || \
	defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) || \
	defined( __VMS__ )
	BOOLEAN hasDeviceSpec = FALSE;
#endif /* __AMIGA__ || __ATARI__ || __MAC__ || __MSDOS16__ || __MSDOS32__ ||__OS2__ || __VMS__ */
	int count, lastSlash = ERROR;

#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	/* Check whether there is a drive specifier prepended to the path */
	if( fullPath[ 1 ] == ':' )
		{
		lastSlash = 1;
		hasDeviceSpec = TRUE;
		}
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
#pragma run "/usr/games/moria"

	/* Separate the path and filename components, converting directory
	   seperators into an OS-compatible format at the same time */
	for( count = 0; fullPath[ count ]; count++ )
		{
		fullPath[ count ] = caseConvert( fullPath[ count ] );
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
		if( fullPath[ count ] == '\\' )
			fullPath[ count ] = SLASH;
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */
#if defined( __AMIGA__ ) || defined( __MAC__ ) || defined( __VMS__ )
		if( fullPath[ count ] == ':' )
			{
			hasDeviceSpec = TRUE;
			lastSlash = count;
			}
#endif /* __AMIGA__ || __ARC__ || __MAC__ || __VMS__ */
		if( fullPath[ count ] == SLASH )
			lastSlash = count;
		}

	/* If there is only a device/node spec or only a device/node spec and a
	   slash we must handle it differently than a normal path:  We can't
	   stomp the char at lastSlash as we would for a normal path */
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	if( hasDeviceSpec && ( lastSlash == 1 || lastSlash == 2 ) )
		{
		strncpy( pathName, fullPath, ++lastSlash );
		pathName[ lastSlash ] = '\0';
		}
	else
#elif defined( __AMIGA__ ) || defined( __MAC__ ) || defined( __VMS__ )
	if( hasDeviceSpec && lastSlash != ERROR &&
		( fullPath[ lastSlash ] == ':' || ( lastSlash && fullPath[ lastSlash - 1 ] == ':' ) ) )
		{
		strncpy( pathName, fullPath, ++lastSlash );
		pathName[ lastSlash ] = '\0';
		}
	else
#endif /* Various OS-specific drive handling */
		if( lastSlash > 0 )
			{
			/* Create the pathname, stomping the last SLASH for the
			   directory-handling functions */
			strncpy( pathName, fullPath, lastSlash );
			pathName[ lastSlash++ ] = '\0';
			}
		else
			{
			if( !lastSlash )
				/* Special case:  File is off the root dir */
				*pathName = SLASH;
			pathName[ ++lastSlash ] = '\0';
			}

	return( lastSlash );
	}

/* Find the filename component of a given filePath */

char *findFilenameStart( char *filePath )
	{
	int lastSlash = 0, i = 0;

	/* Skip prepended device/node specifier if necessary */
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	if( filePath[ 1 ] == ':' )
		filePath += 2;
#elif defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __MAC__ ) || defined( __VMS__ )
	while( filePath[ i ] )
		if( filePath[ i++ ] == ':' )
			{
			filePath += i;
			break;	/* Exit after first colon */
			}
#endif /* Various OS-specific device/node specifier skips */

	/* Find the first char after the last SLASH */
	for( i = 0; filePath[ i ]; i++ )
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || defined( __OS2__ )
		if( ( filePath[ i ] == SLASH ) || ( filePath[ i ] == '\\' ) )
#else
		if( filePath[ i ] == SLASH )
#endif /* __ATARI__ || __MSDOS16__ || __OS2__ */
			lastSlash = i + 1;	/* +1 to skip SLASH */

	return( filePath + lastSlash );
	}

#if 0
/* Using a given filename, build a path to the directory for temporary files */

BOOLEAN makeTempPath( const char *fileName, char *tempPath )
	{
	char *tmpPath;

	/* Build path to keyring file */
	*tempPath = '\0';
	if( ( tmpPath = getenv( TMPPATH ) ) != NULL )
		{
		if( strlen( tmpPath ) + fileName > MAX_PATH )
			error( PATH_ss_TOO_LONG, tmpPath, fileName );
		strcpy( tempPath, tmpPath );
		if( ( i = tempPath[ strlen( tempPath ) - 1 ] ) != SLASH && i != '\\' )
			/* Add trailing slash if necessary */
			strcat( tempPath, SLASH_STR );
		strcat( tempPath, fileName );
		return( TRUE );
		}

	/* Couldn't create temp path */
	return( FALSE );
	}
#endif /* 0 */

/* Check whether a given directory exists */

static BOOLEAN dirnameConflict;		/* Whether there is a file/directory
									   name conflict */
BOOLEAN dirExists( char *pathName )
	{
	FILEINFO pathInfo;
	BOOLEAN retVal = FALSE;
	char dirPath[ MAX_PATH ];
#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __ATARI__ ) || \
	defined( __MAC__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDSO32__ ) || defined( __OS2__ )
	int strEnd;
#endif /* __AMIGA__ || __ARC__ || __ATARI__ || __MAC__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */

	dirnameConflict = FALSE;

	/* Convert the path into an OS-compatible format */
	extractPath( pathName, dirPath );

#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __ATARI__ ) || \
	defined( __MAC__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
	/* If the pathName is a drive spec only, we need to add a dummy arg.
	   The test for whether we need to add the MATCH_ALL is safe since
	   extractPath will only leave a trailing slash if the path component
	   is a pure drive spec.  When we do the findFirst() we will always
	   match ".", so there is no need for special-case processing for this */
	strEnd = strlen( dirPath ) - 1;
	if( dirPath[ strEnd ] == ':' || dirPath[ strEnd ] == SLASH )
		strcat( dirPath, MATCH_ALL );
#endif /* __AMIGA__ || __ARC__ || __ATARI__ || __MAC__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */

	/* See if we can find the given directory */
	if( findFirst( dirPath, ALLFILES_DIRS, &pathInfo ) )
		retVal = TRUE;
	findEnd( &pathInfo );

	/* Make sure what we've found is a directory */
	if( retVal && !isDirectory( pathInfo ) )
		/* A filesystem entity of this name already exists */
		dirnameConflict = retVal = TRUE;

	return( retVal );
	}

/* Filter out illegal chars.  This is virtually identical for the Amiga,
   Atari ST, MSDOS, OS/2, and Unix, the only differences being '#' for the
   Amiga, ':', ';', '+', ',', '=' for the Atari ST, DOS and OS/2, and '&',
   '!' for Unix.  The Mac only has ':' as an illegal char, the Arc has its
   own wierd selection, and VMS has everything except '_' illegal.  Note that
   the space char must be the first one in the exclusion list if it's used */

#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  #define BAD_CHARS			" \"*+,/:;<=>?[\\]|"
#elif defined( __AMIGA__ )
  #define BAD_CHARS			"\"#$/:<>?|"
#elif defined( __ARC__ )
  #define BAD_CHARS			" \"#$%&*.:^"
#elif defined( __ATARI__ )
  #define BAD_CHARS			" \"*+,/:;<=>?[\\]|"
#elif defined( __MAC__ )
  #define BAD_CHARS			":"
#elif defined( __OS2__)
  #define BAD_CHARS			" \"*+,/:;<=>?[\\]|"
#elif defined( __UNIX__ )
  #define BAD_CHARS			" !\"$&'*/;<>?[\\]`{|}"
#elif defined( __VMS__ )
  #define BAD_CHARS			" !\"#$%&'()*+,-/:;<=>?@[\\]^`{|}~"
#else
  #error "Need to define BAD_CHARS in FILESYS.C"
#endif /* Various OS-specific illegal filename chars */

static inline char xlateChar( const char ch )
	{
	int i;

	/* Simple rejection check for non basic ASCII characters */
	if( ch < ' ' || ch > '~' )
		return( '_' );

	/* Check for character in exclusion list.  OS/2 sets SPACE_OK
	   dynamically depending on the filesystem in use */
	for( i = sizeof( BAD_CHARS ) - 2; i >= SPACE_OK; i-- )
		if( ch == BAD_CHARS[ i ] )
			return( '_' );

	return( ch );
	}

/* Intelligently truncate a filename to eight-dot-three format for MSDOS,
   MAX_FILENAME for the Amiga, the Mac, and Unix, or eight-dot-three/
   MAX_FILENAME for OS/2 FAT/HPFS.  The MSDOS version goes to great lengths
   to produce an optimal translation (ie the best a human could do); the
   Amiga/Mac/Unix versions don't do so well due to the more free-form nature
   of Amiga/Mac/Unix filenames.  In any case suffixes of up to three
   characters are preserved (.c, .h, .cpp), otherwise the name is truncated
   at MAX_FILENAME.  The VMS one is somewhat complicated since we need to
   truncate either to a rather limited 39.39 format or the canonical RSX-11
   format, with additional complications if the pathname component is a
   directory.  The Arc one just truncates down to 10 chars with no extensions,
   which is the ADFS limit and not necessarily imposed by an IFS.  However
   under Risc-OS 2 there is no way to tell exactly what to truncate to so we
   use the worst-case length */

#ifdef __VMS__
  static BOOLEAN isDirComponent;	/* dir.components get handled specially */
#endif /* __VMS__ */

static BOOLEAN fileNameTruncated;	/* Used by isTruncated() */

static char *truncFileName( char *fileName )
	{
	int i, lastDit, fileNameLength = strlen( fileName );
	static char truncName[ MAX_FILENAME ];
#if defined( __AMIGA__ ) || defined( __IIGS__ ) || defined( __ATARI__ ) || \
	defined( __MAC__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ ) || defined( __UNIX__ ) || \
	defined( __VMS__ )
	int j;
#endif /* __AMIGA__ || __ATARI__ || __IIGS__ || __MAC__ || __MSDOS16__ || __MSDOS32__ || __OS2__ || __UNIX__ || __VMS__ */
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ ) || defined( __VMS__ )
	char ch;
#endif /* __ATARI__ || __MSDOS16__ || __MSDOS32__ || __OS2__ || __VMS__ */
#ifdef __VMS__
	int versionNoStart = 0;
#endif /* __VMS__ */

	/* Sanity check in case we're called with a dummy filename */
	if( !fileNameLength )
		return( "" );

#ifdef __VMS__
	if( !isDirComponent )
		/* Prescan the filename for a version number suffix */
		for( i = fileNameLength - 1; i; i-- )
			if( fileName[ i ] == ';' )
				{
				/* Found a possible suffix, check it's valid */
				for( j = i + 1; j < fileNameLength; j++ )
					if( fileName[ j ] < '0' || fileName[ j ] > '9' )
						break;

				/* If it's a valid suffix, don't include it in
				   further calculations */
				if( j == fileNameLength )
					{
					versionNoStart = fileNameLength = i;
					break;
					}
				}
#endif /* __VMS__ */

	/* Find last dit */
	for( lastDit = fileNameLength - 1; lastDit && fileName[ lastDit ] != '.';
		 lastDit-- );
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ )
	if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
		lastDit = fileNameLength;

	/* Copy across first 8 chars or up to lastDit */
	for( i = 0; i < 8 && i < lastDit && i < fileNameLength; i++ )
		{
		ch = truncName[ i ] = xlateChar( fileName[ i ] );
		if( ch == '.' )
			truncName[ i ] = '_';
		}

	/* Add dit and continue if necessary */
	if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' )
		{
		j = i;
		truncName[ i++ ] = '.';

		/* Check for no dit present.  Note that we also check for the
		   special case of the dit being at MAX_FILENAME since the
		   lastDit loop can stop either at fileNameLength or on a dit */
		if( lastDit == MAX_FILENAME && fileName[ MAX_FILENAME ] != '.' )
			{
			/* No dit, copy across next 3 chars */
			while( i < MAX_FILENAME - 1 && i < fileNameLength )
				truncName[ i++ ] = xlateChar( fileName[ j++ ] );
			}
		else
			{
			/* Copy across last 1-3 chars after dit */
			for( j = 0; fileName[ fileNameLength - 1 ] != '.' && j < 3 &&
				 i <= fileNameLength; fileNameLength--, j++ );
			while( j-- )
				truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
			}
		}
#elif defined( __AMIGA__ ) || defined( __UNIX__ )
	if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
		{
		lastDit = fileNameLength;
		j = MAX_FILENAME - 1;
		}
	else
		/* Make sure we preserve at least 3 characters of suffix */
		j = ( MAX_FILENAME - 1 ) -
			( ( fileNameLength - lastDit > 3 ) ? 4 : fileNameLength - lastDit );

	/* Copy across appropriate no.of chars or up to lastDit */
	for( i = 0; i < j && i < lastDit && i < fileNameLength; i++ )
		truncName[ i ] = xlateChar( fileName[ i ] );

	/* Add dit and continue if necessary */
	if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' &&
		i < MAX_FILENAME && fileNameLength != lastDit )
		{
		truncName[ i++ ] = '.';

		/* Copy across last 1-3 chars after dit */
		for( j = 0; fileName[ fileNameLength - 1 ] != '.' &&
			 i <= fileNameLength; fileNameLength--, j++ );
		if( j > 3 && fileNameLength == MAX_FILENAME )
			j = 3;		/* Handle overshoot */
		while( j-- )
			truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
		}
#elif defined( __ARC__ )
	/* Just copy across the first 10 chars (the ADFS limit) */
	for( i = 0; i < 10 && i < fileNameLength; i++ )
		truncName[ i ] = xlateChar( fileName[ i ] );
#elif defined( __MAC__ )
	if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
		{
		lastDit = fileNameLength;
		j = MAX_FILENAME - 1;
		}
	else
		/* Make sure we preserve at least 3 characters of suffix */
		j = ( MAX_FILENAME - 1 ) -
			( ( fileNameLength - lastDit > 3 ) ? 4 : fileNameLength - lastDit );

	/* Copy across appropriate no.of chars or up to lastDit */
	for( i = 0; i < j && i < lastDit && i < fileNameLength; i++ )
		truncName[ i ] = xlateChar( fileName[ i ] );

	/* If the name starts with a dit, zap it (device names start with dits) */
	if( truncName[ 0 ] == '.' )
		truncName[ 0 ] = '_';

	/* Add dit and continue if necessary */
	if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' &&
		i < MAX_FILENAME && fileNameLength != lastDit )
		{
		j = i;
		truncName[ i++ ] = '.';

		/* Copy across last 1-3 chars after dit */
		for( j = 0; fileName[ fileNameLength - 1 ] != '.' &&
			 i <= fileNameLength; fileNameLength--, j++ );
		if( j > 3 && fileNameLength == MAX_FILENAME )
			j = 3;		/* Handle overshoot */
		while( j-- )
			truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
		}
#elif defined( __OS2__ )
	if( destIsHPFS )
		{
		/* Truncate to HPFS specs */
		if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
			{
			lastDit = fileNameLength;
			j = MAX_FILENAME - 1;
			}
		else
			/* Make sure we preserve at least 3 characters of suffix */
			j = ( MAX_FILENAME - 1 ) -
				( ( fileNameLength - lastDit > 3 ) ? 4 : fileNameLength - lastDit );

		/* Copy across appropriate no.of chars or up to lastDit */
		for( i = 0; i < j && i < lastDit && i < fileNameLength; i++ )
			truncName[ i ] = xlateChar( fileName[ i ] );

		/* Add dit and continue if necessary */
		if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' &&
			i < MAX_FILENAME && fileNameLength != lastDit )
			{
			j = i;
			truncName[ i++ ] = '.';

			/* Copy across last 1-3 chars after dit */
			for( j = 0; fileName[ fileNameLength - 1 ] != '.' &&
				 i <= fileNameLength; fileNameLength--, j++ );
			if( j > 3 && fileNameLength == MAX_FILENAME )
				j = 3;		/* Handle overshoot */
			while( j-- )
				truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
			}
		}
	else
		{
		/* Truncate to 8.3 */
		if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
			lastDit = fileNameLength;

		/* Copy across first 8 chars or up to lastDit */
		for( i = 0; i < 8 && i < lastDit && i < fileNameLength; i++ )
			{
			ch = truncName[ i ] = xlateChar( fileName[ i ] );
			if( ch == '.' )
				truncName[ i ] = '_';
			}

		/* Add dit and continue if necessary */
		if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' )
			{
			j = i;
			truncName[ i++ ] = '.';

			/* Check for no dit present.  Note that we also check for the
			   special case of the dit being at MAX_FILENAME since the
			   lastDit loop can stop either at fileNameLength or on a dit */
			if( lastDit == MAX_FILENAME && fileName[ MAX_FILENAME ] != '.' )
				{
				/* No dit, copy across next 3 chars */
				while( i < MAX_FILENAME - 1 && i < fileNameLength )
					truncName[ i++ ] = xlateChar( fileName[ j++ ] );
				}
			else
				{
				/* Copy across last 1-3 chars after dit */
				for( j = 0; fileName[ fileNameLength - 1 ] != '.' && j < 3 &&
					 i <= fileNameLength; fileNameLength--, j++ );
				while( j-- )
					truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
				}
			}
		}
#elif defined( __VMS__ )
	/* Check whether we need to truncate to RSX-11 format */
	if( sysSpecFlags & SYSPEC_RSX11 )
		{
		/* If it's a directory, just copy across up to the first 9 chars */
		if( isDirComponent )
			for( i = 0; i < 9 && i < fileNameLength; i++ )
				{
				ch = truncName[ i ] = xlateChar( fileName[ i ] );
				if( ch == '.' )
					truncName[ i ] = '_';
				}
		else
			{
			/* Truncate to 9.3 */
			if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
				lastDit = fileNameLength;

			/* Copy across first 9 chars or up to lastDit */
			for( i = 0; i < 9 && i < lastDit && i < fileNameLength; i++ )
				{
				ch = truncName[ i ] = xlateChar( fileName[ i ] );
				if( ch == '.' )
					truncName[ i ] = '_';
				}

			/* Add dit and continue if necessary */
			if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' )
				{
				j = i;
				truncName[ i++ ] = '.';

				/* Check for no dit present.  Note that we also check for the
				   special case of the dit being at MAX_FILENAME since the
				   lastDit loop can stop either at fileNameLength or on a dit */
				if( lastDit == MAX_FILENAME && fileName[ MAX_FILENAME ] != '.' )
					{
					/* No dit, copy across next 3 chars */
					while( i < MAX_FILENAME - 1 && i < fileNameLength )
						truncName[ i++ ] = xlateChar( fileName[ j++ ] );
					}
				else
					{
					/* Copy across last 1-3 chars after dit */
					for( j = 0; fileName[ fileNameLength - 1 ] != '.' && j < 3 &&
						 i <= fileNameLength; fileNameLength--, j++ );
					while( j-- )
						truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
					}
				}
			}
		}
	else
		{
		/* If it's a directory, just copy across up to the first 39 chars */
		if( isDirComponent )
			for( i = 0; i < 39 && i < fileNameLength; i++ )
				{
				ch = truncName[ i ] = xlateChar( fileName[ i ] );
				if( ch == '.' )
					truncName[ i ] = '_';
				}
		else
			{
			/* Truncate to 39.39 */
			if( !lastDit || fileName[ fileNameLength - 1 ] == '.' )
				lastDit = fileNameLength;

			/* Copy across first 39 chars or up to lastDit */
			for( i = 0; i < 39 && i < lastDit && i < fileNameLength; i++ )
				{
				ch = truncName[ i ] = xlateChar( fileName[ i ] );
				if( ch == '.' )
					truncName[ i ] = '_';
				}

			/* Add dit and continue if necessary */
			if( i < fileNameLength && fileName[ fileNameLength - 1 ] != '.' )
				{
				j = i;
				truncName[ i++ ] = '.';

				/* Check for no dit present.  Note that we also check for the
				   special case of the dit being at MAX_FILENAME since the
				   lastDit loop can stop either at fileNameLength or on a dit */
				if( lastDit == MAX_FILENAME && fileName[ MAX_FILENAME ] != '.' )
					{
					/* No dit, copy across next 39 chars */
					while( i < MAX_FILENAME - 1 && i < fileNameLength )
						truncName[ i++ ] = xlateChar( fileName[ j++ ] );
					}
				else
					{
					/* Copy across last 1-39 chars after dit */
					for( j = 0; fileName[ fileNameLength - 1 ] != '.' && j < 39 &&
						 i <= fileNameLength; fileNameLength--, j++ );
					while( j-- )
						truncName[ i++ ] = xlateChar( fileName[ fileNameLength++ ] );
					}
				}
			}
		}

	/* Finally, append the version number if there is one */
	if( versionNoStart )
		while( fileName[ fileNameLength ] )
			truncName[ i++ ] = fileName[ fileNameLength++ ];
#else
	#error "Need to implement truncFileName() in FILESYS.C"
#endif /* Various OS-specific defines */
	truncName[ i ] = '\0';

	fileNameTruncated = strcmp( fileName, truncName );
	return( truncName );
	}

/****************************************************************************
*																			*
*							File Type Handling								*
*																			*
****************************************************************************/

#if defined( __ARC__ ) || defined( __IIGS__ ) || defined( __MAC__ )

/* Routines to handle user-defined type settings.  These are extension-type
   associations of the form <file extension> = <type information>, and can
   be used to add new types and override built-in ones */

#define EXT_LENGTH			8				/* 7 chars + '\0' */

#if defined( __ARC__ )
  #define TYPE_LENGTH		sizeof( WORD )	/* Length of file type field */
  #define CREATOR_LENGTH	0				/* No secondary type */
#elif defined( __IIGS__ )
  #define TYPE_LENGTH		sizeof( WORD )	/* Length of file type field */
  #define CREATOR_LENGTH	sizeof( LONG )	/* Length of auxType field */
#elif defined( __MAC__ )
  #define TYPE_LENGTH		sizeof( LONG )	/* Length of file type field */
  #define CREATOR_LENGTH	sizeof( LONG )	/* Length of file creator field */
#endif /* Various system-dependant file type sizes */

typedef struct TA {
				  struct TA *next;				/* The next header in the list */
				  char extension[ EXT_LENGTH ];	/* The file extension */
				  LONG type;					/* The file type */
#if defined( __IIGS__ ) || defined( __MAC__ )
				  LONG creator;					/* The file creator */
#endif /* __IIGS__ || __MAC__ */
				  } TYPE_ASSOC;

static TYPE_ASSOC *typeAssocListHead = NULL;

void addTypeAssociation( char *assocData )
	{
	TYPE_ASSOC *namePtr, *prevPtr, *newRecord;
	char *typePtr, *creatorPtr;
	char type[ TYPE_LENGTH ];
#if defined( __IIGS__ ) || defined( __MAC__ )
	char creator[ CREATOR_LENGTH ];
#endif /* __IIGS__ || __MAC__ */

	/* Break up the assocData string into its component parts */
	for( typePtr = assocData; *typePtr && *typePtr != '='; typePtr++ );
	if( !*typePtr )		/* Check for end of string reached */
		return;
	for( creatorPtr = typePtr; *creatorPtr && *creatorPtr != ','; creatorPtr++ );
#if defined( __IIGS__ ) || defined( __MAC__ )
	if( !*creatorPtr )	/* Check for end of string reached */
		return;
#endif /* __IIGS__ || __MAC__ */
	*typePtr++ = *creatorPtr++ = '\0';	/* Break up string */

	/* Perform some simple range checks.  Note that the creatorPtr check
	   works even when no creator is present since it checks for spurious
	   noise at the end of the string */
	if( strlen( assocData ) > EXT_LENGTH ||
		strlen( creatorPtr ) > CREATOR_LENGTH ||
		strlen( typePtr ) > TYPE_LENGTH )
		return;

	/* Pad type and creator with zeroes if necessary */
	memset( type, 0, TYPE_LENGTH );
	memcpy( type, typePtr, TYPE_LENGTH );
#if defined( __IIGS__ ) || defined( __MAC__ )
	memset( creator, 0, CREATOR_LENGTH );
	memcpy( creator, creatorPtr, CREATOR_LENGTH );
#endif /* __IIGS__ || __MAC__ */

	for( namePtr = typeAssocListHead; namePtr != NULL;
		 prevPtr = namePtr, namePtr = namePtr->next )
		/* Check whether we've already processed an association of this name */
		if( !strcmp( assocData, namePtr->extension ) )
			return;

	/* We have reached the end of the list without a match, so we add this
	   archive name to the list */
	if( ( newRecord = ( TYPE_ASSOC * ) hmalloc( sizeof( TYPE_ASSOC ) ) ) == NULL )
		error( OUT_OF_MEMORY );
	strcpy( newRecord->extension, assocData );
	memcpy( &newRecord->type, type, TYPE_LENGTH );
#if defined( __IIGS__ ) || defined( __MAC__ )
	memcpy( &newRecord->creator, creator, CREATOR_LENGTH );
#endif /* __IIGS__ || __MAC__ */
	newRecord->next = NULL;
	if( typeAssocListHead == NULL )
		typeAssocListHead = newRecord;
	else
		prevPtr->next = newRecord;
	return;
	}

/* Free up the memory used by the list of archive names */

void freeTypeAssociations( void )
	{
	TYPE_ASSOC *namePtr = typeAssocListHead, *headerPtr;

	/* A simpler implementation of the following would be:
	   for( namePtr = typeAssocListHead; namePtr != NULL; namePtr = namePtr->next )
		   hfree( namePtr );
	   However this will fail on some systems since we will be trying to
	   read namePtr->next out of a record we have just free()'d */
	while( namePtr != NULL )
		{
		headerPtr = namePtr;
		namePtr = namePtr->next;
		hfree( headerPtr );
		}
	}

/* Try and determine the type of a file based on the filename suffix.  Note
   that sometimes we can't tell the exact type from the suffix (.hpk, Unix
   executables), and sometimes the formats are incompatible, undocumented,
   and constantly changing (MS Word) so we don't bother */

#if defined( __ARC__ )

typedef struct {
			   char *suffix;			/* The suffix for the file */
			   WORD type;				/* The file type */
			   } FILE_TYPEINFO;

static FILE_TYPEINFO typeInfo[] =
			{
			{ "arc", 0x0DDC },			/* ARC archive (Sparc archiver) */
			{ "arj", 0x0DDC },			/* ARJ archive (Sparc archiver) */
			{ "bat", 0x0FDA },			/* MSDOS batch file */
			{ "bmp", 0x069C },			/* Windoze BMP graphics file */
			{ "c", 0x0FFF },			/* C code file */
			{ "cgm", 0x0405 },			/* Computer Graphics Metafile */
			{ "com", 0x0FD8 },			/* MSDOS executable */
			{ "csv", 0x0DFE },			/* Comma Seperated Variable file */
			{ "dif", 0x0DFD },			/* DIF spreadsheet data */
			{ "dbf", 0x0DB3 },			/* dBase III file */
			{ "dvi", 0x0CE5 },			/* DVI (actually TeX) file */
			{ "dxf", 0x0DEA },			/* DXF portalbe CAD file format */
			{ "h", 0x0FFF },			/* C header files */
			{ "gif", 0x0695 },			/* GIF graphics file */
			{ "exe", 0x0FD9 },			/* MSDOS executable */
			{ "iff", 0x0693 },			/* Amiga IFF graphics file */
			{ "img", 0x0692 },			/* Atari IMG graphics file */
			{ "fits", 0x06A3 },			/* Flexible Image Transport Sys. file */
			{ "jpg", 0x0C85 },			/* JPEG graphics file */
			{ "lzh", 0x0DDC },			/* Lharc archive (Sparc archiver) */
			{ "mac", 0x0694 },			/* MacPaint graphics file */
			{ "mod", 0x0CB6 },			/* Amiga .MOD sound file */
			{ "ndx", 0x0DB1 },			/* dBase III index file */
			{ "pbm", 0x069E },			/* PBM graphics file */
			{ "pcx", 0x0697 },			/* PCX graphics file */
			{ "pi1", 0x0691 },			/* Atari Degas graphics file */
			{ "pi2", 0x0691 },			/* Atari Degas graphics file */
			{ "pi3", 0x0691 },			/* Atari Degas graphics file */
			{ "pi4", 0x0691 },			/* Atari Degas graphics file */
			{ "ps", 0x0FF5 },			/* PostScript file */
			{ "qrt", 0x0698 },			/* QRT graphic file. The 'Q' is for 'slow' */
			{ "rf", 0x0FC9 },			/* Sun raster file */
			{ "rle", 0x06A1 },			/* Unix RLE image */
			{ "rtf", 0x0FFF },			/* Rich Text Format data */
			{ "tar", 0x0C46 },			/* tar'd files */
			{ "tif", 0x0FF0 },			/* TIFF graphics etc data */
			{ "txt", 0x0FFF },			/* Text files */
			{ "uue", 0x0DDC },			/* uuencoded file (Sparc archiver) */
			{ "wks", 0x0DB0 },			/* Lotus 123 v1 worksheet */
			{ "wk1", 0x0DB0 },			/* Lotus 123 v2 worksheet */
			{ "xbm", 0x069E },			/* XBM graphics file */
			{ "z", 0x0DDC },			/* compress file (Sparc archiver) */
			{ "zip", 0x0DDC },			/* ZIP archive (Sparc archiver) */
/*			{ "", 0x0FE6 },				\* Unix executable */
/*			{ "???", 0x0DB2 },			\* dBase II file */
/*			{ "???", 0x0DB4 }, 			\* SuperCalc III file */
			{ "", 0 }					/* End marker */
			};

#elif defined( __IIGS__ )

typedef struct {
			   char *suffix;			/* The suffix for the file */
			   WORD type;				/* The file type */
			   LONG auxType;			/* The file auxiliary type */
			   } FILE_TYPEINFO;

static FILE_TYPEINFO typeInfo[] =
			{
			{ "", 0, 0L }				/* End marker */
			};

#elif defined( __MAC__ )

typedef struct {
			   char *suffix;			/* The suffix for the file */
			   LONG type;				/* The file type */
			   LONG creator;			/* The file creator */
			   } FILE_TYPEINFO;

static FILE_TYPEINFO typeInfo[] =
			{
			{ "c", 'TEXT', 'KAHL' },	/* ThinkC C source */
			{ "com", 'PCFA', 'PCXT' },	/* SoftPC MSDOS .COM file */
			{ "cpt", 'PACT', 'CPCT' },	/* Compact archive */
			{ "ctx", 'PGP ', 'PGP ' },	/* PGP ciphertext file */
			{ "dbf", 'F+DB', 'FOX+' },	/* FoxBase+ (.dbf) */
			{ "dbt", 'F+DT', 'FOX+' },	/* FoxBase+ (.dbt) */
			{ "exe", 'PCFA', 'PCXT' },	/* SoftPC MSDOS .EXE file */
			{ "frm", 'F+FR', 'FOX+' },	/* FoxBase+ (.frm) */
			{ "gif", 'GIFf', '8BIM' },	/* GIF graphics file (Photoshop) */
			{ "h", 'TEXT', 'KAHL' },	/* ThinkC C source */
			{ "hpk", 'HPAK', 'HPAK' },	/* HPACK archive */
			{ "idx", 'F+IX', 'FOX+' },	/* FoxBase+ (.idx) */
			{ "jpg", 'JFIF', '8BIM' },	/* JPEG graphics file (Photoshop) */
			{ "mac", 'PNTG', 'MPNT' },	/* Macpaint graphics file */
			{ "p", 'TEXT', 'PJMM' },	/* Think Pascal source */
			{ "pas", 'TEXT', 'PJMM' },	/* Think Pascal source */
/*			{ "pas", 'TEXT', 'MPS ' },	\* MPW Pascal source */
			{ "pit", 'PIT ', 'PIT ' },	/* Packit archive */
			{ "rpt", 'F+RP', 'FOX+' },	/* FoxBase+ (.rpt) */
			{ "rtf", 'WDBN', 'MSWD' },	/* Rich Text Format (MS Word) */
			{ "scx", 'F+FM', 'FOX+' },	/* FoxBase+ (.scx) */
			{ "sit", 'SIT!', 'SIT!' },	/* StuffIt archive */
			{ "txt", 'TEXT', 'ttxt' },	/* Generic text (TeachText) */
			{ "vue", 'F+VU', 'FOX+' },	/* FoxBase+ (.vue) */
			{ "xls", 'XLS ', 'XCEL' },	/* Excel worksheet */
			{ "xlm", 'XLM ', 'XCEL' },	/* Excel macros */
			{ "xlc", 'XLC ', 'XCEL' },	/* Excel charts */
			{ "xlw", 'XLW ', 'XCEL' },	/* Excel workspace document */
			{ "tif", 'TIFF', 'dpnt' },	/* TIFF graphics file (Desk Paint) */
			{ "zip", 'pZIP', 'pZIP' },	/* Info-ZIP archive */
/*			{ "doc", 'WDBN', 'MSWD' },	\* MS Word document (sometimes) */
/*			{ "???", 'GLOS', 'MSWD' },	\* MS Word glossary */
/*			{ "dic", 'DICT', 'MSWD' },	\* MS Word dictionary */
			{ "", 0L, 0L },				/* End marker */
			};

#endif /* Various OS-specific file type info */

void findFileType( FILEHDRLIST *fileInfoPtr )
	{
	TYPE_ASSOC *assocInfoPtr;
	char *suffixPtr, *fileName = fileInfoPtr->fileName;
	int i;

	/* If we already have type info, return now */
	if( fileInfoPtr->type )
		return;

	/* Find the filename suffix */
	for( suffixPtr = fileName + strlen( fileName ) - 1;
		 suffixPtr != fileName && *suffixPtr != '.';
		 suffixPtr-- );
#ifdef __ARC__
	if( ( suffixPtr++ == fileName )	&& ( !*fileInfoPtr->oldExtension ) )
		return;	/* Move past dit, return if no suffix or deleted suffix */
#else
	if( suffixPtr++ == fileName )	/* Move past dit, return if no suffix */
		return;
#endif /* __ARC__ */

	/* First look through the user-defined types.  These can override the
	   built-in ones */
	for( assocInfoPtr = typeAssocListHead; assocInfoPtr != NULL;
		 assocInfoPtr = assocInfoPtr->next )
		{
		if( !stricmp( assocInfoPtr->extension, suffixPtr ) )
			{
			/* Have you ever wondered why the HPACK file permissions were all 666? */
			fileInfoPtr->type = assocInfoPtr->type;
#if defined( __IIGS__ )
			fileInfoPtr->auxType = assocInfoPtr->auxType;
#elif defined( __MAC__ )
			fileInfoPtr->creator = assocInfoPtr->creator;
#endif /* Setting of other file info */
			return;
			}
#ifdef __ARC__
		/* Check for the existence of a deleted extension due to the
		   +invert operation */
		if( !stricmp( assocInfoPtr->extension, fileInfoPtr->oldExtension ) )
			{
			fileInfoPtr->type = assocInfoPtr->type;
			return;
			}
#endif /* __ARC__ */
		}

	/* Then look through the built-in suffix table trying to match the file
	   suffix */
	for( i = 0; *typeInfo[ i ].suffix; i++ )
		{
		if( !stricmp( typeInfo[ i ].suffix, suffixPtr ) )
			{
			fileInfoPtr->type = typeInfo[ i ].type;
#if defined( __IIGS__ )
			fileInfoPtr->auxType = typeInfo[ i ].auxType;
#elif defined( __MAC__ )
			fileInfoPtr->creator = typeInfo[ i ].creator;
#endif /* Setting of other file info */
			return;
			}
#ifdef __ARC__
		/* Check for the existence of a deleted extension due to the
		   +invert operation */
		if( !stricmp( typeInfo[ i ].suffix, fileInfoPtr->oldExtension ) )
			{
			fileInfoPtr->type = typeInfo[ i ].type;
			return;
			}
#endif /* __ARC__ */
		}
	}

#endif /* __ARC__ || __IIGS__ || __MAC__ */

/****************************************************************************
*																			*
*						Directory-Extraction Routines						*
*																			*
****************************************************************************/

/* Build the path to a given directory index, truncating names if necessary */

char *getPath( WORD dirNo )
	{
	WORD dirStack[ MAX_PATH ];	/* We'll run out of path length before we run
								   out of directory stack so this is safe */
	static char pathName[ MAX_PATH ];
	int stackIndex = 0, pathLen = 0;
	char *dirNamePtr;

	/* Get a chain of directories from the root to the desired directory */
	if( dirNo )
		do
			dirStack[ stackIndex++ ] = dirNo;
#pragma warn -pia
		while( ( dirNo = getParent( dirNo ) ) && stackIndex <= MAX_PATH );
#pragma warn +pia

	/* Build the path to the current directory */
	*pathName = '\0';
	while( stackIndex )
		{
#ifdef __VMS__
		isDirComponent = TRUE;
#endif /* __VMS__ */
		dirNamePtr = truncFileName( getDirName( dirStack[ --stackIndex ] ) );
		if( ( pathLen += strlen( dirNamePtr ) ) >= MAX_PATH - 1 )
			error( PATH_s__TOO_LONG, pathName );
		strcat( pathName, dirNamePtr );
		strcat( pathName, SLASH_STR );
		}

	return( pathName );
	}

/* Recreate a directory tree stored within an archive */

void extractDirTree( int action )
	{
	char dirPath[ MAX_PATH ], *pathName;
	int outputPathLen = basePathLen;
	WORD count;
	BOOLEAN inPosition = FALSE;
	long dataLen;
	TAGINFO tagInfo;
#if defined( __MSDOS16__ )
	/* Bypass compiler bug */
#elif defined( __MAC__ ) || defined( __OS2__ )
	LONG timeStamp;
#elif defined( __UNIX__ )
	struct utimbuf timeStamp;
#endif /* Various OS-specific attribute variables */

	skipDist = 0L;
	strcpy( dirPath, basePath );	/* We have already checked the validity
									   of basePath in handleArchive() */
	getFirstDir();					/* Skip archive root directory */
	for( count = getNextDir(); count != END_MARKER; count = getNextDir() )
		{
		/* Get the path to the current directory.  Note that getPath() returns
		   SLASH at the end of the pathname.  This is then stomped by the call
		   to extractPath() in dirExists(), and is in fact essential since
		   otherwise extractPath() will treat the last directory name as a filename */
		pathName = getPath( count );
		if( outputPathLen + strlen( pathName ) >= MAX_PATH )
			error( PATH_ss__TOO_LONG, basePath, pathName );
		strcpy( dirPath + outputPathLen, pathName );

		/* Check whether we want to create directories, or extract attribute
		   information for previously created directories */
		if( action == EXTRACTDIR_CREATE_ONLY )
			{
			/* Try and create the new directory if it's marked for creation
			   and doesn't already exist */
			if( getDirTaggedStatus( count ) && !dirExists( dirPath ) )
				{
				dirPath[ strlen( dirPath ) - 1 ] = '\0';	/* Stomp final SLASH */
#ifndef GUI
				hprintfs( MESG_CREATING_DIRECTORY_s, ( fileNameTruncated ) ?
						  getDirName( count ) : dirPath );
				if( fileNameTruncated )
					hprintfs( MESG_AS_s, dirPath );
				hputchars( '\n' );
#endif /* !GUI */
				if( hmkdir( dirPath, MKDIR_ATTR ) == IO_ERROR )
					error( CANNOT_CREATE_DIR, dirPath );

#ifdef __OS2__
				/* If we've truncated the directory name, add the full name
				   as an EA */
				if( fileNameTruncated )
					addLongName( dirPath, getDirName( count ) );
#endif /* __OS2__ */
				}
			else
				{
				/* Complain if an entity of this name already exists and skip
				   it and all the files and directories contained within it */
				if( dirnameConflict )
					{
					pathName[ strlen( pathName ) - 1 ] = '\0';	/* Stomp final SLASH */
#ifdef GUI
					alert( ALERT_DIRNAME_CONFLICTS_WITH_FILE, pathName );
#else
					hprintf( WARN_DIRNAME_s_CONFLICTS_WITH_FILE, pathName );
#endif /* GUI */
					selectDirNo( count, FALSE );
					}
				}
			}
		else
			{
			/* Set the directories attribute information if necessary */
#pragma warn -pia
			if( getDirTaggedStatus( count ) && ( flags & STORE_ATTR ) &&
				( dataLen = getDirAuxDatalen( count ) ) )
#pragma warn +pia
				{
				/* Move to the correct position in the archive if we're not
				   already there */
				if( !inPosition )
					{
					movetoDirData();
					skipDist = 0L;		/* Skipped data has been bypassed */
					inPosition = TRUE;
					}
				else
					skipToData();

				/* Grovel through the directory data field looking for
				   attribute tags */
				while( dataLen )
					switch( readTag( &dataLen, &tagInfo ) )
						{
						case TAGCLASS_ATTRIBUTE:
							/* Read in the attributes and set them */
							hchmod( dirPath, readAttributeData( tagInfo.tagID ) );
							break;

#if defined( __MSDOS16__ )
						/* Fix compiler bug */
#elif defined( __MAC__ )
						case TAGCLASS_TIME:
							/* Read in the file times and set them */
							if( tagInfo.tagID == TAG_BACKUP_TIME )
								/* Set file's backup time */
								setBackupTime( dirPath, fgetLong() );
							else
								if( tagInfo.tagID == TAG_CREATION_TIME )
									/* Set file's creation time */
									setCreationTime( dirPath, fgetLong() );
								else
									skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_MISC:
							if( tagInfo.tagID == TAG_VERSION_NUMBER )
								setVersionNumber( dirPath, ( BYTE ) fgetWord() );
							else
								skipSeek( tagInfo.dataLength );
							break;
#elif defined( __OS2__ )
						case TAGCLASS_TIME:
							/* Read in the file times and set them */
							if( tagInfo.tagID == TAG_ACCESS_TIME )
								/* Set file's access time */
								setAccessTime( dirPath, fgetLong() );
							else
								if( tagInfo.tagID == TAG_CREATION_TIME )
									/* Set file's creation time */
									setCreationTime( dirPath, fgetLong() );
								else
									skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_ICON:
							/* Set the icon EA if possible */
							if( tagInfo.tagID == TAG_OS2_ICON )
								setEAinfo( dirPath, &tagInfo, archiveFD );
							else
								skipSeek( tagInfo.dataLength );
							break;

						case TAGCLASS_MISC:
							/* Set the longname EA if possible */
							if( tagInfo.tagID == TAG_LONGNAME ||
								tagInfo.tagID == TAG_OS2_MISC_EA )
								{
								setEAinfo( dirPath, &tagInfo, archiveFD );
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
								timeStamp.modtime = getDirTime( count );
								utime( dirPath, &timeStamp );
								}
							else
								skipSeek( tagInfo.dataLength );
							break;
#endif /* Various OS-dependant attribute reads */

						default:
							/* We don't know what to do with this tag,
							   skip it */
							skipSeek( tagInfo.dataLength );
							break;
						}
				}
			else
				/* Skip the data for this directory */
				skipDist += ( long ) getDirAuxDatalen( count );

			/* Give the directory its real date if necessary.  We don't
			   need to check that the open is successful since we've just
			   created the directory so we know it exists */
			if( !( flags & TOUCH_FILES ) )
				setDirecTime( dirPath, getDirTime( count ) );
			}

		/* Finally, make sure we don't get a "Nothing to do" error if there
		   are no files to extract */
		if( getDirTaggedStatus( count ) )
			archiveChanged = TRUE;
		}
	}

/****************************************************************************
*																			*
*						Archive Path-Handling Routines						*
*																			*
****************************************************************************/

/* Various vars used by several routines */

static int internalPathLen;

/* Build the path within the archive to a given file */

char *buildInternalPath( FILEHDRLIST *infoPtr )
	{
	static char internalPath[ MAX_PATH ];
	char *truncName;

	strcpy( internalPath, ( dirFlags & DIR_FLATTEN ) ?
							"" : getPath( infoPtr->data.dirIndex ));
#ifdef __VMS__
	isDirComponent = FALSE;
#endif /* __VMS__ */
	truncName = truncFileName( infoPtr->fileName );
	if( ( internalPathLen = strlen( internalPath ) + strlen( truncName ) ) >= MAX_PATH )
		error( PATH_ss_TOO_LONG, internalPath, truncName );
	strcat( internalPath, truncName );
	return( internalPath );
	}

/* Build the path outside the archive to a given file */

char *buildExternalPath( FILEHDRLIST *infoPtr )
	{
	static char externalPath[ MAX_PATH ];
	char *pathPtr;

	strcpy( externalPath, basePath );
	pathPtr = buildInternalPath( infoPtr );
	if( basePathLen + internalPathLen >= MAX_PATH )
		error( PATH_ss_TOO_LONG, externalPath, pathPtr );
	strcat( externalPath, pathPtr );
	return( externalPath );
	}

BOOLEAN isTruncated( void )
	{
	return( fileNameTruncated );
	}
