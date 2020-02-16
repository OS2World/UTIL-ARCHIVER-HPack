#ifndef _STDARG_DEFINED

#define _STDARG_DEFINED

typedef char *va_list;		/* Assume compiler doesn't have void * */

#define va_start(argPtr,lastParm)	( argPtr = ( char * ) ( &( lastParm ) + 1 ) )
#define va_arg(argPtr,type)			( ( type * ) ( argPtr += sizeof( type ) ) )[ -1 ]
#define va_end(argPtr)

#endif /* _STDARG_DEFINED */
