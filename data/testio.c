#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "system.h"
#include "io/hpackio.h"

/* Any system-specific functions we may need */

void initMacInfo( void );
void clearLocks( void );

/* External vars expected by some routines */

int screenHeight, screenWidth;
int dateFormat = 0;

/* Test vars used in the program: Two filenames, a SLASH-delimited path, and
   input/output data for the read/write tests.  Note that the R/W test data
   contains \n, \r, and \n\r to test for EOL translation by the OS */

char *fileName = "test", *fileName2 = "test1";
BYTE outData[] = "abcdefghijklmnopqrstuvwxyz\nxxx\rxxx\n\rxxx";
BYTE inData[ 100 ];
#if defined( __MSDOS__ )
  char filePath[ 100 ] = "d:";
  char fdFileName[ 100 ] = "a:test";
#elif defined( __AMIGA__ )
/*  char filePath[ 100 ] = "Nutta:Applications/Lattice"; */
  char filePath[ 100 ] = "DH0:Dice";
  char fdFileName[ 100 ] = "fd0:test";
#elif defined( __ARC__ )
  char filePath[ 100 ] = "$.HPACK";
  char fdFileName[ 100 ] = "adfs:0.$.test";
#elif defined( __ATARI__ )
  char filePath[ 100 ] = "???????";		/*!!!!!!!!!!*/
  char fdFileName[ 100 ] = "a:test";
#elif defined( __MAC__ )
  char filePath[ 100 ] = "Macintosh HD:\x01Grads\x01Peter";
  char fdFileName[ 100 ] = "Unlabeled:\x01test";
#endif /* Various OS-dependant pathnames */
char fileNamePath[ 100 ];

/* The following function is normally in error.c */

void fileError( void )
	{
	puts( "fileError() called - miscellaneous file error" );
	}

/* The test code for the <os_name>.c functions */

