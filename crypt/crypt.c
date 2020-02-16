/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								Encryption Routines							*
*							 CRYPT.C  Updated 11/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* "Modern cryptography is nothing more than a mathematical framework for
	debating the implications of various paranoid delusions".
												- Don Alvarez */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "choice.h"
#include "error.h"
#include "filesys.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "crypt\keymgmt.h"
  #include "crypt\md5.h"
  #include "crypt\mdc.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "keymgmt.h"
  #include "md5.h"
  #include "mdc.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
#else
  #include "crc/crc16.h"
  #include "crypt/keymgmt.h"
  #include "crypt/md5.h"
  #include "crypt/mdc.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* These files use the environmental variable PEMPATH as a default path */

#ifdef __ARC__
char PEM_PUBKEY_FILENAME[] = "ripemprv";
char PEM_SECKEY_FILENAME[] = "rpubkeys";
#else
char PEM_PUBKEY_FILENAME[] = "ripemprv";
char PEM_SECKEY_FILENAME[] = "rpubkeys";
#endif /* __ARC__ */

/* These files use the environmental variable PGPPATH as a default path */

#ifdef __ARC__
char PGP_PUBKEY_FILENAME[] = "pubring";
char PGP_SECKEY_FILENAME[] = "secring";
#else
char PGP_PUBKEY_FILENAME[] = "pubring.pgp";
char PGP_SECKEY_FILENAME[] = "secring.pgp";
#endif /* __ARC__ */

/* The encryption passwords */

char key[ MAX_KEYLENGTH + 1 ], secKey[ MAX_KEYLENGTH + 1 ];
int keyLength = 0, secKeyLength = 0;

/* The following are defined in MDC.C */

extern BYTE auxKey[];

/* Prototypes for functions in VIEWFILE.C */

void extractDate( const LONG time, int *date1, int *date2, int *date3,
								   int *hours, int *minutes, int *seconds );

/* Prototypes for functions in ARCHIVE.C */

void blankLine( int length );
void blankChars( int length );

/* Prototypes for functions in GUI.C */

char *getPassword( char *passWord, const char *prompt );
void showSig( const BOOLEAN isGoodSig, const char *userID, const LONG timeStamp );

#ifdef __MAC__
/* Prototypes for functions in MAC.C */

FD openResourceFork( const char *filePath, const int mode );
void closeResourceFork( const FD theFD );
#endif /* __MAC__ */

/* Prototypes for functions in RSA.C (not included in RSA.H because of
   recursive include file nesting problem */

void mp_modexp( MP_REG *expOut, MP_REG *expIn, MP_REG *exponent, MP_REG *modulus );
void rsaDecrypt( MP_REG *M, MP_REG *C, KEYINFO *privateKey );

/* Whether we're big-endian or not.  A lot of the encryption bit-twiddling
   code is very particular about this, mainly the core MD5 and RSA routines */

BOOLEAN bigEndian;

/* The general-purpose crypto buffer */

BYTE *cryptBuffer = NULL;

#if CRYPT_BUFSIZE < ( MAX_BYTE_PRECISION * 33 )
  #error Need to increase CRYPT_BUFSIZE
#endif /* Safety check for size of crypto buffer */

/* The keyInfo structures for the encryption and digital signature keys (made
   global to save on stack space, especially with very long keys) */

KEYINFO cryptKeyInfo, signKeyInfo;

/****************************************************************************
*																			*
* 						Initialise the Encryption System 					*
*																			*
****************************************************************************/

#ifndef GUI

char *getPassword( char *passWord, const char *prompt )
	{
	int i;
	char ch;

	while( TRUE )
		{
		hprintf( prompt );

		/* Get a password string */
		i = 0;
		while( ( ch = hgetch() ) != '\r' && ch != '\n' )
			{
			/* Handle backspace key.  The way this is handled is somewhat
			   messy, since hitting BS at the last char position deletes
			   the previous char */
			if( ch == '\b' )
				{
				if( i )
					i--;
				else
					hputchar( '\007' );
				continue;
				}

			if( i == MAX_KEYLENGTH )
				hputchar( '\007' );
			else
				passWord[ i++ ] = ch;
			}

		/* Exit if format is valid */
		if( i >= MIN_KEYLENGTH )
			break;

		hputs( MESG_KEY_INCORRECT_LENGTH );
		}

	passWord[ i ] = '\0';
	blankLine( screenWidth - 1 );

	return( passWord );
	}
#endif /* !GUI */

/* Set up the encryption password */

void initPassword( const BOOLEAN isMainKey )
	{
	char key1[ MAX_KEYLENGTH + 1 ], key2[ MAX_KEYLENGTH + 1 ];
	BOOLEAN exitNow = FALSE;

	/* Set up the password */
	getPassword( key1, ( isMainKey ) ? MESG_ENTER_PASSWORD : MESG_ENTER_SEC_PASSWORD );
	if( choice == ADD || choice == FRESHEN || choice == REPLACE || choice == UPDATE )
		exitNow = strcmp( key1, getPassword( key2, MESG_REENTER_TO_CONFIRM ) );

	/* Set up the appropriate encryption key */
	if( isMainKey )
		{
		strcpy( key, key1 );
		keyLength = strlen( key );
		}
	else
		{
		strcpy( secKey, key1 );
		secKeyLength = strlen( key );
		}

	/* Scrub temporary passwords */
	memset( key1, 0, MAX_KEYLENGTH + 1 );
	memset( key2, 0, MAX_KEYLENGTH + 1 );

	if( exitNow )
		error( PASSWORDS_NOT_SAME );
	}

