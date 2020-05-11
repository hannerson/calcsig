#include "utils.h"
#include "config.h"
#include <iostream>
#include "glog/logging.h"

int main(){
	timeval tv;
    	gettimeofday(&tv, 0);
    	srand((int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec);
	//mkdirs("a/b/c",S_IRUSR|S_IWUSR|S_IXUSR);
	FLAGS_logbufsecs = 0;
	FLAGS_log_prefix = true;
	google::InitGoogleLogging("test");
	google::SetLogDestination(google::INFO, "test");
	google::FlushLogFiles(google::INFO);
	class configParser* configptr = new configParser(std::string ("../conf/server.conf"));
	configptr->loadConfig();
	LOG(INFO) << "----------------glog--------------";
	std::cout << randString(16,true,true) << std::endl;
	std::cout << randString(16,true,true) << std::endl;
	std::cout << randString(16,true,true) << std::endl;
	std::cout << randString(16,true,true) << std::endl;
	std::cout << randString(16,true,false) << std::endl;
	std::cout << randString(16,false,false) << std::endl;
	std::cout << randString(16,false,true) << std::endl;
	std::cout << mvfile("test.o","a/b/c.o") << std::endl;
	std::string base_path = "./utils";
	std::cout << "---" <<  mkdirs(base_path + "/tmp/", S_IRUSR|S_IWUSR|S_IXUSR) << std::endl;
	
	std::cout << sigPath(3618479176,3060740984,"Mp3") << std::endl;
	std::string a = "sdfdfd";
	std::cout << formatStr("oook - %s - %d", a.c_str(), 2345) << std::endl;
	uint32_t sig1,sig2;
	calcSig("420500511.flac",&sig1,&sig2);
	std::cout << sig1 << "--" << sig2 << std::endl;
}
