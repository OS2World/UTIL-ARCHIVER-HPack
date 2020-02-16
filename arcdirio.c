/****************************************************************************
*																			*
*						  HPACK Multi-System Archiver						*
*						  ===========================						*
*																			*
*					     Archive Directory I/O Routines						*
*						  ARCDIRIO.C  Updated 20/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  				  and if you do so there will be....trubble.				*
* 			  And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* "You used HPACK to compress a 54 *megabyte* file??!?!  WOW!
	How many months did it take?"

						- HPACK developers private correspondence */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "choice.h"
#include "error.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
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

/* Prototypes for functions in ARCDIR.C */

void fixEverything( void );
void invertDirectory( void );

/* Prototypes for functions in ARCHIVE.C */

void blankLine( int length );
void idiotBox( const char *message );
BOOLEAN confirmAction( const char *message );

/* The following are declared in ARCDIR.C */

extern BOOLEAN doFixEverything;
extern LONG fileDataOffset, dirDataOffset;
extern DIRHDRLIST **dirHdrList;
extern int currDirHdrListIndex;
extern WORD lastEntry;

/* The size of the memory buffer for archive information */

#if ARCHIVE_TYPE == 1
  #define MEM_BUFSIZE	1000	/* More modest memory usage in test vers. */
#else
  #define MEM_BUFSIZE	8000
#endif /* ARCHIVE_TYPE == 1 */

/* The structure which holds the archive trailer info.  The checksum field
   overlays the security field length for secured archives */

typedef struct {
			   WORD noDirHdrs;			/* No.of directory headers */
			   WORD noFileHdrs;			/* No.of file headers */
			   LONG dirInfoSize;		/* Length of dir.info.block */
			   WORD checksum;			/* CRC16 of all preceding dir.data */
			   BYTE specialInfo;		/* The kludge byte */
			   BYTE archiveID[ 4 ];		/* 'HPAK' */
			   } ARCHIVE_TRAILER;

#define TRAILER_SIZE		( sizeof( WORD ) + sizeof( WORD ) + sizeof( LONG ) + \
							  sizeof( WORD ) + sizeof( BYTE ) + HPACK_ID_SIZE )

/* The size of the trailer for multipart and/or secured archives, and the
   magic bit set in the multipart trailer which indicates that the
   segmentation info and final trailer are on a seperate disk */

#define SHORT_TRAILER_SIZE	( sizeof( WORD ) + sizeof( WORD ) + sizeof( LONG ) + \
							  sizeof( WORD ) )

#define MULTIPART_SEPERATE_DISK		0x8000

/* The size of the checksummable information in the trailer */

#define TRAILER_CHK_SIZE	( sizeof( WORD ) + sizeof( WORD ) + sizeof( LONG ) )

/* The size of the multipart/secured archive final trailer */

#define MULTIPART_TRAILER_SIZE	( sizeof( WORD ) + sizeof( BYTE ) + HPACK_ID_SIZE )
#define SECURED_TRAILER_SIZE	MULTIPART_TRAILER_SIZE

/* The magic ID and temporary name extensions for HPACK archives */

char HPACK_ID[] = "HPAK";
#ifdef __ARC__
  char TEMP_EXT[] = "?1";
  char DIRTEMP_EXT[] = "?2";
  char SECTEMP_EXT[] = "?3";
#else
  char TEMP_EXT[] = "$$1";
  char DIRTEMP_EXT[] = "$$2";
  char SECTEMP_EXT[] = "$$3";
#endif /* __ARC__ */

/* A special value for the fileKludge to indicate the prototype versions of
   either the LZW, LZA, or MBWA versions of HPACK */

#if ARCHIVE_TYPE == 1
  #define PROTOTYPE	0x20				/* LZW development version */
#elif ARCHIVE_TYPE == 2
  #define PROTOTYPE	0x40				/* LZA development version */
#elif ARCHIVE_TYPE == 3
  #define PROTOTYPE	0x60				/* LZA' release version */
#elif ARCHIVE_TYPE == 4
  #define PROTOTYPE	0x80				/* LZA" release version */
#elif ARCHIVE_TYPE == 5
  #define PROTOTYPE	0xA0				/* MBWA prototype version 1 */
#elif ARCHIVE_TYPE == 6
  #define PROTOTYPE	0xC0				/* MBWA prototype version 2 */
#else
  #error "Need to define ARCHIVE_TYPE"
#endif /* Various ARCHIVE_TYPE-specific defines */

#define isPrototype(value)	( ( value ) & 0xE0 )/* Whether this archive was
													created by a proto-HPACK */

/* Symbolic defines to specify whether we want getArchiveInfo() to read in
   the ID bytes or not */

#define READ_ID		TRUE
#define NO_READ_ID	FALSE

/* Defines to handle the ^Z padding which some versions of Xmodem/Ymodem do */

#define CPM_EOF				0x1A
#define	YMODEM_BLOCKSIZE	1024	/* We can have up to this many ^Z's
									   appended */

/****************************************************************************
*																			*
*								Global Variables							*
*																			*
****************************************************************************/

PARTSIZELIST *partSizeStartPtr, *partSizeCurrPtr;
		/* Start and current position in the list of archive parts */

WORD currPart;						/* Current part no.of multipart archive */
WORD lastPart;						/* Last part no.of multipart archive */
long segmentEnd;					/* End of current archive segment */
long endPosition;					/* Virtual end of archive */

static ARCHIVE_TRAILER archiveInfo;	/* The archive trailer info */

/* The sizes of the encryption headers for the file and directory data
   (used only when writing directories) */

int cryptFileDataLength, cryptDirDataLength;

/****************************************************************************
*																			*
*						Read a Directory from a File						*
*																			*
****************************************************************************/

/* Whether the archive directory passes a sanity check when we read it.
   Note that each of the four routines which reads in file and directory
   headers and names contains a sanity check exit-condition in case we
   run out of input data.  In addition, various routines in ARCDIR.C also
   perform sanity checking.  It may be useful at some point to change the
   BOOLEAN to an int which is incremented each time a check fails to indicate
   the seriousness of the problem */

BOOLEAN arcdirCorrupted;

/* Read the file headers from a file */

