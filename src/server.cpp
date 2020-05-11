#include "server.h"
#include <iostream>
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
#include "json/json.h"
#include <event2/thread.h>
#include <event2/util.h>
#include "curl/curl.h"

#define BUFFSIZE 1024*1024

struct downloadInfo{
	int fd;
	uint32_t file_sig1;
	uint32_t file_sig2;
	char file_path[128];
	char buffer[BUFFSIZE];
	uint64_t content_length;
	uint64_t content_read;
	uint64_t buffer_used_size;
	std::string fileformat;
	bool isOK;
};

struct callbackInfo{
	uint64_t content_length;
	uint64_t content_read;
	std::string content;
};

class http_task:public task_item{
	public:
		http_task(){}
		~http_task(){
		}
	public:
		std::string action_id;
		std::string src_url;
		std::string file_format;
		std::string table;
		std::string callback_url;
};

static size_t data_reader(void *ptr, size_t size, size_t nmemb, void *param)
{
    downloadInfo *curl_info = (downloadInfo*)param;
    curl_info->content_read += size * nmemb;

    int data_read = 0;
    int read_size;
    write(curl_info->fd, (char*)ptr, size * nmemb);
    LOG(INFO) << "flush data to file, already " << curl_info->content_read << " data" << std::endl;
    /*while (data_read < size * nmemb)
    {
        read_size = BUFFSIZE - curl_info->buffer_used_size > size * nmemb - data_read
                    ? size * nmemb - data_read
                    : BUFFSIZE - curl_info->buffer_used_size;

        memcpy(curl_info->buffer + curl_info->buffer_used_size, 
               (char *)ptr + data_read,
               read_size);

        data_read += read_size;
        curl_info->buffer_used_size += read_size;

        if (curl_info->buffer_used_size == BUFFSIZE)
        {
            LOG(INFO) << "flush data to file, already " << curl_info->content_read << " data" << std::endl;
            write(curl_info->fd, curl_info->buffer, BUFFSIZE);
            curl_info->buffer_used_size = 0;
        }
    }*/
	return size * nmemb;
}


static size_t head_reader(void* ptr, size_t size, size_t nmemb, void* param)
{
    char* pos = strstr((char*)ptr, ":");
    if (pos == NULL)
    {
        return size * nmemb;
    }    

    downloadInfo *curl_info = (downloadInfo*)param;
    if (ptr == strstr((char*)ptr, "Content-Length"))
    {
       std::string number(pos + 2, size * nmemb - (pos + 2 - (char*)ptr)); 
       curl_info->content_length = strtoul(number.c_str(), NULL, 10);
    }    
	return size * nmemb;
}

static size_t callback_head_reader(void* ptr, size_t size, size_t nmemb, void* param)
{
    char* pos = strstr((char*)ptr, ":");
    if (pos == NULL)
    {
        return size * nmemb;
    }    

    callbackInfo *curl_info = (callbackInfo*)param;
    if (ptr == strstr((char*)ptr, "Content-Length"))
    {
       std::string number(pos + 2, size * nmemb - (pos + 2 - (char*)ptr)); 
       curl_info->content_length = strtoul(number.c_str(), NULL, 10);
    }    
	return size * nmemb;
}

static size_t callback_data_reader(void *ptr, size_t size, size_t nmemb, void *param)
{
    callbackInfo *curl_info = (callbackInfo*)param;
    std::string content((char*)ptr, size * nmemb);
    curl_info->content += content;
    curl_info->content_read += size * nmemb;
	return size * nmemb;
}

