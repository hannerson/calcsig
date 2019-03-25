#include "server.h"
#include <iostream>
#include <thread>
#include <mutex>
#include "utils/config.h"
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

std::unordered_map<std::string, server_func> map_server_func;

class http_task:public Task{
	public:
		http_task(class httpServer * server, struct evhttp_request *request):server(server),request(request){}
		~http_task(){
		}
		void run(){
			if(server == nullptr){
				return;
			}
			server->run_worker(request);
		}
	private:
		class httpServer *server;
		struct evhttp_request *request;
};

const struct{
	std::string uri;
	void (httpServer::*func)(struct evhttp_request *);
} request_process_config[] = {
	{"/test",&httpServer::test_process},
	{"/echo",&httpServer::echo_process},
	{"/getinfo",&httpServer::getinfo_process},
	{"/other",&httpServer::other_process},
};


int httpServer::init(std::unordered_map<std::string,std::unordered_map<std::string,std::string> > config){
	try{
		server_config = config;
		std::unordered_map<std::string,std::string> config_common = config["common"];
		if(config_common.count("threadnum") > 0){
			threadnum = atoi(config_common["threadnum"].c_str());
		}else{
			std::cout << "config threadnum not exists" << std::endl;
			return -1;
		}

		if(config_common.count("threadmax") > 0){
			threadmax = atoi(config_common["threadmax"].c_str());
		}else{
			std::cout << "config threadmax not exists" << std::endl;
			return -1;
		}

		if(config_common.count("queue_max_size") > 0){
			queue_max_size = atoi(config_common["queue_max_size"].c_str());
		}else{
			std::cout << "config queue_max_size not exists" << std::endl;
			return -1;
		}

		if(config_common.count("secret_key") > 0){
			secret_key = config_common["secret_key"];
		}else{
			std::cout << "config secret_key not exists" << std::endl;
			return -1;
		}

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
		thread_pool = new threadPool();
		std::cout << threadnum << ":" << threadmax << std::endl;
		if(thread_pool-> init(threadnum,threadmax) != 0){
			throw std::runtime_error("thread pool init failed");
		}
		//init process function
		map_server_func.emplace("/test",&httpServer::test_process);
		map_server_func.emplace("/echo",&httpServer::echo_process);
		map_server_func.emplace("/getinfo",&httpServer::getinfo_process);
		map_server_func.emplace("/other",&httpServer::other_process);
	}
	catch(char * e){
	}
	return 0;
}

int httpServer::stop(){
	std::lock_guard<std::mutex> lock(isstop_mutex);
	isstop = false;
	return 0;
}

bool httpServer::check_request_valid(std::string timestamps, std::string md5sum){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned char md[16];
	char buf[33] = {'\0'};
	char tmp[3] = {'\0'};
	std::string secret(secret_key);
	std::string src = secret + "_" + timestamps;
	std::string md5str;
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, src.c_str(), src.length());
	MD5_Final(md,&ctx);
	for(int i=0; i<16; i++){
		sprintf(tmp, "%02x", md[i]);
		strcat(buf,tmp);
	}
	//std::cout << buf << std::endl;
	if(md5sum.compare(buf) == 0){
		return true;
	}else{
		return false;
	}
}

int httpServer::start(){
	struct event_base *base = event_base_new();
	if(base == nullptr){
		LOG(ERROR) << "create event base failed" << std::endl;
		return -1;
	}
	struct evhttp *http_server = evhttp_new(base);
	if(!http_server){
		LOG(ERROR) << "create event http failed" << std::endl;
		return -1;
	}
	int ret = evhttp_bind_socket(http_server,http_addr.c_str(),port);
	if(ret != 0){
		LOG(ERROR) << "bind socket failed" << std::endl;
		return -1;
	}

	evhttp_set_gencb(http_server, http_generic_handler, this);

	//start threadworker
	//
	/*for(auto i=0; i<threadnum; i++){
		threadpool.emplace_back(std::thread(&httpServer::thread_worker,this));
		LOG(INFO) << "create thread "<< i << " success" << std::endl;
	}*/
	//set timer event and timeout
	struct event *ev_timeout = event_new(NULL,-1, 0, NULL, NULL);
	event_assign(ev_timeout, base, -1, EV_TIMEOUT | EV_PERSIST, event_timeout, NULL);
	event_add(ev_timeout, NULL);

	evhttp_set_timeout(http_server,5);
	event_base_dispatch(base);
	evhttp_free(http_server);
}

