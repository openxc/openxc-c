#include "openxc/log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

const int MAX_LOG_LINE_LENGTH = 120;

void debug_no_newline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif // __DEBUG__
}

void initialize_logging() { }
