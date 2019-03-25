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
#include "config.h"
#include <glog/logging.h>
#include "mysqlpool/mysqlpool.h"
#include "redispool/redispool.h"
#include "objectpool/objectpool.hpp"
#include "threadpool/threadpooltask.hpp"
#include "utils.h"
#include <json/json.h>

class indexerServer{
	public:
		indexerServer():isstop(false){}
		~indexerServer(){
			delete mysql_pool;
			delete redis_pool;
		}
		int init(std::unordered_map<std::string, std::unordered_map<std::string,std::string> > config);
		int start(unsigned int type, unsigned int step);
		int stop();
	private:
		bool isstop;
		std::mutex isstop_mutex;
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

#endif
