/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  OS/2-Specific Routines						*
*							  OS2.C  Updated 04/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991 - 1992  Conrad Bullock and John Burnell			*
*								All rights reserved							*
*																			*
****************************************************************************/

/* This file contains a the code for both the 16 and 32-bit versions of
   OS/2.  There is a fair amount of dual-use code, as well as some version-
   specific routines, toggled by defining OS2_32 */

#define INCL_BASE
#define INCL_WINSHELLDATA
#define INCL_WINERRORS
#if defined( __TSC__ )
  #define BYTE	OS2_BYTE
  #include <os2pm.h>
  #undef BYTE
  #undef COMMENT
#elif defined( OS2_32 )
  #define INCL_BASE
  #define INCL_NOPMAPI
  #define BYTE	OS2_BYTE

  #include <os2.h>
  #undef BYTE
#else
  #include <os2.h>
#endif /* __TSC__ */

#undef LONG

#ifdef __TSC__
  #include <alloc.h>
#endif /* __TSC__ */
#include <ctype.h>
#ifdef __TSC__
  #include <dir.h>
#endif /* __TSC__ */
#ifndef OS2_32
  #include <io.h>
#endif /* OS2_32 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "arcdir.h"
#include "frontend.h"
#include "system.h"
#include "wildcard.h"
#include "hpacklib.h"
#include "crc/crc16.h"
#include "io/fastio.h"

#ifndef OS2_32

/* Prototypes for memory-management functions */

void far *farmalloc( unsigned long num );
void far *farrealloc( void far *block, unsigned long num );
void farfree( void far *block );
#endif /* OS2_32 */

/* Whether this file is being added from an HPFS drive or not */

BOOLEAN isHPFS = FALSE;
BOOLEAN destIsHPFS = FALSE;

/* The drive the archive is on */

BYTE archiveDrive;

/* Get an input char, no echo */

int hgetch( void )
	{
#ifdef OS2_32
	return _read_kbd( 0, 1, 1 );
#else
	return( getch() );
#endif /* OS2_32 */
	}

/****************************************************************************
*																			*
*								HPACKLIB Functions							*
*																			*
****************************************************************************/

/* Memory management functions.  The OS2_32 ones could be aliased in
   hpacklib.h since under OS/2 2.0 we don't have the memory limitations of
   the 16-bit versions of OS/2, but to be consistent we implement them as
   wrappers for the std. library calls as in the 16-bit version */

#ifdef OS2_32
void *hmalloc( const unsigned long size )
	{
	return( malloc( size ) );
	}

void *hrealloc( void *buffer, const unsigned long size )
	{
	return( realloc( buffer, size ) );
	}

void hfree( void *buffer )
	{
	free( buffer );
	}
#else

void *hmalloc( const unsigned long size )
	{
	if( size > 0xFFFF )
		/* Farm alloc?  Out of pasture space - don't have a cow man */
		return( farmalloc( size ) );
	else
		return( malloc( ( int ) size ) );
	}

void *hrealloc( void *buffer, const unsigned long size )
	{
	if( size > 0xFFFF )
		return( farrealloc( buffer, size ) );
	else
		return( realloc( buffer, ( int ) size ) );
	}

void hfree( void *buffer )
	{
	free( buffer );
	}
#endif /* OS2_32 */

/****************************************************************************
*																			*
*								HPACKIO Functions							*
*																			*
****************************************************************************/

/* Functions to open and create files in binary mode */

int hopen( const char *fileName, const int mode )
	{
	HFILE hFile;
#ifdef OS2_32
	ULONG action;
#else
	USHORT action;
#endif /* OS2_32 */

	return( ( DosOpen( ( PSZ ) fileName, &hFile, &action, 0, \
					   FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS, mode, 0 ) == NO_ERROR ) ? \
			hFile : IO_ERROR );
	}

int hcreat( const char *fileName, const int attr )
	{
	HFILE hFile;
#ifdef OS2_32
	ULONG action;
#else
	USHORT action;
#endif /* OS2_32 */

	return( ( DosOpen( ( PSZ ) fileName, &hFile, &action, 0, FILE_NORMAL, \
					   OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS, \
					   attr, 0 ) == NO_ERROR ) ? hFile : IO_ERROR );
	}

/* Close a file */

int hclose( const FD theFD )
	{
	return( ( DosClose( theFD ) == NO_ERROR ) ? OK : IO_ERROR );
	}

/* Create a directory */

int hmkdir( const char *dirName, const int attr )
	{
#ifdef OS2_32
	return( ( DosCreateDir( ( PSZ ) dirName, 0 ) == NO_ERROR ) ? OK : IO_ERROR );
#else
	return( ( mkdir( dirName ) == NO_ERROR ) ? OK : IO_ERROR );
#endif
	}

/* Seek to a specified position in a file, and return the current position
   in a file */

long hlseek( const FD theFD, const long position, const int whence )
	{
	LONG where;

#ifdef OS2_32
	return( ( DosSetFilePtr( theFD, position, whence, &where ) == NO_ERROR ) ? \
			where : IO_ERROR );
#else
	return( ( DosChgFilePtr( theFD, position, whence, &where ) == NO_ERROR ) ? \
			where : IO_ERROR );
#endif
	}

long htell( const FD theFD )
	{
	ULONG where;

#ifdef OS2_32
	if( DosSetFilePtr( theFD, 0L, FILE_CURRENT, &where ) != NO_ERROR )
#else
	if( DosChgFilePtr( theFD, 0L, FILE_CURRENT, &where ) != NO_ERROR )
#endif
		return( IO_ERROR );
	return( where );
	}

/* Read/write a specified amount of data from/to a file */

int hread( const FD inFD, void *buffer, const unsigned int bufSize )
	{
#ifdef OS2_32
	ULONG bytesRead;
#else
	USHORT bytesRead;
#endif /* OS2_32 */

	return( ( DosRead( inFD, buffer, bufSize, &bytesRead ) == NO_ERROR ) ? \
			( int ) bytesRead : IO_ERROR );
	}

