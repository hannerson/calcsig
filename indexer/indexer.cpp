#include "indexer.h"
#include <iostream>
#include <thread>
#include <mutex>
#include "config.h"
#include <openssl/md5.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <unistd.h>
#include <stdlib.h>
#include <exception>
#include <strings.h>
#include <string>
#include <set>
#include <event.h>
#include <json/json.h>


int indexerServer::init(std::unordered_map<std::string,std::unordered_map<std::string,std::string> > config){
	try{
		server_config = config;
		std::unordered_map<std::string,std::string> config_common = config["common"];

		if(config_common.count("pic_artist") > 0){
			pic_artist = config_common["pic_artist"];
		}else{
			std::cout << "config pic_artist not exists" << std::endl;
			return -1;
		}

		if(config_common.count("pic_album") > 0){
			pic_album = config_common["pic_album"];
		}else{
			std::cout << "config pic_album not exists" << std::endl;
			return -1;
		}

		std::unordered_map<std::string,std::string> config_mysql = config["mysql"];
		if(config_mysql.count("dbres_ip") > 0){
			db_ip = config_mysql["dbres_ip"];
		}else{
			std::cout << "config dbres_ip not exists" << std::endl;
			return -1;
		}

		if(config_mysql.count("dbres_user") > 0){
			db_user = config_mysql["dbres_user"];
		}else{
			std::cout << "config dbres_user not exists" << std::endl;
			return -1;
		}

		if(config_mysql.count("dbres_passwd") > 0){
			db_passwd = config_mysql["dbres_passwd"];
		}else{
			std::cout << "config dbres_passwd not exists" << std::endl;
			return -1;
		}

		if(config_mysql.count("dbres_name") > 0){
			db_name = config_mysql["dbres_name"];
		}else{
			std::cout << "config dbres_name not exists" << std::endl;
			return -1;
		}

		if(config_mysql.count("dbres_charset") > 0){
			db_charset = config_mysql["dbres_charset"];
		}else{
			return -1;
		}

		if(config_mysql.count("dbres_port") > 0){
			db_port = atoi(config_mysql["dbres_port"].c_str());
		}else{
			return -1;
		}

		if(config_mysql.count("db_pool_num") > 0){
			db_pool_num = atoi(config_mysql["db_pool_num"].c_str());
		}else{
			return -1;
		}

		if(config_mysql.count("db_pool_maxnum") > 0){
			db_pool_maxnum = atoi(config_mysql["db_pool_maxnum"].c_str());
		}else{
			return -1;
		}

		std::unordered_map<std::string,std::string> config_redis = config["redis"];
		if(config_redis.count("redis_ip") > 0){
			redis_ip = config_redis["redis_ip"];
		}else{
			return -1;
		}

		if(config_redis.count("redis_port") > 0){
			redis_port = atoi(config_redis["redis_port"].c_str());
		}else{
			return -1;
		}

		if(config_redis.count("redis_auth") > 0){
			redis_auth = config_redis["redis_auth"].c_str();
		}else{
			return -1;
		}

		if(config_redis.count("pool_num") > 0){
			redis_pool_num = atoi(config_redis["pool_num"].c_str());
		}else{
			return -1;
		}

		if(config_redis.count("pool_maxnum") > 0){
			redis_pool_maxnum = atoi(config_redis["pool_maxnum"].c_str());
		}else{
			return -1;
		}
		//init mysql pool
		mysql_pool = new mysqlPool(db_pool_num,db_pool_maxnum,db_ip,db_port,db_user,db_passwd,db_name,db_charset);
		if(redis_auth == ""){
			redis_pool = new redisPool(redis_pool_num, redis_pool_maxnum, redis_ip, redis_port);
		}else{
			redis_pool = new redisPool(redis_pool_num, redis_pool_maxnum, redis_ip, redis_port,redis_auth);
		}
	}
	catch(char * e){
	}
	return 0;
}

int indexerServer::stop(){
	std::lock_guard<std::mutex> lock(isstop_mutex);
	isstop = true;
	return 0;
}

