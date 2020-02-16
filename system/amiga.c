/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  Amiga-Specific Routines						*
*							 AMIGA.C  Updated 20/11/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
* 		Copyright 1992  Peter Gutmann, Nick Little, and Joerg Plate.		*
*							All rights reserved								*
*																			*
****************************************************************************/

/* "The difference is not in the hardware, but in the operating system...
	the Amiga has one, the PC has DOS" - James Pullen */

/* We can't include "defs.h" since some of the defines conflict with
   built-in ones and Lattice/SAS C 5.10 doesn't seem to like them being
   #undef'd and redefined, so we define the necessary defs.h defines here.

   "You're talking about an Amiga compiler here" - Nutta McBastard

   "It could be seen as a very good argument for believing in some form of
	divine retribution" - Arne Rohde

   "This compiler has a plethora of insects" - Misquote from the Amiga Rom
											   Kernel Reference Manual

   This code was originally written by Peter Gutmann, then rendered
   functional under SAS/C 5.10 by Nick Little during several late-night phone
   debugging sessions.  Finally, Joerg Plate changed it to work with
   SAS/C 6.3 */

typedef int BOOLEAN;

#define FALSE	0
#define TRUE	1

#define OK		0
#define ERROR	-1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __SASC_60
#pragma msg 148 ign push
#pragma msg 149 ign push
#pragma msg 190 ign push
#endif /* __SASC_60 */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#ifdef __SASC_60
#pragma msg 148 pop
#pragma msg 149 pop
#pragma msg 190 pop
#endif /* __SASC_60 */

#ifdef ACTION_READ		/* Undef conflicting Amiga/ISO 8573 symbols */
  #undef ACTION_READ
  #undef ACTION_WRITE
#endif /* ACTION_READ */

#define _DEFS_DEFINED	/* Trick crc16.h into not #including defs.h */

#define inline			/* Get rid of inline define for now */

#include "error.h"
#include "filesys.h"
#include "flags.h"
#include "frontend.h"
#include "system.h"
#include "tags.h"
#include "hpacklib.h"
#include "io/hpackio.h"
#include "io/fastio.h"
#include "crc/crc16.h"
#include "language/language.h"

/* The time difference between the Amiga and Unix epochs, in seconds */

#define AMIGA_TIME_OFFSET		0x0F0C3F00L

/* DICE doesn't seem to have MODE_READWRITE defined */

#ifndef MODE_READWRITE
  #define MODE_READWRITE		1004
#endif /* !MODE_READWRITE */

/* The maximum length of a file comment (80 chars + '\0') */

#define COMMENT_SIZE			81

/* AmigaDOS handles all files in terms of a FileHandle struct.  The HPACK
   routines can be generalized to handle this, but it's far easier to simply
   add our own file table management code */

typedef struct DiskObject		DISKOBJECT;
typedef struct FileHandle		FILEHANDLE;
typedef struct FileInfoBlock	FILEINFOBLOCK;
typedef struct FileLock			LOCK;
typedef struct Image			IMAGE;
typedef struct Message			MESSAGE;
typedef struct MsgPort			MSGPORT;
typedef struct StandardPacket	STANDARDPACKET;

#define MAX_HANDLES		20

static FILEHANDLE *fileTable[ MAX_HANDLES ] = { NULL };

/* Find a free entry in the file table */

static int getHandle( void )
	{
	int i;

	/* Search the file table for a free handle */
	for( i = 0; i < MAX_HANDLES; i++ )
		if( fileTable[ i ] == NULL )
			return( i );

	/* File table full */
	return( ERROR );
	}

/* Free a file table entry */

static inline void freeHandle( const FD theFile )
	{
	fileTable[ theFile ] = NULL;
	}

/* All directories are accessed via read locks.  If an error occurs, we have
   to clear the read locks or they can't be written to any more.  We do this
   by keeping a global table of locks and stepping through unlocking
   directories when we perform an error exit */

#define MAX_LOCKS		25

