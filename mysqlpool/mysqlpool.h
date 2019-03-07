#ifndef _MYSQLPOOL_H_
#define _MYSQLPOOL_H_
#include <stdio.h>
#include <iostream>
#include "objectpool/objectpool.hpp"
#include <string>
#include "mysqlconn.h"

using namespace mysqlconn;

class mysqlPool{
	public:
		mysqlPool(size_t num, size_t max_num, std::string host, unsigned int port, std::string user, std::string passwd, std::string db, std::string charset):num(num),max_num(max_num),host(host),port(port),user(user),passwd(passwd),db(db),charset(charset) {
			sqlpool = new objectPool<mysqlconnector, std::string, unsigned int, std::string, std::string, std::string, std::string>;
			sqlpool->init(num, max_num, std::forward<std::string>(host), std::forward<int>(port), std::forward<std::string>(user), std::forward<std::string>(passwd), std::forward<std::string>(db), std::forward<std::string>(charset));
		}
		~mysqlPool() {
			//std::cout << "mysqlPool desctruct before" << std::endl;
			if(sqlpool){
				delete sqlpool;
			}
		}
		std::shared_ptr<mysqlconnector> get_connection(){
			return sqlpool->get(host, port, user, passwd, db, charset);
		}
		void print_num(){
			sqlpool->print_num();
		}
	private:
		objectPool<mysqlconnector, std::string, unsigned int, std::string, std::string, std::string, std::string> *sqlpool;
		std::string host;
		std::string user;
		std::string passwd;
		unsigned int port;
		std::string charset;
		std::string db;
		size_t num;
		size_t max_num;
};
#endif
