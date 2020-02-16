#include <stdlib.h>
#include <string.h>
#include "md5.h"

/* CPU endianness */

extern BOOLEAN bigEndian;

/****************************************************************************
*																			*
*	Implementation RSA Data Security, Inc. MD5 Message-Digest Algorithm		*
*	Created 2/17/90 RLR, revised 1/91 SRD,AJ,BSK,JT Reference C Version		*
*	  Copyright (C) 1990, RSA Data Security, Inc. All rights reserved		*
*																			*
****************************************************************************/

/* To form the message digest for a message M, initialize a context buffer
   mdContext using MD5Init(); call MD5Update() on mdContext and M; and call
   MD5Final() on mdContext.  The message digest is now in
   mdContext->digest[ 0 ... 15 ] */

/****************************************************************************
*																			*
*							The MD5 Transformation							*
*																			*
****************************************************************************/

/* The Mysterious Constants used in the MD5 transformation */

static LONG md5const[] = {
	3614090360UL, 3905402710UL,  606105819UL, 3250441966UL,
	4118548399UL, 1200080426UL, 2821735955UL, 4249261313UL,
	1770035416UL, 2336552879UL, 4294925233UL, 2304563134UL,
	1804603682UL, 4254626195UL, 2792965006UL, 1236535329UL,
	4129170786UL, 3225465664UL,  643717713UL, 3921069994UL,
	3593408605UL,   38016083UL, 3634488961UL, 3889429448UL,
	 568446438UL, 3275163606UL, 4107603335UL, 1163531501UL,
	2850285829UL, 4243563512UL, 1735328473UL, 2368359562UL,
	4294588738UL, 2272392833UL, 1839030562UL, 4259657740UL,
	2763975236UL, 1272893353UL, 4139469664UL, 3200236656UL,
	 681279174UL, 3936430074UL, 3572445317UL,   76029189UL,
	3654602809UL, 3873151461UL,  530742520UL, 3299628645UL,
	4096336452UL, 1126891415UL, 2878612391UL, 4237533241UL,
	1700485571UL, 2399980690UL, 4293915773UL, 2240044497UL,
	1873313359UL, 4264355552UL, 2734768916UL, 1309151649UL,
	4149444226UL, 3174756917UL,  718787259UL, 3951481745UL
	};
/* Storage for the Mysterious Constants (either md5const or user-defined) */

LONG mConst[ MD5_ROUNDS ];

#ifndef ASM_MD5

/* F, G, H and I are basic MD5 functions */

#define F(X,Y,Z)	( ( X & Y ) | ( ~X & Z ) )
#define G(X,Y,Z)	( ( X & Z ) | ( Y & ~Z ) )
#define H(X,Y,Z)	( X ^ Y ^ Z )
#define I(X,Y,Z)	( Y ^ ( X | ~Z ) )

/* ROTATE_LEFT rotates x left n bits */

#define ROTATE_LEFT(x,n)	( ( x << n ) | ( x >> ( 32 - n ) ) )

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.  Rotation is
   separate from addition to prevent recomputation */

#define FF(A,B,C,D,X,shiftAmt,magicConst) \
	{ \
	A += F( B, C, D ) + X + magicConst; \
	A = ROTATE_LEFT( A, shiftAmt ); \
	A += B; \
	}

#define GG(A,B,C,D,X,shiftAmt,magicConst) \
	{ \
	A += G( B, C, D ) + X + magicConst; \
	A = ROTATE_LEFT( A, shiftAmt ); \
	A += B; \
	}

#define HH(A,B,C,D,X,shiftAmt,magicConst) \
	{ \
	A += H( B, C, D ) + X + magicConst; \
	A = ROTATE_LEFT( A, shiftAmt ); \
	A += B; \
	}

#define II(A,B,C,D,X,shiftAmt,magicConst) \
	{ \
	A += I( B, C, D ) + X + magicConst; \
	A = ROTATE_LEFT( A, shiftAmt ); \
	A += B; \
	}

/* Round 1 shift amounts */

#define S11	7
#define S12	12
#define S13	17
#define S14	22

/* Round 2 shift amounts */

#define S21 5
#define S22 9
#define S23 14
#define S24 20

/* Round 3 shift amounts */

#define S31 4
#define S32 11
#define S33 16
#define S34 23

