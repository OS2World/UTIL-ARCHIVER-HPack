/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						ASN.1 Basic Types Handling Routines					*
*							 ASN1.C  Updated 05/01/93						*
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
#include "error.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "asn1\asn1.h"
  #include "asn1\ber.h"
  #include "io\stream.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "asn1.h"
  #include "ber.h"
  #include "stream.h"
  #include "language.h"
#else
  #include "asn1/asn1.h"
  #include "asn1/ber.h"
  #include "io/stream.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* Limits of these routines (mainly for easy portability to all systems):

   Type identifiers are assumed to be less then 2^16
   Lengths of some types (bit strings, octet strings, text strings, integers)
	are assumed to be less than 2^16

   This is mainly to allow easy use on systems with 16-bit ints, and
   shouldn't be a serious problem in either HPACK or PGP.

   Encoding mostly follows the Distinguished Encoding Rules - in particular
   things like constructed encodings and alternate long forms aren't used */

/* Some ASN.1 structures are unused in HPACK, either because they aren't
   needed or because they're impractical.  These are made conditional by
   the following define.  Uncommenting it will make these routines available
   for use (as well as increasing the total code size somewhat) */

/* #define STRICT_ASN1 */

/* The difference between the Unix and Julian epochs, in seconds */

#define EPOCH_DIFF	0x18A41200L		/* 0x3118A41200 or 210866803200 secs */

/****************************************************************************
*																			*
*								Utility Routines							*
*																			*
****************************************************************************/

#ifdef STRICT_ASN1

/* Calculate the size of an encoded flagged value */

static int calculateFlaggedSize( const long value )
	{
	if( value >= 16384L )
		return( 3 );
	else
		if( value >= 128L )
			return( 2 );
		else
			return( 1 );
	}
#endif /* STRICT_ASN1 */

/* Calculate the size of the encoded length octets */

static int calculateLengthSize( const int length )
	{
	if( length < 127 )
		/* Use short form of length octets */
		return( 1 );
	else
		/* Use long form of length octets: length-of-length followed by
		   16 or 8-bit length */
		return( 1 + ( ( length > 256 ) ? 1 : 0 ) + 1 );
	}

/****************************************************************************
*																			*
*					Constructors/Destructors for ASN.1 Types				*
*																			*
****************************************************************************/

/* Initialise an integer to a given value, and destroy it afterwards */

void newInteger( INTEGER *integer, const long value )
	{
	/* Initialise long format fields */
	integer->precision = 0;
	integer->longInteger = NULL;

	/* Initialise short format fields */
	integer->shortInteger = value;
	}

void deleteInteger( INTEGER *integer )
	{
	/* Free any storage used and zero fields */
	if( integer->longInteger != NULL )
		hfree( integer->longInteger );
	newInteger( integer, 0L );
	}

/* Assign a value to an integer */

void assignInteger( INTEGER *integer, const long value )
	{
	deleteInteger( integer );
	newInteger( integer, value );
	}

/* Initialise an octet string to a given value, and destroy it afterwards */

void newOctetString( OCTETSTRING *octetString, const BYTE *value,
					  const int length )
	{
	octetString->length = length;
	if( !length )
		/* No length, set string field to null value */
		octetString->string = NULL;
	else
		{
		/* Allocate room for data and initialise it */
		if( ( octetString->string = ( BYTE * ) hmalloc( length ) ) == NULL )
			error( OUT_OF_MEMORY );
		memcpy( octetString->string, value, length );
		}
	}

void deleteOctetString( OCTETSTRING *octetString )
	{
	/* Free any storage used and zero fields */
	if( octetString->string != NULL )
		hfree( octetString->string );
	newOctetString( octetString, NULL, 0 );
	}

/* Assign a value to an octet string */

void assignOctetString( OCTETSTRING *octetString, const BYTE *value,
						const int length )
	{
	deleteOctetString( octetString );
	newOctetString( octetString, value, length );
	}

/* Initialise a bit string to a given value, and destroy it afterwards */

void newBitString( BITSTRING *bitString, const BYTE *value,
					const int length )
	{
	int octetLength = ( length >> 3 ) + ( ( length & 7 ) ? 1 : 0 );

	bitString->length = length;
	if( !length )
		/* No length, set string field to null value */
		bitString->string = NULL;
	else
		{
		/* Allocate room for data and initialise it */
		if( ( bitString->string = ( BYTE * ) hmalloc( octetLength ) ) == NULL )
			error( OUT_OF_MEMORY );
		memcpy( bitString->string, value, octetLength );
		}
	}

void deleteBitString( BITSTRING *bitString )
	{
	/* Free any storage used and zero fields */
	if( bitString->string != NULL )
		hfree( bitString->string );
	newBitString( bitString, NULL, 0 );
	}

/* Assign a value to a bit string */

void assignBitString( BITSTRING *bitString, const BYTE *value,
					  const int length )
	{
	deleteBitString( bitString );
	newBitString( bitString, value, length );
	}

#ifdef STRICT_ASN1

/* Initialise a generalized time structure to a given value, and destroy it
   afterwards */

