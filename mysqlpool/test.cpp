#include <stdlib.h>
#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <unordered_map>
#include "mysqlconn.h"
#include "mysqlpool.h"
#include <glog/logging.h>

//#define __MYSQL__
#ifdef __MYSQL__

int main(){
	MYSQL mysql;
	mysql_init(&mysql);
	mysql_real_connect(&mysql, "192.168.73.111", "autodms", "yeelion", "Resource", 13306, NULL, 0);

	if(mysql_set_character_set(&mysql,"utf8")){
		std::cout << "set character error " << mysql_error(&mysql) << std::endl;
	}

	std::string sql = "select name,album,artist from Music limit 3";
	if(mysql_real_query(&mysql, sql.c_str(), sql.length())){
	}

	MYSQL_RES *res = mysql_store_result(&mysql);
	MYSQL_ROW row;
	while((row = mysql_fetch_row(res))){
		std::cout << row[0] << " --- " << row[1] << " --- " << row[2] << std::endl;
	}
}
#endif

#define __MYSQL_CONN__

#ifdef __MYSQL_CONN__

using namespace mysqlconn;

int select(){
	mysqlconnector *conn = new mysqlconnector("192.168.73.111",13306,"autodms", "yeelion", "Resource","utf8");
	/*if(conn->connect() == -1){
		std::cout << "connect error" << std::endl;
		return -1;
	}*/
	{
		std::string sql = "select id,name,album,artist from Music limit 3";
		int cnt = conn->execute(sql);
		std::cout << cnt << std::endl;
		if(cnt > 0){
			mysqlresult *res = conn->fetchall();
			for(auto iter=res->m_rows.begin();iter!=res->m_rows.end();iter++){
				int id = (*iter)->get_value("id");
				std::cout << id << std::endl;
				std::cout <<(int)(*iter)->get_value("id") << std::endl;
				//std::cout << (**iter)["id"] << "---" << (**iter)["name"] << "---" << (**iter)[1] << "---" << (**iter)[2] << std::endl;
			}
			res->free();
		}
	}
	std::cout << "abcd" << std::endl;
	//conn->close();
	delete conn;
	return 0;
}

int select_pool(){
	mysqlPool *pool = new mysqlPool(4,20,"192.168.253.161",3306,"autodms", "yeelion", "AutoDMS","utf8");
	std::shared_ptr<mysqlconnector> conn = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn1 = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn2 = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn3 = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn4 = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn5 = pool->get_connection();
	pool->print_num();
	std::shared_ptr<mysqlconnector> conn6 = pool->get_connection();
	pool->print_num();
	{
		std::shared_ptr<mysqlconnector> conn = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn1 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn2 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn3 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn4 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn5 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn6 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn8 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn9 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn10 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn11 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn12 = pool->get_connection();
		pool->print_num();
		std::shared_ptr<mysqlconnector> conn13 = pool->get_connection();
		pool->print_num();
	}
	pool->print_num();
	if(conn == nullptr){
		std::cout << "conn null" << std::endl;
	}
	std::string sql = "select id,m_name,m_album_id,m_artists from MusicSrc limit 3";
	int cnt = conn->execute(sql);
	std::cout << cnt << std::endl;
	if(cnt > 0){
		mysqlresult *res = conn->fetchall();
		for(auto iter=res->m_rows.begin();iter!=res->m_rows.end();iter++){
			if((*iter)->field_exists("id")){
				int id = (*iter)->get_value("id");
				std::cout << id << std::endl;
				std::cout <<(int)(*iter)->get_value("id") << std::endl;
			}
			//std::cout << (**iter)["id"] << "---" << (**iter)["name"] << "---" << (**iter)[1] << "---" << (**iter)[2] << std::endl;
		}
		/*mysqlrow *row;
		while((row = res->next())){
			std::cout << (int)row->get_value("id") << std::endl;
		}*/
		res->free();
	}
	//delete pool;
	return 0;
}

int main(){
	google::InitGoogleLogging("sql_pool");
	google::SetLogDestination(google::INFO, "/home/dmsMusic/huangfan/mysql/sql_pool");
	google::FlushLogFiles(google::INFO);
	select_pool();
	//select();
}

#endif
