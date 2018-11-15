
#ifndef ELASTIC_IRC_BOT_IRC_MESSAGE_H
#define ELASTIC_IRC_BOT_IRC_MESSAGE_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#include "irc_spec.h"

struct irc_prefix {
  size_t length;
  char *value;
};

struct irc_command_parameter {
  size_t length;
  char *value;
};

enum irc_command_type {
  IRC_CMD_CODE = 0,
  IRC_CMD_NAME
};

struct irc_command {
  union {
    struct {
      size_t length;
      char *value;
    } name;
    int code;
  } command;
  uint8_t parameter_count;
  struct irc_command_parameter *parameters[IRC_SPEC_MAX_COMMAND_PARAMETER_COUNT];
  enum irc_command_type command_type;
  time_t datetime_created;
};

struct irc_message {
  struct irc_prefix *prefix;
  struct irc_command *command;
};

inline
static bool irc_command_is_type(struct irc_command *cmd, enum irc_command_type type)
{
  return cmd->command_type == type;
}

bool irc_command_name_is(struct irc_command *cmd, char *name);

bool irc_command_code_is(struct irc_command *cmd, int code);

void irc_message_pretty_print(struct irc_message *msg, FILE *descriptor);

#endif
