/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  UNIX-Specific Routines						*
*							 UNIX.C  Updated 12/02/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
* 		Copyright 1991 - 1992  Stuart A. Woolford and Peter C. Gutmann.		*
*							All rights reserved								*
*																			*
****************************************************************************/

/*	You press the keys with no effect,
	Your mode is not correct.
	The screen blurs, your fingers shake;
	You forgot to press escape.
	Can't insert, can't delete,
	Cursor keys won't repeat.
	You try to quit, but can't leave,
	An extra "bang" is all you need.

	You think it's neat to type an "a" or an "i"--
	Oh yeah?
	You won't look at emacs, no you'd just rather die
	You know you're gonna have to face it;
	You're addicted to vi!

	You edit files one at a time;
	That doesn't seem too out of line?
	You don't think of keys to bind--
	A meta key would blow your mind.
	H, J, K, L?  You're not annoyed?
	Expressions must be a Joy!
	Just press "f", or is it "t"?
	Maybe "n", or just "g"?

	Oh--You think it's neat to type an "a" or an "i"--
	Oh yeah?
	You won't look at emacs, no you'd just rather die
	You know you're gonna have to face it;
	You're addicted to vi!

	Might as well face it,
	You're addicted to vi!
	You press the keys without effect,
	Your life is now a wreck.
	What a waste!  Such a shame!
	And all you have is vi to blame.

	Oh--You think it's neat to type an "a" or an "i"--
	Oh yeah?
	You won't look at emacs, no you'd just rather die
	You know you're gonna have to face it;
	You're addicted to vi!

	Might as well face it,
	You're addicted to vi!

		- Chuck Musciano

	YOU CAN HAVE MY vi WHEN YOU PRY IT FROM MY COLD DEAD HANDS

		- Stuart Woolford

	Improving the vi editor:

	Put it in the middle of a field, rent a tank and drive over it
	approximately five times, then back off about 100m and fire at it
	until there is nothing left but a large crater.

		- Andrew Elzenaar */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "defs.h"
#include "error.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#include "tags.h"
#include "io/hpackio.h"
#include "language/language.h"

/* The extra routines needed by each variation of Unix:

   AIX							strlwr	stricmp
   AIX386						strlwr	stricmp
   AIX370						strlwr	stricmp
   BSD386						strlwr	stricmp
   CONVEX								stricmp
   HPUX							strlwr	stricmp
   IRIX							strlwr	stricmp
   ISC
   LINUX						strlwr	stricmp
   NEXT			memmove			strlwr	stricmp
   MINT				?		?		?		?
   OSF1							strlwr	stricmp
   POSIX
   SUNOS		memmove					stricmp
   SVR4							strlwr	stricmp
   ULTRIX						strlwr	stricmp
   ULTRIX_OLD	memmove			strlwr	stricmp
   UTS4							strlwr	stricmp */

/* The buffer of random numbers.  This is filled by measuring the latency
   between keystrokes as acquired by hgetch() */

#define RANDOM_BUFSIZE	64

static BYTE randomBuffer[ RANDOM_BUFSIZE ];
static int randomBufferPos = 0;

/* Get an input character, no echo */

#if defined( BSD386 ) || defined( CONVEX ) || defined( HPUX ) || \
	defined( IRIX ) || defined( LINUX ) || defined( POSIX ) || \
	defined( SVR4 ) || defined( UTS4 )

#include <termios.h>

