/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   ASN.1 Information Header File					*
*							 ASN1.H  Updated 05/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _ASN1_DEFINED

#define _ASN1_DEFINED

#include <time.h>
#if defined( __ATARI__ )
  #include "io\stream.h"
#elif defined( __MAC__ )
  #include "stream.h"
#else
  #include "io/stream.h"
#endif /* System-specific include directives */

/* Data structures and routines for handling ISO 8824:1990 Abstract Syntax
   Notation One data types.  HPACK uses the types BOOLEAN (defined in
   "defs.h"), INTEGER, OCTET STRING, BITSTRING, ENUMERATION, NULL, and
   various constructed types such as OBJECT IDENTIFIER and SEQUENCE which are
   handled by the ASN.1 I/O routines.

   Each of the structures below act somewhat like a C++ object - their
   constructors *must* be called before use, and their destructors should
   be called after use.  In addition they can be queried as to their encoded
   size, and written to and read from disk.  The methods are:

   new<objectName>();		// Constructor
   delete<objectName>();	// Destructor
   assign<objectName>();	// Assign value to object
   sizeof<objectName>();	// Find encoded size of object
   eval<objectName>();		// Determine if object has a magnitude
   read<objectName>();		// Read object
   read<objectName>Data();	// Read object's data
   write<objectName>();		// Write object

   The read() calls actually have two types, the read() form which reads
   the basic ASN.1 object type and performs type checking, and the more
   general readData() form which only reads the data contained in the ASN.1
   object and performs no type checking */

/****************************************************************************
*																			*
*							Universal ASN.1 Structures						*
*																			*
****************************************************************************/

/* ASN.1 types.  These include dummy values for currently undefined types */

typedef enum { ASN1_TYPE_RESERVED, ASN1_TYPE_BOOLEAN, ASN1_TYPE_INTEGER,
			   ASN1_TYPE_BITSTRING, ASN1_TYPE_OCTETSTRING, ASN1_TYPE_NULL,
			   ASN1_TYPE_OBJECT_IDENTIFIER, ASN1_TYPE_OBJECT_DESCRIPTOR,
			   ASN1_TYPE_EXTERNAL, ASN1_TYPE_REAL, ASN1_TYPE_ENUMERATED,
			   ASN1_TYPE_11, ASN1_TYPE_12, ASN1_TYPE_13, ASN1_TYPE_14,
			   ASN1_TYPE_15, ASN1_TYPE_SEQUENCE, ASN1_TYPE_SET,
			   ASN1_TYPE_STRING_NUMERIC, ASN1_TYPE_STRING_PRINTABLE,
			   ASN1_TYPE_STRING_T61, ASN1_TYPE_STRING_VIDEOTEX,
			   ASN1_TYPE_STRING_IA5, ASN1_TYPE_TIME_UTC,
			   ASN1_TYPE_TIME_GENERALIZED, ASN1_TYPE_STRING_GRAPHIC,
			   ASN1_TYPE_STRING_ISO646, ASN1_TYPE_STRING_GENERAL,
			   ASN1_TYPE_28, ASN1_TYPE_29, ASN1_TYPE_30 } ASN1_TYPE;

/* The ASN.1 INTEGER type.  In most cases this will fit into a machine
   word, so we use a variant record which contains the integer in this
   form, but which can also store the value as an array of bytes, in little-
   endian format */

typedef struct {
			   /* An integer value which will fit into a machine word */
			   long shortInteger;

			   /* A worst-case true ASN.1 integer value.  Precision is set
				  to zero if it'll fit into a longint */
			   int precision;
			   BYTE *longInteger;
			   } INTEGER;

/* The ASN.1 OCTET STRING type */

typedef struct {
			   BYTE *string;		/* The octet string itself */
			   int length;			/* The length of the string */
			   } OCTETSTRING;

/* The ASN.1 BIT STRING type */

typedef struct {
			   BYTE *string;		/* The bit string itself */
			   int length;			/* The length of the string in bits */
			   } BITSTRING;

/* The ASN.1 OBJECT IDENTIFIER type */

