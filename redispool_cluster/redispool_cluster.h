#ifndef _REDISPOOL_H_
#define _REDISPOOL_H_
#include <stdio.h>
#include <iostream>
#include "objectpool/objectpool.hpp"
#include <string>
#include "redisconn_cluster.hpp"


class redisClusterPool{
	public:
		redisClusterPool(size_t num, size_t max_num, std::string hosts,std::string auth=""):num(num),max_num(max_num),redishosts(hosts),auth(auth) {
			redispool_cluster = new objectPool<redisconn_cluster, std::string, std::string>;
			std::cout << redishosts << "---" << "---" << auth << std::endl;
			redispool_cluster->init(num, max_num, redishosts, auth);
		}
		~redisClusterPool() {
			std::cout << "redisPool desctruct before" << std::endl;
			if(redispool_cluster){
				delete redispool_cluster;
			}
		}
		std::shared_ptr<redisconn_cluster> get_connection(){
			return redispool_cluster->get(redishosts, auth);
		}
		void print_num(){
			redispool_cluster->print_num();
		}
	private:
		objectPool<redisconn_cluster, std::string, std::string> *redispool_cluster;
		std::string redishosts;
		std::string auth;
		size_t num;
		size_t max_num;
};
#endif
