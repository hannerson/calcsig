#ifndef _OBJECTPOOL_H_
#define _OBJECTPOOL_H_
#include <iostream>
#include <list>
#include <set>
#include <queue>
#include <memory>
#include <assert.h>
#include <thread>
#include <mutex>
template <typename T>
void Myprint(T head){
	std::cout << head << std::endl;
}

template <typename T, typename ... Args>
void Myprint(T head,Args ... args){
	std::cout << head << ",";
	Myprint(args...);
}

template <typename T, typename ... Args>
class objectPool{
	public:
		objectPool():free_object(false){}
		~objectPool(){free_object = true;}
		int init(size_t num, size_t max, Args ... args){
			init_num = num;
			cur_num = num;
			used_num = 0;
			//assert(init_num >= 4);
			max_num = max;
			std::cout << cur_num << ":" << max << std::endl;
			//Myprint(args ...);
			for(int i=0; i<init_num; i++){
				Myprint(args ...);
				user_object_list.emplace_back(std::shared_ptr<T>(new T(args ...), [this](T *t){
					auto_create_ptr(t);
				}));
			}
		}

		void auto_create_ptr(T *t){
			if(free_object){
				if(t){
					//std::cout << "delete before" << std::endl;
					delete t;
					t = nullptr;
				}
			}else{
				std::lock_guard<std::mutex> lock(list_mutex);
				if(cur_num - used_num > init_num){
					if(t){
						delete t;
						t = nullptr;
					}
					cur_num --;
				}else{
					user_object_list.emplace_back(std::shared_ptr<T>(t));
				}
				used_num --;
			}
		}

		std::shared_ptr<T> get(Args ... args){
			std::lock_guard<std::mutex> lock(list_mutex);
			if(!user_object_list.empty()){
				//LOG(INFO) << "object ok" << std::endl;
				auto ptr = user_object_list.front();
				user_object_list.remove(ptr);
				used_num ++;
				return ptr;
			}else{
				if(cur_num >= max_num){
					return nullptr;
				}else{
					if(cur_num * 2 <= max_num){
						Myprint(args ...);
						expand_list(cur_num, args ...);
						cur_num += cur_num;
					}else{
						expand_list(max_num - cur_num, args ...);
						cur_num = max_num;
					}
					auto ptr = user_object_list.front();
					user_object_list.remove(ptr);
					used_num ++;
					return ptr;
				}
			}
		}
		void print_num(){
			std::cout << "cur num:" << cur_num << std::endl;
			std::cout << "used num:" << used_num << std::endl;
			std::cout << "max num:" << max_num << std::endl;
		}
	private:
		void expand_list(size_t num, Args ... args){
			for(int i=0; i<num; i++){
				Myprint(args ...);
				user_object_list.emplace_back(std::shared_ptr<T>(new T(args ...), [this](T *t){
					auto_create_ptr(t);
				}));
			}
		}
	private:
		objectPool(const objectPool&);
		objectPool& operator = (const objectPool&);
		std::list<std::shared_ptr<T>> user_object_list;
		std::mutex list_mutex;
		bool free_object;
		int init_num;
		int cur_num;
		int used_num;
		int max_num;
};

#endif