/* Round 4 shift amounts */

#define S41 6
#define S42 10
#define S43 15
#define S44 21

/* Basic MD5 step. Transforms buf based on in.  Note that if the Mysterious
   Constants are arranged backwards in little-endian order and decrypted with
   the DES they produce OCCULT MESSAGES! */

#if defined( __ARC__ ) || defined( IRIX ) || defined( __TSC__ )

/* MD5Transform is split into its component rounds since many optimizers
   choke on the grand unified version */

static void MD5TransformRound1( LONG *A, LONG *B, LONG *C, LONG *D, LONG *in )
	{
	/* Round 1 */
	FF( *A, *B, *C, *D, in[  0 ], S11, mConst[  0 ] );	/*  1 */
	FF( *D, *A, *B, *C, in[  1 ], S12, mConst[  1 ] );	/*  2 */
	FF( *C, *D, *A, *B, in[  2 ], S13, mConst[  2 ] );	/*  3 */
	FF( *B, *C, *D, *A, in[  3 ], S14, mConst[  3 ] );	/*  4 */
	FF( *A, *B, *C, *D, in[  4 ], S11, mConst[  4 ] );	/*  5 */
	FF( *D, *A, *B, *C, in[  5 ], S12, mConst[  5 ] );	/*  6 */
	FF( *C, *D, *A, *B, in[  6 ], S13, mConst[  6 ] );	/*  7 */
	FF( *B, *C, *D, *A, in[  7 ], S14, mConst[  7 ] );	/*  8 */
	FF( *A, *B, *C, *D, in[  8 ], S11, mConst[  8 ] );	/*  9 */
	FF( *D, *A, *B, *C, in[  9 ], S12, mConst[  9 ] );	/* 10 */
	FF( *C, *D, *A, *B, in[ 10 ], S13, mConst[ 10 ] );	/* 11 */
	FF( *B, *C, *D, *A, in[ 11 ], S14, mConst[ 11 ] );	/* 12 */
	FF( *A, *B, *C, *D, in[ 12 ], S11, mConst[ 12 ] );	/* 13 */
	FF( *D, *A, *B, *C, in[ 13 ], S12, mConst[ 13 ] );	/* 14 */
	FF( *C, *D, *A, *B, in[ 14 ], S13, mConst[ 14 ] );	/* 15 */
	FF( *B, *C, *D, *A, in[ 15 ], S14, mConst[ 15 ] );	/* 16 */
	}

static void MD5TransformRound2( LONG *A, LONG *B, LONG *C, LONG *D, LONG *in )
	{
	/* Round 2 */
	GG( *A, *B, *C, *D, in[  1 ], S21, mConst[ 16 ] );	/* 17 */
	GG( *D, *A, *B, *C, in[  6 ], S22, mConst[ 17 ] );	/* 18 */
	GG( *C, *D, *A, *B, in[ 11 ], S23, mConst[ 18 ] );	/* 19 */
	GG( *B, *C, *D, *A, in[  0 ], S24, mConst[ 19 ] );	/* 20 */
	GG( *A, *B, *C, *D, in[  5 ], S21, mConst[ 20 ] );	/* 21 */
	GG( *D, *A, *B, *C, in[ 10 ], S22, mConst[ 21 ] );	/* 22 */
	GG( *C, *D, *A, *B, in[ 15 ], S23, mConst[ 22 ] );	/* 23 */
	GG( *B, *C, *D, *A, in[  4 ], S24, mConst[ 23 ] );	/* 24 */
	GG( *A, *B, *C, *D, in[  9 ], S21, mConst[ 24 ] );	/* 25 */
	GG( *D, *A, *B, *C, in[ 14 ], S22, mConst[ 25 ] );	/* 26 */
	GG( *C, *D, *A, *B, in[  3 ], S23, mConst[ 26 ] );	/* 27 */
	GG( *B, *C, *D, *A, in[  8 ], S24, mConst[ 27 ] );	/* 28 */
	GG( *A, *B, *C, *D, in[ 13 ], S21, mConst[ 28 ] );	/* 29 */
	GG( *D, *A, *B, *C, in[  2 ], S22, mConst[ 29 ] );	/* 30 */
	GG( *C, *D, *A, *B, in[  7 ], S23, mConst[ 30 ] );	/* 31 */
	GG( *B, *C, *D, *A, in[ 12 ], S24, mConst[ 31 ] );	/* 32 */
	}

