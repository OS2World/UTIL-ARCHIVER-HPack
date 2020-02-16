/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						ASN.1 Complex Types Handling Routines				*
*							ASN1CPLX.C  Updated 20/03/93					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#if defined( __ATARI__ )
  #include "asn1\asn1.h"
  #include "asn1\asn1cplx.h"
  #include "asn1\ber.h"
  #include "io\stream.h"
#elif defined( __MAC__ )
  #include "asn1.h"
  #include "asn1cplx.h"
  #include "ber.h"
  #include "stream.h"
#else
  #include "asn1/asn1.h"
  #include "asn1/asn1cplx.h"
  #include "asn1/ber.h"
  #include "io/stream.h"
#endif /* System-specific include directives */

/****************************************************************************
*																			*
*							DirectoryEntry Type Routines					*
*																			*
****************************************************************************/

/* Initialise a DirectoryEntry type, and destroy it afterwards */

void newDirectoryEntry( DIR_ENTRY *directoryEntry )
	{
	/* Initialise all fields */
	newGeneralizedString( &directoryEntry->fileName, NULL, 0, 0 );
	newInteger( &directoryEntry->linkID, 0 );
	newInteger( &directoryEntry->internalType, 0 );
	newTime( &directoryEntry->modificationTime, 0, 0 );
	newInteger( &directoryEntry->fileSize, 0 );
	}

void deleteDirectoryEntry( DIR_ENTRY *directoryEntry )
	{
	/* Clear all fields */
	deleteGeneralizedString( &directoryEntry->fileName );
	deleteInteger( &directoryEntry->linkID );
	deleteInteger( &directoryEntry->internalType );
	deleteTime( &directoryEntry->modificationTime );
	deleteInteger( &directoryEntry->fileSize );
	}

/* Determine the encoded size of a file header */

int sizeofDirectoryEntry( DIR_ENTRY *directoryEntry )
	{
	int headerSize;

	/* Calculate size of file identification fields */
	headerSize = sizeofGeneralizedString( &directoryEntry->fileName );
	if( evalInteger( &directoryEntry->linkID ) )
		headerSize += sizeofInteger( &directoryEntry->linkID );
	if( evalInteger( &directoryEntry->internalType ) )
		headerSize += sizeofInteger( &directoryEntry->internalType );

	/* Calculate size of timestamp field */
	if( evalTime( &directoryEntry->modificationTime ) )
		headerSize += sizeofTime( &directoryEntry->modificationTime );

	/* Calculate size of data size field */
	headerSize += sizeofInteger( &directoryEntry->fileSize );

	return( headerSize );
	}

/* Write a directory entry to disk */

void writeDirectoryEntry( STREAM *stream, DIR_ENTRY *directoryEntry )
	{
	/* Write file identification fields */
	writeGeneralizedString( stream, &directoryEntry->fileName, DEFAULT_TAG );
	if( evalInteger( &directoryEntry->linkID ) )
		writeInteger( stream, &directoryEntry->linkID, 0 );
	if( evalInteger( &directoryEntry->internalType ) )
		writeInteger( stream, &directoryEntry->internalType, 1 );

	/* Write optional datestamp information */
	if( evalTime( &directoryEntry->modificationTime ) )
		writeTime( stream, &directoryEntry->modificationTime, DEFAULT_TAG );

	/* Write data size field */
	writeInteger( stream, &directoryEntry->fileSize, DEFAULT_TAG );
	}

/* Read a file header from disk */

void readDirectoryEntry( STREAM *stream, DIR_ENTRY *directoryEntry )
	{
	/* Read file identification fields */
	readGeneralizedString( stream, &directoryEntry->fileName );

	/* Read data size field */
	readInteger( stream, &directoryEntry->fileSize );
	}

/****************************************************************************
*																			*
*							FileProperty Type Routines						*
*																			*
****************************************************************************/

/* The tags for the fields in a FileProperty record. These correspond
   directly to the tags in the ASN.1 definition */

