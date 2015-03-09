#ifdef DEBUG
#ifndef DEBUG_H

#include <stdio.h>

#  define debug_print(x) printf x
#else
#  define debug_print(x) do {} while (0)

#endif
#endif