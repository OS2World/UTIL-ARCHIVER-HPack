/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						  HPACK Messages Symbolic Defines					*
*							HPAKTEXT.H  Updated 07/04/93					*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*					and if you do so there will be....trubble.				*
*			 And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1991 - 1993  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* Initialise and shut down the messages system */

int initMessages( const char *path, const BOOLEAN doShowHelp );
void endMessages( void );

/* The following two procedures display the title message and help screen */

void showTitle( void );
void showHelp( void );

/* The following procedures take care of special-case strings which vary
   depending on various conditions */

char *getCase( const int count );

/* Below are the interface declarations for the messages in HPAKTEXT.C.
   Note that the message tags in these two files are automatically generated
   by a preprocessor from a text definition file and should not be relied
   upon to be consistent from one version to another - they serve merely as
   tags for the messages */

extern char RESPONSE_YES, RESPONSE_NO, RESPONSE_ALL, RESPONSE_QUIT;

extern char *mesg[];

/****************************************************************************
*																			*
*									Errors									*
*																			*
****************************************************************************/

#define INTERNAL_ERROR							mesg[   0 ]
#define OUT_OF_MEMORY							mesg[   1 ]
#define OUT_OF_DISK_SPACE						mesg[   2 ]
#define CANNOT_OPEN_ARCHFILE					mesg[   3 ]
#define CANNOT_OPEN_TEMPFILE					mesg[   4 ]
#define PATH_NOT_FOUND							mesg[   5 ]
#define CANNOT_ACCESS_BASEDIR					mesg[   6 ]
#define CANNOT_CREATE_DIR						mesg[   7 ]
#define STOPPED_AT_USER_REQUEST					mesg[   8 ]
#define FILE_ERROR								mesg[   9 ]
#define ARCHIVE_DIRECTORY_CORRUPTED				mesg[  10 ]
#define PATH_s_TOO_LONG							mesg[  11 ]
#define PATH_s__TOO_LONG						mesg[  12 ]
#define PATH_ss_TOO_LONG						mesg[  13 ]
#define PATH_ss__TOO_LONG						mesg[  14 ]
#define PATH_s_s_TOO_LONG						mesg[  15 ]
#define CANNOT_OVERRIDE_BASEPATH				mesg[  16 ]
#define TOO_MANY_LEVELS_NESTING					mesg[  17 ]
#define N_ERRORS_DETECTED_IN_SCRIPTFILE			mesg[  18 ]
#define NOT_HPACK_ARCHIVE						mesg[  19 ]
#define NO_FILES_ON_DISK						mesg[  20 ]
#define NO_FILES_IN_ARCHIVE						mesg[  21 ]
#define NO_ARCHIVES								mesg[  22 ]
#define BAD_KEYFILE								mesg[  23 ]
#define UNKNOWN_COMMAND							mesg[  24 ]
#define UNKNOWN_DIR_OPTION						mesg[  25 ]
#define UNKNOWN_OVERWRITE_OPTION				mesg[  26 ]
#define UNKNOWN_VIEW_OPTION						mesg[  27 ]
#define UNKNOWN_OPTION							mesg[  28 ]
#define MISSING_USERID							mesg[  29 ]
#define CANNOT_FIND_SECRET_KEY_FOR_s			mesg[  30 ]
#define CANNOT_FIND_SECRET_KEY					mesg[  31 ]
#define CANNOT_FIND_PUBLIC_KEY_FOR_s			mesg[  32 ]
#define CANNOT_READ_RANDOM_SEEDFILE				mesg[  33 ]
#define PASSWORDS_NOT_SAME						mesg[  34 ]
#define CANNOT_CHANGE_DEL_ARCH					mesg[  35 ]
#define CANNOT_CHANGE_MULTIPART_ARCH			mesg[  36 ]
#define CANNOT_CHANGE_ENCRYPTED_ARCH			mesg[  37 ]
#define CANNOT_CHANGE_UNENCRYPTED_ARCH			mesg[  38 ]
#define CANNOT_CHANGE_UNIFIED_ARCH				mesg[  39 ]
#define LONG_ARG_NOT_SUPPORTED					mesg[  40 ]
#define BAD_WILDCARD_FORMAT						mesg[  41 ]
#define WILDCARD_TOO_COMPLEX					mesg[  42 ]
#define CANNOT_USE_WILDCARDS_s					mesg[  43 ]
#define CANNOT_USE_BOTH_CKE_PKE					mesg[  44 ]
#define CANNOT_PROCESS_CRYPT_ARCH				mesg[  45 ]

/****************************************************************************
*																			*
*									Warnings								*
*																			*
****************************************************************************/

