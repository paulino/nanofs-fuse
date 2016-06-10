#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"


#ifdef DEBUG

void log_error(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    fprintf(stderr,"** Error: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr,"\n");
}

void log_debug(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
}

#else

void log_error(const char *format, ...) {(void)format;}
void log_debug(const char *format, ...) {(void)format;}

#endif
