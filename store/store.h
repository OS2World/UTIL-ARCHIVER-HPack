/****************************************************************************
*                                                                           *
*                          HPACK Multi-System Archiver                 		*
*                          ===========================                      *
*                                                                           *
*                 High-Speed Data Movement Routines Header File             *
*							STORE.H   Updated 17/07/91						*
*                                                                           *
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*                                                                           *
* 		Copyright 1989 - 1991  Peter C.Gutmann.  All rights reserved       	*
*                                                                           *
****************************************************************************/

#ifndef _STORE_DEFINED

#define _STORE_DEFINED		/* Flag the fact that we've been included */

/* Prototypes for functions in STORE.C */

LONG store( BOOLEAN *isText );
BOOLEAN unstore( long noBytes );
void moveData( long noBytes );

#endif /* !_STORE_DEFINED */