static LOCK *lockTable[ MAX_LOCKS ] = { NULL };
static int lockCount = 0;

static inline void addLock( LOCK *theLock )
	{
	if( lockCount < MAX_LOCKS )
		lockTable[ lockCount++ ] = theLock;
	else
		/* Should never happen */
		puts( "Lock table overflow" );
	}

static inline void clearLastLock( void )
	{
	if( lockCount )
		lockTable[ --lockCount ] = NULL;
	else
		/* Should never happen */
		puts( "Lock table underflow" );
	}

void clearLocks( void )
	{
	while( lockCount )
		{
		UnLock( ( BPTR ) lockTable[ --lockCount ] );
		lockTable[ lockCount ] = NULL;
		}
	}

/****************************************************************************
*																			*
*								HPACKIO Functions							*
*																			*
****************************************************************************/

/* Create a new file - use Open() with the MODE_NEWFILE parameter */

FD hcreat( const char *fileName, const int attr )
	{
	FILEHANDLE *theHandle;
	FD theFD;

	if( ( theHandle = ( FILEHANDLE * ) Open( ( char * ) fileName, MODE_NEWFILE ) ) == NULL ||
		( theFD = getHandle() ) == ERROR )
		return( ERROR );

	fileTable[ theFD ] = theHandle;
	return( theFD );
	}

/* Open an existing file */

FD hopen( const char *fileName, const int mode )
	{
	FILEHANDLE *theHandle;
	LONG openMode;
	FD theFD;

	if( ( ( mode & S_MASK ) == S_DENYNONE ) || \
		( ( mode & O_MASK ) == O_RDONLY ) )
		openMode = MODE_OLDFILE;		/* Allow shared access */
	else
		openMode = MODE_READWRITE;		/* Use exclusive access */

	if( ( theHandle = ( FILEHANDLE * ) Open( ( char * ) fileName, openMode ) ) == NULL ||
		( theFD = getHandle() ) == ERROR )
		return( ERROR );

	fileTable[ theFD ] = theHandle;
	return( theFD );
	}

/* Close a file */

int hclose( const FD theFile )
	{
	Close( ( BPTR ) fileTable[ theFile ] );
	freeHandle( theFile );
	return( OK );
	}

/* Read data from a file */

int hread( const FD theFile, void *buffer, const unsigned int count )
	{
	return( ( int ) Read( ( BPTR ) fileTable[ theFile ], buffer, ( long ) count ) );
	}

/* Write data to a file */

int hwrite( const FD theFile, void *buffer, const unsigned int count )
	{
	return( ( int ) Write( ( BPTR ) fileTable[ theFile ], buffer, ( long ) count ) );
	}

/* Seek to a position in a file */

long hlseek( const FD theFile, const long offset, const int origin )
	{
	/* The AmigaDOS Seek() call returns the *previous* position rather than
	   the current one, so we need to perform two calls, one to move the
	   pointer and a second one to find out where we now are.  In addition
	   the origin codes are SEEK_SET = -1, SEEK_CURR = 0, SEEK_END = 1 so
	   we subtract one from the given codes to get the Amiga ones */
	if( Seek( ( BPTR ) fileTable[ theFile ], offset, ( long ) origin - 1 ) != IO_ERROR )
		return( Seek( ( BPTR ) fileTable[ theFile ], 0L, OFFSET_CURRENT ) );
	else
		return( IO_ERROR );
	}

/* Return the current position in a file */

long htell( const FD theFile )
	{
	return( Seek( ( BPTR ) fileTable[ theFile ], 0L, OFFSET_CURRENT ) );
	}

/* Truncate a file at the current position.  Needs KS 2.0 or above.  Under
   KS 1.3 it's possible by sending an ACTION_SET_FILE_SIZE packet to the
   file handler */

int htruncate( const FD theFile )
	{
	return( ( SetFileSize( theFile, OFFSET_BEGINNING, htell( theFile ) ) != IO_ERROR ) ?
			OK : IO_ERROR );
	}

/* Remove a file */

