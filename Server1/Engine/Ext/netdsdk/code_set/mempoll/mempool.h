#ifndef mem_pool_h
#define mem_pool_h

#include <stdio.h>

// �ֽڶ�����
#define ALIGN				512
// ���BLOCK�ĳߴ�
#define MAX_BLOCK_SIZE		20 * 1024
// HASH�������
#define BLOCK_LIST_NUM		MAX_BLOCK_SIZE / ALIGN
// HASH����ÿ�γ�ʼ��ʱLIST��Ԫ�ص�����
#define LIST_NODE_NUM		20

class mem_pool
{
public:
	mem_pool(void);
	~mem_pool(void);

	void* allocate(size_t n);
	void deallocate(void* p, size_t n);
	inline size_t mem_size(){return heap_size;}

private:
	union obj
	{
		union obj* free_list_link;
		char client_data[1];
	};
	// HASH-LIST, ��BLOCK_LIST_NUM��HASH��
	obj* free_list [BLOCK_LIST_NUM];

	static inline size_t round_up(size_t bytes)
	{
		return (bytes + ALIGN - 1) & ~(ALIGN - 1);
	}

	static inline size_t free_list_index(size_t bytes)
	{
		return (bytes + ALIGN - 1) / ALIGN - 1;
	}

	char* start_free;
	char* end_free;
	size_t heap_size;

	char* chunk_alloc(size_t size, int& nobjs);
	void* refill(size_t n);
};

#endif
