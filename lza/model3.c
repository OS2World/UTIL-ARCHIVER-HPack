/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								High Position Model							*
*							 MODEL3.C  Updated 01/05/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990, 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#include "defs.h"
#if defined( __ATARI__ )
  #include "lza\model3.h"
#elif defined( __MAC__ )
  #include "model3.h"
#else
  #include "lza/model3.h"
#endif /* System-specific include directives */

/* Translation tables between high positions and symbol indices */

int highPosToIndex[ NO_HIGH_POS ];
int indexToHighPos[ NO_HIGH_POS + 1 ];

int highPosCumFreq[ NO_HIGH_POS + 1 ];	/* Cumulative symbol frequencies */
int highPosFreq[ NO_HIGH_POS + 1 ];		/* Symbol frequencies */

/* The size of the model, set depending on the window size */

extern int windowSize;

static int noHighPos;

/* Initalize the model */

void startHighPosModel( void )
	{
	int i;

	/* Set the size of the model depending on the window size */
	noHighPos = NO_HIGH_POS >> windowSize;

	/* Set up tables that translate between symbol indices and high positions */
	for( i = 0; i < noHighPos; i++ )
		{
		highPosToIndex[ i ] = i + 1;
		indexToHighPos[ i + 1 ] = i;
		}

	/* Set up initial frequency counts to be one for all symbols */
	for( i = 0; i <= noHighPos; i++ )
		{
		highPosFreq[ i ] = 1;
		highPosCumFreq[ i ] = noHighPos - i;
		}
	highPosFreq[ 0 ] = 0;	/* Freq[ 0 ] must not be the same as freq[ 1 ] */
	}

/* The counter used to trigger the adaptive count scaling */

extern unsigned int modelByteCount;

#define THRESHOLD	20000

#define SCALE1		2
#define SCALE2		5

/* Update the model to account for a new symbol */

void updateHighPosModel( const int symbol )
	{
	int i, cum = 0;				/* New index for symbol */
	int scaleValue;

	/* Change the scalevalue according to how many chars we have processed */
	scaleValue = ( modelByteCount < THRESHOLD ) ? SCALE1 : SCALE2;

	/* See if frequency counts are at their maximum */
	if( highPosCumFreq[ 0 ] > MAX_FREQ - scaleValue )
		{
		/* If so, halve all the counts (keeping them non-zero) */
		for( i = noHighPos; i >= 0; i-- )
			{
			highPosFreq[ i ] = ( highPosFreq[ i ] + 1 ) / 2;
			highPosCumFreq[ i ] = cum;
			cum += highPosFreq[ i ];
			}
		}

	/* Find the symbol's new index */
	for( i = symbol; highPosFreq[ i ] == highPosFreq[ i - 1 ]; i-- );

	/* Update the translation tables if the symbol has moved */
	if( i < symbol )
		{
		int ch_i, ch_symbol;

		ch_i = indexToHighPos[ i ];
		ch_symbol = indexToHighPos[ symbol ];
		indexToHighPos[ i ] = ch_symbol;
		indexToHighPos[ symbol ] = ch_i;
		highPosToIndex[ ch_i ] = symbol;
		highPosToIndex[ ch_symbol ] = i;
		}

	/* Increment the freq.count for the symbol and update the cum.freq's */
	highPosFreq[ i ] += scaleValue;
	while( i > 0 )
		{
		i--;
		highPosCumFreq[ i ] += scaleValue;
		}
	}
