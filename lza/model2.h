/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  LZSS Code Length Model Interface					*
*							 MODEL2.H  Updated 08/04/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* The set of symbols that may be encoded */

#define NO_LENGTHS	64 - 4

/* Translation tables between high positions and symbol indices */

extern int lengthToIndex[];
extern int indexToLength[];

/* Cumulative frequency table */

#ifndef MAX_FREQ
  #define MAX_FREQ	16383				/* Max freq.count: 2^14 - 1 */
#endif /* !MAX_FREQ */

extern int lengthCumFreq[];				/* Cumulative symbol frequencies */

/* Prototypes for functions in MODEL2.C */

void startLengthModel( void );
void updateLengthModel( const int symbol );
