#ifndef HOLYHTTP_TEMPLATE_H
#define HOLYHTTP_TEMPLATE_H

#include <stdarg.h>
#include <utils/dataset.h>

char *holy_srender(char *s, char *fmt, ...);
char *holy_frender(char *filename, char *fmt, ...);
char *holy_frender_by(char *filename, dataset_t *ds);
char *holy_srender_by(char *s, dataset_t *ds);
char *holy_vsrender(char *s, char *fmt, va_list ap);
char *holy_vfrender(char *filename, char *fmt, va_list ap);

#endif // HOLYHTTP_TEMPLATE_H
