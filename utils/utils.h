#ifndef __UTILS_H__
#define __UTILS_H__

#include <ctype.h>
#include <algorithm>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>

std::string remove_space_punct(std::string source);
int mkdirs(std::string pathname, uint32_t mode);
int mvfile(std::string src, std::string dst);
std::string randString(int len, bool isCapital, bool isNum);
std::string sigPath(uint32_t sig1, uint32_t sig2, std::string fmt);
std::string formatStr(const std::string fmt, ...);
bool calcSig(const char* path, uint32_t* nsig1, uint32_t* nsig2);
#endif
