#ifndef HOLYHTTP_STRING_H
#define HOLYHTTP_STRING_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"

#define STR_APPEND(buf, buf_sz, fmt, ...) \
    snprintf((buf) + strlen(buf), (buf_sz) - strlen(buf), fmt, ##__VA_ARGS__)

#define foreach_item_in_str(item, str, delimiter) \
    char *LINE_VAR(tmp);\
    for ((item) = strtok_r((str), (delimiter), &LINE_VAR(tmp));\
        (item);\
        (item) = strtok_r(NULL, (delimiter), &LINE_VAR(tmp)))

#define FREE_IF_NOT_NULL(ptr) if (ptr) free(ptr)

void holy_str_trim_left(char *s, char c);
void holy_str_trim_right(char *s, char c);
void holy_str_trim(char *s, char c);
void holy_str2lower(char *str);
void holy_str2upper(char *str);
void holy_replace_char(char *str, char old_char, char new_char);
int holy_str_starts_with(char *str, char *prefix);
int holy_str_ends_with(char *str, char *prefix);
char *holy_uint_to_s64(u64 n);
void holy_str_append(char **s, char *d);
int holy_str_isdecimal(char *str);
u32 holy_bkdr_hash_bin(void *bin, u32 len);
u32 holy_bkdr_hash(char *str);
void *holy_memfind(void *src, u32 slen, void *pattern, u32 plen);
char *holy_strnstr(char *src, char *pattern, u32 slen);
void *holy_memdup(void *src, u32 len);
char *holy_join_path(char *buf, u32 bufsz, char *path1, char *path2);
char *holy_get_real_path(char *path, char *buf, u32 bufsz);

#endif // HOLYHTTP_STRING_H