int hgetch( void )
	{
	static struct itimerval itimerVal;
	struct termios ttyInfo;
	FD ttyFD = ERROR;
	LONG data;
	char ch;

	/* Flush all pending output */
	fflush( stdout );

	/* Turn echo off */
	if( isatty( STDERR ) && ( ttyFD = hopen( ttyname( STDERR ), O_RDWR ) ) != ERROR )
		{
		tcgetattr( ttyFD, &ttyInfo );
#if defined( BSD386 ) || defined( CONVEX ) || defined( HPUX ) || \
	defined( IRIX ) || defined( LINUX ) || defined( POSIX ) || \
	defined( UTS4 )
		ttyInfo.c_lflag &= ~ECHO;
		ttyInfo.c_lflag &= ~ICANON;
#else
		ttyInfo.c_lflags &= ~ECHO;
		ttyInfo.c_lflags |= CBREAK;
#endif /* BSD386 || CONVEX || HPUX || IRIX || LINUX || POSIX || UTS4 */
		tcsetattr( ttyFD, TCSANOW, &ttyInfo );
		}

	/* Set expiry time at 24 hours */
	itimerVal.it_value.tv_sec = 86400;
	itimerVal.it_value.tv_usec = 0;

	/* Let timer expire after this interval */
	itimerVal.it_interval.tv_sec = 0;
	itimerVal.it_interval.tv_usec = 0;

	/* Start the timer */
	setitimer( ITIMER_REAL, &itimerVal, NULL );

	/* Get the character */
	read( ttyFD, &ch, 1 );

	/* Get the expired time */
	getitimer( ITIMER_REAL, &itimerVal );

	/* Add result to random data.  This is usually at least to millisecond
	   accuracy, worst-case is a 60Hz timer */
	data = mgetLong( randomBuffer + randomBufferPos );
	mputLong( randomBuffer + randomBufferPos, data ^ itimerVal.it_value.tv_usec );
	randomBufferPos = ( randomBufferPos + sizeof( LONG ) ) % RANDOM_BUFSIZE;

	/* Turn echo on again if it was off */
	if( ttyFD != ERROR )
		{
		tcgetattr( ttyFD, &ttyInfo );
#if defined( BSD386 ) || defined( CONVEX ) || defined( HPUX ) || \
	defined( IRIX ) || defined( LINUX ) || defined( POSIX ) || \
	defined( UTS4 )
		ttyInfo.c_lflag |= ECHO;
		ttyInfo.c_lflag |= ICANON;
#else
		ttyInfo.c_lflags |= ECHO;
		ttyInfo.c_lflags &= ~CBREAK;
#endif /* BSD386 || CONVEX || HPUX || IRIX || LINUX || POSIX || UTS4 */
		tcsetattr( ttyFD, TCSANOW, &ttyInfo );
		}

	return( ch );
	}

#else

#include <sgtty.h>

int hgetch( void )
	{
	static struct itimerval itimerVal;
	struct sgttyb ttyInfo;
	FD ttyFD = ERROR;
	LONG data;
	char ch;

	/* Flush all pending output */
	fflush( stdout );

	/* Turn echo off */
	if( isatty( STDERR ) && ( ttyFD = hopen( ttyname( STDERR ), O_RDWR ) ) != ERROR )
		{
		gtty( ttyFD, &ttyInfo );
		ttyInfo.sg_flags &= ~ECHO;
		ttyInfo.sg_flags |= CBREAK;
		stty( ttyFD, &ttyInfo );
		}

	/* Set expiry time at 24 hours */
	itimerVal.it_value.tv_sec = 86400;
	itimerVal.it_value.tv_usec = 0;

	/* Let timer expire after this interval */
	itimerVal.it_interval.tv_sec = 0;
	itimerVal.it_interval.tv_usec = 0;

	/* Start the timer */
	setitimer( ITIMER_REAL, &itimerVal, NULL );

	/* Get the character */
	read( ttyFD, &ch, 1 );

	/* Get the expired time */
	getitimer( ITIMER_REAL, &itimerVal );

	/* Add result to random data.  This is usually at least to millisecond
	   accuracy, worst-case is a 60Hz timer */
	data = mgetLong( randomBuffer + randomBufferPos );
	mputLong( randomBuffer + randomBufferPos, data ^ itimerVal.it_value.tv_usec );
	randomBufferPos = ( randomBufferPos + sizeof( LONG ) ) % RANDOM_BUFSIZE;

	/* Turn echo on again if it was off */
	if( ttyFD != ERROR )
		{
		gtty( ttyFD, &ttyInfo );
		ttyInfo.sg_flags |= ECHO;
		ttyInfo.sg_flags &= ~CBREAK;
		stty( ttyFD, &ttyInfo );
		}

	return( ch );
	}

