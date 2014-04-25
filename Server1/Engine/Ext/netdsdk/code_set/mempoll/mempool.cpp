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

// 分配单位尺寸为size, 共nobjs个元素
// 这些内存在物理地址上是连在一起的, 返回其指针
char* mem_pool::chunk_alloc(size_t size, int& nobjs)
{
	char* result;
	// 总共所需的内存
	size_t total_bytes = size * nobjs;
	// 剩余的内存
	size_t bytes_left = end_free - start_free;

	// 如果剩余的内存可以满足需求, 就直接返回之, 并且更新内存池的指针
	if(bytes_left >= total_bytes)
	{
		result = start_free;
		start_free += total_bytes;
		return result;
	}

	// 如果剩余的内存大于单位内存数量, 也就是说至少还可以分配一个单位内存
	// 计算出最多可以分配多少块单位内存, 保存至nobjs, 返回内存的指针
	if(bytes_left >= size)
	{
		nobjs = (int)(bytes_left / size);
		total_bytes = size * nobjs;
		result = start_free;
		start_free += total_bytes;
		return result;
	}

	// 如果还有剩余的内存, 将它放到对应的HASH-LIST头部
	if(bytes_left > 0)
	{
		obj **my_free_list = free_list + free_list_index(bytes_left);
		((obj*)start_free)->free_list_link = *my_free_list;
		*my_free_list = (obj*)start_free;
	}

	// 需要获取的内存, 注意第一次分配都要两倍于total_bytes的大小
	// 同时要加上原有的heap_size / 4的对齐值
	size_t bytes_to_get = 2 * total_bytes + round_up(heap_size >> 4);
	start_free = (char *)malloc(bytes_to_get);

	// 获取成功 重新调用chunk_alloc函数分配内存
	if(start_free != 0)
	{
		// 更新heap_size, end_free
		heap_size += bytes_to_get;
		end_free = start_free + bytes_to_get;
		// 重新调用chunk_alloc进行分配
		return chunk_alloc(size, nobjs);
	}

	// 下面是获取不成功的处理....

	// 从下一个HASH-LIST中寻找可用的内存
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

// 重新分配尺寸为n的内存, 其中n是经过字节对齐处理的数
void* mem_pool::refill(size_t n)
{
	// 每个链表每次初始化时最多LIST_NODE_NUM个元素
	int nobjs = LIST_NODE_NUM;
	char* chunk = chunk_alloc(n, nobjs);
	obj** my_free_list;
	obj* result;
	obj *current_obj, *next_obj;
	int i;

	// 如果只请求成功了一个元素, 直接返回之
	if(1 == nobjs) 
		return chunk;
	// 获得尺寸n的HASH表地址
	my_free_list = free_list + free_list_index(n);

	// 获得请求的内存地址
	result = (obj *)chunk;
	// 请求了一个单位内存, 减少一个计数
	--nobjs;
	// 从下一个单位开始将剩余的obj连接起来
	*my_free_list = next_obj = (obj *)(chunk + n);

	// 将剩余的obj连接起来
	for(i = 1; ; ++i)
	{
		current_obj = next_obj;
		next_obj = (obj *)((char*)next_obj + n);

		// 分配完毕, 下一个节点为NULL, 退出循环
		if(nobjs == i)
		{
			current_obj->free_list_link = NULL;
			break;
		}
		current_obj->free_list_link = next_obj;
	}

	return result;
}

// 从内存池中分配尺寸为n的内存
void* mem_pool::allocate(size_t n)
{
	obj** my_free_list;
	obj* result;

	if(n <= 0) 
		return 0;
	// 如果要分配的内存大于MAX_BLOCK_SIZE, 直接调用malloc分配内存
	if(n > MAX_BLOCK_SIZE)
		return malloc(n);

	try
	{
		// 获得尺寸n的HASH表地址
		my_free_list = free_list + free_list_index(n);
		result = *my_free_list;
		
		if(NULL == result)
		{
			// 如果之前没有分配, 或者已经分配完毕了, 就调用refill函数重新分配
			// 需要注意的是, 传入refill的参数是经过对齐处理的
			result = (obj *)refill(round_up(n));
		}
		else
		{
			// 否则就更新该HASH表的LIST头节点指向下一个LIST的节点, 当分配完毕时, 头结点为NULL
			*my_free_list = result->free_list_link;
		}
	}catch(...){
		result = NULL;
	}

	return result;
}

// 将尺寸为n的内存回收到内存池中
void mem_pool::deallocate(void* p, size_t n)
{
	obj *q = (obj *)p;
	obj** my_free_list;

	// 如果要回收的内存大于MAX_BLOCK_SIZE, 直接调用free回收内存
	if(n > MAX_BLOCK_SIZE)
	{
		free(p);
		return;
	}

	// 将回收的内存作为链表的头回收
	my_free_list = free_list + free_list_index(n);

	q->free_list_link = *my_free_list;
	*my_free_list = q;
}
