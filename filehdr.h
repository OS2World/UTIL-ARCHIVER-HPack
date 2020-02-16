/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						 Archive Directory Structure Header					*
*							FILEHDR.H  Updated 20/03/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _FILEHDR_DEFINED

#define _FILEHDR_DEFINED

#if defined( __ATARI__ )
  #include "asn1\asn1.h"
#elif defined( __MAC__ )
  #include "asn1.h"
#else
  #include "asn1/asn1.h"
#endif /* System-specific include directives */

/****************************************************************************
*																			*
*					File and Directory Information Bitfields				*
*																			*
****************************************************************************/

/* The bitfields are handled via bit twiddling of a WORD/BYTE for portability
   reasons.

typedef struct {
			   unsigned origLen	   : 1;	\* fileLen field WORD/LONG bit *\
			   unsigned coprLen	   : 1;	\* dataLen field WORD/LONG bit *\
			   unsigned otherLen   : 2;	\* dirIndex/extraLen field length *\
			   unsigned isSpecial  : 1; \* Whether this is type SPECIAL hdr *\
			   unsigned hasExtra   : 1;	\* Whether extra info is attached *\
			   unsigned isText	   : 1; \* Whether this is a text file *\
			   unsigned dataFormat : 3; \* Storage format type *\
			   unsigned systemType : 6;	\* The system the archive was created on *\
			   } ARCHIVEINFO;

typedef struct {
			   unsigned otherLen   : 2; \* parentIndex/dataLen field length *\
			   unsigned isSpecial  : 1; \* Whether this is a type SPECIAL header *\
			   unsigned linked     : 1; \* Whether this is a linked header *\
			   unsigned unicode    : 1; \* Whether dir.name is Unicode *\
			   unsigned spare      : 3; \* Reserved for future use *\
			   } DIRINFO; */

/* The values for the ARCHIVEINFO bitfield */

#define ARCH_ORIG_LEN	0x8000	/* fileLen field WORD/LONG flag */
#define ARCH_COPR_LEN	0x4000	/* dataLen field WORD/LONG flag */
#define ARCH_OTHER_LEN	0x3000	/* dirIndex/auxLen field length */
#define ARCH_OTHER_HI	0x2000	/* High bit of ARCH_OTHER_LEN */
#define ARCH_OTHER_LO	0x1000	/* Low bit of ARCH_OTHER_LEN */
#define ARCH_SPECIAL	0x0800	/* Whether this is type SPECIAL header */
#define ARCH_EXTRAINFO	0x0400	/* Whether there is extra data attached */
#define ARCH_ISTEXT		0x0200	/* Whether this is a text file */
#define ARCH_STORAGE	0x01C0	/* Storage type */
#define ARCH_SYSTEM		0x003F	/* The system the archive was created on */

/* The values for the DIRINFO bitfield */

#define DIR_OTHER_LEN	0xC0	/* parentIndex/dataLen field length */
#define DIR_OTHER_HI	0x80	/* High bit of ARCH_OTHER_LEN */
#define DIR_OTHER_LO	0x40	/* How bit of ARCH_OTHER_LEN */
#define DIR_SPECIAL		0x20	/* Whether this is a type SPECIAL header */
#define DIR_LINKED		0x10	/* Whether the directory header is linked */
#define DIR_UNICODE		0x08	/* Whether directory name is Unicode */
#define DIR_RESERVED	0x07	/* Reserved for future use */

/* The value of the ARCH_OTHER_LEN field */

#define ARCH_OTHER_ZERO_ZERO 0x0000				/* Fields: ROOT_DIR, 0 */
#define ARCH_OTHER_BYTE_BYTE ( ARCH_OTHER_LO )	/* Fields: BYTE, BYTE */
#define ARCH_OTHER_BYTE_WORD ( ARCH_OTHER_HI )	/* Fields: BYTE, WORD */
#define ARCH_OTHER_WORD_LONG ( ARCH_OTHER_HI | ARCH_OTHER_LO )/* Fields: WORD, LONG */

/* The value of the DIR_OTHER_LEN field */

#define DIR_OTHER_ZERO_ZERO	0x00				/* Fields: ROOT_DIR, 0 */
#define DIR_OTHER_BYTE_BYTE	( DIR_OTHER_LO )	/* Fields: BYTE, BYTE */
#define DIR_OTHER_BYTE_WORD ( DIR_OTHER_HI )	/* Fields: BYTE, WORD */
#define DIR_OTHER_WORD_LONG	( DIR_OTHER_HI | DIR_OTHER_LO )/* Fields: WORD, LONG */

/* The format of the data within the archive.  FORMAT_UNKNOWN must be the
   last entry for a valid file */

#define FORMAT_STORED		( 0 << 6 )
#define FORMAT_PACKED		( 1 << 6 )
#define FORMAT_UNKNOWN		( 2 << 6 )

/* The OS we are running under. 'OS_UNKNOWN' must always be the last entry. */

enum { OS_MSDOS, OS_UNIX, OS_AMIGA, OS_MAC, OS_ARCHIMEDES, OS_OS2, OS_IIGS,
	   OS_ATARI, OS_VMS, OS_PRIMOS, OS_UNKNOWN };

/****************************************************************************
*																			*
*							  Internal File Headers							*
*																			*
****************************************************************************/

/* Note that due to the variable length of the headers when written to disk
   the lengths of some fields in these structures may not correspond to the
   actual data on disk */

