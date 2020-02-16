/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						 Dynamic Memory Manager Header File					*
*							 MEMMGR.C  Updated 17/03/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			  Copyright 1992  Peter C Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#ifndef _MEMMGR_DEFINED

#define _MEMMGR_DEFINED

BOOLEAN memInit( size_t size );
void **memAlloc( size_t size );
void memFree( void **block );

#endif /* _MEMMGR_DEFINED */