#endif /* BSD386 || CONVEX || HPUX || IRIX || LINUX || POSIX || SVR4 */

#if 0		/* Last-ditch code if all else fails */
int hgetch( void )
	{
	char ch;

	system( "/bin/stty +raw" );
	ch = getchar();
	system( "/bin/stty +edit" );
	return( ch );
	}
#endif /* 0 */

#ifdef POSIX

/* Memory handling functions - need special-case code for QNX to handle
   the 64K memory block which is needed by the LZA compressor.

   "This isn't a proper Unix - I type 'moria' and nothing happens"
		- PurpleX */

#include <sys/seginfo.h>
#include <i86.h>

static unsigned int segmentBase;
static void *segmentPtr = NULL;

void *hmalloc( const unsigned long size )
	{
	if( size > 0x0000FFFFL )
		{
		/* Try and allocate a 64K block from the system */
		if( ( segmentBase = qnx_segment_alloc( size ) ) == 0xFFFF )
			return( segmentPtr = NULL );
		segmentPtr = MK_FP( segmentBase, 0 );
		return( segmentPtr );
		}
	else
		return( malloc( ( size_t ) size ) );
	}

void *hrealloc( void *buffer, const unsigned long size )
	{
	return( realloc( buffer, ( size_t ) size ) );
	}

void hfree( void *buffer )
	{
	if( buffer == segmentPtr )
		qnx_segment_free( segmentBase );
	else
		free( buffer );
	}
#endif /* POSIX */

/* Set a file's timestamp */

void setFileTime( const char *fileName, const LONG fileTime )
	{
	struct utimbuf timeStamp;

	/* For now we just set the update time the same as the access time */
	timeStamp.actime = timeStamp.modtime = fileTime;
	utime( fileName, &timeStamp );
	}

#ifdef NEED_RENAME

/* Rename a file */

int rename( const char *srcFile, const char *destFile )
	{
	if( link( srcFile, destFile ) == IO_ERROR )
		return( IO_ERROR );
	return( unlink( srcFile ) );
	}

#endif /* NEED_RENAME */

/* Create a file.  A custom version is necessary since creating the file
   doesn't give read access to it, so we must open it, close it, and reopen
   it with read access enabled */

int hcreat( const char *filePath, const int attr )
	{
	FD theFD;

	if( ( theFD = creat( filePath, attr ) ) != IO_ERROR )
		{
		close( theFD );
		return( open( filePath, O_RDWR | S_DENYRDWR | A_SEQ ) );
		}
	else
		return( IO_ERROR );
	}

/* Create a directory */

int hmkdir( const char *dirName, const int mode )
	{
	int oldUmask = umask( mode ^ 0777 ), retVal, newMode = mode;

	/* We have to be clever with umask here since if no x bits are set
	   noone will be able to access the directory, so we always give the
	   owner an x bit in MKDIR_ATTR.  Also if umask has group or other r
	   enabled we give them x as well */
	if( !( oldUmask & UNIX_ATTR_GROUP_R ) )
		newMode |= UNIX_ATTR_GROUP_X;
	if( !( oldUmask & UNIX_ATTR_OTHER_R ) )
		newMode |= UNIX_ATTR_OTHER_X;

	if( sysSpecFlags & SYSPEC_NOUMASK )
		umask( newMode ^ 0777 );	/* Make sure umask doesn't interfere */
	else
		umask( oldUmask );
	retVal = mkdir( dirName, newMode );
	umask( oldUmask );				/* Restore original umask */

	return( retVal );
	}

