#include "redisconn_cluster.hpp"
#include <iostream>
#include <stdlib.h>
#include "redispool_cluster.h"

int main(){
	//class redisconn * conn = new class redisconn("127.0.0.1",6379);
	//class redisconn * conn2 = new class redisconn("127.0.0.1",6379);
	//std::cout << conn->redisIP << std::endl;
	//conn->connect();
	class redisClusterPool *connpool = new redisClusterPool(5,10,"127.0.0.1:6379","yeelion");
	std::shared_ptr<redisconn_cluster> conn = connpool->get_connection();
	//auth
	/*conn->sendCommand("AUTH yeelion");
	redisReply * m_reply = conn->getReply();
	if(m_reply->type == REDIS_REPLY_STATUS){
		std::cout << m_reply->str << std::endl;
	}
	conn->freeReply();*/
	conn->sendCommand("HMSET 12345 id 12345 name hanson age 20");
	redisReply *m_reply = conn->getReply();
	if(m_reply->type == REDIS_REPLY_STATUS){
		std::cout << m_reply->str << std::endl;
	}
	conn->freeReply();
	conn->sendCommand("HGETALL 12345");
	m_reply = conn->getReply();
	std::cout << m_reply->type << std::endl;
	if(m_reply->type == REDIS_REPLY_ARRAY){
		for(int i=0; i<m_reply->elements;i++){
			if(m_reply->element[i]->type == REDIS_REPLY_STRING){
				std::cout << m_reply->element[i]->str << std::endl;
			}
		}
	}
	conn->freeReply();
	return 0;
}
