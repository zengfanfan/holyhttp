#ifndef HOLYHTTP_FILE_H
#define HOLYHTTP_FILE_H

#include <time.h>
#include "dict.h"

#define MAX_FILE_CACHE_SIZE (1024*1024) // 1M

/*
 * fetch_file - get file content
 * @filename: full path of file
 * @data: save the pointer of contnet
 * @len: save the length of content
 *
 * return: 1-ok, 0-fail
 */
int fetch_file(char *filename, void **data, u32 *len);

/*
 * get_file - get file content
 * @filename: full path of file
 * @data: save the pointer of contnet
 * @len: save the length of content
 *
 * use a simple caching mechanism
 *
 * return: 1-ok, 0-fail
 */
int get_file(char *filename, void **data, u32 *len);

char *guest_mime_type(char *filename);

#endif // HOLYHTTP_FILE_H
