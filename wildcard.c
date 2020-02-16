/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						 Wildcard String Matching Routines					*
*							WILDCARD.C  Updated 20/06/90					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1989, 1990 Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

#include <ctype.h>
#include <string.h>
#ifdef CONVEX
  #include <sys/types.h>
#endif /* CONVEX */
#include "defs.h"
#include "system.h"
#include "wildcard.h"

/* The types we can match.  These are:

	END					- End of string
	MATCHEND			- Match all following chars
	MATCHTO <end>		- Match all chars till <end>
	MATCHONE			- Match any one char
	RANGE <from> <to>	- Match all chars in range <from>...<to>
	NOTRANGE <from> <to>- Match all chars but those in range <from>...<to>
	<char>				- Match this char */

enum { END, MATCHEND, MATCHTO, MATCHONE, RANGE, NOTRANGE, ENDRANGE };

/* The escape character which is used to override standard wildcard
   characters.  Usually this is '\' but MSDOS and OS/2 need to use '#' since
   '\' is used in pathnames */

#if defined( __MSDOS16__ ) || defined( __MSDOS32__ ) || defined( __OS2__ )
  #define ESC	'#'
#else
  #define ESC	'\\'
#endif /* __MSDOS16__ || __MSDOS32__ || __OS2__ */

/* A utility function to determine whether a string segment contains any
   wildcard characters */

BOOLEAN hasWildcards( char *string, int count )
	{
	char ch;

	while( count-- )
		if( ( ch = *string++ ) == '*' || ch == '?' || ch == '[' || ch == ESC )
			return( TRUE );

	/* No wildcards found in string */
	return( FALSE );
	}

BOOLEAN strHasWildcards( char *string )
	{
	return( hasWildcards( string, strlen( string ) ) );
	}

/****************************************************************************
*																			*
* 						String "Compiling" Function							*
*																			*
****************************************************************************/

/* Compile a string into a form acceptable by the wildcard-matching FSM.
   The form is pretty simple.  The only part which may require a bit of
   explanation is the mechanism for matching ranges.  This is given by the
   RE { "^" | "!" }? { letter { "-" letter }? }+ */

int compileString( const char *src, char *dest )
	{
	int srcIndex = 0, destIndex = 0;
	BOOLEAN inRange;
	char ch;

#pragma warn -pia
	while( ( ch = src[ srcIndex++ ] ) && destIndex < MATCH_DEST_LEN )
#pragma warn +pia
		switch( ch )
			{
			case '*':
#ifdef AIX
				/* Fix a bug in the RS6000 cc optimizer where it gets a bit
				   enthusiastic with reusing a CR which was set for the test
				   of src[ srcIndex++ ] and doesn't notice that srcIndex has
				   changed.  The following forces a reevaluation of
				   src[ srcIndex ] */
				srcIndex--;
				srcIndex++;
#endif /* AIX */
				if( src[ srcIndex ] )
					dest[ destIndex++ ] = MATCHTO;
				else
					dest[ destIndex++ ] = MATCHEND;

				/* Stomp repeated '*'s */
				while( src[ srcIndex ] == '*' )
					srcIndex++;

				break;

			case '?':
				dest[ destIndex++ ] = MATCHONE;
				break;

			case '[':
				if( src[ srcIndex ] == '^' || src[ srcIndex ] == '!' )
					{
					dest[ destIndex++ ] = NOTRANGE;
					srcIndex++;
					}
				else
					dest[ destIndex++ ] = RANGE;
				if( src[ srcIndex ] == ']' || src[ srcIndex ] == '-' )
					return( WILDCARD_FORMAT_ERROR );

				inRange = FALSE;
				while( ( ( ch = src[ srcIndex++ ] ) != ']' ) &&
										( destIndex < MATCH_DEST_LEN - 1 ) )
							/* destIndex < MATCH_DEST_LEN - 1 allows room for
							   the ENDRANGE and END tokens */
					switch( ch )
						{
						case END:
							return( WILDCARD_FORMAT_ERROR );

						case '-':
							if( inRange )
								return( WILDCARD_FORMAT_ERROR );
							inRange = TRUE;
							dest[ destIndex ] = dest[ destIndex - 1 ];
							dest[ destIndex - 1 ] = RANGE;
							destIndex++;
							break;

						case ESC:
#pragma warn -pia
							if( !( ch = src[ srcIndex++ ] ) )
#pragma warn +pia
								return( WILDCARD_FORMAT_ERROR );
							/* Drop through */

						default:
							dest[ destIndex++ ] = ch;
							inRange = FALSE;
							break;
						}
				if( inRange )
					return( WILDCARD_FORMAT_ERROR );
				dest[ destIndex++ ] = ENDRANGE;
				break;

			case ESC:
#pragma warn -pia
				if( !( ch = src[ srcIndex++ ] ) )
#pragma warn +pia
					return( WILDCARD_FORMAT_ERROR );
				/* Drop through */

			default:
				dest[ destIndex++ ] = ch;
			}
	dest[ destIndex++ ] = END;

	/* Very complex regular expressions can overflow the destination buffer */
	if( destIndex >= MATCH_DEST_LEN )
		return( WILDCARD_COMPLEX_ERROR );

	/* Return count of no.of chars in output + null delimiter */
	return( destIndex );
	}

