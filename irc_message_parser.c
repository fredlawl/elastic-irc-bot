#include <stdlib.h>
#include <stdio.h>

#include "irc_token.h"
#include "irc_lexer.h"
#include "irc_message_parser.h"
#include "irc_spec.h"
#include "irc_message.h"

struct irc_message_parser {
  size_t prefix_handler_count;
  size_t command_handler_count;
  struct irc_lexer *lexer;
  struct irc_token *current_token;
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

  return parser;
}

void deallocate_irc_message_parser(struct irc_message_parser *parser) {
  deallocate_irc_lexer(parser->lexer);
  free(parser);
}

bool irc_message_parser_can_parse(struct irc_message_parser *parser) {
  return !irc_token_is_token_type(parser->current_token, IRC_TOKEN_EOF);
}

struct irc_message *irc_message_parser_parse(struct irc_message_parser *parser) {
  struct irc_token *token;

  deallocate_irc_token(parser->current_token);
  token = irc_lexer_get_next_token(parser->lexer);
  parser->current_token = token;

  if (irc_token_is_token_type(token, IRC_TOKEN_EOF)) {
    return NULL;
  }

  if (irc_token_is_token_type(token, IRC_TOKEN_EOL)) {
    printf("\n");
  }

  if (irc_token_is_token_type(token, IRC_TOKEN_NOSPCRLFCL) ||
      irc_token_is_token_type(token, IRC_TOKEN_LETTER)) {
    printf("%c", irc_token_get_token_value(token).character);
  }

  return NULL;
}

