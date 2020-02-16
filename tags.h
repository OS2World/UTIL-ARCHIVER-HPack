/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							HPACK Archive Tag Settings						*
*							 TAGS.H  Updated 22/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* This file defines all tags values used in HPACK.  The format for tags
   is described in HPACKSTD.TXT */

#if defined( __ATARI__ )
  #include "io\hpackio.h"
#elif defined( __MAC__ )
  #include "hpackio.h"
#else
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* 						--------- WARNING --------

				This file should be treated as READ-ONLY!
   Never make any changes to this file yourself as it will probably render
   your version of HPACK incompatible with all other versions.  Changes to
   this  file  should  be  submitted to  Peter  Gutmann  so  they  can  be
   redistributed to all HPACK developers.

						--------------------------- */

/* The number of short tags.  ID's of 0 .. NO_SHORT_TAGS are short tag ID,
   ID's over NO_SHORT_TAGS are long ID's.  The maximum short-tag ID has a
   special status in that is denotes a long tag (a variable-length structure
   rather than the fixed-format 16-bit short tag) */

#define NO_SHORT_TAGS		1024
#define isShortTag(tagID)	( ( tagID ) < NO_SHORT_TAGS )

/* The magic value for the tagID which denotes a long tag */

#define LONG_BASE		0xFFC0

/* The basic size of each tag (long tags are actially variable-length,
   the value given here is the size of the tag header field */

#define SHORT_TAGSIZE			sizeof( WORD )
#define LONG_TAGSIZE			( sizeof( WORD ) + sizeof( BYTE ) )

/* Macros to handle short tags */

#define TAGID_MASK		0xFFC0		/* Mask to extract tag ID's */
#define DATALEN_MASK	0x003F		/* Mask to extract short tag lengths */
#define TAGID_SHIFT		6
#define SHORT_TAGLEN	( 1 << TAGID_SHIFT )	/* Max.data size for short tag */

/* Macros to handle long tags */

#define LONGTAG_BYTE_WORD	0x0000	/* Length = BYTE, uncoprLength = WORD */
#define LONGTAG_WORD_WORD	0x0010	/* Length = WORD, uncoprLength = WORD */
#define LONGTAG_WORD_LONG	0x0020	/* Length = WORD, uncoprLength = LONG */
#define LONGTAG_LONG_LONG	0x0030	/* Length = LONG, uncoprLength = LONG */

#define LONGTAG_FORMAT_MASK	0x000E	/* Mask for long tag storage type */
#define LONGTAG_LENGTH_MASK	0x0030	/* Mask for long tag length field */
#define LONGTAG_FORMAT_SHIFT	1	/* Shift amount for format field */

/* A dummy length for tags.  If the tag is stored uncompressed, we don't
   really need to store the compressed and uncompressed length, so if
   writeTag() finds this value it just ignores it */

#define LEN_NONE		0L

/* An illegal tag ID used to indicate tags which we can't handle */

#define BAD_TAGID		0xFFFF

/* Classes of tags */

enum { TAGCLASS_NONE, TAGCLASS_ATTRIBUTE, TAGCLASS_COMMENT, TAGCLASS_TIME,
	   TAGCLASS_ICON, TAGCLASS_MISC };

/* Storage formats for long tags */

enum { TAGFORMAT_STORED, TAGFORMAT_PACKED };

/* The struct containing information on each tag */

typedef struct {
			   LONG dataLength;		/* Size of tagged data */
			   LONG uncoprLength;	/* Size of uncompressed tagged data */
			   WORD tagID;			/* The tag's ID */
			   BYTE dataFormat;		/* Storage format type */
			   } TAGINFO;

/* Prototypes for the read/write tag functions */

int writeTag( WORD tagID, const WORD store, const LONG dataLength, const LONG uncoprLength );
int writeDirTag( WORD tagID, const WORD store, const LONG dataLength, const LONG uncoprLength );
int readTag( long *tagFieldSize, TAGINFO *tagInfo );

ATTR readAttributeData( const WORD attributeID );

