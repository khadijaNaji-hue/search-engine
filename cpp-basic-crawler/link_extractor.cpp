#include <string>
#include <set>
#include <iostream>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

std::set<std::string> extract_links( const std::string& html, const std::string& base_url)
{
    std::set<std::string> links;

    htmlDocPtr doc = htmlReadMemory(
        html.c_str(),
        html.size(),
        base_url.c_str(),
        nullptr,
        HTML_PARSE_NOERROR
    );

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

    xmlXPathObjectPtr result = xmlXPathEvalExpression(  BAD_CAST "//a/@href", context );

    if (result && result->nodesetval)
    {
        xmlNodeSetPtr nodes = result->nodesetval;

        for (int i = 0; i < nodes->nodeNr; ++i)
        {
            xmlChar* href = xmlNodeGetContent(nodes->nodeTab[i]);

            if (href)
            {
                xmlChar* absolute_url =  xmlBuildURI(href, BAD_CAST base_url.c_str()); // بتشوف لو الرابط مفروض يتنسب للبيز لينك وتربطه

                if (absolute_url)
                {
                    links.insert(
                        reinterpret_cast<const char*>(absolute_url)
                    );
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