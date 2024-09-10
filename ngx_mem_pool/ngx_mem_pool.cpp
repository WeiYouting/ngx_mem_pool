#include "ngx_mem_pool.h"
#include <stdexcept>

/*
   @brief: ngx_mem_pool �Ĺ��캯�������Դ����ڴ�ء�
   @param size: Ҫ������ڴ�صĴ�С��
   @ret: �޷���ֵ��������ڴ�ش���ʧ�ܣ�����ڳ�Ա���� error_message_ �м�¼������Ϣ��
*/
ngx_mem_pool::ngx_mem_pool(size_t size)
{
	if (nullptr == ngx_create_pool(size)) {
		this->error_message_ = "ngx_create_pool fail";
	}
}

/*
   @brief: ����ڴ���Ƿ񴴽��ɹ���
   @param: void
   @ret: �޷���ֵ������ڴ�ش���ʧ�ܣ����׳� std::runtime_error �쳣���쳣��ϢΪ����ʧ��ʱ��¼�Ĵ�����Ϣ��
*/
void ngx_mem_pool::check_valid() {
	if (pool_ == nullptr) {
		throw std::runtime_error(error_message_); // �������׳��쳣
	}
}

/*
	@brief: ���������������ڴ��
	@param:	void
	@ret:	�޷���ֵ
*/
ngx_mem_pool::~ngx_mem_pool() {
	ngx_destory_pool();
}


/*
   @brief: ����һ��ָ����С���ڴ�ء�
   @param size: Ҫ������ڴ�صĴ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ�򴴽����ڴ�ص�ָ�롣����ڴ����ʧ�ܣ��򷵻� nullptr��
*/
void* ngx_mem_pool::ngx_create_pool(size_t size)
{
	ngx_pool_s *p;
	
	// ���Է���ָ����С���ڴ��
	p = (ngx_pool_s*)malloc(size);
	if (p == nullptr) {
		return nullptr;
	}

	// ��ʼ���ڴ�ص����ݲ���
	p->d.last	= (u_char *)p + sizeof(ngx_pool_s);
	p->d.end	= (u_char *)p + size;
	p->d.next	= nullptr;
	p->d.failed = 0; 

	// ������õ�����ڴ��С ��󲻳���һ��ҳ��
	size = size - sizeof(ngx_pool_s);
	p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;


	// ��ʼ���ڴ�ص�������Ա����
	p->current = p;
	p->large   = nullptr;
	p->cleanup = nullptr;


	// �����ڴ��ָ�뵽���Ա����
	this->pool_ = p;

	// �����ڴ�ص�ָ��
	return p;
}


/*
   @brief: ���ڴ���з���ָ����С���ڴ�顣
   @param size: Ҫ������ڴ��Ĵ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ�������ڴ���ָ�롣�������ʧ�ܣ��򷵻� nullptr��
		  ��������Ĵ�С������С������ڴ棺
		  - ���������ڴ��СС�ڻ�����ڴ�ص����ֵ������� ngx_palloc_small ����С���ڴ���䡣
		  - ���������ڴ��С�����ڴ�ص����ֵ������� ngx_palloc_large ���д���ڴ���䡣
*/
void* ngx_mem_pool::ngx_palloc(size_t size)
{   
	if (size <= this->pool_->max) {
		return ngx_palloc_small(size,1);
	}
	return ngx_palloc_large(size);
}


/*
   @brief: ���ڴ���з���ָ����С���ڴ�飬���������ڴ���롣
   @param size: Ҫ������ڴ��Ĵ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ�������ڴ���ָ�롣�������ʧ�ܣ��򷵻� nullptr��
		  ��������Ĵ�С������С������ڴ棺
		  - ���������ڴ��СС�ڻ�����ڴ�ص����ֵ������� ngx_palloc_small ����С���ڴ���䣬�������롣
		  - ���������ڴ��С�����ڴ�ص����ֵ������� ngx_palloc_large ���д���ڴ���䡣
*/
void *ngx_mem_pool::ngx_pnalloc(size_t size)
{
	if (size <= this->pool_->max) {
		return ngx_palloc_small(size, 0);
	}
	return ngx_palloc_large(size);
}


