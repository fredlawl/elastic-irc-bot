
#ifndef ELASTIC_IRC_BOT_ELASTICSEARCH_H
#define ELASTIC_IRC_BOT_ELASTICSEARCH_H

#include <stdbool.h>

struct elasticsearch;
struct elasticsearch_connection;
struct irc_message;

struct elasticsearch *allocate_elasticsearch(char *index_name, char *document_name);

void deallocate_elasticsearch(struct elasticsearch *search);

struct elasticsearch_connection *elasticsearch_connect(struct elasticsearch *search, char *url);

bool elasticsearch_disconnect(struct elasticsearch_connection *connection);

bool elasticsearch_insert(struct elasticsearch_connection *connection, struct irc_message *msg);

#endif