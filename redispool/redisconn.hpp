#ifndef __REDIS_CONN_HPP__
#define __REDIS_CONN_HPP__
#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include "hiredis.h"

class redisconn{
	public:
		redisconn(std::string ip, unsigned int port):redisIP(ip),redisPort(port),m_redis(nullptr){
			std::cout << redisIP << "-*-" << redisPort << std::endl;
			connect();
		}
		~redisconn(){
			if(m_redis != nullptr){
				redisFree(m_redis);
				m_redis = nullptr;
			}
		}
		void connect(){
			std::cout << redisIP << "--" << redisPort << std::endl;
			m_redis = redisConnect(redisIP.c_str(), redisPort);
			if(m_redis == nullptr || m_redis->err){
				//error throw error
				std::ostringstream ostr;
				ostr << "connect redis error: redis-" << redisIP << " port-" << redisPort << std::endl;
				throw std::runtime_error(ostr.str());
			}
		}

		void sendCommand( std::string command ){
			m_reply = (redisReply*)redisCommand(m_redis, command.c_str());
		}

		redisReply * getReply(){
			return m_reply;
		}

		void freeReply(){
			if(m_reply != nullptr){
				freeReplyObject(m_reply);
			}
		}
	private:
		redisContext * m_redis;
		redisReply * m_reply;
		std::string redisIP;
		unsigned int redisPort;
		
	private:
		redisconn(const redisconn&);
		redisconn& operator = (const redisconn&);
		/*redisContext * m_redis;
		redisReply * m_reply;
		std::string redisIP;
		unsigned int redisPort;*/
};

#endif