/* Initialise the encryption system */

void initCrypt( void )
	{
	/* Determine processor endianness.  If the machine is big-endian (up
	   to 64 bits) the following value will be signed, otherwise it will
	   be unsigned.  Unfortunately we can't test for things like middle-
	   endianness without knowing the size of the data types */
	bigEndian = ( *( long * ) "\x80\x00\x00\x00\x00\x00\x00\x00" < 0 );

	/* Allocate general-purpose crypt buffer */
	if( ( cryptBuffer = ( BYTE * ) hmalloc( CRYPT_BUFSIZE ) ) == NULL )
		error( OUT_OF_MEMORY );

	/* Clear out ID fields in PKC keys */
	clearIDfields( &cryptKeyInfo );
	clearIDfields( &signKeyInfo );

	/* Clear out ID fields for rest of archiver */
	*signerID = *mainUserID = *secUserID = '\0';
	}

/* Shut down the encryption system */

void endCrypt( void )
	{
	/* Clear the MDC Mysterious Constants if necessary by resetting them to
	   the normal MD5 values */
	MD5SetConst( NULL );

	/* Scrub the encryption keys so other users can't find them by examining
	   core after HPACK has run */
	memset( key, 0, MAX_KEYLENGTH + 1 );
	memset( secKey, 0, MAX_KEYLENGTH + 1 );
	memset( auxKey, 0, MD5_BLOCKSIZE );
	memset( &cryptKeyInfo, 0, sizeof( KEYINFO ) );
	memset( &signKeyInfo, 0, sizeof( KEYINFO ) );

	/* Scrub the cryptBuffer if we've allocated it */
	if( cryptBuffer != NULL )
		{
		memset( cryptBuffer, 0, CRYPT_BUFSIZE );
		hfree( cryptBuffer );
		}
	}

/* Delete a file (if we're using encryption, "annihilate" would be a better
   term).  Vestigia nulla retrorsum */

void wipeFile( const char *filePath )
	{
	FILEINFO fileInfo;
	long fileSize, count;
	char renameFilePath[ MAX_PATH ];
	FD wipeFileFD;
#ifdef __MAC__
	FD resourceForkFD;
#endif /* __MAC__ */
	int i;

	/* Perform a simple delete if we're not encrypting the data */
	if( !( flags & CRYPT ) )
		{
		/* You'll NEVER get rid of *me*, Toddy */
		hunlink( filePath );
		return;
		}

	/* If we're encrypting the data, we need to comprehensively destroy the
	   file.  We do this by getting its size, overwriting it with a pattern
	   of ones and zeroes, breaking it up into sector-size chunks (presumably
	   scattered all over the disk), resetting its attributes and timestamp
	   to zero, renaming it to 'X', and finally deleting it.  The most anyone
	   can ever recover from this is that there was once some file on the disk
	   which HPACK has now deleted */
	findFirst( filePath, ALLFILES_DIRS, &fileInfo );
	findEnd( &fileInfo );
	fileSize = fileInfo.fSize;
	hchmod( filePath, CREAT_ATTR );		/* Make sure we can get at the file */
	if( ( wipeFileFD = hopen( filePath, O_WRONLY ) ) != IO_ERROR )
		{
		/* Overwrite the data with a block of random bytes.  We can't use any
		   sort of fixed pattern since a compressing filesystem will simply
		   compress these to virtually nothing and overwrite only a small
		   fraction of the file.  Note that the random data doesn't have to
		   be cryptographically strong, just noncompressible */
		srand( ( int ) time( NULL ) );
		for( i = 0; i < _BUFSIZE; i++ )
			_outBuffer[ i ] = ( BYTE ) rand();
		for( count = 0; count < fileSize; count++ )
			{
			hwrite( wipeFileFD, _outBuffer, ( count + _BUFSIZE < fileSize ) ?
					_BUFSIZE : ( int ) ( fileSize - count ) );
			count += _BUFSIZE;
			}

#ifndef __ATARI__
		/* Truncate the file every 512 bytes to break up the linked chain of
		   sectors */
		for( count = fileSize & ~0x1FFL; count >= 0; count -= 512 )
			{
			hlseek( wipeFileFD, count, SEEK_SET );
			htruncate( wipeFileFD );
			}
#endif /* !__ATARI__ */
#ifdef __MAC__
		/* Wipe the resource fork as well */
		if( ( resourceForkFD = openResourceFork( filePath, O_WRONLY ) ) != IO_ERROR )
			{
			for( count = 0; count < fileSize; count++ )
				{
				hwrite( resourceForkFD, _outBuffer, ( count + _BUFSIZE < fileSize ) ?
						_BUFSIZE : fileSize - count );
				count += _BUFSIZE;
				}
			for( count = fileSize; count >= 0; count -= 512 )
				{
				hlseek( resourceForkFD, count, SEEK_SET );
				htruncate( resourceForkFD );
				}
			closeResourceFork( resourceForkFD );
			}
#endif /* __MAC__ */
		hclose( wipeFileFD );
		}

	/* Rename the file, reset it's timestamp to zero, and delete it.  If a
	   file called 'X' already exists we just do it to the original file.
	   Note that the setting of the date can have interesting effects if the
	   epoch predates the epoch of the host filesystem */
	strcpy( renameFilePath, filePath );
	strcpy( findFilenameStart( renameFilePath ), "X" );
	if( hrename( filePath, renameFilePath ) != IO_ERROR )
		{
		setFileTime( renameFilePath, 0L );
		hunlink( renameFilePath );
		}
	else
		{
		setFileTime( filePath, 0L );
		hunlink( filePath );
		}

	/* An additional precaution would be to force a flush of the system cache,
	   but this is highly system-specific and will usually happen within a
	   few seconds anyway */
	}

