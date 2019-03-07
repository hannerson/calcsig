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

class threadPool{
	using Task = std::function<void()>;
	public:
		threadPool():is_stopped(false){}
		~threadPool(){is_stopped = true;}
		int init(size_t num, size_t max){
			init_num = num > 4 ? num:4;
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
				std::function<void()> task;
				int task_nums;
				{
					std::unique_lock<std::mutex> lock(m_lock);
					cv_task.wait(lock,[this](){return is_stopped.load() || !tasks.empty();});
					if(is_stopped && tasks.empty()){
						return;
					}
					task = std::move(tasks.front());
					tasks.pop();
					task_nums = tasks.size();
				}
				used_num ++;
				task();
				used_num --;
				//adjust the num of thread : add thread
				while(!is_stopped && task_nums > cur_num - used_num && cur_num < max_num){
					std::lock_guard<std::mutex> lock(m_lock);
					thread_pool_list.emplace_back([this](){
						thread_loop();
					});
					cur_num ++;
				}
				//adjust the num of thread : destroy thread
				if(!is_stopped && task_nums == 0 && cur_num > init_num){
					cur_num --;
					return;
				}
			}
		}

		template<class F, class ... Args>
		auto task_add(F && f, Args && ... args) -> std::future<decltype(f(args...))>{
			if(is_stopped){
				throw std::runtime_error("add task on threadpool which is stopped");
			}
			using ReType = decltype(f(args...));
			auto task = std::make_shared<std::packaged_task<ReType()> >(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);
			std::future<ReType> future = task->get_future();
			{
				std::lock_guard<std::mutex> lock(m_lock);
				tasks.emplace(
					[task](){
						(*task)();
					}
				);
			}
			cv_task.notify_all();
			return future;
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
		std::queue<Task> tasks;
		std::mutex m_lock;
		std::condition_variable cv_task;
		std::atomic<bool> is_stopped;
		std::atomic<int> init_num;
		std::atomic<int> cur_num;
		std::atomic<int> used_num;
		std::atomic<int> max_num;
};

#endif