typedef struct {
			   int noFields;		/* Number of fields in object ID */
			   int root;			/* ID of root node */
			   int type;			/* ID of type */
			   long ident;			/* Main identifier */
			   long subIdent1, subIdent2, subIdent3;
									/* Subidentifiers */
			   } OBJECT_IDENTIFIER;

/* Values for object identifier root node and type */

enum { OBJECT_ID_CCITT, OBJECT_ID_ISO, OBJECT_ID_JOINT_ISO_CCITT };
enum { OBJECT_ID_STANDARD, OBJECT_ID_REGISTRATION_AUTHORITY,
	   OBJECT_ID_MEMBER_BODY, OBJECT_ID_IDENTIFIED_ORGANISATION };

/* The ASN.1 GENERALIZED TIME type */

typedef struct {
			   int years, months, days;		/* Years, months, days */
			   int hours, minutes, seconds;	/* Hours, minutes, seconds */
			   int fractional;				/* Fractional seconds */
			   int timeDiff;				/* Offset from UTC time in hours */
			   } GENERALIZED_TIME;

/****************************************************************************
*																			*
*						HPACK-Specific ASN.1 Structures						*
*																			*
****************************************************************************/

/* The types of strings we may use */

typedef enum {
			 /* Standard string types */
			 STRINGTYPE_ISO646, STRINGTYPE_ISO10646,

			 /* Single-byte nonstandard string types */
			 STRINGTYPE_ISO8859_1, STRINGTYPE_ISO8859_2,
			 STRINGTYPE_ISO8859_3, STRINGTYPE_ISO8859_4,
			 STRINGTYPE_ISO8859_5, STRINGTYPE_ISO8859_6,
			 STRINGTYPE_ISO8859_7, STRINGTYPE_ISO8859_8,
			 STRINGTYPE_ISO8859_9, STRINGTYPE_ISO8859_10,
			 STRINGTYPE_KOI8, STRINGTYPE_ALTKOI8,
			 STRINGTYPE_MACINTOSH, STRINGTYPE_MAC_CYRILLIC,
			 STRINGTYPE_MAZOVIA, STRINGTYPE_EBCDIC,

			 /* Multibyte nonstandard string types */
			 STRINGTYPE_JIS, STRINGTYPE_SHIFT_JIS, STRINGTYPE_EUC_C,
			 STRINGTYPE_EUC_H, STRINGTYPE_EUC_J, STRINGTYPE_EUC_K,

			 /* Unicode/ISO10646 types */
			 STRINGTYPE_UTF_1, STRINGTYPE_UTF_2,

			 /* String non-type */
			 STRINGTYPE_NONE
			 } STRINGTYPE;

/* The ASN.1 GENERALIZED_STRING type */

typedef struct {
			   STRINGTYPE type;				/* The string type */
			   int length;					/* Length of string */
			   BYTE *string;				/* The string itself */
			   } GENERALIZED_STRING;

/* The ASN.1 TIME type */

typedef struct {
			   time_t seconds;				/* Seconds since 1970 */
			   time_t fractional;			/* Fractional seconds */
			   } TIME;

/* The ASN.1 PERMITTED_ACTIONS type */

typedef LONG PERMITTED_ACTIONS;				/* Permitted actions for file */

/* Bitflag values for the permitted actions.  Note that externally these are
   encoded in little-endian format (that is, bit (0) is the first bit in
   the string, and so on).  The defines are arranged to make this encoding
   easier */

#define ACTION_NONE					0x00000000L		/* No action */
#define ACTION_READ					0x00000080L		/* read (0) */
#define ACTION_WRITE				0x00000040L		/* write (1) */
#define ACTION_EXECUTE				0x00000020L		/* execute (2) */
#define ACTION_INSERT				0x00000010L		/* insert (3) */
#define ACTION_REPLACE				0x00000008L		/* replace (4) */
#define ACTION_EXTEND				0x00000004L		/* extend (5) */
#define ACTION_ERASE				0x00000002L		/* erase (6) */
#define ACTION_READ_ATTRIBUTE		0x00000001L		/* read-attribute (7) */
#define ACTION_CHANGE_ATTRIBUTE		0x00008000L		/* change-attribute (8) */
#define ACTION_DELETE_FILE			0x00004000L		/* delete-file (9) */
#define ACTION_TRAVERSAL			0x00002000L		/* traversal (10) */
#define ACTION_REVERSE_TRAVERSAL	0x00001000L		/* reverse-traversal (11) */
#define ACTION_RANDOM_ACCESS		0x00000800L		/* random-access (12) */

