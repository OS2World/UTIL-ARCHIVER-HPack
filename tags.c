/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  HPACK Archive Tag Handling Code					*
*							TAGS.C  Updated 08/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include "defs.h"
#include "error.h"
#include "hpacklib.h"
#include "system.h"
#include "tags.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
#elif defined( __MAC__ )
  #include "fastio.h"
#else
  #include "io/fastio.h"
#endif /* System-specific include directives */

/****************************************************************************
*																			*
*							Read Tagged Data Routines						*
*																			*
****************************************************************************/

/* Grovel through a tag field looking for tags we can handle */

int readTag( long *tagFieldSize, TAGINFO *tagInfo )
	{
	WORD theTag;
	int tagClass = TAGCLASS_NONE;
	LONG dataLength;

	while( *tagFieldSize > 0 && !tagClass )
		{
		dataLength = 0L;
		theTag = fgetWord();

		/* Extract the tag ID and data length depending on the type of the tag */
		if( theTag >= LONG_BASE )
			{
			/* Get 9-bit tag ID and tag length information */
			tagInfo->tagID = ( ( ( theTag & 1 ) << 8 ) | fgetByte() ) + NO_SHORT_TAGS;
			tagInfo->dataFormat = ( theTag & LONGTAG_FORMAT_MASK ) >> LONGTAG_FORMAT_SHIFT;
			switch( theTag & LONGTAG_LENGTH_MASK )
				{
				case LONGTAG_BYTE_WORD:
					tagInfo->dataLength = tagInfo->uncoprLength = ( LONG ) fgetByte();
					dataLength += LONG_TAGSIZE + sizeof( BYTE );
					if( tagInfo->dataFormat )
						{
						tagInfo->uncoprLength = ( LONG ) fgetWord();
						dataLength += sizeof( WORD );
						}
					break;

				case LONGTAG_WORD_WORD:
					tagInfo->dataLength = tagInfo->uncoprLength = ( LONG ) fgetWord();
					dataLength += LONG_TAGSIZE + sizeof( WORD );
					if( tagInfo->dataFormat )
						{
						tagInfo->uncoprLength = ( LONG ) fgetWord();
						dataLength += sizeof( WORD );
						}
					break;

				case LONGTAG_WORD_LONG:
					tagInfo->dataLength = tagInfo->uncoprLength = ( LONG ) fgetWord();
					dataLength += LONG_TAGSIZE + sizeof( WORD );
					if( tagInfo->dataFormat )
						{
						tagInfo->uncoprLength = fgetLong();
						dataLength += sizeof( LONG );
						}
					break;

				case LONGTAG_LONG_LONG:
					tagInfo->dataLength = tagInfo->uncoprLength = fgetLong();
					dataLength += LONG_TAGSIZE + sizeof( LONG );
					if( tagInfo->dataFormat )
						{
						tagInfo->uncoprLength = fgetLong();
						dataLength += sizeof( LONG );
						}
					break;
				}
			}
		else
			{
			tagInfo->tagID = ( theTag & TAGID_MASK ) >> TAGID_SHIFT;
			tagInfo->dataFormat = TAGFORMAT_STORED;
			tagInfo->dataLength = tagInfo->uncoprLength = theTag & DATALEN_MASK;
			dataLength += SHORT_TAGSIZE;

			/* If it's an alternative short-form of a tag, convert the ID to
			   the usual form */
			if( tagInfo->tagID == TAG_S_LONGNAME )
				tagInfo->tagID = TAG_LONGNAME;
			else
				if( tagInfo->tagID == TAG_S_COMMENT )
					tagInfo->tagID = TAG_COMMENT;
				else
					if( tagInfo->tagID == TAG_S_ERROR )
						tagInfo->tagID = TAG_ERROR;
			}
		*tagFieldSize -= dataLength + tagInfo->dataLength;

		/* If the length field has been corrupted, set the tagID to a non-
		   tag and perform error recovery later on */
		if( *tagFieldSize < 0 )
			tagInfo->tagID = BAD_TAGID;

		/* Now handle each tag.  At this point we only check whether we know
		   what to do with the tag, but don't do any processing.  Any tags
		   which contain useless information (as well as those we don't know
		   about) are skipped */
		switch( tagInfo->tagID )
			{
			case TAG_AMIGA_ATTR:
			case TAG_APPLE_ATTR:
			case TAG_ARC_ATTR:
			case TAG_ATARI_ATTR:
			case TAG_MAC_ATTR:
			case TAG_MSDOS_ATTR:
			case TAG_OS2_ATTR:
			case TAG_UNIX_ATTR:
			case TAG_VMS_ATTR:
				tagClass = TAGCLASS_ATTRIBUTE;
				break;

			case TAG_COMMENT:
				tagClass = TAGCLASS_COMMENT;
				break;

			case TAG_ACCESS_TIME:
			case TAG_BACKUP_TIME:
			case TAG_CREATION_TIME:
			case TAG_EXPIRATION_TIME:
				tagClass = TAGCLASS_TIME;
				break;

			case TAG_AMIGA_ICON:
			case TAG_OS2_ICON:
				tagClass = TAGCLASS_ICON;
				break;

			case TAG_EXECUTE_ADDRESS:
			case TAG_LOAD_ADDRESS:
			case TAG_LONGNAME:
			case TAG_MAC_TYPE:
			case TAG_MAC_CREATOR:
			case TAG_OS2_MISC_EA:
			case TAG_POSITION:
			case TAG_SECURITY_INFO:
			case TAG_RESOURCE_FORK:
			case TAG_VERSION_NUMBER:
			case TAG_WINSIZE:
				tagClass = TAGCLASS_MISC;
				break;

			default:
				/* Unrecognised tag, skip it and look for next one */
				if( *tagFieldSize < 0 )
					{
					/* If the tag length field is corrupted, just skip
					   over whatever data is left */
					skipSeek( *tagFieldSize + tagInfo->dataLength );
					tagInfo->dataLength = tagInfo->uncoprLength = 0L;
					*tagFieldSize = 0;
					}
				else
					skipSeek( tagInfo->dataLength );
				break;
			}
		}

	return( tagClass );
	}