void newGeneralizedTime( GENERALIZED_TIME *generalizedTime,
						  const int years, const int months, const int days,
						  const int hours, const int minutes, const int seconds )
	{
	/* Set up main time fields */
	generalizedTime->years = years;
	generalizedTime->months = months;
	generalizedTime->days = days;
	generalizedTime->hours = hours;
	generalizedTime->minutes = minutes;
	generalizedTime->seconds = seconds;

	/* Fractional seconds are system-specific */
	generalizedTime->fractional = 0;

	/* Currently we don't have any time differential information */
	generalizedTime->timeDiff = 0;
	}

void deleteGeneralizedTime( GENERALIZED_TIME *generalizedTime )
	{
	/* Zero fields */
	newGeneralizedTime( generalizedTime, 0, 0, 0, 0, 0, 0 );
	}

/* Assign a value to a generalized time structure */

void assignGeneralizedTime( GENERALIZED_TIME *generalizedTime,
							const int years, const int months, const int days,
							const int hours, const int minutes, const int seconds )
	{
	deleteGeneralizedTime( generalizedTime );
	newGeneralizedTime( generalizedTime, years, months, days,
						hours, minutes, seconds );
	}
#endif /* STRICT_ASN1 */

/* Initialise a generalized string string structure, and destroy it
   afterwards */

void newGeneralizedString( GENERALIZED_STRING *generalizedString,
						   BYTE *string, int length, int type )
	{
	/* Set up general information */
	generalizedString->length = length;
	generalizedString->type = type;

	if( !length )
		/* No length, set string field to null value */
		generalizedString->string = NULL;
	else
		{
		/* Allocate room for data and initialise it, adding a null terminator
		   for those routines which expect one */
		if( ( generalizedString->string = ( BYTE * ) hmalloc( length + 1 ) ) == NULL )
			error( OUT_OF_MEMORY );
		memcpy( generalizedString->string, string, length );
		generalizedString->string[ length ] = '\0';
		}
	}

void deleteGeneralizedString( GENERALIZED_STRING *generalizedString )
	{
	/* Free any storage used and zero fields */
	if( generalizedString->string != NULL )
		hfree( generalizedString->string );
	newGeneralizedString( generalizedString, NULL, 0, STRINGTYPE_NONE );
	}

/* Assign a value to a generalized string */

void assignGeneralizedString( GENERALIZED_STRING *generalizedString,
							  BYTE *string, int length, int type )
	{
	deleteGeneralizedString( generalizedString );
	newGeneralizedString( generalizedString, string, length, type );
	}

/* Initialise a time structure to a given value, and destroy it afterwards */

void newTime( TIME *time, time_t seconds, time_t fractional )
	{
	/* Set up time fields */
	time->seconds = seconds;
	time->fractional = fractional;
	}

void deleteTime( TIME *time )
	{
	/* Zero fields */
	newTime( time, 0L, 0L );
	}

/* Assign a value to a time structure */

void assignTime( TIME *time, time_t seconds, time_t fractional )
	{
	deleteTime( time );
	newTime( time, seconds, fractional );
	}

/* Initialise a permittedAction structure to a given value, and destroy it
   afterwards */

void newPermittedActions( PERMITTED_ACTIONS *permittedActions, LONG action )
	{
	*permittedActions = action;
	}

void deletePermittedActions( PERMITTED_ACTIONS *permittedActions )
	{
	newPermittedActions( permittedActions, ACTION_NONE );
	}

/* Assign a value to a permittedActions structure */

void assignPermittedActions( PERMITTED_ACTIONS *permittedActions, LONG action )
	{
	deletePermittedActions( permittedActions );
	newPermittedActions( permittedActions, action );
	}

/* Initialise an accountingInfo structure to a given value, and destroy it
   afterwards */

void newAccountingInfo( ACCOUNTING_INFO *accountingInfo, int info )
	{
	accountingInfo->id = info;
	}

void deleteAccountingInfo( ACCOUNTING_INFO *accountingInfo )
	{
	newAccountingInfo( accountingInfo, ACCOUNTINFO_NONE );
	}

/* Assign a value to an accountingInfo structure */

void assignAccountingInfo( ACCOUNTING_INFO *accountingInfo, int info )
	{
	deleteAccountingInfo( accountingInfo );
	newAccountingInfo( accountingInfo, info );
	}

/****************************************************************************
*																			*
*						sizeof() methods for ASN.1 Types					*
*																			*
****************************************************************************/

/* Determine the encoded size of an integer value */

int sizeofInteger( const INTEGER *integer )
	{
	int size;

	if( !integer->precision )
		/* It's stored internally as a short value */
		size = ( integer->shortInteger < 256L ) ? 1 :
			   ( integer->shortInteger < 65536L ) ? 2 :
			   ( integer->shortInteger < 16777216L ) ? 3 : 4;
	else
		/* It's stored internally as a little-endian long value */
		size = integer->precision;

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

/* Determine the encoded size of an enumerated value.  This is encoded as an
   integer so we just create an integer for it and calculate its value */

int sizeofEnumerated( const int enumerated )
	{
	INTEGER integer;
	int size;

	/* Create an integer to hold the numeric value, find it's size, and
	   delete it.  The encoding is the same as that of an integer */
	newInteger( &integer, ( long ) enumerated );
	size = sizeofInteger( &integer );
	deleteInteger( &integer );
	return( size );
	}

#ifdef STRICT_ASN1

/* Determine the encoded size of a null value */

int sizeofNull( void )
	{
	/* Always encoded into a tag and a single byte of data */
	return( sizeof( BYTE ) + sizeof( BYTE ) );
	}

#endif /* STRICT_ASN1 */

/* Determine the encoded size of a boolean value */

int sizeofBoolean( void )
	{
	/* Always encoded into a tag byte, a length byte, and single data byte */
	return( sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE ) );
	}