int hwrite( const FD outFD, void *buffer, const unsigned int bufSize )
	{
#ifdef OS2_32
	USHORT bytesWritten;
#else
	ULONG bytesWritten;
#endif /* OS2_32 */
	int bytesToWrite = bufSize;
	FSALLOCATE fsAllocate;
	ULONG SpaceAvail;
	USHORT Result;

	if( DosWrite( outFD, buffer, bytesToWrite, &bytesWritten ) != NO_ERROR )
		return( IO_ERROR );

	/* Fix the behaviour of DosWrite() by retrying the write with less data
	   if it fails (the DOS/OS2 write() is a boolean all-or-nothing affair
	   rather than doing the Right Thing as writing as much as it can) */
	if( ( int ) bytesWritten != bytesToWrite )
		{
#ifdef OS2_32
		if( DosQueryFSInfo( archiveDrive, FSIL_ALLOC, &fsAllocate, \
						sizeof( fsAllocate ) ) != NO_ERROR )
#else
		if( DosQFSInfo( archiveDrive, FSIL_ALLOC, &fsAllocate, \
						sizeof( fsAllocate ) ) != NO_ERROR )
#endif
			return( IO_ERROR );

		/* Calculate the disk space available and retry the write with that
		   amount */
		SpaceAvail = fsAllocate.cUnitAvail * fsAllocate.cSectorUnit * fsAllocate.cbSector;
		return( ( DosWrite( outFD, buffer, SpaceAvail, &bytesWritten ) == NO_ERROR) ? \
				bytesWritten : IO_ERROR );
		}
	else
		return( bytesWritten );
	}

/* Change a file's attributes */

int hchmod( const char *fileName, const WORD attr )
	{
#ifdef OS2_32
	FILESTATUS3 fileInfo;

	if( DosQueryPathInfo( ( PSZ ) fileName, FIL_STANDARD, &fileInfo, \
						  sizeof( fileInfo ) ) != NO_ERROR )
		return( IO_ERROR );
	fileInfo.attrFile = attr;
	return( ( DosSetPathInfo( ( PSZ ) fileName, FIL_STANDARD, &fileInfo, \
							  sizeof( fileInfo ), 0 ) == NO_ERROR ) ? \
			OK : IO_ERROR );
#else
	return( DosSetFileMode( ( PSZ ) fileName, attr, 0L ) );
#endif /* OS2_32 */
	}

/* Delete a file */

int hunlink( const char *fileName )
	{
#ifdef OS2_32
	return( ( DosDelete( ( PSZ ) fileName ) == NO_ERROR ) ? OK : IO_ERROR );
#else
	return( ( DosDelete( ( PSZ ) fileName, 0L ) == NO_ERROR ) ? OK : IO_ERROR );
#endif
	}

/* Rename a file */

int hrename( const char *srcName, const char *destName )
	{
#ifdef OS2_32
	return( ( DosMove( ( PSZ ) srcName, ( PSZ ) destName ) == NO_ERROR ) ? OK : IO_ERROR );
#else
	return( ( DosMove( ( PSZ ) srcName, ( PSZ ) destName, 0L ) == NO_ERROR ) ? OK : IO_ERROR );
#endif /* OS2_32 */
	}

int htruncate ( const FD theFile )
	{
	ULONG FilePos;

#ifdef OS2_32
	DosSetFilePtr( ( HFILE ) theFile, 0L, FILE_CURRENT, &FilePos );
	return( ( DosSetFileSize( ( HFILE ) theFile, FilePos ) == NO_ERROR ) ? OK : IO_ERROR );
#else
	DosChgFilePtr( ( HFILE ) theFile, 0L, FILE_CURRENT, &FilePos );
	return( ( DosNewSize( ( HFILE ) theFile, FilePos )  == NO_ERROR ) ? OK : IO_ERROR );
#endif
	}

/****************************************************************************
*																			*
*							SYSTEM Functions								*
*																			*
*****************************************************************************

/* Which of the three file dates to set */

typedef enum { LASTWRITE, LASTACCESS, CREATION } TIMES;

/* Local function to set one of the timestamps of a file (or directory) */

static void setOneOfFileTimes( const char *fileName, const LONG Time,
							   const TIMES TimeToSet )
	{
	FILESTATUS info;
	FDATE date;
	FTIME time;
	struct tm *tmtime;

	tmtime = localtime( ( time_t * ) &Time );

	time.twosecs = tmtime->tm_sec / 2;
	time.minutes = tmtime->tm_min;
	time.hours = tmtime->tm_hour ;
	date.day = tmtime->tm_mday;
	date.month = tmtime->tm_mon + 1;
	date.year = tmtime->tm_year - 80;

	/* Get information for file */
#ifdef OS2_32
	DosQueryPathInfo( ( PSZ ) fileName, FIL_STANDARD, &info, sizeof( info ) );
#else
	DosQPathInfo( ( PSZ ) fileName, FIL_STANDARD, ( PBYTE ) &info, \
				  sizeof( info ), 0L );
#endif /* OS2_32 */

	switch( TimeToSet )
		{
		case LASTWRITE:
			info.fdateLastWrite = date;
			info.ftimeLastWrite = time;
			break;

		case LASTACCESS:
			info.fdateLastAccess = date;
			info.ftimeLastAccess = time;
			break;

		case CREATION:
			info.fdateCreation = date;
			info.ftimeCreation = time;
			break;
        }
		/* The default is not to change the data */

#ifdef OS2_32
	DosSetPathInfo( ( PSZ ) fileName, FIL_STANDARD, &info, \
					sizeof( info ), 0 );
#else
	DosSetPathInfo( ( PSZ ) fileName, FIL_STANDARD,( PBYTE) &info, \
					sizeof( info ), 0, 0L );
#endif /* OS2_32 */
	}

/* Set a files (or directories) timestamp */

void setFileTime( const char *fileName, const LONG fileTime )
	{
	setOneOfFileTimes( fileName, fileTime, LASTWRITE );
	}

/* Set various other file timestamps */

void setAccessTime( const char *fileName, const LONG accessTime )
	{
	setOneOfFileTimes( fileName, accessTime, LASTACCESS );
	}

void setCreationTime( const char *fileName, const LONG creationTime )
	{
	setOneOfFileTimes( fileName, creationTime, CREATION );
	}

int getCountry( void )
	{
	COUNTRYCODE CountCode;
	COUNTRYINFO CountInfo;
	HAB hab;
#ifdef OS2_32
	ULONG retLen;
#else
	USHORT retLen;
#endif /* OS2_32 */
	USHORT result;

#if defined( OS2_32 ) && defined( __gcc )
	typedef LHANDLE	HINI;
	#define HINI_USERPROFILE	( HINI ) -1L
	SHORT PrfQueryProfileInt( HINI hini, PSZ pszApp, PSZ pszKey, SHORT sDefault );
#endif /* OS2_32 && __gcc */

	/* Clear country and codepage */
	CountCode.country = 0;
	CountCode.codepage = 0;

	/* Try and get country info, defaulting to US if we can't find anything */
#ifdef OS2_32
	DosQueryCtryInfo( sizeof( CountInfo ), &CountCode, &CountInfo, \
					  &retLen );
#else
	DosGetCtryInfo( ( USHORT ) sizeof( CountInfo ), &CountCode, \
					&CountInfo, &retLen );
#endif /* OS2_32 */

	/* Get user-defined date preference */
	hab = WinInitialize( 0 );
	CountInfo.fsDateFmt = PrfQueryProfileInt( HINI_USERPROFILE, \
								"PM_National", "iDate",	CountInfo.fsDateFmt	);
	WinTerminate( hab );

	return( CountInfo.fsDateFmt );
	}

