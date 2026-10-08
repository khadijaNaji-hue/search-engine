#include <curl/curl.h>
#include <iostream>

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << curl_version() << "\n";
    curl_global_cleanup();
}