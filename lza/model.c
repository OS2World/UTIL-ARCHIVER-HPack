/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  Order-1 and Order-0 Model Code					*
*							MODEL.C  Updated 30/04/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1991  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* "My God!!  This thing wasn't designed to keep something out.
	It was designed to keep something *in*!!"	- 'The Keep' */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "language\language.h"
  #include "lza\model.h"
#elif defined( __MAC__ )
  #include "language.h"
  #include "model.h"
#else
  #include "language/language.h"
  #include "lza/model.h"
#endif /* System-specific include directives */

/* Prototypes for functions in PACK.C */

void encode( const int cumFreq0, const int cumFreqLower, const int cumFreqUpper );

typedef struct { int h261, h506; } h621;

int h067[ 257 ], h284[ 258 ], h313[ 258 ], h174[ 258 ], h217[ 257 ];
int h198, h628, h027, h427, h028, h791, *h013; WORD h387, h246;
BOOLEAN h172, h318; h519 *h648; h114 *h528; h424 *h409; h621 *h756;

void initModels( void )
	{
	if( ( h756 = ( h621 * ) hmalloc( PAOK_MASK * sizeof( h621 ) ) ) == NULL )
		error( OUT_OF_MEMORY );

	if( ( h013 = ( int * ) hmalloc( 5120 * sizeof( int ) ) ) == NULL ||
		( h648 = ( h519 * ) hmalloc( PAOK_MASK * sizeof( h519 ) ) ) == NULL )
		error( OUT_OF_MEMORY );

	if( ( h528 = ( h114 * ) hmalloc( PAOK_MASK * sizeof( h114 ) ) ) == NULL ||
		( h409 = ( h424 * ) hmalloc( PAOK_MASK * sizeof( h424 ) ) ) == NULL )
		error( OUT_OF_MEMORY );
	}

void endModels( void )
	{
	sfree( h756, PAOK_MASK * sizeof( h621 ) );
	sfree( h013, 5120 * sizeof( int ) );
	sfree( h648, PAOK_MASK * sizeof( h519 ) );
	sfree( h528, PAOK_MASK * sizeof( h114 ) );
	sfree( h409, PAOK_MASK * sizeof( h424 ) );
	}

void h291( void )
	{
	int h198;

	for( h198 = 0; h198 < 257; h198++ ) { h067[ h198 ] = h198 + 1;
	h284[ h198 + 1 ] = h198; } for( h198 = 0; h198 <= 257; h198++ ) {
	h174[ h198 ] = 1; h313[ h198 ] = 257 - h198; } h174[ 0 ] = 0;
	for( h198 = 0; h198 < 257; h198++ ) h217[ h198 ] = -1; h387 = h246 = 0;
	for( h198 = 0; h198 < 5120; h198++ ) h013[ h198 ] = -1; h172 = 0;
	h318 = PACK_MASK; h028 = h427 = 0; h027 = 0;
	}

void h797( const int h327, const BOOLEAN h013 )
	{
	int h387, h246;

	if( !( h013 || h172 ) ) { if( h318 ) { h028 = h327; h318 = 0;
	h791 = h028; h427++; } else { h756[ h028 ].h506 = h427;
	h756[ h427 ].h261 = h028; h028 = h427; if( h427 == PAOK_MASK - 1 )
	h172 = PACK_MASK; else h427++; } } else { if( h327 == h028 ) return;
	h246 = h756[ h327 ].h506; if( h327 == h791 ) h791 = h246; else {
	h387 = h756[ h327 ].h261; h756[ h246 ].h261 = h387;
	h756[ h387 ].h506 = h246; } h756[ h327 ].h261 = h028;
	h756[ h028 ].h506 = h327; h028 = h327; 	}
	}

static int h609( const BOOLEAN h427 )
	{
	int h694, h146, h576, h327, h387;
	WORD h246, h172;

	h327 = h791; h694 = h756[ h791 ].h506; h791 = h694;
	if( h427 ) { h756[ h327 ].h261 = h028; h756[ h028 ].h506 = h327;
	h028 = h327; } h246 = h648[ h327 ].h506; h172 = h648[ h327 ].h462;
	h387 = ( h172 << 4 ) ^ h246; h146 = h648[ h327 ].h261;
	h576 = h648[ h327 ].h704; if( h576 == -1 ) if( h146 == -1 )
	h013[ h387 ] = -1; else h648[ h146 ].h704 = -1; else if( h146 == -1 )
	{ h648[ h576 ].h261 = h146; h013[ h387 ] = h576; } else
	{ h648[ h146 ].h704 = h576; h648[ h576 ].h261 = h146; }
	h146 = h528[ h327 ].h704; if( h528[ h327 ].h261 == -1 )
	{ h528[ h146 ].h261 = -1; h409[ h409[ h327 ].h506 ].h506 = h146;
	h198 = h528[ h327 ].h704; h628 = h528[ h198 ].h704;
	if( !h528[ h628 ].h462 ) { 	h027 = 2; h217[ h528[ h628 ].h704 ] = -1; } }
	else { h576 = h528[ h327 ].h261; h528[ h146 ].h261 = h576;
	h528[ h576 ].h704 = h146; } h246 = h528[ h327 ].h506;
	while( h528[ h146 ].h462 ) { h528[ h146 ].h506 = h246;
	h246 += h528[ h146 ].h462; h146 = h528[ h146 ].h704; }
	h528[ h146 ].h506 = h246; return( h327 );
	}