int hunlink( const char *fileName )
	{
	return( DeleteFile( ( char * ) fileName ) ? OK : ERROR );
	}

/* Create a directory.  Since the lock is only transient, we don't bother
   adding it to the lock table */

int hmkdir( const char *dirName, const int attr )
	{
	LOCK *dirLock;

	if( ( dirLock = ( LOCK * ) CreateDir( ( char * ) dirName ) ) == NULL )
		return( ERROR );
	UnLock( ( BPTR ) dirLock );
	return( OK );
	}

/* Rename a file */

int hrename( const char *oldName, const char *newName )
	{
	return( Rename( ( char * ) oldName, ( char * ) newName ) ? OK : ERROR );
	}

/* Set/change a file's attributes */

int hchmod( const char *fileName, const int attr )
	{
	return( SetProtection( ( char * ) fileName, ( long ) attr ) ? OK : ERROR );
	}

/****************************************************************************
*																			*
*								HPACKLIB Functions							*
*																			*
****************************************************************************/

/* Modes for the CON: handler (raw and cooked) */

#define CON_MODE	0
#define RAW_MODE	1

/* Get a char, no echo.  Needs KS 2.0 or above.  Under KS 1.x it's possible
   by sending an ACTION_SCREEN_MODE packet to the console handler */

#ifdef __SASC_60

extern int getch( void );	/* It's in <stdio.h> but it ain't ANSI ... */

int hgetch( void )
	{
	return( getch() );
	}

#else

int hgetch( void )
	{
	int ch;

	SetMode( Input(), RAW_MODE );
	Write( Output(), "\x1b[20l", 5 );	/* Turn off CRLF translation */
	ch = getchar();
	SetMode( Input(), CON_MODE );
	return( ch );
	}
#endif /* __SASC_60 */

/****************************************************************************
*																			*
*								SYSTEM Functions							*
*																			*
****************************************************************************/

/* Set a file's timestamp.  Needs KS 2.0.  Under 1.2 or 1.3 it's possible by
   sending an ACTION_SET_DATE packet to the file's handler */

#define SECS_PER_DAY	86400		/* Seconds per day */

void setFileTime( const char *fileName, const LONG fileTime )
	{
	struct DateStamp dateStamp;
#if 0
	MSGPORT *taskPort;
	LOCK *dirLock, *fileLock;
	MSGPORT *replyPort;
	STANDARDPACKET packet;
	char pFileName[ MAX_FILENAME + 1 ];
#endif /* 0 */
	long seconds;

	/* Extract the day, minute, tick counts and put in the DateStamp structure */
	seconds = fileTime - AMIGA_TIME_OFFSET;
	dateStamp.ds_Days = seconds / SECS_PER_DAY;
	seconds %= SECS_PER_DAY;
	dateStamp.ds_Minute  = seconds / 60;
	dateStamp.ds_Tick = ( seconds % 60 ) * TICKS_PER_SECOND;

	/* Set the timestamp */
	SetFileDate( ( char * ) fileName, &dateStamp );

#if 0
	/* Try and open a message port to the file, and put a lock on it */
	if( ( taskPort = ( MSGPORT * ) DeviceProc( fileName ) ) == NULL ||
		( fileLock = ( LOCK * ) Lock( fileName, SHARED_LOCK ) ) == NULL )
		return;

	/* Get a lock on the directory containing the file, and convert the
	   filename to a Pascal string (euurgghh, Macintoshness!) */
	dirLock = ( LOCK * ) ParentDir( fileLock );
	UnLock( fileLock );
	strcpy( pFileName + 1, findFilenameStart( fileName ) );
	*pFileName = strlen( pFileName + 1 );

	/* Get a port for the message reply */
	if( replyPort = ( MSGPORT * ) CreatePort( 0L, 0L ) != NULL )
		{
		/* Set up the packet to send */
		packet.sp_Msg.mn_Node.ln_Name = ( char * ) &( packet.sp_Pkt );
		packet.sp_Pkt.dp_Link = &( packet.sp_Msg );
		packet.sp_Pkt.dp_Port = replyPort;
		packet.sp_Pkt.dp_Type = ACTION_SET_DATE;
		packet.sp_Pkt.dp_Arg1 = ( LONG ) NULL;
		packet.sp_Pkt.dp_Arg2 = ( LONG ) dirLock;
		packet.sp_Pkt.dp_Arg3 = ( LONG ) pFileName >> 2;
		packet.sp_Pkt.dp_Arg4 = ( LONG ) &dateStamp;

		/* Send the message and wait for a reply */
		PutMsg( taskPort, ( MESSAGE * ) &packet );
		WaitPort( replyPort );
		GetMsg( replyPort );	/* Status = packet.sp_Pkt.dp_Res1 */
		DeletePort( replyPort );
		}

	/* Clean up */
	UnLock( dirLock );
#endif /* 0 */
	}

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
	return( 0 );	/* Default to US */
	}
