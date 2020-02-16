/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							   RSA Library Routines							*
*							  RSA.C  Updated 18/04/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Parts copyright 1991  Philip Zimmermann.  Used with permission		*
*		  Adapted 1991, 1992  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#include <string.h>
#include "defs.h"
#if defined( __ATARI__ )
  #include "crypt\keymgmt.h"
  #include "crypt\rsa.h"
#elif defined( __MAC__ )
  #include "keymgmt.h"
  #include "rsa.h"
#else
  #include "crypt/keymgmt.h"
  #include "crypt/rsa.h"
#endif /* System-specific include directives */

/* General-purpose encryption buffer, defined in CRYPT.C */

extern BYTE *cryptBuffer;

/* Level of precision for all routines */

int globalPrecision;

/****************************************************************************
*																			*
*							RSA Library Support Routines					*
*																			*
****************************************************************************/

/* Set the precision to which MP numbers are calculated */

int precMod8, precDiv8;		/* Global precision mod 8, div 8 */

void mp_setp( const int precision )
	{
	globalPrecision = precision;
	precMod8 = precision & 7;
	precDiv8 = precision >> 3;
	}

/* Returns number of significant units in mpReg */

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

/* Begin sniffing at the MSB */

WORD bitMask;

int initBitSniffer( MP_REG *mpReg )
	{
	int bits, precision = significance( mpReg );

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

/* Init multiprecision register mpReg with short value.  Note that mp_init
   doesn't extend sign bit for >32767 */

void mp_init( MP_REG *mpReg, WORD value )
	{
	*mpReg++ = value;
	mp_clear( mpReg, globalPrecision - 1 );
	}

/* Compares multiprecision registers r1 and r2, and returns -1 if r1 < r2,
   0 if r1 == r2, or +1 if r1 > r2 */

static int mp_compare( MP_REG *r1, MP_REG *r2 )
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

static void mp_dec( MP_REG *mpReg )
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

/****************************************************************************
*																			*
*						Multiprecision Integer Primitives					*
*																			*
****************************************************************************/

#ifndef ASM_RSA

/* Add one MP number to another */

void mp_add( MP_REG *reg1, MP_REG *reg2 )
	{
	int count = precDiv8, carry = 0;
	long value;

	switch( precMod8 )
		{
		case 0:
			do {
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 7:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 6:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 5:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 4:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 3:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 2:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;

		case 1:
			value = ( long ) *reg1 + ( long ) *reg2 + carry;
			*reg1++ += *reg2++ + carry;
			carry = ( value & 0x00010000L ) ? 1 : 0;
			} while( count-- > 0 );
		}
	}

/* Subtract one MP number from another */

int mp_sub( MP_REG *reg1, MP_REG *reg2 )
	{
	int count = precDiv8, borrow = 0;
	long value;

	switch( precMod8 )
		{
		case 0:
			do {
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 7:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 6:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 5:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 4:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 3:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 2:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;

		case 1:
			value = ( long ) *reg1 - ( long ) ( *reg2 + borrow );
			*reg1++ -= *reg2++ + borrow;
			borrow = ( value & 0x00010000L ) ? 1 : 0;
			} while( count-- > 0 );
		}

	return( borrow );
	}

/* Rotate a MP number */

void mp_rotateleft( MP_REG *reg, const int carry )
	{
	int carryIn = carry, carryOut, count = precDiv8;

	switch( precMod8 )
		{
		case 0:
			do {
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 7:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 6:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 5:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 4:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 3:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 2:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;

		case 1:
			carryOut = ( *reg & 0x8000 ) ? 1 : 0;
			*reg = ( *reg << 1 ) | carryIn;
			carryIn = carryOut;
			reg++;
			} while( count-- > 0 );
		}
	}
#endif /* !ASM_RSA */

/****************************************************************************
*																			*
*							RSA Signature-Check Routine						*
*																			*
****************************************************************************/

/* Charlie Merritt's MODMULT algorithm as implemented by Phil Zimmerman,
   refined by Zhahai Stewart to reduce the number of subtracts, and hacked by
   Peter Gutmann. It performs a multiply combined with a modulo operation,
   without going into "double precision".  It is faster than the generic
   peasant method and still works well on machines that lack a fast hardware
   multiply instruction */

/*	Shift r1 one whole unit to the left */

static void mp_shiftleftUnit( MP_REG *r1 )
	{
	int precision = globalPrecision;
	MP_REG *r2;

	r1 += precision - 1;	/* Point to end of register */
	r2 = r1;
	while( --precision )
		*r1-- = *--r2;
	*r1 = 0;
	}

/* Preshifted images of the modulus, and the multiplicand mod n */

static MP_REG *moduliBuffer;
static MP_REG *mpdBuffer;

/* To optimize msubs, we need the following 2 unit arrays, each filled with
   the most significant unit and next-most significant unit of the preshifted
   images of the modulus */

static MP_REG msUnitModuli[ UNITSIZE + 1 ];		/* Most sig. unit */
static MP_REG nmsUnitModuli[ UNITSIZE + 1 ];	/* Next-most sig. unit */

/* The following macro is used to calculate indices into the MP buffers, and
   is needed because of C's difficulty with handling dynamically allocated
   multidimensional arrays */

#define MP(i)	( ( i ) * MAX_UNIT_PRECISION )

/* Computes UNITSIZE preshifted images of r1, each shifted left 1 more bit */

static void stageMPimages( MP_REG *r1 )
	{
	int i;
	MP_REG *mpdBufPtr;	/* Pointer to address in mpdBuffer */

	/* Use cryptBuffer as scratch space */
	mpdBuffer = ( MP_REG * ) cryptBuffer;

	/* Copy in first image */
	mp_move( mpdBuffer, r1 );

	/* Compute the preshifted images */
	for( i = 1; i < UNITSIZE; i++ )
		{
		mpdBufPtr = mpdBuffer + MP( i );
		mp_move( mpdBufPtr, mpdBuffer + MP( i - 1 ) );
		mp_shiftleft( mpdBufPtr );
		}
	}

/* Compute UNITSIZE + 1 images of modulus, each shifted left 1 more bit.
   Before calling mp_modmult, we must first stage the moduli images by
   calling stageModulus().  Assumes that global_precision has already been
   adjusted to the size of the modulus, plus SLOP_BITS */

void stageModulus( MP_REG *modulus )
	{
	int i;
	MP_REG *modBufPtr;	/* Pointer to address in moduliBuffer */

	/* Use more of cryptBuffer as scratch space */
	moduliBuffer = ( MP_REG * ) cryptBuffer + ( UNITSIZE * MAX_UNIT_PRECISION );

	/* Set up table for use by msubs macro; first do zero'th element */
	mp_move( moduliBuffer, modulus );
	modulus += globalPrecision - 1;
	msUnitModuli[ 0 ] = *modulus--;
	nmsUnitModuli[ 0 ] = *modulus;

	/* Now do rest of table */
	for( i = 1; i < UNITSIZE + 1; i++ )
		{
		modBufPtr = moduliBuffer + MP( i );
		mp_move( modBufPtr, moduliBuffer + MP( i - 1 ) );
		mp_shiftleft( modBufPtr );
		modBufPtr += globalPrecision - 1;
		msUnitModuli[ i ] = *modBufPtr--;
		nmsUnitModuli[ i ] = *modBufPtr;
		}
	}

#define sniffAdd(i)	if( *multiplier & ( 1 << i ) ) \
						mp_add( product, mpdBuffer + MP( i ) )

/* To optimize msubs, compare the most significant units of the product and
   the shifted modulus before deciding to call the full mp_compare routine.
   Better still, compare the upper two units before deciding to call mp_compare */

#define msubs(i)	if( ( ( p_m = *msUnitProduct - msUnitModuli[ i ] ) > 0 ) || \
						( !p_m && ( ( *nmsUnitProduct > nmsUnitModuli[ i ] ) || \
						  ( ( *nmsUnitProduct == nmsUnitModuli[ i ] ) && \
							( ( mp_compare( product, moduliBuffer + MP( i ) ) >= 0 ) ) ) ) ) ) \
						mp_sub( product, moduliBuffer + MP( i ) )

