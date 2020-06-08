#include "sylar/sylar.h"

#include <vector>
#include <unistd.h>

#define LOG_YAML_FILE "/home/jiaobendaye/Documents/lab/c++/bzhan-sylar/fzh/bin/conf/log2.yml"

int count = 0;

//sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void fun1() {
	SYLAR_LOG_INFO(g_logger)  << "name: " << sylar::Thread::GetName()
														<< " this.name: " << sylar::Thread::GetThis()->getName()
														<< " id: " << sylar::GetThreadId()
														<< " this.id: " << sylar::Thread::GetThis()->getId();
	for (int i = 0; i < 100000; ++i) {
		//sylar::RWMutex::WriteLock lock(s_mutex);
		sylar::Mutex::Lock lock(s_mutex);
		++count;
	}
	// sleep(20);
}

void fun2() {
	while(true) {
		SYLAR_LOG_INFO(g_logger) << "#####################################";
	}
}

void fun3() {
	while(true) {
		SYLAR_LOG_INFO(g_logger) << "=====================================";
	}
}

int main() {
	SYLAR_LOG_INFO(g_logger) << "thread test begin";
	YAML::Node root = YAML::LoadFile(LOG_YAML_FILE);
	sylar::Config::LoadFromYaml(rot);
	// sylar::Thread::ptr thr(new sylar::Thread(&fun1, "name_" + std::to_string(0)));
	// thr->join();
	std::vector<sylar::Thread::ptr> thrs;
	for(int i = 0; i < 2; i++) {
		sylar::Thread::ptr thr1(new sylar::Thread(&fun2, "name_" + std::to_string(i)));
		sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i)));
		thrs.push_back(thr1);
		thrs.push_back(thr2);
	}

	for(auto& t : thrs) {
		t->join();
	}
	SYLAR_LOG_INFO(g_logger) << "thread test end";
	SYLAR_LOG_INFO(g_logger) << "count=" << count;
	return 0;
}