/****************************************************************************
*																			*
* 							RSAREF Support Routines 						*
*																			*
****************************************************************************/

/* The following routines are needed by the US version of HPACK which doesn't
   include RSA.C, but uses the RSAREF toolkit instead.

   Internally RSAREF uses a little-endian numeric representation, but
   externally uses the PKCS big-endian format, so we have to convert
   everything into a canonical big-endian format before calling any RSAREF
   code, and back again afterwards.  This isn't helped by the fact that
   RSAREF stores key components at the maximum precision, but other data
   at whatever precision is necessary.

   RSAREF also uses large amounts of stack variables, so the RSAREF-format
   key structures are made static to save on stack usage */

#ifdef RSAREF

#if defined( __ATARI__ )
  #include "rsaref\global.h"
  #include "rsaref\rsaref.h"
#elif defined( __MAC__ )
  #include "global.h"
  #include "rsaref.h"
#else
  #include "rsaref/global.h"
  #include "rsaref/rsaref.h"
#endif /* System-specific include directives */

/* Level of precision for all routines */

int globalPrecision;

/* Unused function needed to keep RSAREF happy */

void R_GenerateBytes( void )
	{
	/* Do nothing */
	}

/* Memory routines normally included in r_stdlib.c */

void R_memset( BYTE *memPtr, int value, unsigned int size )
	{
	memset( memPtr, value, size );
	}

void R_memcpy( BYTE *dest, BYTE *src, unsigned int size )
	{
	memcpy( dest, src, size );
	}

int R_memcmp( BYTE *memPtr1, BYTE *memPtr2, unsigned int size )
	{
	return( memcmp( memPtr1, memPtr2, size ) );
	}

/* Init multiprecision register mpReg with short value */

void mp_init( MP_REG *mpReg, WORD value )
	{
	*mpReg++ = value;
	mp_clear( mpReg, globalPrecision - 1 );
	}

/* Returns number of significant units in mpReg.  Used solely by
   initBitSniffer() */

int significance( MP_REG *mpReg )
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

/* Begin sniffing at the MSB.  Used to find the magnitude of a MP number */

