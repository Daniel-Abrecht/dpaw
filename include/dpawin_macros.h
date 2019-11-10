#ifndef DPAWIN_MACROS_H
#define DPAWIN_MACROS_H

#include <assert.h>

#define DPAWIN_STR(A) DPAWIN_STR_EVAL(A)
#define DPAWIN_STR_EVAL(A) #A
#define DPAWIN_CONCAT(A, B) DPAWIN_CONCAT_EVAL(A, B)
#define DPAWIN_CONCAT_EVAL(A, B) A ## B

#endif
