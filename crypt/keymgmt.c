/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  Key Management Routines						*
*							KEYMGMT.C  Updated 11/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "error.h"
#include "frontend.h"
#include "hpacklib.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "crc\crc16.h"
  #include "crypt\crypt.h"
  #include "crypt\md5.h"
  #include "crypt\mdc.h"
  #include "crypt\keymgmt.h"
  #include "io\fastio.h"
  #include "io\hpackio.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "crc16.h"
  #include "keymgmt.h"
  #include "crypt.h"
  #include "md5.h"
  #include "mdc.h"
  #include "fastio.h"
  #include "hpackio.h"
  #include "language.h"
#else
  #include "crc/crc16.h"
  #include "crypt/crypt.h"
  #include "crypt/md5.h"
  #include "crypt/mdc.h"
  #include "crypt/keymgmt.h"
  #include "io/fastio.h"
  #include "io/hpackio.h"
  #include "language/language.h"
#endif /* System-specific include directives */

/* The environment variable where various key files hide */

const char PEMPATH[] = "PEMPATH";	/* Env.var for PEM path */
const char PGPPATH[] = "PGPPATH";	/* Env.var for PGP path */

/* These files can use either the PGP or PEM path as the default path */

#if defined( __ARC__ )
  const char RANDSEED_FILENAME[] = "hpakseed";
#elif defined( __MAC__ )
  const char RANDSEED_FILENAME[] = "HPACK Random Seedfile";
#else
  const char RANDSEED_FILENAME[] = "hpakseed.bin";
#endif /* Various system-specific seedfile names */

/* The number of password attempts the user is allowed to decrypt their
   secret key */

#define PASSWORD_RETRIES	3

/* Prototypes for functions in CRYPT.C */

char *getPassword( char *passWord, const char *prompt );

/* The following are declared in CRYPT.C */

extern BYTE *cryptBuffer;

/* The random key and IV, seedFile FD, MD5 context, and position in seed
   buffer used for public-key encryption */

BYTE mdcKey[ MDC_BLOCKSIZE ];
BYTE seedIV[ MDC_BLOCKSIZE ];	/* Needed for non-interference with main IV */
FD seedFileFD = ERROR;			/* No seedfile to begin with */
MD5_CTX mdSeedContext;
int seedPos = MDC_BLOCKSIZE;	/* Always reseed generator */

/****************************************************************************
*																			*
* 							General-Purpose Routines 						*
*																			*
****************************************************************************/

/* Clear out the ID fields of a public key */

void clearIDfields( KEYINFO *keyInfo )
	{
	memset( keyInfo->userID, 0, sizeof( keyInfo->userID ) );
	memset( keyInfo->keyID, 0, sizeof( keyInfo->keyID ) );
	memset( keyInfo->ascKeyID, 0, sizeof( keyInfo->ascKeyID ) );
	}

/* Shut down keymgmt routines */

void endKeyMgmt( void )
	{
	/* Write the random MDC seed to the seedFile if necessary */
	if( seedFileFD != ERROR )
		{
		/* Step the MD5 RNG to hide the previous key, then write it to the
		   seed file */
		MD5Update( &mdSeedContext, mdcKey, MDC_BLOCKSIZE );
		MD5Final( &mdSeedContext );
		hlseek( seedFileFD, 0L, SEEK_SET );
		hwrite( seedFileFD, mdSeedContext.digest, BLOCKSIZE );
		hclose( seedFileFD );
		}

	/* Scrub the encryption key so other users can't find it by examining
	   core after HPACK has run */
	memset( mdcKey, 0, MDC_BLOCKSIZE );
	}

/****************************************************************************
*																			*
*						Key Packet Handling Routines						*
*																			*
****************************************************************************/

#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
/* Endianness-reverse the words in an MPI (only called on a big-endian CPU) */

void wordReverse( BYTE *regPtr, int count )
	{
	BYTE temp;
	int i;

	for( i = 0; i < count; i += sizeof( WORD ) )
		{
		temp = regPtr[ i ];
		regPtr[ i ] = regPtr[ i + 1 ];
		regPtr[ i + 1 ] = temp;
		}
	}
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