/* Change a file's attributes */

int hchmod( const char *filePath, const int mode )
	{
	int oldUmask, retVal;

	/* Check whether we should override the umask */
	if( sysSpecFlags & SYSPEC_NOUMASK )
		{
		/* Make sure the umask doesn't interfere when we change the file's
		   attributes */
		oldUmask = umask( mode ^ 0777 );
		retVal = chmod( filePath, mode );
		umask( oldUmask );					/* Restore original umask */
		}
	else
		retVal = chmod( filePath, mode );

	return( retVal );
	}

/* opendir(), readdir(), closedir() for those systems which don't have them.
   If anyone ever uses this code let me know so I can open a bottle of
   champagne or something */

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
		for( pos = strlen( dirPath ) - 1; pos && dirPath[ pos - 1 ] != SLASH;
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
	while( ( fileInfo->matchAttr == ALLFILES ) ?
		   ( ( fileInfo->statInfo.st_mode & S_IFMT ) == S_IFDIR ) : 0 );

	return( TRUE );
	}

/* End findFirst/Next() for this directory */

void findEnd( FILEINFO *fileInfo )
	{
	if( fileInfo->dirBuf != NULL )
		closedir( fileInfo->dirBuf );
	}

/* Get the screen size.  The environment variables $LINES and $COLUMNS will
   be used if they exist.  If not, then the TIOCGWINSZ call to ioctl() is
   used (if it is defined).  If not, then the TIOCGSIZE call to ioctl() is
   used (if it is defined).  If not, then the WIOCGETD call to ioctl() is
   used (if it is defined).  If not, then get the info from terminfo/termcap
   (if it is there).  Otherwise, assume we have a 24x80 model 33 */

#define DEFAULT_ROWS		24
#define DEFAULT_COLS		80

/* Try to access terminfo through the termcap-interface in the curses library
   (which requires linking with -lcurses) or use termcap directly (which
   requires linking with -ltermcap) */

#ifndef USE_TERMCAP
  #if defined( USE_TERMINFO ) || defined( USE_CURSES )
	#define USE_TERMCAP
  #endif /* USE_TERMINFO || USE_CURSES */
#endif /* USE_TERMCAP */

#ifdef USE_TERMCAP
  #define TERMBUFSIZ	1024
  #define UNKNOWN_TERM	"unknown"
  #define DUMB_TERMBUF	"dumb:co#80:hc:"

  extern int tgetent(), tgetnum();
#endif /* USE_TERMCAP */

/* Try to get TIOCGWINSZ from termios.h, then from sys/ioctl.h */

#ifndef NEXT
  #include <termios.h>
#endif /* NEXT */

#if !defined( TIOCGWINSZ ) && !defined( TIOCGSIZE ) && !defined( WIOCGETD )
  #include <sys/ioctl.h>
#endif /* !( TIOCGWINSIZ || TIOCGSIZE || WIOCGETD ) */

/* If we still don't have TIOCGWINSZ (or TIOCGSIZE) try for WIOCGETD */

#if !defined( TIOCGWINSZ ) && !defined( TIOCGSIZE ) && !defined( WIOCGETD )
  #include <sgtty.h>
#endif /* !( TIOCGWINSZ || TIOCGSIZE || WIOCGETD ) */

