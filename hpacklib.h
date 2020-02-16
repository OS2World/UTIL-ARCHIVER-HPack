/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						HPACK Library Code Interface Header					*
*							HPACKLIB.H  Updated 22/03/92					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _HPACKLIB_DEFINED

#define _HPACKLIB_DEFINED

#if !defined( __MSDOS16__ ) || defined( GUI )

/* Macros to turn the generic HPACKLIB functions into their equivalent on
   the current system */

#include <stdio.h>					/* Prototypes for generic functions */
#if defined( __UNIX__ ) && !( defined( BSD386 ) || defined( CONVEX ) || \
							  defined( NEXT ) || defined( ULTRIX_OLD ) )
  #include <malloc.h>				/* Needed for mem.functions on some systems */
#endif /* __UNIX__ && !( BSD386 || CONVEX || NEXT || ULTRIX_OLD ) */
#if defined( AIX386 ) || ( defined( __AMIGA__ ) && !defined( LATTICE ) ) || \
	defined( __ARC__ ) || defined( __ATARI__ ) || defined( __MSDOS16__ ) || \
	defined( __MSDOS32__ )
  #include <stdlib.h>				/* Needed for mem.functions */
#endif /* AIX386 || ( __AMIGA__ && !LATTICE ) || __ARC__ || __ATARI__ || __MSDOS16__ || __MSDOS32__ */
#if defined( __ATARI__ ) || ( defined( __OS2__ ) && !defined( __GCC__ ) )
  #include <conio.h>				/* For console I/O */
#endif /* __ATARI__ || ( __OS2__ && !__GCC__ ) */
#if defined( CONVEX )
  #include <sys/types.h>			/* Verschiedene komische Typen */
#endif /* CONVEX */
#include "flags.h"					/* Flags for stealth-mode check */

#ifndef GUI
  #define hprintf		printf		/* Output formatted string/arg-list */
  #ifndef __ARC__
	#define hputchar	putchar		/* Output single char */
  #endif /* __ARC__ */
  #define hputs			puts		/* Output string */
  #define hgets			gets		/* Input string */

  #define hprintfs	if( !( flags & STEALTH_MODE ) ) \
						printf		/* printf() with stealth-mode check */
  #ifndef __ARC__
	#define hputchars	if( !( flags & STEALTH_MODE ) ) \
							putchar	/* putchar() with stealth-mode check */
  #endif /* __ARC__ */
  #if defined( __AMIGA__ ) || defined( __OS2__ ) || defined( __UNIX__ )
	#define hflush	fflush			/* Overcome buffering on stdout */
  #else
	#define hflush(x)				/* Ba-woooosshhhh!! */
  #endif /* __AMIGA__ || __OS2__ || __UNIX__ */
  #if defined( __AMIGA__ ) || defined( __ARC__ ) || defined( __MAC__ ) || \
	  defined( __OS2__ ) || defined( __UNIX__ )
	int hgetch( void );				/* Get char, no echo */
  #else
	#define hgetch	getch			/* Get char, no echo */
  #endif /* __AMIGA__ || __ARC__ || __MAC__ || __UNIX__ */
  #ifdef __ARC__
	void hputchar( const int ch );	/* Output single char */
	#define hputchars	if( !( flags & STEALTH_MODE ) ) \
							hputchar	/* putchar() with stealth-mode check */
  #endif /* __ARC__ */
#endif /* GUI */

#if defined( __OS2__ ) || ( defined( __MSDOS16__ ) && defined( GUI ) ) || \
	defined( __MAC__ ) || ( defined( __UNIX__ ) && defined( POSIX ) )
  void *hmalloc( const unsigned long size );	/* Allocate block of memory */
  void *hrealloc( void *buffer, const unsigned long size );	/* Re-allocate block of memory */
  void hfree( void *buffer );		/* Free block of memory */
#else
  #define hmalloc	malloc			/* Allocate block of memory */
  #define hrealloc	realloc			/* Re-allocate block of memory */
  #define hfree		free			/* Free block of memory */
#endif /* __OS2__ */

#if defined( __MSDOS16__ ) && defined( GUI )
  void *hmallocSeg( const WORD size );	/* Segment-aligned malloc() */
#endif /* __MSDOS16__ && GUI */

#define sfree(buf,size)		if( flags & CRYPT ) \
								memset( buf, 0, size ); \
							hfree( buf );
									/* Safe hfree() which destroys sensitive
									   data in memory */
#else

/* Prototypes for functions in the HPACK libraries */

void hprintf( const char *format, ... );
void hprintfs( const char *format, ... );
void hvprintf( void );
void hputchar( const char ch );
void hputchars( const char ch );
void hputs( const char *string );
int hgetch( void );

#define hflush(x)				/* No need for hflush() under a real OS */

/* The hfree() pseudo-function:  Unlike free(), hfree() is a null function
   since all cases where hmalloc() is called keep the memory allocated until
   HPACK terminates, when a block free is done.  The use of the DYNA memory
   manager does away with the need for a hfree() function.  In addition there
   is no need for an sfree() since this is taken care of by the startup code */

#define hfree(ptr)
#define sfree(x,y)

/* hmalloc() is called getMem() externally to reduce any confusion that it's
   a malloc() clone */

void *getMem( const int size );

#define hmalloc(size)	getMem( size )

/* hmallocSeg() is a MSDOS-specific hmalloc() which ensure the data is
   segment aligned.  This greatly enhances the simplicity of array indexing
   since a segment register can point at the array base and an index register
   can be used to index into it */

void *getMemSeg( const int size );

#define hmallocSeg(size)	getMemSeg( size )

/* initMem() and endMem() start up and shut down the memory manager */

void initMem( void );
void endMem( void );

#endif /* !__MSDOS16__ || GUI */

#ifdef GUI

/* Prototypes and enumerated types for idiot boxes */

typedef enum { ALERT_FILE_IN_ARCH, ALERT_TRUNCATED_PADDING,
			   ALERT_DATA_ENCRYPTED, ALERT_DATA_CORRUPTED,
			   ALERT_PATH_TOO_LONG, ALERT_CANNOT_PROCESS_ENCR_INFO,
			   ALERT_CANNOT_OPEN_DATAFILE, ALERT_CANNOT_OPEN_SCRIPTFILE,
			   ALERT_UNKNOWN_ARCH_METHOD, ALERT_NO_OVERWRITE_EXISTING,
			   ALERT_FILE_IS_DEVICEDRVR, ALERT_ARCHIVE_UPTODATE,
			   ALERT_ERROR_DURING_ERROR_REC, ALERT_UNKNOWN_SCRIPT_COMMAND,
			   ALERT_PATH_TOO_LONG_LINE, ALERT_BAD_CHAR_IN_FILENAME_LINE,
			   ALERT_CANT_FIND_PUBLIC_KEY, ALERT_SEC_INFO_CORRUPTED,
			   ALERT_SECRET_KEY_COMPROMISED, ALERT_ARCH_SECTION_TOO_SHORT,
			   ALERT_DIRNAME_CONFLICTS_WITH_FILE, ALERT_PASSWORD_INCORRECT,
			   ALERT_FILES_CORRUPTED
			 } ALERT_TYPE;

void alert( ALERT_TYPE alertType, const char *message );

#endif /* GUI */

#endif /* !_HPACKLIB_DEFINED */
