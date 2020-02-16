/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							 		SQL Routines							*
*							  SQL.C  Updated 12/05/93						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1992 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* Just when you thought there couldn't be a more pointless archiver
   interface feature than ZIP's "Sort by CRC".... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "arcdir.h"
#include "choice.h"
#include "flags.h"
#include "frontend.h"
#include "hpacklib.h"
#include "script.h"
#include "sql.h"
#include "system.h"
#if defined( __ATARI__ )
  #include "asn1\asn1.h"
  #include "language\language.h"
#elif defined( __MAC__ )
  #include "asn1.h"
  #include "language.h"
#else
  #include "asn1/asn1.h"
  #include "language/language.h"
#endif /* Various system-specific include directives */

/* The following are declared in CLI.C */

int setBasePath( char *argString );
int setUserID( char *idString, char *argString );

/* The types of input we can expect */

typedef enum { BOOL, NUMERIC, STRING } INPUT_TYPE;

/* Possible settings for variables */

#define NO_SETTINGS			2

static char *settings[] = { "FALSE", "TRUE" };

/* Various variables to keep track of errors */

static int line;				/* The line on which an error occurred */

/* The settings parsed out by getAssignment() - a string, an integer value,
   or a boolean flag */

#define LINEBUF_SIZE	100				/* Size of the input buffer */

static char str[ LINEBUF_SIZE ];
static int value;
static BOOLEAN flag;

/****************************************************************************
*																			*
*						Line- and Keyword-Parsing Routines					*
*																			*
****************************************************************************/

/* A pointer to the string being parsed, and a buffer for the current token */

static char *tokenStrPtr;
static char tokenBuffer[ LINEBUF_SIZE ];

/* Search a list of keywords for a match */

static int lookup( char *key, char *keyWords[] )
	{
	int index = -1;

	/* Simply do a linear search through a list of keywords.  The keyword
	   lists are currently so short that there's little to be gained from a
	   more fancy binary search */
	while( *keyWords[ ++index ] )
		if( !stricmp( key, keyWords[ index ] ) )
			return( index );

	return ERROR;
	}

/* Get a string constant */

static int getString( void )
	{
	BOOLEAN noQuote = FALSE;
	int stringIndex = 0;
	char ch = *tokenStrPtr++;

	/* Skip whitespace */
	while( ch && ( ch == ' ' || ch == '\t' ) )
		ch = *tokenStrPtr++;

	/* Check for non-string */
	if( ch != '\"' )
		{
		/* Check for special case of null string */
		if( !ch )
			{
			*str = '\0';
			return( OK );
			}

		/* Use nasty non-rigorous string format */
		noQuote = TRUE;
		}

	/* Get first char of string */
	if( !noQuote )
		ch = *tokenStrPtr++;

	/* Get string into string value */
	while( ch && ch != '\"' )
		{
		/* Exit on '#' if using non-rigorous format */
		if( noQuote && ch == '#' )
			break;

		str[ stringIndex++ ] = ch;
		if( ( ch = *tokenStrPtr ) != '\0' )
			tokenStrPtr++;
		}

	/* If using the non-rigorous format, stomp trailing spaces */
	if( noQuote )
		while( stringIndex > 0 && str[ stringIndex - 1 ] == ' ' )
			stringIndex--;
	str[ stringIndex ] = '\0';

	/* Check for missing string terminator */
	if( ch != '\"' && !noQuote )
		{
		hprintf( MESG_UNTERM_STRING_s_LINE_d, str, line );
		return( ERROR );
		}

	return( OK );
	}

/* Get the first/next token from a string of tokens */

static char *getNextToken( void )
	{
	int index = 0;
	char ch;

	/* Check to see if it's a quoted string */
	if( *tokenStrPtr == '\"' )
		{
		getString();
		strcpy( tokenBuffer, str );
		}
	else
		{
		/* Find end of current token */
		while( ( index < LINEBUF_SIZE ) && ( ch = tokenStrPtr[ index ] ) != '\0' &&
			   ( ch != ' ' ) && ( ch != '\t' ) && ( ch != '=' ) &&
			   ( ch != '\"' ) && ( ch != ',' ) )
			index++;
		if( !index && ( ch == ',' ) || ( ch == '=' ) || ( ch == '\"' ) )
			index++;

		/* Copy the token to the token buffer */
		strncpy( tokenBuffer, tokenStrPtr, index );
		tokenBuffer[ index ] = '\0';
		tokenStrPtr += index;
		}

	/* Skip to start of next token */
	while( ( ch = *tokenStrPtr ) != '\0' && ( ch == ' ' ) || ( ch == '\t' ) )
		tokenStrPtr++;
	if( ch == '\0' || ch == '#' )
		/* Set end marker when we pass the last token */
		tokenStrPtr = "";

	return( tokenBuffer );
	}