void getScreenSize( void )	/* Rot bilong kargo */
	{
	char *envLines, *envColumns;
	long rowTemp = 0, colTemp = 0;
#ifdef USE_TERMCAP
	char termBuffer[ TERMBUFSIZ ], *termInfo;
#endif /* USE_TERMCAP */
#ifdef TIOCGWINSZ
	struct winsize windowInfo;
#else
  #ifdef TIOCGSIZE
	struct ttysize windowInfo;
  #else
	#ifdef WIOCGETD
	  struct uwdata windowInfo;
	#endif /* WIOCGETD */
  #endif /* TIOCGSIZE */
#endif /* TIOCGSIZE */

	screenHeight = screenWidth = 0;

	/* Make sure that we're outputting to a terminal */
	if( !isatty( fileno( stderr ) ) )
		{
		screenHeight = DEFAULT_ROWS;
		screenWidth = DEFAULT_COLS;
		return;
		}

	/* LINES & COLUMNS environment variables override everything else */
	envLines = getenv( "LINES" );
	if( envLines != NULL && ( rowTemp = atol( envLines ) ) > 0 )
		screenHeight = ( int ) rowTemp;

	envColumns = getenv( "COLUMNS" );
	if( envColumns != NULL && ( colTemp = atol( envColumns ) ) > 0 )
		screenWidth = ( int ) colTemp;

#ifdef TIOCGWINSZ
	/* See what ioctl() has to say (overrides terminfo & termcap) */
	if( ( !screenHeight || !screenWidth ) && ioctl( fileno( stderr ), TIOCGWINSZ, &windowInfo ) != -1 )
		{
		if( !screenHeight && windowInfo.ws_row > 0 )
			screenHeight = ( int ) windowInfo.ws_row;

		if( !screenWidth && windowInfo.ws_col > 0 )
			screenWidth = ( int ) windowInfo.ws_col;
		}
#else
  #ifdef TIOCGSIZE
	/* See what ioctl() has to say (overrides terminfo & termcap) */
	if( ( !screenHeight || !screenWidth ) && ioctl( fileno( stderr ), TIOCGSIZE, &windowInfo ) != -1 )
		{
		if( !screenHeight && windowInfo.ts_lines > 0 )
			screenHeight = ( int ) windowInfo.ts_lines;

		if( !screenWidth && windowInfo.ts_cols > 0 )
			screenWidth = ( int ) windowInfo.ts_cols;
		}
  #else
	#ifdef WIOCGETD
	/* See what ioctl() has to say (overrides terminfo & termcap) */
	if( ( !screenHeight || !screenWidth ) && ioctl( fileno( stderr ), WIOCGETD, &windowInfo) != -1 )
		{
		if( !screenHeight && windowInfo.uw_height > 0 )
			screenHeight = ( int ) ( windowInfo.uw_height / windowInfo.uw_vs );

		if( !screenWidth && windowInfo.uw_width > 0 )
			screenWidth = ( int ) ( windowInfo.uw_width / windowInfo.uw_hs );
		}	/* You are in a twisty maze of standards, all different */
	#endif /* WIOCGETD */
  #endif /* TIOCGSIZE */
#endif /* TIOCGWINSZ */

#ifdef USE_TERMCAP
	/* See what terminfo/termcap has to say */
	if( !screenHeight || !screenWidth )
		{
		if( ( termInfo = getenv( "TERM" ) ) == NULL )
			termInfo = UNKNOWN_TERM;

		if( ( tgetent( termBuffer, termInfo ) <= 0 ) )
			strcpy( termBuffer, DUMB_TERMBUF );

		if( !screenHeight && ( rowTemp = tgetnum( "li" ) ) > 0 )
				screenHeight = ( int )rowTemp;

		if( !screenWidth && ( colTemp = tgetnum( "co" ) ) > 0 )
				screenWidth = ( int ) colTemp;
	}
#endif /* USE_TERMCAP */

	/* Use 80x24 if all else fails */
	if( !screenHeight )
		screenHeight = DEFAULT_ROWS;
	if( !screenWidth )
		screenWidth = DEFAULT_COLS;
	}

int getCountry( void )
	{
	return( 0 );	/* Default to US - not very nice */
	}

#if defined( AIX ) || defined( AIX370 ) || defined( AIX386 ) || \
	defined( BSD386 ) || defined( CONVEX ) || defined( GENERIC ) || \
	defined( HPUX ) || defined( IRIX ) || defined( LINUX ) || \
	defined( NEXT ) || defined( OSF1 ) || defined( SUNOS ) || \
	defined( SVR4 ) || defined( ULTRIX ) || defined( ULTRIX_OLD ) || \
	defined( UTS4 )

