// function take file and string and get urls from the string and write them to the file

/*std::vector<std::string> extractLinks(const std::string &html)
{
    std::vector<std::string> urls;
    size_t pos = 0;

    while ((pos = html.find("href=", pos)) != std::string::npos)
    {
        pos += 5;

        char quote = html[pos];

        if (quote != '"' && quote != '\'')
            continue;

        size_t end = html.find(quote, pos + 1);

        if (end == std::string::npos)
            break;

        urls.push_back(html.substr(pos + 1, end - pos - 1));

        pos = end + 1;
    }

    return urls;
}*/


/*#include <curl/curl.h>
#include <iostream>
#include <string>

size_t write_data_instr(char *buffer, size_t size, size_t nmemb, void* userptr)
{
    std::string *html = static_cast<std::string*>(userptr);
    int total_bytes = size * nmemb;
    html->append(buffer, total_bytes);

    //std::cout << "Received " << std::endl;

    return total_bytes;
}
int main()
{
    std::string html;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *handle = curl_easy_init();
    if (!handle)
        std::cout << "error"<< std::endl;

    curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_instr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &html);

    curl_easy_setopt(handle, CURLOPT_URL, "https://www.kaggle.com/learn/python");

    //curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    CURLcode result = curl_easy_perform(handle);

    //std::cout << "Result: " << curl_easy_strerror(result) << std::endl;

    std::cout << "HTML content:\n" << html << std::endl;
    curl_global_cleanup();
    return 0;
}*/

// test function
/**
 * int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << curl_version() << "\n";

    std::string url  = "https://example.com";
    std::string html = "<html><body><a href=\"/a\">A</a><a href=\"/b\">B</a></body></html>";

    htmlDocPtr doc = htmlReadMemory(html.c_str(), (int)html.size(), url.c_str(), nullptr,
                                    HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        std::cerr << "parse failed\n";
        curl_global_cleanup();
        return 1;
    }

    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr res = xmlXPathEvalExpression((const xmlChar*)"//a/@href", ctx);

    if (res && res->nodesetval) {
        for (int i = 0; i < res->nodesetval->nodeNr; ++i) {
            xmlChar* href = xmlNodeGetContent(res->nodesetval->nodeTab[i]);
            std::cout << "link: " << (const char*)href << "\n";
            xmlFree(href);
        }
    }

    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    curl_global_cleanup();
}
 */