static char *getFirstToken( const char *buffer )
	{
	char ch;

	/* Skip any leading whitespace in the string */
	tokenStrPtr = ( char * ) buffer;
	while( ( ch = *tokenStrPtr ) != '\0' && ( ch == ' ' ) || ( ch == '\t' ) )
		tokenStrPtr++;

	return( getNextToken() );
	}

/* Get an assignment to an intrinsic */

static int getAssignment( INPUT_TYPE settingType )
	{
	int index;
	char *token;

	token = getNextToken();
	if( !*token && ( settingType == BOOL ) )
		{
		/* Boolean option, no assignment gives setting of TRUE */
		flag = TRUE;
		return( OK );
		}

	/* Check for an assignment operator */
	if ( *token != '=' )
		{
		hprintf( MESG_EXP_ASST_BEFORE_s_LINE_d, token, line );
		return( ERROR );
		}

	switch( settingType )
		{
		case BOOL:
			/* Check for known intrinsic - really more general than just
			   checking for TRUE or FALSE */
			if( ( index = lookup( getNextToken(), settings ) ) == ERROR )
				return( ERROR );
			flag = index;
			break;

		case STRING:
			/* Get a string */
			getString();
			break;

		case NUMERIC:
			/* Get numeric input.  Error checking is a pain since atoi()
			   has no real equivalent of NAN */
			value = atoi( getNextToken() );
			break;
			}

	return( OK );
	}

/****************************************************************************
*																			*
*						Config File Handling Routines						*
*																			*
****************************************************************************/

/* The state we're in when handling an option which can span multiple lines */

typedef enum { STATE_NORMAL, STATE_ADD, STATE_EXTRACT, STATE_MODIFIER,
			   STATE_LIST } STATE;

static STATE state, oldState;

/* Possible commands.  These have two attributes associated with them,
   a name and a magic number */

static char *commands[] = { "ADD", "EXTRACT", "SET", "" };
enum { COMMAND_ADD, COMMAND_EXTRACT, COMMAND_SET };

/* Intrinsic variables.  These have three attributes associated with them,
   a name, a magic number, and a type */

static char *intrinsics[] = {
	"BASEPATH",
	"CRYPT", "CRYPT-ALL", "CRYPT-INDIVIDUAL",
		"CRYPT.SECONDARY", "CRYPT-ALL.SECONDARY",
	"FORCE-MOVE",
	"INTERACTIVE-MODE",
	"MULTIPART-ARCHIVE",
	"OVERWRITE-EXISTING",
	"PUBCRYPT", "PUBCRYPT-ALL", "PUBCRYPT-INDIVIDUAL",
		"PUBCRYPT.SECONDARY", "PUBCRYPT-ALL.SECONDARY",
	"RECURSE-DIRECTORIES",
	"SIGN", "SIGN-ALL", "SIGN-INDIVIDUAL",
	"SIGNERID",
	"STEALTH-MODE",
	"STORE-ATTRIBUTES",
	"STORE-ONLY",
	"TOUCH-FILES",
	"UNIT-COMPRESSION-MODE",
	"USERID", "USERID.MAIN", "USERID.SECONDARY",
	"VIEW-DIRECTORIES-ONLY", "VIEW-FILES-ONLY",
	"" };