void httpServer::http_generic_handler(struct evhttp_request *request, void *args){
	class httpServer *me = (class httpServer*) args;
	std::cout << (void*) request << std::endl;
	const char* uri = evhttp_request_get_uri(request);
	std::cout << "uri is " << uri << std::endl;
	class http_task task(me,request);
	me->thread_pool->task_add(&task);
	LOG(INFO) << "add task" << std::endl;
	//std::cout << "add task" << std::endl;
	//sleep(3);
}

void httpServer::add_task(struct evhttp_request *request){
	std::unique_lock<std::mutex> lock(queue_mutex);
	m_not_full.wait(lock, [this]{return isstop || not_full();});
	if(isstop){
		return;
	}
	task_queue.emplace(request);
	m_not_empty.notify_all();
}

void httpServer::get_task(struct evhttp_request **request){
	std::unique_lock<std::mutex> lock(queue_mutex);
	m_not_empty.wait(lock, [this]{return isstop || not_empty();});
	if(isstop){
		return;
	}
	*request = task_queue.front();
	task_queue.pop();
	m_not_full.notify_all();
}

bool httpServer::not_full(){
	bool full = task_queue.size() >= queue_max_size;
	if(full){
		LOG(INFO) << "task queue is full:" << queue_max_size << std::endl;
	}
	return !full;
}

bool httpServer::not_empty(){
	bool empty = task_queue.empty();
	if(empty){
		LOG(INFO) << "task queue is empty" << std::endl;
	}
	return !empty;
}

int httpServer::run_worker(struct evhttp_request *request){
	//struct evhttp_request *request;
	//std::cout << (void*) request << std::endl;
	LOG(INFO) << "get task ok" << std::endl;
	//std::cout << "get task ok " << std::this_thread::get_id() << std::endl;
	//process request
	const char* uri = evhttp_request_get_uri(request);
	if(uri == nullptr){
		LOG(INFO) << "uri is null" << std::endl;
		//std::cout << "uri is null" << std::endl;
		struct evbuffer *retbuff = nullptr;
		retbuff = evbuffer_new();
		if(retbuff == nullptr){
			LOG(ERROR) << "evbuffer create failed" << std::endl;
			return -1;
		}
		Json::Value result;
		Json::FastWriter writer;
		result["status"] = "success";
		result["msg"] = "uri is null";
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return -1;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
	}
	std::cout << "uri is " << uri << std::endl;
	//std::string s_uri = evhttp_uri_parse(uri);
	//std::size_t pos = s_uri.find("?");
	struct evhttp_uri *decoded = nullptr;
	decoded = evhttp_uri_parse(uri);
	if(decoded == nullptr){
		std::cout << "error" << std::endl;
		LOG(INFO) << "evhttp_uri_parse error" << std::endl;
		evhttp_send_error(request, HTTP_BADREQUEST, 0);
		return -1;
	}
	std::string uri_path = evhttp_uri_get_path(decoded);
	free(decoded);
	/*if(pos != std::string::npos){
		uri_path = s_uri.substr(0,pos);
	}else{
		uri_path = uri;
	}*/

	
	if(uri_path == ""){
		LOG(INFO) << "uri is empty" << std::endl;
		std::cout << "uri is empty" << std::endl;
		struct evbuffer *retbuff = nullptr;
		retbuff = evbuffer_new();
		if(retbuff == nullptr){
			LOG(ERROR) << "evbuffer create failed" << std::endl;
			return -1;
		}
		Json::Value result;
		Json::FastWriter writer;
		result["status"] = "success";
		result["msg"] = "uri is empty";
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return -1;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
	}else{
		//process request
		if(map_server_func.find(uri_path) != map_server_func.end()){
			(this->*map_server_func[uri_path])(request);
		}else{
			(this->*map_server_func["/other"])(request);
		}
	}
	return 0;
}

