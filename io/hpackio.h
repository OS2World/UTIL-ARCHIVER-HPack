/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  HPACK I/O Routines Header File					*
*							HPACKIO.H  Updated 30/06/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _HPACKIO_DEFINED

#define _HPACKIO_DEFINED

/* The type of a FD */

#ifdef __MAC__
  typedef short			FD;
#else
  typedef int			FD;
#endif /* __MAC__ */

/* File I/O error return codes */

#define IO_ERROR		-1

/* File access mode */

#if !defined( __ATARI__ ) && !defined( __UNIX__ )
  #ifdef __MAC__
	#define O_RDONLY	1
	#define O_WRONLY	2
	#define O_RDWR		3
  #else
	#define O_RDONLY	0
	#define O_WRONLY	1
	#define O_RDWR		2
  #endif /* __MAC__ */
#endif /* !__UNIX__ */

#define O_MASK			0x07

/* Sharing mode - necessary when multiple users can access a file */

#if defined( __AMIGA__ ) || defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || \
	defined( __OS2__ )
  #define S_DENYRDWR	( 1 << 4 )
  #define S_DENYWR		( 2 << 4 )
  #define S_DENYRD		( 3 << 4 )
  #define S_DENYNONE	( 4 << 4 )
#else
  #define S_DENYRDWR	0
  #define S_DENYWR		0
  #define S_DENYRD		0
  #define S_DENYNONE	0
#endif /* __AMIGA__ || __MSDOS16__ || __MSDOS32__ || __OS2__ */

#define S_MASK			0xF0

/* Access information for the file - extra information which may improve
   handling under some OS's */

#ifdef __OS2__
  #define A_SEQ			0x100
  #define A_RAND		0x200
  #define A_RANDSEQ		0x300
#else
  #define A_SEQ			0
  #define A_RAND		0
  #define A_RANDSEQ		0
#endif /* __OS2__ */

/* Origin codes for hlseek() */

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/* Standard streams */

#define STDIN		0
#define STDOUT		1
#define STDERR		2
#define STDAUX		3
#define STDPRN		4

#if !defined( __AMIGA__ ) && !defined( __ARC__ ) && !defined( __ATARI__ ) && \
	!defined( __IIGS__ ) && !defined( __MAC__ ) && !defined( __MSDOS16__ ) && \
	!defined( __MSDOS32__ ) && !defined( __OS2__ )

#ifndef C370
  #include <fcntl.h>
#endif /* C370 */

/* Macros to turn the generic HPACKIO functions into their equivalent on
   the current system */

#define hopen	open			/* Open existing file */
#define hclose	close			/* Close file */
#define hread	read			/* Read data from a file */
#define hwrite	write			/* Write data to a file */
#define hrename	rename			/* Rename file */

/* HPACKIO functions which need special versions on some systems */

#ifdef AMDAHL
  #define hlseek(fd, position, whence) \
				( long ) lseek( ( fd ), ( longlong_t ) ( ( long ) ( position ) ), ( whence ) )
#else
  #define hlseek	lseek			/* Seek to position in file */
#endif /* AMDAHL */

int htruncate( const FD theFD );/* Truncate a file at current position */

#if defined( __UNIX__ ) || defined( __AMIGA__ )
  int hcreat( const char *filePath, const int attr );
  int hchmod( const char *filePath, const int attr );
#else
  #define hcreat	creat		/* Create a file */
  #define hchmod	chmod		/* Change file attributes */
#endif /* __UNIX__ || __AMIGA__ */

#if defined( __VMS__ )
  #define htell(theFD)	hlseek( theFD, 0L, SEEK_CUR )
  #define hunlink		delete
#elif defined( AMDAHL )
  #define htell(fd)		( long ) lseek( ( fd ), ( longlong_t ) 0, SEEK_CUR )
  #define hunlink		unlink	/* Delete file */
#elif defined( BSD386 )
  #define htell(theFD)	hlseek( theFD, 0L, SEEK_CUR )
  #define hunlink		unlink
#else
  #define htell			tell	/* Return current position in file */
  #define hunlink		unlink	/* Delete file */
#endif /* __VMS__ */

#if defined( __UNIX__ ) || defined( __VMS__ )
  int hmkdir( const char *dirName, const int attr );
#else
  #define hmkdir		mkdir	/* Create directory */
#endif /* __UNIX__ || __VMS__ */

#else

/* Prototypes for functions in HPACKIO library */

FD hcreat( const char *fileName, const int attr );
FD hopen( const char *fileName, const int mode );
int hclose( const FD theFD );
long hlseek( const FD theFD, const long position, const int whence );
long htell( const FD theFD );
int htruncate( const FD theFD );
int hchmod( const char *fileName, const int attr );
int hunlink( const char *fileName );
int hmkdir( const char *dirName, const int attr );
int hrename( const char *srcName, const char *destName );

#ifdef __ATARI__
  #define hread		read
  #define hwrite	write
#else
  int hread( const FD theFD, void *buffer, const unsigned int bufSize );
  int hwrite( const FD theFD, void *buffer, const unsigned int bufSize );
#endif /* __ATARI__ */

#endif /* !( __AMIGA__ || __ARC__ || __IIGS__ || __MAC__ || __MSDOS16__ || __OS2__ ) */

#endif /* !_HPACKIO_DEFINED */
