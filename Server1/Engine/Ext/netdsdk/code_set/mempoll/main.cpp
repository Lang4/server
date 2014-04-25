#include "mempool.h"
#include <string.h>
#include <stdlib.h>

// 测试程序要分配的内存大小, 可以选取大于或者小于20 * 1024分别进行测试
// 20 * 1024 这个数据是mempool.h中宏MAX_BLOCK_SIZE对应的数据
// 当大于这个数值时, 内存池调用malloc进行分配
// 当小于等于这个数据时, 内存池调用自己的内存管理程序进行分配, 比之单纯的使用malloc效率提高很多
#define TEST_BYTES 1024 * 10

int main()
{
	mem_pool objMemPoll;

	for (int i = 0; i < 90000000; ++i)
	{
#if 1
		char *p = (char*)objMemPoll.allocate(TEST_BYTES * sizeof(char));
		objMemPoll.deallocate(p, TEST_BYTES);
#else
		char *p = (char*)malloc(TEST_BYTES * sizeof(char));
		free(p);
#endif
	}
	
	return 0;
}
