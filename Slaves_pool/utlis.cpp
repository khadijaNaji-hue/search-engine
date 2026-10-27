#include "slave.hpp"
#include "utlis.hpp"
#include <curl/curl.h>

// for extract_links
#include <iostream>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

void slave_work(CURL *&handle, std::queue<std::string> &tasks, std::mutex &tasks_mutx,
                std::vector<url_html> &results, std::mutex &result_mutx,
                std::vector<std::string> &extracted_links, std::mutex &extracted_links_mutx,
                std::condition_variable &cv)
{
    // global init & clean done by the engine
    CURLcode result;
    url_html html_url;
    std::string html, cur_base_url;

    setup_curlhadle(handle , html);

    while (1)
    {
        std::unique_lock<std::mutex> taskQ_lk(tasks_mutx);
        cv.wait(taskQ_lk, [&]() { return !(tasks.empty()); }); // sleep until tasks avalaIble
        cur_base_url = tasks.front();
        tasks.pop();
        taskQ_lk.unlock();

        if (!get_request( handle, cur_base_url, html))
            continue;

        std::set<std::string> links = extract_links(html, cur_base_url);
        
        std::unique_lock<std::mutex> links_lk(extracted_links_mutx);
        for (const std::string &link : links)
        {
            extracted_links.push_back(link);
        }
        links_lk.unlock();

        html_url.html = html;
        html_url.url = cur_base_url;

        std::unique_lock<std::mutex> result_lk(result_mutx);
        results.push_back(html_url);
        result_lk.unlock();

        if(tasks.empty())
           cv.notify_all();

    }
}

bool get_request(CURL *handle, std::string base_url, std::string &html)
{
    html = "";
    curl_easy_setopt(handle, CURLOPT_URL, base_url.c_str());

    CURLcode result = curl_easy_perform(handle);
    if (result != CURLE_OK)
    {
        std::cout << "Download failed: "
                  << curl_easy_strerror(result) << '\n';
        html = "";
        return 0;
    }
    return 1;
}

void setup_curlhadle(CURL * &handle, std::string &buffer)
{
    handle = curl_easy_init();
    if (!handle)
        throw std::runtime_error("failed ti init curl handle");

    auto write_data_instr = [](char *buffer, size_t size, size_t nmemb, void *userptr)
    {
        std::string *html = static_cast<std::string *>(userptr);
        size_t total_bytes = size * nmemb;
        html->append(buffer, total_bytes);

        return total_bytes;
    };

    curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, +write_data_instr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 300L); // setup 5 mins error
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L); // fail on 400 requests
}
std::set<std::string> extract_links(const std::string &html, const std::string &base_url)
{
    std::set<std::string> links;
    htmlDocPtr doc = htmlReadMemory(
        html.c_str(),
        html.size(),
        base_url.c_str(),
        nullptr,
        HTML_PARSE_NOERROR);

    if (doc == nullptr)
    {
        std::cerr << "Failed to parse HTML\n";
        return links;
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);

    if (context == nullptr)
    {
        xmlFreeDoc(doc);
        return links;
    }

    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "//a/@href", context);

    if (result && result->nodesetval)
    {
        xmlNodeSetPtr nodes = result->nodesetval;

        for (int i = 0; i < nodes->nodeNr; ++i)
        {
            xmlChar *href = xmlNodeGetContent(nodes->nodeTab[i]);

            if (href)
            {
                xmlChar *absolute_url = xmlBuildURI(href, BAD_CAST base_url.c_str()); // بتشوف لو الرابط مفروض يتنسب للبيز لينك وتربطه

                if (absolute_url)
                {
                    links.insert(
                        reinterpret_cast<const char *>(absolute_url));
                    xmlFree(absolute_url);
                }

                xmlFree(href);
            }
        }
    }

    if (result)
        xmlXPathFreeObject(result);

    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    return links;
}