/*	Performs combined multiply/modulo operation. Computes:
	product = ( multiplicand * multiplier ) mod modulus */

void mp_modmult( MP_REG *product, MP_REG *multiplicand, MP_REG *multiplier )
	{
	/* p_m, msUnitProduct, and nmsUnitProduct are used by the msubs macro */
	int p_m;
	MP_REG *msUnitProduct;	/* Pointer to most significant unit of product */
	MP_REG *nmsUnitProduct;	/* Next-most signif. unit of product */
	int multPrec;			/* Precision of multiplier, in units */

	/* Compute preshifted images of multiplicand, mod n */
	stageMPimages( multiplicand );

	/* To optimize msubs, set up msUnitProduct and nmsUnitProduct as pointers
	   to MSU and next-MSU of product */
	msUnitProduct = product + globalPrecision - 1;
	nmsUnitProduct = msUnitProduct - 1;

	/* To understand this algorithm, it would be helpful to first study the
	   conventional generic peasant (aka Edouard) modmult algorithm.  This
	   one does about the same thing as generic peasant, but is organized
	   differently to save some steps.  It loops through the multiplier a
	   word (unit) at a time, instead of a bit at a time.  It word-shifts the
	   product instead of bit-shifting it, so it should be faster.  It also
	   does about half as many subtracts as the generic peasant algorithm */
	mp_init( product, 0 );			/* Set product to 0 */

	/* Normalize and compute number of units in multiplier first: */
	multPrec = significance( multiplier );
	multiplier += multPrec - 1;	/* Start at MSU of multiplier */

	/* Loop for the number of units in the multiplier */
	while( multPrec-- )
		{
		/*	Shift the product one whole unit to the left.  This is part of
			the multiply phase of modmult */
		mp_shiftleftUnit( product );

		/* Sniff each bit in the current unit of the multiplier, and
		   conditionally add the the corresponding preshifted image of the
		   multiplicand to the product.  This is also part of the multiply
		   phase of modmult.  The following loop is unrolled for speed,
		   using macros:

		   for( i = UNITSIZE - 1; i >= 0; i-- )
			  if( *multiplier & ( 1 << i ) )
				 mp_add( product, mpdBuffer[ i ] ); */
		sniffAdd( 15 );
		sniffAdd( 14 );
		sniffAdd( 13 );
		sniffAdd( 12 );
		sniffAdd( 11 );
		sniffAdd( 10 );
		sniffAdd( 9 );
		sniffAdd( 8 );
		sniffAdd( 7 );
		sniffAdd( 6 );
		sniffAdd( 5 );
		sniffAdd( 4 );
		sniffAdd( 3 );
		sniffAdd( 2 );
		sniffAdd( 1 );
		sniffAdd( 0 );

		/* The product may have grown by as many as UNITSIZE + 1 bits.
		   That's why we have globalPrecision set to the size of the modulus
		   plus UNITSIZE + 1 slop bits.  Now reduce the product back down by
		   conditionally subtracting the UNITSIZE + 1 preshifted images of
		   the modulus.  This is the modulo reduction phase of modmult.  The
		   following loop is unrolled for speed, using macros:

		   for( i = UNITSIZE; i >= 0; i-- )
			  if( mp_compare( product, moduliBuffer[ i ] ) >= 0 )
				 mp_sub( product, moduliBuffer[ i ] ); */
		msubs( 16 );
		msubs( 15 );
		msubs( 14 );
		msubs( 13 );
		msubs( 12 );
		msubs( 11 );
		msubs( 10 );
		msubs( 9 );
		msubs( 8 );
		msubs( 7 );
		msubs( 6 );
		msubs( 5 );
		msubs( 4 );
		msubs( 3 );
		msubs( 2 );
		msubs( 1 );
		msubs( 0 );

		/* Bump pointer to next lower unit of multiplier */
		multiplier--;
		}
	}