/* The ASN.1 ACCOUNTING_INFO type */

typedef struct {
			   int id;						/* ??? */
			   } ACCOUNTING_INFO;

/* Values for the accounting info field */

#define ACCOUNTINFO_NONE			0

/* The types of data we may use */

typedef enum {
			 /* Universal types */
			 DATATYPE_TEXT, DATATYPE_DATA,

			 /* System-specific types */
			 DATATYPE_RESOURCEFORK,

			 /* Executable file types */
			 DATATYPE_EXE_AMIGA, DATATYPE_EXE_ARC, DATATYPE_EXE_ATARI,
			 DATATYPE_EXE_MAC, DATATYPE_EXE_MSDOS, DATATYPE_EXE_OS2,

			 /* Document types */
			 DATATYPE_DOC_GENERIC, DATATYPE_DOC_POSTSCRIPT,

			 /* Graphical data types */
			 DATATYPE_GRAPHIC_GENERIC, DATATYPE_GRAPHIC_GIF,
			 DATATYPE_GRAPHIC_JPEG,

			 /* Sound data types */
			 DATATYPE_SOUND_GENERIC, DATATYPE_SOUND_MOD
			 } DATATYPE;

/* The storage method for the data */

typedef enum {
			 STORAGETYPE_NONE, STORAGETYPE_MBWA
			 } STORAGETYPE;

/* The ECC method for the data */

typedef enum {
			 ECCTYPE_NONE, ECCTYPE_RS, ECCTYPE_CIRC
			 } ECCTYPE;

/* The encryption method for the data */

typedef enum {
			 CRYPTYPE_NONE, CRYPTYPE_MDC, CRYPTYPE_MDC_RSA
			 } CRYPTYPE;

/* The authentication method for the data */

typedef enum {
			 AUTHENTYPE_NONE, AUTHENTYPE_MD5_RSA, AUTHENTYPE_SHS_DSS,
			 AUTHENTYPE_MULTIPLE
			 } AUTHENTYPE;

/* The format type for the data */

typedef enum {
			 /* No format information */
			 FORMATYPE_NONE,

			 /* Stream format file types */
			 FORMATYPE_STREAM_PRINT, FORMATYPE_STREAM_LF,
			 FORMATYPE_STREAM_CR, FORMATYPE_STREAM_CRLF,
			 FORMATYPE_STREAM_LFCR,

			 /* Record format file types */
			 FORMATYPE_FIXED, FORMATYPE_VARIBLE_8,
			 FORMATYPE_VARIBLE_16, FORMATYPE_VARIBLE_32,

			 /* Strange formats */
			 FORMATYPE_PRIMETEXT, FORMATYPE_NOS
			 } FORMATYPE;

/****************************************************************************
*																			*
*									ASN.1 Routines							*
*																			*
****************************************************************************/

/* Generalized ASN.1 type manipulation routines */

int readUniversalData( STREAM *stream );

/* Routines for handling integers */

void newInteger( INTEGER *integer, const long value );
void deleteInteger( INTEGER *integer );
void assignInteger( INTEGER *integer, const long value );
BOOLEAN evalInteger( INTEGER *integer );
int sizeofInteger( const INTEGER *integer );
void writeInteger( STREAM *stream, INTEGER *integer, int tag );
int _readInteger( STREAM *stream, INTEGER *integer, BOOLEAN readIdent );

#define readInteger(stream,integer) \
		_readInteger( stream, integer, TRUE )
#define readIntegerData(stream,integer)	\
		_readInteger( stream, integer, FALSE )

