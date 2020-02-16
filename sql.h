/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							 SQL Interface Header File						*
*							  SQL.H  Updated 12/05/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#ifndef _SQL_DEFINED

#define _SQL_DEFINED

/* Variables controlling the data fields which are displayed */

extern int selectOrder[];
extern int selectOrderIndex;

enum { SELECT_NAME, SELECT_DATE, SELECT_TIME, SELECT_SIZE, SELECT_LENGTH,
	   SELECT_SYSTEM, SELECT_RATIO, SELECT_END };

/* Variables controlling the way sorting of data is performed.  sortOrder
   is an array of consecutive sort options to use on data.  An entry with
   a value of SORT_REVERSE_BASE or above indicates that a descending sort
   order should be used */

#define SORT_REVERSE_BASE	100		/* Base value for "sort descending" */

extern int sortOrder[];
extern int sortOrderIndex;

enum { SORT_NAME, SORT_DATE, SORT_TIME, SORT_SIZE, SORT_LENGTH, SORT_OS, SORT_END };

/* Prototypes for functions in SQL.C */

void initSQL( void );
void initConfigLine( void );
void setDefaultSql( void );
int processSqlCommand( const char *sqlCommand );
int processConfigLine( const char *configLine, const int lineNo );

#endif /* !_SQL_DEFINED */
