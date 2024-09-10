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


	// �����ڴ��
	ngx_mem_pool pool;
	try {
		pool.check_valid(); // ����Ƿ���Ч
	}catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}

	//if (nullptr == pool.ngx_create_pool(512)) {
	//	std::cout << "ngx_create_pool fail" << std::endl;
	//	return -1;
	//}


	// С���ڴ����
	void *p1 = pool.ngx_palloc(128);
	if (nullptr == p1) {
		std::cout << "ngx_palloc 128 bytes fail" << std::endl;
		return -1;
	}

	// ����ڴ����
	stData *p2 = (stData*)pool.ngx_palloc(512);
	if (p2 == nullptr) {
		std::cout << "ngx_palloc 512 bytes fail" << std::endl;
		return -1;
	}

	//����ڴ�����ⲿ��Դ
	p2->ptr = (char*)malloc(12);
	strcpy(p2->ptr, "hello world");
	p2->pfile = fopen("data.txt", "w");


	// ����Զ����������
	ngx_pool_cleanup_s *c1 = pool.ngx_pool_cleanup_add(sizeof(char *));
	c1->handler = func1;
	c1->data = p2->ptr;

	ngx_pool_cleanup_s *c2 = pool.ngx_pool_cleanup_add(sizeof(FILE *));
	c2->handler = func2;
	c2->data = p2->pfile;

	// �����ڴ��
	//pool.ngx_destory_pool();

	return 0;
}