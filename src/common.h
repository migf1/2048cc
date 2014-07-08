/****************************************************************
 * This file is part of the "2048 Console Clone" game.
 *
 * Author:       migf1 <mig_f1@hotmail.com>
 * Version:      0.3a
 * Date:         July 7, 2014
 * License:      Free Software (see comments in main.c for limitations)
 * --------------------------------------------------------------
 *
 ****************************************************************
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdarg.h>

/* Determine the compilation OS & define accordingly
 * some constants related to replay files.
 */
#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) \
|| defined(__TOS_WIN__)
	#define CC2048_OS_WINDOWS
	#define REPLAYS_FOLDER  "replays"
	#define LS_REPLAYS      "dir/D " REPLAYS_FOLDER "\\*" REPLAY_FNAME_EXT

#elif ( defined(__APPLE__) && defined(__MACH__) )              \
|| ( defined(__APPLE__) && defined(__MACH) )
	#define CC2048_OS_OSX
	#define REPLAYS_FOLDER  "replays"
	#define LS_REPLAYS      "ls -Gp " REPLAYS_FOLDER "/*" REPLAY_FNAME_EXT

#elif defined(__linux__) || defined(__linux) || defined(linux) \
|| defined(__gnu_linux__)
	#define CC2048_OS_LINUX
	#define REPLAYS_FOLDER  "replays"
	#define LS_REPLAYS      "ls --color -p " REPLAYS_FOLDER "/*" REPLAY_FNAME_EXT

#elif defined(__unix__) || defined(__unix) || defined(unix)    \
|| defined(__CYGWIN__)
	#define CC2048_OS_UNIX
	#define REPLAYS_FOLDER  "replays"
	#define LS_REPLAYS      "ls -Gp " REPLAYS_FOLDER "/*" REPLAY_FNAME_EXT

#else
	#define CC2048_OS_UNKNOWN
	#define LS_REPLAYS      /* void */
	#define REPLAYS_FOLDER  "replays"

#endif

/* Constants related to replay files, independently of OS.
 */
#define SZMAX_FNAME             BUFSIZ
#define REPLAY_FNAME_EXT        ".sav"

/* Cross-platform alternative to Windows' system( "pause" ).
 */
#define pressENTER()                                              \
do {                                                              \
	int c;                                                    \
        printf( "%s", "Press ENTER.." );                          \
	fflush(stdout);                                           \
	while ( '\n' != (c=getchar()) && EOF != c );              \
} while(0)

/* Print specified msg (a-la printf) along with debugging information.
 */
#define DBGF( format, ... )                                       \
do {                                                              \
	int c_;                                                   \
	puts("*** RUNTIME ERROR CAUGHT ****");                    \
	fprintf(stderr, "*** File: %s | Line: %d | Func: %s()\n", \
		__FILE__, __LINE__, __func__);                    \
	fprintf(stderr, "%s", "*** ");                            \
	fprintf(stderr, (const char *)(format), __VA_ARGS__);     \
	fflush(stderr);                                           \
        printf( " ***\n%s", "Press ENTER.." );                    \
	fflush(stdout);                                           \
	while ( '\n' != (c_=getchar()) && EOF != c_ );            \
} while(0)


/*
 * Prototypes of public functions.
 */

#ifndef COMMON_C
extern char *vprintf_to_text( const char *fmt, va_list vargs );
extern char *printf_to_text( const char *fmt, ... );

extern int  f_exists( const char *fname );

extern char *s_getflushed( char *s, size_t ssize );
extern int  s_tokenize(
                    char *s,
                    char *tokens[],
                    int  ntoks,
                    const char *delims
                    );
extern char *s_char_replace( char *s, int cin, int cout );
extern char *s_trim( char *s );
extern char *s_strip( char *s, const char *del );
extern char *s_fixeol( char *s  );
#endif

#endif