static void readFileHeaders( WORD noFileHdrs )
	{
	FILEHDR theHeader;
	WORD hType, linkID;
	BYTE extraInfo;
	int extraInfoLen = 0, extraInfoIndex;

	while( noFileHdrs-- )
		{
		/* Assemble each header and add it to the directory tree, with
		   sanity check */
		hType = TYPE_NORMAL;		/* Reset header hType */
		extraInfo = linkID = 0;		/* Reset extraInfo and linkID */
		if( ( theHeader.archiveInfo = fgetWord() ) == ( WORD ) FEOF )
			{
			arcdirCorrupted = TRUE;
			return;
			}
		switch( theHeader.archiveInfo & ARCH_OTHER_LEN )
			{
			case ARCH_OTHER_ZERO_ZERO:
				/* dirIndex = ROOT_DIR, auxDataLen = 0 */
				theHeader.dirIndex = ROOT_DIR;
				theHeader.auxDataLen = 0L;
				break;

			case ARCH_OTHER_BYTE_BYTE:
				/* dirIndex = BYTE, auxDataLen = BYTE */
				theHeader.dirIndex = ( WORD ) fgetByte();
				theHeader.auxDataLen = ( LONG ) fgetByte();
				break;

			case ARCH_OTHER_BYTE_WORD:
				/* dirIndex = BYTE, auxDataLen = WORD */
				theHeader.dirIndex = ( WORD ) fgetByte();
				theHeader.auxDataLen = ( LONG ) fgetWord();
				break;

			case ARCH_OTHER_WORD_LONG:
				/* dirIndex = WORD, auxDataLen = LONG */
				theHeader.dirIndex = fgetWord();
				theHeader.auxDataLen = fgetLong();
			}

		theHeader.fileTime = fgetLong();
		if( theHeader.archiveInfo & ARCH_ORIG_LEN )
			theHeader.fileLen = fgetLong();
		else
			theHeader.fileLen = ( LONG ) fgetWord();
		if( theHeader.archiveInfo & ARCH_COPR_LEN )
			theHeader.dataLen = fgetLong();
		else
			theHeader.dataLen = ( LONG ) fgetWord();

		/* Handle type SPECIAL files by reading in the extra WORD giving
		   the file's hType */
		if( theHeader.archiveInfo & ARCH_SPECIAL )
			hType = fgetWord();

		/* Handle any extra information attached to the header */
		if( theHeader.archiveInfo & ARCH_EXTRAINFO )
			{
			/* Get extra info.byte and extract length information */
			extraInfo = fgetByte();
			extraInfoLen = getExtraInfoLen( extraInfo );
			extraInfoIndex = 1;		/* Data follows extraInfo byte */
			}

		/* Read link ID if there is one */
		if( extraInfo & EXTRA_LINKED )
			linkID = fgetWord();

		addFileHeader( &theHeader, hType, extraInfo, linkID );
		fileHdrCurrPtr->tagged = FALSE;

		/* Read in any extra information attached to the header if necessary */
		if( extraInfoLen )
			{
#if defined( __MSDOS16__ )
			/* Fix stupid compiler bug */
#elif defined( __ARC__ )
			if( ( theHeader.archiveInfo & ARCH_SYSTEM ) == OS_ARCHIMEDES )
				{
				/* Read in file type */
				extraInfoLen -= sizeof( WORD );
				if( extraInfoLen < 0 )
					{
					/* Length info has been corrupted */
					extraInfoLen = 0;
					fileHdrCurrPtr->type = 0;
					}
				else
					fileHdrCurrPtr->type = fgetWord();
				}
			else
				/* File type is unknown */
				fileHdrCurrPtr->type = 0;

			/* Currently we have no deleted file extension */
			fileHdrCurrPtr->oldExtension[ 0 ] = '\0';
#elif defined( __IIGS__ )
			if( ( theHeader.archiveInfo & ARCH_SYSTEM ) == OS_IIGS )
				{
				/* Read in file type and auxiliary type */
				extraInfoLen -= sizeof( WORD ) + sizeof( LONG );
				if( extraInfoLen < 0 )
					{
					/* Length info has been corrupted */
					extraInfoLen = 0;
					fileHdrCurrPtr->type = 0;
					fileHdrCurrPtr->auxType = 0L;
					}
				else
					{
					fileHdrCurrPtr->type = fgetWord();
					fileHdrCurrPtr->auxType = fgetLong();
					}
				}
			else
				{
				/* File type and auxiliary type are unknown */
				fileHdrCurrPtr->type = 0;
				fileHdrCurrPtr->auxType = 0L;
				}
#elif defined( __MAC__ )
			if( ( theHeader.archiveInfo & ARCH_SYSTEM ) == OS_MAC )
				{
				/* Read in file type and creator */
				extraInfoLen -= sizeof( LONG ) + sizeof( LONG );
				if( extraInfoLen < 0 )
					{
					/* Length info has been corrupted, default to type 'TEXT' */
					extraInfoLen = 0;
					fileHdrCurrPtr->type = fileHdrCurrPtr->creator = 'TEXT';
					}
				else
					{
					fileHdrCurrPtr->type = fgetLong();
					fileHdrCurrPtr->creator = fgetLong();
					}
				}
			else
				/* File type and creator are unknown.  Note that we set the
				   type to 0L rather than 'TEXT' since we may be able to
				   pick up type information later on */
				fileHdrCurrPtr->type = fileHdrCurrPtr->creator = 0L;
#elif defined( __UNIX__ )
			if( ( theHeader.archiveInfo & ARCH_SYSTEM ) == OS_UNIX )
				{
				/* Read in link ID */
				extraInfoLen -= sizeof( WORD );
				if( extraInfoLen < 0 )
					{
					/* Length info has been corrupted */
					extraInfoLen = 0;
					fileHdrCurrPtr->fileLinkID = 0;
					}
				else
					fileHdrCurrPtr->fileLinkID = fgetWord();
				}
			else
				/* No linkID given */
				fileHdrCurrPtr->fileLinkID = 0;
			fileHdrCurrPtr->linkID = 0L;	/* No dev/inode for this file */
#endif /* Various OS-dependant extraInfo reads */

			/* Read in any remaining information */
			while( extraInfoLen-- )
				fileHdrCurrPtr->extraInfo[ extraInfoIndex++ ] = fgetByte();
			extraInfoLen = 0;	/* Fix fencepost error */
			}
		else
			{
#if defined( __AMIGA__ ) || defined( __ATARI__ ) || \
	defined( __MSDOS16__ ) || defined( __MSDOS32__ )
			;	/* Nothing */
#elif defined( __ARC__ )
			/* No file type or old extension */
			fileHdrCurrPtr->type = 0;
			fileHdrCurrPtr->oldExtension[ 0 ] = '\0';
#elif defined( __IIGS__ )
			/* No file type and auxiliary type */
			fileHdrCurrPtr->type = 0;
			fileHdrCurrPtr->auxType = 0L;
#elif defined( __MAC__ )
			/* No type/creator information */
			fileHdrCurrPtr->type = fileHdrCurrPtr->creator = 0L;
#elif defined( __UNIX__ )
			/* No link information */
			fileHdrCurrPtr->linkID = 0L;
#endif /* Various OS-dependant extraInfo initializations */
			}
		}
	}

/* Read the filenames from a file */

static void readFileNames( void )
	{
	FILEHDRLIST *prevPtr = NULL;

	/* Step along the chain of headers reading in the corresponding filenames */
	for( fileHdrCurrPtr = fileHdrStartPtr; fileHdrCurrPtr != NULL;
		 prevPtr = fileHdrCurrPtr, fileHdrCurrPtr = fileHdrCurrPtr->next )
		addFileName( fileHdrCurrPtr->data.dirIndex, fileHdrCurrPtr->hType, NULL );

	fileHdrCurrPtr = prevPtr;	/* Reset currPos to last header */
	}

/* Read the directory headers from a file */

