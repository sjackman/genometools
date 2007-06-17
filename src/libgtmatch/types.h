/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#ifndef TYPES_H
#define TYPES_H
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/*
  Some rules about types:
  - do not use Ulong, these are not portable.
  - do not use the constants, UINT_MAX, INT_MAX and INT_MIN
  The following are the assumptions about the types:
  - size(Uint) >= 4
  - size(Ushort) = 2
  - size(Sshort) = 2
  No other assumptions are to be made.
*/

/*
  Show a boolean value as a string or as a character 0 or 1.
*/

#define SHOWBOOL(B) ((B) ? "true" : "false")
#define SHOWBIT(B)  ((B) ? '1' : '0')

/*
  This file contains some basic type definition.
*/

typedef unsigned char  Uchar;         /* \Typedef{Uchar} */
typedef unsigned short Ushort;        /* \Typedef{Ushort} */

/*
  The following is the central case distinction to accomodate
  code for 32 bit integers and 64 bit integers.
*/

#if __WORDSIZE == 64

#define LOGWORDSIZE    6              /* base 2 logarithm of wordsize */
#define UintConst(N)   (N##UL)        /* unsigned integer constant */
typedef unsigned long  Uint;          /* \Typedef{Uint} */
typedef signed long ScanUint64;       /* \Typedef{Scaninteger} */
typedef unsigned long Uint64;         /* \Typedef{Uint64} */
#define FormatUint64     "%lu"
#define FormatScanUint64 "%ld"
#define CHECKIFFITS32BITS(VAL)        /* Nothing */

#else

#define LOGWORDSIZE   5              /* base 2 logarithm of wordsize */
#define UintConst(N)  (N##U)         /* unsigned integer constant */
typedef unsigned int  Uint;          /* \Typedef{Uint} */
typedef signed long long ScanUint64; /* \Typedef{Scaninteger} */
typedef unsigned long long Uint64;   /* \Typedef{Uint64} */
#define FormatUint64     "%llu"
#define FormatScanUint64 "%lld"
#define MAXUintValue  UINT_MAX       /* only possible in 32 bit mode */

#define CHECKIFFITS32BITS(VAL)\
        if ((VAL) > (Uint64) MAXUintValue)\
        {\
          /*@ignore@*/\
          fprintf(stderr,"%s, %d: " FormatUint64\
                         "cannot be stored in 32bit word",\
                         __FILE__,__LINE__,VAL);\
          /*@end@*/\
          exit(EXIT_FAILURE);\
        }
#endif

/*
  Type of unsigned integer in \texttt{printf}.
*/

typedef unsigned long Showuint;     /* \Typedef{Showuint} */

/*
  Type of signed integer in \texttt{printf}.
*/

typedef signed long Showsint;       /* \Typedef{Showsint} */

/*
  Type of integer in \texttt{scanf}.
*/

typedef signed long Scaninteger;         /* \Typedef{Scaninteger} */

/*
  Argument of a function from \texttt{ctype.h}.
*/

typedef int Ctypeargumenttype;      /* \Typedef{Ctypeargumenttype} */

/*
  type of second argument of fgets
*/

typedef int Fgetssizetype;      /* \Typedef{Fgetssizetype} */

/*
  Return type of \texttt{fgetc} and \texttt{getc}.
*/

typedef int Fgetcreturntype;        /* \Typedef{Fgetcreturntype} */

/*
  Type of first argument of \texttt{putc}.
*/

typedef int Fputcfirstargtype;      /* \Typedef{Fputcfirstargtype} */

/*
  Returntype of \texttt{putc}.
*/

typedef int Fputcreturntype;      /* \Typedef{Fputcreturntype} */

/*
  Return type of \texttt{strcmp}.
*/

typedef int Strcmpreturntype;       /* \Typedef{Strcmpreturntype} */

/*
  Type of a file descriptor.
*/

typedef int Filedesctype;           /* \Typedef{Filedesctype} */