enum { FP_TAG_FILESIZEASSTORED, FP_TAG_FILEFUTURESIZE, FP_TAG_DIRECTORYID,
	   FP_TAG_LINKID, FP_TAG_INTERNALTYPE, FP_TAG_VERSION, FP_TAG_EXTRAINFO,
	   FP_TAG_CREATIONTIME, FP_TAG_MODIFICATIONTIME, FP_TAG_READACCESSTIME,
	   FP_TAG_ATTRIBUTEMODTIME, FP_TAG_EFFECTIVETIME, FP_TAG_EXPIRYTIME,
	   FP_TAG_BACKUPTIME, FP_TAG_OWNERPERMITTEDACTIONS,
	   FP_TAG_GROUPPERMITTEDACTIONS, FP_TAG_WORLDPERMITTEDACTIONS,
	   FP_TAG_SYSTEMPERMITTEDACTIONS, FP_TAG_CREATORIDENTITY,
	   FP_TAG_MODIFIERIDENTITY, FP_TAG_READERIDENTITY,
	   FP_TAG_ATTRIBUTEMODEIDENTITY };

/* Initialise a FileProperty type, and destroy it afterwards */

void newFileProperty( FILEHEADER *fileProperty )
	{
	/* Initialise all fields */
	newGeneralizedString( &fileProperty->fileName, NULL, 0, 0 );
	newInteger( &fileProperty->fileSize, 0 );
	newInteger( &fileProperty->fileSizeAsStored, 0 );
	newInteger( &fileProperty->fileFutureSize, 0 );
	newInteger( &fileProperty->directoryID, 0 );
	newInteger( &fileProperty->linkID, 0 );
	newInteger( &fileProperty->internalType, 0 );
	newInteger( &fileProperty->version, 0 );
	newOctetString( &fileProperty->miscInfo, NULL, 0 );
	newTime( &fileProperty->creationTime, 0, 0 );
	newTime( &fileProperty->modificationTime, 0, 0 );
	newTime( &fileProperty->readAccessTime, 0, 0 );
	newTime( &fileProperty->attributeModTime, 0, 0 );
	newTime( &fileProperty->backupTime, 0, 0 );
	newTime( &fileProperty->effectiveTime, 0, 0 );
	newTime( &fileProperty->expiryTime, 0, 0 );
	newPermittedActions( &fileProperty->owner, ACTION_NONE );
	newPermittedActions( &fileProperty->group, ACTION_NONE );
	newPermittedActions( &fileProperty->world, ACTION_NONE );
	newPermittedActions( &fileProperty->system, ACTION_NONE );
	newAccountingInfo( &fileProperty->creatorID, ACCOUNTINFO_NONE );
	newAccountingInfo( &fileProperty->modifierID, ACCOUNTINFO_NONE );
	newAccountingInfo( &fileProperty->readerID, ACCOUNTINFO_NONE );
	newAccountingInfo( &fileProperty->attributeModID, ACCOUNTINFO_NONE );
	}

void deleteFileProperty( FILEHEADER *fileProperty )
	{
	/* Clear all fields */
	deleteGeneralizedString( &fileProperty->fileName );
	deleteInteger( &fileProperty->fileSize );
	deleteInteger( &fileProperty->fileSizeAsStored );
	deleteInteger( &fileProperty->fileFutureSize );
	deleteInteger( &fileProperty->directoryID );
	deleteInteger( &fileProperty->linkID );
	deleteInteger( &fileProperty->internalType );
	deleteInteger( &fileProperty->version );
	deleteOctetString( &fileProperty->miscInfo );
	deleteTime( &fileProperty->creationTime );
	deleteTime( &fileProperty->modificationTime );
	deleteTime( &fileProperty->readAccessTime );
	deleteTime( &fileProperty->attributeModTime );
	deleteTime( &fileProperty->backupTime );
	deleteTime( &fileProperty->effectiveTime );
	deleteTime( &fileProperty->expiryTime );
	deletePermittedActions( &fileProperty->owner );
	deletePermittedActions( &fileProperty->group );
	deletePermittedActions( &fileProperty->world );
	deletePermittedActions( &fileProperty->system );
	deleteAccountingInfo( &fileProperty->creatorID );
	deleteAccountingInfo( &fileProperty->modifierID );
	deleteAccountingInfo( &fileProperty->readerID );
	deleteAccountingInfo( &fileProperty->attributeModID );
	}