enum { INTRINSIC_BASEPATH,
	   INTRINSIC_CRYPT, INTRINSIC_CRYPT_ALL, INTRINSIC_CRYPT_INDIVIDUAL,
		INTRINSIC_CRYPT_SECONDARY, INTRINSIC_CRYPT_ALL_SECONDARY,
	   INTRINSIC_FORCE_MOVE,
	   INTRINSIC_INTERACTIVE_MODE,
	   INTRINSIC_MULTIPART_ARCHIVE,
	   INTRINSIC_OVERWRITE_EXISTING,
	   INTRINSIC_PUBCRYPT, INTRINSIC_PUBCRYPT_ALL, INTRINSIC_PUBCRYPT_INDIVIDUAL,
		INTRINSIC_PUBCRYPT_SECONDARY, INTRINSIC_PUBCRYPT_ALL_SECONDARY,
	   INTRINSIC_RECURSE_DIRECTORIES,
	   INTRINSIC_SIGN, INTRINSIC_SIGN_ALL, INTRINSIC_SIGN_INDIVIDUAL,
	   INTRINSIC_SIGNERID,
	   INTRINSIC_STEALTH_MODE,
	   INTRINSIC_STORE_ATTRIBUTES,
	   INTRINSIC_STORE_ONLY,
	   INTRINSIC_TOUCH_FILES,
	   INTRINSIC_UNIT_COMPRESSION_MODE,
	   INTRINSIC_USERID, INTRINSIC_USERID_MAIN, INTRINSIC_USERID_SECONDARY,
	   INTRINSIC_VIEW_DIRECTORIES_ONLY, INTRINSIC_VIEW_FILES_ONLY
	   };

static INPUT_TYPE intrinsicType[] = {
	STRING,						/* Basepath */
	BOOL, BOOL, BOOL,			/* Crypt, Crypt-All, Crypt-Individual */
		BOOL, BOOL,				/* Crypt.Secondary, Crypt-All.Secondary */
	BOOL,						/* Force Move */
	BOOL,						/* Interactive Mode */
	BOOL,						/* Multipart Archive */
	BOOL,						/* Kill Existing Archive */
	BOOL, BOOL, BOOL,			/* Pubcrypt, Pubcrypt-All, Pubcrypt-Individual */
		BOOL, BOOL,				/* Pubcrypt.Secondary, Pubcrypt-All.Secondary */
	BOOL,						/* Recurse Subdirs */
	BOOL, BOOL, BOOL,			/* Sign, Sign-All, Sign-Individual */
	STRING,						/* SignerID */
	BOOL,						/* Stealth Mode */
	BOOL,						/* Store Attributes */
	BOOL,						/* Store Only */
	BOOL,						/* Touch Files */
	BOOL,						/* Unit Compression mode */
	STRING, STRING, STRING,		/* UserID, UserID.Main, UserID.Secondary */
	BOOL, BOOL					/* View Directories Only, View Files Only */
	};

/* Modifiers for data types.  These are like intrinsics but used for a
   different purpose */

static char *modifiers[] = {
	"CHARSET",
	"ERROR-CORRECTION",
	"LANGUAGE",
	"NAME",
	"TEXTTYPE",
	"" };

enum { MODIFIER_CHARSET,
	   MODIFER_ERROR_CORRECTION,
	   MODIFIER_LANGUAGE,
	   MODIFIER_NAME,
	   MODIFER_TEXTTYPE };

static INPUT_TYPE modifierType[] = {
	STRING,						/* Character set for text */
	BOOL,						/* Error correction */
	STRING,						/* Language text is written in */
	STRING,						/* Alternate filename to use */
	STRING						/* Text type (CR, LF, CRLF, record, etc) */
	};

/* Process an assignment to an intrinsic type */

