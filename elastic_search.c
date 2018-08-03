#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "elastic_search.h"

#ifndef DEBUG
#define DEBUG 1
#endif

#define MAX_URL_BUFF 250

struct elastic_search {
  CURL *curl;
  char *base_url;
  size_t base_url_len;
#ifdef DEBUG
  char curl_error_buffer[CURL_ERROR_SIZE];
#endif
};

struct elastic_search_connection {
  struct elastic_search *search;
};

static char *__append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional);
static size_t __curl_write(void *buffer, size_t size, size_t nmemb, void *userdata);

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

#ifdef DEBUG
  memset(search->curl_error_buffer, '\0', CURL_ERROR_SIZE);
#endif

  strcpy(search->base_url, url);

  curl_easy_setopt(search->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(search->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(search->curl, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(search->curl, CURLOPT_WRITEFUNCTION, &__curl_write);
  curl_easy_setopt(search->curl, CURLOPT_WRITEDATA, NULL);

#ifdef DEBUG
  curl_easy_setopt(search->curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(search->curl, CURLOPT_ERRORBUFFER, &search->curl_error_buffer);
  curl_easy_setopt(search->curl, CURLOPT_VERBOSE, 1L);
#endif

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
  struct curl_slist *headers = NULL;

  if (connection->search->curl == NULL)
    return false;

  url = __append_index_document(connection, index_name, NULL, NULL);

  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(connection->search->curl, CURLOPT_URL, url);
  curl_easy_setopt(connection->search->curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(connection->search->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(connection->search->curl, CURLOPT_POSTFIELDS, "{"
                                                                 "\"settings\": {"
                                                                 "\"index\": {"
                                                                 "\"number_of_shards\": \"5\","
                                                                 "\"number_of_replicas\": \"1\""
                                                                 "}"
                                                                 "},"
                                                                 "\"mappings\": {"
                                                                 "\"doctest3\": {"
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

  // TODO: Add in curl opt for getting *just* the status code


  if (curl_easy_perform(connection->search->curl) != CURLE_OK) {
#ifdef DEBUG
    fprintf(stderr, "Error with curl\n%s\n", connection->search->curl_error_buffer);
#endif
    curl_slist_free_all(headers);
    return false;
  }

  curl_slist_free_all(headers);

  return true;
}


bool elastic_search_insert(struct elastic_search_connection *connection, char *index_name, char *document_name) {
  char *url;
  struct curl_slist *headers = NULL;

  if (connection->search->curl == NULL)
    return false;

  url = __append_index_document(connection, index_name, document_name, NULL);

  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(connection->search->curl, CURLOPT_URL, url);
  curl_easy_setopt(connection->search->curl, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(connection->search->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(connection->search->curl, CURLOPT_POSTFIELDS, "{"
                                                                 "\"username\": \"flawlztest\","
                                                                 "\"message\": \"create a message\","
                                                                 "\"datetime_created\": \"2018-08-03T00:24:08+00:00\""
                                                                 "}");

  // TODO: Add in curl opt for getting *just* the status code

  if (curl_easy_perform(connection->search->curl) != CURLE_OK) {
#ifdef DEBUG
    fprintf(stderr, "Error with curl\n%s\n", connection->search->curl_error_buffer);
#endif
    curl_slist_free_all(headers);
    return false;
  }

  curl_slist_free_all(headers);

  return true;
}

static char * __append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional) {
  char buffer[MAX_URL_BUFF];
  size_t total_url_len;
  char *built_url;

  memset(buffer, '\0', MAX_URL_BUFF);

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

static size_t __curl_write(void *buffer, size_t size, size_t nmemb, void *userdata) {
  // do nothing
  printf("Called\n");
  return size * nmemb;
}