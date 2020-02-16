/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*					 	Output Intercept Handling Routines					*
*							DISPLAY.C  Updated 25/10/91						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			Copyright 1991  Peter C.Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* Note that the RFC-1341 Richtext format is currently defined in a very
   vague and nonspecific manner.  The following code is more general-purpose
   than the existing Richtext specification.  It is expected that at some
   point the Richtext format will be defined more rigorously, in which case
   the commented-out code (which implements a simple nroff-like formatter)
   can be enabled */

#include <string.h>
#include "defs.h"
#include "frontend.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "io\fastio.h"
#elif defined( __MAC__ )
  #include "fastio.h"
#else
  #include "io/fastio.h"
#endif /* System-specific include directives */

/* Maximum possible screen width */

#define MAX_WIDTH	150

#if defined( __MSDOS16__ ) && !defined( GUI )

/* Prototypes for functions in BIOSCALL.ASM */

BYTE getAttribute( void );
void writeChar( const char ch, const BYTE attribute );

/* Text mode information */

#define NORMAL_ATTR		0x07	/* Normal text */
#define BLINK_MASK		0x80	/* Mask for blink bit */
#define INTENSITY_MASK	0x08	/* Mask for intensity bit */

#endif /* __MSDOS16__ && !GUI */

/* Command settings */

BOOLEAN doJustify, doCenter;
int leftMargin, rightMargin, tempIndent;
BOOLEAN doFixed, doBold;

/* Internal status info */

#ifndef GUI
  int lineNo;
#endif /* !GUI */
int outPos, outWords;
char outBuffer[ MAX_WIDTH ];	/* Max possible screen width */
BOOLEAN breakFollows;
#if defined( __MSDOS16__ ) && !defined( GUI )
  BYTE outAttrBuffer[ MAX_WIDTH ];
  BYTE attribute;
  BOOLEAN putSpace;				/* Whether there is a space queued */
#endif /* __MSDOS16__ && !GUI */

/* Prototypes for various functions */

static void putWord( char *word, int length );

#if 0

/* Get an optional numeric parameter */

static void getParam( char *str, int *theValue, const int defaultValue,
					 const int minValue, const int maxValue )
	{
	char ch;
	int value = 0;
	BOOLEAN sign = FALSE, absolute = TRUE;

	/* Skip whitespace */
	while( ( ch = *str ) == ' ' || ch == '\t' )
		str++;

	/* Check for sign */
	if( ( ch = *str ) == '-' || ch == '+' )
		{
		str++;
		absolute = FALSE;	/* This is a delta not an absolute value */
		sign = ch - '+';	/* sign = ~( ch - '+' + 1 ); value *= sign */
		}

	/* Get digit */
	while( *str > '0' && *str < '9' )
		value = ( value * 10 ) + ( *str++ - '0' );

	/* Adjust sign */
	if( sign )
		value = -value;

	/* Do range check */
	if( value )
		if( absolute )
			*theValue = value;
		else
			*theValue += value;
	else
		*theValue = defaultValue;
	if( *theValue < minValue )
		*theValue = minValue;
	if( *theValue > maxValue )
		*theValue = maxValue;
	}
#endif /* 0 */

/* Force a line break */

static void doBreak( void )
	{
	BOOLEAN justifyValue = doJustify;

	doJustify = FALSE;		/* Make sure we don't justify half-full line */
	putWord( "", rightMargin );
	doJustify = justifyValue;
	outPos = leftMargin + tempIndent;
	tempIndent = 0;
	}

/* Process a formatting command.  Note that we can't do an immediate break in
   some cases when the command has an implied .br since it may be followed
   by further implied-.br commands, so we set the breakFollows flag which
   forces a break when we next output text */

enum { NL, NP, BOLD, ITALIC, FIXED, SMALLER, BIGGER, UNDERLINE, CENTER,
	   FLUSHLEFT, FLUSHRIGHT, INDENT, INDENTRIGHT, OUTDENT, OUTDENTRIGHT,
	   SAMEPAGE, SUBSCRIPT, SUPERSCRIPT, HEADING, FOOTING, ISO8859, ASCII,
	   EXCERPT, PARAGRAPH, SIGNATURE, COMMENT };

