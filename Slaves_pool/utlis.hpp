#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
#include <set>

#include "slave.hpp"

void slave_work(CURL*&handle , std::queue<std::string> &tasks ,std::mutex &tasks_mutx,
                                 std::vector<url_html> &results ,std::mutex &result_mutx,
                                 std::vector<std::string>& extracted_links ,std::mutex &extracted_links_mutx,
                                 std::condition_variable& cv);


bool get_request(CURL* handle , std::string base_url , std::string &html );

std::set<std::string> extract_links( const std::string& html, const std::string& base_url);

void setup_curlhadle(CURL * &handle, std::string &buffer);