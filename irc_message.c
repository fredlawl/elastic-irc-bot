#include <string.h>

#include "irc_message.h"
#include "irc_logger.h"

char* __to_string(struct irc_message *msg);

bool irc_command_name_is(struct irc_command *cmd, char *name)
{
  return strcasecmp(name, cmd->command.value) == 0;
}

void irc_message_pretty_print(struct irc_message *msg, FILE* descriptor)
{
  // 512 + delim (4 chars) * 2 + 18 (spaces for parameters) + 1 null char
  char buf[539] = {'\0'};
  char *log_fmt = "Message: %s\n";

  if (msg->prefix != NULL) {
    strncat(buf, msg->prefix->value, msg->prefix->length);
    strcat(buf, " -- ");
  }

  strncat(buf, msg->command->command.value,
          msg->command->command.length);

  if (msg->command->parameter_count > 0) {
    strcat(buf, " -- ");
    for (uint8_t i = 0; i < msg->command->parameter_count; i++) {
      strncat(buf, msg->command->parameters[i]->value,
              msg->command->parameters[i]->length);
      strcat(buf, " ");
    }
  }

  if (descriptor == stdout || descriptor == NULL) {
    log_info(log_fmt, buf);
  } else if (descriptor == stderr) {
    log_error(log_fmt, buf);
  }
}