static void readDirHeaders( WORD noDirHdrs )
	{
	WORD theEntry, parentDirNo;
	DIRHDRLIST *dirInfoPtr;
	DIRHDR *theHeader;
	int dirInfoFlags;

	if( noDirHdrs )
		{
		while( noDirHdrs-- )
			{
			/* Add each header to the directory tree */
			if( ( dirInfoPtr = ( DIRHDRLIST * ) hmalloc( sizeof( DIRHDRLIST ) ) ) == NULL )
				error( OUT_OF_MEMORY );
			currDirHdrListIndex++;
			if( currDirHdrListIndex >= MEM_BUFSIZE )
				error( OUT_OF_MEMORY );
			theHeader = &dirInfoPtr->data;

			/* Set up housekeeping info for the header */
			dirHdrList[ currEntry ]->next = currEntry + 1;
			dirHdrList[ ++currEntry ] = dirInfoPtr;
			dirInfoPtr->offset = dirDataOffset;
			dirInfoPtr->dirIndex = currEntry;
			dirInfoPtr->hType = DIRTYPE_NORMAL;
			dirInfoPtr->linkID = NO_LINK;

			/* Read the header information off disk, with sanity check */
			if( ( dirInfoFlags = fgetByte() ) == FEOF )
				{
				arcdirCorrupted = TRUE;
				return;
				}
			theHeader->dirInfo = dirInfoFlags;
			switch( theHeader->dirInfo & DIR_OTHER_LEN )
				{
				case DIR_OTHER_ZERO_ZERO:
					/* parentIndex = ROOT_DIR, dataLen = 0 */
					theHeader->parentIndex = ROOT_DIR;
					theHeader->dataLen = 0L;
					break;

				case DIR_OTHER_BYTE_BYTE:
					/* parentIndex = BYTE, dataLen = BYTE */
					theHeader->parentIndex = ( WORD ) fgetByte();
					theHeader->dataLen = ( LONG ) fgetByte();
					break;

				case DIR_OTHER_BYTE_WORD:
					/* parentIndex = BYTE, dataLen = WORD */
					theHeader->parentIndex = ( WORD ) fgetByte();
					theHeader->dataLen = ( LONG ) fgetWord();
					break;

				case DIR_OTHER_WORD_LONG:
					/* parentIndex = WORD, dataLen = LONG */
					theHeader->parentIndex = fgetWord();
					theHeader->dataLen = fgetLong();
				}
			theHeader->dirTime = fgetLong();

			/* Read in type SPECIAL word and linkID if necessary */
			if( theHeader->dirInfo & DIR_SPECIAL )
				dirInfoPtr->hType = fgetWord();
			if( theHeader->dirInfo & DIR_LINKED )
				dirInfoPtr->linkID = fgetWord();

			dirInfoPtr->fileInDirListHead = dirInfoPtr->fileInDirListTail = NULL;
			dirInfoPtr->dirInDirListHead = dirInfoPtr->dirInDirListTail = NULL;
			dirInfoPtr->tagged = FALSE;
			dirDataOffset += dirInfoPtr->data.dataLen;
			}
		dirInfoPtr->next = END_MARKER;
		}
	lastEntry = currEntry;

	/* We've read in all the directories, now create the list of directories
	   inside directories (this has to be done after all the directories are
	   read in since they will not necessarily be in the correct order as they
	   are read) */
	for( theEntry = dirHdrList[ ROOT_DIR ]->next; theEntry != END_MARKER;
		 theEntry = dirHdrList[ theEntry ]->next )
		{
		/* Make sure we don't have any incorrect directory ID's */
		dirInfoPtr = dirHdrList[ theEntry ];
		if( ( parentDirNo = dirInfoPtr->data.parentIndex ) > lastEntry )
			{
			/* Bad entry, move to ROOT_DIR */
			dirInfoPtr->data.parentIndex = ROOT_DIR;
			arcdirCorrupted = TRUE;
			}

		/* Link the header into the directory structure */
		linkDirHeader( dirHdrList[ parentDirNo ]->dirInDirListTail,
					   dirInfoPtr );
		}
	}

/* Read the directory names from a file */

static void readDirNames( void )
	{
	WORD *wordPtr = ( WORD * ) mrglBuffer;
	WORD theEntry;
	int dirNameLength, ch;

	/* Read in directory names */
	for( theEntry = dirHdrList[ ROOT_DIR ]->next; theEntry != END_MARKER;
		 theEntry = dirHdrList[ theEntry ]->next )
		{
		/* Read in directory name with sanity check */
		dirNameLength = 0;
		if( dirHdrList[ theEntry ]->data.dirInfo & DIR_UNICODE )
			{
			/* Read Unicode directory name */
#pragma warn -pia
			while( ch = fgetWord() )
#pragma warn +pia
				{
				/* Perform a sanity check for end of data or excessively
				   long string */
				if( ch == FEOF || dirNameLength > ( DIRBUFSIZE / sizeof( WORD ) ) )
					{
					arcdirCorrupted = TRUE;
					return;
					}

				wordPtr[ dirNameLength++ ] = ch;
				}

			wordPtr[ dirNameLength++ ] = 0x0000;
			}
		else
			{
			/* Read ASCII directory name */
#pragma warn -pia
			while( ch = fgetByte() )
#pragma warn +pia
				{
				/* Perform a sanity check for end of data or excessively
				   long string */
				if( ch == FEOF || dirNameLength > DIRBUFSIZE )
					{
					arcdirCorrupted = TRUE;
					return;
					}

#if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __MAC__ ) || \
	defined( __OS2__ ) || defined( __UNIX__ )
				/* Check if we want to smash case */
				if( sysSpecFlags & SYSPEC_FORCELOWER )
					mrglBuffer[ dirNameLength++ ] = tolower( ch );
				else
#endif /* __AMIGA__ || __ARC__ ||__MAC__ || __OS2__ || __UNIX__ */
				mrglBuffer[ dirNameLength++ ] = ch;
				}

			mrglBuffer[ dirNameLength++ ] = '\0';
			}

		if( ( dirHdrList[ theEntry ]->dirName =
					( char * ) hmalloc( dirNameLength ) ) == NULL )
			error( OUT_OF_MEMORY );
		strcpy( dirHdrList[ theEntry ]->dirName, ( char * ) mrglBuffer );
		}
	}

/* Read in the section sizes for a multipart archive */

static void readPartSizes( void )
	{
	int count = lastPart;

	/* Read in each sections size and set the current part to the last part
	   read */
	while( count-- )
		addPartSize( fgetLong() );
	currPart = lastPart;
	}

/* Read the archive trailer information from a file */

static void getArchiveInfo( const BOOLEAN readID )
	{
	archiveInfo.noDirHdrs = fgetWord();
	archiveInfo.noFileHdrs = fgetWord();
	archiveInfo.dirInfoSize = fgetLong();
	archiveInfo.checksum = fgetWord();

	if( readID )
		{
		archiveInfo.specialInfo = fgetByte();

		/* We must use fgetByte() to get the archive ID since fgetLong() will
		   do endianness conversion */
		archiveInfo.archiveID[ 0 ] = fgetByte();
		archiveInfo.archiveID[ 1 ] = fgetByte();
		archiveInfo.archiveID[ 2 ] = fgetByte();
		archiveInfo.archiveID[ 3 ] = fgetByte();
		}
	}

/* Save the directory data to a temporary file */