static char *commands[] = {
	"nl", "np", "bold", "italic", "fixed", "smaller", "bigger", "underline",
	"center", "flushLeft", "flushRight", "indent", "indentRight", "outdent",
	"outdentRight", "samePage", "subscript", "superscript", "heading",
	"footing", "iso-8859-x", "us-ascii", "excerpt", "paragraph",
	"signature", "comment", ""
	};

static void processCommand( char *command )
	{
	BOOLEAN isNegated = FALSE;
	int commandVal;

	/* Check whether it's a negated command */
	if( *command == '/' )
		{
		command++;	/* Skip negation char */
		isNegated = TRUE;
		}

	/* Find the command */
	for( commandVal = 0; commands[ commandVal ]; commandVal++ )
		if( !strcmp( commands[ commandVal ], command ) )
			break;

	switch( commandVal )
		{
		case BOLD:
			/* Boldface font */
#if defined( __MSDOS16__ ) && !defined( GUI )
			if( isNegated )
				attribute &= ~INTENSITY_MASK;
			else
				attribute |= INTENSITY_MASK;
#else
			if( isNegated )
				doFixed = FALSE;
			else
				doFixed = TRUE;

			/* Force output of either initial "*" or trailing space */
			putWord( "", 0 );
#endif /* __MSDOS16__ && !GUI */
			break;

		case NP:
			/* Page break */
			doBreak();
#ifndef GUI
			while( lineNo )
				/* Print lots of blank lines */
				doBreak();
#endif /* !GUI */

		case NL:
			/* Line break */
			doBreak();
			break;

		case CENTER:
			/* Center text */
			doBreak();
			doCenter = TRUE;
			break;

		case FLUSHLEFT:
			/* Justify text (was FILL) */
			if( isNegated )
				{
				breakFollows = TRUE;
				doJustify = FALSE;
				}
			else
				breakFollows = doJustify = TRUE;
			break;

		case FIXED:
			/* Fixed-width font */
			if( isNegated )
				doFixed = FALSE;
			else
				doFixed = TRUE;

			/* Force output of either initial "_" or trailing space */
			putWord( "", 0 );
			break;

		case INDENT:
			/* Set left margin */
			breakFollows = TRUE;
/*			getParam( command + 2, &leftMargin, 0, -leftMargin, rightMargin ); */
			if( isNegated )
				leftMargin = 0;
			else
				leftMargin = 5;
			break;

#if defined( __MSDOS16__ ) && !defined( GUI )
		case ITALIC:
			/* Italic text */
			if( isNegated )
				attribute &= ~INTENSITY_MASK;
			else
				attribute |= INTENSITY_MASK;	/* Treat as italic text */
			break;
#endif /* __MSDOS16__ && !GUI */

		case INDENTRIGHT:
			/* Set right margin (was RIGHT_MARGIN) */
			breakFollows = TRUE;
/*			rightMargin = screenWidth - rightMargin; */
/*			getParam( command + 2, &rightMargin, 0, 0, screenWidth - leftMargin - 1 ); */
/*			rightMargin = screenWidth - rightMargin; */
			rightMargin = screenWidth - 5;
			break;

		case OUTDENT:
			/* Temporary indent (was TEMP_INDENT) */
			breakFollows = TRUE;
/*			getParam( command + 2, &tempIndent, -leftMargin, -leftMargin, rightMargin ); */
			if( leftMargin )
				tempIndent = -5;
			break;
		}
	}

/* Output a word with all sorts of fancy formatting */