/* Determine the encoded size of an octet string */

int sizeofOctetString( const OCTETSTRING *octetString )
	{
	/* Return the total encoded size */
	return( ( int ) sizeof( BYTE ) + calculateLengthSize( octetString->length ) +
			octetString->length );
	}

/* Determine the encoded size of a bit string */

int sizeofBitString( const BITSTRING *bitString )
	{
	int size = ( ( bitString->length >> 3 ) +
			   ( ( bitString->length & 7 ) ? 1 : 0 ) );

	/* Return the total encoded size (the extra byte is for the bitcount
	   mod 8) */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + 1 + size ) );
	}

/* Determine the encoded size of an ISO 646 string */

int sizeofISO646string( const char *iso646string )
	{
	int size = strlen( iso646string );

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

#ifdef STRICT_ASN1

/* Determine the encoded size of an object identifier */

int sizeofObjectIdentifier( const OBJECT_IDENTIFIER *objectIdentifier )
	{
	int size = sizeof( BYTE ) + calculateFlaggedSize( objectIdentifier->ident );

	/* Calculate the size of the optional fields */
	if( objectIdentifier->noFields > 3 )
		size += calculateFlaggedSize( objectIdentifier->subIdent1 );
	if( objectIdentifier->noFields > 4 )
		size += calculateFlaggedSize( objectIdentifier->subIdent2 );
	if( objectIdentifier->noFields > 5 )
		size += calculateFlaggedSize( objectIdentifier->subIdent3 );

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

/* Determine the encoded size of a generalized time value */

int sizeofGeneralizedTime( const GENERALIZED_TIME *generalizedTime )
	{
	char buffer[ 10 ];
	int size = 14;			/* Size of fixed-size fields */

	/* Add the fractional seconds field if there is one present */
	if( generalizedTime->fractional )
		size += sprintf( buffer, ".%d", generalizedTime->fractional );

	/* Add the time differential if there is one */
	if( generalizedTime->timeDiff )
		size += 5;			/* '+' + 4-digit hour difference */

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}
#endif /* STRICT_ASN1 */

/* Determine the encoded size of a generalizedString value */

int sizeofGeneralizedString( const GENERALIZED_STRING *generalizedString )
	{
	OCTETSTRING octetString;
	int size;

	/* If it's a basic type, just return its size */
	if( ( generalizedString->type == STRINGTYPE_ISO646 ) ||
		( generalizedString->type == STRINGTYPE_ISO10646 ) )
		return( ( int ) ( sizeof( BYTE ) +
				calculateLengthSize( generalizedString->length ) +
				generalizedString->length ) );

	/* It's a composite type.  Evaluate the size of the enumerated value
	   needed to encode the string type and the octet string needed to
	   encode the string itself */
	size = sizeofEnumerated( generalizedString->type );
	newOctetString( &octetString, generalizedString->string,
								  generalizedString->length );
	size += sizeofOctetString( &octetString );
	deleteOctetString( &octetString );

	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

/* Determine the encoded size of a time value */

int sizeofTime( const TIME *time )
	{
	int size = 5;			/* Size of fixed-size fields */

	if( time->fractional )
		/* Calculate size of fractional component */
		size += ( time->fractional < 0xFF ) ? 1 :
				( time->fractional < 0xFFFF ) ? 2 :
				( time->fractional < 0xFFFFFFL ) ? 3 : 4;

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

/* Determine the encoded size of a permittedActions value */

int sizeofPermittedActions( const PERMITTED_ACTIONS *permittedActions )
	{
	int size = ( *permittedActions < 0x00FF ) ? 2 : 1;

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( size ) + size ) );
	}

/* Determine the encoded size of an accountingInfo value */

int sizeofAccountingInfo( const ACCOUNTING_INFO *accountingInfo )
	{
	if( accountingInfo->id )
		;	/* Get rid of warning */

	/* Return the total encoded size */
	return( ( int ) ( sizeof( BYTE ) + calculateLengthSize( 0 ) + 0 ) );
	}

/****************************************************************************
*																			*
*						Magnitude methods for ASN.1 types					*
*																			*
****************************************************************************/

/* Determine whether an integer has a magnitude */

BOOLEAN evalInteger( INTEGER *integer )
	{
	return( ( integer->precision || integer->shortInteger ) ? TRUE : FALSE );
	}

/* Determine whether an enumerated type has a magnitude */

BOOLEAN evalEnumerated( const int enumerated )
	{
	return( ( enumerated ) ? TRUE : FALSE );
	}

/* Determine whether an octet string has a magnitude */

BOOLEAN evalOctetString( OCTETSTRING *octetString )
	{
	return( ( octetString->length ) ? TRUE : FALSE );
	}

/* Determine whether a time type has a magnitude */

BOOLEAN evalTime( TIME *time )
	{
	return( ( time->seconds ) ? TRUE : FALSE );
	}

/* Determine whether a permittedActions type has a magnitude */

BOOLEAN evalPermittedActions( PERMITTED_ACTIONS *permittedActions )
	{
	return( ( *permittedActions ) ? TRUE : FALSE );
	}

/* Determine whether an accountingInfo type has a magnitude */

BOOLEAN evalAccountingInfo( ACCOUNTING_INFO *accountingInfo )
	{
	if( accountingInfo->id )
		;	/* Get rid of warning */

	return( FALSE );
	}

/****************************************************************************
*																			*
*							ASN.1 Output Routines							*
*																			*
****************************************************************************/

/* Write a value in 7-bit flagged format */

static void writeFlagged( STREAM *stream, const long value )
	{
	long flaggedValue = value;
	BOOLEAN hasHighBits = FALSE;

	/* Write the high octets (if necessary) with flag bits set, followed by
	   the final octet */
	if( flaggedValue >= 16384L )
		{
		sputc( stream, ( BYTE ) ( 0x80 | ( flaggedValue >> 14 ) ) );
		flaggedValue -= 16384L;
		hasHighBits = TRUE;
		}
	if( ( flaggedValue > 128L ) || hasHighBits )
		{
		sputc( stream, ( BYTE ) ( 0x80 | ( flaggedValue >> 7 ) ) );
		flaggedValue -= 128L;
		}
	sputc( stream, ( BYTE ) flaggedValue );
	}

/* Write the identifier octets for an ASN.1 data type */

void writeIdentifier( STREAM *stream, const int class,
					  const BOOLEAN constructed, const int identifier )
	{
	/* Check if we can write it as a short encoding of the type */
	if( identifier <= MAX_SHORT_BER_ID )
		sputc( stream, ( BYTE ) ( class | constructed | identifier ) );
	else
		{
		/* Write it as a long encoding */
		sputc( stream, ( BYTE ) ( class | constructed | LONG_BER_ID ) );

		/* Write the identifier as a flagged value */
		writeFlagged( stream, identifier );
		}
	}

/* Write the length octets for an ASN.1 data type */

void writeLength( STREAM *stream, int length )
	{
	/* Check if we can use the short form of length octets */
	if( length < 127 )
		sputc( stream, ( BYTE ) length );
	else
		{
		/* Write number of length octets */
		sputc( stream, ( BYTE ) ( 0x80 | ( length >> 7 ) ) );

		/* Write the length octets themselves */
		if( length > 256 )
			{
			sputc( stream, ( BYTE ) ( length >> 8 ) );
			length >>= 8;
			}
		sputc( stream, ( BYTE ) length );
		}
	}

/* Write a numeric value - used by several routines */

static void writeNumeric( STREAM *stream, INTEGER *integer )
	{
	long value;
	int length;

	/* Check if it's stored internally as a short value */
	if( !integer->precision )
		{
		/* Determine the number of bytes necessary to encode the value */
		length = ( ( integer->shortInteger < 256L ) ? 1 :
				   ( integer->shortInteger < 65536L ) ? 2 :
				   ( integer->shortInteger < 16777216L ) ? 3 : 4 );
		writeLength( stream, length );

		/* Write the appropriate number of bytes */
		value = integer->shortInteger;
		switch( length )
			{
			case 4:
				sputc( stream, ( BYTE ) ( value >> 24 ) );
			case 3:
				sputc( stream, ( BYTE ) ( value >> 16 ) );
			case 2:
				sputc( stream, ( BYTE ) ( value >> 8 ) );
			case 1:
				sputc( stream, ( BYTE ) value );
			}
		}
	else
		{
		/* It's stored internally as a little-endian long value */
		writeLength( stream, integer->precision );
		for( length = integer->precision - 1; length >= 0; length-- )
			sputc( stream, integer->longInteger[ length ] );
		}
	}

/* Write a string value - used by several routines */

static void writeString( STREAM *stream, const BYTE *string, const int length )
	{
	/* Write the length field */
	writeLength( stream, length );

	/* Write the string itself */
	swrite( stream, string, length );
	}

/* Write an integer */

void writeInteger( STREAM *stream, INTEGER *integer, int tag )
	{
	/* Write the identifier and numeric fields */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_INTEGER );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeNumeric( stream, integer );
	}

/* Write an enumerated value.  This is encoded as an integer so we just
   create an integer for it and write it as such */

void writeEnumerated( STREAM *stream, const int enumerated, int tag )
	{
	INTEGER integer;

	/* Create an integer to hold the numeric value */
	newInteger( &integer, ( long ) enumerated );

	/* Write the identifier and numeric fields */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_ENUMERATED );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeNumeric( stream, &integer );

	/* Delete the created integer */
	deleteInteger( &integer );
	}