static void saveDirData( void )
	{
	WORD multipartFlagSave = multipartFlags;

	/* Turn off multipart writes when saving the directory data */
	multipartFlags &= ~MULTIPART_WRITE;

	/* Open a temporary file (with the same reason given in FRONTEND.C for
	   using the archive name with the DIRTEMP_EXT suffix) and move the
	   directory data to it */
	strcpy( dirFileName, archiveFileName );
	strcpy( dirFileName + strlen( dirFileName ) - 3, DIRTEMP_EXT );
	if( ( dirFileFD = hcreat( dirFileName, CREAT_ATTR ) ) == IO_ERROR )
		error( CANNOT_OPEN_TEMPFILE );
	setOutputFD( dirFileFD );
	hlseek( archiveFD, fileDataOffset + HPACK_ID_SIZE, SEEK_SET );
	moveData( dirDataOffset );
	flushBuffer();
	setOutputFD( archiveFD );

	multipartFlags = multipartFlagSave;
	}

/*  Save the security data and final trailer to a temporary file */

static void saveSecData( void )
	{
	WORD multipartFlagSave = multipartFlags;

	/* Turn off multipart writes when saving the security data */
	multipartFlags &= ~MULTIPART_WRITE;

	/* Open a temporary file and move the data as above */
	strcpy( secFileName, archiveFileName );
	strcpy( secFileName + strlen( secFileName ) - 3, SECTEMP_EXT );
	if( ( secFileFD = hcreat( secFileName, CREAT_ATTR ) ) == IO_ERROR )
		error( CANNOT_OPEN_TEMPFILE );
	setOutputFD( secFileFD );
	memcpy( _outBuffer, _inBuffer + SHORT_TRAILER_SIZE,
			secInfoLen + SECURED_TRAILER_SIZE );
	writeBuffer( secInfoLen + SECURED_TRAILER_SIZE );
	setOutputFD( archiveFD );

	multipartFlags = multipartFlagSave;
	}

/* Read in the trailer information at the very end of an archive */

static int paddingSize;		/* No.bytes ^Z padding found by readArcTrailer() */

void readArcTrailer( void )
	{
	WORD multipartFlagSave, cryptFlagSave;
	BOOLEAN notArchive;
	int inByteCountSave, bytesRead;
#ifdef GUI
	char string[ 10 ];
#endif /* GUI */

	/* If we are reading in the trailer of a multipart archive, move any data
	   in the input buffer which may be overwritten to a safe area and turn
	   off multipart reads */
	if( flags & MULTIPART_ARCH )
		{
		memcpy( mrglBuffer, _inBuffer, HPACK_ID_SIZE + YMODEM_BLOCKSIZE );
		multipartFlagSave = multipartFlags;
		multipartFlags &= ~MULTIPART_READ;
		cryptFlagSave = cryptFlags;
		cryptFlags = 0;
		inByteCountSave = _inByteCount;
		}

	/* Make sure this is an HPACK archive */
	paddingSize = 0;
	hlseek( archiveFD, -( long ) TRAILER_SIZE, SEEK_END );
	resetFastIn();
	getArchiveInfo( READ_ID );
	if( memcmp( archiveInfo.archiveID, HPACK_ID, HPACK_ID_SIZE ) )
		{
		notArchive = TRUE;
		if( archiveInfo.archiveID[ 3 ] == CPM_EOF )
			{
			/* This may be a valid archive with CPM EOF's added to the end.
			   Try and find a valid HPACK ID before the ^Z's */
			hlseek( archiveFD, -( long ) ( HPACK_ID_SIZE + YMODEM_BLOCKSIZE ), SEEK_END );
			if( htell( archiveFD ) < 0L )
				/* We've gone past the start of the file; just seek to the
				   start */
				hlseek( archiveFD, 0L, SEEK_SET );
			bytesRead = hread( archiveFD, _inBuffer, HPACK_ID_SIZE + YMODEM_BLOCKSIZE );
			for( paddingSize = bytesRead - 1; paddingSize >= HPACK_ID_SIZE &&
							  _inBuffer[ paddingSize ] == CPM_EOF; paddingSize-- );
			paddingSize++;	/* Fix fencepost error */
			if( !memcmp( _inBuffer + paddingSize - HPACK_ID_SIZE, HPACK_ID, HPACK_ID_SIZE ) )
				{
				/* It is an HPACK archive, try to truncate the ^Z padding.  If
				   we can't, remember the offset of the start of any useful data */
				notArchive = FALSE;
				paddingSize = bytesRead - paddingSize;
				hlseek( archiveFD, -( long ) paddingSize, SEEK_END );
				if( htruncate( archiveFD ) != IO_ERROR )
					{
#ifdef GUI
					itoa( paddingSize, string, 10 );
					alert( ALERT_TRUNCATED_PADDING, string );
#else
					hprintf( WARN_TRUNCATED_u_BYTES_EOF_PADDING, paddingSize );
#endif /* GUI */
					paddingSize = 0;
					}

				/* Re-read the trailer info */
				hlseek( archiveFD, -( ( long ) ( TRAILER_SIZE + paddingSize ) ), SEEK_END );
				resetFastIn();
				getArchiveInfo( READ_ID );
				}
			}

		if( notArchive )
			error( NOT_HPACK_ARCHIVE );
		}

	/* If this is part of a multipart archive, set the correct part number */
	if( archiveInfo.specialInfo & SPECIAL_MULTIPART )
		currPart = archiveInfo.checksum;
	if( archiveInfo.specialInfo & SPECIAL_MULTIEND )
		currPart = lastPart;

	/* Restore the input buffer and turn multipart reads on again if necessary */
	if( flags & MULTIPART_ARCH )
		{
		memcpy( _inBuffer, mrglBuffer, HPACK_ID_SIZE + YMODEM_BLOCKSIZE );
		multipartFlags = multipartFlagSave;
		cryptFlags = cryptFlagSave;
		_inByteCount = inByteCountSave;
		}
	}

/* Read the archive directory into memory */

