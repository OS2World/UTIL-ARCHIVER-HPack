/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  ASN.1 Complex Types Header File					*
*							ASN1CPLX.H  Updated 20/03/93					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _ASN1CPLX_DEFINED

#define _ASN1CPLX_DEFINED

#include "filehdr.h"

/****************************************************************************
*																			*
*							ASN.1 Complex Types Routines					*
*																			*
****************************************************************************/

/* Routines for handling DirectoryEntry types */

void newDirectoryEntry( DIR_ENTRY *fileHeader );
void deleteDirectoryEntry( DIR_ENTRY *fileHeader );
int sizeofDirectoryEntry( DIR_ENTRY *fileHeader );
void writeDirectoryEntry( STREAM *stream, DIR_ENTRY *fileHeader );
void readDirectoryEntry( STREAM *stream, DIR_ENTRY *fileHeader );

/* Routines for handling FileProperty types */

void newFileProperty( FILEHEADER *fileHeader );
void deleteFileProperty( FILEHEADER *fileHeader );
int sizeofFileProperty( FILEHEADER *fileHeader );
void writeFileProperty( STREAM *stream, FILEHEADER *fileHeader );
int readFileProperty( STREAM *stream, FILEHEADER *fileHeader );

#endif /* _ASN1CPLX_DEFINED */