/* Read in attribute data and translate it for the host OS.  Note that
   execute bits are never translated, for obvious reasons */

ATTR readAttributeData( const WORD attributeID )
	{
	WORD wordData;
	BYTE byteData;
	ATTR retVal = 0;

	switch( attributeID )
		{
		case TAG_ATARI_ATTR:
		case TAG_MSDOS_ATTR:
		case TAG_OS2_ATTR:
			byteData = fgetByte();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			retVal = byteData & ~DIRECT;	/* Can't set dir.attr */
#elif defined( __AMIGA__ )
			retVal |= AMIGA_ATTR_READ | AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE;
			if( byteData & MSDOS_ATTR_ARCHIVE )
				retVal |= AMIGA_ATTR_ARCHIVE;
			if( byteData & MSDOS_ATTR_RDONLY )
				retVal ^= AMIGA_ATTR_READ;
			if( byteData & MSDOS_ATTR_HIDDEN )
				retVal |= AMIGA_ATTR_HIDDEN;
#elif defined( __MAC__ )
			if( byteData & MSDOS_ATTR_ARCHIVE )
				retVal |= MAC_ATTR_CHANGED;
			if( byteData & MSDOS_ATTR_SYSTEM )
				retVal |= MAC_ATTR_SYSTEM;
			if( byteData & MSDOS_ATTR_HIDDEN )
				retVal |= MAC_ATTR_INVISIBLE;
			if( byteData & MSDOS_ATTR_RDONLY )
				retVal |= MAC_ATTR_LOCKED;
#elif defined( __UNIX__ )
			if( byteData & MSDOS_ATTR_RDONLY )
				retVal = UNIX_ATTR_RBITS;
			else
				retVal = UNIX_ATTR_RBITS | UNIX_ATTR_WBITS;
#elif defined( __VMS__ )
			if( byteData & MSDOS_ATTR_RDONLY )
				retVal = VMS_ATTR_RBITS;
			else
				retVal = VMS_ATTR_RBITS | VMS_ATTR_DBITS;
#else
			hprintf( "Don't know what to do with MSDOS attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_AMIGA_ATTR:
			byteData = fgetByte();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( !( byteData & AMIGA_ATTR_WRITE ) )
				retVal |= MSDOS_ATTR_RDONLY;
			if( !( byteData & AMIGA_ATTR_ARCHIVE ) )
				retVal |= MSDOS_ATTR_ARCHIVE;
#elif defined( __AMIGA__ )
			retVal = byteData;
#elif defined( __MAC__ )
			if( !( wordData & AMIGA_ATTR_WRITE ) )
				retVal |= MAC_ATTR_LOCKED;
			if( !( wordData & AMIGA_ATTR_ARCHIVE ) )
				retVal |= MAC_ATTR_CHANGED;
#elif defined( __UNIX__ )
			if( byteData & AMIGA_ATTR_READ )
				retVal |= UNIX_ATTR_RBITS;
			if( byteData & AMIGA_ATTR_WRITE )
				retVal |= UNIX_ATTR_WBITS;
#elif defined( __VMS__ )
			if( byteData & AMIGA_ATTR_READ )
				retVal |= VMS_ATTR_RBITS;
			if( byteData & AMIGA_ATTR_WRITE )
				retVal |= VMS_ATTR_WBITS;
#else
			hprintf( "Don't know what to do with Amiga attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_APPLE_ATTR:
			wordData = fgetWord();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( !( wordData & APPLE_ATTR_WRITE ) )
				retVal |= MSDOS_ATTR_RDONLY;
			if( wordData & APPLE_ATTR_BACKUP )
				retVal |= MSDOS_ATTR_ARCHIVE;
#elif defined( __AMIGA__ )
			if( wordData & APPLE_ATTR_READ )
				retVal |= AMIGA_ATTR_READ;
			if( wordData & APPLE_ATTR_WRITE )
				retVal |= AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE;
			if( wordData & APPLE_ATTR_BACKUP )
				retVal |= AMIGA_ATTR_ARCHIVE;
#elif defined( __UNIX__ )
			if( wordData & APPLE_ATTR_READ )
				retVal |= UNIX_ATTR_RBITS;
			if( wordData & APPLE_ATTR_WRITE )
				retVal |= UNIX_ATTR_WBITS;
#elif defined( __MAC__ )
			if( !( wordData & APPLE_ATTR_WRITE ) )
				retVal |= MAC_ATTR_LOCKED;
			if( wordData & APPLE_ATTR_BACKUP )
				retVal |= MAC_ATTR_CHANGED;
#elif defined( __UNIX__ )
			if( wordData & APPLE_ATTR_READ )
				retVal |= VMS_ATTR_RBITS;
			if( wordData & APPLE_ATTR_WRITE )
				retVal |= VMS_ATTR_WBITS;
#else
			hprintf( "Don't know what to do with IIgs attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_ARC_ATTR:
			byteData = fgetByte();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( byteData & ARC_ATTR_LOCKED )
				retVal |= MSDOS_ATTR_RDONLY;
#elif defined( __AMIGA__ )
			if( byteData & ARC_ATTR_READ_OWNER )
				retVal |= AMIGA_ATTR_READ;
			if( byteData & ARC_ATTR_WRITE_OWNER )
				retVal |= AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE;
#elif defined( __MAC__ )
			if( byteData & ARC_ATTR_LOCKED )
				retVal |= MAC_ATTR_LOCKED;
#elif defined( __UNIX__ )
			if( byteData & ARC_ATTR_READ_OWNER )
				retVal |= UNIX_ATTR_OWNER_R;
			if( byteData & ARC_ATTR_WRITE_OWNER )
				retVal |= UNIX_ATTR_OWNER_W;
			if( byteData & ARC_ATTR_READ_OTHER )
				retVal |= UNIX_ATTR_GROUP_R | UNIX_ATTR_OTHER_W;
			if( byteData & ARC_ATTR_WRITE_OTHER )
				retVal |= UNIX_ATTR_GROUP_W | UNIX_ATTR_OTHER_W;
#elif defined( __VMS__ )
			if( byteData & ARC_ATTR_READ_OWNER )
				retVal |= VMS_ATTR_SYSTEM_R | VMS_ATTR_OWNER_R;
			if( byteData & ARC_ATTR_WRITE_OWNER )
				retVal |= VMS_ATTR_SYSTEM_W | VMS_ATTR_OWNER_W;
			if( byteData & ARC_ATTR_READ_OTHER )
				retVal |= VMS_ATTR_GROUP_R | VMS_ATTR_WORLD_R;
			if( byteData & ARC_ATTR_WRITE_OTHER )
				retVal |= VMS_ATTR_GROUP_W | VMS_ATTR_WORLD_W;
#else
			hprintf( "Don't know what to do with Archimedes attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_MAC_ATTR:
			wordData = fgetWord();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( wordData & MAC_ATTR_CHANGED )
				retVal |= MSDOS_ATTR_ARCHIVE;
			if( wordData & MAC_ATTR_SYSTEM )
				retVal |= MSDOS_ATTR_SYSTEM;
			if( wordData & MAC_ATTR_INVISIBLE )
				retVal |= MSDOS_ATTR_HIDDEN;
			if( wordData & MAC_ATTR_LOCKED )
				retVal |= MSDOS_ATTR_RDONLY;
#elif defined( __AMIGA__ )
			retVal |= AMIGA_ATTR_READ | AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE;
			if( wordData & MAC_ATTR_CHANGED )
				retVal |= AMIGA_ATTR_ARCHIVE;
			if( wordData & MAC_ATTR_INVISIBLE )
				retVal |= AMIGA_ATTR_HIDDEN;
			if( wordData & MAC_ATTR_LOCKED )
				retVal ^= AMIGA_ATTR_WRITE;
#elif defined( __MAC__ )
			retVal = wordData;
#elif defined( __UNIX__ )
			if( wordData & MAC_ATTR_LOCKED )
				retVal = UNIX_ATTR_RBITS;
			else
				retVal = UNIX_ATTR_RBITS | UNIX_ATTR_WBITS;
#elif defined( __VMS__ )
			if( wordData & MAC_ATTR_LOCKED )
				retVal = VMS_ATTR_RBITS;
			else
				retVal = VMS_ATTR_RBITS | VMS_ATTR_DBITS;
#else
			hprintf( "Don't know what to do with Mac attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_UNIX_ATTR:
			wordData = fgetWord();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( !( wordData & UNIX_ATTR_OWNER_W ) )
				retVal |= MSDOS_ATTR_RDONLY;
#elif defined( __AMIGA__ )
			retVal |= AMIGA_ATTR_READ | AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE;
			if( !( wordData & UNIX_ATTR_OWNER_R ) )
				retVal ^= AMIGA_ATTR_READ;
			if( !( wordData & UNIX_ATTR_OWNER_W ) )
				retVal ^= ( AMIGA_ATTR_WRITE | AMIGA_ATTR_DELETE );
#elif defined( __MAC__ )
			if( !( wordData & UNIX_ATTR_OWNER_W ) )
				retVal |= MAC_ATTR_LOCKED;
#elif defined( __UNIX__ )
			retVal = wordData;
#elif defined( __VMS__ )
			if( wordData & UNIX_ATTR_OTHER_R )
				retVal |= VMS_ATTR_WORLD_R;
			if( wordData & UNIX_ATTR_OTHER_W )
				retVal |= VMS_ATTR_WORLD_W | VMS_ATTR_WORLD_D;
			if( wordData & UNIX_ATTR_GROUP_R )
				retVal |= VMS_ATTR_GROUP_R;
			if( wordData & UNIX_ATTR_GROUP_W )
				retVal |= VMS_ATTR_GROUP_W | VMS_ATTR_GROUP_D;
			if( wordData & UNIX_ATTR_OWNER_R )
				retVal |= VMS_ATTR_OWNER_R;
			if( wordData & UNIX_ATTR_OWNER_W )
				retVal |= VMS_ATTR_OWNER_W | VMS_ATTR_OWNER_D;
#else
			hprintf( "Don't know what to do with Unix attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
			break;

		case TAG_VMS_ATTR:
			wordData = fgetWord();
#if defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ ) || defined( __OS2__ )
			if( !( wordData & VMS_ATTR_OWNER_W ) )
				retVal |= MSDOS_ATTR_RDONLY;
#elif defined( __AMIGA__ )
			if( wordData & VMS_ATTR_OWNER_R )
				retVal |= AMIGA_ATTR_READ;
			if( wordData & VMS_ATTR_OWNER_W )
				retVal |= AMIGA_ATTR_WRITE;
			if( wordData & VMS_ATTR_OWNER_D )
				retVal |= AMIGA_ATTR_DELETE;
#elif defined( __MAC__ )
			if( !( wordData & VMS_ATTR_OWNER_W ) )
				retVal |= MAC_ATTR_LOCKED;
#elif defined( __UNIX__ )
			if( wordData & VMS_ATTR_WORLD_R )
				retVal |= UNIX_ATTR_OTHER_R;
			if( wordData & ( VMS_ATTR_WORLD_W | VMS_ATTR_WORLD_D ) )
				retVal |= UNIX_ATTR_OTHER_W;
			if( wordData & VMS_ATTR_GROUP_R )
				retVal |= UNIX_ATTR_GROUP_R;
			if( wordData & ( VMS_ATTR_GROUP_W | VMS_ATTR_GROUP_D ) )
				retVal |= UNIX_ATTR_GROUP_W;
			if( wordData & VMS_ATTR_OWNER_R )
				retVal |= UNIX_ATTR_OWNER_R;
			if( wordData & ( VMS_ATTR_OWNER_W | VMS_ATTR_OWNER_D ) )
				retVal |= UNIX_ATTR_OWNER_W;
#elif defined( __VMS__ )
			retVal = wordData;
#else
			hprintf( "Don't know what to do with VMS attributes, TAGS.C line %d\n", __LINE__ );
#endif /* Various OS-specific defines */
		}

	return( retVal );
	}

/****************************************************************************
*																			*
*							Write Tagged Data Routines						*
*																			*
****************************************************************************/

/* Write a long tag */

int writeTag( WORD tagID, const WORD store, const LONG dataLength, const LONG uncoprLength )
	{
	WORD tagLength = ( dataLength <= 0xFF ) ? LONGTAG_BYTE_WORD :
					 ( dataLength > 0xFFFF ) ? LONGTAG_LONG_LONG :
					 ( uncoprLength <= 0xFFFF ) ? LONGTAG_WORD_WORD :
					 LONGTAG_WORD_LONG;
	int headerLen = sizeof( WORD ) + sizeof( BYTE );
	BOOLEAN isShortForm = ( store == TAGFORMAT_STORED ) &&
						  ( dataLength < SHORT_TAGLEN ) &&
						  ( tagID == TAG_LONGNAME || tagID == TAG_COMMENT ||
							tagID == TAG_ERROR );

	/* Determine whether we can write a short tag or not.  We can do this
	   either if the tagID is a short tagID, or if there is a short form of
	   the tag and it fulfils the requirements for a short tag */
	if( isShortTag( tagID ) || isShortForm )
		{
		/* If it's a short form tag, convert the ID to the short-form equivalent */
		if( isShortForm )
			{
			if( tagID == TAG_LONGNAME )
				tagID = TAG_S_LONGNAME;
			else
				if( tagID == TAG_COMMENT )
					tagID = TAG_S_COMMENT;
				else
					tagID = TAG_S_ERROR;
			}

		fputWord( ( WORD ) ( ( tagID << TAGID_SHIFT ) | dataLength ) );
		return( sizeof( WORD ) );
		}

	/* Write long tag ID, storage class, and 9-bit actual tag ID */
	tagID -= NO_SHORT_TAGS;			/* Convert to base value of zero */
	fputWord( ( WORD ) ( LONG_BASE | tagLength | ( store << LONGTAG_FORMAT_SHIFT ) | ( tagID >> 8 ) ) );
	fputByte( ( BYTE ) tagID );
	switch( tagLength )
		{
		case LONGTAG_BYTE_WORD:
			fputByte( ( BYTE ) dataLength );
			headerLen += sizeof( BYTE );
			if( store )
				{
				fputWord( ( WORD ) uncoprLength );
				headerLen += sizeof( WORD );
				}
			break;

		case LONGTAG_WORD_WORD:
			fputWord( ( WORD ) dataLength );
			headerLen += sizeof( WORD );
			if( store )
				{
				fputWord( ( WORD ) uncoprLength );
				headerLen += sizeof( WORD );
				}
			break;

		case LONGTAG_WORD_LONG:
			fputWord( ( WORD ) dataLength );
			headerLen += sizeof( WORD );
			if( store )
				{
				fputLong( uncoprLength );
				headerLen += sizeof( LONG );
				}
			break;

		case LONGTAG_LONG_LONG:
			fputLong( dataLength );
			headerLen += sizeof( LONG );
			if( store )
				{
				fputLong( uncoprLength );
				headerLen += sizeof( LONG );
				}
			break;
		}

	return( headerLen );
	}

#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) )	/* Save a bit of code size */

int writeDirTag( WORD tagID, const WORD store, const LONG dataLength, const LONG uncoprLength )
	{
	WORD tagLength = ( dataLength <= 0xFF ) ? LONGTAG_BYTE_WORD :
					 ( dataLength > 0xFFFF ) ? LONGTAG_LONG_LONG :
					 ( uncoprLength <= 0xFFFF ) ? LONGTAG_WORD_WORD :
					 LONGTAG_WORD_LONG;
	int headerLen = sizeof( WORD ) + sizeof( BYTE );
	BOOLEAN isShortForm = ( store == TAGFORMAT_STORED ) &&
						  ( dataLength < SHORT_TAGLEN ) &&
						  ( tagID == TAG_LONGNAME || tagID == TAG_COMMENT ||
							tagID == TAG_ERROR );

	/* Determine whether we can write a short tag or not.  We can do this
	   either if the tagID is a short tagID, or if there is a short form of
	   the tag and it fulfils the requirements for a short tag */
	if( isShortTag( tagID ) || isShortForm )
		{
		/* If it's a short form tag, convert the ID to the short-form equivalent */
		if( isShortForm )
			{
			if( tagID == TAG_LONGNAME )
				tagID = TAG_S_LONGNAME;
			else
				if( tagID == TAG_COMMENT )
					tagID = TAG_S_COMMENT;
				else
					tagID = TAG_S_ERROR;
			}

		fputDirWord( ( WORD ) ( ( tagID << TAGID_SHIFT ) | dataLength ) );
		return( sizeof( WORD ) );
		}

	/* Write long tag ID, storage class, and 9-bit actual tag ID */
	tagID -= NO_SHORT_TAGS;			/* Convert to base value of zero */
	fputDirWord( ( WORD ) ( LONG_BASE | tagLength | ( store << LONGTAG_FORMAT_SHIFT ) | ( tagID >> 8 ) ) );
	fputDirByte( ( BYTE ) tagID );
	switch( tagLength )
		{
		case LONGTAG_BYTE_WORD:
			fputDirByte( ( BYTE ) dataLength );
			headerLen += sizeof( BYTE );
			if( store )
				{
				fputDirWord( ( WORD ) uncoprLength );
				headerLen += sizeof( WORD );
				}
			break;

		case LONGTAG_WORD_WORD:
			fputDirWord( ( WORD ) dataLength );
			headerLen += sizeof( WORD );
			if( store )
				{
				fputDirWord( ( WORD ) uncoprLength );
				headerLen += sizeof( WORD );
				}
			break;

		case LONGTAG_WORD_LONG:
			fputDirWord( ( WORD ) dataLength );
			headerLen += sizeof( WORD );
			if( store )
				{
				fputDirLong( uncoprLength );
				headerLen += sizeof( LONG );
				}
			break;

		case LONGTAG_LONG_LONG:
			fputDirLong( dataLength );
			headerLen += sizeof( LONG );
			if( store )
				{
				fputDirLong( uncoprLength );
				headerLen += sizeof( LONG );
				}
			break;
		}

	return( headerLen );
	}

#else

int writeDirTag( WORD tagID, const WORD store, const LONG dataLength, const LONG uncoprLength )
	{
	/* Minimal writeTag() routine - MSDOS doesn't need all this code */
	fputDirWord( ( tagID << TAGID_SHIFT ) | ( WORD ) dataLength );
	return( sizeof( WORD ) );
#pragma warn -par
	}
#pragma warn +par
#endif /* __MSDOS16__ || __MSDOS32__ */

/* The following routines are used to write variable-length tagged fields to
   the archive or directory information files.  Only some versions of HPACK
   will need them */

#if 0

static BYTE *tempBuffer;	/* Kludge for now */
extern BYTE *tempBuffer;

/* Try and compress tagged data using the auxiliary compressor */

static WORD compressTag( const BYTE *inBuffer, const BYTE *outBuffer, WORD count )
	{
	/* Get rid of "Unused parameter" warning */
	if( inBuffer == outBuffer );

	/* Do nothing at the moment */
	return( count );
	}

/* Write a tag to the archive */

WORD writeTag( WORD tagID, WORD tagLength, BYTE *dataBuffer )
	{
	WORD compressedTagLength, i = 0;

	/* Write the tag itself as either a short or a long tag */
	if( tagLength < SHORT_TAGLEN )
		fputWord( tagID | tagLength );
	else
		{
		/* Try and compress the tag data */
		if( ( compressedTagLength = compressTag( dataBuffer, tempBuffer, tagLength ) ) >
			tagLength - sizeof( WORD ) )
			{
			/* Data couldn't be compressed, stored in uncompressed form */
			fputWord( LONG_TAG( TAGFORMAT_STORED ) );
			fputByte( ( BYTE ) tagID );
			fputWord( tagLength );
			}
		else
			{
			/* Data was compressed, save uncompressed length at start of data */
			fputWord( LONG_TAG( TAGFORMAT_PACKED ) );
			fputByte( ( BYTE ) tagID );
			fputWord( compressedTagLength );
			fputWord( tagLength );

			/* Process the compressed data not the original data */
			tagLength = compressedTagLength;
			dataBuffer = tempBuffer;
			}
		}

	/* Get the tag length if it has an implicit length */
	if( !tagLength )
		tagLength = extractTagLen( tagID );

	/* Write the data associated with the tag */
	while( i < tagLength )
		fputByte( dataBuffer[ i++ ] );

	return( TAGSIZE + tagLength );
	}

WORD writeDirTag( WORD tagID, WORD tagLength, BYTE *dataBuffer )
	{
	int i = 0;

	/* Write the tag itself */
	tagID |= tagLength;
	fputDirWord( tagID );

	/* Get the tag length if it has an implicit length */
	if( !tagLength )
		tagLength = extractTagLen( tagID );

	/* Write the data associated with the tag */
	while( i < tagLength )
		fputDirByte( dataBuffer[ i++ ] );

	return( TAGSIZE + tagLength );
	}

#endif /* 0 */