/*
  Return type of \texttt{qsort} function.
*/

typedef int Qsortcomparereturntype; /* \Typedef{Qsortcomparefunction} */

/*
  Return type of \texttt{sprintf} function.
*/

typedef int Sprintfreturntype;     /* \Typedef{Sprintfreturntype}  */

/*
  Type of fieldwidth in \texttt{printf} format string.
*/

typedef int Fieldwidthtype;         /* \Typedef{Fieldwidthtype} */

/*
  Type of \texttt{argc}-parameter in main.
*/

typedef int Argctype;               /* \Typedef{Argctype} */

/*
  Return type of \texttt{getrlimit}
*/

typedef int Getrlimitreturntype;    /* \Typedef{Getrlimitreturntype} */

/*
  type of error flag for gzerror
*/

typedef int Gzerrorflagtype;   /* \Typedef{Gzerrorflagtype} */

/*
  This is the type of the third argument of the function gzread
*/

typedef unsigned int Gzreadthirdarg;  /* \Typedef{Gzreadthirdarg} */

/*
  This is the return type of fclose like functions
*/

typedef int Fclosereturntype;  /* \Typedef{Fclosereturntype} */

/*
  This is the return type of fprintf like functions
*/

typedef int Fprintfreturntype;   /* \Typedef{Fprintfreturntype} */

/*
  This is the return value of the function regcomp and regexec
*/

typedef int Regexreturntype;    /* \Typedef{Regexreturntype} */

/*
  This is the type of arguments for function sysconf.
*/

typedef int Sysconfargtype;         /* \Typedef{Sysconfargtype} */

/*
  This is the type for integer codes for strings of some fixed length
*/

/*
  The following structure stores information about special characters.
*/

typedef struct
{
  Uint specialcharacters,      /* total number of special syms */
       specialranges,          /* number of ranges with special syms */
       lengthofspecialprefix,  /* number of specials at beginning of sequence */
       lengthofspecialsuffix;  /* number of specials at end of sequence */
} Specialcharinfo;

/*
  Pairs, triples, and quadruples of unsigned integers.
*/

typedef struct
{
  Uint uint0, 
       uint1;
} PairUint;                /* \Typedef{PairUint} */

/*
  The following type stores an unsigned character only if defined is True
*/

typedef struct
{
  Uchar ucharvalue;
  bool defined;
} DefinedUchar;   /* \Typedef{DefinedUchar} */

/*
  The following type stores an unsigned integer only if defined is True
*/

typedef struct
{
  Uint uintvalue;
  bool defined;
} DefinedUint;   /* \Typedef{DefinedUint} */

/*
  The following type stores a double only if defined is True
*/

typedef struct
{
  double doublevalue;
  bool defined;
} Defineddouble;   /* \Typedef{Defineddouble} */

/*
  The default number of bytes in a bitvector used for dynamic programming
  is 4.
*/

#ifndef DPBYTESINWORD
#define DPBYTESINWORD 4
#endif

/*
  The number of bytes in a dynamic programming bitvector determines the type
  of integers, the dp-bits are stored in.
*/

#if DPBYTESINWORD == 1
typedef unsigned char DPbitvector;          /* \Typedef{DPbitvector} */
#else
#if DPBYTESINWORD == 2
typedef unsigned short DPbitvector;
#else
#if DPBYTESINWORD == 4
typedef unsigned int DPbitvector;
#else
#if DPBYTESINWORD == 8
typedef unsigned long long DPbitvector;
#endif
#endif
#endif
#endif

/*
  The following is the type of the comparison function
  to be provided to the function \texttt{qsort}.
*/

typedef int (*Qsortcomparefunction)(const void *,const void *);

/*
  The following is the type of the function showing information in
  verbose mode.
*/

typedef void (*Showverbose)(char *);

typedef struct
{
  Uint64 uint0,
         uint1;
} PairUint64;                /* \Typedef{PairUint64} */

/*
  The following type stores an unsigned integer only of defined is True
*/

#endif
