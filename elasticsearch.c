#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "elasticsearch.h"
#include "irc_message.h"
#include "str_helpers.h"

#define MAX_URL_BUFF 250

struct elasticsearch {
  char *index_name;
  char *document_name;
};

struct elasticsearch_connection {
  size_t base_url_len;
  CURL *curl;
  char *base_url;
  struct elasticsearch *search;
#ifdef DEBUG
  char curl_error_buffer[CURL_ERROR_SIZE];
#endif
};

static char *__append_index_document(struct elasticsearch_connection *connection, char *index, char *document, char *additional);
static size_t __curl_write(char *buffer, size_t size, size_t nmemb, void *userdata);

struct elasticsearch *allocate_elasticsearch(char *index_name, char *document_name) {
  struct elasticsearch *search;

  if ((search = (struct elasticsearch *) malloc(sizeof(struct elasticsearch))) == NULL) {
    return NULL;
  }

  search->index_name = (char *) calloc(strlen(index_name) + 1, sizeof(char));
  search->document_name = (char *) calloc(strlen(document_name) + 1, sizeof(char));

  if (search->index_name == NULL || search->document_name == NULL) {
    free(search->index_name);
    free(search->document_name);
    free(search);
    return NULL;
  }

  strcpy(search->index_name, index_name);
  strcpy(search->document_name, document_name);

  return search;
}

void deallocate_elasticsearch(struct elasticsearch *search) {
  free(search);
}

struct elasticsearch_connection *elasticsearch_connect(struct elasticsearch *search, char *url) {
  struct elasticsearch_connection *con;

  con = (struct elasticsearch_connection *) malloc(sizeof(struct elasticsearch_connection));
  if (con == NULL)
    return NULL;

  con->base_url_len = strlen(url);
  con->base_url = (char *) calloc(con->base_url_len + 1, sizeof(char));
  con->search = search;

  if (con->base_url == NULL) {
    free(con);
    return NULL;
  }

  strcpy(con->base_url, url);

  con->curl = curl_easy_init();
  if (con->curl == NULL) {
    free(con->base_url);
    free(con);
    return NULL;
  }

#ifdef DEBUG
  memset(con->curl_error_buffer, '\0', sizeof(char) * CURL_ERROR_SIZE);
#endif

  curl_easy_setopt(con->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(con->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(con->curl, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(con->curl, CURLOPT_WRITEFUNCTION, &__curl_write);
  curl_easy_setopt(con->curl, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt(con->curl, CURLOPT_VERBOSE, 0L);

#ifdef DEBUG
  curl_easy_setopt(con->curl, CURLOPT_ERRORBUFFER, &con->curl_error_buffer);
  curl_easy_setopt(con->curl, CURLOPT_VERBOSE, 1L);
#endif

  // TODO: Ping server first to see if it's up
  // TODO: Setup error flags incase there's an error for clients to see what's going on

  return con;
}

bool elasticsearch_disconnect(struct elasticsearch_connection *connection) {
  curl_easy_cleanup(connection->curl);
  free(connection->base_url);
  free(connection);

  return true;
}

bool elasticsearch_insert(struct elasticsearch_connection *connection, struct irc_message *msg) {
  size_t buffer_length;

  bool succeeded = true;
  char *url = NULL;
  char time_as_string[26];
  char *payload = NULL;
  char *username = NULL;
  char *channel = NULL;
  char *message = NULL;
  struct curl_slist *headers = NULL;
  struct tm *pt;

  char *format = "{"
                    "\"username\": \"%s\","
                    "\"message\": \"%s\","
                    "\"datetime_created\": \"%s\","
                    "\"channel\": \"%s\""
                 "}";

  url = __append_index_document(connection, connection->search->index_name, connection->search->document_name, NULL);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  pt = localtime(&msg->command->datetime_created);
  strftime(time_as_string, 26, "%Y-%m-%dT%H:%M:%S", pt);

  buffer_length = strlen(format) +
      26 +
      sanitize_str(msg->prefix->value, &username) +
      sanitize_str(msg->command->parameters[1]->value, &message) +
      sanitize_str(msg->command->parameters[0]->value, &channel);

  payload = (char *) calloc(buffer_length + 1, sizeof(char));
  if (payload == NULL) {
    succeeded = false;
    goto cleanup;
  }

  snprintf(payload, buffer_length, format,
      username,
      message,
      time_as_string,
      channel);

  curl_easy_setopt(connection->curl, CURLOPT_URL, url);
  curl_easy_setopt(connection->curl, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(connection->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(connection->curl, CURLOPT_POSTFIELDS, payload);

  // TODO: Add in curl opt for getting *just* the status code

  if (curl_easy_perform(connection->curl) != CURLE_OK) {
#ifdef DEBUG
    fprintf(stderr, "Error with curl\n%s\n", connection->curl_error_buffer);
#endif
  }

cleanup:

  curl_slist_free_all(headers);
  free(payload);
  free(url);
  free(username);
  free(channel);
  free(message);

  return succeeded;
}

static char * __append_index_document(struct elasticsearch_connection *connection, char *index, char *document, char *additional) {
  char *buffer = (char *) calloc(MAX_URL_BUFF, sizeof(char));
  if (buffer == NULL)
    return NULL;

  strcat(buffer, connection->base_url);
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

  return buffer;
}

static size_t __curl_write(char *buffer, size_t size, size_t nmemb, void *userdata) {
  // do nothing
  printf("%s\n", buffer);
  return size * nmemb;
}