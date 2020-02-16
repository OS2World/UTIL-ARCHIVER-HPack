/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							Error Correction Header File					*
*							  ECC.H  Updated 05/01/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _ECC_DEFINED

#define _ECC_DEFINED

#include "defs.h"

/* The structure for keeping track of the ECC information */

typedef struct {
			   int eccType;		/* Type of ECC used */
			   } ECC_INFO;

#endif /* !_ECC_DEFINED */