#endif /* GUI */

/* Find the first/next file in a directory.  The Amiga requires that a
   lock be placed on a directory before it is accessed.  These functions
   try to unlock a directory as soon as possible once they've finished with
   it rather than relying on findEnd(), to allow other processes to access
   the information as well */

static void getFileInfo( FILEINFO *fileInfo )
	{
	FILEINFOBLOCK *infoBlock = ( FILEINFOBLOCK * ) fileInfo->infoBlock; /* Took "&" off parameter, PNM 22 Nov 92 */

	/* Copy relevant fields across to fileInfo block.  When converting the
	   timestamp we approximate 1 tick == 1 second, which is close enough.
	   Note the redundant expression for isDir - this is necessary to get
	   around yet another bug in Lattice C (present in 5.03, but not
	   apparently 5.10 or greater) */
	fileInfo->isDir = ( infoBlock->fib_DirEntryType > 0 ) ? TRUE : FALSE;
	strcpy( fileInfo->fName, infoBlock->fib_FileName );
	fileInfo->fSize = infoBlock->fib_Size;
	fileInfo->fTime = AMIGA_TIME_OFFSET +
					  ( infoBlock->fib_Date.ds_Days * 86400 ) +
					  ( infoBlock->fib_Date.ds_Minute * 60 ) +
					  ( infoBlock->fib_Date.ds_Tick / TICKS_PER_SECOND );
	fileInfo->fAttr = infoBlock->fib_Protection;
	fileInfo->hasComment = *infoBlock->fib_Comment ? TRUE : FALSE;
	}