static void MD5TransformRound3( LONG *A, LONG *B, LONG *C, LONG *D, LONG *in )
	{
	/* Round 3 */
	HH( *A, *B, *C, *D, in[  5 ], S31, mConst[ 32 ] );	/* 33 */
	HH( *D, *A, *B, *C, in[  8 ], S32, mConst[ 33 ] );	/* 34 */
	HH( *C, *D, *A, *B, in[ 11 ], S33, mConst[ 34 ] );	/* 35 */
	HH( *B, *C, *D, *A, in[ 14 ], S34, mConst[ 35 ] );	/* 36 */
	HH( *A, *B, *C, *D, in[  1 ], S31, mConst[ 36 ] );	/* 37 */
	HH( *D, *A, *B, *C, in[  4 ], S32, mConst[ 37 ] );	/* 38 */
	HH( *C, *D, *A, *B, in[  7 ], S33, mConst[ 38 ] );	/* 39 */
	HH( *B, *C, *D, *A, in[ 10 ], S34, mConst[ 39 ] );	/* 40 */
	HH( *A, *B, *C, *D, in[ 13 ], S31, mConst[ 40 ] );	/* 41 */
	HH( *D, *A, *B, *C, in[  0 ], S32, mConst[ 41 ] );	/* 42 */
	HH( *C, *D, *A, *B, in[  3 ], S33, mConst[ 42 ] );	/* 43 */
	HH( *B, *C, *D, *A, in[  6 ], S34, mConst[ 43 ] );	/* 44 */
	HH( *A, *B, *C, *D, in[  9 ], S31, mConst[ 44 ] );	/* 45 */
	HH( *D, *A, *B, *C, in[ 12 ], S32, mConst[ 45 ] );	/* 46 */
	HH( *C, *D, *A, *B, in[ 15 ], S33, mConst[ 46 ] );	/* 47 */
	HH( *B, *C, *D, *A, in[  2 ], S34, mConst[ 47 ] );	/* 48 */
	}

static void MD5TransformRound4( LONG *A, LONG *B, LONG *C, LONG *D, LONG *in )
	{
	/* Round 4 */
	II( *A, *B, *C, *D, in[  0 ], S41, mConst[ 48 ] );	/* 49 */
	II( *D, *A, *B, *C, in[  7 ], S42, mConst[ 49 ] );	/* 50 */
	II( *C, *D, *A, *B, in[ 14 ], S43, mConst[ 50 ] );	/* 51 */
	II( *B, *C, *D, *A, in[  5 ], S44, mConst[ 51 ] );	/* 52 */
	II( *A, *B, *C, *D, in[ 12 ], S41, mConst[ 52 ] );	/* 53 */
	II( *D, *A, *B, *C, in[  3 ], S42, mConst[ 53 ] );	/* 54 */
	II( *C, *D, *A, *B, in[ 10 ], S43, mConst[ 54 ] );	/* 55 */
	II( *B, *C, *D, *A, in[  1 ], S44, mConst[ 55 ] );	/* 56 */
	II( *A, *B, *C, *D, in[  8 ], S41, mConst[ 56 ] );	/* 57 */
	II( *D, *A, *B, *C, in[ 15 ], S42, mConst[ 57 ] );	/* 58 */
	II( *C, *D, *A, *B, in[  6 ], S43, mConst[ 58 ] );	/* 59 */
	II( *B, *C, *D, *A, in[ 13 ], S44, mConst[ 59 ] );	/* 60 */
	II( *A, *B, *C, *D, in[  4 ], S41, mConst[ 60 ] );	/* 61 */
	II( *D, *A, *B, *C, in[ 11 ], S42, mConst[ 61 ] );	/* 62 */
	II( *C, *D, *A, *B, in[  2 ], S43, mConst[ 62 ] );	/* 63 */
	II( *B, *C, *D, *A, in[  9 ], S44, mConst[ 63 ] );	/* 64 */
	}

