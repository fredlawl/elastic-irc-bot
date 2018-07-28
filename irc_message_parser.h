#ifndef ELASTIC_IRC_BOT_IRC_MESSAGE_PARSER_H
#define ELASTIC_IRC_BOT_IRC_MESSAGE_PARSER_H

#include <stdbool.h>

struct irc_lexer;
struct irc_message_parser;
struct irc_message;

//typedef void (*irc_prefix_handler_t)(char *prefix, void *userdata);
//typedef void (*irc_command_handler_t)(struct irc_command command, void *userdata);

struct irc_message_parser *allocate_irc_message_parser(struct irc_lexer *lexer);

void deallocate_irc_message_parser(struct irc_message_parser *parser);

void deallocate_irc_message(struct irc_message *message);

bool irc_message_parser_can_parse(struct irc_message_parser *parser);

struct irc_message *irc_message_parser_parse(struct irc_message_parser *parser);

#endif
