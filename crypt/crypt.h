/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							Encryption Interface Header						*
*							 CRYPT.H  Updated 10/03/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#if defined( __ATARI__ )
  #include "io\hpackio.h"
#elif defined( __MAC__ )
  #include "hpackio.h"
#else
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* The size of the GP crypto buffer.  Set to MAX_BYTE_PRECISION * 33 */

#define CRYPT_BUFSIZE   8580

/* The type of encryption key to use (main or secondary) */

#define MAIN_KEY		TRUE
#define SECONDARY_KEY	FALSE

/* The structure for keeping track of encryption information */

typedef struct {
			   int algorithmType;		/* Encryption algorithm type */
			   } CRYPT_INFO;

/* Prototypes for the encryption functions */

void initCrypt( void );
void initPassword( const BOOLEAN isMainKey );
void encryptCFB( BYTE *buffer, int noBytes );
void decryptCFB( BYTE *buffer, int noBytes );
void saveCryptState( void );
void restoreCryptState( void );
void endCrypt( void );
void wipeFile( const char *filePath );
BYTE getStrongRandomByte( void );
WORD getStrongRandomWord( void );
LONG getStrongRandomLong( void );
BOOLEAN checkSignature( LONG dataStartPos, LONG dataLength );
int createSignature( LONG dataStartPos, LONG dataLength, char *userID );
BOOLEAN getEncryptionInfo( int *length );
int putEncryptionInfo( const BOOLEAN isMainKey );
