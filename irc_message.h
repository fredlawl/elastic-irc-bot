
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

struct irc_command {
  struct {
    size_t length;
    char *value;
  } command;
  uint8_t parameter_count;
  struct irc_command_parameter *parameters[IRC_SPEC_MAX_COMMAND_PARAMETER_COUNT];
  time_t datetime_created;
};

struct irc_message {
  struct irc_prefix *prefix;
  struct irc_command *command;
};

bool irc_command_name_is(struct irc_command *cmd, char *name);

void irc_message_pretty_print(struct irc_message *msg, FILE* descriptor);

#endif