#ifdef STRICT_ASN1

/* Write a null value */

void writeNull( STREAM *stream, int tag )
	{
	/* Write the identifier and null length octet */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_NULL );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	sputc( stream, 0 );
	}

#endif /* STRICT_ASN1 */

/* Write a boolean value.  Note that we always encode TRUE as 1 and FALSE
   as 0 even though the BER states that TRUE can be any nonzero value */

void writeBoolean( STREAM *stream, const BOOLEAN boolean, int tag )
	{
	/* Write the identifier and boolean value */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_BOOLEAN );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	sputc( stream, 1 );								/* Length is one byte */
	sputc( stream, ( BYTE ) ( boolean ? 1 : 0 ) );	/* Write TRUE or FALSE */
	}

/* Write an octet string */

void writeOctetString( STREAM *stream, const OCTETSTRING *octetString, int tag )
	{
	/* Write the identifier and string fields */
    if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_OCTETSTRING );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeString( stream, octetString->string, octetString->length );
	}

/* Write a bit string */

void writeBitString( STREAM *stream, const BITSTRING *bitString, int tag )
	{
	int octetLength = ( bitString->length >> 3 ) +
					  ( ( bitString->length & 7 ) ? 1 : 0 );

	/* Write the identifier and length (string length in octets + extra octet
	   for remaining bits) */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_BITSTRING );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeLength( stream, octetLength + 1 );
	sputc( stream, ( BYTE ) ( bitString->length & 7 ) );	/* Write bit remainder octet */

	/* Write the string itself */
	swrite( stream, bitString->string, octetLength );
	}