/* Get the length of a packet.  If it's an HPACK packet we have to make sure
   we calculate the checksum on the header bytes, if it's a PGP packet we
   need to look out for longer packet types */

static int getPacketLength( const BYTE ctb )
	{
	/* If reading a PGP packet we don't calculate a checksum, and need to
	   take longer packet types into account */
	if( ctbLength( ctb ) == CTB_LEN_BYTE )
		return( ( int ) fgetByte() );
	else
		if( ctbLength( ctb ) == CTB_LEN_WORD )
			return( ( int ) fgetWord() );
		else
			return( ( int ) fgetLong() );
	}

int getCheckPacketLength( const BYTE ctb )
	{
	BYTE buffer[ sizeof( BYTE ) + sizeof( BYTE ) ];

	/* If reading an HPACK packet we need to calculate a checksum on the
	   packet header as we read it */
	crc16 = 0;
	buffer[ 0 ] = ctb;
	buffer[ 1 ] = fgetByte();
	crc16buffer( buffer, sizeof( BYTE ) + sizeof( BYTE ) );
	return( ( int ) buffer[ 1 ] );
	}

/* Read a MPI in the form of a byte array preceded by a 16-bit bitcount from
   a file or memory into a multiprecision register */

int readMPI( MP_REG *mpReg, BOOLEAN doSetPrecision )
	{
	int count, byteCount;
	BYTE *regPtr = ( BYTE * ) mpReg;
	WORD bitCount;

	/* Read in the MPI itself from the file.  First, read in the bit count
	   and set the global precision accordingly */
	mp_init( mpReg, 0 );
	bitCount = fgetWord();
	count = byteCount = bits2bytes( bitCount );
	if( ( count < 1 ) || ( count > MAX_BYTE_PRECISION ) )
		/* Impossibly small or large MPI value */
		return( ERROR );
	if( doSetPrecision )
		/* Set the precision to that specified by the number read */
		setPrecision( bits2units( bitCount + SLOP_BITS ) );

#ifdef RW_BIG_ENDIAN
	/* Read in the value */
	while( count-- )
		*regPtr++ = fgetByte();
#else
	/* Read in the value with endianness reversal */
	regPtr += count - 1;
	while( count-- )
		*regPtr-- = fgetByte();
#endif /* RW_BIG_ENDIAN */

	/* Return number of bytes processed */
	return( ( int ) ( byteCount + sizeof( WORD ) ) );
	}

static int getMPI( BYTE *buffer, MP_REG *mpReg )
	{
	int count, byteCount;
	BYTE *regPtr = ( BYTE * ) mpReg;

	/* Get the MPI from an in-memory buffer.  We don't need to bother with
	   setting the precision since this has already been done */
	mp_init( mpReg, 0 );
	count = byteCount = bits2bytes( mgetWord( buffer ) );
	if( ( count < 1 ) || ( count > MAX_BYTE_PRECISION ) )
		/* Impossibly small or large MPI value */
		return( ERROR );
	buffer += sizeof( WORD );

	/* Read in the value with endianness reversal */
	regPtr += count - 1;
	while( count-- )
		*regPtr-- = *buffer++;

	/* Return number of bytes processed */
	return( ( int ) ( byteCount + sizeof( WORD ) ) );
	}

/* Decrypt the encrypted fields of a previously-read key packet */

BYTE keyBuffer[ ( 4 * MAX_BYTE_PRECISION ) + sizeof( WORD ) ];
BYTE keyIV[ IV_SIZE ];
int keyBufSize;

