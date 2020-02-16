/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						   		User Choice Header							*
*							CHOICE.H  Updated 23/05/90						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1989, 1990 Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* The variable to store the command - defined in FRONTEND.C */

extern char choice;

/* The commands HPACK accepts */

#define DELETE	'D'
#define DISPLAY	'P'
#define FRESHEN	'F'
#define ADD		'A'
#define REPLACE	'R'
#define UPDATE	'U'
#define TEST	'T'
#define EXTRACT	'X'
#define VIEW    'V'