int htruncate( const FD theFD )
	{
	return( ftruncate( theFD, tell( theFD ) ) );
	}

#elif defined( POSIX )

int htruncate( const FD theFD )
	{
	return( ( ltrunc( theFD, 0L, SEEK_CUR ) == 0L ) ? IO_ERROR : OK );
	}

#else

/* No two htruncate()'s are the same, so we make sure everyone gets an error
   here so they must define one they can use.  Note that the following sample
   htruncate() hasn't been fully tested - things have never been bad enough
   to make its use necessary */

#error "Need to implement/define htruncate in UNIX.C"

/* The worst-case htruncate() - open a new file, copy everything across to
   the truncation point, and delete the old file.  This only works because
   htruncate is almost never called, because this worst-case version will
   only be used on a few systems, and because the drive access speed is
   assumed to be fast */

void moveData( const FD _inFD, const FD destFD, const long noBytes );

int htruncate( const FD theFD )
	{
	FD newArchiveFD;
	char newArchiveName[ MAX_PATH ];
	long truncatePoint;
	int retVal;

	/* Set up temporary filename to be the archive name with a ".TMP" suffix */
	strcpy( newArchiveName, archiveFileName );
	strcpy( newArchiveName, strlen( newArchiveName ) - 3, "tmp" );

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

	stat( pathName1, &statInfo1 );
	stat( pathName2, &statInfo2 );
	return( statInfo1.st_ino == statInfo2.st_ino &&
			statInfo1.st_dev == statInfo2.st_dev );
	}

/* Get any randomish information the system can give us (not terribly good,
   but every extra bit helps - if it's there we may as well use it) */

void getRandomInfo( BYTE *buffer, int bufSize )
	{
	struct timeval timeInfo;
	struct timezone timeZoneInfo;
	LONG data;
	int srcIndex, destIndex;

	/* Get user and process ID information */
	data = mgetLong( buffer );
	mputLong( buffer, data ^ getuid() );
	data = mgetLong( buffer + sizeof( LONG ) );
	mputLong( buffer + sizeof( LONG ), data ^ getgid() );
	data = mgetLong( buffer + 2 * sizeof( LONG ) );
	mputLong( buffer + ( 2 * sizeof( LONG ) ), data ^ getpid() );
	data = mgetLong( buffer + 3 * sizeof( LONG ) );
	mputLong( buffer + ( 3 * sizeof( LONG ) ), data ^ getppid() );

	/* Get time to greatest possible resolution */
	gettimeofday( &timeInfo, &timeZoneInfo );
	data = mgetLong( buffer );
	mputLong( buffer, data ^ timeInfo.tv_sec );
	data = mgetLong( buffer + sizeof( LONG ) );
	mputLong( buffer + sizeof( LONG ), data ^ timeInfo.tv_usec );

	/* Add the data from the keystroke latency buffer */
	for( srcIndex = destIndex = 0; srcIndex < RANDOM_BUFSIZE; srcIndex++, destIndex++ )
		{
		buffer[ destIndex ] ^= randomBuffer[ srcIndex ];
		destIndex %= bufSize;
		}
	}

#ifdef NEED_STRLWR

/* strlwr() for those systems which don't have it */

void strlwr( char *string )
	{
	while( *string )
		{
		*string = tolower( *string );
		string++;
		}
	}
#endif /* NEED_STRLWR */

#ifdef NEED_MEMMOVE

/* memmove() for those systems which don't have it */

void memmove( char *dest, char *src, int length )
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
#endif /* NEED_MEMMOVE */

#ifdef NEED_STRICMP

int strnicmp( const char *src, const char *dest, int length )
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
#endif /* NEED_STRICMP */
