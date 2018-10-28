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

enum irc_command_parser_substate {
  IRC_PARSER_SUBSTATE_NONE = 0,
  IRC_PARSER_SUBSTATE_CODE,
  IRC_PARSER_SUBSTATE_NAME
};

struct irc_message_parser {
  struct irc_lexer *lexer;
  struct irc_token *current_token;
  enum irc_parser_state state;
};

typedef void (*handle_parser_state_t)(struct irc_message_parser *parser, struct irc_message *message);

static inline struct irc_message *__allocate_irc_message();
static void __eat_token(struct irc_message_parser *parser, enum irc_token_type expected_token_type);
static inline void __force_advance(struct irc_message_parser *parser);
static void __handle_prefix(struct irc_message_parser *parser, struct irc_message *message);
static void __handle_command(struct irc_message_parser *parser, struct irc_message *message);
static void __handle_parameter(struct irc_message_parser *parser, struct irc_message *message);

static void __expecting_error(struct irc_message_parser *parser,
                              enum irc_token_type expected,
                              enum irc_token_type got);

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

  return msg;
}

void deallocate_irc_message(struct irc_message *msg) {
  uint8_t i;

  if (msg->prefix != NULL) {
    free(msg->prefix->value);
    free(msg->prefix);
  }


  if (msg->command != NULL) {
    if (irc_command_is_type(msg->command, IRC_CMD_NAME)) {
      free(msg->command->command.name.value);
    }

    for (i = 0; i < msg->command->parameter_count; i++) {
      free(msg->command->parameters[i]->value);
      free(msg->command->parameters[i]);
    }

    free(msg->command);
  }

  free(msg);
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

// @todo: Make a variadic function
static void __eat_token(struct irc_message_parser *parser, enum irc_token_type expected_token_type) {
  enum irc_token_type current_token_type;

  current_token_type = irc_token_get_token_type(parser->current_token);

  if (irc_token_is_token_type(parser->current_token, expected_token_type)) {
    __force_advance(parser);
    return;
  }

  __expecting_error(parser, expected_token_type, current_token_type);

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
    __force_advance(parser);
  }

  prefix = (struct irc_prefix *) malloc(sizeof(struct irc_prefix));
  if (prefix == NULL) {
    fprintf(stderr, "Unable to allocate prefix.\n");
    exit(EXIT_FAILURE);
  }

  prefix->length = prefix_len;
  prefix->value = (char *) calloc(prefix->length + 1, sizeof(char));
  strncpy(prefix->value, irc_lexer_get_current_line(parser->lexer) + 1, prefix_len);

  message->prefix = prefix;

  parser->state = IRC_PARSER_STATE_COMMAND;
}

static void __handle_command(struct irc_message_parser *parser, struct irc_message *message) {
  size_t cmd_length = 0;
  int cmd_code = 0;
  int cmd_parser_substate = IRC_PARSER_SUBSTATE_NONE;
  struct irc_command *command;

  if ((command = (struct irc_command *) malloc(sizeof(struct irc_command))) == NULL) {
    fprintf(stderr, "Unable to allocate irc command.\n");
    exit(EXIT_FAILURE);
  }

  command->parameter_count = 0;
  command->datetime_created = time(NULL);
  memset(command->parameters, '\0', sizeof(struct irc_command_parameter *) * IRC_SPEC_MAX_COMMAND_PARAMETER_COUNT);

  while (true) {
    if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_SPACE)) {
      __eat_token(parser, IRC_TOKEN_SPACE);
      break;
    }

    if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOL)) {
      __eat_token(parser, IRC_TOKEN_EOL);
      break;
    }

    if (cmd_parser_substate == IRC_PARSER_SUBSTATE_NONE) {
      if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_DIGIT))
        cmd_parser_substate = IRC_PARSER_SUBSTATE_CODE;
      else
        cmd_parser_substate = IRC_PARSER_SUBSTATE_NAME;
    }

    cmd_length++;
    if (cmd_parser_substate == IRC_PARSER_SUBSTATE_CODE) {
      cmd_code = cmd_code * 10 + irc_token_get_token_value(parser->current_token).integer;
      __eat_token(parser, IRC_TOKEN_DIGIT);
    }

    if (cmd_parser_substate == IRC_PARSER_SUBSTATE_NAME) {
      __eat_token(parser, IRC_TOKEN_LETTER);
    }

  }


  if (cmd_parser_substate == IRC_PARSER_SUBSTATE_NONE) {
    free(command);
    goto advance_state;
  }

  if (cmd_parser_substate == IRC_PARSER_SUBSTATE_CODE) {
    if (cmd_length != 3) {
      fprintf(stderr, "Command must be 3 digits.\n");
      exit(EXIT_FAILURE);
    }

    command->command.code = cmd_code;
    command->command_type = IRC_CMD_CODE;
  }

  if (cmd_parser_substate == IRC_PARSER_SUBSTATE_NAME) {
    command->command.name.length = cmd_length;
    command->command.name.value = (char *) calloc(cmd_length + 1, sizeof(char));

    char *line = irc_lexer_get_current_line(parser->lexer);
    size_t prefix_len = (message->prefix == NULL) ? 0 : message->prefix->length + 2;
    strncpy(command->command.name.value, line + prefix_len, cmd_length);

    command->command_type = IRC_CMD_NAME;
  }

  message->command = command;

