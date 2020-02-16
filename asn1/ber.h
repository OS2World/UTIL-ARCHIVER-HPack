/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*				ASN.1 Basic Encoding Rules Information Header File			*
*							 BER.H  Updated 05/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _BER_DEFINED

#define _BER_DEFINED

/* Definitions for the ISO 8825:1990 Basic Encoding Rules */

/* Tag class */

#define BER_UNIVERSAL			0x00
#define BER_APPLICATION			0x40
#define BER_CONTEXT_SPECIFIC	0x80
#define BER_PRIVATE				0xC0

/* Whether the encoding is constructed or primitive */

#define BER_CONSTRUCTED			0x20
#define BER_PRIMITIVE			0x00

/* The ID's for universal tag numbers 0-31.  Tag number 0 is reserved for
   encoding the end-of-contents value when an indefinite-length encoding
   is used */

enum { BER_ID_RESERVED, BER_ID_BOOLEAN, BER_ID_INTEGER, BER_ID_BITSTRING,
	   BER_ID_OCTETSTRING, BER_ID_NULL, BER_ID_OBJECT_IDENTIFIER,
	   BER_ID_OBJECT_DESCRIPTOR, BER_ID_EXTERNAL, BER_ID_REAL,
	   BER_ID_ENUMERATED, BER_ID_11, BER_ID_12, BER_ID_13, BER_ID_14,
	   BER_ID_15, BER_ID_SEQUENCE, BER_ID_SET, BER_ID_STRING_NUMERIC,
	   BER_ID_STRING_PRINTABLE, BER_ID_STRING_T61, BER_ID_STRING_VIDEOTEX,
	   BER_ID_STRING_IA5, BER_ID_TIME_UTC, BER_ID_TIME_GENERALIZED,
	   BER_ID_STRING_GRAPHIC, BER_ID_STRING_ISO646, BER_ID_STRING_GENERAL,
	   BER_ID_28, BER_ID_29, BER_ID_30 };

/* The ID's for application-specific tag numbers 0-31 */

enum { BER_APPL_GENERALIZED_STRING, BER_APPL_TIME, BER_APPL_SECRET_KEY_INFO,
	   BER_APPL_FILE_PROPERTY, BER_APPL_FILE_DATA };

/* The encodings for the types */

#define BER_RESERVED	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_RESERVED )
#define BER_BOOLEAN		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_BOOLEAN )
#define BER_INTEGER		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_INTEGER )
#define BER_BITSTRING	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_BITSTRING )
#define BER_OCTETSTRING	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_OCTETSTRING )
#define BER_NULL		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_NULL )
#define BER_OBJECT_IDENTIFIER	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_OBJECT_IDENTIFIER )
#define BER_OBJECT_DESCRIPTOR	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_OBJECT_DESCRIPTOR )
#define BER_EXTERNAL	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_EXTERNAL )
#define BER_REAL		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_REAL )
#define BER_ENUMERATED	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_ENUMERATED )
#define BER_11			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_11 )
#define BER_12			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_12 )
#define BER_13			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_13 )
#define BER_14			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_14 )
#define BER_15			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_15 )
#define BER_SEQUENCE	( BER_UNIVERSAL | BER_CONSTRUCTED | BER_ID_SEQUENCE )
#define BER_SET			( BER_UNIVERSAL | BER_CONSTRUCTED | BER_ID_SET )
#define BER_STRING_NUMERIC		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_NUMERIC )
#define BER_STRING_PRINTABLE	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_PRINTABLE )
#define BER_STRING_T61			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_T61 )
#define BER_STRING_VIDEOTEX		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_VIDEOTEX )
#define BER_STRING_IA5	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_IA5 )
#define BER_TIME_UTC	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_TIME_UTC )
#define BER_TIME_GENERALIZED	( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_TIME_GENERALIZED )
#define BER_STRING_GRAPHIC		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_GRAPHIC )
#define BER_STRING_ISO646		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_ISO646 )
#define BER_STRING_GENERAL		( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_STRING_GENERAL )
#define BER_28			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_BER28 )
#define BER_29			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_BER29 )
#define BER_30			( BER_UNIVERSAL | BER_PRIMITIVE | BER_ID_BER30 )

/* Masks to extract information from a tag number */

#define BER_CLASS_MASK			0xC0
#define BER_CONSTRUCTED_MASK	0x20
#define BER_SHORT_ID_MASK		0x1F

/* The maximum size for the short tag number encoding, and the magic value
   which indicates that a long encoding of the number is being used */

#define MAX_SHORT_BER_ID	30
#define LONG_BER_ID			0x1F

/* The default value for tagged types.  If this value is given the basic
   type is used, otherwise the value is used as a context-specific tag */

#define DEFAULT_TAG			-1

/* A struct to store information about a tag */

typedef struct {
			   int class;				/* Tag class */
			   BOOLEAN constructed;		/* Constructed or primitive */
			   int identifier;			/* Tag type */
			   long length;				/* Tag length */
			   } BER_TAGINFO;

/* Routines to read and write the identifier information for an ASN.1 value.
   These are occasionally needed by higher-level routines to handle a stream
   of complex ASN.1 structures involving constructed and choice types */

int readIdentifier( STREAM *stream, BER_TAGINFO *tagInfo );
int readLength( STREAM *stream, int *length );
void writeIdentifier( STREAM *stream, const int class, const BOOLEAN constructed, const int identifier );
void writeLength( STREAM *stream, int length );

#endif /* !_ASN1_DEFINED */