/* Write an ISO 646 string */

void writeISO646string( STREAM *stream, const char *iso646string, int tag )
	{
	/* Write the identifier and string fields */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_STRING_ISO646 );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeString( stream, ( BYTE * ) iso646string, strlen( iso646string ) );
	}

#ifdef STRICT_ASN1

/* Write an object identifier */

void writeObjectIdentifier( STREAM *stream, const OBJECT_IDENTIFIER *objectIdentifier, int tag )
	{
	int length = sizeof( BYTE ) + calculateFlaggedSize( objectIdentifier->ident );

	/* Write the identifier and length fields.  The root, type, and ident
	   fields are always present, the rest are optional.  The first two are
	   encoded as one byte, the remaining values are variable - length and
	   their size must be determined at runtime */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_OBJECT_IDENTIFIER );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	if( objectIdentifier->noFields > 3 )
		length += calculateFlaggedSize( objectIdentifier->subIdent1 );
	if( objectIdentifier->noFields > 4 )
		length += calculateFlaggedSize( objectIdentifier->subIdent2 );
	if( objectIdentifier->noFields > 5 )
		length += calculateFlaggedSize( objectIdentifier->subIdent3 );
	writeLength( stream, length );

	/* Write the object identifier */
	writeFlagged( stream, ( objectIdentifier->root * 40 ) + objectIdentifier->type );
	writeFlagged( stream, objectIdentifier->ident );
	if( objectIdentifier->noFields > 3 )
		writeFlagged( stream, objectIdentifier->subIdent1 );
	if( objectIdentifier->noFields > 4 )
		writeFlagged( stream, objectIdentifier->subIdent2 );
	if( objectIdentifier->noFields > 5 )
		writeFlagged( stream, objectIdentifier->subIdent3 );
	}

/* Write a generalized time value */

void writeGeneralizedTime( STREAM *stream, const GENERALIZED_TIME *generalizedTime, int tag )
	{
	char buffer[ 40 ];
	int count;

	/* Print the main time fields */
	count = sprintf( buffer, "%04d%02d%02d%02d%02d%02d", generalizedTime->years,
					 generalizedTime->months, generalizedTime->days,
					 generalizedTime->hours, generalizedTime->minutes,
					 generalizedTime->seconds );

	/* Add the fractional seconds field if there is one present */
	if( generalizedTime->fractional )
		count += sprintf( buffer + count, ".%d", generalizedTime->fractional );

	/* Add the time differential if there is one */
	if( generalizedTime->timeDiff )
		count += sprintf( buffer + count, "+%02d00", generalizedTime->timeDiff );

	/* Write the identifier and length fields */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_TIME_GENERALIZED );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeLength( stream, count );

	/* Write the time string */
	swrite( stream, ( BYTE * ) buffer, count );
	}
#endif /* STRICT_ASN1 */

/* Write a generalized string value */

void writeGeneralizedString( STREAM *stream, const GENERALIZED_STRING *generalizedString, int tag )
	{
	OCTETSTRING octetString;

	/* If it's a basic type, just write it as such */
	if( ( generalizedString->type == STRINGTYPE_ISO646 ) ||
		( generalizedString->type == STRINGTYPE_ISO10646 ) )
		writeISO646string( stream, ( char * ) generalizedString->string, tag );
	else
		{
		/* Turn the string into an octetString */
		newOctetString( &octetString, generalizedString->string,
									  generalizedString->length );

		/* Write the identifier and length fields */
		if( tag == DEFAULT_TAG )
			writeIdentifier( stream, BER_APPLICATION, BER_CONSTRUCTED, BER_APPL_GENERALIZED_STRING );
		else
			writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_CONSTRUCTED, tag );
		writeLength( stream, sizeofEnumerated( generalizedString->type ) +
							 sizeofOctetString( &octetString ) );

		/* It's a composite type.  Write the size of the enumeration needed
		   to encode the string type, followed by the data as an octet
		   string */
		writeEnumerated( stream, generalizedString->type, DEFAULT_TAG );
		writeOctetString( stream, &octetString, DEFAULT_TAG );

		deleteOctetString( &octetString );
		}
	}

/* Write a time value */

