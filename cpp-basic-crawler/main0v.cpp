#include "indexer.hpp"
#include <curl/curl.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

#include <set>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

size_t write_data_instr(char *buffer, size_t size, size_t nmemb, void *userptr)
{
    std::string *html = static_cast<std::string *>(userptr);
    size_t total_bytes = size * nmemb;
    html->append(buffer, total_bytes);

    // std::cout << "Received " << std::endl;

    return total_bytes;
}

std::set<std::string> extract_links(const std::string &html, const std::string &base_url);
std::set<std::string> extract_tokens(const std::string &html);

int main()
{
    std::string html, url, pagename;
    indexer indexer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *handle = curl_easy_init();
    if (!handle)
        std::cout << "error" << std::endl;

    curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_instr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &html);
    std::ifstream urlsfile("../files/urls.txt");
    if (!urlsfile.is_open())
    {
        std::cout << "Error opening file urls.txt" << std::endl;
        return 1;
    }
    std::ofstream urlsOutput("../files/urls.txt", std::ios::app);
    if (!urlsOutput.is_open())
    {
        std::cout << "Error opening urls.txt for writing\n";
        return 1;
    }
    int i = 1;
    CURLcode result;
    // urlsfile>>url
    while (std::getline(urlsfile, url))
    {
        html = "";
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        result = curl_easy_perform(handle);

        if (result != CURLE_OK)
        {
            std::cout << "Download failed: "
                      << curl_easy_strerror(result) << '\n';
            continue;
        }

        pagename = "../files/page" + std::to_string(i) + ".txt";
        std::ofstream pagehtmlfile(pagename);

        if (!pagehtmlfile.is_open())
        {
            std::cout << "Error opening file to write" << std::endl;
            return 1;
        }

        pagehtmlfile << html;
        pagehtmlfile.close();
        i++;

        std::set<std::string> links = extract_links(html, url);
        for (const std::string &link : links)
        {
            urlsOutput << link << '\n';
        }
        std::set<std::string> tokens = extract_tokens(html);

        indexer.add_to_index(tokens, url);

        const std::set<std::string> *res_urls = indexer.search_getLinks("cat");

        if (res_urls)
        {
            std::cout << "URLs for 'cat':\n";
            for (const auto &res_url : *res_urls)
            {
                std::cout << res_url << '\n';
            }
        }
        else
        {
            std::cout << "No URLs found for 'cat'\n";
        }
        break;
    }
    urlsOutput.close(); // write
    urlsfile.close();   // read

    std::cout << "Result: " << curl_easy_strerror(result) << std::endl;
    // std::cout << "HTML content:\n" << html << std::endl;
    curl_easy_cleanup(handle);
    curl_global_cleanup();
    return 0;
}
