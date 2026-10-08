cmake_minimum_required(VERSION 3.16)
project(basicCrawler CXX)
set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)

add_subdirectory(Libs/curl EXCLUDE_FROM_ALL)

add_executable(basicCrawler main.cpp)
target_link_libraries(basicCrawler PRIVATE CURL::libcurl)