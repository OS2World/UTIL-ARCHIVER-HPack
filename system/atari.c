/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  Atari-Specific Routines						*
*							 ATARI.C  Updated 21/02/93						*
*																			*
*							Written by Martin Braschler						*
*				Based on work by Murray Moffatt and Peter Gutmann			*
*																			*
*	This program is protected by copyright. If you want to know what that	*
* means to you, read one of the other headers of the source distribution of *
*						HPACK. Greetings to all Tolkien fans!				*
*																			*
*		This file copyright 1993 Peter C Gutmann and Martin C Braschler		*
*																			*
****************************************************************************/

/* This port was done using Pure C 1.0 */

#ifndef __PUREC__
  #error "Port only works on Pure C."
#endif /* __PUREC__ */

#include <tos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ext.h>
#include <linea.h>
#include "defs.h"
#include "frontend.h"
#include "system.h"
#include "hpacklib.h"
#include "io\hpackio.h"
#include "io\fastio.h"

/*
================================================
= Prototypes for functions declared in ATARI.C =
================================================
*/

/* --- time handling --- */

void timeHPackToGem( const LONG time, struct ftime *tim );
long timeGemToHPack( long gemTime );
void setFileTime( const char *fileName, const LONG time );

/* --- system functions --- */

BOOLEAN findFirst( const char *filePath, const ATTR fileAttr, FILEINFO *fileInfo );
BOOLEAN findNext( FILEINFO *fileInfo );
void getScreenSize( void );
int getCountry( void );
void waitForKey( void );
void isItA_030( void );

/* --- hpackio functions --- */

static void translateName( const char *srcName, char *destName );
FD hcreat( const char *fileName, const int attr );
FD hopen( const char *fileName, const int mode );
int hclose( const FD theFD );
long hlseek( const FD theFD, const long position, const int whence );
long htell( const FD theFD );
#define	hread	read
#define	hwrite	write
#define htruncate	hwrite( theFD, NULL, 0 ) /* --DOESN'T-- work */
int chmod( const char *fileName, const int attr );
int hunlink( const char *fileName );
int hmkdir( const char *dirName, const int attr );
int hrename( const char *srcName, const char *destName );

/*
========================
= Atari implementation =
========================
*/

/*
=============================================================================
=								SYSTEM Functions							=
=============================================================================
*/

/* Set file time */

#define SETDATE		1

void timeHPackToGem( const LONG time, struct ftime *tim )
	{
	struct tm *tmtime;

	tmtime = localtime( ( time_t * ) &time );

	tim->ft_tsec   = tmtime->tm_sec;
	tim->ft_min    = tmtime->tm_min;
	tim->ft_hour   = tmtime->tm_hour;
	tim->ft_day    = tmtime->tm_mday;
	tim->ft_month  = tmtime->tm_mon + 1;
	tim->ft_year   = tmtime->tm_year - 80;
}

long timeGemToHPack( long gemTime )
	{
	struct tm *normTime;

	normTime = ftimtotm( (struct ftime *) &gemTime );
	return( (long) mktime( normTime ) );
}

void setFileTime( const char *fileName, const LONG time )
	{
	FD theFD;
	struct ftime tim;

	timeHPackToGem( time, &tim );

	if( ( theFD = hopen( fileName, O_RDWR ) ) != ERROR )
		{
		setftime( theFD, &tim );
		hclose( theFD );
	}
}

/* Find the first/next file in a directory. */

BOOLEAN findFirst( const char *filePath, const ATTR fileAttr, FILEINFO *fileInfo )
	{
	char translatedPathName[ MAX_PATH + 1 ];
	BOOLEAN success;

	translateName( filePath, translatedPathName );

	success = ( ( findfirst( translatedPathName, ( struct ffblk * ) fileInfo, fileAttr ) != IO_ERROR ) ? TRUE : FALSE );

	fileInfo->fTime = timeGemToHPack( fileInfo->fTime );

	return( success );
}

BOOLEAN findNext( FILEINFO *fileInfo )
	{
	BOOLEAN success;

	success = ( ( findnext( ( struct ffblk * ) fileInfo ) != IO_ERROR ) ? TRUE : FALSE );

	fileInfo->fTime = timeGemToHPack( fileInfo->fTime );

	return( success );
}

void getScreenSize( void )
	{
	/* Get screen size - done with Line A (should not use GEM!) */
	linea_init();

	screenWidth  = Vdiesc->v_cel_mx;
	screenHeight = Vdiesc->v_cel_my;
	}

int getCountry( void )
	{
	SYSHDR *sysbase;
	int country;
	long stack;

	stack = Super( 0 );

	sysbase = *(SYSHDR **) 0x4F2L;
	country = sysbase->os_palmode;

	Super( (void *) stack );

	switch ( country )
		{
		case 3:		/* Germany */
			return( 1 );	/* Day-Month-Year */
		case 5:		/* France */
			return( 1 );
		case 7:		/* Great Britain */
			return( 1 );
		case 17:	/* The Netherlands */
			return( 1 );
		default:	/* All others, namely the USA */
			return( 0 );	/* Month-Day-Year */
	}
}