int indexerServer::start( unsigned int type,  unsigned int step){
	while(!isstop){
		std::string type_table;
		if(type == 0){
			type_table = "Music";
		}else if(type == 1){
			type_table = "Album";
		}else if(type == 2){
			type_table = "Artist";
		}else{
			type_table = "Music";
		}
		//get max id
		std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
		unsigned int max_id = 0;
		unsigned int begin_num = 0, end_num = step;
		std::ostringstream s_sql;
		std::ostringstream hashKey;
		s_sql << "select max(id) from " << type_table;
		std::cout << s_sql.str() << std::endl;
		std::string sql = s_sql.str();
		int cnt = sql_conn->execute(sql);
		std::cout << "count:" << cnt << std::endl;
		if(cnt > 0){
			mysqlresult *res = sql_conn->fetchall();
			max_id = res->m_rows[0]->get_value("max(id)");
			res->free();
		}
		//check in redis
		std::shared_ptr<redisconn> redis_conn = redis_pool->get_connection();
		unsigned int audioSourceId = 0;
		
		while( begin_num < max_id ){
			//check in mysql
			//std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
			//std::ostringstream s_sql;
			s_sql.str("");
			s_sql << "select * from " << type_table << " where id>=" << begin_num << " and id<" << end_num;
			std::cout << s_sql.str() << std::endl;
			std::string sql = s_sql.str();
			int cnt = sql_conn->execute(sql);
			std::cout << "count:" << cnt << std::endl;
			if(cnt > 0){
				mysqlresult *res = sql_conn->fetchall();
				std::cout << "XXXXXX" << std::endl;
				if(res != nullptr){
					for(auto iter=res->m_rows.begin();iter!=res->m_rows.end();iter++){
						std::cout << "XXXXXX" << std::endl;
						std::unordered_map<std::string,std::string> hashData;
						unsigned int rid = (*iter)->get_value("id");
						audioSourceId = (*iter)->get_value("audiosourceid");
						std::cout <<(int)(*iter)->get_value("id") << std::endl;
						if(server_config.count(type_table) == 0){
							//error
						}
						std::unordered_map<std::string,std::string> table_config = server_config[type_table];
						for(auto it=table_config.begin(); it!=table_config.end(); ++it){
							//std::cout << it->second << "---" << it->first << std::endl;
							std::string val = (*iter)->get_value(it->first);
							hashData.emplace(it->second,val);
						}
						//set redis data
						hashKey.str("");
						hashKey << type_table << ":" << rid;
						std::cout << hashKey.str() << std::endl;
						if(!redis_conn->setHash(hashKey.str(), hashData)){
							std::cout << "set hash failed : " << rid << std::endl;
						}
						if( type == 0 || type == 1){
							//pay info
							//check in mysql pay info
							//std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
							//std::ostringstream s_sql;
							s_sql.str("");
							s_sql << "select * from MusicPay where policy!=\"none\" and type=" << type << " and rid=" << rid;
							std::cout << s_sql.str() << std::endl;
							std::string sql = s_sql.str();
							int cnt = sql_conn->execute(sql);
							std::cout << "count:" << cnt << std::endl;
							if(cnt > 0){
								mysqlresult *res_pay = sql_conn->fetchall();
								if(res_pay != nullptr){
									for(auto iter=res_pay->m_rows.begin();iter!=res_pay->m_rows.end();iter++){
										std::unordered_map<std::string,std::string> hashData;
										int id = (*iter)->get_value("id");
										std::cout <<(int)(*iter)->get_value("id") << std::endl;
										std::string policy = (*iter)->get_value("policy");
										if(server_config.count("Payinfo") == 0){
											//error
										}
										std::unordered_map<std::string,std::string> table_config = server_config["Payinfo"];
										for(auto it=table_config.begin(); it!=table_config.end(); ++it){
											//std::cout << it->second << "---" << it->first << std::endl;
											std::string val = (*iter)->get_value(it->first);
											hashData.emplace(it->second,val);
										}
										//set redis data
										hashKey.str("");
										hashKey << type_table << ":" << policy << ":" << rid;
										std::cout << hashKey.str() << std::endl;
										if(!redis_conn->setHash(hashKey.str(), hashData)){
										}
										//set redis
										hashKey.str("");
										hashKey << type_table << ":Pay:" << rid;
										std::cout << hashKey.str() << std::endl;
										if(!redis_conn->setAdd(hashKey.str(), policy)){
										}
										if(!redis_conn->setDel(hashKey.str(), "none")){
										}
									}
									res_pay->free();
								}else{
									std::cout << "sql fetch res pay null" << std::endl;
								}
							}else{
								//set redis null
								hashKey.str("");
								hashKey << type_table << ":Pay:" << rid;
								std::cout << hashKey.str() << std::endl;
								if(!redis_conn->setAdd(hashKey.str(), "none")){
								}
							}
						}
						//audio info
						if(type == 0){
							//audio info
							hashKey.str("");
							if(audioSourceId > 0){
								//check in mysql audio info
								//std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
								//std::ostringstream s_sql;
								s_sql.str("");
								s_sql << "select * from AudioProduct where audiosourceid=" << audioSourceId;
								std::cout << s_sql.str() << std::endl;
								std::string sql = s_sql.str();
								int cnt = sql_conn->execute(sql);
								std::cout << "count:" << cnt << std::endl;
								if(cnt > 0){
									mysqlresult *res_audio = sql_conn->fetchall();
									if(res_audio != nullptr){
										for(auto iter=res_audio->m_rows.begin();iter!=res_audio->m_rows.end();iter++){
											std::unordered_map<std::string,std::string> hashData;
											std::string audioProductId = (*iter)->get_value("id");
											std::cout <<(int)(*iter)->get_value("id") << std::endl;
											if(server_config.count("Audioinfo") == 0){
												//error
											}
											std::unordered_map<std::string,std::string> table_config = server_config["Audioinfo"];
											for(auto it=table_config.begin(); it!=table_config.end(); ++it){
												//std::cout << it->second << "---" << it->first << std::endl;
												std::string val = (*iter)->get_value(it->first);
												hashData.emplace(it->second,val);
											}
											//set redis data
											hashKey.str("");
											hashKey << "Audio:" << audioProductId;
											std::cout << hashKey.str() << std::endl;
											if(!redis_conn->setHash(hashKey.str(), hashData)){
											}
											//set redis
											hashKey.str("");
											hashKey << type_table << ":Audio:" << rid;
											std::cout << hashKey.str() << std::endl;
											if(!redis_conn->setAdd(hashKey.str(), audioProductId)){
											}
										}
										res_audio->free();
									}else{
										std::cout << "sql fetch res audio null" << std::endl;
									}
								}
							}
						}//audio info end
					}

				}
			}
			begin_num = end_num;
			end_num += step;
			break;
		}
		sleep(3);
		break;
	}
}

