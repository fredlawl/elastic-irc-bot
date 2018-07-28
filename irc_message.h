
#ifndef ELASTIC_IRC_BOT_IRC_MESSAGE_H
#define ELASTIC_IRC_BOT_IRC_MESSAGE_H

#include <unistd.h>

#include "irc_spec.h"

struct irc_prefix {
  size_t length;
  char *string;
};

struct irc_command_parameter {
  size_t length;
  char *string;
};

enum irc_command_type {
  IRC_CMD_CODE = 0,
  IRC_CMD_NAME
};

struct irc_command {
  union {
    struct {
      size_t length;
      char *name;
    } name;
    int code;
  } command;
  uint32_t parameter_count;
  struct irc_command_parameter *parameters[IRC_SPEC_MAX_COMMAND_PARAMETER_COUNT];
  enum irc_command_type command_type;
  time_t datetime_created;
};

struct irc_message {
  struct irc_prefix *prefix;
  struct irc_command *command;
};

#endif
