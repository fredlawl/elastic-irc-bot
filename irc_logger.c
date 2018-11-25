#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "irc_logger.h"

static void __get_time_str(char *buf);

void log_error(const char *fmt, ...)
{
  char time_as_str[26];
  va_list args;
  va_start(args, fmt);

  __get_time_str(time_as_str);

  fprintf(stderr, "[%s error] ", time_as_str);
  vfprintf(stderr, fmt, args);

  va_end(args);
}

void log_info(const char *fmt, ...)
{
  char time_as_str[26];
  va_list args;
  va_start(args, fmt);

  __get_time_str(time_as_str);

  fprintf(stdout, "[%s  info] ", time_as_str);
  vfprintf(stdout, fmt, args);

  va_end(args);
}

static void __get_time_str(char *buf)
{
  struct tm *tm;
  time_t log_time;

  log_time = time(NULL);
  tm = localtime(&log_time);
  strftime(buf, 26, "%Y-%m-%dT%H:%M:%S", tm);
}