/****************************************************************************
*																			*
*									Tag ID's								*
*																			*
****************************************************************************/

/* Tag ID's are specified as an enumerated type to solve potential problems
   with duplication of tag ID's.  New types *must* be added to the end of
   the list, *never* to the middle.  The first group is the short tag ID's,
   the second the long tag ID's.  In some cases long tag ID's can have short
   forms, which are the same as the long forms but with an internal '_S_'.
   These are never seen by the rest of HPACK, and are only used by read/
   writeTag() */

enum { TAG_MSDOS_ATTR, TAG_UNIX_ATTR, TAG_MAC_ATTR, TAG_QNX_ATTR,
	   TAG_ARC_ATTR, TAG_AMIGA_ATTR, TAG_ATARI_ATTR, TAG_APPLE_ATTR,
	   TAG_OS2_ATTR, TAG_VMS_ATTR, TAG_ACCESS_TIME, TAG_BACKUP_TIME,
	   TAG_CREATION_TIME, TAG_EXPIRATION_TIME, TAG_S_ERROR, TAG_S_COMMENT,
	   TAG_S_LONGNAME, TAG_POSITION, TAG_WINSIZE, TAG_VERSION_NUMBER,
	   TAG_MAC_TYPE, TAG_MAC_CREATOR, TAG_LOAD_ADDRESS, TAG_EXECUTE_ADDRESS };

enum { TAG_COMMENT = NO_SHORT_TAGS, TAG_OS2_ICON, TAG_LONGNAME, TAG_ERROR,
	   TAG_OS2_MISC_EA, TAG_SECURITY_INFO, TAG_RESOURCE_FORK, TAG_AMIGA_ICON };

/****************************************************************************
*                                                                           *
*								Attribute Tags								*
*																			*
****************************************************************************/

/* TAG_AMIGA_ATTR: Amiga file info, organised as follows:

   BYTE : attributes

   7 6 5 4 3 2 1 0
   H S P A R W X D
   | | | | | | | +-> Delete flag
   | | | | | | +---> Execute bit
   | | | | | +-----> Write bit
   | | | | +-------> Read bit
   | | | +---------> Archive bit (clear = file changed)
   | | +-----------> Pure flag (program is reentrant)
   | +-------------> Script file flag
   +---------------> Hidden flag */

#define AMIGA_ATTR_DELETE	0x01
#define AMIGA_ATTR_EXECUTE	0x02
#define AMIGA_ATTR_WRITE	0x04
#define AMIGA_ATTR_READ		0x08
#define AMIGA_ATTR_ARCHIVE	0x10
#define AMIGA_ATTR_PURE		0x20
#define AMIGA_ATTR_SCRIPT	0x40
#define AMIGA_ATTR_HIDDEN	0x80

#define LEN_AMIGA_ATTR		1

/* TAG_APPLE_ATTR: Apple IIgs file info, organised as follows:

   WORD : attributes

   FEDCBA98 7 6 5 432 1 0
   ----+--- | | | -+- | +-> Read permission
	   |    | | |  |  +---> Write permission
	   |    | | |  +------> Unused - set to 0
	   |    | | +---------> Backup needed
	   |    | +-----------> Rename
	   |    +-------------> Destroy
	   +------------------> Unused - set to 0 */

#define APPLE_ATTR_READ		0x0001
#define APPLE_ATTR_WRITE	0x0002
#define APPLE_ATTR_BACKUP	0x0020
#define APPLE_ATTR_RENAME	0x0040
#define APPLE_ATTR_DESTROY	0x0080

#define LEN_APPLE_ATTR		2

/* TAG_ARC_ATTR: Archimedes file info, organised as follows:

   BYTE : attributes

   76 54 3 2 10
   xx rw L x RW
   -+ -+ | | ++-> Read/Write Access - Owner
	|  | | +----> Unused - set to 0
	|  | +------> Locked bit
	|  +--------> Read/Write Access - Others
	+-----------> Unused - set to 0 */

