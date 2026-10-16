#include "indexer.hpp"
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include <string>
#include <set>
#include <unordered_map>

#include <mutex>
#include <shared_mutex>

void indexer::add_to_index(const std::set<std::string> &tokens, const std::string &url)
{
    std::unique_lock lock(mtx);
    for (const auto &word : tokens)
    {
        if (word.size() > 2)
            inverted_index[word].insert(url);
    }
}

const std::set<std::string> *indexer::search_getLinks(const std::string &word) const
{
    std::shared_lock lock(mtx);  
    const auto it = inverted_index.find(word);
    if (it == inverted_index.end())
        return nullptr;

    return &(it->second);
}