static BOOLEAN decryptKeyPacket( KEYINFO *keyInfo )
	{
	BYTE decryptBuffer[ ( 4 * MAX_BYTE_PRECISION ) + sizeof( WORD ) ];
	char keyPassword[ MAX_KEYLENGTH + 1 ];
	WORD savedCRC16 = crc16;
	int count, i, j, k;

	/* Try and decrypt the key information */
	for( count = 0; count < PASSWORD_RETRIES; count++ )
		{
		getPassword( keyPassword, MESG_ENTER_SECKEY_PASSWORD );
		initKey( ( BYTE * ) keyPassword, strlen( keyPassword ), keyIV,
				 DEFAULT_MDC_ITERATIONS );

		/* Read in key and try to decrypt it */
		memcpy( decryptBuffer, keyBuffer, keyBufSize );
		decryptCFB( decryptBuffer, keyBufSize );
		crc16 = 0;
		crc16buffer( decryptBuffer, keyBufSize - sizeof( WORD ) );
		if( crc16 != mgetWord( decryptBuffer + keyBufSize - sizeof( WORD ) ) )
#ifdef GUI
			alert( ALERT_PASSWORD_INCORRECT, NULL );
#else
			hputs( MESG_PASSWORD_INCORRECT );
#endif /* GUI */
		else
			/* Key was successfully decrypted, exit */
			break;
		}

	/* Restore callers checksum information */
	crc16 = savedCRC16;

	/* If we couldn't get the password, invaldiate key and give up */
	if( count == PASSWORD_RETRIES )
		return( FALSE );

	/* Get information from buffer.  First read the non-PEM fields and make
	   sure they're OK */
	count = getMPI( decryptBuffer, keyInfo->d );
	i = getMPI( decryptBuffer + count, keyInfo->p );
	j = getMPI( decryptBuffer + count + i, keyInfo->q );
	k = getMPI( decryptBuffer + count + i + j, keyInfo->u );
	if( ( count | i | j | k ) == ERROR )
		error( BAD_KEYFILE );

	/* Now read the remaining PEM fields if necessary */
	count += i + j + k;
	if( keyBufSize - count > sizeof( WORD ) )
		{
		i = getMPI( decryptBuffer + count, keyInfo->exponent1 );
		j = getMPI( decryptBuffer + count + j, keyInfo->exponent2 );
		if( ( i | j ) == ERROR )
			error( BAD_KEYFILE );

		keyInfo->isPemKey = TRUE;
		}

#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
	/* Perform an endianness-reversal if necessary */
	if( bigEndian )
		{
		wordReverse( ( BYTE * ) keyInfo->d, globalPrecision * sizeof( MP_REG ) );
		wordReverse( ( BYTE * ) keyInfo->p, globalPrecision * sizeof( MP_REG ) );
		wordReverse( ( BYTE * ) keyInfo->q, globalPrecision * sizeof( MP_REG ) );
		wordReverse( ( BYTE * ) keyInfo->u, globalPrecision * sizeof( MP_REG ) );
		wordReverse( ( BYTE * ) keyInfo->exponent1, globalPrecision * sizeof( MP_REG ) );
		wordReverse( ( BYTE * ) keyInfo->exponent2, globalPrecision * sizeof( MP_REG ) );
		}
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */

	keyInfo->isEncrypted = FALSE;	/* Key is now decrypted */
	return( TRUE );
	}

/* Read in a key packet from a file.  It will return the ctb, timestamp,
   userid, keyID, public key components n and e, and private key components
   d, p, q and u if necessary.  The strategy we use is to keep parsing data
   until we find a userID, then return.   Since userID's always follow keys,
   and since there can be multiple keys attached to each userID, each call
   yields a new userID (but not necessarily a new key) */

