#include "ngx_mem_pool.h"
#include <stdexcept>

/*
   @brief: ngx_mem_pool 的构造函数，尝试创建内存池。
   @param size: 要分配的内存池的大小。
   @ret: 无返回值，但如果内存池创建失败，则会在成员变量 error_message_ 中记录错误信息。
*/
ngx_mem_pool::ngx_mem_pool(size_t size)
{
	if (nullptr == ngx_create_pool(size)) {
		this->error_message_ = "ngx_create_pool fail";
	}
}

/*
   @brief: 检查内存池是否创建成功。
   @param: void
   @ret: 无返回值。如果内存池创建失败，则抛出 std::runtime_error 异常，异常信息为创建失败时记录的错误消息。
*/
void ngx_mem_pool::check_valid() {
	if (pool_ == nullptr) {
		throw std::runtime_error(error_message_); // 在这里抛出异常
	}
}

/*
	@brief: 析构函数中销毁内存池
	@param:	void
	@ret:	无返回值
*/
ngx_mem_pool::~ngx_mem_pool() {
	ngx_destory_pool();
}


/*
   @brief: 创建一个指定大小的内存池。
   @param size: 要分配的内存池的大小（以字节为单位）。
   @ret: 返回指向创建的内存池的指针。如果内存分配失败，则返回 nullptr。
*/
void* ngx_mem_pool::ngx_create_pool(size_t size)
{
	ngx_pool_s *p;
	
	// 尝试分配指定大小的内存块
	p = (ngx_pool_s*)malloc(size);
	if (p == nullptr) {
		return nullptr;
	}

	// 初始化内存池的数据部分
	p->d.last	= (u_char *)p + sizeof(ngx_pool_s);
	p->d.end	= (u_char *)p + size;
	p->d.next	= nullptr;
	p->d.failed = 0; 

	// 计算可用的最大内存大小 最大不超过一个页面
	size = size - sizeof(ngx_pool_s);
	p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;


	// 初始化内存池的其他成员变量
	p->current = p;
	p->large   = nullptr;
	p->cleanup = nullptr;


	// 保存内存池指针到类成员变量
	this->pool_ = p;

	// 返回内存池的指针
	return p;
}


/*
   @brief: 从内存池中分配指定大小的内存块。
   @param size: 要分配的内存块的大小（以字节为单位）。
   @ret: 返回指向分配的内存块的指针。如果分配失败，则返回 nullptr。
		  根据请求的大小，分配小块或大块内存：
		  - 如果请求的内存大小小于或等于内存池的最大值，则调用 ngx_palloc_small 进行小块内存分配。
		  - 如果请求的内存大小大于内存池的最大值，则调用 ngx_palloc_large 进行大块内存分配。
*/
void* ngx_mem_pool::ngx_palloc(size_t size)
{   
	if (size <= this->pool_->max) {
		return ngx_palloc_small(size,1);
	}
	return ngx_palloc_large(size);
}


/*
   @brief: 从内存池中分配指定大小的内存块，但不进行内存对齐。
   @param size: 要分配的内存块的大小（以字节为单位）。
   @ret: 返回指向分配的内存块的指针。如果分配失败，则返回 nullptr。
		  根据请求的大小，分配小块或大块内存：
		  - 如果请求的内存大小小于或等于内存池的最大值，则调用 ngx_palloc_small 进行小块内存分配，但不对齐。
		  - 如果请求的内存大小大于内存池的最大值，则调用 ngx_palloc_large 进行大块内存分配。
*/
void *ngx_mem_pool::ngx_pnalloc(size_t size)
{
	if (size <= this->pool_->max) {
		return ngx_palloc_small(size, 0);
	}
	return ngx_palloc_large(size);
}


