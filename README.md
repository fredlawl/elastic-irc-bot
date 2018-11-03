# Elastic IRC Bot

### Build Dependencies

**Release**
1. pthreads
1. libcurl
   1. libcurl-dev (libcurl4-gnutls-dev or other flavors for Linux)
1. [Elasticsearch](https://www.elastic.co/guide/en/elasticsearch/reference/current/install-elasticsearch.html)

**Debug**
1. [Criterion](https://github.com/Snaipe/Criterion#downloads)

### Install

#### Configure Bot for IRC
**settings.h** contains the following default settings:

```
#define IRC_SERVER_IP "38.229.70.22"
#define IRC_SERVER_PORT 6667
#define IRC_CHANNEL "#flawlztest"
#define IRC_USER_SERVER_IP "127.0.0.1"
#define IRC_USER_PASS "somepassword"
#define IRC_USER_NICK "elasticirc"
#define IRC_USER_REALNAME "Elastic IRC Bot"
#define ELASTICSEARCH_BASE_URL "http://localhost:9200"
```

Replace these with your own settings.

**NOTE:** Each time **settings.h** is modified, the source must be
recompiled.

#### Compile
Under Linux, for production, I recommend running the application under a
specially created user to isolate the application from the rest of the system.

Run the following commands to compile the project.

`git submodule update --init --recursive`

`cmake CMakeLists.txt` (for Release)

or

`cmake -D CMAKE_BUILD_TYPE=Debug CMakeLists.txt` (for Debug)

`make`

Binaries can be found in `./bin`

#### Elasticsearch Index
Run the following command to establish the index with Elasticsearch. The
program does not currently stray away from this mapping. The index is
named **elastic_irc_bot** and the document is named **privmsg**.

```
curl -X PUT -H "Content-Type: application/json" -d '{"settings": {"index": {"number_of_shards": "5", "number_of_replicas": "1"} }, "mappings": {"privmsg": {"properties": {"username": {"type": "keyword"}, "message": {"type": "text"}, "datetime_created": {"type": "date", "format": "strict_date_hour_minute_second"}, "channel": {"type": "keyword"} } } } }' http://localhost:9200/elastic_irc_bot
```

Calling: `curl -X GET http://localhost:9200/_cat/indices` should show that
the index was correctly created.

## [License](./LICENSE)