void readArcDir( const BOOLEAN doSaveDirData )
	{
	long securedDataLen;
	int cryptInfoLen;

	/* Read in the trailer information */
	readArcTrailer();
	if( archiveInfo.specialInfo & SPECIAL_MULTIPART )
		/* This is part of a multipart archive, prompt for the last part */
		while( !( archiveInfo.specialInfo & SPECIAL_MULTIEND ) )
			{
			hclose( archiveFD );
			archiveFD = IO_ERROR;		/* Mark it as invalid */
			multipartWait( WAIT_PARTNO | WAIT_LASTPART, archiveInfo.checksum );
			if( ( archiveFD = hopen( archiveFileName, O_RDONLY | S_DENYWR | A_RANDSEQ ) ) == ERROR )
				error( CANNOT_OPEN_ARCHFILE, archiveFileName );
			setInputFD( archiveFD );
			readArcTrailer();
			}

	/* Perform bootstrap read for multipart archives */
	if( archiveInfo.specialInfo & SPECIAL_MULTIEND )
		{
		/* Set special status flag */
		flags |= MULTIPART_ARCH;

		/* Move the information from the archiveInfo fields into the segment
		   information which they overlay */
		lastPart = archiveInfo.noFileHdrs & ~MULTIPART_SEPERATE_DISK;

		/* Read in the segmentation information */
		hlseek( archiveFD, -( ( long ) ( ( lastPart * sizeof( LONG ) ) + TRAILER_SIZE ) ), SEEK_CUR );
		resetFastIn();
		checksumSetInput( ( lastPart * sizeof( LONG ) ) + TRAILER_CHK_SIZE, RESET_CHECKSUM );
		readPartSizes();
		getArchiveInfo( NO_READ_ID );	/* For checksumming purposes */
		if( crc16 != archiveInfo.checksum )
			/* Directory data was corrupted, confirm to continue */
			idiotBox( MESG_ARCHIVE_DIRECTORY_CORRUPTED );

		/* We have segmentation information in memory, turn on virtual read
		   mode, establish current and end positions in archive */
		setCurrPosition( archiveInfo.dirInfoSize );
		endPosition = archiveInfo.dirInfoSize;
		multipartFlags |= MULTIPART_READ;

		/* Get length of security info in correct field in case it's needed
		   later on */
		archiveInfo.checksum = archiveInfo.noDirHdrs;

		/* If the multipart information is on a seperate disk, get the disk
		   with the archive itself */
		if( archiveInfo.noFileHdrs & MULTIPART_SEPERATE_DISK )
			getPart( lastPart );
		}

	/* Set the block-mode flag if necessary */
	if( archiveInfo.specialInfo & SPECIAL_BLOCK )
		{
		/* Make sure we don't try and change an archive using unified compression */
		if( choice == ADD || choice == DELETE || choice == FRESHEN ||
			choice == REPLACE || choice == UPDATE || ( flags & MOVE_FILES ) )
		error( CANNOT_CHANGE_UNIFIED_ARCH );

		flags |= BLOCK_MODE;
		}
	else
		flags &= ~BLOCK_MODE;

	/* Handle archive security if secured archive */
	if( archiveInfo.specialInfo & SPECIAL_SECURED )
		{
		/* Read in archive trailer and security/encryption info.  The length
		   of this field is given by archiveTrailer.checksum, which overlays
		   the length field.  The length of the secured data is the current
		   position plus the trailer size minus the size of the ID at the
		   start and the ID and kludge bytes which aren't part of the trailer
		   any more but are at the end of the archive */
		secInfoLen = archiveInfo.checksum;
		vlseek( -( ( long ) ( sizeof( WORD ) + secInfoLen +
						  ( TRAILER_SIZE ) + paddingSize ) ), SEEK_END );
		securedDataLen = vtell() + SHORT_TRAILER_SIZE - HPACK_ID_SIZE;
		resetFastIn();
		getArchiveInfo( NO_READ_ID );

		/* If it's a multipart archive the security info check can take
		   a while, so we ask the luser whether they want to do it or not */
		if( !( flags & MULTIPART_ARCH ) ||
			confirmAction( MESG_VERIFY_SECURITY_INFO ) )
			{
			if( choice == EXTRACT || choice == TEST || choice == DISPLAY )
				{
				/* Check the entire archive */
#ifndef GUI
				hputs( MESG_VERIFYING_ARCHIVE_AUTHENTICITY );
#endif /* !GUI */
				if( !checkSignature( 0L, securedDataLen ) )
					idiotBox( MESG_AUTH_CHECK_FAILED );
#ifndef GUI
				hputchar( '\n' );
#endif /* !GUI */
				}
			else
				{
				/* Make sure the user wants to overwrite the security info */
				if( choice != VIEW )
					idiotBox( MESG_SECURITY_INFO_WILL_BE_DESTROYED );

				/* Save the information for later recovery if necessary */
				if( choice == ADD )
					saveSecData();
				}
			}

		/* Now treat security info as padding */
		paddingSize += secInfoLen + sizeof( WORD );
		}
	else
		if( flags & MULTIPART_ARCH )
			{
			/* We still need to read in the archive trailer */
			vlseek( -( long ) SHORT_TRAILER_SIZE, SEEK_END );
			resetFastIn();
			getArchiveInfo( NO_READ_ID );

			/* Adjust the padding size by the difference between the full
			   trailer and the shortened trailer */
			paddingSize -= ( TRAILER_SIZE - SHORT_TRAILER_SIZE );
			}

	/* Handle encrypted archive */
	if( archiveInfo.specialInfo & SPECIAL_ENCRYPTED )
		{
		/* Make sure we don't try to update an encrypted archive */
		if( choice == ADD || choice == DELETE ||
			choice == FRESHEN || choice == UPDATE || ( flags & MOVE_FILES ) )
			error( CANNOT_CHANGE_ENCRYPTED_ARCH );

		/* If the luser didn't specify decryption, try it anyway */
		if( !( flags & CRYPT ) )
			flags |= CRYPT;

		/* Assume conventional-key encryption for now.  This may be changed
		   later when we read in the encryption header */
		if( !( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) ) )
			cryptFlags = CRYPT_CKE_ALL;
		}
	else
		if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
			if( choice == DISPLAY || choice == TEST || choice == EXTRACT || choice == VIEW )
				/* The archive isn't encrypted, try processing individual
				   encrypted files.  Assume conventional-key encryption for now */
				cryptFlags = CRYPT_CKE;
			else
				/* Make sure we don't try to block-encrypt an unencrypted archive */
				error( CANNOT_CHANGE_UNENCRYPTED_ARCH );

	/* Check this is the correct version of archive/HPACK */
	if( !isPrototype( archiveInfo.specialInfo ) )
		idiotBox( "Please upgrade to a non-prototype HPACK release" );
	else
		if( ( archiveInfo.specialInfo & 0xE0 ) != PROTOTYPE )
			/* This archive was created by a different prototype version,
			   confirm to continue */
			idiotBox( "Peter has been messing with the filekludge again" );

	/* Read in archive directory */
	if( ( ( long ) archiveInfo.dirInfoSize < 0L ) ||
		( vlseek( -( ( long ) ( archiveInfo.dirInfoSize +
				  TRAILER_SIZE + paddingSize ) ), SEEK_END ) < HPACK_ID_SIZE ) )
		/* dirInfoSize has impossible value or tried to seek past archive
		   start, trailer information corrupted.  Note that we can actually
		   seek right up to the HPACK_ID at the start if the archive contains
		   only directories */
		error( ARCHIVE_DIRECTORY_CORRUPTED );
#ifndef GUI
	if( flags & MULTIPART_ARCH )
		hprintfs( MESG_PROCESSING_ARCHIVE_DIRECTORY );
#endif /* !GUI */
	arcdirCorrupted = FALSE;
	resetFastIn();
	if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
		{
		preemptCryptChecksum( archiveInfo.dirInfoSize + TRAILER_CHK_SIZE );
		if( !cryptSetInput( archiveInfo.dirInfoSize, &cryptInfoLen ) )
			error( CANNOT_PROCESS_CRYPT_ARCH );
		archiveInfo.dirInfoSize -= cryptInfoLen;
		}
	else
		checksumSetInput( archiveInfo.dirInfoSize + TRAILER_CHK_SIZE, RESET_CHECKSUM );
	readDirHeaders( archiveInfo.noDirHdrs );
	readFileHeaders( archiveInfo.noFileHdrs );
	readDirNames();
	readFileNames();
	getArchiveInfo( NO_READ_ID );	/* For checksumming purposes */
#ifndef GUI
	if( flags & MULTIPART_ARCH )
		blankLine( 60 );
