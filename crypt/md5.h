/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  MD5 Message Digest Code Header					*
*							  MD5.C  Updated 21/09/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#ifndef _MD5_DEFINED

#define _MD5_DEFINED

#include "defs.h"

/****************************************************************************
*																			*
*			RSA Data Security, Inc. MD5 Message-Digest Algorithm			*
*  Created 2/17/90 RLR, revised 12/27/90 SRD,AJ,BSK,JT Reference C version	*
*	Revised (for MD5) RLR 4/27/91: G modified to have y&~z instead of y&z,	*
* FF, GG, HH modified to add in last register done, access pattern: round 2 *
*	works mod 5, round 3 works mod 3, distinct additive constant for each	*
*					step round 4 added, working mod 7						*
*																			*
****************************************************************************/

/* The size of an MD5 data block and the number of rounds in the MD5 transformation */

#define MD5_BLOCKSIZE	64
#define MD5_DIGESTSIZE	16
#define MD5_ROUNDS		64

/* The structure for storing MD5 info */

typedef struct {
			   LONG i[ 2 ];			/* Number of bits handled mod 2^64 */
			   LONG buf[ 4 ];		/* Scratch buffer */
			   BYTE in[ MD5_BLOCKSIZE ];	/* Input buffer */
			   BYTE digest[ 16 ];	/* Actual digest after MD5Final() call */
			   } MD5_CTX;

/* Message digest functions */

void MD5SetConst( BYTE *buffer );
void MD5Init( MD5_CTX *mdContext );
void MD5Update( MD5_CTX *mdContext, BYTE *buffer, unsigned int noBytes );
void MD5Final( MD5_CTX *mdContext );

#endif /* _MD5_DEFINED */
