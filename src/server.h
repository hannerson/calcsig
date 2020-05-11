#ifndef _SEVER_H_
#define _SEVER_H_
#include <event2/util.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <evhttp.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include "utils/config.h"
#include "glog/logging.h"
#include "threadpool/threadpool_98.hpp"
#include "utils/utils.h"
#include "json/json.h"

class httpServer{
	public:
		httpServer(std::string http_addr, int port):port(port),http_addr(http_addr){}
		~httpServer(){
			delete thread_pool;
		}
		int init(std::map<std::string, std::map<std::string,std::string> > config);
		bool check_request_valid(std::string timestamps, std::string md5sum);
		static void test_process(struct evhttp_request *request, void *args);
		static void calcsig_process(struct evhttp_request *request, void *args);
		static void other_process(struct evhttp_request *request, void *args);
		static void event_timeout(int fd, short events, void *args);
		int start();
		void stop();
		bool stop_status(){
			return isstop;
		}
	private:
		class threadpool_98 *thread_pool;
		int threadnum;
		int threadmax;
		int queue_max_size;
		std::string secret_key;
		std::string http_addr;
		int port;
		bool isstop;
	public:
		pthread_mutex_t init_lock;
		pthread_cond_t init_cond;
		std::string my_ip;
		int init_count;
		std::map<std::string, std::map<std::string,std::string> > server_config;
};

typedef void (httpServer::*server_func)(struct evhttp_request *);

#endif
