#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include "config.h"
#include "glog/logging.h"

int configParser::loadConfig(){
	std::ifstream infile(fileconfig.c_str());
	if(!infile){
		//LOG(ERROR) << "file open:" << fileconfig << " failed." << std::endl;
		return -1;
	}
	std::string instr;
	std::string cur_key;
	std::map<std::string, std::string> temp_map;
	while(getline(infile, instr)){
		int index = 0, index_left = 0, index_right = 0;
		//del ' '
		if(!instr.empty()){
			//std::cout << instr << std::endl;
			index=instr.find(' ',index);
			while(index != std::string::npos){
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
		std::cout << instr << std::endl;

		if(index_left != std::string::npos){
			if(cur_key != ""){
				config.insert(std::pair<std::string,std::map<std::string, std::string> >(cur_key,temp_map));
			}
			cur_key = instr.substr(index_left+1,index_right-index_left-1);
			std::cout << cur_key << std::endl;
			temp_map.clear();
			continue;
		}

		if(index == std::string::npos){
			continue;
		}
		temp_map.insert(std::pair<std::string,std::string>(instr.substr(0,index),instr.substr(index+1)));
	}
	config.insert(std::pair<std::string,std::map<std::string, std::string> >(cur_key,temp_map));
	infile.close();
	return 0;
}