static void processIntrinsicAssignment( int intrinsicIndex )
	{
	switch( intrinsicIndex )
		{
		case INTRINSIC_BASEPATH:
			setBasePath( str );
			break;

		case INTRINSIC_CRYPT:
		case INTRINSIC_CRYPT_ALL:
			flags |= CRYPT;
			cryptFlags |= CRYPT_CKE_ALL;
			break;

		case INTRINSIC_CRYPT_INDIVIDUAL:
			flags |= CRYPT;
			cryptFlags |= CRYPT_CKE;

		case INTRINSIC_CRYPT_SECONDARY:
		case INTRINSIC_CRYPT_ALL_SECONDARY:
			flags |= CRYPT;
			cryptFlags |= CRYPT_SEC | CRYPT_CKE_ALL;
			break;

		case INTRINSIC_FORCE_MOVE:
			flags |= MOVE_FILES;
			break;

		case INTRINSIC_INTERACTIVE_MODE:
			flags |= INTERACTIVE;
			break;

		case INTRINSIC_MULTIPART_ARCHIVE:
			flags |= MULTIPART_ARCH;
			break;

		case INTRINSIC_OVERWRITE_EXISTING:
			flags |= OVERWRITE_SRC;
			break;

		case INTRINSIC_PUBCRYPT:
		case INTRINSIC_PUBCRYPT_ALL:
			flags |= CRYPT;
			cryptFlags |= CRYPT_PKE_ALL;
			break;

		case INTRINSIC_PUBCRYPT_INDIVIDUAL:
			flags |= CRYPT;
			cryptFlags |= CRYPT_PKE;
			break;

		case INTRINSIC_PUBCRYPT_SECONDARY:
		case INTRINSIC_PUBCRYPT_ALL_SECONDARY:
			flags |= CRYPT;
			cryptFlags |= CRYPT_SEC | CRYPT_PKE_ALL;
			break;

		case INTRINSIC_RECURSE_DIRECTORIES:
			flags |= RECURSE_SUBDIR;
			break;

	   case INTRINSIC_SIGN:
	   case INTRINSIC_SIGN_ALL:
			cryptFlags |= CRYPT_SIGN_ALL;
			break;

	   case INTRINSIC_SIGN_INDIVIDUAL:
			cryptFlags |= CRYPT_SIGN;
			break;

		case INTRINSIC_SIGNERID:
			strcpy( signerID, str );
			break;

		case INTRINSIC_STEALTH_MODE:
			flags |= STEALTH_MODE;
			break;

		case INTRINSIC_STORE_ATTRIBUTES:
			flags |= STORE_ATTR;
			break;

		case INTRINSIC_STORE_ONLY:
			flags |= STORE_ONLY;
			break;

		case INTRINSIC_TOUCH_FILES:
			flags |= TOUCH_FILES;
			break;

		case INTRINSIC_UNIT_COMPRESSION_MODE:
			flags |= BLOCK_MODE;
			break;

		case INTRINSIC_USERID:
		case INTRINSIC_USERID_MAIN:
			strcpy( mainUserID, str );
			break;

		case INTRINSIC_USERID_SECONDARY:
			strcpy( secUserID, str );
			break;

		case INTRINSIC_VIEW_DIRECTORIES_ONLY:
			viewFlags |= VIEW_DIRS;
			break;

		case INTRINSIC_VIEW_FILES_ONLY:
			viewFlags |= VIEW_FILES;
			break;
		}
	}

/* Process an assignment to a modifier */

static void processModifierAssignment( int modifierIndex )
	{
	/* Currently unused - only needed in Level 2 version */
	modifierIndex++;	/* Get rid of warning */
	}

/* Start handling a config file */

void initConfigLine( void )
	{
	state = oldState = STATE_NORMAL;
	}

/* Process an individual line from a config file */