BOOLEAN findFirst( const char *pathName, const ATTR fileAttr, FILEINFO *fileInfo )
	{
	char filePath[ MAX_PATH ];
	int filePathLen = strlen( ( char * ) pathName );
	BOOLEAN matchIndividual = TRUE;

	fileInfo->matchAttr = fileAttr;
	fileInfo->infoBlock = NULL;

	/* If we want to open a directory we need to zap the slash to get a pure
	   directory path component */
	strcpy( filePath, ( char * ) pathName );
	if( !filePathLen )
		/* No path, match all files in current directory */
		matchIndividual = FALSE;
	else
		/* Check for whole-directory match */
		if( filePath[ filePathLen - 1 ] == SLASH )
			{
/*			filePath[ filePathLen - 1 ] = '\0';		Kultur */
			matchIndividual = FALSE;
			}
		else
			if( filePath[ filePathLen - 1 ] == ':' )
				matchIndividual = FALSE;

	/* Place a lock on the file/directory, allocate room for the
	   information, and get it */
	if( ( fileInfo->infoBlock = AllocMem( sizeof( FILEINFOBLOCK ),
										  MEMF_PUBLIC | MEMF_CLEAR ) ) == NULL )
		error( OUT_OF_MEMORY );
	if( ( fileInfo->lock = ( void * ) Lock( filePath, ACCESS_READ ) ) == NULL )
		return( FALSE );
	if( !Examine( ( BPTR ) fileInfo->lock,
				  ( FILEINFOBLOCK * ) fileInfo->infoBlock ) ) /* Took "&" off parameter, PNM 22 Nov 92 */
		{
		UnLock( ( BPTR ) fileInfo->lock ); /* Took "&" off parameter, PNM 22 Nov 92 */
		fileInfo->lock = NULL;
		FreeMem( fileInfo->infoBlock, sizeof( FILEINFOBLOCK ) );
		fileInfo->infoBlock = NULL;
		return( FALSE );
		}
	addLock( ( LOCK * ) fileInfo->lock ); /* Took "&" off parameter, PNM 22 Nov 92 */
	getFileInfo( fileInfo );

	/* If we're looking for files within a directory, we need to call
	   findNext() since the Examine() call will read the parent directory
	   not it's contents.  ExNext() reads the contents */
	if( !matchIndividual )
		return( findNext( fileInfo ) );

	/* Return FALSE if we've found the wrong type of file */
	return( ( ( fileInfo->matchAttr != FILES_DIRS ) && fileInfo->isDir ) ?
			FALSE : TRUE );
	}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
	do
		/* Try and get info on the next file/dir */
		if( !ExNext( ( BPTR ) fileInfo->lock,
					 ( FILEINFOBLOCK * ) fileInfo->infoBlock ) ) /* Took "&" off parameter, PNM 22 Nov 92 */
			{
			UnLock( ( BPTR ) fileInfo->lock ); /* Took "&" off parameter, PNM 22 Nov 92 */
			clearLastLock();
			fileInfo->lock = NULL;
			FreeMem( fileInfo->infoBlock, sizeof( FILEINFOBLOCK ) );
			fileInfo->infoBlock = NULL;
			return( FALSE );
			}
	/* Sometimes we only want to match files, not directories */
	while( ( fileInfo->matchAttr == ALLFILES ) ?
		   ( ( FILEINFOBLOCK * ) fileInfo->infoBlock)->fib_DirEntryType > 0 : 0 );

	getFileInfo( fileInfo );
	return( TRUE );
	}

void findEnd( FILEINFO *fileInfo )
	{
	/* Unlock the directory (rarely used since it's usually done by findNext()) */
	if( fileInfo->lock != NULL )
		{
		UnLock( ( BPTR ) fileInfo->lock ); /* Took "&" off parameter, PNM 22 Nov 92 */
		clearLastLock();
		fileInfo->lock = NULL;
		}

	/* Free the memory used by the FileInfoBlock (also rarely used since
	   findNext() does it */
	if( fileInfo->infoBlock != NULL )
		{
		FreeMem( fileInfo->infoBlock, sizeof( FILEINFOBLOCK ) );
		fileInfo->infoBlock = NULL;
		}
	}

/* Get any randomish information the system can give us (not terribly good,
   but every extra bit helps - if it's there we may as well use it) */

void getRandomInfo( BYTE *buffer, int bufSize )
	{
	BYTE *vbeamPtr = ( BYTE * ) 0xDFF007L;
	BYTE *thingPtr = ( BYTE * ) 0xBFE801L;
	BYTE *clockPtr = ( BYTE * ) 0xDC0001L;

	mputLong( buffer, ( LONG ) ( ( *vbeamPtr << 24 ) | ( *thingPtr << 16 ) | \
								 ( *clockPtr << 8 ) ) );
	}

/* Store the comment for a file/directory in an archive in HPACK tagged format */