static void putWord( char *word, int length )
	{
	int noBlanks, i;
	static BOOLEAN forwards = TRUE;

	/* See if there is a break in the pipeline */
	if( breakFollows )
		{
		breakFollows = FALSE;
		doBreak();
		}

#if defined( __MSDOS16__ ) && !defined( GUI )
	/* Add a space if there's one queued.  We couldn't output this earlier
	   since there may have been an attribute change since then */
	if( putSpace )
		{
		putSpace = FALSE;
		outBuffer[ outPos++ ] = ' ';
		outAttrBuffer[ outPos - 1 ] = attribute;
		}
#endif /* __MSDOS16__ && !GUI */

	/* Check whether line is full */
	if( outPos + length >= rightMargin )
		{
		outBuffer[ outPos ] = '\0';

		/* Delete trailing blank */
		if( outBuffer[ outPos - 1 ] == ' ' )
			outBuffer[ --outPos ] = '\0';

		/* Check for special text formatting */
		if( doCenter )
			{
			/* Pad out line with spaces to center text */
			for( i = 0; outBuffer[ i ] == ' '; i++ );
			noBlanks = ( ( rightMargin - leftMargin ) - strlen( outBuffer + i ) ) >> 1;
			while( noBlanks-- )
#ifdef GUI
				putchar( ' ' );
#else
				hputchar( ' ' );
#endif /* GUI */
			doCenter = FALSE;
			}
		else
			if( doJustify && outPos < rightMargin )
				{
				/* Spread out the words on this line.  Note the use of
				   memmove() instead of strcpy() since the source and
				   destination strings overlap */
				noBlanks = ( rightMargin - outPos );
				while( noBlanks )
					{
					if( forwards )
						{
						/* Starting from the beginning of the line, insert
						   an extra space where there is already one */
						for( i = leftMargin + 1; i <= rightMargin; i++ )
							if( outBuffer[ i ] == ' ' )
								{
								memmove( outBuffer + i + 1, outBuffer + i, rightMargin - i );
#if defined( __MSDOS16__ ) && !defined( GUI )
								memmove( outAttrBuffer + i + 1, outAttrBuffer + i, rightMargin - i );
#endif /* __MSDOS16__ && !GUI */
								noBlanks--;
								i++;	/* Skip new blank */
								if( !noBlanks )
									break;
								}
						}
					else
						/* Starting from the end of the line, insert an extra
						   space where there is already one */
						for( i = outPos - 1; i > leftMargin; i-- )
							if( outBuffer[ i ] == ' ' )
								{
								memmove( outBuffer + i + 1, outBuffer + i, rightMargin - i );
#if defined( __MSDOS16__ ) && !defined( GUI )
								memmove( outAttrBuffer + i + 1, outAttrBuffer + i, rightMargin - i );
#endif /* __MSDOS16__ && !GUI */
								noBlanks--;
								i--;	/* Skip new blank */
								if( !noBlanks )
									break;
								}

					/* Reverse direction for next round */
					forwards = !forwards;
					}
				}
		outBuffer[ screenWidth + 1 ] = '\0'; /* Make sure we never overrun screen */
#if defined( __MSDOS16__ ) && !defined( GUI )
		noBlanks = strlen( outBuffer );
		for( i = 0; i < noBlanks; i++ )
			writeChar( outBuffer[ i ], outAttrBuffer[ i ] );
		if( noBlanks <= screenWidth )
			hputchar( '\n' );
#else
  #ifdef GUI
		printf( "%s\n", outBuffer );
  #else
		if( strlen( outBuffer ) == screenWidth + 1 )
			hprintf( "%s", outBuffer );	/* Line = screen width, no need for NL */
		else
			hprintf( "%s\n", outBuffer );
  #endif /* GUI */
#endif /* __MSDOS16__ && !GUI */

		/* Indent next line of text */
		memset( outBuffer, ' ', leftMargin + tempIndent );
#if defined( __MSDOS16__ ) && !defined( GUI )
		memset( outAttrBuffer, NORMAL_ATTR, leftMargin + tempIndent );
#endif /* __MSDOS16__ && !GUI */
		outPos = leftMargin + tempIndent;
		outWords = 0;

#ifndef GUI
		/* Handle page breaks */
		lineNo++;
		if( lineNo >= screenHeight - 1 )
			{
			hgetch();
			lineNo = 0;
			}
#endif /* GUI */
		}
	strncpy( outBuffer + outPos, word, length );
#if defined( __MSDOS16__ ) && !defined( GUI )
	memset( outAttrBuffer + outPos, attribute, length );
#endif /* __MSDOS16__ && !GUI */
	outPos += length;
	if( outPos < rightMargin )
#if defined( __MSDOS16__ ) && !defined( GUI )
		if( doFixed )
			{
			/* Do pseudo-fixed font */
			outBuffer[ outPos++ ] = '_';
			outAttrBuffer[ outPos - 1 ] = attribute;
			}
		else
			if( attribute != NORMAL_ATTR )
				/* We can't add the space now since the attribute may change by
				   the time it is due to be inserted, so we queue it for later
				   insertion */
				putSpace = TRUE;
			else
				{
				outBuffer[ outPos++ ] = ' ';
				outAttrBuffer[ outPos - 1 ] = attribute;
				}
#else
		if( doFixed )
			/* Do pseudo-fixed font */
			outBuffer[ outPos++ ] = '_';
		else
			if( doBold )
				/* Do pseudo-bold font */
				outBuffer[ outPos++ ] = '*';
			else
				outBuffer[ outPos++ ] = ' ';
#endif /* __MSDOS16__ && !GUI */
	outWords++;
	}