/* Find the first/next file in a directory.  When we do the findFirst() we
   enquire about any EA's attached to the file/directory for later
   processing */

time_t TimetoLong( FDATE FDate, FTIME FTime )
	{
	struct tm timeBuf;

	timeBuf.tm_sec = FTime.twosecs * 2;
	timeBuf.tm_min = FTime.minutes;
	timeBuf.tm_hour = FTime.hours;
	timeBuf.tm_mday = FDate.day;
	if( FDate.month > 0 )
		/* Month range is from 1-12 */
		timeBuf.tm_mon = FDate.month - 1;
	else
		/* Clear date if no access or creation time */
		timeBuf.tm_mon = 0;
	timeBuf.tm_year = FDate.year + 80;	/* Years since 1980 */
	timeBuf.tm_isdst = -1;				/* Daylight saving indeterminate */
	timeBuf.tm_wday = 0;
	timeBuf.tm_yday = 0;

	return( mktime( &timeBuf ) );
	}

BOOLEAN findFirst( const char *filePath, const ATTR fileAttr, FILEINFO *fileInfo )
	{
#ifdef OS2_32
	FILEFINDBUF4 FindBuffer;
	ULONG FindCount = 1;
#else
	FILEFINDBUF2 FindBuffer;
	USHORT FindCount = 1;
#endif /* OS2_32 */
	HDIR FindHandle = HDIR_CREATE;	/* OS allocates new file handle */

#ifdef OS2_32
	if( DosFindFirst( ( PSZ ) filePath, &FindHandle, fileAttr, \
					  &FindBuffer, sizeof( FindBuffer ), &FindCount, \
					  FIL_QUERYEASIZE ) != NO_ERROR )
#else
	if( DosFindFirst2( ( PSZ ) filePath, &FindHandle, fileAttr, \
					   &FindBuffer, sizeof( FindBuffer ), &FindCount, \
					   FIL_QUERYEASIZE, 0L ) )
#endif /* OS2_32 */
		return( FALSE );

	fileInfo->hdir = FindHandle;
	strncpy( fileInfo->fName, FindBuffer.achName, FindBuffer.cchName );
	fileInfo->fName[ FindBuffer.cchName ] = '\0';
	fileInfo->fSize = FindBuffer.cbFile;
	fileInfo->fAttr = FindBuffer.attrFile;
	fileInfo->eaSize = FindBuffer.cbList;
	fileInfo->fTime = TimetoLong( FindBuffer.fdateLastWrite, \
								  FindBuffer.ftimeLastWrite );
	fileInfo->cTime = TimetoLong( FindBuffer.fdateCreation, \
								  FindBuffer.ftimeCreation );
	fileInfo->aTime = TimetoLong( FindBuffer.fdateLastAccess, \
								  FindBuffer.ftimeLastAccess );
	return TRUE;
	}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
#ifdef OS2_32
	FILEFINDBUF4 FindBuffer;
	ULONG FileCount = 1;
#else
	FILEFINDBUF2 FindBuffer;
	USHORT FileCount = 1;
#endif /* OS2_32 */

	if( DosFindNext( fileInfo->hdir, ( PFILEFINDBUF ) &FindBuffer,
					 sizeof( FindBuffer ), &FileCount ) != NO_ERROR )
		return( FALSE );

	strncpy( fileInfo->fName, FindBuffer.achName, FindBuffer.cchName );
	fileInfo->fName[ FindBuffer.cchName ] = '\0';
	fileInfo->fSize = FindBuffer.cbFile;
	fileInfo->fAttr = FindBuffer.attrFile;
	fileInfo->eaSize = FindBuffer.cbList;
	fileInfo->fTime = TimetoLong( FindBuffer.fdateLastWrite, \
								  FindBuffer.ftimeLastWrite );
	fileInfo->cTime = TimetoLong( FindBuffer.fdateCreation, \
								  FindBuffer.ftimeCreation );
	fileInfo->aTime = TimetoLong( FindBuffer.fdateLastAccess, \
								  FindBuffer.ftimeLastAccess );
	return TRUE;
	}

void findEnd( FILEINFO *fileInfo )
	{
	DosFindClose( ( HDIR ) fileInfo->hdir );
	}

/* Check whether two pathnames refer to the same file */

BOOLEAN isSameFile( const char *pathName1, const char *pathName2 )
	{
	char InfoBuf1[ MAX_PATH ], InfoBuf2[ MAX_PATH ];
	USHORT cbInfoBuf = MAX_PATH;

	/* Get the qualified filenames for the two paths */
#ifdef OS2_32
	DosQueryPathInfo( ( PSZ ) pathName1, FIL_QUERYFULLNAME, \
					  &InfoBuf1, MAX_PATH );
	DosQueryPathInfo( ( PSZ ) pathName2, FIL_QUERYFULLNAME, \
					  &InfoBuf2, MAX_PATH );
#else
	DosQPathInfo( ( PSZ ) pathName1, FIL_QUERYFULLNAME, \
				  ( PBYTE ) &InfoBuf1, cbInfoBuf, 0 );
	DosQPathInfo( ( PSZ ) pathName2, FIL_QUERYFULLNAME, \
				  ( PBYTE ) &InfoBuf2, cbInfoBuf, 0 );
#endif /* OS2_32 */

	return( stricmp( InfoBuf1, InfoBuf2 ) );
	}

void getScreenSize( void )
	{
#ifdef OS2_32
	/* Unfortunately the GNU GCC/2 port cannot generated thunked calls to the
	   old 16-bit DLL's. If this code is recompiled with ICC or maybe BCC
	   then the 16-bit code can be used */
	screenWidth = 80;
	screenHeight = 25;
#else
	VIOMODEINFO ModeInfo;

	ModeInfo.cb = sizeof( ModeInfo );
	VioGetMode( &ModeInfo, 0 );

	screenWidth = ModeInfo.col;
	screenHeight = ModeInfo.row;
#endif /* OS2_32 */
	}

/****************************************************************************
*																			*
*					Extended-Attribute Handling Functions					*
*																			*
****************************************************************************/

#include "error.h"
#include "tags.h"

/* The EA's used by HPACK archives */

