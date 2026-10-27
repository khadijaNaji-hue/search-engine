
#include "crawler.hpp"
#include "slave.hpp"
#include "utlis.hpp"

#include <fstream>
#include <iostream>

crawler::crawler() : slave_pool(20) , read_urls_db("../files/urls_db.txt") , write_urls_db("../files/urls_db.txt", std::ios::app)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    for (int i = 0; i < 20; i++)
        slave_pool[i] = new slave(slave_work, tasks_mutx, cv, tasks, results, result_mutx, extracted_links, extracted_links_mutx);

    if (!read_urls_db.is_open())
        throw std::runtime_error("failed to open urls.db to read");
    if (!write_urls_db.is_open())
        throw std::runtime_error("failed to open urls.db to write");   
}

void crawler::crawl()
{   
    // open file
    // loop

    while(1)
    {
        get_links_db();
        cv.notify_all();

        std::unique_lock<std::mutex> taskQ_lk(tasks_mutx);
        cv.wait(taskQ_lk, [&](){ return tasks.empty(); }); // sleep until tasks ends
        taskQ_lk.unlock();
        write_links_db();
    }

    // fill tasks queue form urls_db
    // wait until tasks queue is empty
    // take extracted_links push this to urls_db after filteer dublicates
    //& results to indexer  v
}

void crawler::get_links_db()
{
	constexpr size_t BATCH = 100;
	read_urls_db.clear();

	std::vector<std::string> batch;
	std::string line;

	while(batch.size() < BATCH && std::getline(read_urls_db, line))
	{
		if (!line.empty())
			batch.push_back(std::move(line));

	}
	if (batch.empty())
		return;

	{
		std::lock_guard<std::mutex> lk(tasks_mutx);
		for (auto& url : batch)
			tasks.push(std::move(url));
	}
}

void crawler::write_links_db()
{
    std::vector<std::string> links;
    {
        std::lock_guard<std::mutex> lk(extracted_links_mutx);
        links.swap(extracted_links);
    }
    for(auto& link : links)
          write_urls_db << link << '\n';  
    
    write_urls_db.flush();

}

crawler::~crawler()
{
     for (int i = 0; i < 20; i++)
          delete slave_pool[i] ;
      read_urls_db.close();
      write_urls_db.close();  

      curl_global_cleanup();

}