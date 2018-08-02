#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "elastic_search.h"

struct elastic_search {
  CURL *curl;
  char *base_url;
  size_t base_url_len;
  char curl_error_buffer[CURL_ERROR_SIZE];
};

struct elastic_search_connection {
  struct elastic_search *search;
};

static char *__append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional);

struct elastic_search *allocate_elastic_search() {
  struct elastic_search *search;

  if ((search = (struct elastic_search *) malloc(sizeof(struct elastic_search))) == NULL) {
    return NULL;
  }

  search->curl = NULL;

  return search;
}

void deallocate_elastic_search(struct elastic_search *search) {
  free(search->base_url);
  free(search);
}

struct elastic_search_connection *elastic_search_connect(struct elastic_search *search, char *url) {
  struct elastic_search_connection *con;

  search->curl = curl_easy_init();
  if (search->curl == NULL) {
    return NULL;
  }

  con = (struct elastic_search_connection *) malloc(sizeof(struct elastic_search_connection));
  search->base_url_len = strlen(url);
  search->base_url = (char *) malloc(sizeof(char) * search->base_url_len);

  if (con == NULL || search->base_url == NULL) {
    free(con);
    free(search->base_url);
    curl_easy_cleanup(search->curl);
    return NULL;
  }

  memset(search->curl_error_buffer, '\0', CURL_ERROR_SIZE);

  strncpy(search->base_url, url, search->base_url_len);

  curl_easy_setopt(search->curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(search->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(search->curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(search->curl, CURLOPT_ERRORBUFFER, &search->curl_error_buffer);
  curl_easy_setopt(search->curl, CURLOPT_VERBOSE, 0L);

  con->search = search;

  // TODO: Ping server first to see if it's up
  // TODO: Setup error flags incase there's an error for clients to see what's going on

  return con;
}

bool elastic_search_disconnect(struct elastic_search_connection *connection) {

  if (connection->search->curl == NULL) {
    return false;
  }

  curl_easy_cleanup(connection->search->curl);
  free(connection);

  return true;
}

bool elastic_search_create_index_if_not_exists(struct elastic_search_connection *connection, char *index_name, char *document_name) {
  char *url;

  if (connection->search->curl == NULL)
    return false;

  url = __append_index_document(connection, index_name, NULL, NULL);
  curl_easy_setopt(connection->search->curl, CURLOPT_URL, url);
  curl_easy_setopt(connection->search->curl, CURLOPT_PUT, 1L);
  curl_easy_setopt(connection->search->curl, CURLOPT_POSTFIELDS, "{"
                                                                 "\"settings\": {"
                                                                 "\"index\": {"
                                                                 "\"number_of_shards\": \"5\","
                                                                 "\"number_of_replicas\": \"1\""
                                                                 "}"
                                                                 "},"
                                                                 "\"mappings\": {"
                                                                 "\"docnametest\": {"
                                                                 "\"properties\": {"
                                                                 "\"username\": {"
                                                                 "\"type\": \"text\""
                                                                 "},"
                                                                 "\"message\": {"
                                                                 "\"type\": \"text\""
                                                                 "},"
                                                                 "\"datetime_created\": {"
                                                                 "\"type\": \"date\","
                                                                 "\"format\": \"strict_date_optional_time||epoch_millis\""
                                                                 "}"
                                                                 "}"
                                                                 "}"
                                                                 "}"
                                                                 "}");

  printf("test");
  if (curl_easy_perform(connection->search->curl) != CURLE_OK) {
    fprintf(stderr, "Error with curl\n%s\n", connection->search->curl_error_buffer);
  }

  return true;
}

// ... something about a type ...

bool elastic_search_insert(struct elastic_search_connection *connection, char *index_name, char *document_name) {
  char *url;

  if (connection->search->curl == NULL)
    return false;

  url = __append_index_document(connection, index_name, document_name, NULL);
  curl_easy_setopt(connection->search->curl, CURLOPT_URL, url);

  return true;
}

static char * __append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional) {
  char buffer[250];
  size_t total_url_len;
  char *built_url;

  memset(buffer, '\0', 250);

  strcat(buffer, connection->search->base_url);
  strcat(buffer, "/");
  strcat(buffer, index);

  if (document != NULL  && strlen(document) > 0) {
    strcat(buffer, "/");
    strcat(buffer, document);
  }

  if (additional != NULL && strlen(additional) > 0) {
    strcat(buffer, "/");
    strcat(buffer, additional);
  }

  total_url_len = strlen(buffer);
  built_url = (char *) malloc(sizeof(char) * total_url_len);

  strncpy(built_url, buffer, total_url_len);

  return built_url;
}