
#ifndef ELASTIC_IRC_BOT_IRC_TOKEN_H
#define ELASTIC_IRC_BOT_IRC_TOKEN_H

#include <stdint.h>
#include <stdbool.h>

struct irc_token;

enum irc_token_type {
  IRC_TOKEN_COLON = 0,
  IRC_TOKEN_SPACE,
  IRC_TOKEN_INTEGER_LITERAL,
  IRC_TOKEN_STRING_LITERAL,
  IRC_TOKEN_NOSPCRLFCL,
  IRC_TOKEN_LETTER,
  IRC_TOKEN_EOL,
  IRC_TOKEN_EOF,
  IRC_TOKEN_NONE
};

union irc_token_value {
  struct {
    size_t length;
    char *string;
  } string;
  int integer;
  char character;
};

struct irc_token *allocate_irc_token(enum irc_token_type type, union irc_token_value value);

void deallocate_irc_token(struct irc_token *token);

enum irc_token_type irc_token_get_token_type(struct irc_token *token);

union irc_token_value irc_token_get_token_value(struct irc_token *token);

bool irc_token_is_token_type(struct irc_token *token, enum irc_token_type type);

#endif