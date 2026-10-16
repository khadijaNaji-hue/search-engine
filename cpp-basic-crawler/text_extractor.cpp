#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <string>


#include <set>
#include <iostream>
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include <unordered_map>

static void collect_text(xmlNode* node , std::set<std::string>& tokens)
{
    std::string text;
    //std::set<std::string> tokens;
    std::string word;

    for (xmlNode* current = node ; current != nullptr ; current = current->next)
    {
        if (current->type == XML_ELEMENT_NODE)
        {
            // Skip script and style contents
            if (xmlStrcasecmp(current->name, BAD_CAST "script") == 0 || xmlStrcasecmp(current->name, BAD_CAST "style") == 0)
            {
                continue;
            }
        }
        if (current->type == XML_TEXT_NODE || current->type == XML_CDATA_SECTION_NODE)
        {
            if (current->content != nullptr)
            {
                text += reinterpret_cast<const char*>(current->content);
                text += ' ';
            }
        }
        else
        {
            collect_text(current->children, tokens);
        }
    }
    
    for(char c : text)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
        {
            word += std::tolower(static_cast<unsigned char>(c));
        }
        else
        {
            if (!word.empty())
            {
                tokens.insert(word);
                word.clear();
            }
        }
    }
    if (!word.empty())
                tokens.insert(word);
}

std::set<std::string> extract_tokens(const std::string& html)
{
    std::string text;
    std::set<std::string> tokens;

    htmlDocPtr doc = htmlReadMemory(
        html.data(),
        static_cast<int>(html.size()),
        nullptr,
        nullptr,
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET
    );

    if (doc == nullptr)
        return tokens;

    xmlNode* root = xmlDocGetRootElement(doc);

    if (root != nullptr)
        collect_text(root, tokens);

    xmlFreeDoc(doc);

    return tokens;
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

    const char *query = "//a[@href"
        " and not(starts-with(normalize-space(@href), '#'))"
        " and not(starts-with(normalize-space(@href), '?'))"
        " and not(starts-with(normalize-space(@href), 'mailto:'))"
        " and not(starts-with(normalize-space(@href), 'tel:'))"
        " and not(starts-with(normalize-space(@href), 'javascript:'))"
        " and not(contains(@href, 'add-to-cart'))"
        "]/@href";

    xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *) query, context);

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