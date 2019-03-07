#ifndef _REDISPOOL_H_
#define _REDISPOOL_H_
#include <stdio.h>
#include <iostream>
#include "objectpool/objectpool.hpp"
#include <string>
#include "redisconn.hpp"


class redisPool{
	public:
		redisPool(size_t num, size_t max_num, std::string host, unsigned int port):num(num),max_num(max_num),redishost(host),redisport(port) {
			redispool = new objectPool<redisconn, std::string, unsigned int>;
			std::cout << redishost << "---" << redisport << std::endl;
			redispool->init(num, max_num, redishost, redisport);
		}
		~redisPool() {
			std::cout << "redisPool desctruct before" << std::endl;
			if(redispool){
				delete redispool;
			}
		}
		std::shared_ptr<redisconn> get_connection(){
			return redispool->get(redishost, redisport);
		}
		void print_num(){
			redispool->print_num();
		}
	private:
		objectPool<redisconn, std::string, unsigned int> *redispool;
		std::string redishost;
		unsigned int redisport;
		size_t num;
		size_t max_num;
};
#endif