/*
   @brief: ���ڴ���з���ָ����С���ڴ�飬����ʼ��Ϊ0��
   @param size: Ҫ������ڴ��Ĵ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ�������ڴ���ָ�롣�������ʧ�ܣ��򷵻� nullptr��
		  ��������Ĵ�С������С������ڴ棺
		  - ���������ڴ��СС�ڻ�����ڴ�ص����ֵ������� ngx_palloc_small ����С���ڴ���䡣
		  - ���������ڴ��С�����ڴ�ص����ֵ������� ngx_palloc_large ���д���ڴ���䡣
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
   @brief: ���ڴ�ص�С���ڴ������з���ָ����С���ڴ�飬֧���ڴ���롣
   @param size: Ҫ������ڴ��Ĵ�С��
   @param align: �Ƿ�����ڴ���롣����ֵ��ʾ��Ҫ���룬0 ��ʾ�����롣
   @ret: ����ָ�������ڴ���ָ�롣�����ǰ�ڴ����û���㹻�Ŀռ䣬���Է����µ��ڴ�飬
		 �����Ȼʧ�ܣ��򷵻� nullptr��
*/
void* ngx_mem_pool::ngx_palloc_small(size_t size,ngx_uint_t align)
{
	u_char		*m;
	ngx_pool_s	*p;

	p = this->pool_->current;

	do { 
		m = p->d.last;
		

		// �����Ҫ���룬�����ָ�����
		if (align) {
			m = ngx_align_ptr(m, NGX_ALIGNMENT);
		}

		// ��鵱ǰ�ڴ���Ƿ����㹻�Ŀռ�
		if ((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;	// �����ڴ��ķ������ʼָ��
			return m;	
		}
		// ������һ���ڴ��
		p = p->d.next;
	} while (p);

	// �����ڴ������ָ��
	return ngx_palloc_block(size);	
} 


/*
   @brief: ����һ���µ��ڴ�飬��������ӵ���ǰ�ڴ���С��·�����ڴ��ᱻ���룬���һ���µ�ǰ�ڴ������
   @param size: Ҫ������ڴ��Ĵ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ���·����ڴ���ָ�롣�������ʧ�ܣ��򷵻� nullptr��
		
*/
void *ngx_mem_pool::ngx_palloc_block(size_t size) 
{
	u_char		*m;
	size_t		psize;
	ngx_pool_s  *p, *newpool;


	// �����ڴ�ص�����������ܴ�С
	psize = (size_t)(this->pool_->d.end - (u_char *)this->pool_);

	// �����µ��ڴ��
	m =(u_char *)malloc(psize);

	if (m == nullptr) {
		return nullptr;
	}

	// ��ʼ���·�����ڴ��
	newpool = (ngx_pool_s *)m;

	newpool->d.end	  = m + psize;
	newpool->d.next	  = nullptr;
	newpool->d.failed = 0;


	// �����ڴ�����ʼλ�ã�ȷ������
	m += sizeof(ngx_pool_data_s);
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	newpool->d.last = m + size;


	// �������ʧ���Ĵ� �򽫷����ڴ�����
	for (p = this->pool_->current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) {
			this->pool_->current = p->d.next;
		}
	}

	// ���·�����ڴ����ӵ���ǰ�ڴ��������
	p->d.next = newpool;

	return m; 
}


/*
   @brief: ���ڴ���з���һ����ڴ档
   @param size: Ҫ������ڴ��Ĵ�С�����ֽ�Ϊ��λ����
   @ret: ����ָ�������ڴ���ָ�롣�������ʧ�ܣ��򷵻� nullptr��
*/
void *ngx_mem_pool::ngx_palloc_large(size_t size)
{
	void				*p;
	ngx_uint_t			n;
	ngx_pool_large_s	*large;

	// ����ָ����С���ڴ��
	p = malloc(size);
	if (p == nullptr) {
		return nullptr;
	}

	n = 0;


	// �����еĴ���ڴ��б��в��ұ��ͷŵ��ڴ��
	for (large = this->pool_->large; large; large = large->next) {
		if (large->alloc == nullptr) {
			// ��¼������ڴ��
			large->alloc = p;
			return p;
		}

		// ��������������û�пյ�ָ�� ������ѭ��
		if (n++ > 3) {
			break;
		}
	}


	// ��С���ڴ���� ��������ڴ�ͷ��Ϣ
	large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s),1);
	if (large == nullptr) { 
		free(p);
		return nullptr;
	}


	// ��¼������ڴ�飬ʹ��ͷ�巨�����б�ͷ
	large->alloc = p;
	large->next = this->pool_->large;
	this->pool_->large = large;

	return p;

}