/* Determine the encoded size of a file header */

int sizeofFileProperty( FILEHEADER *fileProperty )
	{
	int headerSize;

	/* Calculate size of fixed fileName and size fields */
	headerSize = sizeofGeneralizedString( &fileProperty->fileName );
	headerSize += sizeofInteger( &fileProperty->fileSize );

	/* Calculate size of optional size fields */
	if( evalInteger( &fileProperty->fileSizeAsStored ) )
		headerSize += sizeofInteger( &fileProperty->fileSizeAsStored );
	if( evalInteger( &fileProperty->fileFutureSize ) )
		headerSize += sizeofInteger( &fileProperty->fileFutureSize );

	/* Calculate size of optional bookkeeping information fields */
	if( evalInteger( &fileProperty->directoryID ) )
		headerSize += sizeofInteger( &fileProperty->directoryID );
	if( evalInteger( &fileProperty->linkID ) )
		headerSize += sizeofInteger( &fileProperty->linkID );
	if( evalInteger( &fileProperty->internalType ) )
		headerSize += sizeofInteger( &fileProperty->internalType );
	if( evalInteger( &fileProperty->version ) )
		headerSize += sizeofInteger( &fileProperty->version );

	/* Calculate size of optional mission-critical information */
	if( evalOctetString( &fileProperty->miscInfo ) )
		headerSize += sizeofOctetString( &fileProperty->miscInfo );

	/* Calculate size of optional datestamp information */
	if( evalTime( &fileProperty->creationTime ) )
		headerSize += sizeofTime( &fileProperty->creationTime );
	if( evalTime( &fileProperty->modificationTime ) )
		headerSize += sizeofTime( &fileProperty->modificationTime );
	if( evalTime( &fileProperty->readAccessTime ) )
		headerSize += sizeofTime( &fileProperty->readAccessTime );
	if( evalTime( &fileProperty->attributeModTime ) )
		headerSize += sizeofTime( &fileProperty->attributeModTime );
	if( evalTime( &fileProperty->backupTime ) )
		headerSize += sizeofTime( &fileProperty->backupTime );
	if( evalTime( &fileProperty->effectiveTime ) )
		headerSize += sizeofTime( &fileProperty->effectiveTime );
	if( evalTime( &fileProperty->expiryTime ) )
		headerSize += sizeofTime( &fileProperty->expiryTime );

	/* Calculate size of optional permitted-actions information */
	if( evalPermittedActions( &fileProperty->owner ) )
		headerSize += sizeofPermittedActions( &fileProperty->owner );
	if( evalPermittedActions( &fileProperty->group ) )
		headerSize += sizeofPermittedActions( &fileProperty->group );
	if( evalPermittedActions( &fileProperty->world ) )
		headerSize += sizeofPermittedActions( &fileProperty->world );
	if( evalPermittedActions( &fileProperty->system ) )
		headerSize += sizeofPermittedActions( &fileProperty->system );

	/* Calculate size of optional accounting information */
	if( evalAccountingInfo( &fileProperty->creatorID ) )
		headerSize += sizeofAccountingInfo( &fileProperty->creatorID );
	if( evalAccountingInfo( &fileProperty->modifierID ) )
		headerSize += sizeofAccountingInfo( &fileProperty->modifierID );
	if( evalAccountingInfo( &fileProperty->readerID ) )
		headerSize += sizeofAccountingInfo( &fileProperty->readerID );
	if( evalAccountingInfo( &fileProperty->attributeModID ) )
		headerSize += sizeofAccountingInfo( &fileProperty->attributeModID );

	return( headerSize );
	}

/* Write a file header to disk */