void writeTime( STREAM *stream, const TIME *time, int tag )
	{
	time_t seconds = time->seconds + EPOCH_DIFF;
	time_t fractional = time->fractional;
	int length = 5;

	/* Write the identifier field */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_APPLICATION, BER_PRIMITIVE, BER_OCTETSTRING );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );

	/* Calculate the length and write it */
	while( fractional > 0 )
		{
		fractional >>= 8;
		length++;
		}
	fractional = time->fractional;
	writeLength( stream, length );

	/* Write seconds component */
	sputc( stream, 0x31 );		/* Write MSB of time in seconds */
	sputc( stream, ( BYTE ) ( seconds >> 24 ) );
	sputc( stream, ( BYTE ) ( seconds >> 16 ) );
	sputc( stream, ( BYTE ) ( seconds >> 8 ) );
	sputc( stream, ( BYTE ) seconds );

	/* Write fractional component */
	if( fractional > 0xFFFFFFL )
		sputc( stream, ( BYTE ) ( fractional >> 24 ) );
	if( fractional > 0xFFFFL )
		sputc( stream, ( BYTE ) ( fractional >> 16 ) );
	if( fractional > 0xFFL )
		sputc( stream, ( BYTE ) ( fractional >> 8 ) );
	if( fractional )
		sputc( stream, ( BYTE ) fractional );
	}

/* Write a permittedActions value */

void writePermittedActions( STREAM *stream, const PERMITTED_ACTIONS *permittedActions, int tag )
	{
	int octetLength = ( *permittedActions > 0x00FF ) ? 2 : 1;

	/* Write the identifier and length (string length in octets + extra octet
	   for remaining bits) */
	if( tag == DEFAULT_TAG )
		writeIdentifier( stream, BER_UNIVERSAL, BER_PRIMITIVE, BER_BITSTRING );
	else
		writeIdentifier( stream, BER_CONTEXT_SPECIFIC, BER_PRIMITIVE, tag );
	writeLength( stream, octetLength + 1 );
	sputc( stream, 0 );					/* Write bit remainder octet */

	/* Write the permittedActions fields */
	sputc( stream, ( BYTE ) *permittedActions );
	if( *permittedActions > 0xFF )
		sputc( stream, ( BYTE ) ( *permittedActions >> 8 ) );
	}

/****************************************************************************
*																			*
*							ASN.1 Input Routines							*
*																			*
****************************************************************************/

/* Read a value in 7-bit flagged format */

static int readFlagged( STREAM *stream, long *flaggedValue )
	{
	long value = 0L;
	int readDataLength = 1;
	BYTE data;

	/* Read the high octets (if any) with flag bits set, followed by
	   the final octet */
	data = sgetc( stream );
	while( data & 0x80 )
		{
		value <<= 7;
		value |= data & 0x7F;
		data = sgetc( stream );
		readDataLength++;
		}
	*flaggedValue = value | data;

	return( readDataLength );
	}

/* Read the identifier octets for an ASN.1 data type */

int readIdentifier( STREAM *stream, BER_TAGINFO *tagInfo )
	{
	BYTE data;
	int readDataLength = 1;

	data = sgetc( stream );
	tagInfo->class = data & BER_CLASS_MASK;
	tagInfo->constructed = ( data & BER_CONSTRUCTED_MASK ) ? TRUE : FALSE;
	if( ( data & BER_SHORT_ID_MASK ) != LONG_BER_ID )
		/* ID is encoded in short form */
		tagInfo->identifier = data & BER_SHORT_ID_MASK;
	else
		{
		/* ID is encoded in long form */
		long value;

		readDataLength += readFlagged( stream, &value );
		tagInfo->identifier = ( int ) value;
		}
	tagInfo->length = 0;

	return( readDataLength );
	}

/* Read the length octets for an ASN.1 data type */

int readLength( STREAM *stream, int *length )
	{
	int readDataLength = 1;
	BYTE data;

	data = sgetc( stream );
	if( !( data & 0x80 ) )
		/* Data is encoded in short form */
		*length = ( unsigned int ) data;
	else
		{
		/* Data is encoded in long form.  First get the octet count */
		int noLengthOctets = data & 0x7F, localLength = 0;

		/* Now read the length octets themselves */
		while( noLengthOctets )
			{
			localLength <<= 8;
			localLength |= ( unsigned int ) sgetc( stream );
			readDataLength++;
			}
		*length = localLength;
		}

	return( readDataLength );
	}

/* Read a numeric value - used by several routines */

static int readNumeric( STREAM *stream, INTEGER *integer )
	{
	int length, readDataLength;

	/* Read the length field */
	readDataLength = readLength( stream, &length );
	readDataLength += length;

	/* Check if it's a short value */
	if( length < sizeof( long ) )
		{
		integer->precision = 0;
		integer->shortInteger = 0L;
		while( length-- )
			{
			integer->shortInteger <<= 8;
			integer->shortInteger |= sgetc( stream );
			}
		}
	else
		{
		/* Read it in as a long value.  First, allocate the room for it */
		if( integer->longInteger != NULL )
			hfree( integer->longInteger );
		if( ( integer->longInteger = ( BYTE * ) hmalloc( length ) ) == NULL )
			error( OUT_OF_MEMORY );

		/* Now read it in, translating it from big-endian to little-endian */
		integer->precision = length--;
		while( length >= 0 )
			integer->longInteger[ length-- ] = sgetc( stream );
		}

	return( readDataLength );
	}

/* Read a string value - used by several routines */

