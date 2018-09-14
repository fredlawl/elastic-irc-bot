#include <stdio.h>
#include <criterion/criterion.h>
#include <sts_queue/sts_queue.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

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

Test(irc_command_parsing, test_got_eol, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  struct irc_token *tok;
  char *input_buffer = "\r\n";
  char *input = (char *) malloc(sizeof(char) * strlen(input_buffer));
  strcpy(input, input_buffer);


  StsQueue.push(__line_buffer, input);
  tok = irc_lexer_get_next_token(__lexer);

  cr_assert(irc_token_is_token_type(tok, IRC_TOKEN_EOL));
}

Test(irc_command_parsing, given_a_command_grab_name, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.1 HELP someparam :someparam\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(strcasecmp(msg->command->command.name.value, "HELP") == 0);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_parameters, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP someparam someparam2 :someparam\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 3);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_no_parameters, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 0);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_no_colon_param, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP coolbeans\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 1);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_all_messages_params_have_no_leading_space, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP someparam someparam :coolbeansbro\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 3);

  for (uint8_t i = 0; i < msg->command->parameter_count; i++) {
    cr_assert(msg->command->parameters[i]->value[0] != ' ', "(%s) failed on param %i\n", msg->command->parameters[i]->value, i);
  }

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_trailing_command_parses_with_strings, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP someparam someparam :cool beans bro\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 3);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_colon_is_not_happening_twice, .init = setup, .fini = teardown, .exit_code = EXIT_FAILURE) {
  char *input_buffer = ":192.168.1.2 HELP someparam :someparam :cool beans bro\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  deallocate_irc_message(msg);
}

Test(irc_command_parsing, test_trailing_parameter, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP :cool beans bro\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameter_count == 1);

  deallocate_irc_message(msg);
}


Test(irc_command_parsing, given_parameters_test_trailing_does_not_include_colon, .init = setup, .fini = teardown, .exit_code = EXIT_SUCCESS) {
  char *input_buffer = ":192.168.1.2 HELP :cool beans bro\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

  cr_assert(msg->command->parameters[msg->command->parameter_count - 1]->value[0] != ':');

  deallocate_irc_message(msg);
}


Test(irc_command_parsing, test_given_too_many_params_fail, .init = setup, .fini = teardown, .exit_code = EXIT_FAILURE) {
  char *input_buffer = ":192.168.1.2 HELP cool beans bro great idea to test all the things make sure this fails due to big param count\r\n";
  struct irc_message *msg = __get_message(input_buffer);

  if (msg == NULL)
    cr_assert(false);

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