#define	ARC_ATTR_WRITE_OWNER	0x0001
#define ARC_ATTR_READ_OWNER		0x0002
#define ARC_ATTR_LOCKED			0x0008
#define ARC_ATTR_WRITE_OTHER	0x0010
#define ARC_ATTR_READ_OTHER		0x0020

#define LEN_ARC_ATTR			1

/* TAG_ATARI_ATTR: Atari ST file info, organised as follows:

   BYTE : attributes

   76 5 4 3 2 1 0
   xx A D V S H R
   +- | | | | | +-> Read-only flag
   |  | | | | +---> Hidden/Invisible flag
   |  | | | +-----> System flag
   |  | | +-------> Volume label flag (not used in HPACK - set to 0)
   |  | +---------> Directory flag (not used in HPACK - set to 0)
   |  +-----------> Archive bit
   +--------------> Unused - set to 0 */

#define ATARI_ATTR_RDONLY	0x01
#define ATARI_ATTR_HIDDEN	0x02
#define ATARI_ATTR_SYSTEM	0x04
#define ATARI_ATTR_ARCHIVE	0x20

#define LEN_ATARI_ATTR		1

/* TAG_MAC_ATTR: Macintosh file info, organised as follows:

   WORD : attributes

   F E D C B A 9 8 76543210
   | | | | | | | | ++++++++-> Unused - set to 0
   | | | | | | | +----------> Init
   | | | | | | +------------> Changed
   | | | | | +--------------> Busy
   | | | | +----------------> Bozo
   | | | +------------------> System
   | | +--------------------> Bundle
   | +----------------------> Invisible
   +------------------------> Locked */

#define MAC_ATTR_INIT		0x0100
#define MAC_ATTR_CHANGED	0x0200
#define MAC_ATTR_BUSY		0x0400
#define MAC_ATTR_BOZO		0x0800
#define MAC_ATTR_SYSTEM		0x1000
#define MAC_ATTR_BUNDLE		0x2000
#define MAC_ATTR_INVISIBLE	0x4000
#define MAC_ATTR_LOCKED		0x8000

#define LEN_MAC_ATTR		2

/* TAG_MSDOS_ATTR: The MSDOS attribute bits, organised as follows:

   BYTE : attributes

   7 6 5 4 3 2 1 0
   N x A D V S H R
   | | | | | | | +-> Read-only flag
   | | | | | | +---> Hidden/Invisible flag
   | | | | | +-----> System flag
   | | | | +-------> Volume label flag (not used in HPACK - set to 0)
   | | | +---------> Directory flag (not used in HPACK - set to 0)
   | | +-----------> Archive bit
   | +-------------> Unused - set to 0
   +---------------> Nominally unused - used by Novell netware? */

#define MSDOS_ATTR_RDONLY	0x01
#define MSDOS_ATTR_HIDDEN	0x02
#define MSDOS_ATTR_SYSTEM	0x04
#define MSDOS_ATTR_ARCHIVE	0x20

#define LEN_MSDOS_ATTR		1

/* TAG_OS2_ATTR: The OS/2 attribute bits, organised as follows:

   BYTE : attributes

   7 6 5 4 3 2 1 0
   N x A D V S H R
   | | | | | | | +-> Read-only flag
   | | | | | | +---> Hidden/Invisible flag
   | | | | | +-----> System flag
   | | | | +-------> Volume label flag (not used in HPACK - set to 0)
   | | | +---------> Directory flag (not used in HPACK - set to 0)
   | | +-----------> Archive bit
   | +-------------> Unused - set to 0
   +---------------> Nominally unused - used by Novell netware? */

#define OS2_ATTR_RDONLY		0x01
#define OS2_ATTR_HIDDEN		0x02
#define OS2_ATTR_SYSTEM		0x04
#define OS2_ATTR_ARCHIVE	0x20

#define LEN_OS2_ATTR		1

/* TAG_QNX_ATTR: QNX 2.x/3.x file info, organised as follows:

   BYTE : group permission
   BYTE : member permission
   BYTE : owner permission
   BYTE : owner group
   BYTE : owner member */

#define LEN_QNX_ATTR			5

