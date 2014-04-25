#include "mempool.h"
#include <string.h>
#include <stdlib.h>

mem_pool::mem_pool(void)
	: heap_size(0)
	, start_free(NULL)
	, end_free(NULL)
{
	memset(free_list, NULL, sizeof(free_list));
}

mem_pool::~mem_pool(void)
{
}

// ���䵥λ�ߴ�Ϊsize, ��nobjs��Ԫ��
// ��Щ�ڴ��������ַ��������һ���, ������ָ��
char* mem_pool::chunk_alloc(size_t size, int& nobjs)
{
	char* result;
	// �ܹ�������ڴ�
	size_t total_bytes = size * nobjs;
	// ʣ����ڴ�
	size_t bytes_left = end_free - start_free;

	// ���ʣ����ڴ������������, ��ֱ�ӷ���֮, ���Ҹ����ڴ�ص�ָ��
	if(bytes_left >= total_bytes)
	{
		result = start_free;
		start_free += total_bytes;
		return result;
	}

	// ���ʣ����ڴ���ڵ�λ�ڴ�����, Ҳ����˵���ٻ����Է���һ����λ�ڴ�
	// ����������Է�����ٿ鵥λ�ڴ�, ������nobjs, �����ڴ��ָ��
	if(bytes_left >= size)
	{
		nobjs = (int)(bytes_left / size);
		total_bytes = size * nobjs;
		result = start_free;
		start_free += total_bytes;
		return result;
	}

	// �������ʣ����ڴ�, �����ŵ���Ӧ��HASH-LISTͷ��
	if(bytes_left > 0)
	{
		obj **my_free_list = free_list + free_list_index(bytes_left);
		((obj*)start_free)->free_list_link = *my_free_list;
		*my_free_list = (obj*)start_free;
	}

	// ��Ҫ��ȡ���ڴ�, ע���һ�η��䶼Ҫ������total_bytes�Ĵ�С
	// ͬʱҪ����ԭ�е�heap_size / 4�Ķ���ֵ
	size_t bytes_to_get = 2 * total_bytes + round_up(heap_size >> 4);
	start_free = (char *)malloc(bytes_to_get);

	// ��ȡ�ɹ� ���µ���chunk_alloc���������ڴ�
	if(start_free != 0)
	{
		// ����heap_size, end_free
		heap_size += bytes_to_get;
		end_free = start_free + bytes_to_get;
		// ���µ���chunk_alloc���з���
		return chunk_alloc(size, nobjs);
	}

	// �����ǻ�ȡ���ɹ��Ĵ���....

	// ����һ��HASH-LIST��Ѱ�ҿ��õ��ڴ�
	int i = (int)free_list_index(size) + 1;
	obj **my_free_list, *p;
	for(; i < BLOCK_LIST_NUM;  ++i)
	{
		my_free_list = free_list + i;
		p = *my_free_list;

		if(NULL != p)
		{
			*my_free_list = p->free_list_link;
			start_free = (char *)p;
			end_free = start_free + (i + 1) * ALIGN;
			return chunk_alloc(size, nobjs);
		}
	}

	end_free = NULL;

	return NULL;
}

// ���·���ߴ�Ϊn���ڴ�, ����n�Ǿ����ֽڶ��봦�����
void* mem_pool::refill(size_t n)
{
	// ÿ������ÿ�γ�ʼ��ʱ���LIST_NODE_NUM��Ԫ��
	int nobjs = LIST_NODE_NUM;
	char* chunk = chunk_alloc(n, nobjs);
	obj** my_free_list;
	obj* result;
	obj *current_obj, *next_obj;
	int i;

	// ���ֻ����ɹ���һ��Ԫ��, ֱ�ӷ���֮
	if(1 == nobjs) 
		return chunk;
	// ��óߴ�n��HASH���ַ
	my_free_list = free_list + free_list_index(n);

	// ���������ڴ��ַ
	result = (obj *)chunk;
	// ������һ����λ�ڴ�, ����һ������
	--nobjs;
	// ����һ����λ��ʼ��ʣ���obj��������
	*my_free_list = next_obj = (obj *)(chunk + n);

	// ��ʣ���obj��������
	for(i = 1; ; ++i)
	{
		current_obj = next_obj;
		next_obj = (obj *)((char*)next_obj + n);

		// �������, ��һ���ڵ�ΪNULL, �˳�ѭ��
		if(nobjs == i)
		{
			current_obj->free_list_link = NULL;
			break;
		}
		current_obj->free_list_link = next_obj;
	}

	return result;
}

// ���ڴ���з���ߴ�Ϊn���ڴ�
void* mem_pool::allocate(size_t n)
{
	obj** my_free_list;
	obj* result;

	if(n <= 0) 
		return 0;
	// ���Ҫ������ڴ����MAX_BLOCK_SIZE, ֱ�ӵ���malloc�����ڴ�
	if(n > MAX_BLOCK_SIZE)
		return malloc(n);

	try
	{
		// ��óߴ�n��HASH���ַ
		my_free_list = free_list + free_list_index(n);
		result = *my_free_list;
		
		if(NULL == result)
		{
			// ���֮ǰû�з���, �����Ѿ����������, �͵���refill�������·���
			// ��Ҫע�����, ����refill�Ĳ����Ǿ������봦���
			result = (obj *)refill(round_up(n));
		}
		else
		{
			// ����͸��¸�HASH���LISTͷ�ڵ�ָ����һ��LIST�Ľڵ�, ���������ʱ, ͷ���ΪNULL
			*my_free_list = result->free_list_link;
		}
	}catch(...){
		result = NULL;
	}

	return result;
}

// ���ߴ�Ϊn���ڴ���յ��ڴ����
void mem_pool::deallocate(void* p, size_t n)
{
	obj *q = (obj *)p;
	obj** my_free_list;

	// ���Ҫ���յ��ڴ����MAX_BLOCK_SIZE, ֱ�ӵ���free�����ڴ�
	if(n > MAX_BLOCK_SIZE)
	{
		free(p);
		return;
	}

	// �����յ��ڴ���Ϊ�����ͷ����
	my_free_list = free_list + free_list_index(n);

	q->free_list_link = *my_free_list;
	*my_free_list = q;
}