void waitForKey( void )
	{
	getch();
}

void isItA_030( void )
	{
	typedef struct _cookies
		{
		long ident;
		long value;
	} COOKIE;

	COOKIE *cookie;
	BOOLEAN itIsA_030;
	long stack;

	itIsA_030 = FALSE;

	stack = Super( 0 );

	cookie = *(COOKIE **) 0x5a0l;

	if ( cookie )
		{
		while( cookie->ident != 0 )
			{
			if( cookie->ident == '_CPU' )	/* Cookie _CPU */
				if( cookie->value >= 30 )
					itIsA_030 = TRUE;
			cookie++;
		}
	}

#ifdef __030__
	if( itIsA_030 == FALSE )
		{
		hputs( "Error: This is no 68030 or higher" );
		waitForKey();
		exit( 0 );
	}
#else
	if( itIsA_030 == TRUE )
		{
		hputs( "Warning: This is a 68030 or higher. Use HPACK/030 for more speed." );
		waitForKey();
	}
#endif

	Super( (void *) stack );
}

/* Translate a name from canonical to GemDOS format */

static void translateName( const char *srcName, char *destName ) /* mB */
	{
	while( *srcName )
		/* Check for SLASH delimiter */
		if( *srcName == SLASH )
			{
			/* Copy as GemDOS slash */
			*destName++ = '\\';
			srcName++;
			}
		else
			/* Copy literal */
			*destName++ = *srcName++;
	*destName = '\000'; /* mB */
	}

/*
=============================================================================
=								HPACKIO Functions							=
=============================================================================
*/

/* Create, open a file */

/* defined here are: -mB
   hcreat, hopen, hclose, hlseek, htell, hread, hwrite,
   htruncate, hchmod, hunlink, hmkdir, hrename */

FD hcreat( const char *fileName, const int attr )
	{
	/* Attr is never used -mB */

	char translatedFileName[ MAX_PATH + 1 ];

	translateName( fileName, translatedFileName );
	return( creat( translatedFileName ) );
	}

FD hopen( const char *fileName, const int mode )
	{
	char translatedFileName[ MAX_PATH + 1 ];

	translateName( fileName, translatedFileName );
	return( open( translatedFileName, mode ) );
	}

/* Close a file */

int hclose( const FD theFD )
	{
	return( close( theFD ) );
	}

/* Seek to a position in a file, return the current position in a file */

long hlseek( const FD theFD, const long position, const int whence )
	{
	return( lseek( theFD, position, whence ) ); /* mB */
	}

long htell( const FD theFD )
	{
	return( hlseek( theFD, 0L, SEEK_CUR ) ); /* mB */
	}

/* Read/write data to/from a file */

#define	hread	read	/* mB */
#define	hwrite	write	/* mB */
#define htruncate	hwrite( theFD, NULL, 0 ) /* --DOESN'T-- work */

/* Truncate a file at the current position */
#if 0
int htruncate( const FD theFD, const FILEINFO *fileInfoPtr )
	{
	/* Life's a piece of shit, when you look at it (Monty Python) */

	FD newFD;
	BYTE *tmpmem;
	int i;
	long pos;
	char *name;

	if( (tmpmem = malloc( 32000 )) != 0 )
		{
		pos = lseek( theFD, 0, SEEK_CUR );	/* Where are we? */
		lseek( theFD, 0, SEEK_SET );	/* Seek start of data */
	
		name = tmpnam( NULL );
		newFD = hcreat( name, 0 );

		if ( newFD != IO_ERROR )
			{
			for( i=0; i < (pos / 32000); i++ )
				{
				hread( theFD, tmpmem, 32000L );
				hwrite( newFD, tmpmem, 32000L );
			}
			hread( theFD, tmpmem, (pos % 32000 ) );
			hwrite( newFD, tmpmem, (pos % 32000 ) );
		}
		else
			{
			free( tmpmem );
			return( -1 );
		}
	}
	else
		return( -1 );
	
	hclose( newFD );
	
	free( tmpmem );
	return( 0 );
}
#endif

/* Set a file's attributes */


int hchmod( const char *fileName, const int attr )
	{
	char translatedFileName[ MAX_PATH + 1 ];

	translateName( fileName, translatedFileName );
	return( Fattrib( translatedFileName, 1, attr ) ); /* mB */
	}


/* Delete a file */

int hunlink( const char *fileName )
	{
	char translatedFileName[ MAX_PATH + 1 ];

	translateName( fileName, translatedFileName );
	return( remove( translatedFileName ) ); /* mB */
	}

/* Create a directory */

int hmkdir( const char *dirName, const int attr )
	{
	/* Attributes are ignored -mB */

	char translatedDirName[ MAX_PATH + 1 ];

	translateName( dirName, translatedDirName );
	return( Dcreate( translatedDirName ) ); /* mB */
	}

/* Rename a file */

int hrename( const char *srcName, const char *destName )
	{
	char translatedSrcName[ MAX_PATH + 1 ], translatedDestName[ MAX_PATH + 1 ];

	translateName( srcName, translatedSrcName );
	translateName( destName, translatedDestName );
	return( rename( translatedSrcName, translatedDestName ) );
	}