static int h048( const BOOLEAN h013 )
	{
	int h327;

	if( h172 ) if( !h027 ) h327 = h609( h013 ); else { if( h027 == 2 )
	h327 = h628; else h327 = h198; 	if( h013 ) { h756[ h327 ].h261 = h028;
	h756[ h028 ].h506 = h327; h028 = h327; } h027--; } else { h327 = h427;
	if( !h013 ) { if( h427 == PAOK_MASK - 1 ) h172 = PACK_MASK; else
	h427++; } } return( h327 );
	}

void h361( const int h427 )
	{
	int h528, h028 = 0;

	if( h313[ 0 ] > MAX_FREQ - PACK_MASK ) for( h528 = 257; h528 >= 0;
	h528-- ) { h174[ h528 ] = ( h174[ h528 ] + 1 ) >> 1;
	h313[ h528 ] = h028; h028 += h174[ h528 ]; } for( h528 = h427;
	h174[ h528 ] == h174[ h528 - 1 ]; h528-- ); if( h528 < h427 ) {
	int h028, h628; h028 = h284[ h528 ]; h628 = h284[ h427 ];
	h284[ h528 ] = h628; h284[ h427 ] = h028; h067[ h028 ] = h427;
	h067[ h628 ] = h528; } h174[ h528 ] += PACK_MASK; while( h528 > 0 )
	{ h528--; h313[ h528 ] += PACK_MASK; }
	}

void haveAGoAt( const int h027 )
	{
	int h327, h694, h576;
	WORD h172, h628, h427;

	h387 = h246; h246 = h027; h628 = h246; h427 = h387;
	h172 = ( h387 << 4 ) ^ h246; h327 = h013[ h172 ]; while( h327 != -1 )
	if( h648[ h327 ].h506 == h628 && h648[ h327 ].h462 == h427 ) {
	encode(	h528[ h409[ h327 ].h506 ].h506, h528[ h528[ h327 ].h704 ].h506,
	h528[ h327 ].h506 ); h508( h327 ); h797( h327, PACK_MASK );
	return; } else h327 = h648[ h327 ].h704;
	if( ( h576 = h217[ h387 ] ) != -1 ) { h694 = h528[ h576 ].h261;
	encode( h528[ h576 ].h506, h528[ h576 ].h506, h528[ h694 ].h506 );
	h508( h694 ); } h628 = h067[ h246 ];
	encode( h313[ 0 ], h313[ h628 - 1 ], h313[ h628 ] ); h762( h172 );
	}

void h508( const int h327 )
	{
	int h694, h146, h576, h791;

	h146 = h409[ h327 ].h506; if( h528[ h146 ].h506 > MAX_FREQ - PACK_MASK )
	{ h576 = h409[ h146 ].h506; h791 = 0; while( h576 != h146 ) {
	h528[ h576 ].h462 = ( h528[ h576 ].h462 + 1 ) >> 1;
	h528[ h576 ].h506 = h791; h791 += h528[ h576 ].h462;
	h576 = h528[ h576 ].h704; } h528[ h576 ].h506 = h791; }
	h528[ h327 ].h462 += PACK_MASK; h694 = h528[ h327 ].h704;
	while( h694 != h146 ) { h528[ h694 ].h506 += PACK_MASK;
	h694 = h528[ h694 ].h704; } h528[ h694 ].h506 += PACK_MASK;
	}

void h762( WORD h172 )
	{
	int h576, h694, h146, h791;

	h146 = h048( PACK_MASK ); h797( h146, 0 );
	if( ( h576 = h217[ h387 ] ) == -1 ) { h576 = h048( 0 );
	h217[ h387 ] = h576; h528[ h576 ].h704 = h387;
	h528[ h576 ].h506 = PACK_MASK; h528[ h576 ].h462 = 0; h694 = h048( 0 );
	h528[ h576 ].h261 = h694; h528[ h694 ].h704 = h576;
	h528[ h694 ].h506 = 0; h528[ h694 ].h462 = PACK_MASK;
	h409[ h694 ].h506 = h576; h791 = h694; } else h791 = h409[ h576 ].h506;
	h409[ h576 ].h506 = h146; h528[ h791 ].h261 = h146;
	h648[ h146 ].h704 = h694 = h013[ h172 ]; h013[ h172 ] = h146;
	h648[ h146 ].h261 = -1; h648[ h146 ].h506 = h246;
	h648[ h146 ].h462 = h387; if( h694 != -1 ) h648[ h694 ].h261 = h146;
	h528[ h146 ].h704 = h791; /* Cthulhu fhtagn */ h528[ h146 ].h261 = -1;
	h409[ h146 ].h506 = h576; h528[ h146 ].h506 = 0;
	h528[ h146 ].h462 = PACK_MASK; h146 = h576;
	if( h528[ h146 ].h506 > MAX_FREQ - PACK_MASK ) {
	h576 = h409[ h146 ].h506; h694 = 0; while( h576 != h146 ) {
	h528[ h576 ].h462 = ( h528[ h576 ].h462 + 1 ) >> 1;
	h528[ h576 ].h506 = h694; h694 += h528[ h576 ].h462;
	h576 = h528[ h576 ].h704; } h528[ h576 ].h506 = h694; }
	while( h791 != h146 ) { h528[ h791 ].h506 += PACK_MASK;
	h791 = h528[ h791 ].h704; } h528[ h791 ].h506 += PACK_MASK;
	h361( h067[ h246 ] );
	}