int storeComment( const FILEINFO *fileInfo, const FD outFD )
	{
	int commentLength = strlen( ( ( FILEINFOBLOCK * ) fileInfo->infoBlock )->fib_Comment );
	int dataLength = commentLength, i;
	BOOLEAN isDir = !outFD;

	/* Write the comment tag header */
	if( isDir )
		dataLength += addDirData( TAG_COMMENT, TAGFORMAT_STORED, commentLength, LEN_NONE );
	else
		dataLength += writeTag( TAG_COMMENT, TAGFORMAT_STORED, commentLength, LEN_NONE );

	/* Now write the comment data */
	if( isDir )
		{
		/* Write comment to directory data area */
		checksumDirBegin( RESET_CHECKSUM );
		for( i = 0; i < commentLength; i++ )
			fputDirByte( ( ( FILEINFOBLOCK * ) fileInfo->infoBlock )->fib_Comment[ i ] );
		checksumDirEnd();
		fputDirWord( crc16 );
		}
	else
		{
		/* Write comment to file data area */
		checksumBegin( RESET_CHECKSUM );
		for( i = 0; i < commentLength; i++ )
			fputByte( ( ( FILEINFOBLOCK * ) fileInfo->infoBlock )->fib_Comment[ i ] );
		checksumEnd();
		fputWord( crc16 );
		}

	return( ( int ) ( dataLength + sizeof( WORD ) ) );
	}

/* Set the comment for a file/directory from an HPACK tag */

void setComment( const char *filePath, const TAGINFO *tagInfo, const FD srcFD )
	{
	char comment[ COMMENT_SIZE ];
	int i, commentSize = ( tagInfo->dataLength > COMMENT_SIZE - 1 ) ?
						 COMMENT_SIZE - 1 : tagInfo->dataLength;

	/* Make sure we can read the comment */
	if( tagInfo->dataFormat != TAGFORMAT_STORED )
		{
		/* Don't know how to handle compressed comments yet, skip it */
		skipSeek( tagInfo->dataLength );
		return;
		}

	/* Read in as much of the comment as we can, skip the rest */
	for( i = 0; i < commentSize; i++ )
		comment[ i ] = fgetByte();
	comment[ i ] = '\0';
	if( tagInfo->dataLength - commentSize )
		skipSeek( tagInfo->dataLength - commentSize );

	/* Set the file's comment */
	SetComment( ( char * ) filePath, comment );
	}

/* Initialise/cleanup functions for extra info handling */

struct Library *IconLib = NULL;

void initExtraInfo( void )
	{
	/* Try and open icon library */
	if( ( IconLib = OpenLibrary( "icon.library", 33 ) ) == NULL )
		error( INTERNAL_ERROR );
	}

void endExtraInfo( void )
	{
	/* Close icon library */
	if( IconLib != NULL )
		CloseLibrary( IconLib );
	}

/* Set extra info for an archive, copy extra info from one file to another */

void setExtraInfo( const char *filePath )
	{
	/* The HPACK icon image (there should be four of these, one per file
	   type of normal, secured, encrypted, both) */

	__chip static UWORD hpackIconData[] = {
					/* Icon image data */
		0xffff,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0xffff,

		0xffff,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0x8001,
		0xffff
		};

	static IMAGE hpackIconImage = {
		0, 0,				/* Top corner */
		16, 16, 2,			/* Width, height, depth */
		hpackIconData,		/* Data */
		0, 0,				/* PlanePick, PlaneOnOff */
		NULL				/* Next image */
		};

	/* The tool types for normal, secured, encrypted, and both, archives */
	static BYTE *normalToolType[] = { "FILETYPE=HPAK", NULL };
	static BYTE *securedToolType[] = { "FILETYPE=HPKS", NULL };
	static BYTE *encrToolType[] = { "FILETYPE=HPKC", NULL };
	static BYTE *bothToolType[] = { "FILETYPE=HPKB", NULL };

	/* The disk object for the HPACK icon */
	static DISKOBJECT hpackIcon = {
		WB_DISKMAGIC,			/* Magic number */
		WB_DISKVERSION,			/* Structure version number */
			{					/* Embedded gadget structure */
			NULL,				/* Next gadget pointer */
			0, 0, 16, 16,		/* Left, top, width, height */
			GADGIMAGE | GADGHBOX,	/* Flags (must use) */
			GADGIMMEDIATE | RELVERIFY, /* Activ.flags (must use) */
			BOOLGADGET,			/* Type (must use) */
			&hpackIconImage,	/* Image data */
			NULL, NULL, NULL, NULL, 0, NULL	/* Unused fields */
			},
		WBPROJECT,				/* Icon type */
		"Hpack",				/* Default tool name */
		normalToolType,			/* Tool type array */
		NO_ICON_POSITION, NO_ICON_POSITION,	/* Put it anywhere */
		NULL, NULL,				/* Unused fields */
		20000					/* Stack size for tool */
		};

	/* Set the icon image and tool type depending on what the archive type is */
	switch( cryptFlags & ( CRYPT_PKE_ALL | CRYPT_CKE_ALL | CRYPT_SIGN_ALL ) )
		{
		case CRYPT_PKE_ALL:
		case CRYPT_CKE_ALL:
			/* Encrypted archive */
			hpackIcon.do_ToolTypes = encrToolType;
			break;

		case CRYPT_SIGN_ALL:
			/* Secured archive */
			hpackIcon.do_ToolTypes = securedToolType;
			break;

		case CRYPT_PKE_ALL | CRYPT_SIGN_ALL:
		case CRYPT_CKE_ALL | CRYPT_SIGN_ALL:
			/* Encrypted + secured archive */
			hpackIcon.do_ToolTypes = bothToolType;
			break;
		}

	/* Write out the new disk object */
	PutDiskObject( ( char * ) filePath, &hpackIcon );
	}

