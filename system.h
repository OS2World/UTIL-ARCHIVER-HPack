/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						System-Specific Information Header					*
*							SYSTEM.H  Updated 22/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _SYSTEM_DEFINED

#define _SYSTEM_DEFINED

/****************************************************************************
*																			*
*							Definitions for Amiga							*
*																			*
****************************************************************************/

#ifdef __AMIGA__

#define ARCHIVE_OS		OS_AMIGA

/* Maximum Amiga path length (105 chars + null terminator) and filename length
   (30 chars + null delimiter) */

#define MAX_PATH		106
#define MAX_FILENAME	31

/* The file attribute type */

typedef BYTE			ATTR;	/* Attribute data type */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		0x0D	/* Read/write/delete permission */
#define MKDIR_ATTR		0		/* hmkdir()'s mode parameter ignored */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   Since the Amiga doesn't distinguish between normal and special files these
   are treated identically */

#define FILES			1			/* Match normal files only */
#define ALLFILES		1			/* Match special files as well */
#define FILES_DIRS 		2			/* Match normal files and directories */
#define ALLFILES_DIRS	2			/* Match all files and directories */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	','

/* File codes for what we want to match.  Since AmigaDos searches by
   directory rather than filespec we just use null strings */

#define MATCH_ALL		""			/* Just match anything */
#define SLASH_MATCH_ALL	"/"			/* As above with directory delimiter */
#define MATCH_ARCHIVE	""			/* All archive files */

/* The type of case conversion to perform on file and directory names */

#define caseConvert(x)	x			/* No conversion */

/* The data block used by findFirst()/findNext()/findEnd() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR fAttr;			/* File attributes */

			   /* Amiga-specific information */
			   void *lock;			/* Dir/file lock (actually a FileLock *) */
			   void *infoBlock;		/* Dir/file info (actually a FileInfoBlock *) */
			   BOOLEAN isDir;		/* Whether file is directory or not */
			   BOOLEAN hasComment;	/* Whether file/dir has attached comment */
			   char matchAttr;		/* Attributes to match on */
			   } FILEINFO;

/* Determine whether a found file is a directory or not (a BOOLEAN set
   up by findFirst()/findNext()).  We can't use a name of isDirectory for
   the member since DICE gets confused with the macro name */

#define isDirectory(fileInfo)		fileInfo.isDir

/* The type of case conversion to perform on file and directory names */

#define caseConvert(x)	x			/* No conversion */
#define caseMatch		toupper		/* Force uppercase */
#define caseStrcmp		stricmp		/* Non-case-sensitive match */
#define caseStrncmp		strnicmp	/* Non-case-sensitive match */

#ifdef _STRICT_ANSI
extern int stricmp( const char *, const char * );
extern int strnicmp( const char *, const char *, size_t );
#endif /* _STRICT_ANSI */

/* Whether spaces are legal in filenames */

#define SPACE_OK		TRUE

/* Dummy routines for nonportable functions */

#define setDirecTime(x,y)	/* Seems impossible to do under AmigaDos */

/* Prototypes for extra routines needed by some compilers */

#if defined( LATTICE ) && !defined( __SASC )
  void memmove( char *dest, char *src, int length );
#endif /* LATTICE && !__SASC */
char *__strlwr( char *str );

#define strlwr  __strlwr

#endif /* __AMIGA__ */

/****************************************************************************
*																			*
*							Definitions for Apple IIgs						*
*																			*
****************************************************************************/

#ifdef __IIGS__

#define ARCHIVE_OS		OS_IIGS

/* Maximum path length and filename length */

#define MAX_PATH		255
#define MAX_FILENAME	15

/* File attributes */

typedef WORD			ATTR;		/* Attribute data type */

#define RDONLY			0x01		/* Read only attribute */
#define HIDDEN			0x02		/* Hidden file */
#define DIRECT			0x000F		/* Directories */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		1 			/* Read/write permission */
#define MKDIR_ATTR		0			/* mkdirs()'s mode parameter ignored */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   On the IIgs invisible files are treated seperately */

#define FILES			1			/* Match normal files only */
#define ALLFILES		2			/* Match special files as well */
#define FILES_DIRS 		3			/* Match normal files and directories */
#define ALLFILES_DIRS	4			/* Match all files and directories */