void writeFileProperty( STREAM *stream, FILEHEADER *fileProperty )
	{
	/* Write file property tag */
	writeIdentifier( stream, BER_APPLICATION, BER_CONSTRUCTED, BER_APPL_FILE_PROPERTY );
	writeLength( stream, sizeofFileProperty( fileProperty ) );

	/* Write fixed fileName and size fields */
	writeGeneralizedString( stream, &fileProperty->fileName, DEFAULT_TAG );
	writeInteger( stream, &fileProperty->fileSize, DEFAULT_TAG );

	/* Write optional size fields */
	if( evalInteger( &fileProperty->fileSizeAsStored ) )
		writeInteger( stream, &fileProperty->fileSizeAsStored,
					  FP_TAG_FILESIZEASSTORED );
	if( evalInteger( &fileProperty->fileFutureSize ) )
		writeInteger( stream, &fileProperty->fileFutureSize,
					  FP_TAG_FILEFUTURESIZE );

	/* Write optional bookkeeping information fields */
	if( evalInteger( &fileProperty->directoryID ) )
		writeInteger( stream, &fileProperty->directoryID, FP_TAG_DIRECTORYID );
	if( evalInteger( &fileProperty->linkID ) )
		writeInteger( stream, &fileProperty->linkID, FP_TAG_LINKID );
	if( evalInteger( &fileProperty->internalType ) )
		writeInteger( stream, &fileProperty->internalType,
					  FP_TAG_INTERNALTYPE );
	if( evalInteger( &fileProperty->version ) )
		writeInteger( stream, &fileProperty->version, FP_TAG_VERSION );

	/* Write optional mission-critical information */
	if( evalOctetString( &fileProperty->miscInfo ) )
		writeOctetString( stream, &fileProperty->miscInfo, FP_TAG_EXTRAINFO );

	/* Write optional datestamp information */
	if( evalTime( &fileProperty->creationTime ) )
		writeTime( stream, &fileProperty->creationTime,
				   FP_TAG_CREATIONTIME );
	if( evalTime( &fileProperty->modificationTime ) )
		writeTime( stream, &fileProperty->modificationTime,
				   FP_TAG_MODIFICATIONTIME );
	if( evalTime( &fileProperty->readAccessTime ) )
		writeTime( stream, &fileProperty->readAccessTime,
				   FP_TAG_READACCESSTIME );
	if( evalTime( &fileProperty->attributeModTime ) )
		writeTime( stream, &fileProperty->attributeModTime,
				   FP_TAG_ATTRIBUTEMODTIME );
	if( evalTime( &fileProperty->backupTime ) )
		writeTime( stream, &fileProperty->backupTime,
				   FP_TAG_EFFECTIVETIME );
	if( evalTime( &fileProperty->effectiveTime ) )
		writeTime( stream, &fileProperty->effectiveTime,
				   FP_TAG_EXPIRYTIME );
	if( evalTime( &fileProperty->expiryTime ) )
		writeTime( stream, &fileProperty->expiryTime,
				   FP_TAG_BACKUPTIME );

	/* Write optional permitted-actions information */
	if( evalPermittedActions( &fileProperty->owner ) )
		writePermittedActions( stream, &fileProperty->owner,
							   FP_TAG_OWNERPERMITTEDACTIONS );
	if( evalPermittedActions( &fileProperty->group ) )
		writePermittedActions( stream, &fileProperty->group,
							   FP_TAG_GROUPPERMITTEDACTIONS );
	if( evalPermittedActions( &fileProperty->world ) )
		writePermittedActions( stream, &fileProperty->world,
							   FP_TAG_WORLDPERMITTEDACTIONS );
	if( evalPermittedActions( &fileProperty->system ) )
		writePermittedActions( stream, &fileProperty->system,
							   FP_TAG_SYSTEMPERMITTEDACTIONS );
	}

/* Read a file header from disk */

