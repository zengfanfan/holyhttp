#ifndef HOLYHTTP_TEMPLATE_H
#define HOLYHTTP_TEMPLATE_H

#include <stdarg.h>
#include <utils/dataset.h>

char *srender(char *s, char *fmt, ...);
char *frender(char *filename, char *fmt, ...);
char *frender_by(char *filename, dataset_t *ds);
char *srender_by(char *s, dataset_t *ds);
char *vsrender(char *s, char *fmt, va_list ap);
char *vfrender(char *filename, char *fmt, va_list ap);

#endif // HOLYHTTP_TEMPLATE_H