/* The data block used by findFirst()/findNext()/findEnd() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR fAttr;			/* File attributes */

			   /* Apple IIgs-specific information */
			   WORD fType;			/* File type */
			   LONG fAuxType;		/* Auxiliary file type */
/*			   WORD fStorage;		// Storage type */
			   LONG fCrtDate;		/* First part of creation time and date */
			   LONG fCrtTime;		/* Second part of creation time and date */
/*			   LONG fModDate;		// First part of modification time and date */
/*			   LONG fModTime;		// Second part of modification time and date */
/*			   LONG fOptionList;	// Option list pointer for FST */
/*			   LONG fBlocks;		// Number of blocks used by file */
			   LONG fResSize;		/* Size of file's resource fork */
/*			   LONG fResBlocks;		// Number of blocks used by resource fork */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)		( fileInfo.fType == DIRECT )

/* Dummy routines for nonportable functions */

#define setFileTime(x,y)	hputs( "Need to implement setFileTime()" )
#define setDirecTime(x,y)	hputs( "Need to implement setDirTime()" )
#define getCountry()		hputs( "Need to implement getCountry()" )
#define getScreenSize()		hputs( "Need to implement getScreenSize()" )
#define findFirst(x,y,z)	hputs( "Need to implement findFirst()" )
#define findNext(x)			hputs( "Need to implement findNext()" )
#define findEnd(x)			hputs( "Need to implement findEnd()" )

#endif /* __IIGS__ */

/****************************************************************************
*																			*
*							Definitions for Archimedes						*
*																			*
****************************************************************************/

#ifdef __ARC__

#define ARCHIVE_OS		OS_ARCHIMEDES

/* Maximum path length and filename length.  Note that although we can
   accept as input a filename of up to 31 chars under some IFS's
   supported by the NetFS, we have to truncate to the ADFS limit of 10
   chars on extraction.  This is handled by code in FILESYS.C.  Once
   Risc-OS 3, the System 7 of the Archimedes world, appears, we can use a
   SWI to find out how long filenames can be */

#define MAX_PATH		256
#define MAX_FILENAME	32

/* The size of the directory buffer used by findFirst/findNext() */

#define DIRINFO_SIZE	100		/* Size of one directory entry (generous value) */
#define DIRBUF_ENTRIES	20		/* No.entries in one buffer */

/* The file attribute type and various useful attributes */

typedef BYTE			ATTR;		/* Attribute data type */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   Since the Arc doesn't distinguish between normal and special files these
   are treated identically */

#define FILES			1			/* Match normal files only */
#define ALLFILES		1			/* Match special files as well */
#define FILES_DIRS 		2			/* Match normal files and directories */
#define ALLFILES_DIRS	2			/* Match all files and directories */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		0			/* Read/write permission */
#define MKDIR_ATTR		0			/* Ignored by hmkdir() */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'.'			/* I kid you not, it uses dots */
#define SLASH_STR		"."

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	','

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"*"			/* All files and directories */
#define SLASH_MATCH_ALL	".*"		/* As above with directory delimiter */
#define MATCH_ARCHIVE	"\x01"		/* All archive files */

/* The data block used by findFirst()/findNext() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR fAttr;			/* File attributes */

			   /* Arc-specific information */
			   int type;			/* File type */
			   LONG loadAddr;		/* File load address */
			   LONG execAddr;		/* Execute address */
			   char matchAttr;		/* File attributes to match on */
			   BOOLEAN isDirectory;	/* Whether file is a directory or not */
			   BOOLEAN wantArchive;	/* Whether we want an archive filetype */
			   long dirPos;			/* Current position in directory.  Force
									   dirBuffer to be longword-aligned */
			   BYTE dirBuffer[ DIRINFO_SIZE * DIRBUF_ENTRIES ];
			   int currEntry;		/* Current buffer entry */
			   int totalEntries;	/* Total entries in buffer */
			   void *nextRecordPtr;	/* Pointer to next record in buffer */
			   char dirPath[ MAX_PATH ];		/* Dir.path to search on */
			   int dirPathLen;					/* Length of dir.path */
			   char fileSpec[ MAX_FILENAME ];	/* Filespec to search for */
			   } FILEINFO;

