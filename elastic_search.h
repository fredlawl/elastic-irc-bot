
#ifndef ELASTIC_IRC_BOT_ELASTIC_SEARCH_H
#define ELASTIC_IRC_BOT_ELASTIC_SEARCH_H

#include <stdbool.h>

struct elastic_search;
struct elastic_search_connection;

struct elastic_search *allocate_elastic_search();

void deallocate_elastic_search(struct elastic_search *search);

struct elastic_search_connection *elastic_search_connect(struct elastic_search *search, char *url);

bool elastic_search_disconnect(struct elastic_search_connection *connection);

bool elastic_search_create_index_if_not_exists(struct elastic_search_connection *connection, char *index_name, char *document_name);

// ... something about a type ...

bool elastic_search_insert(struct elastic_search_connection *connection, char *index_name, char *document_name);

#endif