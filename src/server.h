#ifndef _SEVER_H_
#define _SEVER_H_
#include <event2/util.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <vector>
#include <condition_variable>
#include "utils/config.h"
#include <glog/logging.h>
#include "mysqlpool/mysqlpool.h"
#include "redispool/redispool.h"
#include "objectpool/objectpool.hpp"
#include "threadpool/threadpooltask.hpp"
#include "utils/utils.h"
#include <json/json.h>

class httpServer{
	public:
		httpServer(std::string http_addr, int port):port(port),http_addr(http_addr){}
		~httpServer(){
			delete mysql_pool;
			delete thread_pool;
			delete redis_pool;
		}
		int init(std::unordered_map<std::string, std::unordered_map<std::string,std::string> > config);
		bool check_request_valid(std::string timestamps, std::string md5sum);
		void test_process(struct evhttp_request *request);
		void getinfo_process(struct evhttp_request *request);
		void other_process(struct evhttp_request *request);
		void echo_process(struct evhttp_request *request);
		static void http_generic_handler(struct evhttp_request *request, void *args);
		static void event_timeout(int fd, short events, void *args);
		int run_worker(struct evhttp_request *request);
		int start();
		int stop();
	private:
		bool not_full();
		bool not_empty();
		void get_task(struct evhttp_request **request);
		void add_task(struct evhttp_request *request);
	private:
		threadPool * thread_pool;
		int threadnum;
		unsigned int threadmax;
		std::string secret_key;
		std::string http_addr;
		int port;
		bool isstop;
		std::mutex isstop_mutex;
		std::queue<struct evhttp_request*> task_queue;
		std::mutex queue_mutex;
		int queue_max_size;
		std::vector<std::thread> threadpool;
		std::condition_variable_any m_not_empty;
		std::condition_variable_any m_not_full;
		mysqlPool *mysql_pool;
		std::string db_ip;
		std::string db_user;
		std::string db_name;
		std::string db_passwd;
		std::string db_charset;
		unsigned int db_port;
		unsigned int db_pool_num;
		unsigned int db_pool_maxnum;
		redisPool *redis_pool;
		std::string redis_ip;
		unsigned int redis_port;
		unsigned int redis_pool_num;
		unsigned int redis_pool_maxnum;
		std::string pic_artist;
		std::string pic_album;
		std::string redis_auth;
		std::unordered_map<std::string, std::unordered_map<std::string,std::string> > server_config;
};

typedef void (httpServer::*server_func)(struct evhttp_request *);

#endif