int initBitSniffer( MP_REG *mpReg )
	{
	int bits, precision = significance( mpReg );
	WORD bitMask;

	bitMask = 0x8000;
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

/* Convert RSA data from HPACK to RSAREF format */

static void byteReverse( BYTE *regPtr, int count );

static void hpackToRsaref( BYTE *buffer, MP_REG *value, int precision )
	{
	memset( buffer, 0, precision );
	memcpy( buffer, value, precision );
	byteReverse( buffer, precision );
	}

/* Wrapper for RSA public-key operation */

int RSAPublicBlock( BYTE *output, int *outputLen, BYTE *input, int inputLen,
					R_RSA_PUBLIC_KEY *publicKey );

void mp_modexp( MP_REG *expOut, MP_REG *expIn, MP_REG *exponent, MP_REG *modulus )
	{
	static R_RSA_PUBLIC_KEY rsaPublicKey;	/* Save on stack space */
	int outputLen;

	/* Set up RSAREF-style public key */
	rsaPublicKey.bits = units2bits( globalPrecision );
	hpackToRsaref( rsaPublicKey.modulus, modulus, MAX_RSA_MODULUS_LEN );
	hpackToRsaref( rsaPublicKey.exponent, exponent, MAX_RSA_MODULUS_LEN );

	/* Call the RSAREF public-key code */
	byteReverse( ( BYTE * ) expIn, units2bytes( globalPrecision ) );
	RSAPublicBlock( ( BYTE * ) expOut, &outputLen,
					( BYTE * ) expIn, units2bytes( globalPrecision ),
					&rsaPublicKey );
	byteReverse( ( BYTE * ) expOut, outputLen );
	}

/* Wrapper for RSA private-key operation */

int RSAPrivateBlock( BYTE *output, int *outputLen, BYTE *input, int inputLen,
					 R_RSA_PRIVATE_KEY *privateKey );

void rsaDecrypt( MP_REG *M, MP_REG *C, KEYINFO *privateKey )
	{
	static R_RSA_PRIVATE_KEY rsaPrivateKey;	/* Save on stack space */
	int outputLen;

#if 0
  unsigned int bits;                           /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];                    /* modulus */
unsigned char publicExponent[MAX_RSA_MODULUS_LEN];     /* public exponent */
unsigned char exponent[MAX_RSA_MODULUS_LEN];          /* private exponent */
  unsigned char prime[2][MAX_RSA_PRIME_LEN];               /* prime factors */
  unsigned char primeExponent[2][MAX_RSA_PRIME_LEN];   /* exponents for CRT */
  unsigned char coefficient[MAX_RSA_PRIME_LEN];          /* CRT coefficient */
#endif /* 0 */

	/* Set up RSAREF-style private key */
	rsaPrivateKey.bits = units2bits( globalPrecision );
	hpackToRsaref( rsaPrivateKey.modulus, privateKey->n, MAX_RSA_MODULUS_LEN );
	hpackToRsaref( rsaPrivateKey.prime[ 1 ], privateKey->p, MAX_RSA_PRIME_LEN );
	hpackToRsaref( rsaPrivateKey.prime[ 0 ], privateKey->q, MAX_RSA_PRIME_LEN );
	hpackToRsaref( rsaPrivateKey.primeExponent[ 1 ], privateKey->exponent1, MAX_RSA_PRIME_LEN );
	hpackToRsaref( rsaPrivateKey.primeExponent[ 0 ], privateKey->exponent2, MAX_RSA_PRIME_LEN );
	hpackToRsaref( rsaPrivateKey.coefficient, privateKey->u, MAX_RSA_PRIME_LEN );

	byteReverse( ( BYTE * ) C, units2bytes( globalPrecision ) );
	RSAPrivateBlock( ( BYTE * ) M, &outputLen,
					 ( BYTE * ) C, units2bytes( globalPrecision ),
					 &rsaPrivateKey );
	byteReverse( ( BYTE * ) M, outputLen );

/* rsaDecrypt( buffer2, ( MP_REG * ) buffer1,
			   signKeyInfo.d, signKeyInfo.p, signKeyInfo.q, signKeyInfo.u ); */
	}

#endif /* RSAREF */

/****************************************************************************
*																			*
*						Data Authentication Routines						*
*																			*
****************************************************************************/

/* Calculate the MD5 message digest for a section of a file */

