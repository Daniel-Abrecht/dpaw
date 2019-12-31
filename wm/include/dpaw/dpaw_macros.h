#ifndef DPAW_MACROS_H
#define DPAW_MACROS_H

#include <assert.h>

#define DPAW_STR(A) DPAW_STR_EVAL(A)
#define DPAW_STR_EVAL(A) #A
#define DPAW_UNPACK(...) __VA_ARGS__
#define DPAW_CONCAT(A, B) DPAW_CONCAT_EVAL(A, B)
#define DPAW_CONCAT_EVAL(A, B) A ## B

#endif
