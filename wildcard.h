/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						Header File for the Wildcard Routines				*
*							WILDCARD.H  Updated 20/06/90					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989, 1990 Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* The maximum allowable length of the destination string for the
   compileString() function */

#define MATCH_DEST_LEN	100

/* Symbolic define for whether a string has wildcards */

#define HAS_WILDCARDS	TRUE

/* Possible error return values for compileString */

enum { WILDCARD_FORMAT_ERROR = -1, WILDCARD_COMPLEX_ERROR = -2 };

/* Prototypes for functions in WILDCARD.C */

BOOLEAN hasWildcards( char *string, int length );
BOOLEAN strHasWildcards( char *string );
int compileString( const char *src, char *dest );
BOOLEAN matchString( const char *src, const char *dest, const BOOLEAN hasWildcards );