void httpServer::test_process(struct evhttp_request *request){
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	result["status"] = "success";
	result["msg"] = "just test page!";
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::echo_process(struct evhttp_request *request){
	const char* uri = evhttp_request_get_uri(request);
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	//Json::Value data(Json::arrayValue);
	Json::Value data;
	result["status"] = "success";
	result["msg"] = "uri is " + std::string(uri);
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::other_process(struct evhttp_request *request){
	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	//Json::Value data(Json::arrayValue);
	Json::Value data;
	result["status"] = "success";
	result["msg"] = "wrong access,kidding me?";
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

//uri:/getinfo?timestamp=12345&pkey=dfjkdfkdlfdk&rid=123456&type=0  0-music,1-album,2-artist
void httpServer::getinfo_process(struct evhttp_request *request){
	const char* uri = evhttp_request_get_uri(request);
	std::cout << uri << "  -------  " << std::endl;
	struct evhttp_uri *decoded = nullptr;
	decoded = evhttp_uri_parse(uri);

	if(decoded == nullptr){
		std::cout << "error" << std::endl;
		LOG(INFO) << "evhttp_uri_parse error" << std::endl;
		evhttp_send_error(request, HTTP_BADREQUEST, 0);
		return;
	}

	Json::Value result;
	Json::FastWriter writer;
	//Json::Value data(Json::arrayValue);
	Json::Value data;

	struct evbuffer *retbuff = nullptr;
	retbuff = evbuffer_new();
	if(retbuff == nullptr){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}

	if(evhttp_request_get_command(request) != EVHTTP_REQ_GET){
		result["status"] = "fail";
		result["msg"] = "just support get method";
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}
	char *query = (char*)evhttp_uri_get_query(decoded);
	if(query == nullptr){
	}
	//evhttp_parse_query_str(query, params);
	char* decode_uri = evhttp_decode_uri(uri);
	std::cout << decode_uri << "  -------  " << std::endl;
	struct evkeyvalq params;
	//evhttp_parse_query(decode_uri,&params);
	evhttp_parse_query_str(query, &params);
	std::cout << "----" << std::endl;
	const char* temp = nullptr;
	std::string timestamp,pkey;
	unsigned int type;
	unsigned long rid;

	temp = evhttp_find_header(&params,"timestamp");
	if(temp){
		timestamp = temp;
		std::cout << timestamp << std::endl;
	}
	temp = evhttp_find_header(&params,"pkey");
	if(temp){
		pkey = temp;
		std::cout << pkey << std::endl;
	}

	temp = evhttp_find_header(&params,"rid");
	if(temp){
		rid = atol(temp);
		std::cout << rid << std::endl;
	}

	temp = evhttp_find_header(&params,"type");
	if(temp){
		type = atoi(temp);
		std::cout << type << std::endl;
	}

	free(decode_uri);

	if(timestamp == "" || pkey == ""){
		result["status"] = "fail";
		result["msg"] = "parameter is missing";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}
	//check valid
	/*if(!check_request_valid(timestamp,pkey)){
		result["status"] = "fail";
		result["msg"] = "valid check failed";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}*/

	if(rid == 0){
		result["status"] = "fail";
		result["msg"] = "rid is wrong";
		result["data"] = data;
		std::string return_str = writer.write(result);
		int ret = evbuffer_add_printf(retbuff, return_str.c_str());
		if(ret == -1){
			LOG(ERROR) << "evbuffer printf failed" << std::endl;
			return;
		}
		evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
		evbuffer_free(retbuff);
		return;
	}

	Json::Reader reader;
	Json::Value tempjson;
	std::string toparse;
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

	//check in redis
	std::shared_ptr<redisconn> redis_conn = redis_pool->get_connection();
	unsigned int audioSourceId = 0;
	
	std::ostringstream hashKey;
	hashKey << type_table << ":" << rid;
	std::cout << hashKey.str() << std::endl;
	std::unordered_map<std::string,std::string> hashInfo = redis_conn->getHashAll(hashKey.str());
	Json::Value payinfos(Json::arrayValue);
	if(!hashInfo.empty()){
		if(type == 0 && hashInfo.count("audiosourceid")>0){
			audioSourceId = atol(hashInfo["audiosourceid"].c_str());
		}
		for(auto it=hashInfo.begin();it!=hashInfo.end();++it){
			tempjson[it->first] = it->second;
		}
	}else{
		//check in mysql
		std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
		std::ostringstream s_sql;
		s_sql << "select * from " << type_table << " where id=" << rid;
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
					int id = (*iter)->get_value("id");
					audioSourceId = (*iter)->get_value("audiosourceid");
					std::cout <<(int)(*iter)->get_value("id") << std::endl;
					if(server_config.count(type_table) == 0){
						//error
					}
					std::unordered_map<std::string,std::string> table_config = server_config[type_table];
					for(auto it=table_config.begin(); it!=table_config.end(); ++it){
						//std::cout << it->second << "---" << it->first << std::endl;
						std::string val = (*iter)->get_value(it->first);
						tempjson[it->second.c_str()] = val;
						hashData.emplace(it->second,val);
					}
					//set redis data
					hashKey.str("");
					hashKey << type_table << ":" << rid;
					std::cout << hashKey.str() << std::endl;
					if(!redis_conn->setHash(hashKey.str(), hashData)){
						std::cout << "set hash failed : " << rid << std::endl;
					}
				}
				res->free();
			}else{
				std::cout << "sql fetch res null" << std::endl;
			}
		}else{
			//set redis null
			hashKey.str("");
			hashKey << type_table << ":" << rid;
			std::cout << hashKey.str() << std::endl;
			std::unordered_map<std::string,std::string> hashData;
			hashData.emplace("id","0");
			if(!redis_conn->setHash(hashKey.str(), hashData)){
			}
		}
	}

	if( type == 0 || type == 1){
		//pay info
		Json::Value payinfo;
		hashKey.str("");
		hashKey << type_table << ":Pay:" << rid;
		std::cout << hashKey.str() << std::endl;
		std::vector<std::string> payPolicy = redis_conn->getSet(hashKey.str());
		if(!payPolicy.empty()){
			for(auto itpolicy=payPolicy.begin();itpolicy!=payPolicy.end();++itpolicy){
				if(*itpolicy == "none"){
					continue;
				}
				hashKey.str("");
				hashKey << type_table << ":" << *itpolicy << ":" << rid;
				hashInfo = redis_conn->getHashAll(hashKey.str());
				for(auto it=hashInfo.begin();it!=hashInfo.end();++it){
					payinfo[it->first] = it->second;
				}
				payinfos.append(payinfo);
			}
		}else{
			//check in mysql pay info
			std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
			std::ostringstream s_sql;
			s_sql << "select * from MusicPay where policy!=\"none\" and type=" << type << " and rid=" << rid;
			std::cout << s_sql.str() << std::endl;
			std::string sql = s_sql.str();
			int cnt = sql_conn->execute(sql);
			std::cout << "count:" << cnt << std::endl;
			if(cnt > 0){
				mysqlresult *res = sql_conn->fetchall();
				if(res != nullptr){
					for(auto iter=res->m_rows.begin();iter!=res->m_rows.end();iter++){
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
							payinfo[it->second.c_str()] = val;
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
						payinfos.append(payinfo);
					}
					res->free();
				}else{
					std::cout << "sql fetch res null" << std::endl;
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
	}
	//audio info
	Json::Value audioinfos(Json::arrayValue);
	if(type == 0){
		//audio info
		Json::Value audioinfo;
		hashKey.str("");
		hashKey << type_table << ":Audio:" << rid;
		std::cout << hashKey.str() << std::endl;
		std::vector<std::string> audio_list = redis_conn->getSet(hashKey.str());
		if(!audio_list.empty()){
			for(auto itAudio=audio_list.begin();itAudio!=audio_list.end();++itAudio){
				hashKey.str("");
				hashKey << "Audio:" << *itAudio;
				hashInfo = redis_conn->getHashAll(hashKey.str());
				for(auto it=hashInfo.begin();it!=hashInfo.end();++it){
					audioinfo[it->first] = it->second;
				}
				audioinfos.append(audioinfo);
			}
		}else{
			if(audioSourceId > 0){
				//check in mysql audio info
				std::shared_ptr<mysqlconnector> sql_conn = mysql_pool->get_connection();
				std::ostringstream s_sql;
				s_sql << "select * from AudioProduct where audiosourceid=" << audioSourceId;
				std::cout << s_sql.str() << std::endl;
				std::string sql = s_sql.str();
				int cnt = sql_conn->execute(sql);
				std::cout << "count:" << cnt << std::endl;
				if(cnt > 0){
					mysqlresult *res = sql_conn->fetchall();
					if(res != nullptr){
						for(auto iter=res->m_rows.begin();iter!=res->m_rows.end();iter++){
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
								audioinfo[it->second.c_str()] = val;
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
							audioinfos.append(audioinfo);
						}
						res->free();
					}else{
						std::cout << "sql fetch res null" << std::endl;
					}
				}
			}
		}
	}
	tempjson["payinfo"] = payinfos;
	tempjson["audioinfo"] = audioinfos;
	data[type_table] = tempjson;
	result["status"] = "success";
	result["msg"] = "OK";
	result["data"] = data;
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::event_timeout(int fd, short events, void *args){
}

