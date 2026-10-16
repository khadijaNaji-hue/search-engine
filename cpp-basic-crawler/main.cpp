#include "indexer.hpp"
#include <iostream>
#include <string>
#include <thread>


void crawl(indexer& indexer);

int main()
{    
    indexer indexer ; 
    std::string word ;
    std::thread worker(crawl, std::ref(indexer));
    while(true)
    {

        std::cout<< "What u want to search ?" << std::endl;
        std::cin>> word ; 
        const std::set<std::string> *res_urls = indexer.search_getLinks(word);
        if (res_urls)
        {
            std::cout << "URLs for '" << word << "':\n";
            for (const auto &res_url : *res_urls)
            {
                std::cout << res_url << '\n';
            }
        }
        else
        {
            std::cout << "No URLs found for '" << word << "'\n";
        }
    }
    return 0;
}