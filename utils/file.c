#include <sys/stat.h>
#include <server/timer.h>
#include "print.h"
#include "file.h"

static dict_t cache = {.inited=0};

int fetch_file(char *filename, void **data, u32 *len)
{
	FILE *fp;
	struct stat st;
    int ret, flen;

    if (!filename || !data) {
        return 0;
    }

	if (stat(filename, &st) < 0) {
        ERROR_NO("stat file '%s'", filename);
        return 0;
	}

    if (!S_ISREG(st.st_mode)) {
        ERROR("'%s' is not a file.", filename);
        return 0;
    }

	flen = st.st_size;
    if (len) {
        *len = flen;
    }

	fp = fopen(filename, "rb");
    if (!fp) {
        ERROR_NO("open file '%s'", filename);
        return 0;
    }

    *data = malloc(flen + 1);
    if (!*data) {
        MEMFAIL();
        fclose(fp);
        return 0;
    }

    ret = fread(*data, flen, 1, fp);
    if (ret != 1) {
        ERROR("fread '%s' %d!=1", filename, ret);
    }

    ((char *)(*data))[flen] = 0;
    
    fclose(fp);
    return 1;
}

int get_file(char *filename, void **data, u32 *len)
{
    u32 l;
    void *tmp;

    if (!filename || !data) {
        return 0;
    }

    if (!dict_init(&cache)) {
        return 0;
    }

    if (!len) {
        len = &l;
    }

    if (cache.get_sb(&cache, filename, &tmp, len)) {
        *data = memdup(tmp, *len);// they're gonna free us outside!
        if (!*data) {
            MEMFAIL();
            return 0;
        }
        return 1;
    }

    if (!fetch_file(filename, data, len)) {
        return 0;
    }

    if (*len > MAX_FILE_CACHE_SIZE) {
        return 1;
    }

    cache.set_sb(&cache, filename, *data, *len + 1);

    return 1;
}

char *guest_mime_type(char *filename)
{
    char *fname = filename ? strdup(filename) : NULL;
    char *type;
    
    if (!fname) {
        return NULL;
    }

    str2lower(fname);
    
    if (str_ends_with(fname, ".html")
            || str_ends_with(fname, ".htm")) {
        type = "text/html";
    } else if (str_ends_with(fname, ".css")) {
        type = "text/css";
    } else if (str_ends_with(fname, ".js")) {
        type = "application/x-javascript";
    } else if (str_ends_with(fname, ".txt")
            || str_ends_with(fname, ".text")
            || str_ends_with(fname, ".log")) {
        type = "text";
    } else if (str_ends_with(fname, ".ico")) {
        type = "image/x-icon";
    } else if (str_ends_with(fname, ".gif")) {
        type = "image/gif";
    } else if (str_ends_with(fname, ".jpg")
            || str_ends_with(fname, ".jpeg")) {
        type = "image/jpeg";
    } else if (str_ends_with(fname, ".pdf")) {
        type = "application/pdf";
    } else if (str_ends_with(fname, ".swf")) {
        type = "application/x-shockwave-flash";
    } else if (str_ends_with(fname, ".svg")) {
        type = "image/svg+xml";
    } else if (str_ends_with(fname, ".tiff")
            || str_ends_with(fname, ".tif")) {
        type = "mage/tiff";
    } else if (str_ends_with(fname, ".swf")) {
        type = "application/x-shockwave-flash";
    } else {
        type = "application/octet-stream";
    }

    free(fname);
    return type;
}