void MD5Transform( LONG *buf, LONG *in )
	{
	LONG A = buf[ 0 ], B = buf[ 1 ], C = buf[ 2 ], D = buf[ 3 ];

	MD5TransformRound1 (&A, &B, &C, &D, in);
	MD5TransformRound2 (&A, &B, &C, &D, in);
	MD5TransformRound3 (&A, &B, &C, &D, in);
	MD5TransformRound4 (&A, &B, &C, &D, in);

	buf[ 0 ] += A;
	buf[ 1 ] += B;
	buf[ 2 ] += C;
	buf[ 3 ] += D;
	}

#else

void MD5Transform( LONG *buf, LONG *in )
	{
	LONG A = buf[ 0 ], B = buf[ 1 ], C = buf[ 2 ], D = buf[ 3 ];

	/* Round 1 */
	FF( A, B, C, D, in[  0 ], S11, mConst[  0 ] );	/*  1 */
	FF( D, A, B, C, in[  1 ], S12, mConst[  1 ] );	/*  2 */
	FF( C, D, A, B, in[  2 ], S13, mConst[  2 ] );	/*  3 */
	FF( B, C, D, A, in[  3 ], S14, mConst[  3 ] );	/*  4 */
	FF( A, B, C, D, in[  4 ], S11, mConst[  4 ] );	/*  5 */
	FF( D, A, B, C, in[  5 ], S12, mConst[  5 ] );	/*  6 */
	FF( C, D, A, B, in[  6 ], S13, mConst[  6 ] );	/*  7 */
	FF( B, C, D, A, in[  7 ], S14, mConst[  7 ] );	/*  8 */
	FF( A, B, C, D, in[  8 ], S11, mConst[  8 ] );	/*  9 */
	FF( D, A, B, C, in[  9 ], S12, mConst[  9 ] );	/* 10 */
	FF( C, D, A, B, in[ 10 ], S13, mConst[ 10 ] );	/* 11 */
	FF( B, C, D, A, in[ 11 ], S14, mConst[ 11 ] );	/* 12 */
	FF( A, B, C, D, in[ 12 ], S11, mConst[ 12 ] );	/* 13 */
	FF( D, A, B, C, in[ 13 ], S12, mConst[ 13 ] );	/* 14 */
	FF( C, D, A, B, in[ 14 ], S13, mConst[ 14 ] );	/* 15 */
	FF( B, C, D, A, in[ 15 ], S14, mConst[ 15 ] );	/* 16 */

	/* Round 2 */
	GG( A, B, C, D, in[  1 ], S21, mConst[ 16 ] );	/* 17 */
	GG( D, A, B, C, in[  6 ], S22, mConst[ 17 ] );	/* 18 */
	GG( C, D, A, B, in[ 11 ], S23, mConst[ 18 ] );	/* 19 */
	GG( B, C, D, A, in[  0 ], S24, mConst[ 19 ] );	/* 20 */
	GG( A, B, C, D, in[  5 ], S21, mConst[ 20 ] );	/* 21 */
	GG( D, A, B, C, in[ 10 ], S22, mConst[ 21 ] );	/* 22 */
	GG( C, D, A, B, in[ 15 ], S23, mConst[ 22 ] );	/* 23 */
	GG( B, C, D, A, in[  4 ], S24, mConst[ 23 ] );	/* 24 */
	GG( A, B, C, D, in[  9 ], S21, mConst[ 24 ] );	/* 25 */
	GG( D, A, B, C, in[ 14 ], S22, mConst[ 25 ] );	/* 26 */
	GG( C, D, A, B, in[  3 ], S23, mConst[ 26 ] );	/* 27 */
	GG( B, C, D, A, in[  8 ], S24, mConst[ 27 ] );	/* 28 */
	GG( A, B, C, D, in[ 13 ], S21, mConst[ 28 ] );	/* 29 */
	GG( D, A, B, C, in[  2 ], S22, mConst[ 29 ] );	/* 30 */
	GG( C, D, A, B, in[  7 ], S23, mConst[ 30 ] );	/* 31 */
	GG( B, C, D, A, in[ 12 ], S24, mConst[ 31 ] );	/* 32 */

	/* Round 3 */
	HH( A, B, C, D, in[  5 ], S31, mConst[ 32 ] );	/* 33 */
	HH( D, A, B, C, in[  8 ], S32, mConst[ 33 ] );	/* 34 */
	HH( C, D, A, B, in[ 11 ], S33, mConst[ 34 ] );	/* 35 */
	HH( B, C, D, A, in[ 14 ], S34, mConst[ 35 ] );	/* 36 */
	HH( A, B, C, D, in[  1 ], S31, mConst[ 36 ] );	/* 37 */
	HH( D, A, B, C, in[  4 ], S32, mConst[ 37 ] );	/* 38 */
	HH( C, D, A, B, in[  7 ], S33, mConst[ 38 ] );	/* 39 */
	HH( B, C, D, A, in[ 10 ], S34, mConst[ 39 ] );	/* 40 */
	HH( A, B, C, D, in[ 13 ], S31, mConst[ 40 ] );	/* 41 */
	HH( D, A, B, C, in[  0 ], S32, mConst[ 41 ] );	/* 42 */
	HH( C, D, A, B, in[  3 ], S33, mConst[ 42 ] );	/* 43 */
	HH( B, C, D, A, in[  6 ], S34, mConst[ 43 ] );	/* 44 */
	HH( A, B, C, D, in[  9 ], S31, mConst[ 44 ] );	/* 45 */
	HH( D, A, B, C, in[ 12 ], S32, mConst[ 45 ] );	/* 46 */
	HH( C, D, A, B, in[ 15 ], S33, mConst[ 46 ] );	/* 47 */
	HH( B, C, D, A, in[  2 ], S34, mConst[ 47 ] );	/* 48 */

	/* Round 4 */
	II( A, B, C, D, in[  0 ], S41, mConst[ 48 ] );	/* 49 */
	II( D, A, B, C, in[  7 ], S42, mConst[ 49 ] );	/* 50 */
	II( C, D, A, B, in[ 14 ], S43, mConst[ 50 ] );	/* 51 */
	II( B, C, D, A, in[  5 ], S44, mConst[ 51 ] );	/* 52 */
	II( A, B, C, D, in[ 12 ], S41, mConst[ 52 ] );	/* 53 */
	II( D, A, B, C, in[  3 ], S42, mConst[ 53 ] );	/* 54 */
	II( C, D, A, B, in[ 10 ], S43, mConst[ 54 ] );	/* 55 */
	II( B, C, D, A, in[  1 ], S44, mConst[ 55 ] );	/* 56 */
	II( A, B, C, D, in[  8 ], S41, mConst[ 56 ] );	/* 57 */
	II( D, A, B, C, in[ 15 ], S42, mConst[ 57 ] );	/* 58 */
	II( C, D, A, B, in[  6 ], S43, mConst[ 58 ] );	/* 59 */
	II( B, C, D, A, in[ 13 ], S44, mConst[ 59 ] );	/* 60 */
	II( A, B, C, D, in[  4 ], S41, mConst[ 60 ] );	/* 61 */
	II( D, A, B, C, in[ 11 ], S42, mConst[ 61 ] );	/* 62 */
	II( C, D, A, B, in[  2 ], S43, mConst[ 62 ] );	/* 63 */
	II( B, C, D, A, in[  9 ], S44, mConst[ 63 ] );	/* 64 */

	buf[ 0 ] += A;
	buf[ 1 ] += B;
	buf[ 2 ] += C;
	buf[ 3 ] += D;
	}
