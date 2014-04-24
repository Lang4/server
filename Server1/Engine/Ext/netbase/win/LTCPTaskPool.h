#pragma once
#include "LType.h"
#include "LCantCopy.h"
#include "LThread.h"
#include "LTime.h"
class LSyncThread;
class LRecycleThread;
/**
* \brief �����̳߳��࣬��װ��һ���̴߳��������ӵ��̳߳ؿ��
*
*/
class LTCPTaskPool : private LCantCopy
{

public:

	/**
	* \brief ���캯��
	* \param maxConns �̳߳ز��д�����Ч���ӵ��������
	* \param state ��ʼ����ʱ�������̳߳ص�״̬
	*/
	explicit LTCPTaskPool(const int maxConns,const int state,const int us=50000) : maxConns(maxConns),state(state)/*,usleep_time(us)// */
	{
		setUsleepTime(us);
		syncThread = NULL;
		recycleThread = NULL;
		maxThreadCount = minThreadCount;
	};

	/**
	* \brief ��������������һ���̳߳ض���
	*
	*/
	~LTCPTaskPool()
	{
		final();
	}

	/**
	* \brief ��ȡ�����̳߳ص�ǰ״̬
	*
	* \return ���������̳߳صĵ�ǰ״̬
	*/
	const int getState() const
	{
		return state;
	}

	/**
	* \brief ���������̳߳�״̬
	*
	* \param state ���õ�״̬���λ
	*/
	void setState(const int state)
	{
		this->state |= state;
	}

	/**
	* \brief ��������̳߳�״̬
	*
	* \param state �����״̬���λ
	*/
	void clearState(const int state)
	{
		this->state &= ~state;
	}

	const int getSize();
	inline const int getMaxConns() const { return maxConns; }
	bool addVerify(LTCPTask *task);
	void addSync(LTCPTask *task);
	bool addOkay(LTCPTask *task);
	void addRecycle(LTCPTask *task);
	static void  setUsleepTime(int time)
	{
		usleep_time=time;
	}

	bool init();
	void final();

private:

	const int maxConns;                    /**< �̳߳ز��д������ӵ�������� */

	static const int maxVerifyThreads = 4;          /**< �����֤�߳����� */
	LThreadGroup verifyThreads;                /**< ��֤�̣߳������ж�� */

	LSyncThread *syncThread;                /**< �ȴ�ͬ���߳� */

	static const int minThreadCount = 1;          /**< �̳߳���ͬʱ�����������̵߳����ٸ��� */
	int maxThreadCount;                    /**< �̳߳���ͬʱ�����������̵߳������� */
	LThreadGroup okayThreads;                /**< �������̣߳���� */

	LRecycleThread *recycleThread;              /**< ���ӻ����߳� */

	int state;                        /**< ���ӳ�״̬ */
public:
	static int usleep_time;                    /**< ѭ���ȴ�ʱ�� */

};