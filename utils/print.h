#ifndef HOLYHTTP_PRINT_H
#define HOLYHTTP_PRINT_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <holyhttp.h>

typedef enum {
    B_BLACK = 40,
    B_RED,
    B_GREEN,
    B_YELLOW,
    B_BLUE,
    B_PURPLE,
    B_CYAN,
    B_WHITE,
} background_color_t;

typedef enum {
    F_BLACK = 30,
    F_RED,
    F_GREEN,
    F_YELLOW,
    F_BLUE,
    F_PURPLE,
    F_CYAN,
    F_WHITE,
} foreground_color_t;

#define STYLE_FMT   "\033[1;%d;%dm"
#define STYLE_NONE  "\033[0m"

#define PRINT_COLOR(b_color, f_color, fmt, ...) \
    printf(STYLE_FMT fmt STYLE_NONE, b_color, f_color, ##__VA_ARGS__)

#define PRINT_NORMAL(fmt, ...)      printf(STYLE_NONE fmt STYLE_NONE, ##__VA_ARGS__)
#define PRINT_HOLY_RED(fmt, ...)    PRINT_COLOR(B_RED, F_WHITE, fmt, ##__VA_ARGS__)
#define PRINT_RED(fmt, ...)         PRINT_COLOR(B_BLACK, F_RED, fmt, ##__VA_ARGS__)
#define PRINT_GREEN(fmt, ...)       PRINT_COLOR(B_BLACK, F_GREEN, fmt, ##__VA_ARGS__)
#define PRINT_YELLOW(fmt, ...)      PRINT_COLOR(B_BLACK, F_YELLOW, fmt, ##__VA_ARGS__)
#define PRINT_BLUE(fmt, ...)        PRINT_COLOR(B_BLACK, F_BLUE, fmt, ##__VA_ARGS__)
#define PRINT_PURPLE(fmt, ...)      PRINT_COLOR(B_BLACK, F_PURPLE, fmt, ##__VA_ARGS__)
#define PRINT_CYAN(fmt, ...)        PRINT_COLOR(B_BLACK, F_CYAN, fmt, ##__VA_ARGS__)
#define PRINT_WHITE(fmt, ...)       PRINT_COLOR(B_BLACK, F_WHITE, fmt, ##__VA_ARGS__)

extern holy_dbg_lvl_t holydebug;

#define __LINE_FMT(fmt) "%04d %-10.10s: " fmt "\n", __LINE__, __func__

#define DEBUG(fmt, args...) \
    if (holydebug >= HOLY_DBG_DEBUG) \
    PRINT_CYAN(__LINE_FMT(fmt), ##args)
#define INFO(fmt, args...) \
    if (holydebug >= HOLY_DBG_DETAIL) \
    PRINT_WHITE(__LINE_FMT(fmt), ##args)
#define WARN(fmt, args...) \
    if (holydebug >= HOLY_DBG_WARN) \
    PRINT_YELLOW(__LINE_FMT(fmt), ##args)
#define ERROR(fmt, args...) \
    if (holydebug >= HOLY_DBG_ERROR) \
    PRINT_RED(__LINE_FMT(fmt), ##args)
#define FATAL(fmt, args...) \
    if (holydebug >= HOLY_DBG_FATAL) \
    PRINT_HOLY_RED(__LINE_FMT(fmt), ##args)

#define ERROR_NO(fmt, args...) \
    ERROR("Failed(%d) to " fmt ", %s.", errno, ##args, strerror(errno))
#define FATAL_NO(fmt, args...) \
    FATAL("Failed(%d) to " fmt ", %s.", errno, ##args, strerror(errno))
#define MEMFAIL(...) \
    ERROR("Out of memory!")

/*
    holydump - dump data
    @title: to prompt what's beging dumped
    @data: to be dumped
    @data_len: length of @data

    the dump style: (line number | hexadecimal | ASCII)
    
HOLYDUMP `my data`(length=100):
0001 | 68 65 6c 6c 6f 20 77 6f  72 6c 64 21 00 00 00 00  | hello wo rld!....
0002 | 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  | ........ ........
0003 | 00 00 00 00 00 00 01 00  00 00 00 84 04 08 00 00  | ........ ........
0004 | 00 00 a0 3e 01 40 30 eb  00 40 f4 ff 01 40 01 00  | ...>.@0. .@...@..
0005 | 00 00 00 84 04 08 00 00  00 00 21 84 04 08 b4 84  | ........ ..!.....
0006 | 04 08 01 00 00 00 d4 4d  9c bf 00 88 04 08 70 88  | .......M ......p.
0007 | 04 08 30 eb                                       | ..0.

*/
void holydump(char *title, void *data, int data_len);

#endif // HOLYHTTP_PRINT_H

