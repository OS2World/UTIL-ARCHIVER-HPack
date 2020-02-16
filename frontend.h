/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						Header File for the Main Archiver Code				*
*							FRONTEND.H  Updated 09/05/92					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1991 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* The extensions used for HPACK archives, and used to match HPACK archives */

extern char HPAK_EXT[];
extern char HPAK_MATCH_EXT[];

/* The distance in bytes to the next piece of data to handle */

extern long skipDist;

/* The name of the archive currently being processed */

extern char archiveFileName[];

/* Some general vars */

extern WORD commentType;	/* The type of the archive comment */
extern BOOLEAN archiveChanged;	/* Whether the archive has been changed */
extern BOOLEAN firstFile;	/* Whether this is first file we're processing */
extern char basePath[];		/* The output directory given by the -b option */
extern int basePathLen;		/* The length of the basePath */
extern int screenHeight, screenWidth;	/* The screen size */
extern char signerID[];		/* userID of archive signer */
extern char mainUserID[];	/* userID of main PKE destination */
extern char secUserID[];	/* userID of secondary PKE destination */

/* The structs to handle the filepath/filename info */

typedef struct FN {
				  struct FN *next;		/* The next node in the list */
				  char *fileName;		/* The actual fileName */
				  BOOLEAN hasWildcards;	/* Whether filename has wildcards */
				  } FILENAMEINFO;

typedef struct FP {
				  struct FP *next;		/* The next node in the list */
#ifdef __VMS__
				  char *node;			/* The node the path is on */
#endif /* __VMS__ */
				  char *device;			/* The device the path is on */
				  char *filePath;		/* The filePath */
				  FILENAMEINFO *fileNames;	/* List of files on this path */
				  BOOLEAN hasWildcards;	/* Whether filepath has wildcards */
				  } FILEPATHINFO;

/* The start of the list of fileSpecs */

extern FILEPATHINFO *filePathListStart;

/* Prototypes for functions in FRONTEND.C */

BOOLEAN addArchiveName( char *archiveName );
void freeArchiveNames( void );
void wipeFilePaths( void );
void handleArchive( void );