#endif /* !GUI */
	if( crc16 != archiveInfo.checksum || arcdirCorrupted )
		{
		/* Directory data was corrupted, confirm to continue */
		if( cryptFlags & CRYPT_CKE_ALL )
			idiotBox( MESG_ARCHIVE_DIRECTORY_WRONG_PASSWORD );
		else
			idiotBox( MESG_ARCHIVE_DIRECTORY_CORRUPTED );
		}

	/* Move directory data to a temporary file if necessary since it will be
	   overwritten later */
	if( doSaveDirData && dirDataOffset )
		saveDirData();

#ifdef __ARC__
	/* Check whether we want to invert the directory structure */
	if( sysSpecFlags & SYSPEC_INVERTDIR )
		invertDirectory();
#endif /* __ARC__ */

	/* Only go back to the start of the archive if we have to */
	if( !( flags & MULTIPART_READ ) ||
		( choice == EXTRACT || choice == TEST || choice == DISPLAY ) )
		vlseek( 0L, SEEK_SET );
	}

/****************************************************************************
*																			*
*							Write a Directory to a File						*
*																			*
****************************************************************************/

/* Write the file headers to a file.  This function is split into two seperate
   routines, one of which steps through the list of headers and one which
   writes the headers themselves; the latter is also called to write the
   error recovery information */

int writeFileHeader( FILEHDRLIST *theHeader )
	{
	FILEHDR *fileHeader = &theHeader->data;
	WORD archInfo = fileHeader->archiveInfo;
	int bytesWritten = sizeof( WORD ) + sizeof( LONG );	/* Constant fields */
	int extraInfoIndex, extraInfoLen;

	/* Set the field length bits depending on the field's contents */
	archInfo |= ( fileHeader->fileLen > 0xFFFF ) ? ARCH_ORIG_LEN : 0;
	archInfo |= ( fileHeader->dataLen > 0xFFFF ) ? ARCH_COPR_LEN : 0;
	archInfo |= ( fileHeader->dirIndex > 0xFF ) ? ARCH_OTHER_LEN :
				( fileHeader->auxDataLen > 0xFF ) ? ARCH_OTHER_HI :
				( fileHeader->dirIndex || fileHeader->auxDataLen ) ?
				  ARCH_OTHER_LO : 0;

	/* Write the header itself */
	fputWord( archInfo );
	switch( archInfo & ARCH_OTHER_LEN )
		{
		case ARCH_OTHER_BYTE_BYTE:
			/* dirIndex = BYTE, auxDataLen = BYTE */
			fputByte( ( BYTE ) fileHeader->dirIndex );
			fputByte( ( BYTE ) fileHeader->auxDataLen );
			bytesWritten += sizeof( BYTE ) + sizeof( BYTE );
			break;

		case ARCH_OTHER_BYTE_WORD:
			/* dirIndex = BYTE, auxDataLen = WORD */
			fputByte( ( BYTE ) fileHeader->dirIndex );
			fputWord( ( WORD ) fileHeader->auxDataLen );
			bytesWritten += sizeof( BYTE ) + sizeof( WORD );
			break;

		case ARCH_OTHER_WORD_LONG:
			/* dirIndex = WORD, auxDataLen = LONG */
			fputWord( fileHeader->dirIndex );
			fputLong( fileHeader->auxDataLen );
			bytesWritten += sizeof( WORD ) + sizeof( LONG );
		}
	fputLong( fileHeader->fileTime );
	if( archInfo & ARCH_ORIG_LEN )
		{
		fputLong( fileHeader->fileLen );
		bytesWritten += sizeof( LONG );
		}
	else
		{
		fputWord( ( WORD ) fileHeader->fileLen );
		bytesWritten += sizeof( WORD );
		}
	if( archInfo  & ARCH_COPR_LEN )
		{
		fputLong( fileHeader->dataLen );
		bytesWritten += sizeof( LONG );
		}
	else
		{
		fputWord( ( WORD ) fileHeader->dataLen );
		bytesWritten += sizeof( WORD );
		}

	/* Handle type SPECIAL files by writing an additional WORD containing
	   the file's hType */
	if( archInfo & ARCH_SPECIAL )
		{
		fputWord( theHeader->hType );
		bytesWritten += sizeof( WORD );
		}

	/* Handle any extra info attached to the header */
	if( archInfo & ARCH_EXTRAINFO )
		{
		/* Write out extraInfo byte */
		extraInfoIndex = 1;
		extraInfoLen = getExtraInfoLen( *theHeader->extraInfo );
		fputByte( *theHeader->extraInfo );
		bytesWritten += sizeof( BYTE ) + extraInfoLen;

#if defined( __MSDOS16__ )
		/* Fix compiler bug */
#elif defined( __ARC__ )
		/* Write file type */
		if( extraInfoLen >= sizeof( WORD ) )
			{
			fputWord( theHeader->type );
			extraInfoLen -= sizeof( WORD );
			extraInfoIndex += sizeof( WORD );
			}
#elif defined( __IIGS__ )
		/* Write file type and auxiliary type */
		if( extraInfoLen >= sizeof( WORD ) + sizeof( LONG ) )
			{
			fputWord( theHeader->type );
			fputLong( theHeader->auxType );
			extraInfoLen -= sizeof( WORD ) + sizeof( LONG );
			extraInfoIndex += sizeof( WORD ) + sizeof( LONG );
			}
#elif defined( __MAC__ )
		/* Write file type and creator */
		if( extraInfoLen >= sizeof( LONG ) + sizeof( LONG ) )
			{
			fputLong( theHeader->type );
			fputLong( theHeader->creator );
			extraInfoLen -= sizeof( LONG ) + sizeof( LONG );
			extraInfoIndex += sizeof( LONG ) + sizeof( LONG );
			}
#elif defined( __UNIX__ )
		/* Write link ID */
		if( extraInfoLen >= sizeof( WORD ) )
			{
			fputWord( theHeader->fileLinkID );
			extraInfoLen -= sizeof( WORD );
			extraInfoIndex += sizeof( WORD );
			}
#endif /* Various OS-dependant extraInfo reads */

		/* Write out any remaining info */
		while( extraInfoLen-- )
			fputByte( theHeader->extraInfo[ extraInfoIndex++ ] );
		}

	return( bytesWritten );
	}

static LONG writeFileHeaders( void )
	{
	LONG bytesWritten = 0L;

	archiveInfo.noFileHdrs = 0;

	/* Write a dummy header for the encryption information if necessary */
	if( cryptFileDataLength )
		{
		fputWord( ARCH_SPECIAL );
		fputLong( getStrongRandomLong() );
		fputWord( getStrongRandomWord() );
		fputWord( ( WORD ) cryptFileDataLength );
		fputWord( TYPE_DUMMY );
		bytesWritten += sizeof( WORD ) + sizeof( LONG ) + sizeof( WORD ) +
						sizeof( WORD ) + sizeof( WORD );
		archiveInfo.noFileHdrs++;
		}

	/* Write the file headers themselves */
	for( fileHdrCurrPtr = fileHdrStartPtr; fileHdrCurrPtr != NULL;
		 fileHdrCurrPtr = fileHdrCurrPtr->next )
		{
		bytesWritten += writeFileHeader( fileHdrCurrPtr );
		archiveInfo.noFileHdrs++;
		}

	return( bytesWritten );
	}

