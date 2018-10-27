#include <stdlib.h>

#include "irc_token.h"

struct irc_token {
  enum irc_token_type type;
  union irc_token_value value;
};

struct irc_token *allocate_irc_token(enum irc_token_type type, union irc_token_value value) {
  struct irc_token *tok;

  if ((tok = (struct irc_token *) malloc(sizeof(struct irc_token))) == NULL) {
    return NULL;
  }

  tok->type = type;
  tok->value = value;

  return tok;
}

void deallocate_irc_token(struct irc_token *token) {
  free(token);
}

enum irc_token_type irc_token_get_token_type(struct irc_token *token) {
  return token->type;
}

union irc_token_value irc_token_get_token_value(struct irc_token *token) {
  return token->value;
}

bool irc_token_is_token_type(struct irc_token *token, enum irc_token_type type) {
  return token->type == type;
}