static BOOLEAN readKeyPacket( BOOLEAN isSecretKey, KEYINFO *keyInfo )
	{
	BOOLEAN needMoreInfo = TRUE, gotKeyPacket = TRUE;
	WORD certLength;
	int i, j, k, l;
	int ctb;
	BYTE ch;

	/* Keep reading packets until we have a key packet and userID packet we
	   can use */
	while( needMoreInfo )
		{
		/*  Get key certificate CTB and length info */
		if( ( ctb = fgetByte() ) == FEOF )
			break;
		certLength = getPacketLength( ( BYTE ) ctb );

		/* If it's a userID packet, read the ID */
		switch( ctb )
			{
			case CTB_USERID:
				/* Get userID as Pascal string (I am NAN!) */
				for( i = 0; i < certLength; i++ )
					keyInfo->userID[ i ] = fgetByte();
				keyInfo->userID[ i ] = '\0';
				certLength = 0;

				/* Work out whether we can exit now or not */
				if( gotKeyPacket )
					needMoreInfo = FALSE;
				break;

			case CTB_CERT_PUBKEY:
			case CTB_CERT_SECKEY:
				/* Get version byte, return if it was created by a version
				   we don't know how to handle */
				gotKeyPacket = FALSE;		/* We haven't decoded the packet yet */
				certLength -= sizeof( BYTE );
				if( fgetByte() != PGP_VERSION )
					break;

				/* Get timestamp, validity period, and PKE algorithm ID */
				certLength -= sizeof( LONG ) + sizeof( WORD ) + sizeof( BYTE );
				keyInfo->timeStamp = fgetLong();
				fgetWord();
				if( fgetByte() != PKE_ALGORITHM_RSA )
					break;

				/* We're past certificate headers, get initial key material */
				i = readMPI( keyInfo->n, SET_PRECISION );
				j = readMPI( keyInfo->e, NO_SET_PRECISION );
				certLength -= i + j;

				/* Copy the keyID from the low bits of n, convert it to
				   ASCII hex, and perform an endianness reversal if necessary */
				memcpy( keyInfo->keyID, keyInfo->n, KEYFRAG_SIZE );
				for( k = KEYDISPLAY_SIZE - 1, l = 0; k >= 0; k-- )
					{
					ch = ( keyInfo->keyID[ k ] >> 4 ) + '0';
					keyInfo->ascKeyID[ l++ ] = ( ch <= '9' ) ? ch : ch + 'A' - '9' - 1;
					ch = ( keyInfo->keyID[ k ] & 0x0F ) + '0';
					keyInfo->ascKeyID[ l++ ] = ( ch <= '9' ) ? ch : ch + 'A' - '9' - 1;
					}
				keyInfo->ascKeyID[ l ] = '\0';
#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
				if( bigEndian )
					{
					wordReverse( ( BYTE * ) keyInfo->n, globalPrecision * sizeof( MP_REG ) );
					wordReverse( ( BYTE * ) keyInfo->e, globalPrecision * sizeof( MP_REG ) );
					}
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
				/* Check for possible problems in reading MPI's */
				if( ( i | j ) == ERROR )
					error( BAD_KEYFILE );

				/* Read the secret key fields if necessary */
				keyInfo->isEncrypted = FALSE;
				if( isSecretKey )
					{
					/* Check if we can process the following MPI fields */
					certLength -= sizeof( BYTE );
					if( ( ctb = fgetByte() ) != CKE_ALGORITHM_NONE )
						{
						/* If we can't decrypt the remainder of the
						   information, skip it */
						if( ctb != CKE_ALGORITHM_MDC )
							break;

						/* Get decryption information for the key, and the
						   encrypted key itself */
						for( i = 0; i < IV_SIZE; i++ )
							keyIV[ i ] = fgetByte();
						certLength -= IV_SIZE;
						for( i = 0; i < certLength; i++ )
							keyBuffer[ i ] = fgetByte();
						keyBufSize = certLength;

						certLength = 0;
						keyInfo->isEncrypted = TRUE;
						}

					/* Read key fields if they're non-encrypted */
					if( !keyInfo->isEncrypted )
						{
						/* Get information from file.  First read the non-PEM
						   fields and make sure they're OK */
						i = readMPI( keyInfo->d, NO_SET_PRECISION );
						j = readMPI( keyInfo->p, NO_SET_PRECISION );
						k = readMPI( keyInfo->q, NO_SET_PRECISION );
						l = readMPI( keyInfo->u, NO_SET_PRECISION );
						if( ( i | j | k | l ) == ERROR )
							error( BAD_KEYFILE );
						certLength -= i + j + k + l;

						/* Now read the remaining PEM fields if necessary */
						if( certLength > sizeof( WORD ) )
							{
							i = readMPI( keyInfo->exponent1, NO_SET_PRECISION );
							j = readMPI( keyInfo->exponent2, NO_SET_PRECISION );
							if( ( i | j ) == ERROR )
								error( BAD_KEYFILE );
							certLength -= i + j;

							keyInfo->isPemKey = TRUE;
							}

						/* Finally, skip the checksum at the end */
						fgetWord();
						certLength -= sizeof( WORD );

#if !( defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ ) )
						/* Perform an endianness-reversal if necessary */
						if( bigEndian )
							{
							wordReverse( ( BYTE * ) keyInfo->d, globalPrecision * sizeof( MP_REG ) );
							wordReverse( ( BYTE * ) keyInfo->p, globalPrecision * sizeof( MP_REG ) );
							wordReverse( ( BYTE * ) keyInfo->q, globalPrecision * sizeof( MP_REG ) );
							wordReverse( ( BYTE * ) keyInfo->u, globalPrecision * sizeof( MP_REG ) );
							wordReverse( ( BYTE * ) keyInfo->exponent1, globalPrecision * sizeof( MP_REG ) );
							wordReverse( ( BYTE * ) keyInfo->exponent2, globalPrecision * sizeof( MP_REG ) );
							}
#endif /* !( __MSDOS16__ || __MSDOS32__ || __OS2__ ) */
						}
					}

				/* Now we've got a valid key packet */
				gotKeyPacket = TRUE;
			}

		/* Skip rest of this key certificate */
		if( certLength )
			skipSeek( ( LONG ) certLength );
		}

	/* Return TRUE if we've got a packet, FALSE otherwise */
	return( ctb != FEOF );
	}

