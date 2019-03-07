#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <atomic>
//#include "threadpool.hpp"
#include "threadpooltask.hpp"

int print1( int num ){
	std::cout << "thread:" << std::this_thread::get_id() << " ----- " << num << std::endl;
	sleep(1);
	return num;
}

class echo : public Task{
	public:
		echo():mycount(0){}
		~echo(){}
		void run(){
			std::cout << "thread:" << std::this_thread::get_id() << " ----- " << mycount << std::endl;
			sleep(1);
			//return mycount;
		}
	public:
		void increase(){
			mycount ++;
		}
	private:
		std::atomic<int> mycount;
};

int main(){
	class threadPool* pool = new threadPool();
	pool->init(5,20);

	class echo *ptr = new echo();
	for(int i=0; i<50; i++){
		pool->task_add(ptr);
		ptr->increase();
	}
	for(int i=0; i<10; i++){
		pool->print_num();
		sleep(1);
	}
	for(int i=0; i<10; i++){
		pool->task_add(ptr);
		ptr->increase();
	}
	for(int i=0; i<10; i++){
		pool->print_num();
		sleep(1);
	}

	/*class echo * test_echo = new echo();
	for(int i=0; i<10; i++){
		pool->task_add(test_echo->print_count);
	}
	for(int i=0; i<5; i++){
		pool->print_num();
		sleep(1);
	}*/
	return 0;
}
