#include "indexer.h"

int main(int argc, char ** argv){
	if(argc < 2){
		std::cout << "parameter missing" << std::endl;
		std::cout << "usage: " << std::string(argv[0]) << " " << "configfile" << std::endl;
		return -1;
	}
	google::InitGoogleLogging("http_server");
	google::SetLogDestination(google::INFO, "./");
	google::FlushLogFiles(google::INFO);
	class configParser* configptr = new configParser(std::string (argv[1]));
	if(configptr->loadConfig() == -1){
		return -1;
	}
	class indexerServer *server = new indexerServer();
	if(server->init(configptr->config) == -1){
		std::cout << "init failed" << std::endl;
		return -1;
	}
	server->start(0, 10000);
}