BYTE EA_HPACK_ARCH_TYPE[] = "Hpack Archive";
BYTE EA_HPACK_ICON[] = { 0x00, 0x00, 0x00, 0x00 };		/* Not yet defined */
BYTE EA_HPACK_ASSOCTBL[] = { 0x00, 0x00, 0x00, 0x00 };	/* Not yet defined */

/* Names for various EA types */

#define OS2_MISC	LONG_TAG( TAG_OS2_MISC_EA )

#define ICON_NAME		".ICON"
#define LONGNAME_NAME	".LONGNAME"

#ifndef OS2_32

/* Variables and data structures used for handling EA's, these are
   defined in the 32 BIT API, so these will be included in os2.h */

typedef struct _FEA2         /* fea2 */
		{
#ifdef __32BIT__					/* This redundant for now, but keep it here
									   just in case */
		ULONG oNextEntryOffset;		/* New field */
#endif
		BYTE fEA;
		BYTE cbName;
		USHORT cbValue;
		CHAR szName[1];				/* New field */
		} FEA2;
typedef FEA2 *PFEA2;

typedef struct _FEA2LIST		/* fea2l */
		{
		ULONG cbList;
		FEA2 list[ 1 ];
		} FEA2LIST;

typedef FEA2LIST *PFEA2LIST;

typedef struct _GEA2			/* gea2 */
		{
#ifdef OS2_32
		/* New field - redundant for now, but keep it here just in case */
		ULONG oNextEntryOffset;
#endif /* OS2_32 */
		BYTE cbName;
		CHAR szName[ 1 ];		/* New field */
		} GEA2;

typedef GEA2 *PGEA2;

typedef struct _GEA2LIST		/* gea2l */
		{
		ULONG cbList;
		GEA2 list[ 1 ];
		} GEA2LIST;

typedef GEA2LIST *PGEA2LIST;

typedef struct _EAOP2			/* eaop2 */
		{
		PGEA2LIST fpGEA2List;	/* GEA set */
		PFEA2LIST fpFEA2List;	/* FEA set */
		ULONG oError;			/* Offset of FEA error */
		} EAOP2;

typedef EAOP2 *PEAOP2;

typedef PVOID *PPVOID;

#define DSPI_WRTTHRU	0x10	/* Write through */

#endif /* OS2_32 */

/* The HoldFEA is used to hold individual EAs.  The member names correspond
   directly to those of the FEA structure.  Note however, that both szName
   and aValue are pointers to the values.  An additional field, next, is
   used to link the HoldFEA's together to form a linked list. */

struct _HoldFEA
	{
#ifdef OS2_32
	ULONG   oNextEntryOffset;	/* Not used in 16 bit code */
#endif /* OS2_32 */
	BYTE fEA;					/* Flag byte */
	BYTE cbName;
	USHORT cbValue;
	CHAR *szName;
	BYTE *aValue;
	struct _HoldFEA *next;
	};

typedef struct _HoldFEA HOLDFEA;

#define MAX_GEA				500L /* Max size for a GEA List */
#define REF_ASCIIZ			1	/* Reference type for DosEnumAttribute */

#define GET_INFO_LEVEL1		1	/* Get info from SFT */
#define GET_INFO_LEVEL2		2	/* Get size of FEAlist */
#define GET_INFO_LEVEL3		3	/* Get FEAlist given the GEAlist */
#define GET_INFO_LEVEL4		4	/* Get whole FEAlist */
#define GET_INFO_LEVEL5		5	/* Get FSDname */

#define SET_INFO_LEVEL1		1	/* Set info in SFT */
#define SET_INFO_LEVEL2		2	/* Set FEAlist */

typedef enum { STORE_WHOLE_EA, STORE_EA_VALUE } TAG_TYPE;

/* Free_FEAList (pFEA)
   Frees the memory used by the linked list of HOLDFEA's pointed to by pFEA */

VOID Free_FEAList( HOLDFEA *pFEA )
	{
	HOLDFEA *next;	/* Holds the next field since we free the structure
					   before reading the current next field */

	/* Step through the list freeing all the fields */
	while( pFEA )
		{
		/* Free the fields of the struct */
		next = pFEA->next;
		if( pFEA->szName != NULL )
			/* Free if non-NULL name */
			hfree(pFEA->szName);
		if( pFEA->aValue != NULL )
			/* Free if non-NULL value */
			hfree(pFEA->aValue);

		/* Free the pFEA struct itself and move on to the next field */
		hfree(pFEA);
		pFEA = next;
		}
	}

/* Read the EA type, this is stored at the start of the EA value */

ULONG getEAType( const CHAR *Value )
	{
	USHORT Type = *( USHORT * ) Value;

	return( Type );
	}

/* Return the EA length, stored in pFEA, this done so that it is calculated
   in only one place */

ULONG getEALength( const HOLDFEA *pFEA )
	{
	return( sizeof( FEA2 )
			- sizeof( CHAR )	/* Don't include the first element of aValue */
			+ pFEA->cbName + 1	/* Length of ASCIIZ name */
			+ pFEA->cbValue );	/* The value length */
	}

/* Set the first two words of the EA value, this is usually the
   EA type and EA size in pFEA, from the values Type and Size */

void setEATypeSize( const HOLDFEA *pFEA, const USHORT Type, const USHORT Size )
	{
	USHORT *valPtr = ( USHORT * ) pFEA->aValue;
	valPtr[ 0 ] = Type;
	valPtr[ 1 ] = Size;
	}

/* Read the first two words of the EA value, this is usually the
   EA type and EA size in pFEA, into the Type and Size */

void getEATypeSize( const HOLDFEA *pFEA, USHORT *Type, USHORT *Size )
	{
	USHORT *valPtr = ( USHORT * ) pFEA->aValue;
	*Type = valPtr[ 0 ];
	*Size = valPtr[ 1 ];
	}

/* Get the address of the EA value in pFEA, ie skip over the Type and Size */

VOID* getEADataVal (const HOLDFEA *pFEA)
	{
	/* Skip over the type and size */
	return pFEA->aValue + 2 * sizeof ( USHORT );
	}

/* QueryEAs (szFileName)
      find all EAs that file szFileName has
      return these in the linked list of HOLDFEAs
      if no EAs exist or the linked list cannot be created then
      return NULL
  This function is modelled after code from IBM's Toolkit */