/****************************************************************************
*																			*
*						Formatted Output Driver Code						*
*																			*
****************************************************************************/

/* Variables for keeping track of input data.  Note that we can't make
   leftAnglePos a BOOLEAN since we need to keep track of whether we haven't
   seen a left angle, whether we've seen one at the start of the line, and
   whether we've seen one as part of a line */

int lineBufCount, leftAnglePos;
char lineBuffer[ MAX_WIDTH ];

/* Initialise the format routines */

void initOutFormatText( void )
	{
	/* Set up defaults */
	doJustify = TRUE;
	doCenter = FALSE;
	leftMargin = 0;
	rightMargin = screenWidth;

	/* Set up internal status info */
#ifndef GUI
	lineNo = 0;
#endif /* !GUI */
	outPos = tempIndent = lineBufCount;
	leftAnglePos = ERROR;
	breakFollows = FALSE;
	doFixed = doBold = FALSE;
#if defined( __MSDOS16__ ) && !defined( GUI )
	attribute = getAttribute();
	putSpace = FALSE;
#endif /* __MSDOS16__ && !GUI */
	}

/* Output a char via the format routines */

void outFormatChar( char ch )
	{
	/* Put a word in the line buffer */
	if( ch != '\r' && ch != '\n' && ch != 0x1A &&
		( ( *lineBuffer == '<' ) || ( ch != ' ' && ch != '\t' ) ) )
		{
		/* If we're inside a formatting command and we find a terminator,
		   process the command */
		if( ch == '>' )
			goto lineBreak;

		/* If it's a left angle, remember that we've seen it */
		if( ch == '<' )
			leftAnglePos = lineBufCount;
		else
			/* Simply put it in the buffer */
			if( ch >= ' ' && ch <= '~' )
				lineBuffer[ lineBufCount++ ] = ch;

		/* Force line break if line is overly long */
		if( leftMargin + tempIndent + lineBufCount > rightMargin )
			goto lineBreak;

		/* Process any data in lineBuffer if we've reached a command which
		   comes after data already in the buffer */
		if( lineBufCount && leftAnglePos > 0 )
			{
			putWord( lineBuffer, lineBufCount );
			lineBufCount = 0;
			leftAnglePos = ERROR;
			}
		}
	else
		{
lineBreak:
		lineBuffer[ lineBufCount ] = '\0';

		/* Process the data in the lineBuffer */
		if( leftAnglePos != ERROR )
			{
			/* Check for special literal '<' */
			if( !strcmp( lineBuffer, "lt" ) )
				{
				*lineBuffer = '<';
				lineBufCount = 1;
				return;
				}

			/* It's a normal command, process it */
			processCommand( lineBuffer );
			leftAnglePos = ERROR;
			}
		else
			if( lineBufCount )
				putWord( lineBuffer, lineBufCount );
		lineBufCount = 0;
		}
	}

/* Shutdown function for formatted text output */

void endOutFormatText( void )
	{
	/* Flush last line of text */
	if( outPos )
		{
		doJustify = FALSE;		/* Make sure we don't justify half-full line */
		putWord( "", rightMargin );
		}
	}
