/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   Low Position Model Interface						*
*							MODEL4.H  Updated 03/08/90						*
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

#define NO_LOW_POS	128

/* Translation tables between low positions and symbol indices */

extern int lowPosToIndex[];
extern int indexToLowPos[];

/* Cumulative frequency table */

#ifndef MAX_FREQ
  #define MAX_FREQ	16383				/* Max freq.count: 2^14 - 1 */
#endif /* !MAX_FREQ */

extern int lowPosCumFreq[];				/* Cumulative symbol frequencies */

/* Prototypes for functions in MODEL4.C */

void startLowPosModel( void );
void updateLowPosModel( const int symbol );