static int readString( STREAM *stream, BYTE **string, int *stringLength )
	{
	int length, readDataLength;

	/* Read the string length */
	readDataLength = readLength( stream, &length );
	*stringLength = length;

	/* Read the string itself.  First allocate the room for it */
	if( *string != NULL )
		hfree( *string );
	if( ( *string = ( BYTE * ) hmalloc( length + 1 ) ) == NULL )
		error( OUT_OF_MEMORY );

	/* Now read it in, adding a null terminator for those routines which
	   need it */
	sread( stream, *string, length );
	( *string )[ length ] = '\0';

	return( readDataLength + length );
	}

/* Read a universal type and discard it (used to skip unknown types) */

int readUniversalData( STREAM *stream )
	{
	int length, readDataLength = readLength( stream, &length );

	readDataLength += length;
	while( length-- )
		sgetc( stream );
	return( readDataLength );
	}

/* Read an integer */

int _readInteger( STREAM *stream, INTEGER *integer, BOOLEAN readIdent )
	{
	int readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_INTEGER )
			return( ERROR );
		}

	/* Read the numeric field */
	return( readDataLength + readNumeric( stream, integer ) );
	}

/* Read an enumerated value.  This is encoded as an integer so we just
   create an integer for it and read it as such */

int _readEnumerated( STREAM *stream, int *enumeration, BOOLEAN readIdent )
	{
	INTEGER integer;
	int readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_ENUMERATED )
			return( ERROR );
		}

	/* Create an integer to hold the numeric value */
	newInteger( &integer, 0L );

	/* Read the numeric field and extract the enumerated type */
	readDataLength += readNumeric( stream, &integer );
	*enumeration = ( int ) integer.shortInteger;

	/* Delete the created integer */
	deleteInteger( &integer );

	return( readDataLength );
	}

#ifdef STRICT_ASN1

/* Read a null value */

int _readNull( STREAM *stream, BOOLEAN readIdent )
	{
	int readDataLength = 0;

	/* Read the identifier if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_NULL )
			return( ERROR );
		}

	/* Skip length octet */
	sgetc( stream );
	return( readDataLength + 1 );
	}

#endif /* STRICT_ASN1 */

/* Read a boolean value */

int _readBoolean( STREAM *stream, BOOLEAN *boolean, BOOLEAN readIdent )
	{
	int readDataLength = 0;

	/* Read the identifier if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength += readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_BOOLEAN )
			return( ERROR );
		}

	/* Skip length octet and read boolean value */
	sgetc( stream  );
	*boolean = sgetc( stream ) ? TRUE : FALSE;

	return( readDataLength + 2 );
	}

/* Read an octet string */

int _readOctetString( STREAM *stream, OCTETSTRING *octetString, BOOLEAN readIdent )
	{
	int readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength += readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_OCTETSTRING )
			return( ERROR );
		}

	/* Read the string field */
	return( readDataLength + readString( stream, &octetString->string,
										 &octetString->length ) );
	}

/* Read a bit string */

int _readBitString( STREAM *stream, BITSTRING *bitString, BOOLEAN readIdent )
	{
	int length, readDataLength = 0, extraBits;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_BITSTRING )
			return( ERROR );
		}

	/* Read the length field (string length in octets + extra octet for
	   remaining bits) */
	readDataLength += readLength( stream, &length ) + 1;
	length--;						/* Don't count length of extra octet */
	extraBits = sgetc( stream );	/* Read remainder bits */

	/* Read the string itself.  First allocate the room for it */
	if( bitString->string != NULL )
		hfree( bitString->string );
	if( ( bitString->string = ( BYTE * ) hmalloc( length ) ) == NULL )
		error( OUT_OF_MEMORY );

	/* Now read it in */
	bitString->length = ( length << 3 ) | extraBits;
	sread( stream, bitString->string, length );

	return( readDataLength + length );
	}

/* Read an ISO 646 string */

int _readISO646string( STREAM *stream, char *iso646string, BOOLEAN readIdent )
	{
	int readDataLength = 0, dummy;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength += readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_STRING_ISO646 )
			return( ERROR );
		}

	/* Read the string fields */
	return( readDataLength + readString( stream, ( ( BYTE ** ) &iso646string ), &dummy ) );
	}

#ifdef STRICT_ASN1

/* Read an object identifier */

int _readObjectIdentifier( STREAM *stream, OBJECT_IDENTIFIER *objectIdentifier, BOOLEAN readIdent )
	{
	long value;
	int data, readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_OBJECT_IDENTIFIER )
			return( ERROR );
		}

	/* Read the length fields.  The length field is one shorter than the
	   in-memory value since the first two values are encoded as one */
	readDataLength = readLength( stream, &objectIdentifier->noFields );
	objectIdentifier->noFields++;

	/* Read the identifier itself */
	readDataLength += readFlagged( stream, &value );
	data = ( int ) value;
	objectIdentifier->root = data / 40;
	objectIdentifier->type = data % 40;
	length += readFlagged( stream, &value);
	objectIdentifier->ident = value;
	if( objectIdentifier->noFields > 3 )
		{
		readDataLength += readFlagged( stream, &value );
		objectIdentifier->subIdent1 = value;
		}
	if( objectIdentifier->noFields > 4 )
		{
		readDataLength += readFlagged( stream, &value );
		objectIdentifier->subIdent2 = value;
		}
	if( objectIdentifier->noFields > 5 )
		{
		readDataLength += readFlagged( stream, &value );
		objectIdentifier->subIdent3 = value;
		}
	return( readDataLength );
	}

