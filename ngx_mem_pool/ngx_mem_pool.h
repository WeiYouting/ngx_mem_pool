#pragma once
#include <stdlib.h>
#include <memory.h>
#include <string>
using u_char	 = unsigned char;
using ngx_uint_t = unsigned int;

// 把d调整为临近a的倍数
#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1)) 
// 小块内存分配考虑内存对齐时的单位
#define NGX_ALIGNMENT sizeof(unsigned long) 
//	把指针p调增到a的临近的倍数
#define ngx_align_ptr(p,a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1)) 
// 清空缓冲区
#define ngx_memzero(buf,n)  (void)memset(buf,0,n)


struct ngx_pool_s;

/*
	清空内存回调函数指针类型
*/
typedef void (*ngx_pool_cleanup_pt)(void* data);
struct ngx_pool_cleanup_s
{
	ngx_pool_cleanup_pt		handler;	//  定义函数指针，保存清理操作的回调函数
	void					*data;		//	传递给回调函数的参数
	ngx_pool_cleanup_s		*next;		//  下一个清理操作的回调函数
};


/*
	大块内存的头部信息
*/
struct ngx_pool_large_s
{
	ngx_pool_large_s	*next;		//  下一个大块内存的描述信息
	void				*alloc;		//	保存分配出去的大块内存的起始地址 
};


/*
	小块内存的内存池头部信息
*/
struct ngx_pool_data_s
{
	u_char		*last;		//	小块内存池可用内存的起始地址
	u_char		*end;		//	小块内存池可用内存的结束地址
	ngx_pool_s	*next;		//	下一个小块内存池描述信息的指针
	ngx_uint_t	failed;		//	记录当前小块内存池分配失败的次数
};

struct ngx_pool_s 
{
	ngx_pool_data_s		d;			//	存储当前小块内存池的使用情况
	size_t				max;		//	存储小块内存和大块内存的分界线
	ngx_pool_s			*current;	//  指向第一个提供小块内存池的地址 
	ngx_pool_large_s	*large;		//	指向大块内存的入口地址
	ngx_pool_cleanup_s	*cleanup;	//  指向所有清除回调函数的入口地址
};


// 默认一个物理页面大小为4K
const int ngx_pagesize = 4096;
// ngx小块内存池可分配的最大空间
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
// 默认开辟内存池的大小
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024;
// 内存池按16字节对齐
const int NGX_POOL_ALIGNMENT = 16;
// ngx小块内存的最小size调整成NGX_POOL_ALIGNMENT的临近的倍数
const int NGX_MIN_POOL_SIZE = \
	ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)), \
		NGX_POOL_ALIGNMENT);
 

/*
	移植nginx内存池
*/
class ngx_mem_pool
{
public:
	ngx_mem_pool(size_t size = 512);
	~ngx_mem_pool();


	void *ngx_create_pool(size_t size);						//	创建指定size大小的内存池，最大不超过一个页面的大小 	
	void *ngx_palloc(size_t size);							//	考虑字节对齐，从内存池申请内存
	void *ngx_pnalloc(size_t size);							//	不考虑字节对齐，从内存池申请内存
	void *ngx_pcalloc(size_t size);							//  考虑字节对齐，从内存池申请内存，并初始化为0
	void ngx_pfree(void *p);								//	释放大块内存
	void ngx_reset_pool();									//	内存重置函数
	void ngx_destory_pool();								//	内存池销毁函数
	ngx_pool_cleanup_s *ngx_pool_cleanup_add(size_t size);	//	添加清理回调函数

	void check_valid();


private:
	ngx_pool_s *pool_;											//	指向nginx内存池的入口指针
	void *ngx_palloc_small(size_t size,ngx_uint_t align);		//	小块内存分配 
	void *ngx_palloc_large(size_t size);						//  大块内存分配
	void *ngx_palloc_block(size_t size);						//	分配新的小块内存池吧


	std::string error_message_;									//	错误信息
};