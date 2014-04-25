#include "mempool.h"
#include <string.h>
#include <stdlib.h>

// ���Գ���Ҫ������ڴ��С, ����ѡȡ���ڻ���С��20 * 1024�ֱ���в���
// 20 * 1024 ���������mempool.h�к�MAX_BLOCK_SIZE��Ӧ������
// �����������ֵʱ, �ڴ�ص���malloc���з���
// ��С�ڵ����������ʱ, �ڴ�ص����Լ����ڴ���������з���, ��֮������ʹ��mallocЧ����ߺܶ�
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
