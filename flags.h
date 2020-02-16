/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							  User Flags Header File						*
*							 FLAGS.H  Updated 09/06/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _FLAGS_DEFINED

#define _FLAGS_DEFINED

/* The word used to hold the flag values */

extern WORD flags;
extern WORD dirFlags;
extern WORD cryptFlags;
extern WORD viewFlags;
extern WORD xlateFlags;
extern WORD specialFlags;
extern WORD overwriteFlags;
extern WORD multipartFlags;
extern WORD sysSpecFlags;

/* The values in the flags word */

#define OVERWRITE_SRC	0x0001	/* Delete archive if it already exists */
#define TOUCH_FILES		0x0002	/* Touch files on extraction */
#define RECURSE_SUBDIR	0x0004	/* Recurse through all subdirectories */
#define CRYPT			0x0008  /* Encrypt the data */
#define MOVE_FILES		0x0010	/* Move files to/from archive */
#define STORE_ATTR		0x0020	/* Store file attributes */
#define STORE_ONLY		0x0040	/* Store data only, don't compress */
#define XLATE_OUTPUT	0x0080	/* Apply translation to decompressor output */
#define STEALTH_MODE	0x0100	/* Run in stealth mode */
#define INTERACTIVE		0x0200	/* Confirm each action taken */
#define ERROR_RECOVER	0x0400	/* Write error recovery info */
#define ARCH_COMMENT	0x0800	/* Args are archive comments */
#define MULTIPART_ARCH	0x1000	/* Multi-part archive */
#define BLOCK_MODE		0x2000	/* Compress files in block mode */

/* The values in the overwriteFlags word */

#define OVERWRITE_ALL	0x0001	/* Overwrite existing on extraction */
#define OVERWRITE_NONE	0x0002	/* Don't overwrite existing on extraction */
#define OVERWRITE_SMART	0x0004  /* Rename source to avoid overwrite */
#define OVERWRITE_PROMPT	0x0008	/* Prompt for new filename if collision */

/* The values in the viewFlags word */

#define VIEW_FILES		0x0001	/* View only the files in an archive */
#define VIEW_DIRS		0x0002	/* View only the dirs in an archive */
#define VIEW_SORTED		0x0004	/* Sort files in directories */

/* The values in the dirFlags word */

#define DIR_MKDIR		0x0001	/* Make directory */
#define DIR_RMDIR		0x0002	/* Remove directory */
#define DIR_MVDIR		0x0004	/* Move directory */
#define DIR_ALLPATHS	0x0008	/* Add all paths (even empty ones) */
#define DIR_NOCREATE	0x0010	/* No create dir.if it doesn't already exist */
#define DIR_MULTIDIR	0x0020	/* Full paths for ??DIR, not just single dirs */
#define DIR_CONTAIN		0x0040	/* Only create containing dir, not whole path */
#define DIR_FLATTEN		0x0080	/* Flatten directory structure */

/* The values in the xlateFlags word */

#define XLATE_EOL		0x0001	/* Translate EOL character */
#define XLATE_EBCDIC	0x0002	/* Translate EBCDIC -> ASCII */
#define XLATE_PRIME		0x0004	/* Translate Prime ASCII -> std.ASCII */
#define XLATE_SMART		0x0008	/* Smart xlate depending on OS */

/* The values in the cryptFlags word */

#define CRYPT_PKE		0x0001	/* PKE-en/decrypt data */
#define CRYPT_CKE		0x0002	/* CKE-en/decrypt data */
#define CRYPT_PKE_ALL	0x0004	/* PKE-en/decrypt entire archive */
#define CRYPT_CKE_ALL	0x0008	/* CKE-en/decrypt entire archive */
#define CRYPT_SEC		0x0010	/* Encrypt data and directory seperately */
#define CRYPT_SIGN		0x0020	/* Sign data */
#define CRYPT_SIGN_ALL	0x0040	/* Sign entire archive */

/* The values in the multipartFlags word */

#define MULTIPART_READ	0x0001	/* Reading a multipart archive */
#define MULTIPART_WRITE	0x0002	/* Writing a multipart archive */

/* The values in the sysSpecFlags word */

#define SYSPEC_NOEXT		0x0001	/* Don't force extension for archive */
#define SYSPEC_GOPHER		0x0002	/* Gopher mode */
#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  #define SYSPEC_VOLUME		0x0004	/* Store volume label + serial number */
  #define SYSPEC_CHECKSAFE	0x0008	/* Check for device driver as filename */
#elif defined( __AMIGA__ )
  #define SYSPEC_FORCELOWER	0x0004	/* Force file/dir names to lower case */
#elif defined( __ARC__ )
  #define SYSPEC_FORCELOWER	0x0004	/* Force file/dir names to lower case */
  #define SYSPEC_INVERTDIR	0x0008	/* Invert extensions into directories */
#elif defined( __MAC__ )
  #define SYSPEC_FORCELOWER	0x0004	/* Force file/dir names to lower case */
#elif defined( __OS2__ )
  #define SYSPEC_FORCELOWER	0x0004	/* Force file/dir names to lower case */
#elif defined( __UNIX__ )
  #define SYSPEC_FORCELOWER	0x0004	/* Force file/dir names to lower case */
  #define SYSPEC_NOUMASK	0x0008	/* Ignore umask for file/dir attributes */
  #define SYSPEC_DEVICE		0x0010	/* Treat archive name as device */
#elif defined( __VMS__ )
  #define SYSPEC_RSX11		0x0002	/* Translate filenames to RSX-11 format */
#endif /* Various system-specific options */

#endif /* !_FLAGS_DEFINED */
