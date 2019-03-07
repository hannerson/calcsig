#include <ctype.h>
#include <algorithm>
#include "utils.h"

std::string remove_space_punct(std::string source){
	source.erase(std::remove_if(source.begin(), source.end(), ispunct), source.end());
	source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());
	return source;
}


