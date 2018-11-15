#include <string.h>

#include "irc_message.h"

void irc_message_pretty_print(struct irc_message *msg, FILE *descriptor)
{
  char *time_as_string;

  time_as_string = asctime(gmtime(&msg->command->datetime_created));
  time_as_string[strlen(time_as_string) - 1] = '\0';

  fprintf(descriptor, "[%s] ", time_as_string);

  if (msg->prefix != NULL) {
    fprintf(descriptor, "%s -- ", msg->prefix->value);
  }

  if (irc_command_is_type(msg->command, IRC_CMD_CODE)) {
    fprintf(descriptor, "%i", msg->command->command.code);
  } else {
    fprintf(descriptor, "%s", msg->command->command.name.value);
  }

  fprintf(descriptor, " -- ");

  for (uint8_t i = 0; i < msg->command->parameter_count; i++) {
    fprintf(descriptor, "%s ", msg->command->parameters[i]->value);
  }

  fprintf(descriptor, "\n");
}