/*
   @brief: �ͷŴ���ڴ���е�ָ���ڴ�顣�������������ڴ��б��ҵ�ƥ����ڴ�鲢�ͷ���������ҵ�ƥ����ڴ�飬������Ϊδ���䣬���˳�������
		 ����ڴ�鲻�ڴ���ڴ��б��У���ִ���κβ�����
   @param p: ָ��Ҫ�ͷŵ��ڴ���ָ�롣
   @ret: �޷���ֵ��
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
   @brief: �����ڴ�أ�����״̬�ָ�����ʼ��״̬��
			�������ͷ������ѷ���Ĵ���ڴ棬�������ڴ���еĸ����ڴ���״̬��ʹ��׼���ý����µ��ڴ���䡣
			 - �������ͷŴ���ڴ��б��е������ڴ�顣
			 - �����ڴ�ؼ��������е�ÿ���ڴ���״̬������������λ�ú�ʧ�ܼ�������
			 - ����ǰ�ڴ��ָ������Ϊ��ʼ״̬����������ڴ��б���Ϊ�ա�
   @param: ��
   @ret: �޷���ֵ��
*/
void ngx_mem_pool::ngx_reset_pool()
{
	ngx_pool_s			*p;
	ngx_pool_large_s	*l;

	// �ͷŴ���ڴ��б��е������ڴ��
	for (l = this->pool_->large; l; l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}
	  
	// �����ڴ�ؼ��������е�ÿ���ڴ���״̬
	p = this->pool_;
	p->d.last = (u_char *)p + sizeof(ngx_pool_s);
	p->d.failed = 0;
	 
	for (p = p->d.next; p; p->d.next) {
		p->d.last = (u_char *)p + sizeof(ngx_pool_data_s);
		p->d.failed = 0;
	}

	// ���µ�ǰ�ڴ��ָ��ʹ���ڴ��б�
	this->pool_->current = this->pool_;
	this->pool_->large = nullptr;

}


/*
   @brief: �����ڴ�أ��ͷ������ڴ�鼰�����Դ��
   @param: ��
   @ret: �޷���ֵ��������ִ�����²�����
		 - �����������������ͷ��ڴ���е���Դ��
		 - �ͷŴ���ڴ��б��е������ڴ�顣
		 - �ͷ��ڴ�������е������ڴ�顣
*/
void ngx_mem_pool::ngx_destory_pool()
{
	ngx_pool_s			*p, *n;
	ngx_pool_large_s	*l;
	ngx_pool_cleanup_s	*c;

	// ��������������
	for (c = this->pool_->cleanup; c; c = c->next) {
		if (c->handler) {
			c->handler(c->data);
		}
	}

	// �ͷŴ���ڴ��б��е������ڴ��
	for (l = this->pool_->large; l;l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	// �ͷ��ڴ�������е������ڴ��
	for (p = this->pool_, n = this->pool_->d.next;; p = n, n = n->d.next) {
		free (p);
		if (n == nullptr) {
			break;
		}
	 }
}


/*
   @brief: ���һ�����������ڴ�ص������б��С�
   @param size: ����������������ݿ�Ĵ�С�����ֽ�Ϊ��λ������� `size` Ϊ 0���򲻷������ݿ顣
   @ret: ����ָ������ӵ�����ṹ���ָ�롣����ڴ����ʧ�ܣ��򷵻� nullptr��
		 ����ӵ�����ṹ�彫�ᱻ���뵽�����б�Ŀ�ͷ��
*/
ngx_pool_cleanup_s *ngx_mem_pool::ngx_pool_cleanup_add(size_t size)
{
	ngx_pool_cleanup_s *c;

	// ��������ṹ����ڴ�
	c = (ngx_pool_cleanup_s*)ngx_palloc(sizeof(ngx_pool_cleanup_s));
	if (c == nullptr) {
		return nullptr;
	}

	// �����Ҫ����������ݿ���ڴ�
	if (size) {
		c->data = ngx_palloc(size);
		if (c->data == nullptr) {
			return nullptr;
		}
	}else {
		c->data = nullptr;
	}

	 // ��ʼ������ṹ�壬������������
	c->handler = nullptr;
	c->next = this->pool_->cleanup;
	this->pool_->cleanup = c;

	return c;
}
