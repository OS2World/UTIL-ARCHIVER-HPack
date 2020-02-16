/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   Script File Interface Header						*
*							SCRIPT.H  Updated 20/07/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1991 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* The types of error we check for in script files */

typedef enum { READLINE_NO_ERROR, READLINE_ILLEGAL_CHAR_ERROR,
			   READLINE_LENGTH_ERROR, READLINE_END_DATA } READLINE_STATUS;

#define MAX_ERRORS	10		/* Max.no.errors before we give up */

/* Prototypes for functions in SCRIPT.C */

void addFilespec( char *fileSpec );
void freeFilespecs( void );
READLINE_STATUS readLine( char *buffer, int bufSize, int *lineLength, int *errPos );
void processListFile( const char *listFileName );
