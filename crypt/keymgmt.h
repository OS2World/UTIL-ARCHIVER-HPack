/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  Key Management Routines Header					*
*							KEYMGMT.H  Updated 11/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _KEYMGMT_DEFINED

#define _KEYMGMT_DEFINED

#include "defs.h"
#if defined( __ATARI__ )
  #include "crypt\packet.h"
  #ifdef RSAREF
	#include "crypt\rsadef.h"
  #else
	#include "crypt\rsa.h"
  #endif /* RSAREF */
#elif defined( __MAC__ )
  #include "packet.h"
  #ifdef RSAREF
	#include "rsadef.h"
  #else
	#include "rsa.h"
  #endif /* RSAREF */
#else
  #include "crypt/packet.h"
  #ifdef RSAREF
	#include "crypt/rsadef.h"
  #else
	#include "crypt/rsa.h"
  #endif /* RSAREF */
#endif /* System-specific include directives */

/* The information structure for public and private keys.  Both PEM/PKCS and
   PGP key packets contain more information than this but HPACK doesn't use
   any of it */

typedef struct {
			   /* Key status */
			   BOOLEAN isPemKey;			/* Whether it's a PGP or PEM key */
			   BOOLEAN isEncrypted;			/* Whether secret fields are encrypted */

			   /* General key ID information */
			   char userID[ 256 ];			/* userID associated with the key */
			   BYTE keyID[ KEYFRAG_SIZE ]; 	/* keyID for the key */
			   char ascKeyID[ ( KEYDISPLAY_SIZE * 2 ) + 1 ];
											/* ASCII hex keyID */
			   LONG timeStamp;				/* Timestamp for this key */

			   /* The key itself */
			   MP_REG n[ MAX_UNIT_PRECISION ], e[ MAX_UNIT_PRECISION ];
											/* Public key components */
			   MP_REG d[ MAX_UNIT_PRECISION ], p[ MAX_UNIT_PRECISION ], \
					  q[ MAX_UNIT_PRECISION ], u[ MAX_UNIT_PRECISION ];
			   MP_REG exponent1[ MAX_UNIT_PRECISION ], \
					  exponent2[ MAX_UNIT_PRECISION ];
											/* Secret key components */
			   } KEYINFO;

/* Whether we should try to read secret key fields when we read a key */

#define IS_PUBLICKEY	FALSE
#define IS_SECRETKEY	TRUE

/* Whether the ID we are supplying to getKey() is a keyID or userID */

#define IS_KEYID	TRUE
#define IS_USERID	FALSE

/* The type of key we want to read: PGP or PEM */

#define IS_PEM_KEY	TRUE
#define IS_PGP_KEY	FALSE

/* Whether we should set globalPrecision when calling readMPI() or not */

#define SET_PRECISION		TRUE
#define NO_SET_PRECISION	FALSE

/* The encryption password lengths.  A minimum length of 8 bytes provides a
   reasonable level of security */

#define MIN_KEYLENGTH	8
#define MAX_KEYLENGTH	80		/* Any reasonable value will do */

/* Utility routines */

void clearIDfields( KEYINFO *keyInfo );
int getCheckPacketLength( const BYTE ctb );
int readMPI( MP_REG *mpReg, BOOLEAN doSetPrecision );
BYTE *getStrongRandomKey( void );
void endKeyMgmt( void );
void wordReverse( BYTE *regPtr, int count );

/* Routines to build the path to the keyring file, and to read the file
   itself */

char *getFirstKeyPath( const char *keyringPath, const char *keyringName );
char *getNextKeyPath( void );
BOOLEAN getKey( const BOOLEAN isPemKey, const BOOLEAN isSecretKey, \
				const BOOLEAN isKeyID, char *keyFileName, BYTE *wantedID, \
				KEYINFO *keyInfo );

/* Routines to extract a userID from a list of ID's */

BOOLEAN getFirstUserID( char **userID, const BOOLEAN isMainKey );
BOOLEAN getNextUserID( char **userID );

#endif /* _KEYMGMT_DEFINED */