#define WARN_TRUNCATED_u_BYTES_EOF_PADDING		mesg[  46 ]
#define WARN_s_CONTINUE_YN						mesg[  47 ]
#define WARN_FILE_PROBABLY_CORRUPTED			mesg[  48 ]
#define WARN_UNCORR_ERRORS						mesg[  49 ]
#define WARN_CANT_FIND_PUBLIC_KEY				mesg[  50 ]
#define WARN_SEC_INFO_CORRUPTED					mesg[  51 ]
#define WARN_ARCHIVE_SECTION_TOO_SHORT			mesg[  52 ]
#define WARN_CANNOT_OPEN_DATAFILE_s				mesg[  53 ]
#define WARN_CANNOT_OPEN_SCRIPTFILE_s			mesg[  54 ]
#define WARN_DIRNAME_s_CONFLICTS_WITH_FILE		mesg[  55 ]
#define WARN_FILE_CORRUPTED						mesg[  56 ]
#define WARN_d_s_CORRUPTED						mesg[  57 ]

/****************************************************************************
*																			*
*								General Messages							*
*																			*
****************************************************************************/

#define MESG_VERIFY_SECURITY_INFO				mesg[  58 ]
#define MESG_AUTH_CHECK_FAILED					mesg[  59 ]
#define MESG_VERIFYING_ARCHIVE_AUTHENTICITY		mesg[  60 ]
#define MESG_SECURITY_INFO_WILL_BE_DESTROYED	mesg[  61 ]
#define MESG_ARCHIVE_DIRECTORY_CORRUPTED		mesg[  62 ]
#define MESG_ARCHIVE_DIRECTORY_WRONG_PASSWORD	mesg[  63 ]
#define MESG_PROCESSING_ARCHIVE_DIRECTORY		mesg[  64 ]
#define MESG_s_YN								mesg[  65 ]
#define MESG_s_s_YNA							mesg[  66 ]
#define MESG_SKIPPING_s							mesg[  67 ]
#define MESG_DATA_IS_ENCRYPTED					mesg[  68 ]
#define MESG_CANNOT_PROCESS_ENCR_INFO			mesg[  69 ]
#define MESG_CANNOT_OPEN_DATAFILE				mesg[  70 ]
#define MESG_EXTRACTING							mesg[  71 ]
#define MESG_s_AS								mesg[  72 ]
#define MESG_AS_s								mesg[  73 ]
#define MESG_UNKNOWN_ARCHIVING_METHOD			mesg[  74 ]
#define MESG__SKIPPING_s						mesg[  75 ]
#define MESG_WONT_OVERWRITE_EXISTING_s			mesg[  76 ]
#define MESG_s_ALREADY_EXISTS_ENTER_NEW_NAME	mesg[  77 ]
#define MESG_PATH_s__TOO_LONG					mesg[  78 ]
#define MESG_ALREADY_EXISTS__OVERWRITE			mesg[  79 ]
#define MESG_EXTRACT							mesg[  80 ]
#define MESG_DISPLAY							mesg[  81 ]
#define MESG_TEST								mesg[  82 ]
#define MESG_FILE_TESTED_OK						mesg[  83 ]
#define MESG_d_ERRORS_CORR						mesg[  84 ]
#define MESG_HIT_A_KEY							mesg[  85 ]
#define MESG_ADDING								mesg[  86 ]
#define MESG_ERROR								mesg[  87 ]
#define MESG_ERROR_DURING_ERROR_RECOVERY		mesg[  88 ]
#define MESG_CREATING_DIRECTORY_s				mesg[  89 ]
#define MESG_PROCESSING_SCRIPTFILE_s			mesg[  90 ]
#define MESG_INPUT_s__TOO_LONG_LINE_d			mesg[  91 ]
#define MESG_BAD_CHAR_IN_INPUT_LINE_d			mesg[  92 ]
#define MESG_MAXIMUM_LEVEL_OF					mesg[  93 ]
#define MESG_UNKNOWN_COMMAND_s_LINE_d			mesg[  94 ]
#define MESG_UNKNOWN_VARIABLE_s_LINE_d			mesg[  95 ]
#define MESG_UNTERM_STRING_s_LINE_d				mesg[  96 ]
#define MESG_EXP_ASST_BEFORE_s_LINE_d			mesg[  97 ]
#define MESG_EXP_KEYWORD_s_BEFORE_s_LINE_d		mesg[  98 ]
#define MESG_UNEXP_KEYWORD_s_LINE_d				mesg[  99 ]
#define MESG_EXP_KEYWORD_s_BEFORE_s_IN_s		mesg[ 100 ]
#define MESG_UNKNOWN_OPTION_s_IN_s				mesg[ 101 ]
#define MESG_ADDING_DIRECTORY_s					mesg[ 102 ]
#define MESG_CHECKING_DIRECTORY_s				mesg[ 103 ]
#define MESG_LEAVING_DIRECTORY_s				mesg[ 104 ]
#define MESG_FILE_s_ALREADY_IN_ARCH__SKIPPING	mesg[ 105 ]
#define MESG_ADD								mesg[ 106 ]
#define MESG_DELETE								mesg[ 107 ]
#define MESG_DELETING_s_FROM_ARCHIVE			mesg[ 108 ]
#define MESG_FRESHEN							mesg[ 109 ]
#define MESG_REPLACE							mesg[ 110 ]
#define MESG_ARCHIVE_IS_s						mesg[ 111 ]
#define MESG_ARCH_IS_UPTODATE					mesg[ 112 ]
#define MESG_DONE								mesg[ 113 ]
#define MESG_DIRECTORY							mesg[ 114 ]
#define MESG_DIRECTORY_TIME						mesg[ 115 ]
#define MESG_KEY_INCORRECT_LENGTH				mesg[ 116 ]
#define MESG_ENTER_PASSWORD						mesg[ 117 ]
#define MESG_ENTER_SEC_PASSWORD					mesg[ 118 ]
#define MESG_REENTER_TO_CONFIRM					mesg[ 119 ]
#define MESG_ENTER_SECKEY_PASSWORD				mesg[ 120 ]
#define MESG_PASSWORD_INCORRECT					mesg[ 121 ]
#define MESG_BAD_SIGNATURE						mesg[ 122 ]
#define MESG_GOOD_SIGNATURE						mesg[ 123 ]
#define MESG_SIGNATURE_FROM_s_DATE_dddddd		mesg[ 124 ]
#define MESG_WAIT								mesg[ 125 ]
#define MESG_PART_d_OF_MULTIPART_ARCHIVE		mesg[ 126 ]
#define MESG_PLEASE_INSERT_THE					mesg[ 127 ]
#define MESG_NEXT_DISK							mesg[ 128 ]
#define MESG_PREV_DISK							mesg[ 129 ]
#define MESG_DISK_CONTAINING					mesg[ 130 ]
#define MESG_PART_d								mesg[ 131 ]
#define MESG_THE_LAST_PART						mesg[ 132 ]
#define MESG_OF_THIS_ARCHIVE					mesg[ 133 ]
#define MESG_AND_PRESS_A_KEY					mesg[ 134 ]
#define MESG_CONTINUING							mesg[ 135 ]
#define MESG_HIT_SPACE_FOR_NEXT_SCREEN			mesg[ 136 ]
#define MESG_HIT_ANY_KEY						mesg[ 137 ]