/* Determine whether a found file is a directory or not (a BOOLEAN set up by
   findFirst()/findNext()) */

#define isDirectory(fileInfo)	( fileInfo.isDirectory )

/* The type of case conversion to perform on file and directory names */

#define caseConvert(x)	x			/* No conversion */
#define caseMatch		toupper		/* Force uppercase */
#define caseStrcmp		stricmp		/* Non-case-sensitive match */
#define caseStrncmp		strnicmp	/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		FALSE

/* Dummy routines for nonportable/unused functions */

#define setDirecTime(x,y)	/* Impossible to do */
#define initExtraInfo()		/* Arc files have no memory-held extra info */
#define copyExtraInfo(x,y)
#define endExtraInfo()

/* Dummy routine not needed in Arc version */

#define findEnd(x)

/* Extra routines not part of the Archimedes ANSI libraries */

int strnicmp( const char *src, const char *dest, int length );
int stricmp( const char *src, const char *dest );
void strlwr( char *string );

#endif /* __ARC__ */

/****************************************************************************
*																			*
*							Definitions for Atari							*
*																			*
****************************************************************************/

#ifdef __ATARI__

#define ARCHIVE_OS		OS_ATARI

/* Maximum Gem-DOS path length (63 bytes + null terminator) and filename
   length (8.3 + null delimiter) */

#define MAX_PATH		64
#define MAX_FILENAME	13

/* The file attribute type and various useful attributes */

typedef BYTE			ATTR;	/* Attribute data type */

#define HIDDEN			0x02	/* Hidden file */
#define SYSTEM			0x04	/* System file */
#define DIRECT			0x10	/* Directory bit */

/* Attributes as used by HPACK.  Gem-DOS distinguishes between normal files
   (DEFAULT and RDONLY) and special files (HIDDEN and SYSTEM) so there
   are two sets of attributes for matching, one for normal files and one
   for special files */

#define FILES			0		/* No attributes to match on (will still
								   match DEFAULT and RDONLY under Gem-DOS) */
#define ALLFILES		( HIDDEN | SYSTEM )
								/* All useful file attribute types. Again
								   will also match DEFAULT and RDONLY */
#define FILES_DIRS		DIRECT	/* All normal files + directories */
#define ALLFILES_DIRS	( ALLFILES | DIRECT )
								/* All useful file attribute types + dirs */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		( S_IREAD | S_IWRITE )
#define MKDIR_ATTR		0

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	';'

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"*.*"	/* All files and directories */
#define SLASH_MATCH_ALL	"/*.*"	/* As above with directory delimiter */
#define MATCH_ARCHIVE	"*.HPK"	/* All archive files */

/* The data block used by findFirst()/findNext() */

typedef struct {
			   ATTR fAttr;					/* File attributes */
			   LONG fTime;					/* File modification datestamp */
			   LONG fSize;					/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   BYTE dosReserved[ 22 ];		/* Reserved by GemDOS */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)	( fileInfo.fAttr & DIRECT )

/* The type of case conversion to perform on file and directory names.
   In both cases we force uppercase */

#define caseConvert		toupper				/* Force uppercase */
#define caseMatch		toupper				/* Force uppercase */
#define caseStrcmp		stricmp				/* Non-case-sensitive match */
#define caseStrncmp		strnicmp			/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		FALSE

/* Dummy routines for nonportable/unused functions */

#define setDirecTime(x,y)	/* Virtually impossible to do under Gem-DOS */
#define initExtraInfo()		/* Atari files have no extra info */
#define copyExtraInfo(x,y)
#define setExtraInfo(x)
#define endExtraInfo()

/* Dummy routine not needed in Atari version */

#define findEnd(x)

#endif /* __ATARI__ */

/****************************************************************************
*																			*
*							Definitions for Macintosh						*
*																			*
****************************************************************************/

#ifdef __MAC__

#define ARCHIVE_OS		OS_MAC

/* Maximum path length and filename length.  Note that although in theory
   Mac paths are limited to 254 characters in length, the way HPACK does it
   allows an unlimited path length - 512 seems like a safe upper limit */

#define MAX_PATH		512
#define MAX_FILENAME	31

/* The file attribute type and various useful attributes */