HOLDFEA *QueryEAs( const CHAR *szFileName )
	{
	HOLDFEA *pHoldFEA;		/* The start of the linked list */

	CHAR *pAllocc = NULL;	/* Temp buffer used with DosEnumAttribute */
	CHAR *pBigAlloc = NULL;	/* Temp buffer to hold each EA as it is read in */
	USHORT cbBigAlloc = 0;

	ULONG ulEntryNum = 1;	/* Count of current EA to read (1-relative) */
	ULONG ulEnumCnt;		/* Number of EAs for Enum to return, always 1 */

	HOLDFEA *pLastIn = 0;	/* Points to last EA added, so new EA can link    */
	HOLDFEA *pNewFEA = NULL; /* Struct to build the new EA in                  */

	FEA2 *pFEA;				/* Used to read from Enum's return buffer */
	GEA2LIST *pGEAList;		/* Ptr used to set up buffer for DosQueryPathInfo() */
	EAOP2 eaopGet;			/* Used to call DosQueryPathInfo() */

	pAllocc = hmalloc( MAX_GEA );	/* Allocate room for a GEA list */
	pFEA = ( FEA2 * ) pAllocc;
	pHoldFEA = NULL;		/* Initialize the linked list */

	/* Loop through all the EA's adding them to the list */
	while( TRUE )
		{
		ulEnumCnt = 1;		/* No of EAs to retrieve */
		if( DosEnumAttribute( REF_ASCIIZ,	/* Read into EA name into */
							  szFileName,	/* pAlloc Buffer */
							  ulEntryNum, pAllocc, MAX_GEA, &ulEnumCnt,
#ifndef OS2_32				/* 16 bit API has an extra parameter */
							  ( LONG ) GET_INFO_LEVEL1, 0 ) )
#else
							  ( LONG ) GET_INFO_LEVEL1 ) )
#endif /* OS2_32 */
			break;	/* An error */

		/* Exit if all the EA's have been read */
		if( ulEnumCnt != 1 )
			break;

		/* Move on to next EA */
		ulEntryNum++;

		/* Try and allocate the HoldFEA structure */
		if( ( pNewFEA = hmalloc( sizeof( HOLDFEA ) ) ) == NULL )
			{
			hfree( pAllocc );
			Free_FEAList( pHoldFEA );
			return( NULL );
			}

		/* Fill in the HoldFEA structure */
		pNewFEA->cbName = pFEA->cbName;
		pNewFEA->cbValue= pFEA->cbValue;
		pNewFEA->fEA = pFEA->fEA;
		pNewFEA->next = '\0';

		/* Allocate the two arrays */
		if( ( pNewFEA->szName = hmalloc( pFEA->cbName + 1 ) ) == NULL || \
			( pNewFEA->aValue = hmalloc( pFEA->cbValue ) ) == NULL )
			{
			/* Out of memory, clean up and exit */
			if( pNewFEA->szName )
				hfree( pNewFEA->szName );
			if( pNewFEA->aValue )
				hfree( pNewFEA->aValue );

			hfree( pAllocc );
			hfree( pNewFEA );

			Free_FEAList( pHoldFEA );
			return( NULL );
			}

		/* Copy EA name across */
		strcpy( pNewFEA->szName, pFEA->szName );
		cbBigAlloc = sizeof( FEA2LIST ) + pNewFEA->cbName + 1 + pNewFEA->cbValue;
							/* +1 is for '\0' */
		if( ( pBigAlloc = hmalloc( cbBigAlloc ) ) == NULL )
			{
			hfree( pNewFEA->szName );
			hfree( pNewFEA->aValue );
			hfree( pAllocc );
			hfree( pNewFEA );
			Free_FEAList( pHoldFEA );
			return( NULL );
			}

		/* Set up GEAlist structure */
		pGEAList = ( GEA2LIST * ) pAllocc;
		pGEAList->cbList = sizeof( GEA2LIST ) + pNewFEA->cbName;	/* + 1 for NULL */
#ifdef OS2_32
		pGEAList->list[ 0 ].oNextEntryOffset = 0L;
#endif /* OS2_32 */
		pGEAList->list[ 0 ].cbName = pNewFEA->cbName;
		strcpy( pGEAList->list[ 0 ].szName, pNewFEA->szName );

		eaopGet.fpGEA2List = ( GEA2LIST FAR * ) pAllocc;
		eaopGet.fpFEA2List = ( FEA2LIST FAR * ) pBigAlloc;

		eaopGet.fpFEA2List->cbList = cbBigAlloc;

		/* Get the complete EA info and copy the EA value */
#ifndef OS2_32				/* The 16 bit API has an extra parameter */
		DosQPathInfo( szFileName, FIL_QUERYEASFROMLIST, ( PVOID ) &eaopGet, \
					  sizeof( EAOP2 ), 0 );
#else
		DosQueryPathInfo( szFileName, FIL_QUERYEASFROMLIST, ( PVOID ) &eaopGet, \
						  sizeof( EAOP2 ) );
#endif /* OS2_32 */
		memcpy( pNewFEA->aValue, \
				pBigAlloc + sizeof( FEA2LIST ) + pNewFEA->cbName, \
				pNewFEA->cbValue );

		/* Release the temp. Enum buffer */
		hfree( pBigAlloc );

		/* Add to the list */
		if( pHoldFEA == NULL )
			pHoldFEA = pNewFEA;
		else
			pLastIn->next = pNewFEA;
		pLastIn = pNewFEA;
		}

	/* Free up the GEA buffer for DosEnum() */
	hfree( pAllocc );

	return pHoldFEA;
	}


/* WriteEAs(szFileName, pHoldFEA)

   Write the EAs contained in the linked list pointed to by pHoldFEA
   to the file szFileName.

   Returns TRUE if the write was successful, FALSE otherwise */

BOOLEAN WriteEAs( const char *szFileName, HOLDFEA *pHoldFEA )
	{
	HOLDFEA *pHFEA = pHoldFEA;
	EAOP2 eaopWrite;
	CHAR *aPtr = NULL;
	USHORT usMemNeeded, usRet;

	eaopWrite.fpGEA2List = NULL;
	while( pHFEA )                                  /* Go through each HoldFEA */
		{
#ifndef OS2_32				/* The 16 bit API doesn't use the name */
		usMemNeeded = sizeof( FEA2LIST ) - \
					  sizeof( eaopWrite.fpFEA2List->list[0].szName ) + \
					  pHFEA->cbName + 1 + pHFEA->cbValue;
#else
		usMemNeeded = sizeof( FEA2LIST ) + pHFEA->cbName + 1 + pHFEA->cbValue;
#endif
		if( ( aPtr = hmalloc( usMemNeeded ) ) == NULL )
			return FALSE;

		/* Fill in eaop structure */
		eaopWrite.fpFEA2List = ( FEA2LIST FAR * ) aPtr;
		eaopWrite.fpFEA2List->cbList = usMemNeeded;
#ifdef OS2_32
		eaopWrite.fpFEA2List->list[ 0 ].oNextEntryOffset = pHFEA->oNextEntryOffset;
#endif /* OS2_32 */
		eaopWrite.fpFEA2List->list[ 0 ].fEA = pHFEA->fEA;
		eaopWrite.fpFEA2List->list[ 0 ].cbName = pHFEA->cbName;
		eaopWrite.fpFEA2List->list[ 0 ].cbValue = pHFEA->cbValue;
		strcpy( eaopWrite.fpFEA2List->list[ 0 ].szName, pHFEA->szName );
		memcpy( eaopWrite.fpFEA2List->list[ 0 ].szName + pHFEA->cbName + 1, \
				pHFEA->aValue, pHFEA->cbValue );

		/* Write out the EA */
		usRet = DosSetPathInfo( szFileName, FIL_QUERYEASIZE,
								( PVOID ) &eaopWrite, sizeof( EAOP2 ),
#ifndef OS2_32				/* The 16 bit API has an extra parameter */
								DSPI_WRTTHRU, 0 );
#else
								DSPI_WRTTHRU );
#endif
        /* Free up the FEALIST struct */
		hfree( aPtr );

		/* If the write failed, leave now */
		if( usRet )
			return FALSE;

		pHFEA = pHFEA->next;
		}

	return( TRUE );
	}