int readFileProperty( STREAM *stream, FILEHEADER *fileProperty )
	{
	BER_TAGINFO filePropertyInfo, tagInfo;
	int length;

	/* Read information on the file property structure */
	readIdentifier( stream, &filePropertyInfo );
	if( filePropertyInfo.class != BER_APPLICATION ||
		!filePropertyInfo.constructed ||
		filePropertyInfo.identifier != BER_APPL_FILE_PROPERTY )
		{
		/* Unknown type, skip it */
		readUniversalData( stream );
		return( ERROR );
		}
	readLength( stream, &length );

	/* Read fixed fileName and size fields */
	length -= readGeneralizedString( stream, &fileProperty->fileName );
	length -= readInteger( stream, &fileProperty->fileSize );

	/* Read in any remaining fields */
	while( length )
		{
		length -= readIdentifier( stream, &tagInfo );
		switch( tagInfo.identifier )
			{
			/* Read optional size fields */
			case FP_TAG_FILESIZEASSTORED:
				length -= readIntegerData( stream, &fileProperty->fileSizeAsStored );
				break;
			case FP_TAG_FILEFUTURESIZE:
				length -= readIntegerData( stream, &fileProperty->fileFutureSize );
				break;

			/* Read optional bookkeeping information fields */
			case FP_TAG_DIRECTORYID:
				length -= readIntegerData( stream, &fileProperty->directoryID );
				break;
			case FP_TAG_LINKID:
				length -= readIntegerData( stream, &fileProperty->linkID );
				break;
			case FP_TAG_INTERNALTYPE:
				length -= readIntegerData( stream, &fileProperty->internalType );
				break;
			case FP_TAG_VERSION:
				length -= readIntegerData( stream, &fileProperty->version );
				break;

			/* Read optional mission-critical information */
			case FP_TAG_EXTRAINFO:
				length -= readOctetStringData( stream, &fileProperty->miscInfo );
				break;

			/* Read optional datestamp information */
			case FP_TAG_CREATIONTIME:
				length -= readTimeData( stream, &fileProperty->creationTime );
				break;
			case FP_TAG_MODIFICATIONTIME:
				length -= readTimeData( stream, &fileProperty->modificationTime );
				break;
			case FP_TAG_READACCESSTIME:
				length -= readTimeData( stream, &fileProperty->readAccessTime );
				break;
			case FP_TAG_ATTRIBUTEMODTIME:
				length -= readTimeData( stream, &fileProperty->attributeModTime );
				break;
			case FP_TAG_EFFECTIVETIME:
				length -= readTimeData( stream, &fileProperty->backupTime );
				break;
			case FP_TAG_EXPIRYTIME:
				length -= readTimeData( stream, &fileProperty->effectiveTime );
				break;
			case FP_TAG_BACKUPTIME:
				length -= readTimeData( stream, &fileProperty->expiryTime );
				break;

			/* Read optional permitted-actions information */
			case FP_TAG_OWNERPERMITTEDACTIONS:
				length -= readPermittedActionsData( stream, &fileProperty->owner );
				break;
			case FP_TAG_GROUPPERMITTEDACTIONS:
				length -= readPermittedActionsData( stream, &fileProperty->group );
				break;
			case FP_TAG_WORLDPERMITTEDACTIONS:
				length -= readPermittedActionsData( stream, &fileProperty->world );
				break;
			case FP_TAG_SYSTEMPERMITTEDACTIONS:
				length -= readPermittedActionsData( stream, &fileProperty->system );
				break;

			/* Unknown field, skip it */
			default:
				length -= readUniversalData( stream );
			}
		}

	return( OK );
	}

/****************************************************************************
*																			*
*							DataHeader Type Routines						*
*																			*
****************************************************************************/

/* The tags for the fields in a DataHeader record. These correspond
   directly to the tags in the ASN.1 definition */

enum { DH_TAG_STORAGETYPE, DH_TAG_STORAGEPARAM, DH_TAG_ECCTYPE,
	   DH_TAG_ECCPARAM, DH_TAG_CRYPTYPE, DH_TAG_CRYPTPARAM,
	   DH_TAG_AUTHENTYPE, DH_TAG_AUTHENTPARAM, DH_TAG_FORMATYPE,
	   DH_TAG_FORMATPARAM };

/* Initialise a DataHeader type, and destroy it afterwards */

void newDataHeader( DATAHEADER *dataHeader )
	{
	/* Initialise all fields */
	dataHeader->dataType = DATATYPE_TEXT;
	dataHeader->storageType = STORAGETYPE_NONE;
	dataHeader->eccType = ECCTYPE_NONE;
	dataHeader->cryptType = CRYPTYPE_NONE;
	dataHeader->authentType = AUTHENTYPE_NONE;
	dataHeader->formatType = FORMATYPE_NONE;
	}

