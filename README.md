# Elastic IRC Bot

### Dependencies

**Release**
1. pthreads
1. libcurl
   1. libcurl-dev (libcurl4-gnutls-dev or other flavors for Linux)
1. [Elasticsearch](https://www.elastic.co/guide/en/elasticsearch/reference/current/install-elasticsearch.html)

**Debug**
1. [Criterion](https://github.com/Snaipe/Criterion#downloads)

### Install

Under Linux, for production, I recommend running the application under a
specially created user to isolate the application from the rest of the system.

`git submodule update --init --recursive`

`cmake CMakeLists.txt` (for Release)
or
`cmake -D CMAKE_BUILD_TYPE=Debug CMakeLists.txt` (for Debug)

`make`

Binaries can be found in `./bin`

### Elasticsearch Index
TODO

### Configure Bot for IRC
TODO