typedef WORD			ATTR;		/* Attribute data type */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		0			/* hcreat()'s mode parameter ignored */
#define MKDIR_ATTR		0			/* hmkdir()'s mode parameter ignored */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   On the Mac invisible files are treated seperately */

#define FILES			1			/* Match normal files only */
#define ALLFILES		1			/* Match special files as well */
#define FILES_DIRS 		2			/* Match normal files and directories */
#define ALLFILES_DIRS	2			/* Match all files and directories */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'\x01'
#define SLASH_STR		"\x01"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	';'

/* File codes for what we want to match */

#define MATCH_ALL		"\x02"			/* Just match anything */
#define SLASH_MATCH_ALL	"\x01\x02"		/* As above with directory delimiter */
#define MATCH_ARCHIVE	"\x03"			/* All archive files */

/* The data block used by findFirst()/findNext()/findEnd() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR fAttr;			/* File attributes */

			   /* Mac-specific information */
			   LONG createTime;		/* File creation time */
			   LONG backupTime;		/* File backup time */
			   LONG type;			/* The file's type */
			   LONG creator;		/* The file's creator */
			   LONG resSize;		/* Size of the resource fork */
			   BYTE versionNumber;	/* Version number of the file */
			   int entryNo;			/* Number of this entry in the directory */
			   short vRefNum;		/* Ref.no for volume file is on */
			   long dirID;			/* Directory ID for dir.file is in */
			   char matchAttr;		/* File attributes to match on */
			   BOOLEAN isDirectory;	/* Whether the file is a directory or not */
			   BOOLEAN matchArchive;/* Whether we want to match only archives */
			   } FILEINFO;

/* Determine whether a found file is a directory or not (a BOOLEAN set up
   by findFirst()/findNext()) */

#define isDirectory(fileInfo)		fileInfo.isDirectory

/* The type of case conversion to perform on file and directory names */

#define caseConvert(x)	x			/* No conversion */
#define caseMatch		toupper		/* Force uppercase */
#define caseStrcmp		stricmp		/* Non-case-sensitive match */
#define caseStrncmp		strnicmp	/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		TRUE

/* Prototypes for routines the Mac doesn't have */

void strlwr( const char *string );
int stricmp( const char *src, const char *dest );
int strnicmp( const char *src, const char *dest, int length );

/* Dummy routines for nonportable/nonused functions */

#define setDirecTime(dirName,time)	setFileTime( dirName, time )
#define findEnd(x)

#define initExtraInfo()		/* Not needed at the moment */
#define endExtraInfo()

#endif /* __MAC__ */

/****************************************************************************
*																			*
*							Definitions for MSDOS							*
*																			*
****************************************************************************/

#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )

#define ARCHIVE_OS		OS_MSDOS

/* Maximum DOS path length (63 bytes + null terminator) and filename length
   (8.3 + null delimiter) */

#define MAX_PATH		64
#define MAX_FILENAME	13

/* The file attribute type and various useful attributes */

typedef BYTE			ATTR;	/* Attribute data type */

#define HIDDEN			0x02	/* Hidden file */
#define SYSTEM			0x04	/* System file */
#define DIRECT			0x10	/* Directory bit */

/* Attributes as used by HPACK.  DOS distinguishes between normal files
   (DEFAULT and RDONLY) and special files (HIDDEN and SYSTEM) so there
   are two sets of attributes for matching, one for normal files and one
   for special files */

#define FILES			0		/* No attributes to match on (will still
								   match DEFAULT and RDONLY under DOS) */
#define ALLFILES		( HIDDEN | SYSTEM )
								/* All useful file attribute types. Again
								   will also match DEFAULT and RDONLY */
#define FILES_DIRS		DIRECT	/* All normal files + directories */
#define ALLFILES_DIRS	( ALLFILES | DIRECT )
								/* All useful file attribute types + dirs */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		0		/* Bits to set for hcreat() */
#define MKDIR_ATTR		0		/* Bits to set for hmkdir() */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	';'

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"*.*"	/* All files and directories */
#define SLASH_MATCH_ALL	"/*.*"	/* As above with directory delimiter */
#define MATCH_ARCHIVE	"*.HPK"	/* All archive files */

/* The data block used by findFirst()/findNext().  Remember to change
   findFirst() in HPACKIO.ASM if this struct is changed */