void main( void )
	{
	FILEINFO fileInfo;
	FD theFD;
	int count;
	long pos;
	BYTE *memPtr;

	/* Set up OS-specific info */
#ifdef __MAC__
	initMacInfo();
#endif /* __MAC__ */

	/* Test hcreat() */
	puts( "Performing hcreat()" );
	if( ( theFD = hcreat( fileName, CREAT_ATTR ) ) == IO_ERROR )
		{
		puts( "hcreat() failed" );
		return;
		}
	else
		hclose( theFD );

	/* Test hopen() */
	puts( "Performing hopen()" );
	if( ( theFD = hopen( fileName, O_RDWR ) ) == IO_ERROR )
		{
		puts( "hopen() failed" );
		return;
		}

	/* Test hwrite() */
	puts( "Performing hwrite()" );
	if( ( count = hwrite( theFD, outData, 26 ) ) != 26 )
		{
		hclose( theFD );
		printf( "hwrite() failed, wrote %d bytes\n", count );
		return;
		}

	/* Test hlseek() */
	puts( "Performing hlseek()" );
	if( ( pos = hlseek( theFD, 1L, SEEK_SET ) ) != 1L )
		{
		hclose( theFD );
		printf( "hlseek() failed for SEEK_SET, moved to pos %ld\n", pos );
		return;
		}
	if( ( pos = hlseek( theFD, 5L, SEEK_CUR ) ) != 6L )
		{
		hclose( theFD );
		printf( "hlseek() failed for SEEK_CUR, moved to pos %ld\n", pos );
		return;
		}
	if( ( pos = hlseek( theFD, -5L, SEEK_END ) ) != 21L )
		{
		hclose( theFD );
		printf( "hlseek() failed for SEEK_END, moved to pos %ld\n", pos );
		return;
		}
	hlseek( theFD, 0L, SEEK_SET );		/* Return to start of file */

	/* Test hread() */
	puts( "Performing hread()" );
	if( ( count = hread( theFD, inData, 26 ) ) != 26 )
		{
		hclose( theFD );
		printf( "hread() failed, read %d bytes\n", count );
		return;
		}
	if( memcmp( inData, outData, 26 ) )
		{
		hclose( theFD );
		puts( "Data read != data written" );
		return;
		}

	/* Test hclose() */
	puts( "Performing hclose()" );
	if( hclose( theFD ) == IO_ERROR )
		{
		puts( "hclose() failed" );
		return;
		}

	/* Test hrename() */
	puts( "Performing hrename()" );
	theFD = hcreat( fileName, CREAT_ATTR );
	hclose( theFD );
	if( hrename( fileName, fileName2 ) == IO_ERROR )
		{
		puts( "hrename() failed" );
		return;
		}
	printf( "Check that the file '%s' exists, then hit a key\n", fileName2 );
	getchar();

	/* Test hunlink() */
	puts( "Performing hunlink()" );
	if( hunlink( fileName2 ) == IO_ERROR )
		{
		puts( "hunlink() failed" );
		return;
		}
	printf( "Check that the file '%s' has been deleted, then hit a key\n", fileName2 );
	getchar();

	/* Test htruncate() */
	puts( "Performing htruncate()" );
	theFD = hcreat( fileName, CREAT_ATTR );
	hwrite( theFD, outData, 26 );
	hlseek( theFD, 10, SEEK_SET );
	if( htruncate( theFD ) == IO_ERROR )
		{
		hclose( theFD );
		puts( "htruncate() failed" );
		return;
		}
	hclose( theFD );
	printf( "Check that the file '%s' is 10 bytes long, then hit a key\n", fileName );
	getchar();
	hunlink( fileName );

	/* Test hmkdir() */
	puts( "Performing hmkdir()" );
	if( hmkdir( "TestDir", 0 ) == IO_ERROR )
		{
		puts( "hmkdir() failed" );
		return;
		}
	puts( "Check that the directory 'TestDir' exists, then hit a key" );
	getchar();

	/* Test pathname handling */
	strcpy( fileNamePath, filePath );
	strcat( fileNamePath, SLASH_STR );
	strcat( fileNamePath, "testfile" );
	printf( "Creating file %s with path component %s\n", fileNamePath, filePath );
	puts( "Make sure the path is valid" );
	if( ( theFD = hcreat( fileNamePath, CREAT_ATTR ) ) == IO_ERROR )
		{
		hclose( theFD );
		puts( "hcreat() with path failed" );
		return;
		}
	hclose( theFD );
	if( hunlink( fileNamePath ) == IO_ERROR )
		{
		puts( "hunlink() with path failed" );
		return;
		}

	/* Test findFirst()/findNext() on files only */
	printf( "\nFinding all files on path %s\n", filePath );
	strcat( filePath, SLASH_STR );
	strcat( filePath, MATCH_ALL );
	if( !findFirst( filePath, FILES, &fileInfo ) )
		puts( "No files" );
	else
		do
			{
			printf( "Filename %s, filesize %ld, filetime 0x%lX, file attr 0x%X\n", \
					fileInfo.fName, fileInfo.fSize, fileInfo.fTime, fileInfo.fAttr );
			}
		while( findNext( &fileInfo ) );
	findEnd( &fileInfo );

	/* Test findFirst()/findNext() on files and directories */
	printf( "\nFinding all files and directories on path %s\n", filePath );
	if( !findFirst( filePath, FILES_DIRS, &fileInfo ) )
		puts( "No files/dirs" );
	else
		do
			{
			if( isDirectory( fileInfo ) )
				printf( "Dirname %s, dirtime 0x%lX, dir attr 0x%X\n", \
						fileInfo.fName, fileInfo.fTime, fileInfo.fAttr );
			else
				printf( "Filename %s, filesize %ld, filetime 0x%lX, file attr 0x%X\n", \
						fileInfo.fName, fileInfo.fSize, fileInfo.fTime, fileInfo.fAttr );
			}
		while( findNext( &fileInfo ) );
	findEnd( &fileInfo );

#ifdef __AMIGA__
	clearLocks();
#endif /* __AMIGA__ */

	/* Test setFileTime() */
	printf( "Setting timestamp for file %s to the epoch + delta\n", fileName );
	theFD = hcreat( fileName, CREAT_ATTR );
	hclose( theFD );
	setFileTime( fileName, 1000L );
	printf( "Check that the file '%s' is has a timestamp just past the epoch, then hit a key\n", fileName );
	getchar();
	hunlink( fileName );

	puts( "\nDone" );

	/* Test brokenness of write() call */
	puts( "Testing write() functionality.  Make sure there is less than 10K of" );
	printf( "  free space on the destination disk for file %s,\n" );
	puts( "  then hit a key" );
	getchar();
	if( ( memPtr = hmalloc( 16384 ) ) == NULL )
		{
		puts( "Can't malloc I/O buffer" );
		return;
		}
	theFD = hcreat( fdFileName, CREAT_ATTR );
	count = hwrite( theFD, memPtr, 16384 );
	if( !count )
		puts( "This is a broken write() - no data written" );
	else
		printf( "Successfully wrote %d of 16384 bytes\n", count );
	hclose( theFD );
	puts( "Check the file size just to be sure, then hit a key" );
	getchar();
	hfree( memPtr );
	}
