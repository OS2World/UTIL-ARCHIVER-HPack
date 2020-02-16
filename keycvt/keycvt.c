/* PGP 2.x secret key conversion utility.

   Creates key packets identical to the originals except that they're
	 encrypted with a different algorithm, and they use all the PKCS fields.
	 Useful to bypass the worldwide patents on IDEA, or if you don't trust it
	 (some people prefer DES), and to be more compatible with PKCS/PEM.

   Originally written in one long hacking session on 7 November 1992 by
	 Peter Gutmann (pgut1@cs.aukuni.ac.nz).
   Placed in the public domain like the PGP code itself.

   Known bugs:	Doesn't handle schizophrenic keys too well.  These are caused
					by a bug in PGP 2.0.
				There are several bugs in the PGP 2.0 format and format docs.
					The code contains some workarounds.  These have been
					fixed in 2.1 and 2.2.

   Misfeatures:	PGP doesn't have any provision for storing keys protected by
					multiple encryption algorithms.  keycvt and HPACK handle
					this by storing/using identical-looking keys with the
					second copy being protected by a different cipher and
					thus having a different algorithm ID, which causes PGP to
					skip the key.  This is a real kludge, but it's the only
					way to handle the problem until some sort of explicit
					handling mechanism is added to PGP.

   Changes: 	Updated to handle multiple-ID keys (the schizophrenic keys
					problem was fixed in PGP 2.1, so now if multiple-ID keys
					are encountered it's assumed they are valid).
				Updated to automatically generate the random seed file needed
					by HPACK if necessary.
				Updated to convert keys to proper PKCS format (but encoded in
					the PGP manner) for use by RSAREF code */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mdc.h"

/* Some old versions of stdio.h don't define fseek() origin codes */

#ifndef SEEK_SET
  #define SEEK_SET	0
  #define SEEK_CUR	1
  #define SEEK_END	2
#endif /* !SEEK_SET */

/* Buffering with getchar() is extremely braindamaged.  Under DOS and OS/2
   we can use getch(), otherwise use the ostrich algorithm */

#ifdef __MSDOS__
  int getch( void );

  #undef getchar
  #define getchar	getch
#endif /* __MSDOS__ */

#if defined (__AMIGA__)
  int getch( void );

  #undef getchar
  #define getchar	getch

int mytoupper( int c )
	{
	return( toupper( c ) );
	}

#undef toupper
#define toupper		mytoupper
#endif /* __AMIGA__ */

/* Magic numbers for PGP 2.0 keyrings */

#define PGP_CTB_SECKEY		0x95	/* Secret key packet CTB */
#define PGP_CTB_TRUST		0xB0	/* Trust packet */
#define PGP_CTB_USERID		0xB4	/* Userid packet CTB */

#define PGP2_VERSION_BYTE	2		/* Version number byte for PGP 2.0 */
#define PGP3_VERSION_BYTE	3		/* Version number byte for PGP 3.0 */

#define PKE_ALGORITHM_RSA	1		/* RSA public-key encryption algorithm */

#define CKE_ALGORITHM_NONE	0		/* No CKE algorithm */
#define CKE_ALGORITHM_IDEA	1		/* IDEA cipher */
#define CKE_ALGORITHM_MDC	64		/* MDC cipher */

/* The size of the IDEA IV */

#define IDEA_IV_SIZE		8

/* Storage for MPI's */

#define MAX_MPI_SIZE		256

BYTE n[ MAX_MPI_SIZE ], e[ MAX_MPI_SIZE ], d[ MAX_MPI_SIZE ];
BYTE p[ MAX_MPI_SIZE ], q[ MAX_MPI_SIZE ], u[ MAX_MPI_SIZE ];
BYTE exponent1[ MAX_MPI_SIZE ], exponent2[ MAX_MPI_SIZE ];
int nLen, eLen, dLen, pLen, qLen, uLen, exponent1Len, exponent2Len;

/* Information needed by the IDEA cipher */

char password[ 80 ];			/* The user's password */
BYTE ideaKey[ 16 ];				/* IDEA cipher key */
BYTE ideaIV[ IDEA_IV_SIZE ];	/* IDEA cipher IV */

/* Prototypes for functions in IDEA.C */

