#ifndef __REDIS_CONN_HPP__
#define __REDIS_CONN_HPP__
#include <iostream>
#include <string>
#include <strings.h>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "hircluster.h"

class redisconn_cluster{
	public:
		redisconn_cluster(std::string cluster_ips, std::string auth):redisIPs(cluster_ips),auth(auth),m_redis(nullptr){
			//std::cout << redisIPs << "-*-" << redisPort << std::endl;
			connect();
		}
		~redisconn_cluster(){
			if(m_redis != nullptr){
				redisClusterFree(m_redis);
				m_redis = nullptr;
			}
		}
		void connect(){
			//std::cout << redisIPs << "--" << redisPort << std::endl;
			m_redis = redisClusterConnect(redisIPs.c_str(),HIRCLUSTER_FLAG_NULL);
			if(m_redis == nullptr || m_redis->err){
				//error throw error
				std::ostringstream ostr;
				ostr << "connect redis error: redis-" << redisIPs << std::endl;
				throw std::runtime_error(ostr.str());
			}
			if(auth != ""){
				if(redisAuth() == false){
					std::ostringstream ostr;
					ostr << "redis auth error: redis-" << redisIPs << std::endl;
					throw std::runtime_error(ostr.str());
				}
			}
		}

		std::unordered_map<std::string,std::string> getHashAll(std::string keyHash){
			std::unordered_map<std::string,std::string> returnHash;
			std::stringstream command;
			const char * redisvalues[2];
			size_t valuelens[2];
			int count = 0;
			redisvalues[count] = "HGETALL";
			valuelens[count] = 7;
			count ++;
			redisvalues[count] = keyHash.c_str();
			valuelens[count] = keyHash.length();
			count ++;

			command << "HGETALL " << keyHash;
			std::cout << command.str() << std::endl;
			
			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_ARRAY && m_reply->elements > 0){
				std::string key,value;
				for(int i=0; i<m_reply->elements;i++){
					if(m_reply->element[i]->type == REDIS_REPLY_STRING){
						//std::cout << m_reply->element[i]->str << std::endl;
						//key
						if(i%2 == 0){
							key = m_reply->element[i]->str;
						}else{//value
							value = m_reply->element[i]->str;
							if(key == ""){ //something wrong
							}
							returnHash.emplace(key,value);
						}
					}else{
					}
				}
			}else if(m_reply->type == REDIS_REPLY_ARRAY && m_reply->elements == 0){
				//pass
			}else if(m_reply->type == REDIS_REPLY_ERROR){//error
				std::cout << m_reply->str << std::endl;
			}
			freeReply();
			return returnHash;
		}

		bool setHash(std::string keyHash, std::unordered_map<std::string,std::string> hashData){
			std::stringstream command;
			std::string redisfields;
			const char * redisvalues[hashData.size()*2 + 2];
			size_t valuelens[hashData.size()*2 + 2];
			command << "HMSET " << keyHash;
			int count = 0;
			redisvalues[count] = "HMSET";
			valuelens[count] = 5;
			count ++;
			redisvalues[count] = keyHash.c_str();
			valuelens[count] = keyHash.length();
			count ++;
			
			for(auto it=hashData.begin(); it!=hashData.end(); ++it){
				//std::cout << it->first<< "---" << it->second << std::endl;
				redisfields += " " + it->first + " \"" + it->second + "\"";
				redisvalues[count] = it->first.c_str();
				valuelens[count] = it->first.length();
				count ++;
				redisvalues[count] = it->second.c_str();
				valuelens[count] = it->second.length();
				count ++;
			}
			command << redisfields;
			std::cout << command.str() << std::endl;
			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_STATUS && strcasecmp(m_reply->str,"OK") == 0){
				std::cout << "set ok" << std::endl;
			}else{//error
				std::cout << m_reply->str << std::endl;
				freeReply();
				return false;
			}
			freeReply();
			return true;
		}

		bool setString(std::string key, std::string value){
			std::stringstream command;
			const char * redisvalues[3];
			size_t valuelens[3];
			int count = 0;
			redisvalues[count] = "SET";
			valuelens[count] = 3;
			count ++;
			redisvalues[count] = key.c_str();
			valuelens[count] = key.length();
			count ++;
			redisvalues[count] = value.c_str();
			valuelens[count] = value.length();
			count ++;

			command << "SET " << key << " " << value;
			std::cout << command.str() << std::endl;

			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);

			if(m_reply->type == REDIS_REPLY_STATUS && strcasecmp(m_reply->str,"OK") == 0){
			}else{//error
				std::cout << m_reply->str << std::endl;
				freeReply();
				return false;
			}
			freeReply();
			return true;
		}

		std::string getString(std::string key){
			std::string value;
			std::stringstream command;
			const char * redisvalues[2];
			size_t valuelens[2];
			int count = 0;
			redisvalues[count] = "GET";
			valuelens[count] = 3;
			count ++;
			redisvalues[count] = key.c_str();
			valuelens[count] = key.length();
			count ++;

			command << "GET " << key << " " << value;
			std::cout << command.str() << std::endl;

			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_STATUS && strcasecmp(m_reply->str,"OK") == 0){
				value = m_reply->str;
			}else if(m_reply->type == REDIS_REPLY_NIL){//error
				value = "";
			}else if(m_reply->type == REDIS_REPLY_ERROR){
				std::cout << m_reply->str << std::endl;
			}else{
			}
			freeReply();
			return value;
		}

		std::vector<std::string> getSet(std::string key){
			std::vector<std::string> retVector;
			std::stringstream command;
			const char * redisvalues[2];
			size_t valuelens[2];
			int count = 0;
			redisvalues[count] = "SMEMBERS";
			valuelens[count] = 8;
			count ++;
			redisvalues[count] = key.c_str();
			valuelens[count] = key.length();
			count ++;
			command << "SMEMBERS " << key;
			std::cout << command.str() << std::endl;
			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_ARRAY && m_reply->elements > 0){
				for(int i=0; i<m_reply->elements;i++){
					if(m_reply->element[i]->type == REDIS_REPLY_STRING){
						//std::cout << m_reply->element[i]->str << std::endl;
						std::string val = m_reply->element[i]->str;
						retVector.push_back(val);
					}else{
					}
				}
			}else if(m_reply->type == REDIS_REPLY_ARRAY && m_reply->elements == 0){
				//pass
			}else if(m_reply->type == REDIS_REPLY_ERROR){//error
				std::cout << m_reply->str << std::endl;
			}
			freeReply();
			return retVector;
		}


		bool setAdd(std::string key, std::string value){
			std::stringstream command;
			command << "SADD " << key << " " << value;
			std::cout << command.str() << std::endl;

			const char * redisvalues[3];
			size_t valuelens[3];
			int count = 0;
			redisvalues[count] = "SADD";
			valuelens[count] = 4;
			count ++;
			redisvalues[count] = key.c_str();
			valuelens[count] = key.length();
			count ++;
			redisvalues[count] = value.c_str();
			valuelens[count] = value.length();
			count ++;
			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_STATUS && strcasecmp(m_reply->str,"OK") == 0){
			}else if(m_reply->type == REDIS_REPLY_ERROR){//error
				std::cout << m_reply->str << std::endl;
				freeReply();
				return false;
			}else{
				freeReply();
				return false;
			}
			freeReply();
			return true;
		}

		bool setDel(std::string key, std::string value){
			std::stringstream command;
			command << "SREM " << key << " " << value;
			std::cout << command.str() << std::endl;
			const char * redisvalues[3];
			size_t valuelens[3];
			int count = 0;
			redisvalues[count] = "SREM";
			valuelens[count] = 4;
			count ++;
			redisvalues[count] = key.c_str();
			valuelens[count] = key.length();
			count ++;
			redisvalues[count] = value.c_str();
			valuelens[count] = value.length();
			count ++;
			m_reply = (redisReply*)redisClusterCommandArgv(m_redis, count, redisvalues, valuelens);
			if(m_reply->type == REDIS_REPLY_STATUS && strcasecmp(m_reply->str,"OK") == 0){
			}else if(m_reply->type == REDIS_REPLY_ERROR){//error
				std::cout << m_reply->str << std::endl;
				freeReply();
				return false;
			}else{
				freeReply();
				return false;
			}
			freeReply();
			return true;
		}

		void sendCommand( std::string command ){
			m_reply = (redisReply*)redisClusterCommand(m_redis, command.c_str());
		}

		redisReply * getReply(){
			return m_reply;
		}

		bool redisAuth(){
			redisReply *reply = (redisReply*)redisClusterCommand(m_redis, "AUTH %s", auth.c_str());
			//std::cout << reply->str << std::endl;
			//std::cout << reply->type << std::endl;
			if(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0){
				freeReplyObject(reply);
				return true;
			}else{
				freeReplyObject(reply);
				return false;
			}
		}

		void freeReply(){
			if(m_reply != nullptr){
				freeReplyObject(m_reply);
				m_reply = nullptr;
			}
		}
	private:
		redisClusterContext * m_redis;
		redisReply * m_reply;
		std::string redisIPs;
		std::string auth;
	private:
		redisconn_cluster(const redisconn_cluster&);
		redisconn_cluster& operator = (const redisconn_cluster&);
		/*redisContext * m_redis;
		redisReply * m_reply;
		std::string redisIPs;
		unsigned int redisPort;*/
};

#endif
