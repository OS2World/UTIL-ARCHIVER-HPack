/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					Routines to Parse PEM-Encoded Data Files				*
*							PEMPARSE.C  Updated 13/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/


/* Parse/decode data encoded in the format specified in RFC-1113.

   "When the X.500 revolution comes, your name will be lined up against the
	wall and shot"
		-- John Gilmore */

#include <string.h>
#include "defs.h"
#include "script.h"
#if defined( __ATARI__ )
  #include "asn1\pemcode.h"
#elif defined( __MAC__ )
  #include "pemcode.h"
#else
  #include "asn1/pemcode.h"
#endif /* System-specific include directives */

/* Delimiter strings in PEM public/private keys */

char PEM_PUBKEY_BEGIN[] = "-----BEGIN PUBLIC KEY-----";
char PEM_PUBKEY_END[] = "-----END PUBLIC KEY-----";
char PEM_SECKEY_BEGIN[] = "-----BEGIN PRIVATE KEY-----";
char PEM_SECKEY_END[] = "-----END PRIVATE KEY-----";

/* ID strings in PEM public/private keys */

char PEM_ID_USER[] = "User:";
char PEM_ID_PUBKEY[] = "PublicKeyInfo:";
char PEM_ID_SECKEY[] = "EncryptedPrivateKeyInfo:";
char PEM_ID_MD5OFKEY[] = "MD5OfPublicKey:";

#define PEM_ID_USER_SIZE		sizeof( PEM_ID_USER ) - 1
#define PEM_ID_PUBKEY_SIZE		sizeof( PEM_ID_PUBKEY ) - 1
#define PEM_ID_SECKEY_SIZE		sizeof( PEM_ID_SECKEY ) - 1
#define PEM_ID_MD5OFKEY_SIZE	sizeof( PEM_ID_MD5OFKEY ) - 1

/* State of current PEM file parsing.  This is described by the NFSA:

	header ->	HEADER userID ;
	userID ->	USERID userID | USERID secKey | USERID pubKey ;
	pubKey ->	PUBLIC_KEY secKey | PUBLIC_KEY userID ;
	secKey ->	SECRET_KEY userID ;

   which is equivalent to the DFSA:

	header ->	HEADER userID ;
	userID ->   USERID idOrPubOrSec ;
	idOrPubOrSec ->	USERID idOrPubOrSec | SECRET_KEY userID | PUBLIC_KEY idOrSec ;
	idOrSec -> USERID idOrPubOrSec | SECRET_KEY userID ; */

enum { STATE_HEADER, STATE_USERID, STATE_ID_OR_PUB_OR_SEC, STATE_ID_OR_SEC };

/* Parse a PEM-encoded file.  The FSM implemented by this code is fairly
   non-rigorous - it skips some fields altogether (MD5OfKey) and just keeps
   going until it hits EOF, not checking for the end-of-key delimiter
   strings.  The PEM format has so much redundancy that this is hardly
   necessary */

BOOLEAN parsePemFile( void )
	{
	char inBuffer[ 100 ], userID[ 256 ];
	BYTE keyInfoBuffer[ 512 ];
	int lineLength, errPos /*, line = 0*/;
	int state = STATE_HEADER;

	while( TRUE )
		{
		if( readLine( inBuffer, 100, &lineLength, &errPos ) != READLINE_NO_ERROR )
			return( ERROR );

		switch( state )
			{
			case STATE_HEADER:
				if( !strcmp( inBuffer, PEM_PUBKEY_BEGIN ) ||
					!strcmp( inBuffer, PEM_SECKEY_BEGIN ) )
					state = STATE_USERID;
				break;

			case STATE_USERID:
				if( !strncmp( inBuffer, PEM_ID_USER, PEM_ID_USER_SIZE ) )
					{
					strcpy( userID, inBuffer + PEM_ID_USER_SIZE + 1 );
					state = STATE_ID_OR_PUB_OR_SEC;
					}
				break;

			case STATE_ID_OR_PUB_OR_SEC:
				if( !strncmp( inBuffer, PEM_ID_PUBKEY, PEM_ID_PUBKEY_SIZE ) )
					{
					/* Found public key */
					readPemField( keyInfoBuffer );
					state = STATE_ID_OR_SEC;
					}
				else
					if( !strncmp( inBuffer, PEM_ID_SECKEY, PEM_ID_SECKEY_SIZE ) )
						{
						/* Found secret key */
						readPemField( keyInfoBuffer );
						state = STATE_USERID;
						}
					else
						if( !strncmp( inBuffer, PEM_ID_USER, PEM_ID_USER_SIZE ) )
							/* Found userID, stay in same state */
							strcpy( userID, inBuffer + PEM_ID_USER_SIZE + 1 );
				break;

			case STATE_ID_OR_SEC:
				if( !strncmp( inBuffer, PEM_ID_SECKEY, PEM_ID_SECKEY_SIZE ) )
					{
					/* Found secret key */
					readPemField( keyInfoBuffer );
					state = STATE_USERID;
					}
				else
					if( !strncmp( inBuffer, PEM_ID_USER, PEM_ID_USER_SIZE ) )
						/* Found userID, stay in same state */
						strcpy( userID, inBuffer + PEM_ID_USER_SIZE + 1 );
				break;
			}

		/* Exit if we've run out of data */
		if( !lineLength )
			break;

/*		line++;	*/
		}

	return( OK );
	}