/* Generic peasant combined exponentiation/modulo algorithm.  Calls modmult
   instead of mult.  Computes: expout = ( expin ** exponent ) mod modulus */

#define sniffBit(exp)		( *( exp ) & bitMask )
#define bumpBitSniffer(exp)	if( !( bitMask >>= 1 ) ) \
								{ \
								bitMask = 0x8000; \
								exp--; \
								}

void mp_modexp( MP_REG *expOut, MP_REG *expIn, MP_REG *exponent, MP_REG *modulus )
	{
	int bits;
	int savedPrecision = globalPrecision;
	MP_REG product[ MAX_UNIT_PRECISION ];

	/* Clear output register at full precision before we reset it to the
	   smallest optimum precision */
	mp_clear( expOut, globalPrecision );

	/* Set smallest optimum precision for this modulus and set up preshifted
	   images of the modulus */
	setPrecision( bits2units( countbits( modulus ) + SLOP_BITS ) );
	stageModulus( modulus );

	/* Compute number of bits in exponent and normalize it */
	bits = initBitSniffer( exponent );
	normalize( exponent );

	/* We can optimize out the first modsquare and modmult since we know for
	   sure at this point that bits > 0 */
	bits--;
	bumpBitSniffer( exponent );
	mp_move( expOut, expIn );		/* expOut = ( 1 * 1 ) * expin */

	while( bits-- )
		{
		mp_modsquare( product, expOut );
		mp_move( expOut, product );

		if( sniffBit( exponent ) )
			{
			mp_modmult( product, expOut, expIn );
			mp_move( expOut, product );
			}
		bumpBitSniffer( exponent );
		}
	setPrecision( savedPrecision );	/* Restore original precision */

	/* Destroy sensitive data */
	mp_burn( product, MAX_UNIT_PRECISION );
	mp_burn( msUnitModuli, UNITSIZE + 1 );
	mp_burn( nmsUnitModuli, UNITSIZE + 1 );
	}