/* Copy EA's from a file to an archive in HPACK tagged form.
   Either the whole EA will be stored (flag, name len, data len, name, ...)
   or just the EA value (the rest can be reconstructed from the HPACK tag.

   The current EA tags are ICON, LONGNAME (both just store the value)
   and MISC (store the whole EA).

   MISC is a superset of the other EA types, and all EA's can in fact be
   stored as a MISC tag, however this is less efficient and portable */

LONG storeEAinfo( const BOOLEAN isFile, const char *pathName, const LONG eaSize1, const FD outFD )
	{
	HOLDFEA *pHoldFEA;			/* Global EA linked-list pointer */
	HOLDFEA *pFEA;
	WORD eaLength = ERROR;
	WORD tagLength;
	WORD i, dataLength = 0;
	TAG_TYPE tagType = STORE_WHOLE_EA;
	WORD theTag;
	USHORT eaType, Size;
	BOOLEAN isDir = !isFile;
	BYTE *dataPtr;

	/* Check whether there are any EA's to store */
	if( !( pHoldFEA = QueryEAs( pathName ) ) )
		return (0);

	/* Loop through each EA found, using a temporary pointer */
	pFEA = pHoldFEA;
	while( pFEA != NULL )
		{
		/* Determine EA type and write appropriate tag info */
		switch( getEAType ( pFEA->aValue ) )
			{
			case EAT_ASCII:
				/* Length-preceded ASCII.  Check whether the EA can be
				   recovered from the tag type */
				if( !strncmp( pFEA->szName, ".LONGNAME", pFEA->cbName ) && \
					( pFEA->fEA == 0 ) )
					{
					tagType = STORE_EA_VALUE;
					theTag = TAG_LONGNAME;
					}
				else
					tagType = STORE_WHOLE_EA;
				break;

			case EAT_ICON:
				/* Length-preceded icon.  Check whether the EA can be
				   recovered from the tag type */
				if( !strncmp( pFEA->szName, ".ICON", pFEA->cbName ) && \
					( pFEA->fEA == 0 ) )
					{
					tagType = STORE_EA_VALUE;
					theTag = TAG_OS2_ICON;
					}
				else
					tagType = STORE_WHOLE_EA;
				break;

			default:
				tagType = STORE_WHOLE_EA;
				break;
			}


		/* Store type of EA as a tag */
		switch( tagType )
			{
			case STORE_EA_VALUE:
				/* Just store the EA value, the other fields
				   are implicit in the tag */
				eaLength = pFEA->cbValue;
				eaLength -= 2 * sizeof( USHORT );	/* EA_VALUE.type & .eaSize are not stored */
				tagLength = eaLength + sizeof( WORD );	/* For CRC check */

				/* Write the tag as either a normal file or a directory tag */
				if( isDir )
					{
					dataLength += addDirData( theTag, TAGFORMAT_STORED, tagLength, LEN_NONE );
					checksumDirBegin( RESET_CHECKSUM );
					}
				else
					{
					dataLength += writeTag( theTag, TAGFORMAT_STORED, tagLength, LEN_NONE );
					checksumBegin( RESET_CHECKSUM );
					}

				dataLength += tagLength ;
				break;

			case STORE_WHOLE_EA:
				/* Store the whole EA */

#ifndef OS2_32
				eaLength = getEALength( pFEA ) - sizeof( CHAR );
						/* Don't store the null in the Name */
#else
				eaLength = getEALength( pFEA ) - sizeof( pFEA->oNextEntryOffset ) - sizeof( CHAR );
						/* Don't store the offset or the null in the Name */
#endif /* OS2_32 */
				tagLength = eaLength + sizeof( WORD );	/* For CRC check */

				/* Write the tag as either a normal file or a directory tag */
				if( isDir )
					{
					dataLength += addDirData( TAG_OS2_MISC_EA, TAGFORMAT_STORED, tagLength, LEN_NONE );
					checksumDirBegin( RESET_CHECKSUM );
					}
				else
					{
					dataLength += writeTag( TAG_OS2_MISC_EA, TAGFORMAT_STORED, tagLength, LEN_NONE );
					checksumBegin( RESET_CHECKSUM );
					}

				/* Write the flag, name, etc */
				getEATypeSize( pFEA, &eaType, &Size );
				if( isDir )
					{
					fputDirByte( pFEA->fEA );
/*					fputDirByte( pFEA->cbName ); */
					fputDirWord( pFEA->cbValue );
					for( i = 0; i < pFEA->cbName; i++ )
						fputDirByte( pFEA->szName[ i ] );
					fputDirByte( '\0' );
					fputDirWord( eaType );
					fputDirWord( Size );
					}
				else
					{
					fputByte( pFEA->fEA );
/*					fputByte( pFEA->cbName ); */
					fputWord( pFEA->cbValue );
					for( i = 0; i < pFEA->cbName; i++ )
						fputByte( pFEA->szName[ i ] );
					fputByte( '\0' );
					fputWord( eaType );
					fputWord( Size );
					}

				dataLength += tagLength;

				/* Adjust eaLength for the data already written */
				eaLength = pFEA->cbValue - 2 * sizeof( USHORT );
							/* Don't include type & size */
				break;
			}

		dataPtr = getEADataVal( pFEA );		/* Where data starts */

		/* Copy EA data to outFD */

/* Try and compress the buffer before we write it - not implemented yet */
		if( isDir )
			while( eaLength-- )
				fputDirByte( *dataPtr++ );
		else
			while( eaLength-- )
				fputByte( *dataPtr++ );

		/* Write CRC for data */
		if( isDir )
			{
			checksumDirEnd();
			fputDirWord( crc16 );
			}
		else
			{
			checksumEnd();
			fputWord( crc16 );
			}

		/* Go to next EA */
		pFEA = pFEA->next;
		}

	Free_FEAList( pFEA );

	return( dataLength );
	}

