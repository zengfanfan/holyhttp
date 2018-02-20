#include "print.h"
#include "string.h"

u32 holy_bkdr_hash_bin(void *bin, u32 len)
{
    u32 hash = 0;
    char *cbin = (char *)bin;
    
    if (!bin || !len) {
        return 0;//never fail
    }
    
    while (len--) {
        hash = hash * 131 + cbin[len];
    }
    
    return hash;
}

u32 holy_bkdr_hash(char *str)
{
    u32 hash = 0;
    
    if (!str) {
        return 0;//never fail
    }
    
    while (*str) {
        hash = hash * 131 + (*str++);
    }
    
    return hash;
}

void *holy_memfind(void *src, u32 slen, void *pattern, u32 plen)
{
    int i;
    u8 *pos = src;
    
    if (!src || !pattern || !slen || !plen || slen < plen) {
        return NULL;
    }

    for (i = 0; plen <= (slen - i); ++pos, ++i) {
        if (!memcmp(pos, pattern, plen)) {
            return pos;
        }
    }
    
    return NULL;
}

void *holy_memdup(void *src, u32 len)
{
    void *tmp;
    
    if (!src || !len) {
        return NULL;
    }
    
    tmp = malloc(len);
    if (tmp) {
        memcpy(tmp, src, len);
    }
    
    return tmp;
}

char *holy_strnstr(char *src, char *pattern, u32 slen)
{
    if (!src || !pattern || !slen) {
        return NULL;
    }
    return (char *)holy_memfind(src, slen, pattern, strlen(pattern));
}

void holy_str_trim_left(char *s, char c)
{
    char *head;
    
    if (!s) {
        return;
    }
    
    for (head = s; *head == c; ++head);

    if (head != s) {
        while (*head) *s++ = *head++;
        *s = 0;
        //strcpy(s, head);
    }
}

void holy_str_trim_right(char *s, char c)
{
    char *tail;
    
    if (!s) {
        return;
    }

    tail = s + strlen(s) - 1;
    if (tail < s) {
        return;
    }
    
    for (; tail >= s && *tail == c; --tail);

    tail[1] = 0;
}

void holy_str_trim(char *s, char c)
{
    holy_str_trim_left(s, c);
    holy_str_trim_right(s, c);
}

void holy_str2lower(char *str)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 0x20;//0x61 - 0x41
        }
        ++str;
    }
}

void holy_str2upper(char *str)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str -= 0x20;//0x61 - 0x41
        }
        ++str;
    }
}

int holy_str_isdecimal(char *str)
{
    int dot = 0;
    if (!str || !str[0]) {
        return 0;
    }

    if (*str == '-') {
        ++str;
    }

    if (!isdigit(*str)) {
        return 0;
    }
    
    while (*str) {
        if (!dot && *str == '.') {
            dot = 1;
            continue;
        }
        
        if (!isdigit(*str)) {
            return 0;
        }
        
        ++str;
    }

    return 1;
}

void holy_replace_char(char *str, char old_char, char new_char)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str == old_char) {
            *str = new_char;
        }
        str++;
    }
}

int holy_str_starts_with(char *str, char *prefix)
{
    if (!str || !prefix) {
        return 0;
    }
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int holy_str_ends_with(char *str, char *prefix)
{
    u32 slen, plen;
    char *s, *p;
    
    if (!str || !prefix) {
        return str == prefix;
    }

    slen = strlen(str);
    plen = strlen(prefix);
    if (slen < plen) {
        return 0;
    }

    for (s = str + slen, p = prefix + plen;
            p > prefix;
            --s, --p) {
        if (*s != *p) {
            return 0;
        }
    }

    return 1;
}

const char * const s64 = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890+-";

char *holy_uint_to_s64(u64 n)
{
    static char result[32];
    u32 size = sizeof result;
    int i;
    for (i = 0; i < size-1 && n > 0; ++i) {
        result[i] = s64[n & 0x3f];
        n /= 0x40;
    }
    result[i] = 0;
    return result;
}

void holy_str_append(char **s, char *d)
{
    u32 len;
    char *new;
    
    if (!s || !d) {
        return;
    }

    if (!*s) {
        *s = strdup(d);
        return;
    }

    len = strlen(*s) + strlen(d) + 1;
    new = (char *)malloc(len);
    if (!new) {
        MEMFAIL();
        return;
    }

    strcpy(new, *s);
    strcat(new, d);
    new[len - 1] = 0;
    free(*s);
    *s = new;
}

char *holy_join_path(char *buf, u32 bufsz, char *path1, char *path2)
{
    if (!buf || !bufsz || !path2) {
        return NULL;
    }

    buf[0] = 0;
    if (!path1 || path2[0] == '/') {
        STR_APPEND(buf, bufsz, path2);
        return buf;
    }

    STR_APPEND(buf, bufsz, "%s", path1);
    holy_str_trim_right(buf, '/');
    STR_APPEND(buf, bufsz, "/%s", path2);
    holy_str_trim_right(buf, '/');
    return buf;
}

char *holy_get_real_path(char *path, char *buf, unsigned bufsz)
{
    int i = 0, j = 0, plen, blen, is_abs, is_dir;
    char *item, **array;
    
    if (!path || !buf || !bufsz) {
        FREE_IF_NOT_NULL(path);
        return NULL;
    }

    plen = strlen(path);
    path = strdup(path);
    array = malloc(plen * sizeof(char *));
    if (!path || !array) {
        FREE_IF_NOT_NULL(path);
        FREE_IF_NOT_NULL(array);
        return NULL;
    }

    buf[0] = 0;
    is_abs = path[0] == '/';
    is_dir = path[plen-1] == '/';

    foreach_item_in_str(item, path, "/") {
        if (!strcmp(item, ".")) {
            // ignore
        } else if (!strcmp(item, "..")) {
            if (j > 0) {
                --j;
            }
        } else {
            array[j++] = item;
        }
        ++i;
    }

    if (is_abs) {
        STR_APPEND(buf, bufsz, "/");
    }

    for (i = 0; i < j; ++i) {
        STR_APPEND(buf, bufsz, "%s/", array[i]);
    }
    blen = strlen(buf);

    if (!is_dir && buf[blen-1] == '/' && blen > 1) {
        buf[blen-1] = 0;
    }
    
    free(path);
    free(array);
    return buf;
}

