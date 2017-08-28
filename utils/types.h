#ifndef HOLYHTTP_TYPES_H
#define HOLYHTTP_TYPES_H

typedef unsigned char uchar, u8;
typedef unsigned short ushort, u16;
typedef unsigned int uint, u32;
typedef long long llong, i64;
typedef unsigned long long ullong, u64;

typedef char *str_t;
typedef void *void_ptr_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))

#ifndef __always_inline
#define __always_inline inline
#endif

typedef enum {
    false, true,
} bool;

#undef offset
#define offset(type, member) \
    ((long)&((type *)0)->member)

#undef container_of
#define container_of(ptr, type, member) \
    ((type *)(((long)ptr) - offset(type, member)))

#define MACRO_CONCAT(a, b)          a##b
#define __LINE_VAR(prefix, line)    MACRO_CONCAT(prefix, line)
#define LINE_VAR(name)              __LINE_VAR(MACRO_CONCAT(_, name), __LINE__)

#ifndef MIN
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

#endif // HOLYHTTP_TYPES_H
