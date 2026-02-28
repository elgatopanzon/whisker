/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_std
 * @created     : Thursday Feb 13, 2025 17:44:48 CST
 * @description : standard library umbrella header aggregating all C standard includes
 */

#ifndef WHISKER_STD_H_
#define WHISKER_STD_H_

#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined __STDC__ && defined __STDC_VERSION__

  #if __STDC_VERSION__ >= 199409
    #include <iso646.h>
    #include <wchar.h>
    #include <wctype.h>
  #endif

  #if __STDC_VERSION__ >= 199901
    #ifndef __STDC_NO_COMPLEX__
      #include <complex.h>
    #endif
    #include <fenv.h>
    #include <inttypes.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <tgmath.h>
  #endif

  #if __STDC_VERSION__ >= 201112
    #include <stdalign.h>
    #ifndef __STDC_NO_ATOMICS__
      #include <stdatomic.h>
    #endif
    #include <stdnoreturn.h>
	#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(__has_include)
        #if __has_include(<threads.h>) && !defined(__STDC_NO_THREADS__)                     
        	#include <threads.h>                                                              
        #endif                                                                              
    #endif
  #include <uchar.h>
  #endif

  #if __STDC_VERSION__ >= 201710
    // None added
  #endif

#endif // #if defined __STDC__ && defined __STDC_VERSION__
#endif // WHISKER_STD_H_
