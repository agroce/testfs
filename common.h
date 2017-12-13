#ifndef _COMMON_H
#define _COMMON_H

#include <string.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#define EXIT(error) do { \
        printf("%s: %s: %s\n", __FUNCTION__, error, strerror(errno)); \
        exit(1); \
 } while (0)

#define WARN(error) do { \
        printf("%s: %s: %s\n", __FUNCTION__, error, strerror(errno)); \
 } while (0)

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#define DIVROUNDUP(a,b) (((a)+(b)-1)/(b))
#define ROUNDUP(a,b)    (DIVROUNDUP(a,b)*b)


#endif /* _COMMON_H */