/****************************************************************************
*																			*
*						Wildcard String Matching Functions					*
*																			*
****************************************************************************/

/* A simple queue, used to handle backtracking.  Once the queue is full, we
   stop adding items, even if space has been freed up at the front.  This
   conveniently stops deep recursion for very complex expressions.  The queue
   size is chosen to be in the vicinity of the maximum filename length we can
   expect, to allow for a worst-case scenario of matching "*x" to "xxxxx..." */

#define QUEUE_SIZE	50

static int srcQueue[ QUEUE_SIZE ], destQueue[ QUEUE_SIZE ], queueFront, queueEnd;

/* Match a pattern string with wildcards against a literal string */

BOOLEAN matchString( const char *src, const char *dest, const BOOLEAN hasWildcards )
	{
	char ch, matchChar;
	int srcIndex = 0, destIndex = 0;
	BOOLEAN match = TRUE, notFlag = FALSE, passedChar = FALSE;

	/* Simple case: If there are no wildcards, just do a straight compare */
	if( !hasWildcards )
		{
		/* Check for special cases of empty string and differing string lengths */
		if( strlen( src ) != strlen( dest ) )
			return( FALSE );

		return( !caseStrcmp( src, dest ) );
		}

	/* Set up queue for backtracking */
	queueFront = queueEnd = 0;

retry:
#pragma warn -pia
	while( ( ch = src[ srcIndex++ ] ) && match )
#pragma warn +pia
		switch( ch )
			{
			case MATCHEND:
				return( match );		/* Match all remaining chars */

			case MATCHTO:
				matchChar = src[ srcIndex ];
				while( caseMatch( dest[ destIndex ] ) != matchChar && dest[ destIndex ] )
					{
					/* We can't do this as part of the main loop since that
					   wouldn't let us do a match of 0 chars */
					destIndex++;
					passedChar = TRUE;	/* Remember we've matched at least one char */
					}

				/* See if we exited due to running out of chars to match rather
				   than an actual match */
				if( !dest[ destIndex ] )
					match = FALSE;
				else
					/* Push current state so we can backtrack */
					if( queueEnd < QUEUE_SIZE )
						{
						srcQueue[ queueEnd ] = srcIndex - 1;
						destQueue[ queueEnd++ ] = ( passedChar ) ? destIndex : destIndex + 1;
						}
				passedChar = FALSE;
				break;

			case MATCHONE:
				if( !dest[ destIndex++ ] )
					match = FALSE;
				break;

			case NOTRANGE:
				notFlag = TRUE;
				/* Drop through */

			case RANGE:
				/* Note that when checking for a match it is necessary to use
				   "|=" to indicate a match, since we are not using short
				   circuit evaluation and thus with "=" a later non-match may
				   cancel a previous match.  Using short-circuit evaluation
				   give little advantage, since it involves adding gotos to
				   exit the loop, and yet we must still scan over src[]
				   anyway to get to the ENDRANGE.  Note also that binary
				   &'s are used in the comparisons to avoid short-circuit
				   evaluation, since the expressions have side effects */
				match = FALSE;
				matchChar = caseMatch( dest[ destIndex ] );
				destIndex++;
				while( ( ch = src[ srcIndex++ ] ) != ENDRANGE )
					if( ch == RANGE )
						{
						/* Joining the following two statements with an '&'
						   doesn't seem to work */
						match |= matchChar >= caseMatch( src[ srcIndex ] );
						srcIndex++;
						match &= matchChar <= caseMatch( src[ srcIndex ] );
						srcIndex++;
						}
					else
						match |= matchChar == caseMatch( ch );

				if( notFlag )
					match = !match;
				notFlag = FALSE;
				break;

			default:
				match = caseMatch( ch ) == caseMatch( dest[ destIndex ] );
				destIndex++;
			}

	/* No match if there are more matchable chars in one of the strings */
	if( src[ srcIndex - 1 ] || dest[ destIndex ] )
		match = FALSE;

	/* See if we can backtrack and try again */
	if( !match && ( queueFront != queueEnd ) )
		{
		srcIndex = srcQueue[ queueFront ];
		destIndex = destQueue[ queueFront++ ];
		match = TRUE;
		goto retry;
		}

	return( match );
	}
