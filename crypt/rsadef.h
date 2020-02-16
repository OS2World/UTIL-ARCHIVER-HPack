/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							 RSAREF Library Interface						*
*							RSADEF.H  Updated 25/02/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1993  Peter C.Gutmann  All rights reserved			*
*																			*
****************************************************************************/

/* This file is an alternative to the usual RSA.H which is needed for
   versions of HPACK using the RSAREF toolkit */

#include "defs.h"

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

/* MAX_BIT_PRECISION is the expected upper limit on key size.  The normal
   RSAREF code only goes to 1024 bits, the HPACK version has had a define
   changed to allow 2048-bit keys */

#define	MAX_BIT_PRECISION	2080
#define	MAX_BYTE_PRECISION	( MAX_BIT_PRECISION / 8 )
#define	MAX_UNIT_PRECISION	( MAX_BIT_PRECISION / UNITSIZE )

extern int globalPrecision;			/* Level of precision for all routines */

/* Number of carry bits used in intermediate calculations - not needed in
   this version */

#define SLOP_BITS			0

/* Defines for the RSA routines themselves */

void mp_setp( const int precision );

/* Equivalent functions for some of the primitives */

#define setPrecision(prec)	globalPrecision = ( prec )
#define mp_move(dest,src)	memcpy( dest, src, globalPrecision * sizeof( MP_REG ) )
#define mp_clear(reg,count)	memset( reg, 0, ( count ) * sizeof( MP_REG ) )

/* Defines to perform various tests on MPI's */

#define countbits(mpi)	initBitSniffer( mpi )
#define countbytes(mpi)	( ( countbits( mpi ) + 7 ) >> 3 )

/* Prototypes for functions in normally in RSA.C.  These have been replaced
   by equivalent routines in CRYPT.C */

int initBitSniffer( MP_REG *mpReg );
int significance( MP_REG *mpReg );
void mp_init( MP_REG *mpReg, WORD value );
