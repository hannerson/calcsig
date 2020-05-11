#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include "threadpool_98.hpp"

class sum_task : public task_item{
	public:
		sum_task(int a, int b):a(a),b(b){}
		~sum_task(){}
	public:
		int a;
		int b;
};

class test_pool:public threadpool_98{
	public:
		test_pool(int min, int max, int max_queue):threadpool_98(min, max, max_queue){
		}
		~test_pool(){
		}
		/*void thread_loop(void *args){
			threadpool_98 *threadpool = (threadpool_98*)args;
			sum_task* task = threadpool->get_task();
			std::cout << task->a << " + " << task->b << " = " << task->a+task->b << std::endl;
			sleep(1);
		}*/
};

void test_func(void *args){
	threadpool_98 *threadpool = (threadpool_98*)args;
	while(!(threadpool->stop_status())){
	sum_task* task = (sum_task*) threadpool->get_task();
	if(task != NULL){
		std::cout << task->a << " + " << task->b << " = " << task->a+task->b << std::endl;
	}else{
		std::cout << "task null" << std::endl;
	}
	sleep(1);
	}
}

FUNC_PTR threadpool_98::fp = test_func;
int main(){
	class test_pool* pool = new test_pool(5,20,50);
	pool->start();

	class sum_task *ptr = new sum_task(2,3);
	for(int i=0; i<100; i++){
		pool->add_task(ptr);
	}
	for(int i=0; i<10; i++){
		pool->print_num();
		sleep(1);
	}
	pool->stop();
	for(int i=0; i<10; i++){
		pool->add_task(ptr);
	}

	sleep(30);
	return 0;
}
