/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*								Low Position Model							*
*							MODEL4.C  Updated 01/05/91						*
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
  #include "lza\model4.h"
#elif defined( __MAC__ )
  #include "model4.h"
#else
  #include "lza/model4.h"
#endif /* System-specific include directives */

/* Translation tables between low positions and symbol indices */

int lowPosToIndex[ NO_LOW_POS ];
int indexToLowPos[ NO_LOW_POS + 1 ];

int lowPosCumFreq[ NO_LOW_POS + 1 ];	/* Cumulative symbol frequencies */
int lowPosFreq[ NO_LOW_POS + 1 ];		/* Symbol frequencies */

/* Initalize the model */

void startLowPosModel( void )
	{
	int i;

	/* Set up tables that translate between symbol indices and low positions */
	for( i = 0; i < NO_LOW_POS; i++ )
		{
		lowPosToIndex[ i ] = i + 1;
		indexToLowPos[ i + 1 ] = i;
		}

	/* Set up initial frequency counts to be one for all symbols */
	for( i = 0; i <= NO_LOW_POS; i++ )
		{
		lowPosFreq[ i ] = 1;
		lowPosCumFreq[ i ] = NO_LOW_POS - i;
		}
	lowPosFreq[ 0 ] = 0;	/* Freq[ 0 ] must not be the same as freq[ 1 ] */
	}

/* The counter used to trigger the adaptive count scaling */

extern unsigned int modelByteCount;

#define THRESHOLD	30000

#define SCALE1		1
#define SCALE2		10

/* Update the model to account for a new symbol */

void updateLowPosModel( const int symbol )
	{
	int i, cum = 0;				/* New index for symbol */
	int scaleValue;

	/* Change the scalevalue according to how many chars we have processed */
	scaleValue = ( modelByteCount < THRESHOLD ) ? SCALE1 : SCALE2;

	/* See if frequency counts are at their maximum */
	if( lowPosCumFreq[ 0 ] > MAX_FREQ - scaleValue )
		{
		/* If so, halve all the counts (keeping them non-zero) */
		for( i = NO_LOW_POS; i >= 0; i-- )
			{
			lowPosFreq[ i ] = ( lowPosFreq[ i ] + 1 ) / 2;
			lowPosCumFreq[ i ] = cum;
			cum += lowPosFreq[ i ];
			}
		}

	/* Find the symbol's new index */
	for( i = symbol; lowPosFreq[ i ] == lowPosFreq[ i - 1 ]; i-- );

	/* Update the translation tables if the symbol has moved */
	if( i < symbol )
		{
		int ch_i, ch_symbol;

		ch_i = indexToLowPos[ i ];
		ch_symbol = indexToLowPos[ symbol ];
		indexToLowPos[ i ] = ch_symbol;
		indexToLowPos[ symbol ] = ch_i;
		lowPosToIndex[ ch_i ] = symbol;
		lowPosToIndex[ ch_symbol ] = i;
		}

	/* Increment the freq.count for the symbol and update the cum.freq's */
	lowPosFreq[ i ] += scaleValue;
	while( i > 0 )
		{
		i--;
		lowPosCumFreq[ i ] += scaleValue;
		}
	}
