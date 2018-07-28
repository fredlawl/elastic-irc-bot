#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "elastic_search.h"

struct elastic_search {
  CURL *curl;
  char *url;
  size_t url_len;
};

struct str_hlpr {
  char *str;
  size_t len;
};

static bool has_curl_init = false;

static char *__append_index_document(struct elastic_search *search, char *index, char *document);

struct elastic_search *allocate_elastic_search() {
  struct elastic_search *search;

  if ((search = (struct elastic_search *) malloc(sizeof(struct elastic_search))) == NULL) {
    return NULL;
  }

  if (!has_curl_init) {
    if (curl_global_init(CURL_GLOBAL_ALL) > 0)
      goto error;
    has_curl_init = true;
  }

  search->curl = NULL;

  return search;

error:
  free(search);
  return NULL;
}

void deallocate_elastic_search(struct elastic_search *search) {
  elastic_search_disconnect(search);

  // maybe later count # of instances
  if (has_curl_init) {
    curl_global_cleanup();
    has_curl_init = false;
  }

  free(search->url);
  free(search);
}

bool elastic_search_connect(struct elastic_search *search, char *url) {

  search->curl = curl_easy_init();
  if (!search->curl)
    return false;

  search->url_len = strlen(url);

  search->url = (char *) malloc(sizeof(char) * search->url_len);
  if (search->url == NULL) {
    curl_easy_cleanup(search->curl);
    return false;
  }

  strncpy(search->url, url, search->url_len);

  curl_easy_setopt(search->curl, CURLOPT_URL, search->url);
  curl_easy_setopt(search->curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(search->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(search->curl, CURLOPT_NOBODY, 1L);

  // TODO: Ping server first to see if it's up
  // TODO: Setup error flags incase there's an error for clients to see what's going on

  return true;
}

bool elastic_search_disconnect(struct elastic_search *search) {

  if (search->curl == NULL) {
    return false;
  }

  curl_easy_cleanup(search->curl);

  return true;
}

bool elastic_search_create_index_if_not_exists(struct elastic_search *search, char *index_name, char *document_name) {

  if (search->curl == NULL)
    return false;

  curl_easy_setopt(search->curl, CURLOPT_POST, 1L);
  // curl_easy_setopt(search->curl, CURLOPT_POSTFIELDS, ...);

  return true;
}

// ... something about a type ...

bool elastic_search_insert(struct elastic_search *search) {

  if (search->curl == NULL)
    return false;

  return true;
}

static char *__append_index_document(struct elastic_search *search, char *index, char *document) {
  char *built_url;
  size_t index_len, document_len;

  index_len = strlen(index);
  document_len = strlen(document);

  return built_url;
}