void initcfb_idea( BYTE iv[ IDEA_IV_SIZE ], BYTE ideakey[ 16 ], BOOLEAN decrypt );
void ideacfb( BYTE *buffer, int count );
void close_idea( void );

/* The buffers needed for the MDC encryption */

BYTE cryptBuffer[ 2048 ], outBuffer[ 2048 ];

/* Whether we've converted any keys or not */

BOOLEAN keysConverted;

/* Whether we need to set the global precision for MP routines */

BOOLEAN doSetPrecision;

/* The endianness for the CPU we're running on */

BOOLEAN bigEndian;

/* Storage for userID's.  Just use a simple static array rather than a more
   elegant dynamically allocated linked list */

char userID[ 15 ][ 256 ];

/****************************************************************************
*																			*
*									CRC Routines							*
*																			*
****************************************************************************/

/* The crc16 table and crc16 variable itself */

WORD crc16tbl[ 256 ];
WORD crc16;

/* The block crc16 calculation routine.  Ideally this should be done in
   assembly language for speed */

void crc16buffer( BYTE *bufPtr, int length )
	{
	while( length-- )
		crc16 = crc16tbl[ ( BYTE ) crc16 ^ *bufPtr++ ] ^ ( crc16 >> 8 );
	}

/* The initialisation routine for the crc16 table */

void initCRC16( void )
	{
	int bitCount, tblIndex;
	WORD crcVal;

	for( tblIndex = 0; tblIndex < 256; tblIndex++ )
		{
		crcVal = tblIndex;
		for( bitCount = 0; bitCount < 8; bitCount++ )
			if( crcVal & 0x01 )
				crcVal = ( crcVal >> 1 ) ^ 0xA001;
			else
				crcVal >>=  1;
		crc16tbl[ tblIndex ] = crcVal;
		}
	}

/****************************************************************************
*																			*
*							Byte/Word/Long I/O Routines						*
*																			*
****************************************************************************/

/* Routines to read BYTE, WORD, LONG */

BYTE fgetByte( FILE *inFilePtr )
	{
	return( ( BYTE ) getc( inFilePtr ) );
	}

WORD fgetWord( FILE *inFilePtr )
	{
	WORD value;

	value = ( ( WORD ) getc( inFilePtr ) ) << 8;
	value |= ( WORD ) getc( inFilePtr );
	return( value );
	}

LONG fgetLong( FILE *inFilePtr )
	{
	LONG value;

	value = ( ( LONG ) getc( inFilePtr ) ) << 24;
	value |= ( ( LONG ) getc( inFilePtr ) ) << 16;
	value |= ( ( LONG ) getc( inFilePtr ) ) << 8;
	value |= ( LONG ) getc( inFilePtr );
	return( value );
	}

/* Routines to write BYTE, WORD, LONG */

void fputByte( FILE *outFilePtr, BYTE data )
	{
	putc( data, outFilePtr );
	}

void fputWord( FILE *outFilePtr, WORD data )
	{
	putc( ( BYTE ) ( data >> 8 ), outFilePtr );
	putc( ( BYTE ) data, outFilePtr );
	}

void fputLong( FILE *outFilePtr, LONG data )
	{
	putc( ( BYTE ) ( data >> 24 ), outFilePtr );
	putc( ( BYTE ) ( data >> 16 ), outFilePtr );
	putc( ( BYTE ) ( data >> 8 ), outFilePtr );
	putc( ( BYTE ) data, outFilePtr );
	}

/****************************************************************************
*																			*
*							Multiprecision Maths Routines					*
*																			*
****************************************************************************/

/* Macros to convert a bitcount to various other values */

#define UNITSIZE		8			/* 8 bits per byte */

#define bits2units(n)	( ( ( n ) + 7 ) >> 3 )
#define units2bits(n)	( ( n ) << 3 )
#define bits2bytes(n)	( ( ( n ) + 7 ) >> 3 )

/* Defines for some of the MP primitives */

#define mp_move(dest,src)	memcpy( dest, src, globalPrecision * sizeof( BYTE ) )
#define mp_clear(reg,count)	memset( reg, 0, ( count ) * sizeof( BYTE ) )
#define normalize(mpi)		mpi += significance( mpi ) - 1

/* Level of precision for all routines */

int globalPrecision;

/* Returns number of significant units in mpReg */