typedef struct {
			   BYTE dosReserved[ 21 ];		/* Reserved by DOS */
			   ATTR fAttr;					/* File attributes */
			   LONG fTime;					/* File modification datestamp */
			   LONG fSize;					/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)	( fileInfo.fAttr & DIRECT )

/* The type of case conversion to perform on file and directory names.
   In both cases we force uppercase */

#define caseConvert		toupper				/* Force uppercase */
#define caseMatch		toupper				/* Force uppercase */
#define caseStrcmp		stricmp				/* Non-case-sensitive match */
#define caseStrncmp		strnicmp			/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		FALSE

/* Dummy routines for nonportable/unused functions */

#define setDirecTime(x,y)	/* Virtually impossible to do under DOS */
#define initExtraInfo()		/* DOS files have no extra info */
#define copyExtraInfo(x,y)
#define setExtraInfo(x)
#define endExtraInfo()

/* Dummy routine not needed in MSDOS version */

#define findEnd(x)

#endif /* __MSDOS16__ || __MSDOS32__ */

/****************************************************************************
*																			*
*							Definitions for OS/2							*
*																			*
****************************************************************************/

#ifdef __OS2__

#define ARCHIVE_OS		OS_OS2

/* Maximum OS/2 path length and filename length (For HPFS Max) */

#define MAX_PATH		260
#define MAX_FILENAME	256

/* File attributes */

typedef BYTE			ATTR;		/* Attribute data type */

#define RDONLY			0x01		/* Read only attribute */
#define HIDDEN			0x02		/* Hidden file */
#define SYSTEM			0x04		/* System file */
#define LABEL			0x08		/* Volume label */
#define DIRECT			0x10		/* Directory */
#define ARCH			0x20		/* Archive bit set */

/* Attributes as used by HPACK.  OS/2 distinguishes between normal files
   (DEFAULT and READONLY) and special files (RDONLY and SYSTEM) so there
   are two sets of attributes for matching, one for normal files and one
   for special files */

#define FILES			0		/* No attributes to match on (will still
								   match DEFAULT and RDONLY under OS/2) */
#define ALLFILES		( HIDDEN | SYSTEM )
								/* All useful file attribute types. Again
								   will also match DEFAULT and RDONLY */
#define FILES_DIRS		DIRECT	/* All normal files + directories */
#define ALLFILES_DIRS	( ALLFILES | DIRECT )
								/* All useful file attribute types + dirs */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		( O_RDWR | S_DENYRDWR )
#define MKDIR_ATTR		0

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	';'

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"*.*"	/* All files and directories */
#define SLASH_MATCH_ALL	"/*.*"	/* As above with directory delimiter */
#define MATCH_ARCHIVE	"*.HPK"	/* All archive files */

/* The data block used by findFirst()/findNext() */

typedef struct {
			   LONG fTime; 				    /* File modification datestamp */
			   LONG fSize;					/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR fAttr;					/* File attributes */

			   /* OS/2-specific information */
			   int hdir;					/* Dir.handle ret.by DosFindFirst() */
			   LONG eaSize;					/* Size of extended attributes */
			   LONG cTime;					/* File creation datestamp */
			   LONG aTime;					/* File access datestamp */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)	( fileInfo.fAttr == DIRECT )

/* The type of case conversion to perform on file and directory names.
   If this is an HPFS volume, we leave case as is; otherwise, we force
   uppercase.  When performing a match, we force uppercase so as to be
   case-insensitive */

extern BOOLEAN isHPFS;		/* Whether this is an HPFS filesystem or not */
extern BOOLEAN destIsHPFS;	/* Whether the destination FS is HPFS or not */

#define caseConvert(x)	( isHPFS ? ( x ) : toupper( x ) )
#define caseMatch		toupper				/* Force uppercase */
#define caseStrcmp		stricmp				/* Non-case-sensitive match */
#define caseStrncmp		strnicmp			/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		destIsHPFS			/* Determine at runtime */

/* Dummy routines for nonportable/unused functions */

#define setDirecTime(dirName,time)	setFileTime( dirName, time )
#define initExtraInfo()						/* Done on the fly */
#define endExtraInfo()

#ifdef GCC

/* Extra routines not part of gcc */

int strnicmp( const char *src, const char *dest, int length );
int stricmp( const char *src, const char *dest );
void strlwr( char *string );

