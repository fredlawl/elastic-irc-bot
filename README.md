# Elastic IRC Bot

### Dependencies

**Release**
1. pthreads
1. libcurl

**Debug**
1. [Criterion](https://github.com/Snaipe/Criterion#downloads)

### Install

`git submodule update --init --recursive`

`cmake CMakeLists.txt` (for Relase)
- or -
`cmake -D CMAKE_BUILD_TYPE=Debug CMakeLists.txt` (for Debug)

`make`