/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  Error Handling Routines Header					*
*							ERROR.H  Updated 14/03/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#if defined( __ATARI__ )
  #include "io\hpackio.h"
#elif defined( __MAC__ )
  #include "hpackio.h"
#else
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* Some global vars */

extern char errorFileName[];
extern FD errorFD;
extern char dirFileName[];
extern FD dirFileFD;
extern char secFileName[];
extern FD secFileFD;
extern int secInfoLen;
extern long oldArcEnd;
extern void *oldHdrlistEnd;				/* Actually FILEHDRLIST * */
extern int oldDirEnd;
extern unsigned int oldDirNameEnd;
extern FD archiveFD;
#ifdef __MAC__
  #define RESOURCE_TMPNAME		"HPACK temporary file"
  extern FD resourceTmpFD;
#endif /* __MAC__ */

/* Prototypes for functions in ERROR.C */

void error( char *errMsg, ... );
void fileError( void );
void closeFiles( void );
