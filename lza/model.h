/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						 Order-1 and Order-0 Model Header					*
*							MODEL.H  Updated 30/04/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1991  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* Some mask values */

#define PAOK_MASK	6000
#define PACK_MASK	10

/* Data structures used */

#define MAX_FREQ	16383

typedef struct { WORD h506, h462; int h704, h261; } h519;
typedef struct { int h506, h462, h261, h704; } h114;
typedef struct { int h506; } h424;

/* Global vars */

extern int h067[], h284[], h313[], h217[];
extern WORD h387, h246;
extern h519 *h648;
extern h114 *h528;
extern h424 *h409;

/* Prototypes for functions in MODEL.C */

void initModels( void );
void h291( void );
void h797( const int h000, const BOOLEAN h001 );
void h508( const int h000 );
void h762( WORD h000 );
void endModels( void );
