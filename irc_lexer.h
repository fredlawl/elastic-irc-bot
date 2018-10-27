
#ifndef ELASTIC_IRC_BOT_IRC_LEXER_H
#define ELASTIC_IRC_BOT_IRC_LEXER_H

#include "irc_token.h"

struct irc_lexer;
struct StsHeader;

struct irc_lexer *allocate_irc_lexer(struct StsHeader *line_buffer);

void deallocate_irc_lexer(struct irc_lexer *lexer);

struct irc_token *irc_lexer_get_next_token(struct irc_lexer *lexer);

struct irc_token *irc_lexer_peek_next_token(struct irc_lexer *lexer);

char *irc_lexer_get_current_line(struct irc_lexer *lexer);

size_t irc_lexer_get_current_line_length(struct irc_lexer *lexer);

size_t irc_lexer_get_current_column(struct irc_lexer *lexer);

#endif