static int significance( BYTE *mpReg )
	{
	int precision = globalPrecision;

	/* Parse mpReg arschlings */
	mpReg += precision - 1;
	do
		if( *mpReg-- )
			return( precision );
	while( --precision );

	return( 0 );
	}

/* Begin sniffing at the MSB */

BYTE bitMask;

static int initBitSniffer( BYTE *mpReg )
	{
	int bits, precision = significance( mpReg );

	bitMask = 0x80;
	if( !precision )
		return( 0 );
	bits = units2bits( precision );
	mpReg += precision - 1;
	while( !( *mpReg & bitMask ) )
		{
		bitMask >>= 1;
		bits--;
		}
	return( bits );
	}

/* Init multiprecision register mpReg with short value.  Note that mp_init
   doesn't extend sign bit for >32767 */

static void mp_init( BYTE *mpReg, WORD value )
	{
	*mpReg++ = value;
	mp_clear( mpReg, globalPrecision - 1 );
	}

/* Compares multiprecision registers r1 and r2, and returns -1 if r1 < r2,
   0 if r1 == r2, or +1 if r1 > r2 */

static int mp_compare( BYTE *r1, BYTE *r2 )
	{
	int precision = globalPrecision;	/* Number of units to compare */

	r1 += precision - 1;
	r2 += precision - 1;	/* Point to MSB's of registers */
	do
		{
		if( *r1 < *r2 )
			return( -1 );	/* r1 < r2 */
		if( *r1-- > *r2-- )
			return( 1 );	/* r1 > r2 */
		}
	while( --precision );

	return( 0 );			/* r1 == r2 */
	}

/* Decrement MP register mpReg */

static void mp_dec( BYTE *mpReg )
	{
	int precision = globalPrecision;	/* Number of units to decrement */

	do
		{
		if( ( *mpReg )-- )
			return;
		mpReg++;
		}
	while( --precision );
	}

/* Set the precision to which MP numbers are calculated */

static int precMod8, precDiv8;		/* Global precision mod 8, div 8 */

static void setPrecision( const int precision )
	{
	/* Set global precision */
	globalPrecision = precision;

	precMod8 = globalPrecision & 7;
	precDiv8 = globalPrecision >> 3;
	}

/* Subtract one MP number from another */

static int mp_sub( BYTE *reg1, BYTE *reg2 )
	{
	int count = precDiv8, borrow = 0;
	long value;

	switch( precMod8 )
		{
		case 0:
			do {
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100 ) ? 1 : 0;
			reg1++;
			reg2++;

		case 7:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 6:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 5:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 4:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 3:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 2:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;

		case 1:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1 -= *reg2 + borrow;
			borrow = ( value & 0x0100L ) ? 1 : 0;
			reg1++;
			reg2++;
			} while( count-- > 0 );
		}

	return( borrow );
	}

/* Rotate a MP number */

