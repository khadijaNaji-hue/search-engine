#include <mutex>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include "slave.hpp"

class crawler
{
private:
    std::queue<std::string> tasks;
    std::mutex tasks_mutx;
    std::vector<url_html> results;
    std::mutex result_mutx;
    std::vector<std::string> extracted_links;
    std::mutex extracted_links_mutx;
 
    std::condition_variable cv;
    std::vector<slave*> slave_pool;

    std::ifstream read_urls_db;
    std::ofstream write_urls_db;
public:
    crawler();

    void crawl();
    void get_links_db();
    void write_links_db();
    slave_work_ptr dowork1; 
    ~crawler();

};