/* Set an EA from tagged EA info, see above for EA tag structure */

void setEAinfo( const char *filePath, const TAGINFO *tagInfo, const FD srcFD )
	{
	HOLDFEA *pFEA;
	char *dataPtr;
	WORD eaType, eaSize;
	int i, eaLen = ( int ) tagInfo->dataLength - sizeof( WORD );
			/* Subtract CRC check, Need this since length is const */

	/* Initialize HOLDFEA to receive EA's */
	if( ( pFEA = hmalloc( sizeof( HOLDFEA ) ) ) == NULL )
		{
		/* Skip over the tag before returning */
		skipSeek( tagInfo->dataLength );
		return;
		}
	pFEA->next = NULL;

	/* Read the header size and the EA flag, don't include checksum */
	checksumSetInput( tagInfo->dataLength - sizeof( WORD ), RESET_CHECKSUM );

/* eaName might have to be constructed locally, eg if all icons are called
   "ICON" there is little need to store this information, we can examine the
   tag type and set the name depending on this info.  Otherwise we read the
   name (0..255 chars, Pascal string) from srcFD into the EA buffer */

	switch( tagInfo->tagID )
		{
		case TAG_OS2_ICON:
			/* Construct EA */
			pFEA->cbName = sizeof( ICON_NAME ) - 1;
			if( ( pFEA->szName = hmalloc( pFEA->cbName ) ) == NULL )
				{
				/* Skip data before returning */
				skipSeek( tagInfo->dataLength );
				hfree( pFEA );
				return;
				}
			strcpy( pFEA->szName, ICON_NAME );
			pFEA->fEA = 0;
			pFEA->cbValue = eaLen + 2 * sizeof( USHORT );	/* Add type and length */
			eaType = EAT_ICON;
			eaSize = eaLen;
			eaLen = getEALength( pFEA );
			break;

		case TAG_LONGNAME:
			/* Construct EA */
			pFEA->cbName = sizeof( LONGNAME_NAME ) - 1;
			if( ( pFEA->szName = hmalloc( pFEA->cbName ) ) == NULL )
				{
				/* Skip data before returning */
				skipSeek( tagInfo->dataLength );
				hfree( pFEA );
				return;
				}
			strcpy( pFEA->szName, LONGNAME_NAME );
			pFEA->fEA = 0;
			pFEA->cbValue = eaLen + 2 * sizeof( USHORT );	/* Add type and length */
			eaType = EAT_ASCII;
			eaSize = eaLen;
			eaLen = getEALength( pFEA );
			break;

		case TAG_OS2_MISC_EA:
			/* Read flags, name etc from source file */
			pFEA->fEA = fgetByte();
/*			i = pFEA->cbName = fgetByte();
			if( ( pFEA->szName = hmalloc( pFEA->cbName + 1 ) ) == NULL )
*/
			/* Allocate enough room for the name (can't be bigger that the tag) */
			if( ( pFEA->szName = hmalloc( tagInfo->dataLength - sizeof( WORD ) - sizeof( BYTE ) ) ) == NULL )
				{
				/* Again skip data */
				skipSeek( tagInfo->dataLength - sizeof( BYTE ) - sizeof( BYTE ) );
				hfree( pFEA );
				return;
				}
			pFEA->cbValue = fgetWord();
			dataPtr = pFEA->szName;
/*			while( i-- )
				*dataPtr++ = fgetByte();
			pFEA->szName[ pFEA->cbName ] = '\0';
*/
			i = 0;
			while( ( dataPtr[ i++ ] = fgetByte() ) );
			pFEA->cbName = i - 1;

			/* Read the type and size */
			eaType = fgetWord();
			eaSize = fgetWord();	/* Probably the size, but not for multi valued */
			eaLen = getEALength( pFEA );
		}

	if( ( pFEA->aValue = hmalloc( pFEA->cbValue ) ) == NULL )
		{
		/* Skip data */
		skipSeek( tagInfo->dataLength - \
				  ( pFEA->cbValue - 2 * sizeof( USHORT ) ) + \
				  sizeof( WORD ) );
		hfree( pFEA->szName );
		hfree( pFEA );
		return;
		}

	setEATypeSize( pFEA, eaType, eaSize );

	/* Construct data portion of EA */
	i = pFEA->cbValue - 2 * sizeof( USHORT );	/* Don't include the type or size */
	dataPtr = getEADataVal( pFEA );

/* We may need to decompress this.  Currently compression isn't supported.
   When it is, the uncompressed size is given by tagInfo->uncoprLen */
	while( i-- )
		*dataPtr++ = fgetByte();

	/* Make sure data was non-corrupted */
	if( fgetWord() == crc16 )
		/* Ignore errors on Writing for the moment */
		WriteEAs( filePath, pFEA );

	Free_FEAList( pFEA );
	}

/* Copy all EA's from one file to another */

void copyExtraInfo( const char *srcFilePath, const char *destFilePath )
	{
	HOLDFEA *pFEA;

	pFEA = QueryEAs( srcFilePath );
	WriteEAs( destFilePath, pFEA );

	Free_FEAList (pFEA);
	}

/* Set any necessary extra information associated with a file */

static void setEA( const char *filePath, const WORD type, const char *name, \
				   const BYTE *value, const WORD length )
	{
	HOLDFEA *pFEA;
	BYTE *dataPtr;

	/* Initialize HOLDFEA to receive EA's */
	if( ( pFEA = hmalloc( sizeof( HOLDFEA ) ) ) == NULL )
		return;
	pFEA->next = NULL;
#ifdef OS2_32
	pFEA->oNextEntryOffset = 0L;
#endif /* OS2_32 */
	pFEA->fEA = 0;
	pFEA->cbName = strlen ( name );
	pFEA->cbValue = length + 2 * sizeof( WORD );	/* Include type and size */
	if( ( pFEA->szName = hmalloc( pFEA->cbName ) ) == NULL )
		{
		hfree( pFEA );
		return;
		}
	strcpy( pFEA->szName, name);
	if( ( pFEA->aValue = hmalloc( pFEA->cbValue ) ) == NULL )
		{
		hfree( pFEA->szName );
		hfree( pFEA );
		return;
		}

	setEATypeSize( pFEA, type, length );

	dataPtr = getEADataVal( pFEA );
	memcpy( dataPtr, value, length );

	WriteEAs( filePath, pFEA );
	Free_FEAList( pFEA );
	}