/* Get the public or private key corresponding to a given keyID/userID from
   the public/private key file */

BOOLEAN getKey( const BOOLEAN isPemKey, const BOOLEAN isSecretKey,
				const BOOLEAN isKeyID, char *keyFileName, BYTE *wantedID,
				KEYINFO *keyInfo )
	{
	FD keyFileFD, savedInFD = getInputFD();
	BOOLEAN matched, firstTime = TRUE;
	char *matchID, firstChar;
	char *keyFilePath;
	int userIDlength, i;

	/* Use the keyID to get the public key from a key file */
	keyFilePath = getFirstKeyPath( getenv( PGPPATH ), keyFileName );
	while( keyFilePath != NULL )
		{
		/* Open key file */
		if( ( keyFileFD = hopen( keyFilePath, O_RDONLY | S_DENYWR ) ) == IO_ERROR )
			return( FALSE );
		setInputFD( keyFileFD );
		resetFastIn();

		/* Set up userID info if necessary */
		if( !isKeyID )
			{
			/* Check for a hexadecimal userID, signifying the user wants to
			   match by hex ASCII keyID and not by userID */
			if( !strnicmp( ( char * ) wantedID, "0x", 2 ) )
				{
				wantedID += 2;
				matchID = keyInfo->ascKeyID;
				}
			else
				matchID = keyInfo->userID;

			userIDlength = strlen( ( char * ) wantedID );
			firstChar = toupper( *wantedID );
			}

		/* Grovel through the file looking for the key */
		while( TRUE )
			{
			/* Set up key type information */
			keyInfo->isPemKey = isPemKey;

			/* Try and read a key packet, unless its the first time through
			   the loop in which case the information may already be in memory */
			if( !firstTime && !readKeyPacket( isSecretKey, keyInfo ) )
				break;
			firstTime = FALSE;

			matched = FALSE;
			if( isKeyID )
				{
				/* wantedID contains key fragment, check it against n from
				   keyfile */
				if( !memcmp( wantedID, keyInfo->keyID, KEYFRAG_SIZE ) )
					matched = TRUE;
				}
			else
				/* wantedID contains partial userID, check it against userID
				   from keyfile */
				for( i = 0; matchID[ i ]; i++ )
					if( ( toupper( matchID[ i ] ) == firstChar ) &&
						!strnicmp( ( char * ) wantedID, matchID + i, userIDlength ) )
						matched = TRUE;

			/* Found key matching ID */
			if( matched )
				{
				/* If it's a secret key, try and decrypt it */
				if( keyInfo->isEncrypted && !decryptKeyPacket( keyInfo ) )
					{
					/* Couldn't decrypt, invalidate key ID fields so we don't
					   match the key the next time around, and continue */
					clearIDfields( keyInfo );
					continue;
					}

				/* Got valid key, clean up and exit */
				hclose( keyFileFD );
				setInputFD( savedInFD );
				return( TRUE );
				}
			}

		/* No luck, try the next search path */
		hclose( keyFileFD );
		keyFilePath = getNextKeyPath();
		}

	setInputFD( savedInFD );
	return( FALSE );
	}