/****************************************************************************
*																			*
*					Case-Specific Strings for File Counts					*
*																			*
****************************************************************************/

#define CASE_FILE_STRING						mesg[ 138 ]

/****************************************************************************
*																			*
*					Special Text Strings used in Viewfile.C					*
*																			*
****************************************************************************/

#define VIEWFILE_FIELD1							mesg[ 139 ]
#define VIEWFILE_FIELD2							mesg[ 140 ]
#define VIEWFILE_FIELD3							mesg[ 141 ]
#define VIEWFILE_FIELD4							mesg[ 142 ]
#define VIEWFILE_FIELD5							mesg[ 143 ]
#define VIEWFILE_FIELD6							mesg[ 144 ]
#define VIEWFILE_FIELD7							mesg[ 145 ]
#define VIEWFILE_TOTAL_LINE1					mesg[ 146 ]
#define VIEWFILE_TOTAL_LINE2					mesg[ 147 ]

/****************************************************************************
*																			*
*								OS-Specific Messages						*
*																			*
****************************************************************************/

/* The offset to bypass these messages on other systems */

#if defined( __MSDOS16__ ) || defined( __MSDOS32__ )
  #define OFFSET	0
#else
  #define OFFSET	-3
#endif /* __MSDOS16__ || __MSDOS32__ */

#define OSMESG_FILE_SHARING_VIOLATION			mesg[ 148 ]
#define OSMESG_FILE_LOCKING_VIOLATION			mesg[ 149 ]
#define OSMESG_FILE_IS_DEVICEDRVR				mesg[ 150 ]

/****************************************************************************
*																			*
*									Title Message							*
*																			*
****************************************************************************/

#define TITLE_LINE1								mesg[ 151 + OFFSET ]
#define TITLE_LINE2								mesg[ 152 + OFFSET ]
#define TITLE_LINE3								mesg[ 153 + OFFSET ]

/****************************************************************************
*																			*
*								Help Message Data Block						*
*																			*
****************************************************************************/

/* This is handled in a somewhat different manner.  The help strings are
   simply a large block of strings which are written to screen by a small
   procedure which pauses between screens etc.  As such all we need to know
   is the base address of the block of strings */

#define HELP_STRINGS_OFS						( 154 + OFFSET )