void copyExtraInfo( const char *srcFilePath, const char *destFilePath )
	{
	DISKOBJECT *iconObject;

	/* Read in the .info data from the source and write it to the destination */
	if( ( iconObject = GetDiskObject( ( char * ) srcFilePath ) ) != NULL )
		{
		PutDiskObject( ( char * ) destFilePath, iconObject );
		FreeDiskObject( iconObject );
		}
	}

/* Store the DiskObject associated with a file/directory, in an archive in
   HPACK tagged form */

#define calcImageSize(theImage)	( ( ( ( theImage.Width ) + 15 ) & 0xFFF0 ) * theImage.Height * theImage.Depth )

LONG storeDiskObject( const char *pathName, const FD outFD )
	{
	DISKOBJECT *theObject;
	int i, iconImageSize, iconLength;
	BOOLEAN isDir = !outFD;
	IMAGE *theImage;

	/* Try and get the DiskObject for the file/directory */
	if( ( theObject = GetDiskObject( ( char * ) pathName ) ) == NULL )
		return( 0L );

	/* Perform a few sanity checks on the icon information.  According to
	   the ROM Kernel reference manual we should only handle icons with the
	   following magic values set up */
	if( ( theObject->do_Gadget.Flags & GADGIMAGE ) &&
		( theObject->do_Gadget.Activation == ( GADGIMMEDIATE | RELVERIFY ) ) &&
		( theObject->do_Gadget.GadgetType == BOOLGADGET ) )
		{
		/* It's a icon gadget, store it as an AMIGA_ICON tag */
		theImage = ( IMAGE * ) theObject->do_Gadget.GadgetRender;
		iconImageSize = calcImageSize( ( * theImage ) );
		iconLength = sizeof( WORD ) + sizeof( WORD ) + sizeof( WORD ) +
					 sizeof( WORD ) + sizeof( WORD ) + sizeof( BYTE ) +
					 sizeof( BYTE ) + sizeof( BYTE) + iconImageSize +
					 sizeof( WORD );
		if( isDir )
			{
			/* Write directory icon data */
			addDirData( TAG_AMIGA_ICON, TAGFORMAT_STORED, iconLength, LEN_NONE );
			checksumDirBegin( RESET_CHECKSUM );
			fputDirWord( theImage->LeftEdge );
			fputDirWord( theImage->TopEdge );
			fputDirWord( theImage->Width );
			fputDirWord( theImage->Height );
			fputDirWord( theImage->Depth );
			fputDirByte( theImage->PlanePick );
			fputDirByte( theImage->PlaneOnOff );
			fputDirByte( theObject->do_Type );
			for( i = 0; i < iconImageSize; i++ )
				fputDirWord( theImage->ImageData[ i ] );
			checksumDirEnd();
			fputDirWord( crc16 );
			}
		else
			{
			/* Write file icon data */
			writeTag( TAG_AMIGA_ICON, TAGFORMAT_STORED, iconLength, LEN_NONE );
			checksumBegin( RESET_CHECKSUM );
			fputWord( theImage->LeftEdge );
			fputWord( theImage->TopEdge );
			fputWord( theImage->Width );
			fputWord( theImage->Height );
			fputWord( theImage->Depth );
			fputByte( theImage->PlanePick );
			fputByte( theImage->PlaneOnOff );
			fputByte( theObject->do_Type );
			for( i = 0; i < iconImageSize; i++ )
				fputWord( theImage->ImageData[ i ] );
			checksumEnd();
			fputWord( crc16 );
			}
		}

	FreeDiskObject( theObject );
	}