static void md5file( const long startPos, long noBytes, MD5_CTX *mdContext )
	{
	int bytesToProcess;

	/* Calculate the message digest for the data */
	vlseek( startPos, SEEK_SET );
	MD5Init( mdContext );
	while( noBytes )
		{
		bytesToProcess = ( noBytes < _BUFSIZE ) ?
						 ( int ) noBytes : _BUFSIZE;
		vread( _inBuffer, bytesToProcess );
		noBytes -= bytesToProcess;

		/* Calculate the MD5 checksum for the buffer contents */
		MD5Update( mdContext, _inBuffer, bytesToProcess );
		}
	MD5Final( mdContext );
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

/* Convert plaintext message into an integer less than the modulus n by
   making it 1 byte shorter than the normalized modulus, as per PKCS #1,
   Section 8.  Since padding is easier for little-endian MPI's, we initially
   reverse the MPI to make it little-endian and then perform the padding */

#define BLOCKTYPE_PUBLIC	TRUE
#define BLOCKTYPE_PRIVATE	FALSE

static void preblock( BYTE *regPtr, int byteCount, MP_REG *modulus, BOOLEAN padOpPublic )
	{
	int bytePrecision, leadingZeroes, blockSize, padSize;

	/* Reverse the initial MPI */
	byteReverse( regPtr, byteCount );

	/* Calculate no.leading zeroes and blocksize (incl.data plus pad bytes) */
	bytePrecision = units2bytes( globalPrecision );
	leadingZeroes = bytePrecision - countbytes( modulus ) + 1;
	blockSize = bytePrecision - leadingZeroes;

	/* Calculate number of padding bytes (-2 is for leading 0, trailing 1) */
	padSize = blockSize - byteCount - 2;

	/* Perform padding as per PKCS #1 and RFC 1115 to generate the octet
	   string EB = 00 BT PS 00 D */
	regPtr[ byteCount++ ] = 0;
	while( padSize-- )
		regPtr[ byteCount++ ] = ( padOpPublic ) ? getStrongRandomByte() : 0xFF;
	regPtr[ byteCount++ ] = padOpPublic ? BT_PUBLIC : BT_PRIVATE;

	/* Add leading zeroes */
	while( leadingZeroes-- )
		regPtr[ byteCount++ ] = 0;

#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	/* Swap endianness of padded MPI */
	if( bigEndian )
		wordReverse( regPtr, byteCount );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
	}

/* Reverse the above blocking operation */

static void postunblock( BYTE *regPtr, int count )
	{
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	/* First reverse the individual words */
	if( bigEndian )
		wordReverse( regPtr, count );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

	/* Bytereverse the data */
	byteReverse( regPtr, count );
	}

/* Check a block of data's signature */

BOOLEAN checkSignature( LONG startPos, LONG noBytes )
	{
	BYTE decryptedSignature[ MAX_BYTE_PRECISION ];
	BYTE *mdInfo = decryptedSignature + sizeof( BYTE ) + sizeof( BYTE );
	LONG timeStamp;
	MP_REG signature[ MAX_UNIT_PRECISION ];	/* MP_REG's are now long-aligned */
	BYTE wantedID[ KEYFRAG_SIZE ];
	MD5_CTX mdContext;
	int i;
	BOOLEAN badSig = FALSE;
#ifndef GUI
	int time1, time2, time3, hours, minutes, seconds;
#endif /* !GUI */

	/* Set up checksumming for rest of packet */
	checksumSetInput( ( long ) getCheckPacketLength( ( BYTE ) fgetByte() ), NO_RESET_CHECKSUM );

	/* Make sure we know how to check this signature */
	if( fgetByte() != SIG_ALGORITHM_RSA )
		{
		/* Don't know how to handle this signature scheme, complain and
		   process file anyway */
#ifdef GUI
		alert( ALERT_CANT_FIND_PUBLIC_KEY, NULL );
#else
		hputs( WARN_CANT_FIND_PUBLIC_KEY );
#endif /* GUI */
		return( TRUE );
		}

	/* Copy rest of key fragment */
	for( i = 0; i < KEYFRAG_SIZE; i++ )
		wantedID[ i ] = fgetByte();

	/* Get signed message digest, first, setting precision to max.value to be
	   on the safe side */
	setPrecision( MAX_UNIT_PRECISION );
	i = readMPI( signature, NO_SET_PRECISION );
	if( i == ERROR || fgetWord() != crc16 )
		{
		/* Authentication information bad or corrupted, complain and process
		   file anyway */
#ifdef GUI
		alert( ALERT_SEC_INFO_CORRUPTED, NULL );
#else
		hputs( WARN_SEC_INFO_CORRUPTED );
#endif /* GUI */
		return( FALSE );
		}

	/* Perform a context switch from the general input buffer */
	saveInputState();

	/* Use the keyID to get the public key from a key file */
	if( !getKey( IS_PGP_KEY, IS_PUBLICKEY, IS_KEYID, PGP_PUBKEY_FILENAME,
				 wantedID, &signKeyInfo ) )
		{
#ifdef GUI
		alert( ALERT_CANT_FIND_PUBLIC_KEY, NULL );
#else
		hputs( WARN_CANT_FIND_PUBLIC_KEY );
#endif /* GUI */
		restoreInputState();	/* Switch back to the general input buffer */
		return( TRUE );
		}

	/* Recover message digest via public key */
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	if( bigEndian )
		wordReverse( ( BYTE * ) signature, globalPrecision * sizeof( MP_REG ) );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
	mp_modexp( ( MP_REG * ) decryptedSignature, signature, signKeyInfo.e, signKeyInfo.n );
	postunblock( decryptedSignature, MD_PACKET_LENGTH );

	/* Look at nested stuff within RSA block and make sure it's the correct
	   message digest algorithm */
	if( ctbType( decryptedSignature[ 0 ] ) != CTBTYPE_MD ||
				 decryptedSignature[ 1 ] != MD_INFO_LENGTH ||
				 decryptedSignature[ 2 ] != MD_ALGORITHM_MD5 )
		{
		badSig = TRUE;		/* Bad RSA decrypt */
		timeStamp = 0L;		/* Can't establish timestamp for data */
		}
	else
		{
		/* Extract date and compute message digest for rest of file */
		timeStamp = mgetLong( mdInfo + MD_TIME_OFS );
		setInputFD( archiveFD );
		md5file( startPos, noBytes, &mdContext );
		}

	/* Switch back to the general input buffer */
	restoreInputState();

	/* Check that everything matches */
	if( badSig || memcmp( mdContext.digest, mdInfo + MD_DIGEST_OFS, 16 ) )
		{
		badSig = TRUE;
#ifdef GUI
		showSig( FALSE, signKeyInfo.userID, timeStamp );
#else
		hprintf( MESG_BAD_SIGNATURE );
#endif /* GUI */
		}
	else
#ifdef GUI
		showSig( TRUE, signKeyInfo.userID, timeStamp );
#else
		hprintf( MESG_GOOD_SIGNATURE );
	extractDate( timeStamp, &time1, &time2, &time3, &hours, &minutes, &seconds );
	if( timeStamp )
		hprintf( MESG_SIGNATURE_FROM_s_DATE_dddddd,
				 signKeyInfo.userID, time1, time2, time3, hours, minutes, seconds );
	else
		hprintf( MESG_SIGNATURE_FROM_s_DATE_dddddd, signKeyInfo.userID, 0, 0, 0, 0, 0, 0 );
#endif /* GUI */

	return( !badSig );		/* Return status of signature */
	}

/* Create a signature for a block of data */

int createSignature( LONG startPos, LONG noBytes, char *wantedID )
	{
	MP_REG buffer2[ MAX_UNIT_PRECISION ];
	BYTE buffer1[ MAX_BYTE_PRECISION ];
	BYTE *bufPtr = buffer1 + sizeof( BYTE ) + sizeof( BYTE );
	int bitCount, byteCount, i;
	LONG timeStamp;
	MD5_CTX MD;

	/* Get the secret key corresponding to the userID, setting precision to
	   the maximum value to be on the safe side */
	setPrecision( MAX_UNIT_PRECISION );
	if( !getKey( IS_PGP_KEY, IS_SECRETKEY, IS_USERID, PGP_SECKEY_FILENAME,
				 ( BYTE * ) wantedID, &signKeyInfo ) )
		error( CANNOT_FIND_SECRET_KEY_FOR_s, wantedID );

	/* Build the packet header */
	buffer1[ 0 ] = CTB_MD;
	buffer1[ 1 ] = MD_INFO_LENGTH;
	buffer1[ 2 ] = MD_ALGORITHM_MD5;

	/* Generate the message digest for the data and add it to the packet */
	setInputFD( archiveFD );
	md5file( startPos, noBytes, &MD );
	memcpy( bufPtr + MD_DIGEST_OFS, MD.digest, 16 );

	/* Add the timestamp */
	timeStamp = time( NULL );
	mputLong( bufPtr + MD_TIME_OFS, timeStamp );

	/* Pre-block mdPacket */
	preblock( buffer1, MD_INFO_LENGTH + 2, signKeyInfo.n, BLOCKTYPE_PRIVATE );

	/* Perform the RSA signature calculation */
#ifndef GUI
	hprintf( MESG_WAIT );
	hflush( stdout );
#endif /* !GUI */
	rsaDecrypt( buffer2, ( MP_REG * ) buffer1, &signKeyInfo );
#ifndef GUI
	blankChars( strlen( MESG_WAIT ) );
#endif /* !GUI */
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	if( bigEndian )
		wordReverse( ( BYTE * ) buffer2, globalPrecision * sizeof( MP_REG ) );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

	/* Calculate the size of the MPI needed to store the signature */
	bitCount = countbits( buffer2 );
	byteCount = bits2bytes( bitCount );

	/* Create the full signature packet */
	checksumBegin( RESET_CHECKSUM );
	fputByte( CTB_SKE );			/* Write header */
	fputByte( ( BYTE ) ( sizeof( BYTE ) + KEYFRAG_SIZE + ( 2 + byteCount ) ) );
	fputByte( SIG_ALGORITHM_RSA );
	for( i = 0; i < KEYFRAG_SIZE; i++ )	/* Write key ID */
		fputByte( signKeyInfo.keyID[ i ] );
	fputWord( ( WORD ) bitCount );	/* Write signature as MPI */
	bufPtr = ( BYTE * ) buffer2 + byteCount - 1;
	for( i = 0; i < byteCount; i++ )
		fputByte( *bufPtr-- );
	checksumEnd();
	fputWord( crc16 );				/* Write packet checksum */

	return( ( int ) ( sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE ) +
					  KEYFRAG_SIZE + ( 2 + byteCount ) + sizeof( WORD ) ) );
	}