void deleteDataHeader( DATAHEADER *dataHeader )
	{
	/* Clear all fields */
	newDataHeader( dataHeader );
	}

/* Determine the encoded size of a data header */

int sizeofDataHeader( DATAHEADER *dataHeader )
	{
	int headerSize;

	/* Calculate size of fixed format field */
	headerSize = sizeofEnumerated( dataHeader->dataType );

	/* Calculate size of optional format fields */
	headerSize = sizeofEnumerated( dataHeader->storageType );
	headerSize = sizeofEnumerated( dataHeader->eccType );
	headerSize = sizeofEnumerated( dataHeader->cryptType );
	headerSize = sizeofEnumerated( dataHeader->authentType );
	headerSize = sizeofEnumerated( dataHeader->formatType );

	return( headerSize );
	}

/* Write a data header to disk */

void writeDataHeader( STREAM *stream, DATAHEADER *dataHeader )
	{
	/* Write data header tag */
	writeIdentifier( stream, BER_APPLICATION, BER_CONSTRUCTED, BER_APPL_FILE_DATA );
	writeLength( stream, sizeofDataHeader( dataHeader ) );

	/* Write fixed format field */
	writeEnumerated( stream, dataHeader->dataType, DEFAULT_TAG );

	/* Write optional format fields */
	if( evalEnumerated( dataHeader->storageType ) )
		writeEnumerated( stream, dataHeader->storageType, DH_TAG_STORAGETYPE );
	if( evalEnumerated( dataHeader->eccType ) )
		writeEnumerated( stream, dataHeader->eccType, DH_TAG_ECCTYPE );
	if( evalEnumerated( dataHeader->cryptType ) )
		writeEnumerated( stream, dataHeader->cryptType, DH_TAG_CRYPTYPE );
	if( evalEnumerated( dataHeader->authentType ) )
		writeEnumerated( stream, dataHeader->authentType, DH_TAG_AUTHENTYPE );
	if( evalEnumerated( dataHeader->formatType ) )
		writeEnumerated( stream, dataHeader->formatType, DH_TAG_FORMATYPE );
	}

/* Read a data header from disk */

int readDataHeader( STREAM *stream, DATAHEADER *dataHeader )
	{
	BER_TAGINFO dataHeaderInfo, tagInfo;
	int length;

	/* Read information on the data header structure */
	readIdentifier( stream, &dataHeaderInfo );
	if( dataHeaderInfo.class != BER_APPLICATION ||
		!dataHeaderInfo.constructed ||
		dataHeaderInfo.identifier != BER_APPL_FILE_DATA )
		{
		/* Unknown type, skip it */
		readUniversalData( stream );
		return( ERROR );
		}
	readLength( stream, &length );

	/* Read fixed format field */
	length -= readEnumerated( stream, ( int * ) &dataHeader->dataType );

	/* Read in any remaining fields */
	while( length )
		{
		length -= readIdentifier( stream, &tagInfo );
		switch( tagInfo.identifier )
			{
			/* Read optional format fields */
			case DH_TAG_STORAGETYPE:
				length -= readEnumeratedData( stream, ( int * ) &dataHeader->storageType );
				break;
			case DH_TAG_ECCTYPE:
				length -= readEnumeratedData( stream, ( int * ) &dataHeader->eccType );
				break;
			case DH_TAG_CRYPTYPE:
				length -= readEnumeratedData( stream, ( int * ) &dataHeader->cryptType );
				break;
			case DH_TAG_AUTHENTYPE:
				length -= readEnumeratedData( stream, ( int * ) &dataHeader->authentType );
				break;
			case DH_TAG_FORMATYPE:
				length -= readEnumeratedData( stream, ( int * ) &dataHeader->formatType );
				break;

			/* Unknown field, skip it */
			default:
				length -= readUniversalData( stream );
			}
		}

	return( OK );
	}