#endif /* __ARC__ || IRIX || __TSC__ */

#endif /* ASM_MD5 */

/****************************************************************************
*																			*
*							MD5 Support Routines							*
*																			*
****************************************************************************/

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords.  It is possible to make the code endianness-
   independant by fiddling around with data at the byte level, but this
   makes for very slow code, so we rely on the user to sort out endianness
   at compile time */

void longReverse( LONG *buffer, int byteCount )
	{
	LONG value;

	byteCount /= sizeof( LONG );
	while( byteCount-- )
		{
		value = ( *buffer << 16 ) | ( *buffer >> 16 );
		*buffer++ = ( ( value & 0xFF00FF00L ) >> 8 ) | ( ( value & 0x00FF00FFL ) << 8 );
		}
	}

/* The external buffer for saving the Mysterious Constants.  Since the MD5/
   MDC code is dual-use, we need to save the constants whenever we switch
   from MDC to MD5 */

extern BYTE cryptBuffer[];

/* The routine MD5SetConst sets the Mysterious Constants to either the
   standard MD5 ones or to a user-defined set for MDC */

static BOOLEAN isMD5const = FALSE;

void MD5SetConst( BYTE *buffer )
	{
	if( buffer == NULL )
		{
		/* If the constants are already set up, don't bother re-setting them */
		if( !isMD5const )
			{
			memcpy( mConst, md5const, MD5_ROUNDS * sizeof( LONG ) );
			isMD5const = TRUE;
			}
		}
	else
		{
		/* Copy the values to the mConst array, with endianness conversion
		   if necessary */
		memcpy( mConst, buffer, MD5_ROUNDS * sizeof( LONG ) );
		if( !bigEndian )
			longReverse( mConst, MD5_ROUNDS * sizeof( LONG ) );
		isMD5const = FALSE;
		}
	}

