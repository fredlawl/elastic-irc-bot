#include <stdio.h>
#include <criterion/criterion.h>
#include <sts_queue/sts_queue.h>
#include <string.h>
#include <stdbool.h>

#include "../irc_lexer.h"
#include "../irc_message_parser.h"
#include "../irc_message.h"

static StsHeader *__line_buffer;
static struct irc_lexer *__lexer;
static struct irc_message_parser *__parser;

static struct irc_message *__get_message(char *line);

void setup(void) {
  __line_buffer = StsQueue.create();
  __lexer = allocate_irc_lexer(__line_buffer);
  __parser = allocate_irc_message_parser(__lexer);
}

void teardown(void) {
  deallocate_irc_message_parser(__parser);
}

Test(irc_command_parsing, given_a_command_grab_name, .init = setup, .fini = teardown) {
  char *input_buffer = ":192.168.1.1 HELP someparam :someparam\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(strcasecmp(msg->command->command.name.value, "HELP") == 0);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_parameters, .init = setup, .fini = teardown) {
  char *input_buffer = ":192.168.1.2 HELP someparam someparam2 :someparam\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 2);

  deallocate_irc_message(msg);
}

static struct irc_message *__get_message(char *line) {
  char *input = (char *) malloc(sizeof(char) * strlen(line));
  strcpy(input, line);

  StsQueue.push(__line_buffer, input);

  // todo: add a attempts counter to break infinite loop
  while (true) {
    struct irc_message *msg = irc_message_parser_parse(__parser);

    if (msg != NULL && msg->command != NULL) {
      return msg;
    }

    if (!irc_message_parser_can_parse(__parser)) {
      continue;
    }
  }

  return NULL;
}