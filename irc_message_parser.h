#ifndef ELASTIC_IRC_BOT_IRC_MESSAGE_PARSER_H
#define ELASTIC_IRC_BOT_IRC_MESSAGE_PARSER_H

#include <stdbool.h>

struct irc_lexer;
struct irc_message_parser;
struct irc_message;

struct irc_message_parser *allocate_irc_message_parser(struct irc_lexer *lexer);

/**
 * This frees the parser AND the lexter within the parser
 * @param message
 */
void deallocate_irc_message_parser(struct irc_message_parser *parser);

void deallocate_irc_message(struct irc_message *message);

bool irc_message_parser_can_parse(struct irc_message_parser *parser);

struct irc_message *irc_message_parser_parse(struct irc_message_parser *parser);

#endif