/* Read a generalized time value */

int _readGeneralizedTime( STREAM *stream, GENERALIZED_TIME *generalizedTime, BOOLEAN readIdent )
	{
	char buffer[ 40 ];
	int length, readDataLength = 0, index;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_GENERALIZED_TIME )
			return( ERROR );
		}

	/* Read the length field */
	readDataLength = readLength( stream, &length );

	/* Read the time string itself into memory */
	sread( stream, ( BYTE * ) buffer, length );

	/* Read the main time fields */
	sscanf( buffer, "%04d%02d%02d%02d%02d%02d", &generalizedTime->years,
			&generalizedTime->months, &generalizedTime->days,
			&generalizedTime->hours, &generalizedTime->minutes,
			&generalizedTime->seconds );
	index = 14;		/* Size of main time field */

	/* Read the fractional seconds field if there is one present */
	if( buffer[ index ] == '.' )
		{
		sscanf( buffer + index + 1, "%d", &generalizedTime->fractional );
		index++;		/* Skip dit */
		while( index < length && isalpha( buffer[ index ] ) )
			index++;	/* Skip to end of fractional field */
		}

	/* Read the time differential if there is one */
	if( buffer[ index ] == '-' || buffer[ index ] == '+' )
		sscanf( buffer + index + 1, "%02d", &generalizedTime->timeDiff );

	return( readDataLength + length );
	}
#endif /* STRICT_ASN1 */

/* Read a generalized string value */

int readGeneralizedString( STREAM *stream, GENERALIZED_STRING *generalizedString )
	{
	BER_TAGINFO tagInfo;
	int readDataLength, dummy;

	/* Read the identifier field */
	readDataLength = readIdentifier( stream, &tagInfo );

	/* Check to see if it's a basic type */
	if( tagInfo.class == BER_UNIVERSAL )
		{
		/* Perform some type checking */
		if( tagInfo.constructed || tagInfo.identifier != BER_ID_STRING_ISO646 )
			return( ERROR );

		/* Read it in as a standard ISO646 string */
		generalizedString->type = STRINGTYPE_ISO646;
		readDataLength += readString( stream, &generalizedString->string,
									  &generalizedString->length );
		}
	else
		{
		/* Perform some type checking */
		if( tagInfo.class != BER_APPLICATION || !tagInfo.constructed ||
			tagInfo.identifier != BER_APPL_GENERALIZED_STRING )
			return( ERROR );

		/* Read it in as a constructed type */
		readDataLength += readLength( stream, &dummy );	/* Skip SEQUENCE length info */
		readDataLength += readEnumerated( stream, ( int * ) &generalizedString->type );
		readDataLength += readIdentifier( stream, &tagInfo );	/* Skip OCTET STRING tag */
		readDataLength += readString( stream, &generalizedString->string,
									  &generalizedString->length );
		}

	return( readDataLength );
	}

/* Read a time value */

int _readTime( STREAM *stream, TIME *time, BOOLEAN readIdent )
	{
	time_t timeStamp;
	int length, readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_APPLICATION || tagInfo.constructed ||
			tagInfo.identifier != BER_APPL_TIME )
			return( ERROR );
		}

	/* Read the length field */
	readDataLength += readLength( stream, &length );
	length -= 5;		/* Skip length of fixed fields */

	/* Read seconds component */
	sgetc( stream );	/* Skip billions of seconds */
	timeStamp = ( time_t ) sgetc( stream ) << 24;
	timeStamp |= ( time_t ) sgetc( stream ) << 16;
	timeStamp |= ( time_t ) sgetc( stream ) << 8;
	timeStamp |= ( time_t ) sgetc( stream );
	timeStamp -= EPOCH_DIFF;
	time->seconds = timeStamp;

	/* Read fractional component */
	timeStamp = 0L;
	if( length >= 4 )
		timeStamp = ( time_t ) sgetc( stream ) << 24;
	if( length >= 3 )
		timeStamp = ( time_t ) sgetc( stream ) << 24;
	if( length >= 2 )
		timeStamp = ( time_t ) sgetc( stream ) << 24;
	if( length >= 1 )
		timeStamp = ( time_t ) sgetc( stream ) << 24;
	time->fractional = timeStamp;

	return( readDataLength + 5 + length );
	}

/* Read a permitted actions value */

int _readPermittedActions( STREAM *stream, PERMITTED_ACTIONS *permittedActions, BOOLEAN readIdent )
	{
	LONG action = 0L;
	int length, readDataLength = 0;

	/* Read the identifier field if necessary */
	if( readIdent )
		{
		BER_TAGINFO tagInfo;

		readDataLength = readIdentifier( stream, &tagInfo );
		if( tagInfo.class != BER_UNIVERSAL || tagInfo.constructed ||
			tagInfo.identifier != BER_ID_BITSTRING )
			return( ERROR );
		}

	/* Read the length field (string length in octets + extra octet for
	   remaining bits) */
	readDataLength += readLength( stream, &length );
	readDataLength += length;
	length--;						/* Don't count length of extra octet */
	sgetc( stream );				/* Read remainder bits */

	/* Read the permittedActions data */
	while( length-- )
		action = ( action << 8 ) | ( LONG ) sgetc( stream );
	*permittedActions = action;

	return( readDataLength );
	}
