#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <iostream>
#include <list>
#include <set>
#include <queue>
#include <memory>
#include <assert.h>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <future>
#include <chrono>

class Task{
	public:
		void virtual run() = 0;
};

class threadPool{
	public:
		threadPool():is_stopped(false){}
		~threadPool(){is_stopped = true;}
		int init(size_t num, size_t max){
			init_num = num > 5 ? num:5;
			cur_num = init_num.load();
			used_num = 0;
			max_num = max;
			std::cout << cur_num << ":" << max << std::endl;
			for(int i=0; i<init_num.load(); i++){
				thread_pool_list.emplace_back([this](){
					thread_loop();
				});
			}
			return 0;
		}

		void thread_loop(){
			while(!is_stopped){
				int task_nums;
				class Task *task = nullptr;
				{
					std::unique_lock<std::mutex> lock(m_lock);
					cv_task.wait(lock,[this](){return is_stopped.load() || !tasks.empty();});
					if(is_stopped.load() && tasks.empty()){
						return;
					}
					task = std::move(tasks.front());
					tasks.pop();
					task_nums = tasks.size();
				}
				used_num ++;
				task->run();
				used_num --;
				{
					std::unique_lock<std::mutex> lock(m_lock);
					//adjust the num of thread : add thread
					if(!is_stopped.load() && task_nums >= cur_num - used_num && cur_num < max_num){
						//std::lock_guard<std::mutex> lock(m_lock);
						thread_pool_list.emplace_back([this](){
							thread_loop();
						});
						cur_num ++;
					}
					//adjust the num of thread : destroy thread
					if(!is_stopped.load() && task_nums < cur_num - used_num && cur_num > init_num){
						cur_num --;
						return;
					}
				}
				//std::cout << "cur num:" << cur_num << "   used num:" << used_num << "   max num:" << max_num << std::endl;
			}
		}

		void task_add(class Task *task){
			if(is_stopped){
				throw std::runtime_error("add task on threadpool which is stopped");
			}
			{
				std::lock_guard<std::mutex> lock(m_lock);
				tasks.emplace(task);
			}
			cv_task.notify_all();
		}

		void print_num(){
			std::cout << "cur num:" << cur_num << std::endl;
			std::cout << "used num:" << used_num << std::endl;
			std::cout << "max num:" << max_num << std::endl;
		}
	private:
		threadPool(const threadPool&);
		threadPool& operator = (const threadPool&);
		std::list<std::thread> thread_pool_list;
		std::queue<Task*> tasks;
		std::mutex m_lock;
		std::condition_variable cv_task;
		std::atomic<bool> is_stopped;
		std::atomic<int> init_num;
		std::atomic<int> cur_num;
		std::atomic<int> used_num;
		std::atomic<int> max_num;
};

#endif
