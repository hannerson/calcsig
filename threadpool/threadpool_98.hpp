#ifndef _THREAD_POOL_98_H
#define _THREAD_POOL_98_H

#include <pthread.h>
#include <queue>
#include <iostream>
#include <stdexcept>

typedef void (*FUNC_PTR)(void *);

class task_item{
	public:
		virtual ~task_item(){}
};

class threadpool_98{
	public:
		threadpool_98(int min, int max, int max_q){
			min_thread = min > 1 ? min:1;
			cur_thread = min_thread;
			max_thread = max > 1 ? max:1;
			max_queue = max_q;
			is_stopped = false;
			pthread_mutex_init(&mutex, NULL);
			pthread_cond_init(&cond, NULL);
		}
		~threadpool_98(){
			pthread_cond_destroy(&cond);
			pthread_mutex_destroy(&mutex);
			is_stopped = true;
		}

		class task_item* get_task(){
			//lock
			class task_item *task = NULL;
			pthread_mutex_lock(&mutex);
			while( !is_stopped && task_queue.empty()){
				pthread_cond_wait(&cond, &mutex);
			}
			if(is_stopped){
				//std::cout << "get task null" << std::endl;
				return task;
			}
			task = task_queue.front();
			task_queue.pop();
			pthread_mutex_unlock(&mutex);
			//std::cout << "get task" << std::endl;
			//unlock
			return task;
		}

		void add_task(class task_item* task){
			if(is_stopped){
				//std::cout << "thread pool is stopped" << std::endl;
				throw std::runtime_error("thread pool is stopped");
				return;
			}
			//lock
			pthread_mutex_lock(&mutex);
			//std::cout << "add task" << std::endl;
			task_queue.push(task);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
			//unlock
		}

		void adjust_thread(){
			pthread_mutex_lock(&mutex_num);
			while(!stop_status() && is_full() && cur_thread < max_thread){
				pthread_t thread;
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				//std::cout << "add thread---" << std::endl;
				if(pthread_create(&thread, &attr, &thread_loop, this) != 0){
					//failed
				}else{
					//lock
					pthread_detach(thread);
					//pthread_mutex_lock(&mutex_num);
					cur_thread ++;
					//pthread_mutex_unlock(&mutex_num);
				}
			}
			pthread_mutex_unlock(&mutex_num);

			if(!stop_status()){
				pthread_mutex_lock(&mutex_num);
				if(task_size() < (max_queue >> 1) && (cur_thread > min_thread)){
					cur_thread --;
					pthread_mutex_unlock(&mutex_num);
					return;
				}
				pthread_mutex_unlock(&mutex_num);
			}
		}

		void start(){
			for(int i=0; i<min_thread; i++){
				pthread_t thread;
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				//std::cout << i << std::endl;
				int ret = pthread_create(&thread, &attr, &thread_loop, this);
				if(ret != 0){
					//failed
					//std::cout << ret << "---" << errno << std::endl;
					throw std::runtime_error("pthread create error");
				}else{
					//lock
					pthread_detach(thread);
					//unlock
				}
			}
		}

		bool is_full(){
			bool ret = (task_queue.size() >= max_queue) ? true:false;
			//std::cout << "full---" << ret << std::endl;
			return ret;
		}

		void print_num(){
			std::cout << "cur thread num:" << cur_thread << std::endl;
			std::cout << "max queue:" << max_queue << std::endl;
			std::cout << "max thread num:" << max_thread << std::endl;
			std::cout << "task size:" << task_queue.size() << std::endl;
		}

		static void *thread_loop(void *args){
			fp(args);
			return NULL;
		}
		void stop(){
			is_stopped = true;
		}

		bool stop_status(){
			return is_stopped;
		}

		int task_size(){
			return task_queue.size();
		}
	public:
		void *server;
	private:
		static FUNC_PTR fp;
	private:
		int min_thread;
		int max_thread;
		int cur_thread;
		bool is_stopped;
		unsigned int max_queue;
		pthread_mutex_t mutex;
		pthread_mutex_t mutex_num;
		pthread_cond_t cond;
		std::queue<class task_item*> task_queue;
};

//bool threadpool_98::is_stopped = false;
#endif
