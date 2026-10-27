
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>

#include "slave.hpp"

slave::slave(slave_work_ptr work,
             std::mutex &tasks_mutx,
             std::condition_variable &cv,
             std::queue<std::string> &tasks,
             std::vector<url_html> &results,
             std::mutex &result_mutx,
             std::vector<std::string>& extracted_links,
             std::mutex &extracted_links_mutx): 

             worker(work , std::ref(handle) , 
                    std::ref(tasks) , std::ref(tasks_mutx), 
                    std::ref(results) ,std::ref(result_mutx), 
                    std::ref(extracted_links) , std::ref(extracted_links_mutx),
                    std::ref(cv))
{}

slave::~slave()
{
    worker.join();
    curl_easy_cleanup(handle);
    
}