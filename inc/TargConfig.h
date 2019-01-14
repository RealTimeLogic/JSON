
/* Configuration header file.
   Modify as needed
*/

#ifndef _TargConfig_h
#define _TargConfig_h

/* Define one of B_LITTLE_ENDIAN or B_BIG_ENDIAN */
#if !defined(B_LITTLE_ENDIAN) && !defined(B_BIG_ENDIAN)
#define B_LITTLE_ENDIAN
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Must return 32-bit aligned address */
#define baMalloc(s)        malloc(s)
#define baRealloc(m, s)    realloc(m, s)  /* as above */
#define baFree(m)          free(m)

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <winsock2.h>
#define millisleep(milliseconds) Sleep(milliseconds)
#else
#ifndef millisleep
#define millisleep(ms) do {                   \
      struct timespec tp;                       \
      tp.tv_sec = (ms)/1000;                    \
      tp.tv_nsec = ((ms) % 1000) * 1000000;     \
      nanosleep(&tp,0);                         \
   } while(0)
#endif
#endif


#ifndef INTEGRAL_TYPES
#define INTEGRAL_TYPES
#if (__STDC_VERSION__ >= 199901L) || defined( __GNUC__)
#include <stdint.h>
typedef uint8_t            U8;
typedef int8_t             S8;
typedef uint16_t           U16;
typedef int16_t            S16;
typedef uint32_t           U32;
typedef int32_t            S32;
typedef uint64_t           U64;
typedef int64_t            S64;
#else
typedef unsigned char      U8;
typedef signed   char      S8;
typedef unsigned short     U16;
typedef signed   short     S16;
typedef unsigned long      U32;
typedef signed   long      S32;
typedef unsigned long long U64;
typedef signed   long long S64;
#endif
#endif

#ifndef BaBool
#define BaBool U8
#endif


#define BA_API

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef NDEBUG
#include <assert.h>
#define baAssert assert
#else
#define baAssert(x)
#endif


#endif  /* _TargConfig_h */