/* Write the filenames to a file */

static LONG writeFileNames( void )
	{
	LONG bytesWritten = 0L;
	char *strPtr;

	/* Write a dummy name for the encryption header if necessary */
	if( cryptFileDataLength )
		{
		fputByte( '\0' );
		bytesWritten++;
		}

	/* Write file names */
	for( fileHdrCurrPtr = fileHdrStartPtr; fileHdrCurrPtr != NULL;
		 fileHdrCurrPtr = fileHdrCurrPtr->next )
		{
		strPtr = fileHdrCurrPtr->fileName;
		do
			{
			fputByte( *strPtr );
			bytesWritten++;
			}
		while( *strPtr++ );
		}

	return( bytesWritten );
	}

/* Write the directory headers to a file */

static LONG writeDirHeaders( void )
	{
	WORD theEntry, hType, linkID;
	LONG bytesWritten = 0L;
	DIRHDR *dirHeader;
	BYTE dirInfo;

	archiveInfo.noDirHdrs = 0;

	/* Write a dummy header for the encryption information if necessary */
	if( cryptDirDataLength )
		{
		fputByte( DIR_OTHER_BYTE_BYTE | DIR_SPECIAL );
		fputByte( ROOT_DIR );
		fputByte( ( BYTE ) cryptDirDataLength );
		fputLong( getStrongRandomLong() );
		fputWord( DIRTYPE_DUMMY );
		bytesWritten += sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE ) +
						sizeof( LONG ) + sizeof( WORD );
		archiveInfo.noDirHdrs++;
		}

	/* Write the directory headers themselves */
	for( theEntry = dirHdrList[ ROOT_DIR ]->next; theEntry != END_MARKER;
		 theEntry = dirHdrList[ theEntry ]->next )
		{
		/* Pull out relevant info from directory header */
		dirHeader = &dirHdrList[ theEntry ]->data;
		hType = dirHdrList[ theEntry ]->hType;
		linkID = dirHdrList[ theEntry ]->linkID;

		/* Set the field length bits depending on the field's contents */
		dirInfo = dirHeader->dirInfo;
		dirInfo |= ( dirHeader->parentIndex > 0xFF ) ? DIR_OTHER_LEN :
				   ( dirHeader->dataLen > 0xFF ) ? DIR_OTHER_HI :
				   ( dirHeader->parentIndex || dirHeader->dataLen ) ?
					 DIR_OTHER_LO : 0;

		/* Write the header itself */
		fputByte( dirInfo );
		switch( dirInfo & DIR_OTHER_LEN )
			{
			case DIR_OTHER_BYTE_BYTE:
				/* parentIndex = BYTE, dataLen = BYTE */
				fputByte( ( BYTE ) dirHeader->parentIndex );
				fputByte( ( BYTE ) dirHeader->dataLen );
				bytesWritten += sizeof( BYTE ) + sizeof( BYTE );
				break;

			case DIR_OTHER_BYTE_WORD:
				/* parentIndex = BYTE, dataLen = WORD */
				fputByte( ( BYTE ) dirHeader->parentIndex );
				fputWord( ( WORD ) dirHeader->dataLen );
				bytesWritten += sizeof( BYTE ) + sizeof( WORD );
				break;

			case DIR_OTHER_WORD_LONG:
				/* dirIndex = WORD, auxDataLen = LONG */
				fputWord( dirHeader->parentIndex );
				fputLong( dirHeader->dataLen );
				bytesWritten += sizeof( WORD ) + sizeof( LONG );
			}
		fputLong( dirHeader->dirTime );
		bytesWritten += sizeof( BYTE ) + sizeof( LONG );

		/* Handle type SPECIAL directories by writing an additional WORD
		   containing the directories's hType */
		if( dirInfo & DIR_SPECIAL )
			{
			fputWord( hType );
			bytesWritten += sizeof( WORD );
			}

		/* Handle linked directories by writing the linkID */
		if( dirInfo & DIR_LINKED )
			{
			fputWord( linkID );
			bytesWritten += sizeof( WORD );
			}

		archiveInfo.noDirHdrs++;
		}

	return( bytesWritten );
	}

/* Write the directory names to a file */

static LONG writeDirNames( void )
	{
	LONG bytesWritten = 0L;
	WORD theEntry;
	WORD *wordPtr;
	char *strPtr;

	/* Write a dummy name for the encryption header if necessary */
	if( cryptDirDataLength )
		{
		fputByte( '\0' );
		bytesWritten++;
		}

	/* Write directory names */
	for( theEntry = dirHdrList[ ROOT_DIR ]->next; theEntry != END_MARKER;
		 theEntry = dirHdrList[ theEntry ]->next )
		if( dirHdrList[ theEntry ]->data.dirInfo & DIR_UNICODE )
			{
			/* Write Unicode string */
			wordPtr = ( WORD * ) dirHdrList[ theEntry ]->dirName;
			do
				{
				fputWord( *wordPtr );
				bytesWritten += sizeof( WORD );
				}
			while( *wordPtr++ );
			}
		else
			{
			/* Write ASCII/EBCDIC string */
			strPtr = dirHdrList[ theEntry ]->dirName;
			do
				{
				fputByte( *strPtr );
				bytesWritten++;
				}
			while( *strPtr++ );
		}

	return( bytesWritten );
	}

/* Write the size of each part in a multi-part archive */

static void writePartSizes( void )
	{
	if( flags & MULTIPART_ARCH )
		{
		/* Flag the fact that this is the last part of a multipart archive */
		archiveInfo.specialInfo |= SPECIAL_MULTIEND;

		/* Remember where the segmentation information starts */
		segmentEnd = getCurrPosition();

		/* Write out the size of each section */
		checksumBegin( RESET_CHECKSUM );
		for( partSizeCurrPtr = partSizeStartPtr; partSizeCurrPtr != NULL;
			 partSizeCurrPtr = partSizeCurrPtr->next )
			fputLong( partSizeCurrPtr->segEnd );
		}
	}

/* Write the directory size information */

static void writeArchiveInfo( void )
	{
	fputWord( archiveInfo.noDirHdrs );
	fputWord( archiveInfo.noFileHdrs );
	fputLong( archiveInfo.dirInfoSize );

	/* Calculate checksum for any remaining archive directory data before
	   this point and write it */
	checksumEnd();
	fputWord( crc16 );
	}

/* Write the final archive trailer */

static WORD savedCRC;
static BOOLEAN saveCRC;

static void writeArchiveTrailer( void )
	{
	/* Write the information for the multipart read bootstrap if necessary */
	if( flags & MULTIPART_ARCH )
		{
		/* In case we call writeArchiveTrailer() a second time with altered
		   trailer info we calculate the data checksum to this point and save
		   it now */
		if( saveCRC )
			{
			checksumEnd();
			savedCRC = crc16;
			}
		else
			crc16 = savedCRC;

		/* Write length of authentication/encryption info, total number of
		   segments, and end of last segment */
		checksumBegin( NO_RESET_CHECKSUM );
		fputWord( 0 );
		fputWord( ( WORD ) lastPart );
		fputLong( segmentEnd );
		checksumEnd();

		/* Calculate checksum for any remaining segment data before
		   this point and write it */
		fputWord( crc16 );
		}

	/* Write the kludge byte */
	if( flags & BLOCK_MODE )
		archiveInfo.specialInfo |= SPECIAL_BLOCK;
	if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
		archiveInfo.specialInfo |= SPECIAL_ENCRYPTED;
	if( cryptFlags & CRYPT_SIGN_ALL )
		archiveInfo.specialInfo |= SPECIAL_SECURED;
	fputByte( archiveInfo.specialInfo );

	/* We must use fputByte() to put the archive ID since fputLong() will do
	   endianness conversion */
	fputByte( HPACK_ID[ 0 ] );
	fputByte( HPACK_ID[ 1 ] );
	fputByte( HPACK_ID[ 2 ] );
	fputByte( HPACK_ID[ 3 ] );
	}

