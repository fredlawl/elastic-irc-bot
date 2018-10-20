#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "elastic_search.h"
#include "irc_message.h"

#define MAX_URL_BUFF 250

//void escape_str(char *dest, char *src)
//{
//  *dest = 0;
//  while(*src)
//  {
//    switch(*src)
//    {
//      case '\n' : strcat(dest++, "\\n"); break;
//      case '\"' : strcat(dest++, "\\\""); break;
//      default:  *dest = *src;
//    }
//    *src++;
//    *dest++;
//    *dest = 0;
//  }
//}

struct elastic_search {
  char *index_name;
  char *document_name;
};

struct elastic_search_connection {
  size_t base_url_len;
  CURL *curl;
  char *base_url;
  struct elastic_search *search;
#ifdef DEBUG
  char curl_error_buffer[CURL_ERROR_SIZE];
#endif
};

static char *__append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional);
static size_t __curl_write(char *buffer, size_t size, size_t nmemb, void *userdata);

struct elastic_search *allocate_elastic_search(char *index_name, char *document_name) {
  struct elastic_search *search;

  if ((search = (struct elastic_search *) malloc(sizeof(struct elastic_search))) == NULL) {
    return NULL;
  }

  search->index_name = (char *) malloc(sizeof(char) * strlen(index_name));
  search->document_name = (char *) malloc(sizeof(char) * strlen(document_name));

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

void deallocate_elastic_search(struct elastic_search *search) {
  free(search);
}

struct elastic_search_connection *elastic_search_connect(struct elastic_search *search, char *url) {
  struct elastic_search_connection *con;

  con = (struct elastic_search_connection *) malloc(sizeof(struct elastic_search_connection));
  if (con == NULL)
    return NULL;

  con->base_url_len = strlen(url);
  con->base_url = (char *) malloc(sizeof(char) * con->base_url_len);
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
  memset(con->curl_error_buffer, '\0', CURL_ERROR_SIZE);
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

bool elastic_search_disconnect(struct elastic_search_connection *connection) {

  if (connection->curl == NULL) {
    return false;
  }

  curl_easy_cleanup(connection->curl);
  free(connection->base_url);
  free(connection);

  return true;
}

bool elastic_search_insert(struct elastic_search_connection *connection, struct irc_message *msg)
{
  char *url;
  char *time_as_string;
  char *payload;
  struct curl_slist *headers = NULL;
  size_t buffer_length;

  if (connection->curl == NULL)
    return false;


  char *format = "{"
                    "\"username\": \"%s\","
                    "\"message\": \"%s\","
                    "\"datetime_created\": \"%s\","
                    "\"channel\": \"%s\""
                  "}";

  time_as_string = asctime(gmtime(&msg->command->datetime_created));
  time_as_string[strlen(time_as_string) - 1] = '\0';

  buffer_length = strlen(format) + strlen(time_as_string) + msg->prefix->length +  msg->command->parameters[0]->length + msg->command->parameters[1]->length;

  payload = (char *) calloc(sizeof(char), buffer_length);

  if (payload == NULL)
    return false;

  snprintf(payload, buffer_length, format, msg->prefix->value, msg->command->parameters[1]->value, time_as_string, msg->command->parameters[0]->value);

  url = __append_index_document(connection, connection->search->index_name, connection->search->document_name, NULL);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(connection->curl, CURLOPT_URL, url);
  curl_easy_setopt(connection->curl, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(connection->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(connection->curl, CURLOPT_POSTFIELDS, payload);

  // TODO: Add in curl opt for getting *just* the status code

  if (curl_easy_perform(connection->curl) != CURLE_OK) {
#ifdef DEBUG
    fprintf(stderr, "Error with curl\n%s\n", connection->curl_error_buffer);
#endif
    curl_slist_free_all(headers);
    free(url);
    return false;
  }

  curl_slist_free_all(headers);
  free(url);

  return true;
}

static char * __append_index_document(struct elastic_search_connection *connection, char *index, char *document, char *additional) {
  char buffer[MAX_URL_BUFF];
  size_t total_url_len;
  char *built_url;

  memset(buffer, '\0', MAX_URL_BUFF);

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

  total_url_len = strlen(buffer);
  built_url = (char *) malloc(sizeof(char) * total_url_len);

  strncpy(built_url, buffer, total_url_len);

  return built_url;
}

static size_t __curl_write(char *buffer, size_t size, size_t nmemb, void *userdata) {
  // do nothing
  printf("%s\n", buffer);
  return size * nmemb;
}