#endif /* GCC */

#endif /* __OS2__ */

/****************************************************************************
*																			*
*							Definitions for UNIX							*
*																			*
****************************************************************************/

#ifdef __UNIX__

#define ARCHIVE_OS		OS_UNIX

#include <sys/types.h>
#if defined( NEXT )
  #include <sys/time.h>
#else
  #include <utime.h>
#endif /* NEXT */
#include <sys/stat.h>
#if !defined( ISC ) && !defined( NEXT )
  #include <unistd.h>
  #include <dirent.h>
#endif /* !( ISC || NEXT ) */
#if defined( AIX ) || defined( AIX386 ) || defined( AIX370 ) || \
	defined( CONVEX ) || defined( NEXT )
  #include <limits.h>
  #include <sys/dir.h>
#endif /* AIX || AIX386 || AIX370 || CONVEX || NEXT */
#ifdef NEXT
  #define dirent direct

  struct utimbuf {
				 time_t actime;		/* File/dir access time */
				 time_t modtime;	/* File/dir modification time */
				 };
#endif /* NEXT */

/* Maximum path length (x bytes + null terminator) and filename length
   (y bytes + null terminator) */

#if defined( AIX ) || defined( AIX370 ) || defined( AIX386 )
  #define MAX_PATH			( 1023 + 1 )
  #define MAX_FILENAME		( 254 + 1 )
#elif defined( BSD386 ) || defined( GENERIC ) || defined( HPUX ) || \
	  defined( IRIX ) || defined( LINUX ) || defined( NEXT ) || \
	  defined( OSF1 ) || defined( SUNOS ) || defined( ULTRIX ) || \
	  defined( ULTRIX_OLD ) || defined( UTS4 )
  #define MAX_PATH			( 256 + 1 )
  #define MAX_FILENAME		( 128 + 1 )
#elif defined( CONVEX )
  #define MAX_PATH          ( _POSIX_PATH_MAX + 1 )
  #define MAX_FILENAME		( 254 + 1 )
#elif defined( POSIX )
  #define MAX_PATH          ( _POSIX_PATH_MAX + 1 )
  #define MAX_FILENAME		( NAME_MAX + 1 )
#elif defined( SVR4 )
  #define MAX_PATH			( 64 + 1 )
  #define MAX_FILENAME		( 14 + 1 )
#elif defined( MINT )
  #define MAX_PATH			64
  #define MAX_FILENAME		13
#else
  #error Need to define filename/path length in system.h
#endif /* Various mutation-dependant defines */

/* The size of the directory buffer used by findFirst/findNext() */

#define DIRINFO_SIZE		sizeof( struct direct )
#ifndef POSIX
  #define DIRBUF_ENTRIES	20		/* No.entries in one buffer */
#else
  #define DIRBUF_ENTRIES	_DIRBUF	/* No.entries in one buffer */
#endif /* !POSIX */

/* The file attribute type and various useful attributes */

typedef WORD			ATTR;		/* Attribute data type */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		0644		/* Bits to set for hcreat() */
#define MKDIR_ATTR		0744		/* Bits to set for hmkdir() */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   Since Unix doesn't distinguish between normal and special files these
   are treated identically */

#define FILES			1			/* Match normal files only */
#define ALLFILES		1			/* Match special files as well */
#define FILES_DIRS 		2			/* Match normal files and directories */
#define ALLFILES_DIRS	2			/* Match all files and directories */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	':'

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"."			/* All files and directories */
#define SLASH_MATCH_ALL	"/."		/* As above with directory delimiter */
#define MATCH_ARCHIVE	"."			/* All archive files */

/* The data block used by findFirst()/findNext()/findEnd() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR matchAttr;		/* File attributes to match on */

			   /* Unix-specific information */
			   DIR *dirBuf;			/* Directory buffer. */
			   char dName[ MAX_PATH ];	/* Pathname for this dir. */
			   struct stat statInfo;/* Info returned by stat() */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)	( ( fileInfo.statInfo.st_mode & S_IFMT ) == S_IFDIR )

/* The type of case conversion to perform on file and directory names.
   In both cases we leave it as is, to be case-insensitive */