typedef struct {
			   LONG fileTime;		/* Date + time of the file */
			   LONG fileLen;		/* Original data length */
			   LONG dataLen;		/* Archived data length */
			   LONG auxDataLen;		/* Auxiliary data field length */
			   WORD dirIndex;		/* Where the file is in the dir.tree */
			   WORD archiveInfo;	/* File-specific bitflags */
			   } FILEHDR;

/* The internal directory header */

typedef struct {
			   LONG dirTime;			/* Date + time of the directory */
			   LONG dataLen;			/* Directory data length */
			   WORD parentIndex;		/* The parent of this directory */
			   BYTE dirInfo;			/* Directory-specific bitflags */
			   } DIRHDR;

/****************************************************************************
*																			*
*							Special File/Directory Types					*
*																			*
****************************************************************************/

/* The types of files with main file type SPECIAL in the archInfo field.
   These and a type ID in the word following the header, giving 65536 types
   of special files */

#define TYPE_NORMAL			0			/* Normal file header */
#define TYPE_DUMMY			1			/* Dummy file header */

#define TYPE_COMMENT_TEXT	2			/* Comment types */
#define TYPE_COMMENT_ANSI	3
#define TYPE_COMMENT_GIF	4

#define TYPE_LAST_TYPE		5			/* End of valid type range */

/* The types of directories with the DIR_SPECIAL bit set in the parentIndex
   field */

#define DIRTYPE_NORMAL		0			/* Normal directory header */
#define DIRTYPE_DUMMY		1			/* Dummy directory header */

/****************************************************************************
*																			*
*							File Extrainfo Handling							*
*																			*
****************************************************************************/

/* The defines to handle the extraInfo data:

typedef struct {
			   unsigned secured     : 1; \* Whether data is secured *\
			   unsigned encrypted   : 1; \* Whether data is encrypted *\
			   unsigned linked		: 1; \* Whether file is linked *\
			   unsigned unicode     : 1; \* Whether filename is Unicode *\
			   unsigned extraLength : 4; \* Length of extraInfo field *\
			   } EXTRAINFO; */

#define EXTRALEN_MASK		0x0F		/* Length field of extraInfo byte */

#define getExtraInfoLen(x)	( x & EXTRALEN_MASK )

#define EXTRA_SECURED		0x80		/* isSecured bit */
#define EXTRA_ENCRYPTED		0x40		/* isEncrypted bit */
#define EXTRA_LINKED		0x20		/* isLinked bit */
#define EXTRA_UNICODE		0x10		/* isUnicode bit */

/****************************************************************************
*																			*
*								Link Handling								*
*																			*
****************************************************************************/

/* Defines to handle links */

#define LINK_ANCHOR			0x8000		/* Link is an anchor node */

#define isLinkAnchor(x)		( x & LINK_ANCHOR )
#define getLinkID(x)		( x & ~LINK_ANCHOR )

/****************************************************************************
*																			*
*						Level II Format File Header							*
*																			*
****************************************************************************/

/* The directory entry for each file in an archive */

typedef struct {
			   /* The file's name and ID information */
			   GENERALIZED_STRING fileName;
			   INTEGER linkID;				/* ID of linked files */
			   INTEGER internalType;		/* File type */

			   /* Date information */
			   TIME modificationTime;		/* Time of last modification */

			   /* File size information */
			   INTEGER fileSize;			/* Original file size */
			   } DIR_ENTRY;

/* The file header for each file in an archive */

typedef struct {
			   /* The file's name */
			   GENERALIZED_STRING fileName;

			   /* File size information */
			   INTEGER fileSize;			/* Original file size */
			   INTEGER fileSizeAsStored;	/* Size of file as stored */
			   INTEGER fileFutureSize;		/* Possible future size */

			   /* Internal bookkeeping information */
			   INTEGER directoryID;			/* ID of containing directory */
			   INTEGER linkID;				/* ID of linked files */
			   INTEGER internalType;		/* File type */
			   INTEGER version;				/* File version number */

			   /* Miscellaneous mission-critical information */
			   OCTETSTRING miscInfo;		/* Misc.information */

			   /* Date information */
			   TIME creationTime;			/* Time of creation */
			   TIME modificationTime;		/* Time of last modification */
			   TIME readAccessTime;			/* Time of last read access */
			   TIME attributeModTime;		/* Time of last attribute mod.*/
			   TIME backupTime;				/* Time of last backup */
			   TIME effectiveTime;			/* Time at which use is OK */
			   TIME expiryTime;				/* Time at which deletion is OK */

			   /* Permitted actions */
			   PERMITTED_ACTIONS owner;		/* Owner permitted actions */
			   PERMITTED_ACTIONS group;		/* Group permitted actions */
			   PERMITTED_ACTIONS world;		/* World permitted actions */
			   PERMITTED_ACTIONS system;	/* System permitted actions */

			   /* Accounting information */
			   ACCOUNTING_INFO creatorID;	/* ID of creator */
			   ACCOUNTING_INFO modifierID;	/* ID of last modifier */
			   ACCOUNTING_INFO readerID;	/* ID of last reader */
			   ACCOUNTING_INFO attributeModID;	/* ID of last attribute modifier */
			   } FILEHEADER;

/* The header for each data block in a file */

typedef struct {
			   DATATYPE dataType;			/* Data type */
			   STORAGETYPE storageType;		/* Storage type */
			   ECCTYPE eccType;				/* ECC type */
			   CRYPTYPE cryptType;			/* Encryption type */
			   AUTHENTYPE authentType;		/* Authentication type */
			   FORMATYPE formatType;		/* Data format type */
			   } DATAHEADER;

#endif /* _FILEHDR_DEFINED */
