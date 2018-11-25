
#ifndef ELASTIC_IRC_BOT_LOGGER_H
#define ELASTIC_IRC_BOT_LOGGER_H

#include <stdio.h>

void log_error(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_any(FILE* descriptor, const char *fmt, ...);

#endif