void thread_worker(void *args){
	class threadpool_98 *pool = (class threadpool_98*)args;
	class httpServer *server = (class httpServer*)pool->server;
	//thread init register
	pthread_mutex_lock(&server->init_lock);
	server->init_count ++;
	LOG(INFO) << "init count - " << server->init_count << std::endl;
	pthread_cond_signal(&server->init_cond);
	pthread_mutex_unlock(&server->init_lock);

	const std::string base_path = server->server_config["common"]["base_path"];

	while(!server->stop_status()){
		//get task
		class http_task *task = (class http_task*)pool->get_task();
		if(task == NULL){
			continue;
		}
		std::string tmp_real_path = base_path + "/tmp/" + randString(16,true,true) + "." + task->file_format;
		LOG(INFO) << tmp_real_path << std::endl;
		std::string tmp_real_dir = base_path + "/tmp/";
		if((access(tmp_real_dir.c_str(), R_OK | W_OK | X_OK)!=0) && (mkdirs(base_path + "/tmp/", S_IRUSR|S_IWUSR|S_IXUSR)!=0)){
			LOG(ERROR) << "tmp dir not exists." << std::endl;
			delete task;
			continue;
		}
		downloadInfo *curl_info = new downloadInfo();
		curl_info->fd = open(tmp_real_path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
		curl_info->file_sig1 = 0;
		curl_info->file_sig2 = 0;
		curl_info->content_length = 0;
		curl_info->content_read = 0;
		curl_info->buffer_used_size = 0;
		curl_info->isOK = true;
		//download file
		CURL *curl = curl_easy_init();
		if(curl == NULL){
			delete curl_info;
			curl_info = NULL;
			continue;
		}
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 500);
        	curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 1);
        	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, BUFFSIZE);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, curl_info);
        	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, head_reader);
        	curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_info);
        	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_reader);
		curl_easy_setopt(curl, CURLOPT_URL, task->src_url.c_str());
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			LOG(INFO) << "curl download error. taskid-" << task->action_id << "." << std::endl;
			curl_info->isOK = false;
		}
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if(response_code != 200){
			LOG(INFO) << "curl download error. taskid-" << task->action_id << ". response code-" << response_code << "." << std::endl;
			curl_info->isOK = false;
		}
		curl_easy_cleanup(curl);
		close(curl_info->fd);
		if(!curl_info->isOK){
			delete curl_info;
			curl_info = NULL;
			//remove tmp
			remove(tmp_real_path.c_str());
			continue;
		}
		//calc sig
		if(!calcSig(tmp_real_path.c_str(), &curl_info->file_sig1, &curl_info->file_sig2)){
			LOG(ERROR) << formatStr("calc sig failed. path %s",tmp_real_path.c_str()) << std::endl;
			curl_info->isOK = false;
			delete task;
			delete curl_info;
			curl_info = NULL;
			continue;
		}
		//mv target path
		std::string sig_path = sigPath(curl_info->file_sig1,curl_info->file_sig2,task->file_format);
		std::string target_path = base_path + "/" + sig_path;
		if(mvfile(tmp_real_path, target_path) != 0){
			LOG(ERROR) << formatStr("mv file failed.from %s to %s",tmp_real_path.c_str(),target_path.c_str()) << std::endl;
			curl_info->isOK = false;
			delete task;
			delete curl_info;
			curl_info = NULL;
			continue;
		}
		//callback
		Json::FastWriter writer;
		Json::Reader reader;
		Json::Value item;
		Json::Value result;
		item["action_id"] = task->action_id.c_str();
		item["file_sig1"] = curl_info->file_sig1;
		item["file_sig2"] = curl_info->file_sig2;
		item["file_path"] = sig_path.c_str();
		item["file_size"] = (Json::Value::UInt64)curl_info->content_length;
		item["my_ip"] = server->my_ip.c_str();
		item["table"] = task->table.c_str();

		delete curl_info;
		curl_info = NULL;

		callbackInfo *curl_callback = new callbackInfo();
		std::string ret_str = writer.write(item);
		curl = curl_easy_init();
        	if(curl == NULL){
            		delete curl_callback;
			curl_callback = NULL;
            		continue;
        	}
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 500);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, curl_callback);
        	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, callback_head_reader);
        	curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_callback);
        	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_data_reader);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ret_str.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, task->callback_url.c_str());
		res = curl_easy_perform(curl);
		LOG(INFO) << formatStr("callback-%s. taskid-%s, post data:%s",task->action_id.c_str(),task->callback_url.c_str(),ret_str.c_str()) << std::endl;
		if(res != CURLE_OK){
			LOG(ERROR) << "curl callback error. taskid-" << task->action_id << "." << std::endl;
		}
		LOG(INFO) << formatStr("callback-%s. taskid-%s, response data:%s",task->action_id.c_str(),task->callback_url.c_str(),curl_callback->content.c_str()) << std::endl;
		if(!reader.parse(curl_callback->content,result)){
			LOG(ERROR) << formatStr("callback error parse json result. taskid-%s",task->action_id.c_str()) << std::endl;
		}
		if(result["ret"] != 0){
			LOG(ERROR) << formatStr("callback failed. taskid-%s",task->action_id.c_str()) << std::endl;
		}
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		delete task;
        	delete curl_callback;
	}
}

FUNC_PTR threadpool_98::fp = thread_worker;

int httpServer::init(std::map<std::string,std::map<std::string,std::string> > config){
	try{
		//pthread init
		pthread_mutex_init(&init_lock,NULL);
		pthread_cond_init(&init_cond,NULL);
		//config init
		server_config = config;
		std::map<std::string,std::string> config_common = config["common"];
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

		if(config_common.count("my_ip") > 0){
			my_ip = config_common["my_ip"];
		}else{
			std::cout << "config my_ip not exists" << std::endl;
			return -1;
		}

		thread_pool = new threadpool_98(threadnum,threadmax,queue_max_size);
		thread_pool->server = this; 
		std::cout << threadnum << ":" << threadmax << std::endl;
	}
	catch(char * e){
	}
	return 0;
}

