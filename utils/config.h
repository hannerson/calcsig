#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <iostream>
#include <string>
#include <map>

class configParser{
	public:
		configParser(std::string fileconfig):fileconfig(fileconfig){}
		~configParser(){}
		int loadConfig();

	public:
		std::string fileconfig;
		std::map<std::string,std::map<std::string,std::string> > config;
};

#endif