#define caseConvert(x)	x			/* No conversion */
#define caseMatch(x)	x
#define caseStrcmp		strcmp		/* Case-sensitive string match */
#define caseStrncmp		strncmp		/* Case-sensitive string match */

/* Whether spaces are legal in filenames */

#if defined( AIX ) || defined( AIX386 ) || defined( AIX370 ) || \
	defined( BSD386 ) || defined( GENERIC ) || defined( HPUX ) || \
	defined( IRIX ) || defined( NEXT ) || defined( OSF1 ) || \
	defined( ULTRIX ) || defined( ULTRIX_OLD ) || defined( UTS4 )
  #define SPACE_OK		TRUE
#else
  #define SPACE_OK		FALSE
#endif /* Space OK in filenames */

/* Prototypes/defines for standard UNIX routines */

#define setDirecTime(dirName,time)	setFileTime( dirName, time )

/* Dummy routines for nonportable/unused functions */

#define initExtraInfo()		/* Unix files have no extra info */
#define copyExtraInfo(x,y)
#define setExtraInfo(x)
#define endExtraInfo()

/* Definition profiles for different variants of Unix.  The following can be
   turned on through the use of defines: NEED_MEMMOVE, NEED_RENAME,
   NEED_STRLWR, NEED_STRICMP */

#if defined( AIX ) || defined( AIX370 )
  #define NEED_STRLWR					/* AIX RS6000, AIX 370 */
  #define NEED_STRICMP
#elif defined( AIX386 )
  #define NEED_STRLWR					/* AIX 386 */
  #define NEED_STRICMP
  #define NEED_MEMMOVE
#elif defined( BSD386 )					/* BSD 386 */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( CONVEX )					/* Convex OS */
  #define NEED_STRICMP
#elif defined( GENERIC )				/* Generic */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( HPUX )					/* HPUX */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( IRIX )					/* Irix */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( ISC )					/* ISC Unix */
#elif defined( LINUX )					/* Linux */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( MINT )					/* MiNT */
  #error Need to set up definition profile in system.h
#elif defined( NEXT )					/* NeXT */
  #define NEED_STRLWR
  #define NEED_STRICMP
  #define NEED_MEMMOVE
#elif defined( OSF1 )					/* OSF 1 */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( POSIX )					/* Posix */
#elif defined( SUNOS )					/* SunOS */
  #define NEED_MEMMOVE
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( SVR4 )					/* SVR4 */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( ULTRIX )					/* Ultrix 4.x and above */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( ULTRIX_OLD )				/* Ultrix 3.x and below */
  #define NEED_STRLWR
  #define NEED_MEMMOVE
  #define NEED_STRICMP
#elif defined( UTS4 )					/* Amdahl UTS4 */
  #define NEED_STRLWR
  #define NEED_STRICMP
#elif defined( ESIX )					/* Esix */
  #define NEED_HELP
#endif /* Various mutation-specific defines */

/* Some prototypes which may be necessary for some versions */

#ifdef NEED_RENAME
  int rename( const char *srcName, const char *destName );
#endif /* NEED_RENAME */
#ifdef NEED_STRLWR
  void strlwr( char *string );
#endif /* NEED_STRLWR */
#ifdef NEED_MEMMOVE
  void memmove( char *dest, char *src, int length );
#endif /* NEED_MEMMOVE */
#ifdef NEED_STRICMP
  int stricmp( const char *str1, const char *str2 );
  int strnicmp( const char *src, const char *dest, int length );
#endif /* NEED_STRICMP */

#endif /* __UNIX__ */

/****************************************************************************
*																			*
*								Definitions for VMS							*
*																			*
****************************************************************************/

#ifdef __VMS__

#define ARCHIVE_OS		OS_VMS

/* Any necessary #include's here */

/* Maximum path length (x bytes + null terminator) and filename length
   (y bytes + null terminator) */

#define MAX_PATH		-1			/* Whatever it is */
#define MAX_FILENAME	-1

/* The file attribute type and various useful attributes */

typedef WORD			ATTR;		/* Attribute data type */

/* Mode/attribute bits for hcreat()/hmkdir() */

#define CREAT_ATTR		-1			/* Bits to set for hcreat() */
#define MKDIR_ATTR		-1			/* Bits to set for hmkdir() */