/****************************************************************************
*																			*
*						Public-Key Encryption Routines						*
*																			*
****************************************************************************/

static int pkeEncrypt( BYTE *pkePacket, const char *wantedID, BYTE *keyInfo )
	{
	MP_REG buffer2[ MAX_UNIT_PRECISION ];
	BYTE buffer1[ MAX_BYTE_PRECISION ], *buf2ptr;
	int count, byteCount;

	/* Use the userID to get the public key from a key file, setting the
	   precision to the maximum value to be on the safe side */
	setPrecision( MAX_UNIT_PRECISION );
	if( !getKey( IS_PGP_KEY, IS_PUBLICKEY, IS_USERID, PGP_PUBKEY_FILENAME,
					( BYTE * ) wantedID, &cryptKeyInfo ) )
		error( CANNOT_FIND_PUBLIC_KEY_FOR_s, wantedID );

	/* Copy the algorithm ID and keyID to the PKE packet */
	*pkePacket++ = PKE_ALGORITHM_RSA;
	memcpy( pkePacket, cryptKeyInfo.keyID, KEYFRAG_SIZE );
	pkePacket += KEYFRAG_SIZE;

	/* Assemble a CKE info packet and pre-block it */
	buffer1[ 0 ] = CTB_CKEINFO;
	buffer1[ 1 ] = sizeof( BYTE ) + BLOCKSIZE;
	buffer1[ 2 ] = CKE_ALGORITHM_MDC;
	memcpy( buffer1 + 3, keyInfo, BLOCKSIZE );
	preblock( buffer1, sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE ) + BLOCKSIZE,
			  cryptKeyInfo.n, BLOCKTYPE_PUBLIC );

	/* Now encrypt the crap out of it */
	mp_modexp( buffer2, ( MP_REG * ) buffer1, cryptKeyInfo.e, cryptKeyInfo.n );
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	if( bigEndian )
		wordReverse( ( BYTE * ) buffer2, globalPrecision * sizeof( MP_REG ) );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

	/* Calculate the size of the MPI needed to store the data */
	count = countbits( buffer2 );
	byteCount = bits2bytes( count );

	/* Write MPI to PKE packet with endianness reversal */
	mputWord( pkePacket, ( WORD ) count );
	pkePacket += sizeof( WORD );
	buf2ptr = ( BYTE * ) buffer2 + ( byteCount - 1 );
	count = byteCount;
	while( count-- )
		*pkePacket++ = *buf2ptr--;

	return( ( int ) ( sizeof( BYTE ) + KEYFRAG_SIZE + sizeof( WORD ) +
					  byteCount ) );
	}

