    #pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>

struct url_html {
    std::string url;
    std::string html;
};

typedef void (*slave_work_ptr)(CURL*&handle, std::queue<std::string> &tasks ,std::mutex &tasks_mutx,
                                 std::vector<url_html> &results ,std::mutex &result_mutx,
                                 std::vector<std::string>& extracted_links ,std::mutex &extracted_links_mutx,
                                 std::condition_variable& cv);
class slave {
    private :
        CURL *handle ;
        std::thread worker;
    public :
        slave(slave_work_ptr work,
             std::mutex &tasks_mutx,
             std::condition_variable &cv,
             std::queue<std::string> &tasks,
             std::vector<url_html> &results,
             std::mutex &result_mutx,
             std::vector<std::string>& extracted_links,
             std::mutex &extracted_links_mutx
        );
        ~slave();


        friend class crawler ;
};