/* The following pseudo-attributes must be sorted out in findFirst/Next().
   Since VMS doesn't distinguish between normal and special files these
   are treated identically */

#define FILES			1			/* Match normal files only */
#define ALLFILES		1			/* Match special files as well */
#define FILES_DIRS 		2			/* Match normal files and directories */
#define ALLFILES_DIRS	2			/* Match all files and directories */

/* The directory seperator (a logical slash), and its stringified equivalent */

#define SLASH			'/'
#define SLASH_STR		"/"

/* The seperator for paths in environment variables */

#define PATH_SEPERATOR	';'

/* The types of files findFirst() can match (when it's not matching a
   literal file or directory name) */

#define MATCH_ALL		"*.*;*"		/* All files and directories */
#define SLASH_MATCH_ALL	"/*.*;*"	/* As above with directory delimiter */
#define MATCH_ARCHIVE	"*.HPK;0"	/* Most recent archive */

/* The data block used by findFirst()/findNext()/findEnd() */

typedef struct {
			   LONG fTime;			/* File modification datestamp */
			   LONG fSize;			/* File size */
			   char fName[ MAX_FILENAME ];	/* File name */
			   ATTR matchAttr;		/* File attributes to match on */

			   /* VMS-specific information, eg returned attributes, ACL's, etc */
			   } FILEINFO;

/* Determine whether a found file is a directory or not */

#define isDirectory(fileInfo)	( whatever )

/* The type of case conversion to perform on file and directory names.
   In both cases we force uppercase */

#define caseConvert		toupper				/* Force uppercase */
#define caseMatch		toupper				/* Force uppercase */
#define caseStrcmp		stricmp				/* Non-case-sensitive match */
#define caseStrncmp		strnicmp			/* Non-case-sensitive match */

/* Whether spaces are legal in filenames */

#define SPACE_OK		FALSE

/* Prototypes/defines for standard VMS routines */

#define setDirecTime(dirName,time)	setFileTime( dirName, time )

/* Dummy routines for nonportable/unused functions */

#define initExtraInfo()		/* VMS files have no extra info */
#define copyExtraInfo(x,y)
#define setExtraInfo(x)
#define endExtraInfo()

/* Prototypes for extra routines */

void rename( const char *srcName, const char *destName );
void strlwr( char *string );
void memmove( void *dest, void *src, int length );
int stricmp( const char *str1, const char *str2 );
int strnicmp( char *src, char *dest, int length );

#endif /* __VMS__ */

/****************************************************************************
*																			*
* 					Prototypes for Nonportable Functions 					*
*																			*
****************************************************************************/

/* All the following are conditional since they may be defined as dummy
   functions */

#ifndef setFileTime
  void setFileTime( const char *fileName, const LONG fileTime );
#endif /* setFileTime */
#ifndef setDirecTime
  void setDirecTime( const char *dirName, const LONG dirTime );
#endif /* setDirecTime */
#ifndef getCountry
  int getCountry( void );
#endif /* getCountry */
#ifndef getScreenSize
  void getScreenSize( void );
#endif /* getScreenSize */
#ifndef findFirst
  BOOLEAN findFirst( const char *filePath, const ATTR fileAttr, FILEINFO *fileInfo );
  BOOLEAN findNext( FILEINFO *fileInfo );
#endif /* findFirst */
#ifndef findEnd
  void findEnd( FILEINFO *fileInfo );
#endif /* findEnd */
#ifndef isSameFile
  BOOLEAN isSameFile( const char *pathName1, const char *pathName2 );
#endif /* isSameFile */
#ifndef initExtraInfo
  void initExtraInfo( void );
  void endExtraInfo( void );
#endif /* initExtraInfo */
#ifndef copyExtraInfo
  void copyExtraInfo( const char *srcFilePath, const char *destFilePath );
#endif /* copyExtraInfo */
#ifndef setExtraInfo
  void setExtraInfo( const char *filePath );
#endif /* setExtraInfo */
#ifndef getRandomInfo
  void getRandomInfo( BYTE *buffer, int bufSize );
#endif /* getRandomInfo */

/* Default OS */

#ifndef ARCHIVE_OS
  #define ARCHIVE_OS	OS_UNKNOWN
#endif /* !ARCHIVE_OS */

#endif /* _SYSTEM_DEFINED */