/****************************************************************************
*																			*
*					Key File/User ID Management Routines					*
*																			*
****************************************************************************/

/* Get the first/next search path in a list of paths */

static char *keyPathListPos, keyPath[ MAX_PATH ];
											/* Current position in search
											   path */
static char keyFileName[ MAX_FILENAME ];	/* Keyfile name */

char *getNextKeyPath( void )
	{
	char *keyPathListPtr = keyPathListPos, ch;
	int length;

	/* Check to see if we've run out of entries */
	if( *keyPathListPtr == '\0' )
		return( NULL );

	/* Extract the next search path from the list */
	while( *keyPathListPtr && *keyPathListPtr != PATH_SEPERATOR )
		keyPathListPtr++;
	length = ( int ) ( keyPathListPtr - keyPathListPos );
	strncpy( keyPath, keyPathListPos, length );
#if defined( __ATARI__ ) || defined( __MSDOS__ ) || defined( __OS2__ )
	if( ( ch = keyPath[ length - 1 ] ) != SLASH && ch != '\\' )
#else
	if( ( ch = keyPath[ length - 1 ] ) != SLASH )
#endif /* __ATARI__ || __MSDOS__ || __OS2__ */
		keyPath[ length++ ] = SLASH;	/* Add trailing slash if necessary */
	strcpy( keyPath + length, keyFileName );
	if( *keyPathListPtr == PATH_SEPERATOR )
		keyPathListPtr++;	/* Skip semicolon separator */
	keyPathListPos = keyPathListPtr;

	/* Range check (this should really be done earlier but it complicates
	   things since at that point the path hasn't been extracted yet */
	if( length + strlen( keyFileName ) > MAX_PATH )
		error( PATH_ss_TOO_LONG, keyPath, keyFileName );

	return( keyPath );
	}

char *getFirstKeyPath( const char *keyRingPath, const char *keyringName )
	{
	/* Set up search path and filename */
	keyPathListPos = ( char * ) keyRingPath;
	strcpy( keyFileName, keyringName );

	/* Special-case check for nonexistant keyRingPath */
	if( keyRingPath == NULL )
		{
		/* Point to empty path so we exit on next call, return filename only */
		keyPathListPos = "";
		return( ( char * ) keyringName );
		}

	return( getNextKeyPath() );
	}

/* Generate a cryptographically strong random MDC key */

BYTE *getStrongRandomKey( void )
	{
	BYTE randomBuffer[ MDC_BLOCKSIZE ];
	char *seedFileName;
	int i;

	/* Open the MDC seed file if we need to */
	if( seedFileFD == ERROR )
		{
		/* Build path to seed file and read in the key */
		seedFileName = getFirstKeyPath( getenv( PGPPATH ), RANDSEED_FILENAME );
		while( seedFileName != NULL )
			{
			/* Try and read the seed from the seedfile */
			if( ( ( seedFileFD = hopen( seedFileName, O_RDWR | S_DENYRDWR ) ) != ERROR ) &&
				( hread( seedFileFD, mdcKey, BLOCKSIZE ) >= BLOCKSIZE ) )
				break;		/* We've read the seed, exit */

			/* No luck, reset the error status and try the next search path */
			seedFileFD = ERROR;
			seedFileName = getNextKeyPath();
			}

		/* Make sure we got the seed */
		if( seedFileName == NULL || seedFileFD == ERROR )
			error( CANNOT_READ_RANDOM_SEEDFILE );
		}

	/* Add in any randomish information the system can give us (not terribly
	   good, but every extra bit helps - if it's there we may as well use it) */
	getRandomInfo( randomBuffer, MDC_BLOCKSIZE );
	for( i = 0; i < MDC_BLOCKSIZE; i++ )
		mdcKey[ i ] ^= randomBuffer[ i ];

	/* Step the MD5 RNG and init the MDC system with the random key */
	MD5Init( &mdSeedContext );
	MD5Update( &mdSeedContext, mdcKey, MDC_BLOCKSIZE );
	MD5Final( &mdSeedContext );
	memcpy( mdcKey, mdSeedContext.digest, BLOCKSIZE );
	initKey( mdcKey, BLOCKSIZE, DEFAULT_IV, DEFAULT_MDC_ITERATIONS );

	return( mdcKey );
	}

