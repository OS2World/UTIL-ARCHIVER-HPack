/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					 	Header File for Fast I/O Routines					*
*							FASTIO.H  Updated 08/12/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1990, 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

#ifndef _FASTIO_DEFINED

#define _FASTIO_DEFINED

#if defined( __ATARI__ )
  #include "io\hpackio.h"
#elif defined( __MAC__ )
  #include "hpackio.h"
#else
  #include "io/hpackio.h"
#endif /* System-specific include directives */

/* The size of the I/O buffers */

#if defined( __MSDOS16__ ) || defined( __IIGS__ )
  #define _BUFSIZE	8192
#else
  #define _BUFSIZE	16384
#endif /* __MSDOS16__ || __IIGS__ */

/* The size of the directory info buffer */

#define DIRBUFSIZE	( _BUFSIZE / 2 )

/* The EOF value */

#define FEOF		-1

/* Symbolic defines for the checksumming of input data */

#define RESET_CHECKSUM		TRUE
#define NO_RESET_CHECKSUM	FALSE

/* The format of the message to display while waiting for the luser to
   continue when handling a multipart archive */

#define WAIT_PARTNO		0x01	/* Print archive part number */
#define WAIT_NEXTDISK	0x02	/* Ask for next disk */
#define WAIT_PREVDISK	0x04	/* Ask for previous disk */
#define WAIT_LASTPART	0x08	/* Ask for last disk */

/* The output intercept type:  Normally data to be output is just written to
   a FD, however it can be intercepted in the vwrite() routine and
   redirected/reformatted/etc.  Possible types of redirection are no output,
   raw data output (default) and formatted text output */

typedef enum { OUT_NONE, OUT_DATA, OUT_FMT_TEXT } OUTINTERCEPT_TYPE;

/* The write type:  Standard mode, atomic write, safe write with trailer
   padding for multipart archives */

typedef enum { STD_WRITE, ATOMIC_WRITE, SAFE_WRITE } WRITE_TYPE;

/* Some global vars declared in FASTIO.C */

extern FD _inFD, _outFD;				/* I/O file descriptors */
extern BYTE *_inBuffer, *_outBuffer;	/* The I/O buffers */
extern int _inByteCount, _outByteCount;	/* Current position in buffer */
extern int _inBytesRead;				/* Actual no.bytes read */

/* Macros used to associate the input/output streams with FD's */

#define setOutputFD(theFD)	_outFD = theFD
#define setInputFD(theFD)	_inFD = theFD
#define getOutputFD()		_outFD
#define getInputFD()		_inFD

/* Vars used to handle the general-purpose buffer */

extern BYTE *mrglBuffer, *dirBuffer;
extern int mrglBufCount, dirBufCount;

/* Flag used to indicate status of the atomic write necessary at the end of
   a multipart archive */

extern BOOLEAN atomicWriteOK;

/* Prototypes for the fast I/O functions themselves */

int initFastIO( void );
void endFastIO( void );
void resetFastIn( void );
void resetFastOut( void );
void saveInputState( void );
void restoreInputState( void );
void saveOutputState( void );
void restoreOutputState( void );
inline void initTranslationSystem( const BYTE lineEndChar );
void checksumBegin( const BOOLEAN resetChecksum );
void checksumEnd( void );
void checksumDirBegin( const BOOLEAN resetChecksum );
void checksumDirEnd( void );
void checksumSetInput( const long dataLength, const BOOLEAN resetChecksum );
int cryptBegin( const BOOLEAN isMainKey );
void cryptEnd( void );
BOOLEAN cryptSetInput( long dataLength, int *cryptInfoLength );
void preemptCryptChecksum( const long length );
long getCurrPosition( void );
void setCurrPosition( const long position );
void setWriteType( const WRITE_TYPE writeType );
int vread( BYTE *buffer, int count );
long vlseek( const long offset, const int whence );
long vtell( void );
void skipSeek( LONG skipDist );
void skipToData( void );
void writeBuffer( const int bufSize );
void writeMrglBuffer( const int bufSize );
void writeDirBuffer( const int bufSize );
void flushBuffer( void );
void flushMrglBuffer( void );
void flushDirBuffer( void );
void setOutputIntercept( OUTINTERCEPT_TYPE interceptType );
void resetOutputIntercept( void );
void multipartWait( const WORD promptType, const int partNo );

/* Function versions of the original put/get byte/word/long macros.  These
   are used instead of macro's since the macros tended to produce a huge
   amount of code due to macro expansion.  Following common usage they are
   referred to by the macro names with a prepended 'f' */

void fputByte( const BYTE data );
void fputWord( const WORD data );
void fputLong( const LONG data );
int fgetByte( void );
WORD fgetWord( void );
LONG fgetLong( void );

/* The corresponding routines for directory data.  We don't need to give a
   file descriptor since we always use the same directory file */

void fputDirByte( const BYTE data );
void fputDirWord( const WORD data );
void fputDirLong( const LONG data );

/* The memory equivalents for the above functions - get/put a WORD or LONG
   from/to a section of memory */

WORD mgetWord( BYTE *memPtr );
LONG mgetLong( BYTE *memPtr );
void mputWord( BYTE *memPtr, const WORD data );
void mputLong( BYTE *memPtr, const LONG data );

/* The following works because we only ever do a read when we have no more
   chars available.  Thus when we read the last char in the buffer we don't
   read in more until we need the next char after that, so that _inByteCount
   is never set to 0 which would cause problems */

#define ungetByte()		_inByteCount--

/* Macro to force a read on the next fgetByte() */

#define forceRead()		_inByteCount = _inBytesRead = _BUFSIZE

#endif /* !_FASTIO_DEFINED */
