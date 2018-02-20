#ifndef HOLYHTTP_FILE_H
#define HOLYHTTP_FILE_H

#include <time.h>
#include <holyhttp.h>
#include "dict.h"

/*
 * holy_fetch_file - get file content
 * @filename: full path of file
 * @data: save the pointer of contnet
 * @len: save the length of content
 *
 * return: 1-ok, 0-fail
 */
int holy_fetch_file(char *filename, void **data, u32 *len);

/*
 * holy_get_file - get file content
 * @filename: full path of file
 * @data: save the pointer of contnet
 * @len: save the length of content
 *
 * use a simple caching mechanism
 *
 * return: 1-ok, 0-fail
 */
int holy_get_file(char *filename, void **data, u32 *len);

char *holy_guess_mime_type(char *filename);

#endif // HOLYHTTP_FILE_H