/* TAG_UNIX_ATTR: The UNIX permission flag bits, organised as follows:

   WORD : attributes

   FEDC B A 9 876 543 210
   ???? u g s rwx rwx rwx
   --+- | | | -+- -+- +++-> Other rwx bits
	 |  | | |  |   +------> Group rwx bits
	 |  | | |  +----------> Owner rwx bits
	 |  | | +-------------> Sticky bit
	 |  | +---------------> SGID flag
	 |  +-----------------> SUID flag
	 +--------------------> ? (not used in HPACK - set to 0) */

#define UNIX_ATTR_OTHER_X	0x0001
#define UNIX_ATTR_OTHER_W	0x0002
#define UNIX_ATTR_OTHER_R	0x0004
#define UNIX_ATTR_GROUP_X	0x0008
#define UNIX_ATTR_GROUP_W	0x0010
#define UNIX_ATTR_GROUP_R	0x0020
#define UNIX_ATTR_OWNER_X	0x0040
#define UNIX_ATTR_OWNER_W	0x0080
#define UNIX_ATTR_OWNER_R	0x0100
#define UNIX_ATTR_STICKY	0x0200
#define UNIX_ATTR_SGID		0x0400
#define UNIX_ATTR_SUID		0x0800

#define UNIX_ATTR_RBITS		( UNIX_ATTR_OTHER_R | UNIX_ATTR_GROUP_R | UNIX_ATTR_OWNER_R )
#define UNIX_ATTR_WBITS		( UNIX_ATTR_OTHER_W | UNIX_ATTR_GROUP_W | UNIX_ATTR_OWNER_W )
#define UNIX_ATTR_XBITS		( UNIX_ATTR_OTHER_X | UNIX_ATTR_GROUP_X | UNIX_ATTR_OWNER_X )

#define LEN_UNIX_ATTR		2

/* TAG_VMS_ATTR: The VMS permission flag bits, organised as follows:

   WORD : attributes

   FEDC BA98 7654 3210
   RWED RWED RWED RWED
   --+- --+- --+- --+--> World RWED bits
	 |    |    +-------> Group RWED bits
	 |    +------------> Owner RWED bits
	 +-----------------> System RWED bits */

#define VMS_ATTR_WORLD_R	0x0001
#define VMS_ATTR_WORLD_W	0x0002
#define VMS_ATTR_WORLD_E	0x0004
#define VMS_ATTR_WORLD_D	0x0008
#define VMS_ATTR_GROUP_R	0x0010
#define VMS_ATTR_GROUP_W	0x0020
#define VMS_ATTR_GROUP_E	0x0040
#define VMS_ATTR_GROUP_D	0x0080
#define VMS_ATTR_OWNER_R	0x0100
#define VMS_ATTR_OWNER_W	0x0200
#define VMS_ATTR_OWNER_E	0x0400
#define VMS_ATTR_OWNER_D	0x0800
#define VMS_ATTR_SYSTEM_R	0x1000
#define VMS_ATTR_SYSTEM_W	0x2000
#define VMS_ATTR_SYSTEM_E	0x4000
#define VMS_ATTR_SYSTEM_D	0x8000

#define VMS_ATTR_RBITS		( VMS_ATTR_WORLD_R | VMS_ATTR_GROUP_R | VMS_ATTR_OWNER_R | VMS_ATTR_SYSTEM_R )
#define VMS_ATTR_WBITS		( VMS_ATTR_WORLD_W | VMS_ATTR_GROUP_W | VMS_ATTR_OWNER_W | VMS_ATTR_SYSTEM_W )
#define VMS_ATTR_EBITS		( VMS_ATTR_WORLD_E | VMS_ATTR_GROUP_E | VMS_ATTR_OWNER_E | VMS_ATTR_SYSTEM_E )
#define VMS_ATTR_DBITS		( VMS_ATTR_WORLD_D | VMS_ATTR_GROUP_D | VMS_ATTR_OWNER_D | VMS_ATTR_SYSTEM_D )

#define LEN_VMS_ATTR		2

/****************************************************************************
*																			*
*									Time Tags								*
*																			*
****************************************************************************/

