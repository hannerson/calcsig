#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include "config.h"
#include <glog/logging.h>

int configParser::loadConfig(){
	std::ifstream infile(fileconfig.c_str());
	if(!infile){
		LOG(ERROR) << "file open:" << fileconfig << " failed." << std::endl;
		return -1;
	}
	std::string instr;
	std::string cur_key;
	std::unordered_map<std::string, std::string> temp_map;
	while(getline(infile, instr)){
		int index = 0, index_left = 0, index_right = 0;
		//del ' '
		if(!instr.empty()){
			while((index=instr.find(' ',index)) != std::string::npos){
				instr.erase(index,1);
			}
		}

		if((index=instr.find('#')) != std::string::npos){
			instr = instr.substr(0,index);
		}
		if(instr == "" || instr.find("#") == 0){
			continue;
		}
		index_left = instr.find('[');
		index_right = instr.find(']');
		index = instr.find('=');
		if(index == std::string::npos && (index_left == std::string::npos || index_right == std::string::npos)){
			continue;
		}
		//std::cout << instr << std::endl;

		if(index_left != std::string::npos){
			if(cur_key != ""){
				config.emplace(cur_key,temp_map);
			}
			cur_key = instr.substr(index_left+1,index_right-index_left-1);
			//std::cout << cur_key << std::endl;
			temp_map.clear();
			continue;
		}

		if(index == std::string::npos){
			continue;
		}
		temp_map.emplace(instr.substr(0,index),instr.substr(index+1));
	}
	config.emplace(cur_key,temp_map);
	infile.close();
}

//#define _CONFIG_TEST_
#ifdef _CONFIG_TEST_
int main(){
	google::InitGoogleLogging("http_server");
	google::SetLogDestination(google::INFO, "/home/dmsMusic/http_server");
	class configParser* configptr = new configParser(std::string ("../conf/server.conf"));
	configptr->loadConfig();
	std::cout << configptr->config["common"]["threadnum"] << std::endl;
	std::cout << configptr->config["common"]["secret_key"] << std::endl;
	LOG(INFO) << "file config:" << configptr->fileconfig << std::endl;
	std::cout << configptr->config["mysql"]["dbres_ip"] << std::endl;
	LOG(INFO) << configptr->config["mysql"]["dbres_ip"] << std::endl;
}
#endif
