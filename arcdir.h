/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*				Header File for the Directory Handling Routines				*
*							ARCDIR.H  Updated 07/12/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _ARCDIR_DEFINED

#define _ARCDIR_DEFINED

#if defined( __ATARI__ )
  #include "filehdr.h"
  #include "io\hpackio.h"
#elif defined( __MAC__ )
  #include "filehdr.h"
  #include "hpackio.h"
#else
  #include "filehdr.h"
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* The data structure for the list of file headers.  This contains the
   global information which is identical for all versions of HPACK, plus
   any mission-critical extra information needed by different OS's */

typedef struct FH {
				  /* Information used on all systems */
				  struct FH *next;		/* The next header in the list */
				  struct FH *prev;		/* The previous header in the list */
				  struct FH *nextFile;	/* Next file in this directory */
				  struct FH *prevFile;	/* Prev file in this directory */
				  struct FH *nextLink;	/* The next linked file */
				  struct FH *prevLink;	/* The previous linked file */
				  FILEHDR data;			/* The file data itself */
				  char *fileName;		/* The filename */
				  BYTE *extraInfo;		/* Any extra information needed */
				  long offset;			/* Offset of data from start of archive */
				  WORD hType;			/* The type of the file (within HPACK) */
				  WORD linkID;			/* The magic number of this link */
				  BOOLEAN tagged;		/* Whether this file has been selected */
#if defined( __UNIX__ )
				  /* Information used to handle hard links on Unix systems */
				  LONG fileLinkID;		/* The dev.no/inode used for links */
#elif defined( __MAC__ )
				  /* Information on file types/creators on Macintoshes */
				  LONG type;			/* The file's type */
				  LONG creator;			/* The file's creator */
#elif defined( __ARC__ )
				  /* Information on the file type on Archimedes' */
				  WORD type;			/* The file's type */
				  char oldExtension[ 2 ];	/* The (possibly deleted) file ext.*/
#elif defined( __IIGS__ )
				  /* Information on the file types on Apple IIgs's */
				  WORD type;			/* The file's type */
				  LONG auxType;			/* The file's auxiliary type */
#endif /* Various OS-specific extra information */
				  } FILEHDRLIST;

/* The data structures for the list of directories */

typedef struct DH {
				  WORD dirIndex;		/* The index of this directory */
				  WORD next;			/* The next header in the list */
/*				  struct DH *next;		// The next header in the list */
				  struct DH *nextDir;	/* Next dir in this directory */
				  struct DH *prevDir;	/* Prev dir in this directory */
				  struct DH *nextLink;	/* The next linked directory */
				  struct DH *prevLink;	/* The previous linked directory */
				  DIRHDR data;			/* The directory data */
				  char *dirName;		/* The directory name */
				  long offset;			/* Offset of data from start of dir
										   directory data area */
				  FILEHDRLIST *fileInDirListHead;	/* Start of the list of
										   files in this directory */
				  FILEHDRLIST *fileInDirListTail;	/* End of the list of
										   files in this directory */
				  struct DH *dirInDirListHead;		/* Start of the list of
										   dirs in this directory */
				  struct DH *dirInDirListTail;		/* End of the list of
										   dirs in this directory */
				  WORD hType;			/* The type of the directory (within HPACK) */
				  WORD linkID;			/* The magic number of this link */
				  BOOLEAN tagged;		/* Whether this dir.has been selected */
				  } DIRHDRLIST;

/* The data structure for the list of archive parts for a multi-part archive */

typedef struct PL {
				  struct PL *next;		/* The next header in the list */
				  long segEnd;			/* Extent of this segment */
				  } PARTSIZELIST;

/* The end marker for various lists */

#define END_MARKER		0xFFFF

/* The directory index number for the root directory */

#define ROOT_DIR		0

/* The magic number for a non-linked file */

#define NO_LINK			0

/* The types of path match which matchPath() can return */

enum { PATH_NOMATCH, PATH_PARTIALMATCH, PATH_FULLMATCH };

/* Whether we want readArcDir() to save the directory data to a temporary
   file (this is necessary since it will be overwritten during an ADD) */

#define SAVE_DIR_DATA	TRUE

/* The types of linking we want link/unlinkFileHeader() and
   link/unlinkDirHeader() to perform */

#define LINK_HDRLIST	1		/* Link into list of headers */
#define LINK_ARCDIR		2		/* Link into directory structure */

#define UNLINK_HDRLIST	1		/* Unlink from list of headers */
#define UNLINK_ARCDIR	2		/* Unlink from directory structure */
#define UNLINK_DELETE	4		/* Delete (free) data records */

/* The magic ID and temporary name extensions for HPACK archives */

extern char HPACK_ID[];