/* TAG_ACCESS_TIME, TAG_BACKUP_TIME, TAG_CREATION_TIME, TAG_EXPIRATION_TIME:
   Various timestamps for those systems which store multiple dates for each
   file/directory (the modification date is stored in the file header),
   organised as follows:

   LONG : access time
   LONG : backup time
   LONG : creation time
   LONG : expiration time */

#define LEN_ACCESS_TIME		4
#define LEN_BACKUP_TIME		4
#define LEN_CREATION_TIME	4
#define LEN_EXPIRATION_TIME	4

/****************************************************************************
*                                                                           *
*							System-specific Crufties						*
*																			*
****************************************************************************/

/* TAG_AMIGA_ICON, TAG_OS2_ICON: Tags for icons.  Amiga and OS/2 icons are
   stored as long tags since they are usually quite large.  Amiga icons are
   stored as follows:

   WORD : pixel offset from left of display element
   WORD : pixel offset from top of display element
   WORD : pixel width
   WORD : pixel height
   WORD : number of bitplanes
   BYTE : mask for which bitplanes to use
   BYTE : mask to define what to do with unused bitplanes
   BYTE : Workbench object type (useful only on Amiga)
   WORD[] : icon data, row-first, in bitplane blocks, bits stored flush left.
			Number of words = ( ( width + 15 ) / 16 ) * height * depth
   WORD : crc16

   OS/2 icons are stored in some obscure format whose precise definition is
   stored encrypted with triple Lucifer, fifteen feet underground in a
   lead-lined vault in Armonk with a backup copy in East Fishkill */

/* TAG_LONGNAME: Tags for longer filenames if a shorter version is also
   present (used under the OS/2 HPFS when files must be truncated to 8.3 for
   DOS compatibility), organised as follows:

   BYTE[] : ASCIZ filename

   This tag has a short and long form */

/* TAG_POSITION, TAG_WINSIZE: Tags for window sizes and positions within a
   window, organized as follows:

   WORD : X position in window
   WORD : Y position in window

   WORD : window top coordinate
   WORD : window left coordinate
   WORD : window bottom coordinate
   WORD : window right coordinate */

#define LEN_POSITION		4
#define LEN_WINSIZE			8

/* TAG_VERSION_NUMBER: Version number tag, organised as follows:

   WORD : version number */

#define LEN_VERSION_NUMBER	2

/* TAG_MAC_TYPE, TAG_MAC_CREATOR: Mac file type and creator, organised as
   follows:

   LONG : type

   LONG : creator */

#define LEN_MAC_TYPE		4
#define LEN_MAC_CREATOR		4

/* TAG_RESOURCE_FORK: Mac resource fork, organised as follows:

   BYTE[] : resource fork data */

/* TAG_LOAD_ADDRESS, TAG_EXECUTE_ADDRESS: Load and execute address, used by
   the Archimedes for compatibility with some 6502 software.  Organised as
   follows:

   LONG : load address

   LONG : execute address */

#define LEN_LOAD_ADDRESS	4
#define LEN_EXECUTE_ADDRESS	4

/****************************************************************************
*                                                                           *
*						Security and Data Validation						*
*																			*
****************************************************************************/

/* TAG_ERROR_ID: The error recovery info, organised as follows:

   LONG : error ID
   BYTE[] : file header
   BYTE[] : ASCIZ fileName
   WORD : crc16 */

#define ERROR_ID		"\xFE\xFE\xFE\xFE"	/* Reasonably uncommon pattern */
#define ERROR_ID_SIZE	( sizeof( ERROR_ID ) - 1 )	/* -1 for '\0' */

/* TAG_SECURITY_INFO: Authentication information for a file, stored as an
   encrypted data packet - see HPACKSTD.TXT */

/****************************************************************************
*                                                                           *
*								Miscellaneous								*
*																			*
****************************************************************************/

/* TAG_COMMENT: Comments of 0-63 and 64-4,294,967,296 ASCII chars, organised
   as follows:

   BYTE[] : ASCIZ comment
   WORD : crc16 */
