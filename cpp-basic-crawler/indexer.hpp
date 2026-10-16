#include <string>
#include <set>
#include <unordered_map>
#include <shared_mutex>

class indexer
{
private:
    std::unordered_map<std::string, std::set<std::string>> inverted_index;
    mutable std::shared_mutex mtx;

public:
    void add_to_index(const std::set<std::string> &tokens, const std::string &url);

    const std::set<std::string>* search_getLinks(const std::string& word) const;
};