/****************************************************************************
*																			*
*								RSA Decryption Routine						*
*																			*
****************************************************************************/

/* Generic peasant multiply algorithm */

static void mp_mult( MP_REG *product, MP_REG *multiplicand, MP_REG *multiplier)
	{
	int bits;

	mp_init( product, 0 );
	if( testEq( multiplicand, 0 ) )
		/* Multiplying by zero gives zero */
		return;

	/* Compute number of bits in multiplier and normalize it */
	bits = initBitSniffer( multiplier );
	normalize( multiplier );
	while( bits-- )
		{
		mp_shiftleft( product );
		if( sniffBit( multiplier ) )
			mp_add( product, multiplicand );
		bumpBitSniffer( multiplier );
		}
	}

/* Unsigned divide routine */

static void mp_mod( MP_REG *remainder, MP_REG *dividend, MP_REG *divisor )
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

/* Use the Chinese Remainder Theorem shortcut for RSA decryption.
   M is the output plaintext message, C is the input ciphertext message,
   d is the secret decryption exponent, p and q are the prime factors of n,
   u is the multiplicative inverse of q, mod p.  n, the common modulus, is not
   used because of the Chinese Remainder Theorem shortcut */

void rsaDecrypt( MP_REG *M, MP_REG *C, KEYINFO *privateKey )
	{
	MP_REG p2[ MAX_UNIT_PRECISION ], q2[ MAX_UNIT_PRECISION ];
	MP_REG temp1[ MAX_UNIT_PRECISION ], temp2[ MAX_UNIT_PRECISION ];
	MP_REG *d = privateKey->d, *p = privateKey->p;
	MP_REG *q = privateKey->q, *u = privateKey->u;

	/* Initialize result in case of error */
	mp_init( M, 1 );

	/* Make sure that p < q */
	if( mp_compare( p, q ) >= 0 )
		{
		/* Swap the pointers p and q */
		MP_REG *tmp = p;
		p = q;
		q = tmp;
		}

	/* Rather than decrypting by computing modexp with full mod n precision,
	   compute a shorter modexp with mod p precision:

	   p2 = ( ( C mod p ) ** ( d mod ( p - 1 ) ) ) mod p */
	mp_move( temp1, p );
	mp_dec( temp1 );			/* temp1 = p - 1 */
	mp_mod( temp2, d, temp1 );	/* temp2 = d mod ( p - 1 ) ) */
	mp_mod( temp1, C, p );		/* temp1 = C mod p  */
	mp_modexp( p2, temp1, temp2, p );

	/* Now compute a short modexp with mod q precision:

	   q2 = ( ( C mod q ) ** ( d mod ( q - 1 ) ) ) mod q */
	mp_move( temp1, q );
	mp_dec( temp1 );			/* temp1 = q - 1 */
	mp_mod( temp2, d, temp1 );	/* temp2 = d mod ( q - 1 ) */
	mp_mod( temp1, C, q );		/* temp1 = C mod q  */
	mp_modexp( q2, temp1, temp2, q );

	/* Now use the multiplicative inverse u to glue together the two halves,
	   saving a lot of time by avoiding a full mod n exponentiation.  At key
	   generation time, u was computed with the secret key as the
	   multiplicative inverse of p, mod q such that ( p * u ) mod q = 1,
	   assuming p < q */
	if( mp_compare( p2, q2 ) == 0 )
		/* Only happens if C < p */
		mp_move( M, p2 );
	else
		{
		/* q2 = q2 - p2; if q2 < 0 then q2 = q2 + q */
		if( mp_sub( q2, p2 ) )
			/* If the result went negative, add q to q2 */
			mp_add( q2, q );

		/* M = ( ( ( q2 * u ) mod q ) * p ) + p2 */
		mp_mult( temp1, q2, u );	/* temp1 = q2 * u  */
		mp_mod( temp2, temp1, q );	/* temp2 = temp1 mod q */
		mp_mult( temp1, p, temp2 );	/* temp1 = p * temp2 */
		mp_add( temp1, p2 );		/* temp1 = temp1 + p2 */
		mp_move( M, temp1 );		/* M = temp1 */
		}

	/* Destroy sensitive data */
	mp_burn( p2, MAX_UNIT_PRECISION );
	mp_burn( q2, MAX_UNIT_PRECISION );
	mp_burn( temp1, MAX_UNIT_PRECISION );
	mp_burn( temp2, MAX_UNIT_PRECISION );
	}