static BOOLEAN pkeDecrypt( MP_REG *pkePacket, BYTE *wantedID, BYTE *decryptedMPI )
	{
	BOOLEAN keyOK;

	/* Perform a context switch from the general input buffer */
	saveInputState();

	/* Get the secret key (the precision has already been set when the
	   pkePacket was read) */
	keyOK = getKey( IS_PGP_KEY, IS_SECRETKEY, IS_KEYID, PGP_SECKEY_FILENAME,
					wantedID, &cryptKeyInfo );

	/* Switch back to the general input buffer */
	restoreInputState();

	/* Leave now if we couldn't find a matching key (there may be one in a
	   following PKE packet) */
	if( !keyOK )
		return( FALSE );

	/* Recover CKE packet */
#ifndef GUI
	hprintf( MESG_WAIT );
	hflush( stdout );
#endif /* !GUI */
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	if( bigEndian )
		wordReverse( ( BYTE * ) pkePacket, globalPrecision * sizeof( MP_REG ) );
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
	rsaDecrypt( ( MP_REG * ) decryptedMPI, pkePacket, &cryptKeyInfo );
	postunblock( decryptedMPI, PKE_PACKET_LENGTH );
#ifndef GUI
	blankChars( strlen( MESG_WAIT ) );
#endif /* !GUI */
	return( TRUE );
	}

/****************************************************************************
*																			*
*				Encryption/Security Packet Management Routines				*
*																			*
****************************************************************************/

/* Parse an encryption information packet */

BOOLEAN getEncryptionInfo( int *length )
	{
	BYTE decryptedMPI[ MAX_BYTE_PRECISION ], keyID[ KEYFRAG_SIZE ], ctb;
	MP_REG pkePacket[ MAX_UNIT_PRECISION ];
	BOOLEAN packetOK = TRUE, morePackets = TRUE, foundKey = FALSE;
	int dataLength, totalLength = 0, i;
	BYTE iv[ IV_SIZE ];

	while( packetOK && morePackets )
		{
		/* Pull apart the security info to see what we've got */
		if( !isCTB( ctb = fgetByte() ) )
			{
			packetOK = FALSE;
			break;
			}
		if( !hasMore( ctb ) )
			morePackets = FALSE;
		dataLength = getCheckPacketLength( ctb );
		checksumSetInput( ( long ) dataLength, NO_RESET_CHECKSUM );
		totalLength += sizeof( BYTE ) + sizeof( BYTE );

		/* Process the encryption information packet unless we've already
		   found the key we were looking for */
		if( !foundKey )
			switch( ctbType( ctb ) )
				{
				case CTBTYPE_PKE:
					/* Public-key encryption packet.  First, skip the length
					   field and make sure it's a known PKE algorithm */
					totalLength += sizeof( BYTE );
					dataLength -= sizeof( BYTE );
					if( fgetByte() != PKE_ALGORITHM_RSA )
						{
						packetOK = FALSE;
						break;
						}

					/* If we've guessed at the archive being block encrypted with
					   a CKE, change this to a PKE */
					if( cryptFlags & CRYPT_CKE_ALL )
						cryptFlags ^= CRYPT_CKE_ALL | CRYPT_PKE_ALL;

					/* Read in key ID fragment and ckePacket, setting the
					   precision to the max.precision to be on the safe side */
					for( i = 0; i < KEYFRAG_SIZE; i++ )
						keyID[ i ] = fgetByte();
					setPrecision( MAX_UNIT_PRECISION );
					i += readMPI( pkePacket, SET_PRECISION );
					totalLength += i;	/* Adjust for KEYFRAG and MPI size */
					dataLength -= i;

					/* Try and decrypt the MPI containing the CKE info packet */
					if( !pkeDecrypt( pkePacket, keyID, decryptedMPI ) )
						break;
					foundKey = TRUE;	/* We've found a usable key */

					/* Now process the conventional-key information packet.
					   First make sure it's the right packet, skip its length
					   info, and make sure it's a known CKE algorithm */
					if( decryptedMPI[ 0 ] != CTB_CKEINFO ||
						decryptedMPI[ 2 ] != CKE_ALGORITHM_MDC )
						{
						packetOK = FALSE;
						break;
						}

					/* Finally, we have the CKE keying information and can
					   perform the necessary setup operations with it */
					initKey( decryptedMPI + 3, BLOCKSIZE, DEFAULT_IV,
							 DEFAULT_MDC_ITERATIONS );
					break;

				case CTBTYPE_CKE:
					/* Conventional-key encryption packet.  Make sure it's a
					   known algorithm, and rekey the encryption system if
					   necessary */
					foundKey = TRUE;	/* We've found a usable key */
					totalLength += sizeof( BYTE ) + IV_SIZE;
					dataLength -= sizeof( BYTE ) + IV_SIZE;
					if( ( ctb = fgetByte() ) == CKE_ALGORITHM_MDC_R )
						{
						/* If we don't have a secondary password yet, get it now */
						if( !secKeyLength )
							initPassword( SECONDARY_KEY );

						for( i = 0; i < IV_SIZE; i++ )
							iv[ i ] = fgetByte();
						initKey( ( BYTE * ) secKey, secKeyLength, iv,
								 DEFAULT_MDC_ITERATIONS );
						}
					else
						if( ctb == CKE_ALGORITHM_MDC )
							{
							/* If we don't have a password yet, get it now */
							if( !keyLength )
								initPassword( MAIN_KEY );

							for( i = 0; i < IV_SIZE; i++ )
								iv[ i ] = fgetByte();
							initKey( ( BYTE * ) key, keyLength, iv,
									 DEFAULT_MDC_ITERATIONS );
							}
						else
							packetOK = FALSE;
					break;

				default:
					/* Unknown encryption packet */
					packetOK = FALSE;
				}

		/* Read any unread bytes still in the packet, or skip the packet
		   entirely if we've already found the key we were looking for */
		if( dataLength )
			{
			totalLength += dataLength;
			while( dataLength-- )
				fgetByte();
			}

		/* Make sure the packet wasn't corrupted */
		totalLength += sizeof( WORD );
		if( fgetWord() != crc16 )
			{
			packetOK = FALSE;
			totalLength = 0;	/* Length may be corrupted - need to perform error recovery */
			}
		}

	/* If we haven't been able to find a matching key for a PKE packet, complain */
	if( !foundKey && packetOK )
		error( CANNOT_FIND_SECRET_KEY );

	*length = totalLength;
	return( packetOK );
	}

