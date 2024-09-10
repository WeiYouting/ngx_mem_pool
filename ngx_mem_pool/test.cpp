#include "ngx_mem_pool.h"
#include <iostream>


struct stData {
	char *ptr;
	FILE *pfile;
};

void func1(void *p) {
	
	free((char *)p);
	std::cout << "free" << std::endl;

}

void func2(void *p) {
	
	fclose((FILE *)p);
	std::cout << "fclose" << std::endl;
}

int main() {


	// 创建内存池
	ngx_mem_pool pool;
	try {
		pool.check_valid(); // 检查是否有效
	}catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}

	//if (nullptr == pool.ngx_create_pool(512)) {
	//	std::cout << "ngx_create_pool fail" << std::endl;
	//	return -1;
	//}


	// 小块内存分配
	void *p1 = pool.ngx_palloc(128);
	if (nullptr == p1) {
		std::cout << "ngx_palloc 128 bytes fail" << std::endl;
		return -1;
	}

	// 大块内存分配
	stData *p2 = (stData*)pool.ngx_palloc(512);
	if (p2 == nullptr) {
		std::cout << "ngx_palloc 512 bytes fail" << std::endl;
		return -1;
	}

	//大块内存持有外部资源
	p2->ptr = (char*)malloc(12);
	strcpy(p2->ptr, "hello world");
	p2->pfile = fopen("data.txt", "w");


	// 添加自定义清除函数
	ngx_pool_cleanup_s *c1 = pool.ngx_pool_cleanup_add(sizeof(char *));
	c1->handler = func1;
	c1->data = p2->ptr;

	ngx_pool_cleanup_s *c2 = pool.ngx_pool_cleanup_add(sizeof(FILE *));
	c2->handler = func2;
	c2->data = p2->pfile;

	// 销毁内存池
	//pool.ngx_destory_pool();

	return 0;
}