/* Return cryptographically strong random BYTE, WORD, LONG.  This code
   assumes initKey() has already been called, which is always the case
   in HPACK */

BYTE getStrongRandomByte( void )
	{
	BYTE *auxKey = cryptBuffer;		/* Used in mdcTransform */

	/* Step the MDC RNG if necessary */
	if( seedPos == MDC_BLOCKSIZE )
		{
		/* Transform the seedIV, with the cryptBuffer as the auxKey.  It
		   doesn't really matter what's in it */
		mdcTransform( seedIV );
		seedPos = 0;
		}

	return( mdcKey[ seedPos++ ] );
	}

WORD getStrongRandomWord( void )
	{
	/* Note that although both sides of the '|' have side-effects, we're not
	   particularly worried about the order of evaluation */
	return( ( WORD ) ( ( getStrongRandomByte() << 8 ) | getStrongRandomByte() ) );
	}

LONG getStrongRandomLong( void )
	{
	/* See comment in getStrongRandomWord() */
	return( ( ( LONG ) getStrongRandomWord() << 16 ) | getStrongRandomWord() );
	}

/* Get the first/next userID in a list of userID's.  Since PGP doesn't
   currently implement mailing lists, this is a fairly simple-minded process
   of pulling subsequent userID's from a comma-seperated list.  In future
   this will be expanded to handle full mailing lists as soon as some
   mechanism is defined in PGP */

static char *userIDlistPos, userIDvalue[ 255 ];
static int keyORlevel;

BOOLEAN getNextUserID( char **userID )
	{
	char *userIDlistPtr;
	int length;

	/* Quick check for key-or operation */
	while( !strncmp( userIDlistPos, "or(", 3 ) )
		{
		/* Go down one level and skip the 'or' keyword */
		keyORlevel++;
		userIDlistPos += 3;
		}

	/* Skip whitespace */
	while( *userIDlistPos && *userIDlistPos == ' ' )
		userIDlistPos++;

	/* Extract the next userID from the list.  When there is a proper
	   mailing list mechanism defined this will simply involve returning
	   a pointer to the next name in a linked list */
	userIDlistPtr = userIDlistPos;
	while( *userIDlistPtr && *userIDlistPtr != ',' && *userIDlistPtr != ')' )
		userIDlistPtr++;
	length = ( int ) ( userIDlistPtr - userIDlistPos );
	strncpy( userIDvalue, userIDlistPos, length );
	userIDvalue[ length ] = '\0';
	*userID = userIDvalue;
	if( *userIDlistPtr == ',' )
		userIDlistPtr++;	/* Skip comma separator */
	userIDlistPos = userIDlistPtr;

	/* Skip trailing ')'s.  We need to do this before we leave since we
	   have to be able to return a flag specifying that there are more
	   keys present */
	while( keyORlevel && *userIDlistPos == ')' )
		{
		/* Go up one level and skip ')' */
		keyORlevel--;
		userIDlistPos++;
		}

	return( ( *userIDlistPos ) ? TRUE : FALSE );
	}

BOOLEAN getFirstUserID( char **userID, const BOOLEAN isMainKey )
	{
	/* Point at main or secondary userID string */
	userIDlistPos = ( isMainKey ) ? mainUserID : secUserID;

	keyORlevel = 0;
	return( getNextUserID( userID ) );
	}