void httpServer::stop(){
	isstop = false;
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
	evthread_use_pthreads();
	init_count = 0;
	//init the seeds
	timeval tv;
   	gettimeofday(&tv, 0);
    	srand((int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec);
	//start sub threads
	thread_pool->start();

	pthread_mutex_lock(&init_lock);
	while(init_count < threadnum){
		pthread_cond_wait(&init_cond,&init_lock);
	}
	pthread_mutex_unlock(&init_lock);

	struct event_base *base = event_base_new();
	if(base == NULL){
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

	evhttp_set_cb(http_server,"/test",httpServer::test_process, this);
	evhttp_set_cb(http_server,"/calcsig",httpServer::calcsig_process, this);
	evhttp_set_gencb(http_server, httpServer::other_process, this);

	//set timer event and timeout
	struct timeval tv_in = {10, 0};
	struct event *ev_timeout = event_new(NULL,-1, 0, NULL, NULL);
	event_assign(ev_timeout, base, -1, EV_TIMEOUT | EV_PERSIST, event_timeout, NULL);
	event_add(ev_timeout, &tv_in);

	evhttp_set_timeout(http_server,5);
	event_base_dispatch(base);
	evhttp_free(http_server);
	return 0;
}

void httpServer::test_process(struct evhttp_request *request, void *args){
	LOG(INFO) << "test request" << std::endl;
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if(retbuff == NULL){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	result["status"] = "success";
	result["msg"] = "just test page!";
	result["ret"] = "0";
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

void httpServer::other_process(struct evhttp_request *request, void *args){
	LOG(INFO) << "other request" << std::endl;
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if(retbuff == NULL){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}
	Json::Value result;
	Json::FastWriter writer;
	result["status"] = "success";
	result["msg"] = "wrong access,kidding me?";
	result["ret"] = "-1";
	std::string return_str = writer.write(result);
	int ret = evbuffer_add_printf(retbuff, return_str.c_str());
	if(ret == -1){
		LOG(ERROR) << "evbuffer printf failed" << std::endl;
		return;
	}
	evhttp_send_reply(request,HTTP_OK,"ok",retbuff);
	evbuffer_free(retbuff);
}

//uri:/calcsig {"timestamp":"12345","pkey":"dfjkdfkdlfdk","rid":"123456"}
//1.check valid
//2.parse json
//3.add task
//4.return ok
void httpServer::calcsig_process(struct evhttp_request *request, void *args){
	class httpServer *server = (class httpServer*)args;
	Json::Value result;
	Json::FastWriter writer;
	Json::Value data;
	Json::Reader reader;
	LOG(INFO) << "calcsig request" << std::endl;
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if(retbuff == NULL){
		LOG(ERROR) << "evbuffer create failed" << std::endl;
		return;
	}

	if(evhttp_request_get_command(request) != EVHTTP_REQ_POST){
		result["status"] = "fail";
		result["msg"] = "just support get method";
		result["ret"] = "-1";
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

	struct evbuffer* evInput = evhttp_request_get_input_buffer(request);

	int nRead = 0;
	int offset = 0;
	char szTemp[2048];
	while ((nRead = evbuffer_remove(evInput, szTemp + offset, sizeof(szTemp) - offset)) > 0)
	{
		offset += nRead;
	}
	//parse and add task
	class http_task *task = new http_task();
	if(!reader.parse(szTemp,data)){
		LOG(ERROR) << "parse calc sig action json error" << std::endl;
	}
	LOG(INFO) << szTemp << std::endl;
	if(!data.isMember("action_id") || !data.isMember("src_url") || !data.isMember("callback_url")){
		result["status"] = "success";
		result["msg"] = "parameter missing";
		result["ret"] = "-1";
	}else{
		LOG(INFO) << data["action_id"].asString() << std::endl;
		task->action_id = data["action_id"].asString();
		task->src_url = data["src_url"].asString();
		task->callback_url = data["callback_url"].asString();
		task->table = data["table"].asString();
		task->file_format = data["file_format"].asString();
		server->thread_pool->add_task(task);
		result["status"] = "success";
		result["msg"] = "OK";
		result["ret"] = "0";
	}

	LOG(INFO) << formatStr("add task action-%s success",task->action_id.c_str()) << std::endl;

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
	LOG(INFO) << "timeout 10s" << std::endl;
}

