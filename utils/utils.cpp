#include <ctype.h>
#include <algorithm>
#include "utils.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <map>
#include <fcntl.h>
#include <string.h>
#include <openssl/md5.h>

std::string remove_space_punct(std::string source){
	source.erase(std::remove_if(source.begin(), source.end(), ispunct), source.end());
	source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());
	return source;
}

int mkdirs(std::string pathname, uint32_t mode){
	int pos = pathname.rfind("/");
	if(pos != std::string::npos){
		std::string subpath = pathname.substr(0,pos);
		//std::cout << subpath << std::endl;
		if(subpath !="" && access(subpath.c_str(), F_OK|W_OK|X_OK) != 0){
			if(mkdirs(subpath, mode) != 0){
				return -1;
			}
		}
	}
	if(mkdir(pathname.c_str(), mode) != 0){
		return -1;
	}
	return 0;
}

int mvfile(std::string src, std::string dst){
	if(access(src.c_str(), F_OK|W_OK) != 0){
		return -1;
	}
	std::cout << src << " OK" << std::endl;
	int pos = dst.rfind("/");
	if(pos != std::string::npos){
		std::string dst_path = dst.substr(0,pos);
		if(access(dst_path.c_str(), F_OK|W_OK|X_OK) != 0){
			if(mkdirs(dst_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
				return -1;
			}
		}
	}
	std::cout << dst << " OK" << std::endl;
	if(rename(src.c_str(), dst.c_str()) != 0){
		return -1;
	}
	std::cout << "rename OK" << std::endl;
	return 0;
}

std::string randString(int len, bool isCapital, bool isNum){
	const std::string STR = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::string retStr;
	/*timeval tv;
   	 gettimeofday(&tv, 0);
    	srand((int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec);*/
	if(isCapital && isNum){
		for(int i=0; i<len; i++)
		{
			char tmp = STR[rand()%(STR.length())];
			retStr.push_back(tmp);
		}
	}else if(isCapital && !isNum){
		for(int i=0; i<len; i++)
		{
			char tmp = STR[rand()%(STR.length()-10)];
			retStr.push_back(tmp);
		}
	}else if(!isCapital && isNum){
		for(int i=0; i<len; i++)
		{
			int index = rand()%(STR.length());
			if(index > 25 && index < 51){
				index += 26;
			}
			char tmp = STR[index%(STR.length())];
			retStr.push_back(tmp);
		}
	}else{
		for(int i=0; i<len; i++)
		{
			char tmp = STR[rand()%(STR.length()-36)];
			retStr.push_back(tmp);
		}
	}
	return retStr;
}

std::string sigPath(uint32_t sig1, uint32_t sig2, std::string fmt){
	/*std::map<std::string,std::string> fmt_map = {{"mp3","n"},{"wma","m"},{"mkv","v"},
												{"wmv","v"},{"ape","s"},{"flac","s"},
												{"aac","a"},{"jpg","p"},{"f4v","v"},
												{"mp4","m"},{"default","d"}
											};*/
	std::map<std::string,std::string> fmt_map;
	fmt_map.insert(std::pair<std::string,std::string>("mp3","n"));
	fmt_map.insert(std::pair<std::string,std::string>("wma","m"));
	fmt_map.insert(std::pair<std::string,std::string>("mkv","v"));
	fmt_map.insert(std::pair<std::string,std::string>("wmv","v"));
	fmt_map.insert(std::pair<std::string,std::string>("ape","s"));
	fmt_map.insert(std::pair<std::string,std::string>("flac","s"));
	fmt_map.insert(std::pair<std::string,std::string>("aac","a"));
	fmt_map.insert(std::pair<std::string,std::string>("jpg","p"));
	fmt_map.insert(std::pair<std::string,std::string>("f4v","v"));
	fmt_map.insert(std::pair<std::string,std::string>("mp4","m"));
	fmt_map.insert(std::pair<std::string,std::string>("default","d"));
	transform(fmt.begin(),fmt.end(),fmt.begin(),::tolower);
	int d1 = sig2%100;              // 1级目录
	int d2 = (sig2/100)%100;        // 2级目录
	int dv = (sig1^sig2)%3 + 1;// 分区
	std::stringstream ss;
	ss << dv;
	std::string sdv = ss.str();
	ss.str("");
	ss << d1;
	std::string sd1 = ss.str();
	ss.str("");
	ss << d2;
	std::string sd2 = ss.str();
	ss.str("");
	ss << sig1;
	std::string ssig1 = ss.str();
	ss.str("");
	std::string target_path;
	if(fmt_map.count(fmt) > 0){
		target_path = fmt_map[fmt] + sdv + "/" + sd1 + "/" + sd2 + "/" + ssig1 + "." + fmt;
	}else{
		target_path = fmt_map["default"] + sdv + "/" + sd1 + "/" + sd2 + "/" + ssig1 + "." + fmt;
	}
	return target_path;
}

std::string formatStr(const std::string fmt, ...){
	int length = 0;
	va_list	argList;

	va_start(argList, fmt.c_str());
	length = vsnprintf(NULL, 0, fmt.c_str(), argList) + 10;
	va_end(argList);

	char *buf = new char[length];
	
	va_start(argList, fmt.c_str());
	vsprintf(buf, fmt.c_str(), argList);
	va_end(argList);
	
	buf[length-1] = 0;
	std::string result = buf;
	delete [] buf;

	return result;
}

void reverse_byte_order(unsigned char* buf)
{
	unsigned char *p1, *p2, *p3, *p4;
	p1 = buf;
	p2 = buf + 1;
	p3 = buf + 2;
	p4 = buf + 3;

	unsigned char t;
	t = *p1;
	*p1 = *p4;
	*p4 = t;

	t = *p2;
	*p2 = *p3;
	*p3 = t;
}

bool calcSig(const char* path, uint32_t* nsig1, uint32_t* nsig2)
{
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		return false;
	}

	int BUFSIZE = 8192;
	char* buf = new char[BUFSIZE];
	if(buf == NULL) {
		return false;
	}

	MD5_CTX ctx;
	MD5_Init(&ctx);
    while(1) {
    	int nread = read(fd, buf, BUFSIZE);
		if(nread == 0) {
			break;
		}else if(nread < 0) {
			return false;
		}else {
			MD5_Update(&ctx, buf, nread);			
		}
    }

    close(fd);

	unsigned char* md5val = new unsigned char[16];
	MD5_Final(md5val, &ctx);
	for(int i=0;i<16;i++){
		printf("%02x", md5val[i]);
	}
	printf("\n");

	reverse_byte_order(md5val);
	reverse_byte_order(md5val+4);
	reverse_byte_order(md5val+8);
	reverse_byte_order(md5val+12);

	unsigned int* md5uint = (unsigned int*)md5val;

	//printf("%x%x%x%x\n", md5uint[0],md5uint[1],md5uint[2],md5uint[3]);
	//printf("%u,%u,%u,%u\n", md5uint[0],md5uint[1],md5uint[2],md5uint[3]);
	//printf("%u,%u\n", md5uint[0]^md5uint[1],md5uint[2]^md5uint[3]);

	*nsig1 = md5uint[0] ^ md5uint[1];
	*nsig2 = md5uint[2] ^ md5uint[3];

	delete [] buf;
	delete [] md5val;

	return true;
}