static void mp_rotateleft( BYTE *reg, const int carry )
	{
	int carryIn = carry, carryOut, count = precDiv8;

	switch( precMod8 )
		{
		case 0:
			do {
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 7:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 6:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 5:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 4:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 3:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 2:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 1:
			carryOut = ( *reg & 0x80 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;
			} while( count-- > 0 );
		}
	}

/* MP support routines */

#define sniffBit(exp)		( *( exp ) & bitMask )
#define bumpBitSniffer(exp)	if( !( bitMask >>= 1 ) ) \
								{ \
								bitMask = 0x80; \
								exp--; \
								}

/* Unsigned divide routine */

static void mp_mod( BYTE *remainder, BYTE *dividend, BYTE *divisor )
	{
	int bits;

	mp_init( remainder, 0 );

	/* Compute number of bits in dividend and normalize it */
	bits = initBitSniffer( dividend );
	normalize( dividend );
	while( bits-- )
		{
		mp_rotateleft( remainder, ( sniffBit( dividend ) != 0 ) ? 1 : 0 );
		if( mp_compare( remainder, divisor ) >= 0 )
			mp_sub( remainder, divisor );
		bumpBitSniffer( dividend );
		}
	}

/****************************************************************************
*																			*
*					Multiprecision Integer I/O Routines						*
*																			*
****************************************************************************/

/* Read an MPI, setting the global precision if necessary */

static void readMPI( FILE *inFilePtr, int *mpiLen, BYTE *mpReg )
	{
	BYTE *regPtr;
	int length;

	*mpiLen = fgetWord( inFilePtr );
	length = bits2bytes( *mpiLen );
	if( doSetPrecision )
		{
		/* Set the precision to that specified by the number read */
		setPrecision( length );
		doSetPrecision = FALSE;
		}
	regPtr = mpReg;
	while( length-- )
		*regPtr++ = fgetByte( inFilePtr );
	}

/* Write an MPI */

static void writeMPI( FILE *outFilePtr, int mpiLen, BYTE *mpReg )
	{
	int length = bits2bytes( mpiLen );

	fputWord( outFilePtr, ( WORD ) mpiLen );
	while( length-- )
		fputByte( outFilePtr, *mpReg++ );
	}

/* Store an MPI in an in-memory buffer */

static int storeMPI( BYTE *buffer, int mpiLen, BYTE *mpReg )
	{
	int length = bits2bytes( mpiLen );

	*buffer++ = ( BYTE ) ( mpiLen >> 8 );
	*buffer++ = ( BYTE ) mpiLen;
	memcpy( buffer, mpReg, length );
	return( ( int ) ( sizeof( WORD ) + length ) );
	}

/* Endianness-reverse an MPI */

static void byteReverse( BYTE *regPtr, int count )
	{
	int sourceCount = 0;
	BYTE temp;

	/* Swap endianness of MPI */
	for( count--; count > sourceCount; count--, sourceCount++ )
		{
		temp = regPtr[ sourceCount ];
		regPtr[ sourceCount ] = regPtr[ count ];
		regPtr[ count ] = temp;
		}
	}

/****************************************************************************
*																			*
*								Key Conversion Routines						*
*																			*
****************************************************************************/

/* IDEA support code needed by convertPrivateKey().  Note that the IV must
   be declared static since the IDEA code sets up a pointer to it and uses
   it for storage (ick!) */

static void getPassword( void )
	{
	puts( "Please enter password for this private key." );
	puts( "Warning: Password will be echoed to screen!" );
	printf( "Password: " );
	fflush( stdout );
	gets( password );
	}

void initCrypt( void )
	{
	MD5_CTX	mdContext;
	static BYTE iv[ IDEA_IV_SIZE ];

	/* Get the password from the (l)user */
	getPassword();

	/* Reduce it to 128 bits using MD5 */
	MD5Init( &mdContext );
	MD5Update( &mdContext, ( BYTE * ) password, strlen( password ) );
	MD5Final( &mdContext );

	/* Set up IDEA key */
	memset( iv, 0, IDEA_IV_SIZE );
	initcfb_idea( iv, mdContext.digest, TRUE );
	ideacfb( ideaIV, 8 );
	}

/* Skip to the start of the next key packet block */

void skipToKeyPacket( FILE *inFilePtr, const int length )
	{
	BYTE ctb;

	/* First skip the rest of the current packet if necessary */
	if( length )
		fseek( inFilePtr, length, SEEK_CUR );

	/* Now skip any following userID packets */
	while( ( ctb = fgetByte( inFilePtr ) ) == PGP_CTB_USERID )
		fseek( inFilePtr, ( long ) fgetByte( inFilePtr ), SEEK_CUR );

	/* Finally, put back the last CTB we read */
	ungetc( ctb, inFilePtr );
	}

/* Convert a private key packet from PGP 1.0 to PGP 2.0 format */

WORD checksum( BYTE *data, int length )
	{
	WORD checkSum = ( ( BYTE ) ( length >> 8 ) ) + ( ( BYTE ) length );

	length = bits2bytes( length );
	while( length-- )
		checkSum += *data++;
	return( checkSum );
	}

void convertPrivateKey( FILE *inFilePtr, FILE *outFilePtr )
	{
	BOOLEAN isEncrypted;
	LONG timeStamp;
	WORD checkSum, packetChecksum, validityPeriod;
	int length, i, packetLength, userIDindex = 0;
	struct tm *localTime;
	BYTE *outBufPtr = outBuffer, *mdcIV, *ivPtr, ctb;
	BYTE tmp[ MAX_MPI_SIZE ], dTmp[ MAX_MPI_SIZE ];

	/* Skip CTB, packet length, and version byte.
	   Note the 0xFF check, which is necessary for AIX's broken fgetc() */
	if( ( i = fgetc( inFilePtr ) ) == EOF || i == 0xFF )
		return;
	packetLength = fgetWord( inFilePtr );
	if( fgetByte( inFilePtr ) != PGP2_VERSION_BYTE )
		{
		/* Unknown version number, skip this packet */
		puts( "Skipping unknown packet type..." );
		skipToKeyPacket( inFilePtr, packetLength - sizeof( BYTE ) );
		return;
		}
	packetLength -= sizeof( BYTE );

	/* Read timestamp, validity period */
	timeStamp = fgetLong( inFilePtr );
	validityPeriod = fgetWord( inFilePtr );
	packetLength -= sizeof( LONG ) + sizeof( WORD );

	/* Read public key components */
	doSetPrecision = TRUE;		/* Remember MPI precision */
	if( fgetByte( inFilePtr ) != PKE_ALGORITHM_RSA )
		{
		/* Unknown PKE algorithm type, skip this packet */
		puts( "Skipping unknown PKE algorithm packet..." );
		skipToKeyPacket( inFilePtr, packetLength - sizeof( BYTE ) );
		return;
		}
	readMPI( inFilePtr, &nLen, n );
	readMPI( inFilePtr, &eLen, e );
	packetLength -= sizeof( BYTE ) + sizeof( WORD ) + bits2bytes( nLen ) + \
									 sizeof( WORD ) + bits2bytes( eLen );

	/* Handle decryption info for secret components if necessary */
	if( ( ctb = fgetByte( inFilePtr ) ) == CKE_ALGORITHM_MDC )
		{
		puts( "Key has already been converted, skipping packet..." );
		skipToKeyPacket( inFilePtr, packetLength - sizeof( BYTE ) );
		return;
		}
	isEncrypted = ( ctb == CKE_ALGORITHM_IDEA );
	if( isEncrypted )
		for( i = 0; i < IDEA_IV_SIZE; i++ )
			ideaIV[ i ] = fgetc( inFilePtr );
/*	packetLength -= sizeof( BYTE ) + ( ( isEncrypted ) ? IDEA_IV_SIZE : 0 ); */

	/* Read in private key components and checksum */
	readMPI( inFilePtr, &dLen, d );
	readMPI( inFilePtr, &pLen, p );
	readMPI( inFilePtr, &qLen, q );
	readMPI( inFilePtr, &uLen, u );
/*	packetLength -= sizeof( WORD ) + bits2bytes( dLen ) + \
					sizeof( WORD ) + bits2bytes( pLen ) + \
					sizeof( WORD ) + bits2bytes( qLen ) + \
					sizeof( WORD ) + bits2bytes( uLen ); */
	packetChecksum = fgetWord( inFilePtr );
/*	packetLength -= sizeof( WORD ); */

	/* Read the userID packet */
	while( TRUE )
		{
		if( ( ctb = fgetByte( inFilePtr ) ) != PGP_CTB_USERID )
			{
			/* Check whether we may have found a keyring trust packet
			   (PGP 2.0 bug) */
			if( ctb == PGP_CTB_TRUST )
				{
				/* Remember it for later */
				fgetByte( inFilePtr );	/* Skip length */
				fgetByte( inFilePtr );	/* Skip null trust info */
				ctb = fgetByte( inFilePtr );
				}

			/* Check if we've got a userID packet now */
			if( ctb != PGP_CTB_USERID )
				{
				/* If we've already got at least one userID, continue */
				if( userIDindex )
					{
					ungetc( ctb, inFilePtr );
					break;
					}

				/* We still don't have a userID CTB, complain */
				puts( "Can't find userID packet after key packet, skipping..." );
				skipToKeyPacket( inFilePtr,  ( int ) fgetByte( inFilePtr ) );
				return;
				}
			}
		length = fgetByte( inFilePtr );
		for( i = 0; i < length; i++ )
			userID[ userIDindex ][ i ] = fgetByte( inFilePtr );
		userID[ userIDindex++ ][ i ] = '\0';
		}

	/* Check whether we may have found a keyring trust packet (another bug) */
	if( ( ctb = fgetByte( inFilePtr ) ) == PGP_CTB_TRUST )
		{
		/* Remember it for later */
		fgetByte( inFilePtr );	/* Skip length */
		fgetByte( inFilePtr );	/* Skip null trust info */
		}
	else
		ungetc( ctb, inFilePtr );

	/* Display the key and ask the user if they want to convert it */
	localTime = localtime( ( time_t * ) &timeStamp );
	printf( "%d bits, date %02d/%02d/%02d, main userID '%s'\n", nLen, \
			localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year, \
			userID );
	printf( "Add information for this key so HPACK can use it (y/n) " );
	if( toupper( getchar() ) == 'N' )
		{
#if defined( __MSDOS__ ) || defined( __AMIGA__ )
		putchar( '\n' );
#else
		getchar();	/* Fix getchar() problem */
#endif /* __MSDOS__ || __AMIGA__ */
		puts( "Skipping key..." );
		return;
		}

#if defined( __MSDOS__ ) || defined( __AMIGA__ )
	putchar( '\n' );
#else
	getchar();	/* Try and fix some of getchar()'s problems */
#endif /* __MSDOS__ || __AMIGA__ */

	/* Decrypt the secret-key fields if necessary */
	if( isEncrypted )
		for( i = 0; ; i++ )
			{
			/* Attempt to decrypt the secret-key fields */
			initCrypt();
			ideacfb( d, bits2bytes( dLen ) );
			ideacfb( p, bits2bytes( pLen ) );
			ideacfb( q, bits2bytes( qLen ) );
			ideacfb( u, bits2bytes( uLen ) );

			/* Make sure all was OK */
			checkSum = checksum( d, dLen );
			checkSum += checksum( p, pLen );
			checkSum += checksum( q, qLen );
			checkSum += checksum( u, uLen );
			if( checkSum != packetChecksum )
				{
				/* If they still haven't got it right after 3 attempts,
				   give up */
				if( i == 3 )
					{
					puts( "Can't decrypt key packet, skipping..." );
					return;
					}

				puts( "Incorrect checksum, possibly due to incorrect password" );
				}
			else
				break;
			}
	else
		/* Get the password for encryption, since we didn't need to get one
		   for decryption */
		getPassword();

	/* Evaluate exponent1 = d mod ( p - 1 ) and exponent2 = d mod ( q - 1 ),
	   needed for PKCS compatibility */
	mp_move( dTmp, d );
	byteReverse( dTmp, bits2bytes( dLen ) );
	mp_move( tmp, p );
	byteReverse( tmp, bits2bytes( pLen ) );
	mp_dec( tmp );
	mp_mod( exponent1, dTmp, tmp );
	exponent1Len = initBitSniffer( exponent1 );
	byteReverse( exponent1, bits2bytes( exponent1Len ) );
	exponent1Len = initBitSniffer( exponent1 );
	mp_move( tmp, q );
	byteReverse( tmp, bits2bytes( qLen ) );
	mp_dec( tmp );
	mp_mod( exponent2, dTmp, tmp );
	exponent2Len = initBitSniffer( exponent2 );
	byteReverse( exponent2, bits2bytes( exponent2Len ) );
	exponent2Len = initBitSniffer( exponent2 );

	/* Write the PGP 2.0 header information */
	fputByte( outFilePtr, PGP_CTB_SECKEY );
	fputWord( outFilePtr, ( WORD ) (
						  sizeof( BYTE ) + \
						  sizeof( LONG ) + sizeof( WORD ) + sizeof( BYTE ) + \
						  sizeof( WORD ) + bits2bytes( nLen ) + \
						  sizeof( WORD ) + bits2bytes( eLen ) + \
						  sizeof( BYTE ) + IV_SIZE + \
						  sizeof( WORD ) + bits2bytes( dLen ) + \
						  sizeof( WORD ) + bits2bytes( pLen ) + \
						  sizeof( WORD ) + bits2bytes( qLen ) + \
						  sizeof( WORD ) + bits2bytes( uLen ) + \
						  sizeof( WORD ) + bits2bytes( exponent1Len ) + \
						  sizeof( WORD ) + bits2bytes( exponent2Len ) + \
						  sizeof( WORD ) ) );
	fputByte( outFilePtr, PGP2_VERSION_BYTE );

	/* Write timestamps, algorithm byte */
	fputLong( outFilePtr, timeStamp );
	fputWord( outFilePtr, validityPeriod );
	fputByte( outFilePtr, PKE_ALGORITHM_RSA );

	/* Write public key components */
	writeMPI( outFilePtr, nLen, n );
	writeMPI( outFilePtr, eLen, e );

	/* Write secret key components */
	fputByte( outFilePtr, CKE_ALGORITHM_MDC );
	mdcIV = ivPtr = getIV();
	for( i = 0; i < IV_SIZE; i++ )
		fputByte( outFilePtr, *ivPtr++ );
	outBufPtr += storeMPI( outBufPtr, dLen, d );
	outBufPtr += storeMPI( outBufPtr, pLen, p );
	outBufPtr += storeMPI( outBufPtr, qLen, q );
	outBufPtr += storeMPI( outBufPtr, uLen, u );
	outBufPtr += storeMPI( outBufPtr, exponent1Len, exponent1 );
	outBufPtr += storeMPI( outBufPtr, exponent2Len, exponent2 );
	length = ( int ) ( outBufPtr - outBuffer );

	/* Write secret key checksum */
	crc16 = 0;
	crc16buffer( outBuffer, length );
	outBuffer[ length++ ] = ( BYTE ) ( crc16 >> 8 );
	outBuffer[ length++ ] = ( BYTE ) crc16;

	/* Encrypt the secret key components and write them */
	initKey( ( BYTE * ) password, strlen( password ), mdcIV );
	encryptCFB( outBuffer, length );
	fwrite( outBuffer, length, 1, outFilePtr );

	/* Write the userID packets for the preceding key */
	for( i = 0; i < userIDindex; i++ )
		{
		fputByte( outFilePtr, PGP_CTB_USERID );
		length = strlen( userID[ i ] );
		fputByte( outFilePtr, ( BYTE ) length );
		fwrite( userID[ i ], length, 1, outFilePtr );
		}

	/* Zap secret key components, temporary register, and encryption
	   information */
	memset( d, 0, MAX_MPI_SIZE );
	memset( p, 0, MAX_MPI_SIZE );
	memset( q, 0, MAX_MPI_SIZE );
	memset( u, 0, MAX_MPI_SIZE );
	memset( exponent1, 0, MAX_MPI_SIZE );
	memset( exponent2, 0, MAX_MPI_SIZE );
	memset( tmp, 0, MAX_MPI_SIZE );
	memset( password, 0, 80 );
	close_idea();

	keysConverted = TRUE;
	}

int main( const int argc, const char *argv[] )
	{
	FILE *inFilePtr, *outFilePtr;
	char tempFileName[ 256 ], *outFileName;
	char *pgpPath;
	long fileSize;
	int ch, i, pathLen;

	puts( "HPACK keyring format converter.  For HPACK 0.79 and above.\n" );
	if( argc == 2 || argc == 3 )
		{
		if( ( inFilePtr = fopen( argv[ 1 ], "rb" ) ) == NULL )
			{
			perror( argv[ 1 ] );
			exit( ERROR );
			}
		outFileName = ( char * ) ( ( argc == 3 ) ? argv[ 2 ] : argv[ 1 ] );
		strcpy( tempFileName, outFileName );
		strcpy( tempFileName + strlen( tempFileName ) - 1, "_" );
		if( ( outFilePtr = fopen( tempFileName, "wb" ) ) == NULL )
			{
			perror( tempFileName );
			exit( ERROR );
			}
		}
	else
		{
		puts( "Usage: keycvt <PGP secret keyring> [<output keyfile>]" );
		puts( "\nThe PGP secret keyring may contain one or more secret keys.  For each key" );
		puts( " you will be asked whether you wish to add an HPACK-readable version to the" );
		puts( " keyring.  Adding the HPACK-readable version will add an extra key packet" );
		puts( " which is used by HPACK." );
		puts( "\n If an optional output keyfile name is supplied, the extra information" );
		puts( " will be written to the output file instead of being added to the keyring." );
		puts( "\nIn addition, keycvt will create the seed file needed by HPACK in the same" );
		puts( " directory as the PGP seed file if it doesn't already exist" );
		exit( OK );
		}

	/* Determine processor endianness.  If the machine is big-endian (up
	   to 64 bits) the following value will be signed, otherwise it will
	   be unsigned.  Unfortunately we can't test for things like middle-
	   endianness without knowing the size of the data types */
	bigEndian = ( *( int * ) "\x80\x00\x00\x00\x00\x00\x00\x00" < 0 );

	/* Set up CRC table */
	initCRC16();

	/* Find out whether we're handling a public or private keyring */
	ch = fgetc( inFilePtr );
	ungetc( ch, inFilePtr );
	if( ch != PGP_CTB_SECKEY )
		{
		printf( "%s doesn't appear to be a PGP secret key file.\n", argv[ 1 ] );
		exit( ERROR );
		}
	else
		{
		/* Convert all the keys in the keyring */
		keysConverted = FALSE;
		while( !feof( inFilePtr ) )
			{
			convertPrivateKey( inFilePtr, outFilePtr );
			putchar( '\n' );
			}
		}

	/* Check before we add the new packets to the output keyring */
	fclose( inFilePtr );
	fclose( outFilePtr );
	if( keysConverted )
		{
		printf( "Finished processing keys. Add new key information to output keyring (y/n) " );
		fflush( stdout );
		if( toupper( getchar() ) == 'N' )
			{
			/* Clean up and exit */
			remove( tempFileName );
#if defined( __MSDOS__ ) || defined( __AMIGA__ )
			putchar( '\n' );
#endif /* __MSDOS__ || __AMIGA__ */
			puts( "\nOutput file left unchanged" );
			exit( ERROR );
			}
#if defined( __MSDOS__ ) || defined( __AMIGA__ )
		putchar( '\n' );
#endif /* __MSDOS__ || __AMIGA__ */

		/* Append the new packets to the end of the output keyring */
		if( ( inFilePtr = fopen( tempFileName, "rb" ) ) == NULL )
			{
			puts( "Cannot open temporary work file" );
			exit( ERROR );
			}
		if( ( outFilePtr = fopen( outFileName, "ab" ) ) == NULL )
			{
			perror( outFileName );
			exit( ERROR );
			}
		fseek( inFilePtr, 0L, SEEK_END );
		fileSize = ftell( inFilePtr );
		fseek( inFilePtr, 0L, SEEK_SET );
		while( fileSize-- )
			fputc( fgetc( inFilePtr ), outFilePtr );
		fclose( inFilePtr );
		fclose( outFilePtr );
		}
	else
		puts( "No keys converted, output file left unchanged." );

	/* Remove work file */
	remove( tempFileName );

	/* Try and find the PGP seed file */
	if( ( pgpPath = getenv( "PGPPATH" ) ) == NULL )
		pgpPath = "";
	pathLen = strlen( pgpPath );
	strcpy( tempFileName, pgpPath );
#if defined( __ARC__ )
	if( pathLen && tempFileName[ pathLen - 1 ] != '.' )
		/* Add directory seperator if necessary */
		tempFileName[ pathLen++ ] = '.';
	strcat( tempFileName, "randseed" );
#elif defined( __AMIGA__ )
	if( pathLen && ( ch = tempFileName[ pathLen - 1 ] ) != ':' && ch != '/' )
		/* Add directory seperator if necessary */
		tempFileName[ pathLen++ ] = '/';
	strcat( tempFileName, "randseed.bin" );
#else
	if( pathLen && ( ch = tempFileName[ pathLen - 1 ] ) != '\\' && ch != '/' )
		/* Add directory seperator if necessary */
		tempFileName[ pathLen++ ] = '/';
	strcat( tempFileName, "randseed.bin" );
#endif /* __ARC__ */
	if( ( inFilePtr = fopen( tempFileName, "rb" ) ) == NULL )
		puts( "Cannot find PGP seed file, HPACK seed file not created" );
	else
		{
		/* See if the output file exists */
#ifdef __ARC__
		strcpy( tempFileName + pathLen, "hpakseed" );
#else
		strcpy( tempFileName + pathLen, "hpakseed.bin" );
#endif /* __ARC__ */
		if( ( outFilePtr = fopen( tempFileName, "r" ) ) == NULL )
			{
			outFilePtr = fopen( tempFileName, "wb" );
			for( i = 0; i < 16; i++ )
				fputc( fgetc( inFilePtr ), outFilePtr );
			puts( "HPACK seed file created" );
			}
		else
			puts( "Existing HPACK seed file left unchanged" );

		fclose( inFilePtr );
		fclose( outFilePtr );
		}

	/* Finish up */
	puts( "\nDone" );
	return( OK );
	}