advance_state:
  parser->state = IRC_PARSER_STATE_PARAM;

}

static void __handle_parameter(struct irc_message_parser *parser, struct irc_message *message) {
  bool kill = false;
  bool notcolon = true;
  size_t param_character_counter = 0;
  struct irc_command_parameter *param = NULL;
  char *param_string = NULL;
  char *line = irc_lexer_get_current_line(parser->lexer);
  size_t line_offset;

  line_offset = irc_lexer_get_current_column(parser->lexer);

  while (true) {

    if (kill) {
      break;
    }

    if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOL) ||
        irc_token_is_token_type(parser->current_token, IRC_TOKEN_NONE)) {
      break;
    }

    if (message->command->parameter_count > IRC_SPEC_MAX_COMMAND_PARAMETER_COUNT) {
      fprintf(stderr, "[FATAL PARSE ERROR] Parameter count exceeded.\n");
      exit(EXIT_FAILURE);
      break;
    }

    param_character_counter = 0;
    while (true) {

      if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_COLON) && notcolon) {
        notcolon = false;
        line_offset++;
        __eat_token(parser, IRC_TOKEN_COLON);
        continue;
      }

      if ((irc_token_is_token_type(parser->current_token, IRC_TOKEN_SPACE) && notcolon) ||
          irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOL)) {

        param = (struct irc_command_parameter *) malloc(sizeof(struct irc_command_parameter));
        param_string = (char *) calloc(param_character_counter + 1, sizeof(char));

        line_offset = (line_offset + message->command->parameter_count);
        strncpy(param_string, line + line_offset - param_character_counter - 1, param_character_counter);

        param->length = param_character_counter;
        param->value = param_string;
        message->command->parameters[message->command->parameter_count] = param;
        message->command->parameter_count++;

        if (irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOL)) {
          kill = true;
          __eat_token(parser, IRC_TOKEN_EOL);
          break;
        } else {
          __eat_token(parser, IRC_TOKEN_SPACE);
        }

        break;
      }

      param_character_counter++;
      line_offset++;
      __force_advance(parser);
    }
  }

  parser->state = IRC_PARSER_STATE_DONE;
}

static inline void __force_advance(struct irc_message_parser *parser) {
  deallocate_irc_token(parser->current_token);
  parser->current_token = irc_lexer_get_next_token(parser->lexer);
}

void __expecting_error(struct irc_message_parser *parser,
                       enum irc_token_type expected,
                       enum irc_token_type got) {

  struct irc_lexer *lex = parser->lexer;
  char *line = irc_lexer_get_current_line(lex);
  line[irc_lexer_get_current_line_length(lex) - 2] = '\0';
  line[irc_lexer_get_current_line_length(lex) - 1] = '\0';

  fprintf(stderr, "[FATAL PARSE ERROR] There was an error parsing input. ");
  fprintf(stderr, "Expecting token %i but got %i instead in the line ", expected, got);
  fprintf(stderr, "\"%s\"\n", line);

}
