#ifndef fcaseopen_h
#define fcaseopen_h

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern FILE *fcaseopen(char const *path, char const *mode);
extern FILE *fcaseopenat(char const *base, char const *path, char const *mode);
extern int casechdir(char const *path);

#if defined(__cplusplus)
}
#endif

#endif