/* The routine MD5Init initializes the message-digest context mdContext. All
   fields are set to zero */

void MD5Init( MD5_CTX *mdContext )
	{
	mdContext->i[ 0 ] = mdContext->i[ 1 ] = 0L;

	/* Load magic initialization constants */
	mdContext->buf[ 0 ] = 0x67452301L;
	mdContext->buf[ 1 ] = 0xEFCDAB89L;
	mdContext->buf[ 2 ] = 0x98BADCFEL;
	mdContext->buf[ 3 ] = 0x10325476L;

	/* Set up the Mysterious Constants if necessary */
	if( !isMD5const )
		{
		memcpy( cryptBuffer, mConst, MD5_ROUNDS * sizeof( LONG ) );
		MD5SetConst( NULL );
		}
	}

/* The routine MD5Update updates the message-digest context to account for
   the presence of each of the characters inBuf[ 0 .. inLen-1 ] in the
   message whose digest is being computed.  This is an optimized version
   which assumes that the buffer is a multiple of MD5_BLOCKSIZE bytes long */

#ifdef __MSDOS__

void MD5Update( MD5_CTX *mdContext, BYTE *buffer, unsigned int noBytes )
	{
	int bufIndex = 0;

	/* Update number of bits */
	if( ( mdContext->i[ 0 ] + ( ( LONG ) noBytes << 3 ) ) < mdContext->i[ 0 ] )
		mdContext->i[ 1 ]++;	/* Carry from low to high bitCount */
	mdContext->i[ 0 ] += ( ( LONG ) noBytes << 3 );
	mdContext->i[ 1 ] += ( ( LONG ) noBytes >> 29 );

	/* Process data in MD5_BLOCKSIZE chunks */
	while( noBytes >= MD5_BLOCKSIZE )
		{
		MD5Transform( mdContext->buf, ( LONG * ) ( buffer + bufIndex ) );
		bufIndex += MD5_BLOCKSIZE;
		noBytes -= MD5_BLOCKSIZE;
		}

	/* Handle any remaining bytes of data.  This should only happen once
	   on the final lot of data */
	memcpy( mdContext->in, buffer + bufIndex, noBytes );
	}

/* The routine MD5Final terminates the message-digest computation and ends
   with the desired message digest in mdContext->digest[ 0 ... 15 ] */

void MD5Final( MD5_CTX *mdContext )
	{
	int count;
	LONG lowBitcount = mdContext->i[ 0 ], highBitcount = mdContext->i[ 1 ];

	/* Compute number of bytes mod 64 */
	count = ( int ) ( ( lowBitcount >> 3 ) & 0x3F );

	/* Set the first char of padding to 0x80.  This is safe since there is
	   always at least one byte free */
	( ( BYTE * ) mdContext->in )[ count++ ] = 0x80;

	/* Pad out to 56 mod 64 */
	if( count > 56 )
		{
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset( ( BYTE * ) &mdContext->in + count, 0, 64 - count );
		MD5Transform( mdContext->buf, ( LONG * ) mdContext->in );

		/* Now fill the next block with 56 bytes */
		memset( &mdContext->in, 0, 56 );
		}
	else
		/* Pad block to 56 bytes */
		memset( ( BYTE * ) &mdContext->in + count, 0, 56 - count );

	/* Append length in bits and transform */
	( ( LONG * ) mdContext->in )[ 14 ] = lowBitcount;
	( ( LONG * ) mdContext->in )[ 15 ] = highBitcount;

	MD5Transform( mdContext->buf, ( LONG * ) mdContext->in );

	/* Store buffer in digest */
	memcpy( mdContext->digest, mdContext->buf, 16 );

	/* Restore the previous Mysterious Constants */
	memcpy( mConst, cryptBuffer, MD5_ROUNDS * sizeof( LONG ) );
	isMD5const = FALSE;
	}

