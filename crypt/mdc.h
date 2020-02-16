/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  MDC Message Digest Cipher Header					*
*							  MDC.C  Updated 12/08/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1992  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#include "defs.h"
#if defined( __ATARI__ )
  #include "crypt\md5.h"
  #include "crypt\shs.h"
#elif defined( __MAC__ )
  #include "md5.h"
  #include "shs.h"
#else
  #include "crypt/md5.h"
  #include "crypt/shs.h"
#endif /* System-specific include directives */

/* The block size (in bytes) */

#define BLOCKSIZE		16
#define MDC_BLOCKSIZE	16	/* Temp.hack */

/* The default IV value used to seed the cipher in initKey().  Changing this
   for each file precludes the use of precomputed encrypted data for very
   fast checking against known plaintext */

#define DEFAULT_IV		( ( BYTE * ) "\0\0\0\0\0\0\0\0" )

#define IV_SIZE			8

/* The default number of iterations for the key setup */

#define DEFAULT_MDC_ITERATIONS	100

/* The CPU endianness - determined at runtime in CRYPT.C */

extern BOOLEAN bigEndian;

/* Define for simple block encryption */

void MD5Transform( LONG *digest, LONG *data );
void longReverse( LONG *buffer, int byteCount );

#define mdcTransform(iv)    if( !bigEndian ) \
								longReverse( ( LONG * ) iv, BLOCKSIZE ); \
							MD5Transform( ( LONG * ) iv, ( LONG * ) auxKey ); \
							if( !bigEndian ) \
								longReverse( ( LONG * ) iv, BLOCKSIZE )

/* Prototypes for functions in MDC.C */

void initKey( BYTE *key, int keyLength, const BYTE *iv, int noIterations );
BYTE *getIV( void );