/* Write the directory data to the archive */

static void writeDirData( void )
	{
	LONG position = 0L, offset, delta, dataLen;
	WORD theEntry, multipartFlagSave = multipartFlags;

	/* Turn off multipart reads when saving the directory data */
	multipartFlags &= ~MULTIPART_READ;

	/* Write out any data still in the buffer */
	flushDirBuffer();

	/* Move the directory data from the temporary file to the archive file.
	   We use a loop with fputByte() instead of a moveData() call to take care
	   of buffering and handle encryption for us */
	if( dirDataOffset )
		{
		if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
			cryptFileDataLength = cryptBegin( ( cryptFlags & CRYPT_SEC ) ?
											  SECONDARY_KEY : MAIN_KEY );

		/* Step through the list of headers moving the data corresponding to
		   the header from the dirFile to the archive file.  Doing things
		   this way is necessary since fixEverything() may have reordered the
		   directory headers so that their order no longer corresponds to the
		   data in the dirFile */
		setInputFD( dirFileFD );
		hlseek( dirFileFD, 0L, SEEK_SET );
		forceRead();
		for( theEntry = dirHdrList[ ROOT_DIR ]->next; theEntry != END_MARKER;
			 theEntry = dirHdrList[ theEntry ]->next )
			/* Only do the move if there's something to move */
#pragma warn -pia
			if( dataLen = dirHdrList[ theEntry ]->data.dataLen )
#pragma warn +pia
				{
				if( ( offset = dirHdrList[ theEntry ]->offset ) < position )
					{
					/* Data is behind us, move backwards in dirFile */
					delta = position - offset;
					if( _inByteCount >= delta )
						/* Data is still in buffer; move backwards to it */
						_inByteCount -= ( int ) delta;
					else
						{
						/* Data is outside buffer, go to position */
						hlseek( dirFileFD, offset, SEEK_SET );
						forceRead();
						}
					}
				else
					/* Data is ahead of us; skipSeek to it if necessary */
					if( offset - position )
						skipSeek( offset - position );

				/* Move the data across and update position */
				position = offset + dataLen;
				while( dataLen-- )
					/* Not very elegant but it nicely takes care of buffering */
					fputByte( ( BYTE ) fgetByte() );
				}

		if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
			cryptEnd();
		}
	else
		/* Reset encryption header status */
		cryptDirDataLength = 0;

	multipartFlags = multipartFlagSave;

	/* Zap the dirFile if it exists */
	if( dirFileFD != IO_ERROR )
		{
		hclose( dirFileFD );
		hunlink( dirFileName );
		dirFileFD = IO_ERROR;		/* Mark it as invalid */
		}
	}

/* Write the archive directory to a file */

void writeArcDir( void )
	{
	/* Clean up the directory structure if it's safe to do so */
	if( doFixEverything )
		fixEverything();

	/* Write the directory data (if any) to the archive */
	writeDirData();

	/* Build up archive trailer information and write it.  Note that the
	   cryptBegin() call must be before the checksumBegin() call since the
	   encryption information is covered by its own checksum */
	archiveInfo.specialInfo = 0;
	archiveInfo.dirInfoSize = 0L;
	if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
		archiveInfo.dirInfoSize = ( LONG ) cryptBegin( MAIN_KEY );
	checksumBegin( RESET_CHECKSUM );
	archiveInfo.dirInfoSize += writeDirHeaders();
	archiveInfo.dirInfoSize += writeFileHeaders();
	archiveInfo.dirInfoSize += writeDirNames();
	archiveInfo.dirInfoSize += writeFileNames();
	if( cryptFlags & ( CRYPT_CKE_ALL | CRYPT_PKE_ALL ) )
		cryptEnd();
	archiveInfo.specialInfo |= PROTOTYPE;
	writeArchiveInfo();		/* Write directory size information */
	if( multipartFlags & MULTIPART_WRITE )
		{
		/* Force the evaluation of lastPart and try and write the
		   segmentation information and final trailer as one block */
		setWriteType( SAFE_WRITE );
		flushBuffer();			/* Force lastPart evaluation */
		saveCRC = TRUE;
		writePartSizes();
		writeArchiveTrailer();	/* Put info in buffer */
		setWriteType( ATOMIC_WRITE );
		flushBuffer();			/* Write info as one block */

		if( !atomicWriteOK )
			{
			/* We need to put the information on a seperate disk.  Set the
			   high bit of lastPart to flag the fact */
			_outByteCount -= SHORT_TRAILER_SIZE + sizeof( BYTE ) + HPACK_ID_SIZE;
			lastPart |= MULTIPART_SEPERATE_DISK;
			saveCRC = FALSE;
			writeArchiveTrailer();	/* Re-put trailer info in buffer */
			flushBuffer();			/* Write data to single block on disk */
			saveCRC = TRUE;
			}

		setWriteType( STD_WRITE );
		}
	else
		{
		/* Sign the archive if required */
		if( cryptFlags & CRYPT_SIGN_ALL )
			{
			flushBuffer();		/* Get all info to sign out of buffers */
			fputWord( ( WORD ) createSignature( 0L, getCurrPosition(), signerID ) );
			}

		/* Write final trailer information */
		writeArchiveTrailer();
		flushBuffer();
		}

	/* If we've ended up deleting all the files in the archive, delete
	   the archive itself */
	if( ( archiveInfo.noDirHdrs | archiveInfo.noFileHdrs ) == 0 )
		{
		errorFD = ERROR;
		hunlink( errorFileName );
		}

	/* If it's a multipart archive and there is only one part, redo it
	   as a normal archive */
	if( ( flags & MULTIPART_ARCH ) && !lastPart )
		{
		/* Move back over output padding and trailer and truncate archive */
		hlseek( archiveFD, -( long ) ( MULTIPART_TRAILER_SIZE + SHORT_TRAILER_SIZE +
									   sizeof( BYTE ) + HPACK_ID_SIZE ), SEEK_END );
		htruncate( archiveFD );

		/* Reset its status to a normal single-part archive and rewrite
		   the trailer */
		resetFastOut();
		flags &= ~MULTIPART_ARCH;
		multipartFlags = 0;
		archiveInfo.specialInfo &= ~SPECIAL_MULTIEND;
		writeArchiveTrailer();
		flushBuffer();
		}

	/* Zap the secFile if it exists */
	if( secFileFD != IO_ERROR )
		{
		hclose( secFileFD );
		hunlink( secFileName );
		secFileFD = IO_ERROR;		/* Mark it as invalid */
		}
	}
