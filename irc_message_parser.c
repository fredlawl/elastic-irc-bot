#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irc_token.h"
#include "irc_lexer.h"
#include "irc_message_parser.h"
#include "irc_spec.h"
#include "irc_message.h"

enum irc_parser_state {
  IRC_PARSER_STATE_PREFIX = 0,
  IRC_PARSER_STATE_COMMAND,
  IRC_PARSER_STATE_PARAM,
  IRC_PARSER_STATE_DONE
};

struct irc_message_parser {
  size_t prefix_handler_count;
  size_t command_handler_count;
  struct irc_lexer *lexer;
  struct irc_token *current_token;
  enum irc_parser_state state;
};

typedef void (*handle_parser_state_t)(struct irc_message_parser *parser, struct irc_message *message);

//static struct irc_prefix *__parse_prefix(struct irc_message_parser *parser);
//static struct irc_command_parameter *__parse_command_parameter(struct irc_message_parser *parser);
static inline struct irc_message *__allocate_irc_message();
static void __eat_token(struct irc_message_parser *parser, enum irc_token_type expected_token_type);
static inline void __force_advance(struct irc_message_parser *parser);

static void __handle_prefix(struct irc_message_parser *parser, struct irc_message *message);
static void __handle_command(struct irc_message_parser *parser, struct irc_message *message);
static void __handle_parameter(struct irc_message_parser *parser, struct irc_message *message);

static handle_parser_state_t parser_handlers[] = {
    &__handle_prefix,
    &__handle_command,
    &__handle_parameter
};

struct irc_message_parser *allocate_irc_message_parser(struct irc_lexer *lexer) {
  struct irc_message_parser *parser;
  union irc_token_value token_value;

  if ((parser = (struct irc_message_parser *) malloc(sizeof(struct irc_message_parser))) == NULL)
    return NULL;

  parser->lexer = lexer;

  token_value.character = '\0';
  parser->current_token = allocate_irc_token(IRC_TOKEN_EOF, token_value);
  parser->prefix_handler_count = 0;
  parser->command_handler_count = 0;
  parser->state = IRC_PARSER_STATE_PREFIX;

  return parser;
}

void deallocate_irc_message_parser(struct irc_message_parser *parser) {
  deallocate_irc_token(parser->current_token);
  deallocate_irc_lexer(parser->lexer);
  free(parser);
}

bool irc_message_parser_can_parse(struct irc_message_parser *parser) {
  return !irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOF);
}

struct irc_message *irc_message_parser_parse(struct irc_message_parser *parser) {
  struct irc_message *msg = __allocate_irc_message();

  if (msg == NULL)
    return NULL;

  if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOF)) {
    __eat_token(parser, IRC_TOKEN_EOF);
    return NULL;
  }

  parser->state = IRC_PARSER_STATE_PREFIX;

  while (parser->state != IRC_PARSER_STATE_DONE) {
    parser_handlers[parser->state](parser, msg);
  }

  // todo: remove
  if (msg->prefix != NULL) {
    printf("%s -- ", msg->prefix->value);
    printf("\n");
  }

  if (msg->command != NULL) {
    if (msg->command->command_type == IRC_CMD_CODE) {
      printf("%i", msg->command->command.code);
    } else {
      printf("%s", msg->command->command.name.value);
    }
  }
  // --

  return msg;
}

void deallocate_irc_message(struct irc_message *message) {
  size_t i;

  free(message->prefix->value);

  if (message->command->command_type == IRC_CMD_NAME) {
    free(message->command->command.name.value);
  }

  for (i = 0; i < message->command->parameter_count; i++) {
    free(message->command->parameters[i]->value);
    free(message->command->parameters[i]);
  }

  free(message->prefix);
  free(message->command);
  free(message);
}

static inline struct irc_message *__allocate_irc_message() {
  struct irc_message *msg;
  if ((msg = (struct irc_message *) malloc(sizeof(struct irc_message))) == NULL) {
    return NULL;
  }

  msg->prefix = NULL;
  msg->command = NULL;

  return msg;
}

static void __eat_token(struct irc_message_parser *parser, enum irc_token_type expected_token_type) {
  enum irc_token_type current_token_type;

  current_token_type = irc_token_get_token_type(parser->current_token);

  if (irc_token_is_token_type(parser->current_token, expected_token_type)) {
    __force_advance(parser);
    return;
  }

  fprintf(stderr, "[FATAL PARSE ERROR] There was an error parsing input. ");
  fprintf(stderr, "Expecting token %i but got %i instead.", expected_token_type, current_token_type);

  exit(EXIT_FAILURE);
}

static void __handle_prefix(struct irc_message_parser *parser, struct irc_message *message) {
  size_t prefix_len;
  struct irc_prefix *prefix;

  if (!irc_token_is_token_type(parser->current_token, IRC_TOKEN_COLON)) {
    parser->state = IRC_PARSER_STATE_COMMAND;
    return;
  }

  __eat_token(parser, IRC_TOKEN_COLON);

  prefix_len = 0;
  while (true) {

    if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_SPACE)) {
      __eat_token(parser, IRC_TOKEN_SPACE);
      break;
    }

    prefix_len++;

    if (!irc_token_is_token_type(parser->current_token, IRC_TOKEN_NOSPCRLFCL)) {
      __force_advance(parser);
    } else {
      __eat_token(parser, IRC_TOKEN_NOSPCRLFCL);
    }
  }

  prefix = (struct irc_prefix *) malloc(sizeof(struct irc_prefix));
  if (!prefix) {
    fprintf(stderr, "Unable to allocate prefix.\n");
    exit(EXIT_FAILURE);
  }

  prefix->length = prefix_len + 1;
  prefix->value = (char *) malloc(sizeof(char) * prefix->length);
  strncpy(prefix->value, irc_lexer_get_current_line(parser->lexer) + 1, prefix_len);
  prefix->value[prefix_len] = '\0';

  message->prefix = prefix;

  parser->state = IRC_PARSER_STATE_COMMAND;
}

static void __handle_command(struct irc_message_parser *parser, struct irc_message *message) {
  parser->state = IRC_PARSER_STATE_PARAM;
}

static void __handle_parameter(struct irc_message_parser *parser, struct irc_message *message) {

  while (true) {
    if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOL)) {
      __eat_token(parser, IRC_TOKEN_EOL);
      break;
    }

    __force_advance(parser);
  }

  parser->state = IRC_PARSER_STATE_DONE;
}

static inline void __force_advance(struct irc_message_parser *parser) {
  deallocate_irc_token(parser->current_token);
  parser->current_token = irc_lexer_get_next_token(parser->lexer);
}