/*
   @brief: 从内存池中分配指定大小的内存块，并初始化为0。
   @param size: 要分配的内存块的大小（以字节为单位）。
   @ret: 返回指向分配的内存块的指针。如果分配失败，则返回 nullptr。
		  根据请求的大小，分配小块或大块内存：
		  - 如果请求的内存大小小于或等于内存池的最大值，则调用 ngx_palloc_small 进行小块内存分配。
		  - 如果请求的内存大小大于内存池的最大值，则调用 ngx_palloc_large 进行大块内存分配。
*/
void *ngx_mem_pool::ngx_pcalloc(size_t size)
{
	void *p = ngx_palloc(size);
	if (p) {
		ngx_memzero(p,size);
	}
	return p;
}


/*
   @brief: 从内存池的小块内存区域中分配指定大小的内存块，支持内存对齐。
   @param size: 要分配的内存块的大小。
   @param align: 是否进行内存对齐。非零值表示需要对齐，0 表示不对齐。
   @ret: 返回指向分配的内存块的指针。如果当前内存池中没有足够的空间，则尝试分配新的内存块，
		 如果仍然失败，则返回 nullptr。
*/
void* ngx_mem_pool::ngx_palloc_small(size_t size,ngx_uint_t align)
{
	u_char		*m;
	ngx_pool_s	*p;

	p = this->pool_->current;

	do { 
		m = p->d.last;
		

		// 如果需要对齐，则进行指针对齐
		if (align) {
			m = ngx_align_ptr(m, NGX_ALIGNMENT);
		}

		// 检查当前内存块是否有足够的空间
		if ((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;	// 更新内存块的分配的起始指针
			return m;	
		}
		// 跳到下一个内存块
		p = p->d.next;
	} while (p);

	// 更新内存块的最后指针
	return ngx_palloc_block(size);	
} 


/*
   @brief: 分配一个新的内存块，并将其添加到当前内存池中。新分配的内存块会被对齐，并且会更新当前内存池链表。
   @param size: 要分配的内存块的大小（以字节为单位）。
   @ret: 返回指向新分配内存块的指针。如果分配失败，则返回 nullptr。
		
*/
void *ngx_mem_pool::ngx_palloc_block(size_t size) 
{
	u_char		*m;
	size_t		psize;
	ngx_pool_s  *p, *newpool;


	// 计算内存池的数据区域的总大小
	psize = (size_t)(this->pool_->d.end - (u_char *)this->pool_);

	// 分配新的内存块
	m =(u_char *)malloc(psize);

	if (m == nullptr) {
		return nullptr;
	}

	// 初始化新分配的内存块
	newpool = (ngx_pool_s *)m;

	newpool->d.end	  = m + psize;
	newpool->d.next	  = nullptr;
	newpool->d.failed = 0;


	// 调整内存块的起始位置，确保对齐
	m += sizeof(ngx_pool_data_s);
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	newpool->d.last = m + size;


	// 如果分配失败四次 则将分配内存块后移
	for (p = this->pool_->current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) {
			this->pool_->current = p->d.next;
		}
	}

	// 将新分配的内存块添加到当前内存池链表中
	p->d.next = newpool;

	return m; 
}


/*
   @brief: 从内存池中分配一块大内存。
   @param size: 要分配的内存块的大小（以字节为单位）。
   @ret: 返回指向分配的内存块的指针。如果分配失败，则返回 nullptr。
*/
void *ngx_mem_pool::ngx_palloc_large(size_t size)
{
	void				*p;
	ngx_uint_t			n;
	ngx_pool_large_s	*large;

	// 分配指定大小的内存块
	p = malloc(size);
	if (p == nullptr) {
		return nullptr;
	}

	n = 0;


	// 在现有的大块内存列表中查找被释放的内存块
	for (large = this->pool_->large; large; large = large->next) {
		if (large->alloc == nullptr) {
			// 记录分配的内存块
			large->alloc = p;
			return p;
		}

		// 查找了三个以上没有空的指针 则跳出循环
		if (n++ > 3) {
			break;
		}
	}


	// 在小块内存池中 创建大块内存头信息
	large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s),1);
	if (large == nullptr) { 
		free(p);
		return nullptr;
	}


	// 记录分配的内存块，使用头插法插入列表开头
	large->alloc = p;
	large->next = this->pool_->large;
	this->pool_->large = large;

	return p;

}



