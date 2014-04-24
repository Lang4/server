#include "mythread.h"

class MyRun:public mythread::Runnable{
public:
	MyRun()
	{
		tag = true;
	}
	void start()
	{
		tag = true;
	}
	void run()
	{
		int index = 0;
		while(isAlive())
		{
			mythread::mysleep(1);
			printf("%d\n",index++);
			if (index == 4) break;
		}
	}
	bool isAlive()
	{
		return tag;
	}
	void stop()
	{
		tag = false;
	}
	bool tag;
};

int main()
{
	mythread::Thread<MyRun> thread;
	thread.start();
//	thread.stop();
}