/* Set the icon for a file/directory */

static DISKOBJECT defaultDiskObject = {
		WB_DISKMAGIC,			/* Magic number */
		WB_DISKVERSION, 		/* Structure version number */
			{					/* Embedded gadget structure */
			NULL,				/* Next gadget pointer */
			0, 0, 0, 0,			/* Left, top, width, height */
			GADGIMAGE | GADGHCOMP,	/* Flags (must use) */
			GADGIMMEDIATE | RELVERIFY,	/* Activ.flags (must use) */
			BOOLGADGET,			/* Type (must use) */
			NULL,				/* Image data */
			NULL, NULL, NULL, NULL, 0, NULL	/* Unused fields */
			},
		WBPROJECT,				/* Icon type */
		"",						/* Default tool name */
		NULL,					/* Tool type array */
		NO_ICON_POSITION, NO_ICON_POSITION,	/* Put it anywhere */
		NULL, NULL,				/* Unused fields */
		0L						/* Stack size for tool */
		};

void setIcon( const char *filePath, const TAGINFO *tagInfo, const FD srcFD )
	{
	DISKOBJECT *theObject;
	IMAGE iconImage, *oldIconImage;
	BYTE oldType;
	int i, iconImageSize;

	/* Try and get an existing DiskObject for the file/directory */
	if( ( theObject = GetDiskObject( ( char * ) filePath ) ) == NULL )
		/* None exists yet, use default object */
		theObject = &defaultDiskObject;

	/* Switch to new ImageData information */
	oldType = theObject->do_Type;
	oldIconImage = ( IMAGE * ) theObject->do_Gadget.GadgetRender;
	theObject->do_Gadget.GadgetRender = ( APTR ) &iconImage;

	/* Set up the Image fields */
	checksumSetInput( tagInfo->dataLength - sizeof( WORD ), RESET_CHECKSUM );
	iconImage.LeftEdge = fgetWord();
	iconImage.TopEdge = fgetWord();
	iconImage.Width = fgetWord();
	iconImage.Height = fgetWord();
	iconImage.Depth = fgetWord();
	iconImage.PlanePick = fgetWord();
	iconImage.PlaneOnOff = fgetWord();
	theObject->do_Type = fgetByte();
	iconImageSize = calcImageSize( iconImage );
	if( ( iconImage.ImageData =
		( WORD * ) hmalloc( iconImageSize * sizeof( WORD ) ) ) == NULL )
		error( OUT_OF_MEMORY );
	for( i = 0; i < iconImageSize; i++ )
		iconImage.ImageData[ i ] = fgetWord();
	iconImage.NextImage = NULL;

	/* If the data was non-corrupted, set the new icon */
	if( fgetWord() == crc16 )
		PutDiskObject( ( char * ) filePath, theObject );

	/* Free up in-memory data structures */
	hfree( iconImage.ImageData );
	theObject->do_Gadget.GadgetRender = ( APTR ) oldIconImage;
	theObject->do_Type = oldType;
	FreeDiskObject( theObject );
	}

#if defined( LATTICE ) && !defined( __SASC )

/* memcpy() which handles overlapping memory ranges */

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
#endif /* LATTICE && !__SASC */
