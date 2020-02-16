/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  LZSS Code Length Model						*
*							MODEL2.C  Updated 08/04/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include "defs.h"
#if defined( __ATARI__ )
  #include "lza\model2.h"
#elif defined( __MAC__ )
  #include "model2.h"
#else
  #include "lza/model2.h"
#endif /* System-specific include directives */

/* Translation tables between length count and symbol indices */

int lengthToIndex[ NO_LENGTHS ];
int indexToLength[ NO_LENGTHS + 1 ];

int lengthCumFreq[ NO_LENGTHS + 1 ];	/* Cumulative symbol frequencies */
int lengthFreq[ NO_LENGTHS + 1 ];		/* Symbol frequencies */

/* The scale value */

#define SCALE_VALUE		10

/* Initalize the model */

void startLengthModel( void )
	{
	int i;

	/* Set up tables that translate between symbol indices and high positions */
	for( i = 0; i < NO_LENGTHS; i++ )
		{
		lengthToIndex[ i ] = i + 1;
		indexToLength[ i + 1 ] = i;
		}

	/* Set up initial frequency counts to be one for all symbols */
	for( i = 0; i <= NO_LENGTHS; i++ )
		{
		lengthFreq[ i ] = 1;
		lengthCumFreq[ i ] = NO_LENGTHS - i;
		}
	lengthFreq[ 0 ] = 0;	/* Freq[ 0 ] must not be the same as freq[ 1 ] */
	}

/* Update the model to account for a new symbol */

void updateLengthModel( const int symbol )
	{
	int i, cum = 0;				/* New index for symbol */

	/* See if frequency counts are at their maximum */
	if( lengthCumFreq[ 0 ] > MAX_FREQ - SCALE_VALUE )
		{
		/* If so, halve all the counts (keeping them non-zero) */
		for( i = NO_LENGTHS; i >= 0; i-- )
			{
			lengthFreq[ i ] = ( lengthFreq[ i ] + 1 ) / 2;
			lengthCumFreq[ i ] = cum;
			cum += lengthFreq[ i ];
			}
		}

	/* Find the symbol's new index */
	for( i = symbol; lengthFreq[ i ] == lengthFreq[ i - 1 ]; i-- );

	/* Update the translation tables if the symbol has moved */
	if( i < symbol )
		{
		int ch_i, ch_symbol;

		ch_i = indexToLength[ i ];
		ch_symbol = indexToLength[ symbol ];
		indexToLength[ i ] = ch_symbol;
		indexToLength[ symbol ] = ch_i;
		lengthToIndex[ ch_i ] = symbol;
		lengthToIndex[ ch_symbol ] = i;
		}

	/* Increment the freq.count for the symbol and update the cum.freq's */
	lengthFreq[ i ] += SCALE_VALUE;
	while( i > 0 )
		{
		i--;
		lengthCumFreq[ i ] += SCALE_VALUE;
		}
	}