/* Routines for handling enumerations */

BOOLEAN evalEnumerated( const int enumerated );
int sizeofEnumerated( const int enumerated );
void writeEnumerated( STREAM *stream, const int enumerated, int tag );
int _readEnumerated( STREAM *stream, int *enumeration, BOOLEAN readIdent );

#define readEnumerated(stream,enumeration) \
		_readEnumerated( stream, enumeration, TRUE )
#define readEnumeratedData(stream,enumeration) \
		_readEnumerated( stream, enumeration, FALSE )

/* Routines for handling booleans */

int sizeofBoolean( void );
void writeBoolean( STREAM *stream, const BOOLEAN boolean, int tag );
int _readBoolean( STREAM *stream, BOOLEAN *boolean, BOOLEAN readIdent );

#define readBoolean(stream,boolean) \
		_readBoolean( stream, boolean, TRUE )
#define readBooleanData(stream,boolean) \
		_readBoolean( stream, boolean, FALSE )

/* Routines for handling null values */

int sizeofNull( void );
void writeNull( STREAM *stream, int tag );
int _readNull( STREAM *stream, BOOLEAN readIdent );

#define readNull(stream)		_readNull( stream, TRUE )
#define readNullData(stream)	_readNull( stream, FALSE )

/* Routines for handling octet strings */

void newOctetString( OCTETSTRING *octetString, const BYTE *value, const int length );
void deleteOctetString( OCTETSTRING *octetString );
void assignOctetString( OCTETSTRING *octetString, const BYTE *value, const int length );
BOOLEAN evalOctetString( OCTETSTRING *octetString );
int sizeofOctetString( const OCTETSTRING *octetString );
void writeOctetString( STREAM *stream, const OCTETSTRING *octetString, int tag );
int _readOctetString( STREAM *stream, OCTETSTRING *octetString, BOOLEAN readIdent );

#define readOctetString(stream,octetString) \
		_readOctetString( stream, octetString, TRUE )
#define readOctetStringData(stream,octetString) \
		_readOctetString( stream, octetString, FALSE )

/* Routines for handling bit strings */

void newBitString( BITSTRING *bitString, const BYTE *value, const int length );
void deleteBitString( BITSTRING *bitString );
void assignBitString( BITSTRING *bitString, const BYTE *value, const int length );
int sizeofBitString( const BITSTRING *bitString );
void writeBitString( STREAM *stream, const BITSTRING *bitString, int tag );
int _readBitString( STREAM *stream, BITSTRING *bitString, BOOLEAN readIdent );

#define readBitString(stream,bitString) \
		_readBitString( stream, bitString, TRUE )
#define readBitStringData(stream,bitString) \
		_readBitString( stream, bitString, FALSE )

/* Routines for handling ISO 646 strings */

int sizeofISO646string( const char *iso646string );
void writeISO646string( STREAM *stream, const char *iso646string, int tag );
int _readISO646string( STREAM *stream, char *iso646string, BOOLEAN readIdent );

#define readISO646string(stream,iso646string) \
		_readISO747String( stream, iso646string, TRUE )
#define readISO646stringData(stream,iso646string) \
		_readISO747String( stream, iso646string, FALSE )

/* Routines for handling object identifiers */

int sizeofObjectIdentifier( const OBJECT_IDENTIFIER *objectIdentifier );
void writeObjectIdentifier( STREAM *stream, const OBJECT_IDENTIFIER *objectIdentifier, int tag );
int _readObjectIdentifier( STREAM *stream, OBJECT_IDENTIFIER *objectIdentifier );

#define readObjectIdentifier(stream,objectIdentifier) \
		_readObjectIdentifier( stream, objectIdentifier, TRUE )
#define readObjectIdentifierData(stream,objectIdentifier) \
		_readObjectIdentifier( stream, objectIdentifier, FALSE )

/* Routines for handling generalized time */

void newGeneralizedTime( GENERALIZED_TIME *generalizedTime,
						 const int years, const int months, const int days,
						 const int hours, const int minutes, const int seconds );
