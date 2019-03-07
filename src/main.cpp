#include "server.h"
#include "config.h"
#include <glog/logging.h>


int main(int argc, char **argv){
	if(argc < 2){
		std::cout << "parameter missing" << std::endl;
		std::cout << "usage: " << std::string(argv[0]) << " " << "configfile" << std::endl;
		return -1;
	}
	google::InitGoogleLogging("http_server");
	google::SetLogDestination(google::INFO, "/home/dmsMusic/http_server");
	google::FlushLogFiles(google::INFO);
	class configParser* configptr = new configParser(std::string (argv[1]));
	if(configptr->loadConfig() == -1){
		return -1;
	}
	class httpServer * server = new class httpServer("0.0.0.0",12345);
	server->init(configptr->config);
	LOG(INFO) << server->check_request_valid("1544113635","b6521a86bc4cc0830370f174e1a30b11") << std::endl;
	server->start();
	return 0;
}
