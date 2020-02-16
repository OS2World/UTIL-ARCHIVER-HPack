/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  NIST Secure Hash Standard Header					*
*							  SHS.C  Updated 02/09/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1992  Peter C. Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#ifndef _SHS_DEFINED

#define _SHS_DEFINED

#include "defs.h"

/* The SHS block size and message digest sizes, in bytes */

#define SHS_BLOCKSIZE	64
#define SHS_DIGESTSIZE	20

/* The structure for storing SHS info */

typedef struct {
			   LONG digest[ 5 ];			/* Message digest */
			   LONG countLo, countHi;		/* 64-bit bit count */
			   LONG data[ 16 ];				/* SHS data buffer */
			   } SHS_INFO;

/* Message digest functions */

void shsInit( SHS_INFO *shsInfo );
void shsTransform( SHS_INFO *shsInfo );
void shsUpdate( SHS_INFO *shsInfo, BYTE *buffer, int count );
void shsFinal( SHS_INFO *shsInfo );

#endif /* _SHS_DEFINED */