int processConfigLine( const char *configLine, const int lineNo )
	{
	int index, exitStatus = OK;
	char *token;
	BOOLEAN readToken = TRUE;
	static BOOLEAN startModifier = FALSE;

	/* Copy line count to global variable */
	line = lineNo;

	token = getFirstToken( configLine );
	while( TRUE )
		{
		/* Get the next input token */
		if( !readToken )
			token = getNextToken();
		readToken = FALSE;

		/* If there's no more input, try and get it from the next line */
		if( !*token )
			return( exitStatus );

		switch( state )
			{
			case STATE_NORMAL:
				/* Get the command required */
				if( ( index = lookup( token, commands ) ) == ERROR )
					{
					hprintf( MESG_UNKNOWN_COMMAND_s_LINE_d, token, line );
					return( ERROR );
					}

				/* Handle the SET command */
				if( index == COMMAND_SET )
					{
					/* Get the intrinsic the command applies to */
					if( ( index = lookup( ( token = getNextToken() ),
										  intrinsics ) ) == ERROR )
						{
						hprintf( MESG_UNKNOWN_VARIABLE_s_LINE_d, token, line );
						return( ERROR );
						}

					/* Handle the assignment associated with it */
					if( getAssignment( intrinsicType[ index ] ) != ERROR )
						{
						processIntrinsicAssignment( index );
						return( OK );
						}

					return( ERROR );
					}

				/* Handle the ADD/EXTRACT commands */
				state = ( index == COMMAND_ADD ) ? STATE_ADD : STATE_EXTRACT;
				break;

			case STATE_ADD:
			case STATE_EXTRACT:
				/* Check for optional modifiers */
				if( !stricmp( token, "WITH" ) || !stricmp( token, "AND" ) )
					{
					if( !stricmp( token, "AND" ) )
						{
						hprintf( MESG_UNEXP_KEYWORD_s_LINE_d, token, line );
						exitStatus = ERROR;
						}
					oldState = state;
					state = STATE_MODIFIER;
					startModifier = TRUE;
					break;
					}

				/* Check for start of file list */
				if( stricmp( token, "{" ) )
					{
					hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_LINE_d,
							 "{", token, line );
					exitStatus = ERROR;
					break;
					}
				oldState = state;
				state = STATE_LIST;
				break;

			case STATE_MODIFIER:
				/* Check for the start of the file list */
				if( !strcmp( token, "{" ) )
					{
					/* Check for start of file-list when we expect a modifier */
					if( startModifier )
						{
						hprintf( MESG_UNEXP_KEYWORD_s_LINE_d, "{", line );
						startModifier = FALSE;
						exitStatus = ERROR;
						}
					state = STATE_LIST;	/* Move on to file list handling */
					break;
					}

				/* Check for a continuation of a modifier-list.  Note that
				   we'll have complained previously if we find an AND at this
				   point so there's no need to print an error message */
				if( !stricmp( token, "AND" ) )
/*					if( startModifier )
						hprintf( MESG_UNEXP_KEYWORD_s_LINE_d, token, line );
					else */
						token = getNextToken();
				startModifier = FALSE;

				/* Get the modifier the command applies to */
				if( ( index = lookup( token, modifiers ) ) == ERROR )
					{
					hprintf( MESG_UNKNOWN_VARIABLE_s_LINE_d, token, line );
					exitStatus = ERROR;
					}

				/* Handle the assignment associated with it */
				if( getAssignment( modifierType[ index ] ) != ERROR )
					{
					processModifierAssignment( index );
					break;
					}
				break;

			case STATE_LIST:
				/* Special case when we're entering this state: The next
				   token may be an end-of-list marker */
				if( !strcmp( token, "}" ) )
					{
					state = STATE_NORMAL;
					return( OK );
					}

				/* Get the arguments in the file list */
				while( TRUE )
					{
					/* Perform some simple checking for typos */
					if( !strcmp( token, "{" ) || !strcmp( token, "," ) )
						{
						hprintf( MESG_UNEXP_KEYWORD_s_LINE_d, token, line );
						return( ERROR );
						}

					/* Add argument to file list.  We have to check for the
					   empty arg here in case there was a comma after the
					   last arg given but no more subsequent useful information
					   on the line.  In addition we make sure that ADD and
					   EXTRACT options are only applied where they match the
					   main HPACK command */
					if( *token &&
						( ( ( oldState == STATE_ADD ) &&
							( choice == ADD || choice == FRESHEN ||
							  choice == UPDATE || choice == REPLACE ) ) ||
						  ( ( oldState == STATE_EXTRACT ) &&
							( choice == DELETE || choice == DISPLAY ||
							  choice == TEST || choice == VIEW ||
							  choice == EXTRACT ) ) ) )
						addFilespec( token );

					/* Handle following token (either a comma, a }, or nothing) */
					token = getNextToken();
					if( !*token )
						return( exitStatus );	/* Must be on next line */

					if( !strcmp( token, "}" ) )
						{
						state = oldState = STATE_NORMAL;
						break;
						}
					if( stricmp( token, "," ) )
						{
						hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_LINE_d,
								 ",", token, line );
						return( ERROR );
						}

					token = getNextToken();
					readToken = TRUE;
					}
				break;
			}
		}

	state = STATE_NORMAL;
	return( ERROR );
	}

/****************************************************************************
*																			*
*							SQL Command Handling							*
*																			*
****************************************************************************/

/* Process an SQL command.  Currently somewhat minimal, only selection and
   sorting options are handled.  The general syntax is:

   SELECT <options>
	FROM <archive>
	ORDER BY <options>

   The SELECT option must be given, and can be either * (for the default
	settings) or any of SYSTEM, SIZE, LENGTH, DATE, TIME, and NAME.  The
	keyword can be followed by the optional qualifier DISTINCT, which is
	however ignored as all information in the archive is distinct anyway.

   The FROM keyword is optional (HPACK will ignore it and its argument if
	given since the archive name is already given on the command-line).

   The ORDER BY keyword can have the options NAME, DATE, TIME, SIZE, LENGTH,
	and	SYSTEM, with the optional modifier DESC for descending order.  In
	this case DATE is synonymous with TIME as they are treated as a unified
	field */

/* Select options */

#define NO_SELECT_OPTIONS	10		/* There are never more fields than this */

static char *selectOptions[] = { "NAME", "DATE", "TIME", "SIZE", "LENGTH",
								 "SYSTEM", "RATIO", "" };

int selectOrder[ NO_SELECT_OPTIONS ];
int selectOrderIndex;

