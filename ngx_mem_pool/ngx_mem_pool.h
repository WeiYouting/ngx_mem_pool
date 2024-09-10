#pragma once
#include <stdlib.h>
#include <memory.h>
#include <string>
using u_char	 = unsigned char;
using ngx_uint_t = unsigned int;

// ��d����Ϊ�ٽ�a�ı���
#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1)) 
// С���ڴ���俼���ڴ����ʱ�ĵ�λ
#define NGX_ALIGNMENT sizeof(unsigned long) 
//	��ָ��p������a���ٽ��ı���
#define ngx_align_ptr(p,a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1)) 
// ��ջ�����
#define ngx_memzero(buf,n)  (void)memset(buf,0,n)


struct ngx_pool_s;

/*
	����ڴ�ص�����ָ������
*/
typedef void (*ngx_pool_cleanup_pt)(void* data);
struct ngx_pool_cleanup_s
{
	ngx_pool_cleanup_pt		handler;	//  ���庯��ָ�룬������������Ļص�����
	void					*data;		//	���ݸ��ص������Ĳ���
	ngx_pool_cleanup_s		*next;		//  ��һ����������Ļص�����
};


/*
	����ڴ��ͷ����Ϣ
*/
struct ngx_pool_large_s
{
	ngx_pool_large_s	*next;		//  ��һ������ڴ��������Ϣ
	void				*alloc;		//	��������ȥ�Ĵ���ڴ����ʼ��ַ 
};


/*
	С���ڴ���ڴ��ͷ����Ϣ
*/
struct ngx_pool_data_s
{
	u_char		*last;		//	С���ڴ�ؿ����ڴ����ʼ��ַ
	u_char		*end;		//	С���ڴ�ؿ����ڴ�Ľ�����ַ
	ngx_pool_s	*next;		//	��һ��С���ڴ��������Ϣ��ָ��
	ngx_uint_t	failed;		//	��¼��ǰС���ڴ�ط���ʧ�ܵĴ���
};

struct ngx_pool_s 
{
	ngx_pool_data_s		d;			//	�洢��ǰС���ڴ�ص�ʹ�����
	size_t				max;		//	�洢С���ڴ�ʹ���ڴ�ķֽ���
	ngx_pool_s			*current;	//  ָ���һ���ṩС���ڴ�صĵ�ַ 
	ngx_pool_large_s	*large;		//	ָ�����ڴ����ڵ�ַ
	ngx_pool_cleanup_s	*cleanup;	//  ָ����������ص���������ڵ�ַ
};


// Ĭ��һ������ҳ���СΪ4K
const int ngx_pagesize = 4096;
// ngxС���ڴ�ؿɷ�������ռ�
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
// Ĭ�Ͽ����ڴ�صĴ�С
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024;
// �ڴ�ذ�16�ֽڶ���
const int NGX_POOL_ALIGNMENT = 16;
// ngxС���ڴ����Сsize������NGX_POOL_ALIGNMENT���ٽ��ı���
const int NGX_MIN_POOL_SIZE = \
	ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)), \
		NGX_POOL_ALIGNMENT);
 

/*
	��ֲnginx�ڴ��
*/
class ngx_mem_pool
{
public:
	ngx_mem_pool(size_t size = 512);
	~ngx_mem_pool();


	void *ngx_create_pool(size_t size);						//	����ָ��size��С���ڴ�أ���󲻳���һ��ҳ��Ĵ�С 	
	void *ngx_palloc(size_t size);							//	�����ֽڶ��룬���ڴ�������ڴ�
	void *ngx_pnalloc(size_t size);							//	�������ֽڶ��룬���ڴ�������ڴ�
	void *ngx_pcalloc(size_t size);							//  �����ֽڶ��룬���ڴ�������ڴ棬����ʼ��Ϊ0
	void ngx_pfree(void *p);								//	�ͷŴ���ڴ�
	void ngx_reset_pool();									//	�ڴ����ú���
	void ngx_destory_pool();								//	�ڴ�����ٺ���
	ngx_pool_cleanup_s *ngx_pool_cleanup_add(size_t size);	//	�������ص�����

	void check_valid();


private:
	ngx_pool_s *pool_;											//	ָ��nginx�ڴ�ص����ָ��
	void *ngx_palloc_small(size_t size,ngx_uint_t align);		//	С���ڴ���� 
	void *ngx_palloc_large(size_t size);						//  ����ڴ����
	void *ngx_palloc_block(size_t size);						//	�����µ�С���ڴ�ذ�


	std::string error_message_;									//	������Ϣ
};