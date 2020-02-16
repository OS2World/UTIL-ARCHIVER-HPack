/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   High Position Model Interface					*
*							 MODEL3.H  Updated 14/03/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990, 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* The set of symbols that may be encoded */

#define NO_HIGH_POS	256

/* Translation tables between high positions and symbol indices */

extern int highPosToIndex[];
extern int indexToHighPos[];

/* Cumulative frequency table */

#ifndef MAX_FREQ
  #define MAX_FREQ	16383				/* Max freq.count: 2^14 - 1 */
#endif /* !MAX_FREQ */

extern int highPosCumFreq[];			/* Cumulative symbol frequencies */

/* Prototypes for functions in MODEL3.C */

void startHighPosModel( void );
void updateHighPosModel( const int symbol );
