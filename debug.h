#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#  define debug_print(x) printf x
#else
#  define debug_print(x) do {} while (0)
#endif

#endif