#pragma once
#include "LType.h"
#include "LCantCopy.h"
#include "LThread.h"
class LIocpRecvThread : public LThread
{
public:
	LIocpRecvThread():LThread("LIocpRecvThread"){};
	~LIocpRecvThread(){};

	void run();
};

class LIocp
{
public:
	LIocp();
	~LIocp();

	static LIocp &getInstance()
	{
		if (NULL == instance)
		{
			instance = new LIocp();
			instance->start();
		}

		return *instance;
	}
	static LIocp *instance;

	void start();

	DWORD m_dwThreadCount;

	std::vector<LIocpRecvThread*> m_RecvThreadList;
	HANDLE m_ComPort;
	void BindIocpPort(HANDLE hIo, LSocket* key);// 绑定IO端口
	void PostStatus(LSocket* key, LPOverlappedData lpOverLapped);
	void UpdateNetLog(); // 输出网络流量
};