void deleteGeneralizedTime( GENERALIZED_TIME *generalizedTime );
void assignGeneralizedTime( GENERALIZED_TIME *generalizedTime,
							const int years, const int months, const int days,
							const int hours, const int minutes, const int seconds );
int sizeofGeneralizedTime( const GENERALIZED_TIME *generalizedTime );
void writeGeneralizedTime( STREAM *stream, const GENERALIZED_TIME *generalizedTime, int tag );
int _readGeneralizedTime( STREAM *stream, GENERALIZED_TIME *generalizedTime, BOOLEAN readIdent );

#define readGeneralizedTime(stream,generalizedTime) \
		_readGeneralizedTime( stream, generalizedTime, TRUE )
#define readGeneralizedTimeData(stream,generalizedTime) \
		_readGeneralizedTime( stream, generalizedTime, FALSE )

/* Routines for handling generalized strings.  Since this is a CHOICE type
   we don't have a Data form of the read() routine */

void newGeneralizedString( GENERALIZED_STRING *generalizedString,
						   BYTE *string, int length, int type );
void deleteGeneralizedString( GENERALIZED_STRING *generalizedString );
void assignGeneralizedString( GENERALIZED_STRING *generalizedString,
							  BYTE *string, int length, int type );
int sizeofGeneralizedString( const GENERALIZED_STRING *generalizedString );
void writeGeneralizedString( STREAM *stream, const GENERALIZED_STRING *generalizedString, int tag );
int readGeneralizedString( STREAM *stream, GENERALIZED_STRING *generalizedString );

/* Routines for handling time */

void newTime( TIME *time, time_t seconds, time_t fractional );
void deleteTime( TIME *time );
void assignTime( TIME *time, time_t seconds, time_t fractional );
BOOLEAN evalTime( TIME *time );
int sizeofTime( const TIME *time );
void writeTime( STREAM *stream, const TIME *time, int tag );
int _readTime( STREAM *stream, TIME *time, BOOLEAN readIdent );

#define readTime(stream,time)		_readTime( stream, time, TRUE )
#define readTimeData(stream,time)	_readTime( stream, time, FALSE )

/* Routines for handling permitted actions */

void newPermittedActions( PERMITTED_ACTIONS *permittedActions, LONG action );
void deletePermittedActions( PERMITTED_ACTIONS *permittedActions );
void assignPermittedActions( PERMITTED_ACTIONS *permittedActions, LONG action );
BOOLEAN evalPermittedActions( PERMITTED_ACTIONS *permittedActions );
int sizeofPermittedActions( const PERMITTED_ACTIONS *permittedActions );
void writePermittedActions( STREAM *stream, const PERMITTED_ACTIONS *permittedActions, int tag );
int _readPermittedActions( STREAM *stream, PERMITTED_ACTIONS *permittedActions, BOOLEAN readIdent );

#define readPermittedActions(stream,permittedActions) \
		_readPermittedActions( stream, permittedActions, TRUE )
#define readPermittedActionsData(stream,permittedActions) \
		_readPermittedActions( stream, permittedActions, FALSE )

/* Routines for handling accounting information */

void newAccountingInfo( ACCOUNTING_INFO *accountingInfo, int info );
void deleteAccountingInfo( ACCOUNTING_INFO *accountingInfo );
void assignAccountingInfo( ACCOUNTING_INFO *accountingInfo, int info );
BOOLEAN evalAccountingInfo( ACCOUNTING_INFO *accountingInfo );
int sizeofAccountingInfo( const ACCOUNTING_INFO *accountingInfo );
void writeAccountingInfo( STREAM *stream, const ACCOUNTING_INFO *accountingInfo, int tag );
int _readAccountingInfo( STREAM *stream, ACCOUNTING_INFO *accountingInfo, BOOLEAN readIdent );

#define readAccountingInfo(stream,accountingInfo) \
		_readAccountingInfo( stream, accountingInfo, TRUE )
#define readAccountingInfoData(stream,accountingInfo) \
		_readAccountingInfo( stream, accountingInfo, FALSE )

#endif /* !_ASN1_DEFINED */
