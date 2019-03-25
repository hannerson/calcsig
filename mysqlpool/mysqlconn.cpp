#include "mysqlconn.h"
#include <string.h>
#include <stdexcept>
#include <sstream>
#include <glog/logging.h>

namespace mysqlconn{

mysqlfield::mysqlfield(MYSQL_FIELD *mysql_field):m_value(""),m_length(0),m_inited(false),m_set(false){
	memcpy(&m_field,(void*)mysql_field, sizeof(MYSQL_FIELD));
	m_field.name = new char[strlen(mysql_field->name) + 1];
	snprintf(m_field.name, strlen(mysql_field->name) + 1, "%s", mysql_field->name);
	m_field.org_name = m_field.table = m_field.org_table = m_field.db = m_field.catalog = m_field.def = (char*)(m_field.extension = NULL);
	m_field.org_name_length = m_field.table_length = m_field.org_table_length = m_field.db_length = m_field.catalog_length = m_field.def_length = 0;
	//std::cout << "mysqlfield constructor" << std::endl;
}

mysqlfield::~mysqlfield(){
	unset_value();
	if(m_field.name){
		free(m_field.name);
		m_field.name = nullptr;
	}
	//std::cout << "mysqlfield destructor" << std::endl;
}

void mysqlfield::set_value(const std::string value, unsigned long length){
	m_value = value;
	m_length = length;
}

void mysqlfield::unset_value(){
	m_value = "";
	m_length = 0;
}

void mysqlrow::set_field(const std::string field_name, const std::string value, unsigned long length){
	if(m_fields.find(field_name) == m_fields.end()){
		//error
		std::ostringstream ostr;
		ostr << "sql field name not exists: " << field_name << std::endl;
		throw std::runtime_error(ostr.str());
	}
	m_fields[field_name]->set_value(value, length);
}

mysqlfield& mysqlrow::operator[](std::string field_name){
	if(m_fields.find(field_name) == m_fields.end()){
		//error
		std::ostringstream ostr;
		ostr << "sql field name not exists: " << field_name << std::endl;
		throw std::runtime_error(ostr.str());
	}
	return *m_fields[field_name];
}

mysqlfield& mysqlrow::operator[](unsigned int iter){
	if(iter >= m_vfields.size()){
		//error
		std::ostringstream ostr;
		ostr << "max fields num is " << m_vfields.size() << std::endl;
		throw std::runtime_error(ostr.str());
	}
	return *m_vfields[iter];
}

bool mysqlrow::field_exists(std::string field_name){
	if(m_fields.count(field_name) > 0){
		return true;
	}else{
		return false;
	}
}

mysqlfield& mysqlrow::get_value(std::string field_name){
	if(m_fields.find(field_name) == m_fields.end()){
		//error
		std::ostringstream ostr;
		ostr << "sql field name not exists: " << field_name << std::endl;
		throw std::runtime_error(ostr.str());
	}
	return *m_fields[field_name];
}

mysqlrow mysqlresult::operator[](unsigned int iter){
	if(iter >= m_rows.size()){
		//error
		std::ostringstream ostr;
		ostr << "rows is:" << m_rows.size() << ". but the iter is " << iter << std::endl;
		throw std::runtime_error(ostr.str());
	}
	return *(m_rows[iter]);
}

mysqlrow* mysqlresult::next(){
	if(cur_iter < m_rows.size()){
		cur_iter ++;
		return m_rows[cur_iter-1];
	}else{
		return nullptr;
	}
	
}

void mysqlresult::reset_iter(){
	cur_iter = 0;
}

void mysqlresult::free(){
	//std::cout << "dddd:" << &m_rows << std::endl;
	if(m_rows.size() > 0){
		for(auto it=m_rows.begin(); it!=m_rows.end(); it++){
			if(*it){
				//std::cout << "mysqlresult free" << std::endl;
				delete *it;
				*it = nullptr;
			}
		}
		m_rows.clear();
	}
}

int mysqlconnector::connect(){
	mysql = mysql_init(NULL);
	if(mysql == nullptr){
		//std::cout << "init error" << std::endl;
		//LOG(ERROR) << "init error" << std::endl;
		std::ostringstream ostr;
		ostr << "mysql init failed" << std::endl;
		throw std::runtime_error(ostr.str());
		//return -1;
	}
	if(mysql_real_connect(mysql,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,NULL,0) == nullptr){
		//std::cout << "connect error" << std::endl;
		//LOG(ERROR) << "connect error:" << mysql_error(mysql) << std::endl;
		//return -1;
		std::ostringstream ostr;
		ostr << "mysql connect failed" << std::endl;
		throw std::runtime_error(ostr.str());
	}
	//set charset
	if(mysql_set_character_set(mysql,charset.c_str())){
		//std::cout << "set character error " << mysql_error(mysql) << std::endl;
		//LOG(ERROR) << "set character error" << mysql_error(mysql) << std::endl;
		std::ostringstream ostr;
		ostr << "mysql set charset failed" << std::endl;
		throw std::runtime_error(ostr.str());
		//return -1;
	}
	return 0;
}

long mysqlconnector::execute(const std::string& sql){
	long row_count = -1;
	if(mysql_ping(mysql) != 0){
		//reconnect
		connect();
	}
	//std::cout << sql << std::endl;
	if(mysql_real_query(mysql, sql.c_str(), sql.size())){
		std::cout << "query error" << mysql_error(mysql) << std::endl;
		//LOG(ERROR) << "query error" << mysql_error(mysql) << std::endl;
		return -1;
	}
	result = new mysqlresult();
	//std::cout << "result:" << result << std::endl;
	result->m_result = mysql_store_result(mysql);
	if(result->m_result == NULL){
		std::cout << "store result " << mysql_error(mysql) << std::endl;
		//LOG(ERROR) << "store result " << mysql_error(mysql) << std::endl;
		return -1;
	}
	//std::cout << "store" << std::endl;
	if((row_count = mysql_num_rows(result->m_result)) == -1){
		std::cout << "affect rows " << mysql_error(mysql) << std::endl;
		//LOG(ERROR) << "affect rows " << mysql_error(mysql) << std::endl;
		return -1;
	}
	return row_count;
}

void mysqlconnector::close(){
	mysql_close(mysql);
}

class mysqlresult * mysqlconnector::fetchone(){
}

class mysqlresult * mysqlconnector::fetchall(){
	MYSQL_ROW row;
	MYSQL_FIELD *fields;
	int num_fields = mysql_num_fields(this->result->m_result);
	std::cout << "fields:" << num_fields << std::endl;
	fields = mysql_fetch_fields(this->result->m_result);
	while((row = mysql_fetch_row(this->result->m_result))){
		mysqlrow *sqlrow = new mysqlrow();
		unsigned long * lengths = mysql_fetch_lengths(this->result->m_result);
		for(int i=0; i<num_fields; i++){
			mysqlfield *field_tmp = new mysqlfield(fields+i);
			//std::cout << "fields:" << fields[i].name << std::endl;
			if(row[i] == nullptr){
				//std::cout << "fields value:" << "" << std::endl;
			}else{
				//std::cout << "fields value:" << row[i] << std::endl;
			}
			sqlrow->m_fields[fields[i].name] = field_tmp;
			if(row[i] == nullptr){
				sqlrow->set_field(fields[i].name,"", lengths[i]);
			}else{
				sqlrow->set_field(fields[i].name,std::string(row[i]), lengths[i]);
			}
			sqlrow->m_vfields.emplace_back(field_tmp);
		}
		this->result->m_rows.emplace_back(sqlrow);
	}
	return this->result;
}

};