/*
   @brief: 释放大块内存池中的指定内存块。函数会遍历大块内存列表，找到匹配的内存块并释放它。如果找到匹配的内存块，则将其标记为未分配，并退出函数。
		 如果内存块不在大块内存列表中，则不执行任何操作。
   @param p: 指向要释放的内存块的指针。
   @ret: 无返回值。
*/
void ngx_mem_pool::ngx_pfree(void *p)
{
	ngx_pool_large_s *l;
	for (l = this->pool_->large; l; l = l->next) {
		if (p == l->alloc) {
			free(l->alloc);
			l->alloc = nullptr;
			return;
		} 
	}
 }


/*
   @brief: 重置内存池，将其状态恢复到初始化状态。
			函数会释放所有已分配的大块内存，并重置内存池中的各个内存块的状态，使其准备好进行新的内存分配。
			 - 遍历并释放大块内存列表中的所有内存块。
			 - 重置内存池及其链表中的每个内存块的状态，包括最后分配位置和失败计数器。
			 - 将当前内存池指针设置为初始状态，并将大块内存列表置为空。
   @param: 无
   @ret: 无返回值。
*/
void ngx_mem_pool::ngx_reset_pool()
{
	ngx_pool_s			*p;
	ngx_pool_large_s	*l;

	// 释放大块内存列表中的所有内存块
	for (l = this->pool_->large; l; l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}
	  
	// 重置内存池及其链表中的每个内存块的状态
	p = this->pool_;
	p->d.last = (u_char *)p + sizeof(ngx_pool_s);
	p->d.failed = 0;
	 
	for (p = p->d.next; p; p->d.next) {
		p->d.last = (u_char *)p + sizeof(ngx_pool_data_s);
		p->d.failed = 0;
	}

	// 更新当前内存池指针和大块内存列表
	this->pool_->current = this->pool_;
	this->pool_->large = nullptr;

}


/*
   @brief: 销毁内存池，释放所有内存块及相关资源。
   @param: 无
   @ret: 无返回值。函数会执行以下操作：
		 - 调用所有清理函数，释放内存池中的资源。
		 - 释放大块内存列表中的所有内存块。
		 - 释放内存池链表中的所有内存块。
*/
void ngx_mem_pool::ngx_destory_pool()
{
	ngx_pool_s			*p, *n;
	ngx_pool_large_s	*l;
	ngx_pool_cleanup_s	*c;

	// 调用所有清理函数
	for (c = this->pool_->cleanup; c; c = c->next) {
		if (c->handler) {
			c->handler(c->data);
		}
	}

	// 释放大块内存列表中的所有内存块
	for (l = this->pool_->large; l;l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	// 释放内存池链表中的所有内存块
	for (p = this->pool_, n = this->pool_->d.next;; p = n, n = n->d.next) {
		free (p);
		if (n == nullptr) {
			break;
		}
	 }
}


/*
   @brief: 添加一个清理函数到内存池的清理列表中。
   @param size: 分配给清理函数的数据块的大小（以字节为单位）。如果 `size` 为 0，则不分配数据块。
   @ret: 返回指向新添加的清理结构体的指针。如果内存分配失败，则返回 nullptr。
		 新添加的清理结构体将会被插入到清理列表的开头。
*/
ngx_pool_cleanup_s *ngx_mem_pool::ngx_pool_cleanup_add(size_t size)
{
	ngx_pool_cleanup_s *c;

	// 分配清理结构体的内存
	c = (ngx_pool_cleanup_s*)ngx_palloc(sizeof(ngx_pool_cleanup_s));
	if (c == nullptr) {
		return nullptr;
	}

	// 如果需要，则分配数据块的内存
	if (size) {
		c->data = ngx_palloc(size);
		if (c->data == nullptr) {
			return nullptr;
		}
	}else {
		c->data = nullptr;
	}

	 // 初始化清理结构体，插入清理链表
	c->handler = nullptr;
	c->next = this->pool_->cleanup;
	this->pool_->cleanup = c;

	return c;
}