#else

void MD5Update( MD5_CTX *mdContext, BYTE *inBuf, unsigned int inLen )
	{
	int mdi;
	LONG in[ 16 ];
	unsigned int i, ii;

	/* Compute number of bytes mod 64 */
	mdi = ( int ) ( ( mdContext->i[ 0 ] >> 3 ) & 0x3F );

	/* Update number of bits */
	if( ( mdContext->i[ 0 ] + ( ( LONG ) inLen << 3 ) ) < mdContext->i[ 0 ] )
		mdContext->i[ 1 ]++;	/* Carry from low to high bitCount */
	mdContext->i[ 0 ] += ( ( LONG ) inLen << 3 );
	mdContext->i[ 1 ] += ( ( LONG ) inLen >> 29 );

	while( inLen-- )
		{
		/* Add new character to buffer, increment mdi */
		mdContext->in[ mdi++ ] = *inBuf++;

		/* Transform if necessary */
		if( mdi == 0x40 )
			{
			for( i = 0, ii = 0; i < 16; i++, ii += 4 )
				in[ i ] = ( ( ( LONG ) mdContext->in[ ii + 3 ] ) << 24 ) | \
						  ( ( ( LONG ) mdContext->in[ ii + 2 ] ) << 16 ) | \
						  ( ( ( LONG ) mdContext->in[ ii + 1 ] ) << 8 ) | \
						  ( ( LONG ) mdContext->in[ ii ] );
			MD5Transform( mdContext->buf, in );
			mdi = 0;
			}
		}
	}

/* The routine MD5Final terminates the message-digest computation and ends
   with the desired message digest in mdContext->digest[ 0 ... 15 ] */

void MD5Final( MD5_CTX *mdContext )
	{
	int mdi, padLen;
	BYTE padding[ 64 ];
	unsigned int i, ii;
	LONG in[ 16 ];

	/* Save number of bits */
	in[ 14 ] = mdContext->i[ 0 ];
	in[ 15 ] = mdContext->i[ 1 ];

	/* Compute number of bytes mod 64 */
	mdi = ( int ) ( ( mdContext->i[ 0 ] >> 3 ) & 0x3F );

	/* Pad out to 56 mod 64 */
	padLen = ( mdi < 56 ) ? ( 56 - mdi ) : ( 120 - mdi );
	padding[ 0 ] = 0x80;
	memset( padding + 1, 0, padLen - 1 );
	MD5Update( mdContext, padding, padLen );

	/* Append length in bits and transform */
	for( i = 0, ii = 0; i < 14; i++, ii += 4 )
		in[ i ] = ( ( ( LONG ) mdContext->in[ ii + 3 ] ) << 24 ) | \
				  ( ( ( LONG ) mdContext->in[ ii + 2 ] ) << 16 ) | \
				  ( ( ( LONG ) mdContext->in[ ii + 1 ] ) << 8 ) | \
				  ( ( LONG ) mdContext->in[ ii ] );
	MD5Transform( mdContext->buf, in );

	/* Store buffer in digest */
	for( i = 0, ii = 0; i < 4; i++, ii += 4 )
		{
		mdContext->digest[ ii ] = ( BYTE ) ( mdContext->buf[ i ] & 0xFF );
		mdContext->digest[ ii + 1 ] = ( BYTE ) ( ( mdContext->buf[ i ] >> 8 ) & 0xFF );
		mdContext->digest[ ii + 2 ] = ( BYTE ) ( ( mdContext->buf[ i ] >> 16 ) & 0xFF );
		mdContext->digest[ ii + 3 ] = ( BYTE ) ( ( mdContext->buf[ i ] >> 24 ) & 0xFF );
		}

	/* Restore the previous Mysterious Constants */
	memcpy( mConst, cryptBuffer, MD5_ROUNDS * sizeof( LONG ) );
	isMD5const = FALSE;
	}
#endif /* !__MSDOS__ */
