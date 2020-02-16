/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							   RSA Library Interface						*
*							  RSA.H  Updated 23/08/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991  Phil Zimmermann.  All rights reserved			*
*						Adapted 1991  Peter C.Gutmann						*
*																			*
****************************************************************************/

#include "defs.h"

/* Define the following to enable fast assembly-language RSA primitives */

#if defined( __ARC__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  #define ASM_RSA
#endif /* __ARC__ || __MSDOS16__ || __MSDOS32__ */

/* Basic type for the multiprecision registers.  The internal representation
   for these extended precision integer registers is an array of "units",
   where a unit is a machine word, in this case a WORD.  To perform arithmetic
   on these huge precision integers, we pass pointers to these unit arrays to
   various subroutines */

#define MP_REG			WORD

/* Macros to convert a bitcount to various other values */

#define UNITSIZE		16			/* 16 bits per machine word */

#define bits2units(n)	( ( ( n ) + 15 ) >> 4 )
#define units2bits(n)	( ( n ) << 4 )
#define bytes2units(n)	( ( ( n ) + 1 ) >> 1 )
#define units2bytes(n)	( ( n ) << 1 )
#define bits2bytes(n)	( ( ( n ) + 7 ) >> 3 )

/* MAX_BIT_PRECISION is the expected upper limit on key size.  Note that
   MAX_BIT_PRECISION >= max.key length + SLOP_BITS.  In our case we have
   a maximum key length of 2048 bits + 17 slop bits = 2080 bits (rounded) */

#define	MAX_BIT_PRECISION	2080
#define	MAX_BYTE_PRECISION	( MAX_BIT_PRECISION / 8 )
#define	MAX_UNIT_PRECISION	( MAX_BIT_PRECISION / UNITSIZE )

extern int globalPrecision;			/* Level of precision for all routines */

/* Defines for the RSA routines themselves */

#define mp_burn				mp_clear		/* For destroying sensitive data */
#define mp_modsquare(r1,r2)	mp_modmult( r1, r2, r2 )
#define mp_shiftleft(r1)	mp_rotateleft( r1, 0 )

/* Equivalent functions for some of the multiprecision primitives */

#define setPrecision(prec)	mp_setp( prec )
#define mp_move(dest,src)	memcpy( dest, src, globalPrecision * sizeof( MP_REG ) )
#define mp_clear(reg,count)	memset( reg, 0, ( count ) * sizeof( MP_REG ) )

/* The following primitives should be coded in assembly language if possible */

void mp_setp( const int precision );
void mp_add( MP_REG *reg1, MP_REG *reg2 );
int mp_sub( MP_REG *reg1, MP_REG *reg2 );
void mp_rotateleft( MP_REG *reg1, const int carry );

/* SLOP_BITS is how many "carry bits" to allow for intermediate calculation
   results to exceed the size of the modulus.  It is used by modexp to give
   some overflow elbow room for modmult to use to perform modulo operations
   with the modulus.  The number of slop bits required is determined by the
   modmult algorithm */

#define SLOP_BITS		( UNITSIZE + 1 )

/* Defines to perform various tests on MPI's */

#define testEq(mpi,val)	( ( *( mpi ) == ( val ) ) && ( significance( mpi ) <= 1 ) )
#define countbits(mpi)	initBitSniffer( mpi )
#define countbytes(mpi)	( ( countbits( mpi ) + 7 ) >> 3 )
#define normalize(mpi)	mpi += significance( mpi ) - 1

/* Prototypes for functions in RSA.C */

int initBitSniffer( MP_REG *mpReg );
int significance( MP_REG *mpReg );
void mp_init( MP_REG *mpReg, WORD value );