void setExtraInfo( const char *filePath )
	{
	/* Attach the type, icon, and file creator information to the archive */
	setEA( filePath, EAT_ASCII, ".TYPE", EA_HPACK_ARCH_TYPE, \
		   sizeof( EA_HPACK_ARCH_TYPE ) - 1 );
/*	putEA( theFD, ".ICON", EA_HPACK_ICON, \			*/
/*		   sizeof( EA_HPACK_ICON ), EAT_ICON );		*/
/*	putEA( theFD, ".ASSOCTBL", EA_HPACK_ASSOCTBL, \	*/
/*		   sizeof( EA_HPACK_ASSOCTBL ), EAT_???? );	*/
	}

/* Add a longname EA to a file or directory if it's been truncated on
   extraction */

void addLongName( const char *filePath, const char *longName )
	{
	FD theFD;

	if( ( theFD = hopen( filePath, O_RDONLY | S_DENYRDWR ) ) == ERROR )
		/* This should never happen since we've just created the file/dir */
		return;

	/* Save the non-truncated name as an EA */
	setEA( filePath, EAT_ASCII, ".LONGNAME", ( BYTE * ) longName, strlen( longName ) );

	hclose( theFD );
	}

#ifndef OS2_32

/* Determine the filesystem type.  The structure for the filesystem info
   seems to be:

	USHORT iType;					\* Item type *\
	USHORT cbName;					\* Length of item name, sans NULL *\
	UCHAR szName[];					\* ASCIIZ item name *\
	NULL
	USHORT cbFSDName;				\* Length of FSD name, sans NULL *\
	UCHAR szFSDName[ 1 ];			\* ASCIIZ FSD name *\
	NULL
	USHORT cbFSAData;				\* Length of FSD Attach data returned *\
	UCHAR rgFSAData[ 60 ];			\* FSD Attach data from FSD *\
	NULL */

typedef struct {
			   USHORT cbName;			/* Length of name, sans NULL */
			   UCHAR szName[ 1 ];		/* ASCIIZ name */
			   } FSNAME;

typedef struct {			/* Data structure for QFSAttach */
			   USHORT iType;			/* Item type */
			   FSNAME Name;				/* Item name */
			   UCHAR rgFSAData[ 59 ];	/* Place to store data */
			   } FSQINFO;

typedef FSQINFO FAR *PFSQINFO;

BOOLEAN queryFilesystemType( char *path )
	{
	USHORT nDrive;
	ULONG lMap;
	FSQINFO bData;
	BYTE bName[ 3 ];
	FSNAME *pFSName;
	USHORT cbData;

	/* Get drive */
	if( ( strlen( path ) > 0 ) && path[ 1 ] == ':' )
		bName[ 0 ] = path[ 0 ];
	else
		{
		DosQCurDisk( &nDrive, &lMap );
		bName[ 0 ] = ( char ) ( nDrive + '@' );
		}
	bName[ 1 ] = ':';
	bName[ 2 ] = 0;

	cbData = sizeof( bData );

	DosQFSAttach( ( PSZ ) bName, 0U, 1U, ( PBYTE ) &bData, &cbData, 0L );
	pFSName = &bData.Name;
	( char * ) pFSName += pFSName->cbName + sizeof( pFSName->cbName ) + 1;

	return( strcmp ( ( char * ) &( pFSName->szName[ 0 ] ), "FAT" ) != 0 );
	}

#else

/* Determine the file system type. This uses a strange function that
   populates a variables sised buffer that YOU have to declare, not knowing
   how big to actually make it!  Plus to access the ASCIIZ strings you need
   to play games with the offsets to find the real potition of the data you
   are after */

#define FSQBUFFERSIZE	64

BOOLEAN queryFilesystemType( char *path )
	{
	ULONG nDrive;
	ULONG lMap;
	char buffer[ FSQBUFFERSIZE ];
	FSQBUFFER2 *bData = ( FSQBUFFER2 * ) buffer;
	char bName[ 3 ];
	ULONG bDataLen;

	if( ( strlen( path ) > 0 ) && path[ 1 ] == ':' )
		bName[ 0 ] = path[ 0 ];
	else
		{
		DosQueryCurrentDisk( &nDrive, &lMap );
		bName[ 0 ] = ( char ) ( nDrive + 'A' - 1 );
		}

	bName[ 1 ] = ':';
	bName[ 2 ] = 0;

	bDataLen = FSQBUFFERSIZE;

	DosQueryFSAttach( bName, 0, FSAIL_QUERYNAME, bData, &bDataLen );
	return( strcmp( bData->szFSDName + bData->cbName, "FAT" ) != 0 );
	}

#if defined( __gcc ) && !defined( __EMX__ )

/* NLS support for string case conversion */

static unsigned char cLCtbl[ 256 ], cUCtbl[ 256 ];
static BOOLEAN bNLSinit = FALSE;

static void InitNLS( void )
	{
	int i, uppercaseChar;
	COUNTRYCODE countryCode;

	/* Flag the fact that the xlate tables are initialized */
	bNLSInit = TRUE;

	/* Set up default tables */
	for( i = 0; i < 256; i++ )
		cUCtbl[ i ] = cLCtbl[ i ] = ( unsigned char ) i;

	/* Set up map table */
	countryCode.country = countryCode.codepage = 0;
	DosMapCase( sizeof( cUCtbl ), &countryCode, ( PCHAR ) cUCtbl );

	for( i = 0; i < 256; i++ )
		if( ( uppercaseChar = cUCtbl[ i ] ) != i && \
			cLCtbl[ uppercaseChar ] == ( unsigned char ) uppercaseChar )
			cLCtbl[ uppercaseChar ] = ( unsigned char ) i;

	for( i = 'A'; i <= 'Z'; i++ )
		cLCtbl[ i ] = ( unsigned char ) ( i - 'A' + 'a' );
	}

void strlwr( char *string )
	{
	char *strPtr = string;

	if( !bNLSinit )
		InitNLS();

	while( *strPtr )
		*strPtr = cLowerCase[ ( int ) *strPtr ];
	}

int strnicmp( char *src, char *dest, int length )
	{
	char srcLower[ length ];	/* Note cool GNUC variable length array! */
	char destLower[ length ];	/* Note cool GNUC variable length array! */

	strlwr( strcpy( srcLower, src ) );
	strlwr( strcpy( destLower, dest ) );
	return( strncmp( srcLower, destLower, length ) );
	}

#endif /* __gcc && !__EMX__ */

#endif /* OS2_32 */