/* Sort options */

#define NO_SORT_OPTIONS		10		/* There are never more fields than this */

static char *sortOptions[] = { "NAME", "DATE", "TIME", "SIZE", "LENGTH",
							   "SYSTEM", "" };

int sortOrder[ NO_SORT_OPTIONS ];
int sortOrderIndex;

/* Whether the SQL options have been set yet */

static BOOLEAN sqlOptionsSet;

int processSqlCommand( const char *sqlCommand )
	{
	int index;
	char *token;

	/* Clear selection array */
	for( index = 0; index < SELECT_END; index++ )
		selectOrder[ index ] = FALSE;

	/* Process the SELECT command option */
	if( stricmp( ( token = getFirstToken( sqlCommand ) ), "SELECT" ) )
		{
		hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_IN_s,
				 "SELECT", token, sqlCommand );
		return( ERROR );
		}
	token = getNextToken();
	if( !stricmp( token, "DISTINCT" ) )
		token = getNextToken();		/* DISTINCT keyword is ignored */
	if( !strcmp( token, "*" ) )
		{
		/* Add all default fields, skip to next token */
		selectOrder[ 0 ] = SELECT_SYSTEM, selectOrder[ 1 ] = SELECT_LENGTH;
		selectOrder[ 2 ] = SELECT_SIZE, selectOrder[ 3 ] = SELECT_RATIO;
		selectOrder[ 4 ] = SELECT_DATE, selectOrder[ 5 ] = SELECT_TIME;
		selectOrder[ 6 ] = SELECT_NAME, selectOrder[ 7 ] = SELECT_END;
		token = getNextToken();
		}
	else
		/* Get the selected options */
		while( TRUE )
			{
			if( ( index = lookup( token, selectOptions ) ) == ERROR )
				{
				hprintf( MESG_UNKNOWN_OPTION_s_IN_s, token, sqlCommand );
				return( ERROR );
				}
			selectOrder[ selectOrderIndex++ ] = index;
			selectOrder[ selectOrderIndex ] = SELECT_END;

			/* Handle following token (either a comma or FROM or ORDER) */
			token = getNextToken();
			if( !*token || !stricmp( token, "FROM" ) ||
						   !stricmp( token, "ORDER" ) ||
				( sortOrderIndex > NO_SORT_OPTIONS ) )
				break;
			if( strcmp( token, "," ) )
				{
				hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_IN_s,
						 ",", token, sqlCommand );
				return( ERROR );
				}

			token = getNextToken();
			}
	sqlOptionsSet = TRUE;	/* We've now set the SELECT options */
	if( !*token )
		return( OK );

	/* Process the FROM command option */
	if( !stricmp( token, "FROM" ) )
		{
		getNextToken();			/* Skip arg to FROM operator */
		token = getNextToken();
		if( !*token )
			return( OK );
		}

	if( stricmp( token, "ORDER" ) ||
		stricmp( getNextToken(), "BY" ) )
		{
		hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_IN_s,
				 "ORDER BY", token, sqlCommand );
		return( ERROR );
		}
	viewFlags |= VIEW_SORTED;	/* Make sure we sort them later on */

	/* Get the sort options */
	while( TRUE )
		{
		if( ( index = lookup( ( token = getNextToken() ), sortOptions ) ) == ERROR )
			{
			hprintf( MESG_UNKNOWN_OPTION_s_IN_s, token, sqlCommand );
			return( ERROR );
			}

		/* Handle following token (either a comma or DESC) */
		token = getNextToken();
		if( !stricmp( token, "DESC" ) )
			{
			index += SORT_REVERSE_BASE;
			token = getNextToken();
			}
		sortOrder[ sortOrderIndex++ ] = index;
		sortOrder[ sortOrderIndex ] = SORT_END;
		if( !*token || ( sortOrderIndex > NO_SORT_OPTIONS ) )
			break;
		if( stricmp( token, "," ) )
			{
			hprintf( MESG_EXP_KEYWORD_s_BEFORE_s_IN_s,
					 ",", token, sqlCommand );
			return( ERROR );
			}
		}

	return( OK );
	}

/* Set default SQL settings if none are already set */

void setDefaultSql( void )
	{
	if( !sqlOptionsSet )
		processSqlCommand( "SELECT *" );
	}

/* Reset SQL status */

void initSQL( void )
	{
	sortOrderIndex = selectOrderIndex = 0;
	sqlOptionsSet = FALSE;
	}
