#include "slave.hpp"
#include "utlis.hpp"
#include <curl/curl.h>

#include <algorithm>
#include <cstring>
#include <string>

// for extract_links
#include <iostream>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include <sstream>

void slave_work(CURL *&handle, std::queue<std::string> &tasks, std::mutex &tasks_mutx,
                std::vector<url_html> &results, std::mutex &result_mutx,
                std::vector<std::string> &extracted_links, std::mutex &extracted_links_mutx,
                std::condition_variable &cv)
{
    // global init & clean done by the engine
    CURLcode result;
    url_html html_url;
    std::string html, cur_base_url;

    setup_curlhadle(handle, html);

    while (1)
    {
        std::unique_lock<std::mutex> taskQ_lk(tasks_mutx);
        cv.wait(taskQ_lk, [&]()
                { return !(tasks.empty()); }); // sleep until tasks avalaIble
        cur_base_url = tasks.front();
        tasks.pop();
        if (tasks.empty())
            cv.notify_all();
        taskQ_lk.unlock();

        if (!get_request(handle, cur_base_url, html))
            continue;

        // auto x = std::this_thread::get_id();
        //  std::string sx = std::format("{}", x);
        // std::ostringstream oss;
        // oss << x;
        // std::string sx = oss.str();

        crawl_count++;
        // std::cout << " current count : " << crawl_count << "id : " << sx <<  "\n";

        char *eff = nullptr;
        curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &eff);
        std::string final_url = eff ? eff : cur_base_url; // copy it; curl owns eff
        std::set<std::string> links = extract_links(html, final_url);

        // std::set<std::string> links = extract_links(html, cur_base_url);

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
    }
}

/*bool get_request(CURL *handle, std::string base_url, std::string &html)
{
    html = "";
    char errbuf[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(handle, CURLOPT_URL, base_url.c_str());

    CURLcode result = curl_easy_perform(handle);
    if (result != CURLE_OK)
    {
        std::cout << "FAIL [" << base_url << "] code=" << (int)result
                  << " " << curl_easy_strerror(result)
                  << " | " << errbuf << '\n';
        html = "";
        return 0;
    }
    std::cout << "OK   [" << base_url << "] " << html.size() << " bytes\n";
    return 1;
}*/

bool get_request(CURL *handle, std::string base_url, std::string &html)
{
    html = "";
    curl_easy_setopt(handle, CURLOPT_URL, base_url.c_str());

    CURLcode result = curl_easy_perform(handle);
    if (result != CURLE_OK)
    {
        // std::cout << "Download failed: " << curl_easy_strerror(result) << '\n';
        html = "";
        return 0;
    }
    return 1;
}

void setup_curlhadle(CURL *&handle, std::string &buffer)
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
    // curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA | CURLSSLOPT_NO_REVOKE);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, +write_data_instr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L);    // setup 5 mins error
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L); // fail on 400 requests
    /************* Redirect ***********/
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);
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

        CURLU *base = curl_url();
        bool base_ok = curl_url_set(base, CURLUPART_URL, base_url.c_str(), 0) == CURLUE_OK;

        if (base_ok)
            for (int i = 0; i < nodes->nodeNr; ++i)
            {
                xmlChar *href = xmlNodeGetContent(nodes->nodeTab[i]);

                if (href)
                {
                    std::string out;
                    if (normalize_link(base, reinterpret_cast<const char *>(href), out))
                        links.insert(out);
                    xmlFree(href);
                }

                /*
                if (href)
                {                                                                         // static bool normalize_link(const CURLU *base, std::string href, std::string &out)
                    xmlChar *absolute_url = xmlBuildURI(href, BAD_CAST base_url.c_str()); // بتشوف لو الرابط مفروض يتنسب للبيز لينك وتربطه

                    if (absolute_url)
                    {
                        links.insert(
                            reinterpret_cast<const char *>(absolute_url));
                        xmlFree(absolute_url);
                    }

                    xmlFree(href);
                }
                     */
            }
        curl_url_cleanup(base);
    }

    if (result)
        xmlXPathFreeObject(result);

    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    return links;
}