/* Write an encryption information packet to a file */

int putEncryptionInfo( const BOOLEAN isMainKey )
	{
	int cryptInfoLength, packetLength, i;
	BYTE pkePacket[ sizeof( BYTE ) + KEYFRAG_SIZE + sizeof( WORD ) + MAX_BYTE_PRECISION ];
	BYTE iv[ IV_SIZE ], *randomKey;
	BOOLEAN moreKeys, keyPresent;
	char *userID;

	if( cryptFlags & ( CRYPT_CKE | CRYPT_CKE_ALL ) )
		{
		checksumBegin( RESET_CHECKSUM );

		/* Write a conventional-key encryption information packet:
		   CTB, length, algorithm ID, IV */
		memcpy( iv, getIV(), IV_SIZE );
		fputByte( CTB_CKE );
		fputByte( sizeof( BYTE ) + IV_SIZE );
		fputByte( ( BYTE ) ( ( isMainKey ) ? CKE_ALGORITHM_MDC :
											 CKE_ALGORITHM_MDC_R ) );
		for( i = 0; i < IV_SIZE; i++ )
			fputByte( iv[ i ] );
		checksumEnd();
		fputWord( crc16 );
		cryptInfoLength = sizeof( BYTE ) + sizeof( BYTE ) +
						  sizeof( BYTE ) + IV_SIZE + sizeof( WORD );
		initKey( ( isMainKey ) ? ( BYTE * ) key : ( BYTE * ) secKey,
								 keyLength, iv, DEFAULT_MDC_ITERATIONS );
		}
	else
		{
		cryptInfoLength = 0;
		moreKeys = getFirstUserID( &userID, isMainKey );
		keyPresent = TRUE;
		randomKey = getStrongRandomKey();
		do
			{
			checksumBegin( RESET_CHECKSUM );

			/* Write a public-key encryption information packet: CTB, length,
			   algorithm ID, keyID, encrypted CKE info packet */
			if( moreKeys )
				fputByte( CTB_PKE | CTB_MORE );
			else
				{
				fputByte( CTB_PKE );
				keyPresent = FALSE;
				}
			packetLength = pkeEncrypt( pkePacket, userID, randomKey );
			fputByte( ( BYTE ) packetLength );
			for( i = 0; i < packetLength; i++ )
				fputByte( pkePacket[ i ] );
			checksumEnd();
			fputWord( crc16 );
			cryptInfoLength += sizeof( BYTE ) + sizeof( BYTE ) +
							   packetLength + sizeof( WORD );

			/* Look for another keyID */
			moreKeys = getNextUserID( &userID );
			}
		while( keyPresent );
		}

	return( cryptInfoLength );
	}
