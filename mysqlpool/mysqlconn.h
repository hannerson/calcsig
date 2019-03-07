#ifndef __MYSQL_CONN_H_
#define __MYSQL_CONN_H_
#include<iostream>
#include<mysql/mysql.h>
#include<unordered_map>
#include<vector>
#include<sstream>

namespace mysqlconn{

class mysqlfield{
	public:
		mysqlfield(MYSQL_FIELD *mysql_field);
		~mysqlfield();
		void set_value(const std::string value, unsigned long length);
		void unset_value();
		template <typename T>
		operator T ();
	public:
		MYSQL_FIELD m_field;
		std::string m_value;
		unsigned long m_length;
		bool m_inited;
		bool m_set;
	private:
		mysqlfield(const mysqlfield&);
		mysqlfield(mysqlfield&);
		mysqlfield& operator = (const mysqlfield&);
		mysqlfield& operator = (mysqlfield&);
};

template<typename T>
mysqlfield::operator T(){
	T value;
	if(m_value == ""){
		//error
	}
	std::stringstream ss(m_value);
	ss >> value;
	if(ss.fail() || ss.bad() || !ss.eof()){
		//error
	}
	return value;
}

class mysqlrow{
	public:
		mysqlrow(){}
		~mysqlrow(){
			for(auto it=m_vfields.begin(); it!=m_vfields.end(); it++){
				if(*it){
					delete *it;
					*it = nullptr;
				}
			}
			m_vfields.clear();
			//std::cout << "mysqlrow destructor" << std::endl;
			//LOG(INFO) << "mysqlrow destructor" << std::endl;
		}
		void set_field(const std::string field_name, const std::string value, unsigned long length);
		mysqlfield& operator[](unsigned int iter);
		mysqlfield& operator[](std::string field_name);
		mysqlfield& get_value(std::string field_name);
		bool field_exists(std::string field_name);
	public:
		std::unordered_map<std::string,mysqlfield*> m_fields;
		std::vector<mysqlfield*> m_vfields;
};

class mysqlresult{
	public:
		mysqlresult():cur_iter(0),m_result(nullptr){}
		~mysqlresult(){
			free();
			//std::cout << "mysqlresult destructor" << std::endl;
			//LOG(INFO) << "mysqlresult destructor" << std::endl;
		}
		mysqlrow operator[](unsigned int iter);
		void free();
		mysqlrow* next();
		void reset_iter();
	public:
		MYSQL_RES *m_result;
		std::vector<mysqlrow*> m_rows;
		unsigned int cur_iter;
};

class mysqlconnector{
	public:
		mysqlconnector(std::string host, unsigned int port, std::string user, std::string passwd, std::string db, std::string charset):host(host),port(port),user(user),passwd(passwd),db(db),charset(charset),result(nullptr){
			if(connect() == -1){
				//error
				//std::cout << "conn error" << std::endl;
			}
		}
		~mysqlconnector(){
			//std::cout << "mysqlconnector destructor before" << std::endl;
			//std::cout << "result:" << result << std::endl;
			if(result){
				delete result;
				result = nullptr;
			}
			//std::cout << "mysqlconnector destructor" << std::endl;
			//LOG(INFO) << "mysqlconnector destructor" << std::endl;
			close();
		}
		long execute(const std::string& sql);
		mysqlresult *fetchone();
		mysqlresult *fetchall();
	private:
		int connect();
		void close();
	private:
		MYSQL *mysql;
		std::string host;
		std::string user;
		std::string passwd;
		std::string db;
		std::string charset;
		unsigned int port;
		mysqlresult *result;
};

};
#endif