// Core: base is already parsed (parse once per page, reuse for every href).
static bool normalize_link(const CURLU *base, std::string href, std::string &out)
{
    // 0) clean the raw text the way browsers do
    size_t first = href.find_first_not_of(" \t\r\n\f");
    if (first == std::string::npos)
        return false;
    size_t last = href.find_last_not_of(" \t\r\n\f");
    href = href.substr(first, last - first + 1);

    href.erase(std::remove_if(href.begin(), href.end(),
                              [](char c)
                              { return c == '\t' || c == '\r' || c == '\n'; }), href.end());

    if (href.empty() || href[0] == '#')
        return false;

    // encode inner spaces ourselves so we don't depend on curl's space handling
    for (size_t pos = 0; (pos = href.find(' ', pos)) != std::string::npos; pos += 3)
        href.replace(pos, 1, "%20");

    // 1) private copy of the base (a relative set modifies the handle)
    CURLU *u = curl_url_dup(base);
    if (!u)
        return false;

    char *scheme = nullptr;
    char *host = nullptr;
    char *result = nullptr;
    char *path {nullptr};
    bool ok = false;

    do
    {
        // 2) resolve href against the base
        if (curl_url_set(u, CURLUPART_URL, href.c_str(), CURLU_DISALLOW_USER | CURLU_URLENCODE) != CURLUE_OK)
            break;

        if (curl_url_get(u, CURLUPART_PATH, &path, 0) != CURLUE_OK)
			break;
         
        std::string p = path;
		curl_free(path);

		while (p.size() > 1 && p.back() == '/')
			p.pop_back();
		
        if (has_blocked_extension(p))
			break;

        curl_url_set(u, CURLUPART_PATH, p.c_str(), 0);
        // 3) http / https only
        if (curl_url_get(u, CURLUPART_SCHEME, &scheme, 0) != CURLUE_OK)
            break;
        if (std::strcmp(scheme, "http") != 0 && std::strcmp(scheme, "https") != 0)
            break;

        // 4) drop the #fragment
        curl_url_set(u, CURLUPART_FRAGMENT, nullptr, 0);

		
		

        // 5) lowercase the host (ASCII only)
        if (curl_url_get(u, CURLUPART_HOST, &host, 0) != CURLUE_OK)
            break;
        for (char *p = host; *p; ++p)
            if (*p >= 'A' && *p <= 'Z')
                *p += 'a' - 'A';

        size_t host_len = std::strlen(host); // <-- new
        while (host_len > 1 && host[host_len - 1] == '.')
            host[--host_len] = '\0'; // <-- new

        if (curl_url_set(u, CURLUPART_HOST, host, 0) != CURLUE_OK)
            break;

        // 6) final normalized URL
        if (curl_url_get(u, CURLUPART_URL, &result,
                         CURLU_NO_DEFAULT_PORT | CURLU_PUNYCODE) != CURLUE_OK)
            break;

        out = result;
        ok = true;
    } while (false);

    curl_free(scheme);
    curl_free(host);
    curl_free(result);
    curl_url_cleanup(u);
    return ok;
}

static bool has_blocked_extension(const std::string &path)
{
	size_t slash = path.find_last_of('/');
	std::string last = path.substr(slash == std::string::npos ? 0 : slash + 1);
	size_t dot = last.find_last_of('.');
	if (dot == std::string::npos)
		return false;

	std::string ext = last.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return std::tolower(c); });

	static const std::set<std::string> blocked = {
		"jpg","jpeg","png","gif","svg","webp","ico","bmp","avif",
		"pdf","doc","docx","xls","xlsx","ppt","pptx",
		"zip","gz","tar","rar","7z","iso","exe","dmg","apk",
		"mp3","mp4","avi","mov","webm","mkv","wav",
		"css","js","json","woff","woff2","ttf","eot"};
	return blocked.count(ext) > 0;
}