#define HPACK_ID_SIZE	4			/* 'HPAK' */

extern char TEMP_EXT[], DIRTEMP_EXT[], SECTEMP_EXT[];

/* Bit masks for the fileKludge in the archive trailer */

#define SPECIAL_MULTIPART	0x01	/* Multipart archive */
#define SPECIAL_MULTIEND	0x02	/* Last part of multipart archive */
#define SPECIAL_SECURED		0x04	/* Secured archive */
#define SPECIAL_ENCRYPTED	0x08	/* Encrypted archive */
#define SPECIAL_BLOCK		0x10	/* Block compressed archive */

/* Some global vars, defined in ARCDIR.C */

extern FILEHDRLIST *fileHdrCurrPtr, *fileHdrStartPtr;
extern WORD currEntry, currPart, lastPart;
extern long segmentEnd, endPosition;

/* Prototypes for functions in ARCDIR.C */

void initArcDir( void );
void endArcDir( void );
void resetArcDir( void );
#ifdef __MSDOS16__
  void resetArcDirMem( void );
#endif /* __MSDOS16__ */
void readArcDir( const BOOLEAN doSaveDirData );
void writeArcDir( void );

/* Miscellaneous directory-management functions */

void addFileHeader( FILEHDR *theHeader, WORD hType, BYTE extraInfo, WORD linkID );
void linkFileHeader( FILEHDRLIST *theHeader );
void unlinkFileHeader( FILEHDRLIST *theHeader, int action );
int writeFileHeader( FILEHDRLIST *theHeader );
BOOLEAN addFileName( const WORD dirIndex, const WORD hType, const char *fileName );
void deleteLastFileName( void );

void linkDirHeader( DIRHDRLIST *prevHeader, DIRHDRLIST *theHeader );
WORD createDir( const WORD parentDirNo, char *dirName, const LONG dirTime );
int addDirData( WORD tagID, const int store, LONG dataLength, LONG uncoprDataLength );
void movetoDirData( void );

void sortFiles( const WORD dirIndex );
inline WORD popDir( void );
int matchPath( const char *filePath, const int pathLen, WORD *dirIndex );

/* Directory-traversing functions */

FILEHDRLIST *findFileLinkChain( WORD linkID );
DIRHDRLIST *findDirLinkChain( WORD linkID );
inline FILEHDRLIST *getFirstFile( const FILEHDRLIST *theHeader );
inline FILEHDRLIST *getNextFile( void );
inline FILEHDRLIST *getFirstFileEntry( const WORD dirIndex );
inline FILEHDRLIST *getNextFileEntry( void );
inline FILEHDRLIST *getPrevFileEntry( void );
inline DIRHDRLIST *getFirstDirEntry( const WORD dirIndex );
inline DIRHDRLIST *getNextDirEntry( void );
inline DIRHDRLIST *getPrevDirEntry( void );
inline WORD getFirstDir( void );
inline WORD getNextDir( void );

/* Functions which return information about files/directories */

inline WORD getArchiveCwd( void );
inline void setArchiveCwd( const WORD dirIndex );
inline WORD getParent( const WORD dirIndex );
inline char *getDirName( const WORD dirIndex );
inline LONG getDirTime( const WORD dirIndex );
inline void setDirTime( const WORD dirIndex, const LONG time );
inline LONG getDirAuxDatalen( const WORD dirIndex );
inline BOOLEAN getDirTaggedStatus( const WORD dirIndex );
inline void setDirTaggedStatus( const WORD dirIndex );
void getArcdirState( FILEHDRLIST **hdrListEnd, int *dirListEnd );
void setArcdirState( FILEHDRLIST *hdrListEnd, const int dirListEnd );
long getCoprDataLength( void );
int computeHeaderSize( const FILEHDR *theHeader, const BYTE extraInfo );

/* File/directory selection functions */

void selectFile( FILEHDRLIST *fileHeader, BOOLEAN status );
void selectDir( DIRHDRLIST *dirHeader, BOOLEAN status );
void selectDirNo( const WORD dirNo, BOOLEAN status );
void selectDirNoContents( const WORD dirNo, BOOLEAN status );

/* Multidisk archive support functions */

void addPartSize( const long partSize );
int getPartNumber( const long offset );
long getPartSize( int partNumber );
void getPart( const int thePart );

/* Prototypes for supplementary functions in ARCDIR.C used only by some OS's */

#if defined( __UNIX__ )
  BOOLEAN checkForLink( const LONG linkID );
  void addLinkInfo( const LONG linkID );
#elif defined( __MAC__ )
  void addTypeInfo( LONG owner, LONG creator );
#endif /* Various OS-specific supplementary functions */

#endif
