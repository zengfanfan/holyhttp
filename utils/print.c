#include "print.h"
#include "string.h"

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
void holydump(char *title, void *data, int data_len)
{
    char left[50] = {0}; // 50 = 16*3 + 1 + 1
    char right[18] = {0}; // 18 = 16 + 1 + 1
    unsigned char *u8_data = data;
    int i, col, line;
    
    if (!title || !data || data_len <= 0) {
        return;
    }

    PRINT_CYAN("HOLYDUMP `%s`(length=%d):\n", title, data_len);
    
    for (i = 0; i < data_len; i++) {
        col = i % 0x10;
        line = i / 0x10;
        
        STR_APPEND(left, sizeof(left), "%02hhx ", u8_data[i]);
        if (u8_data[i] >= 0x21 && u8_data[i] <= 0x7E) {// printable
            STR_APPEND(right, sizeof(right), "%c", u8_data[i]);
        } else {
            STR_APPEND(right, sizeof(right), ".");
        }
        
        if (col == 0xF || i == (data_len - 1)) {
            PRINT_WHITE("%04d | %-49s | %s\n", line + 1, left, right);
            left[0] = 0;
            right[0] = 0;
        } else if (col == 0x7) {
            STR_APPEND(left, sizeof(left), " ");
            STR_APPEND(right, sizeof(right), " ");
        }
    }
}

