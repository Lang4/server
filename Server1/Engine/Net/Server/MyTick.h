#pragma once
#include "mythread.h"
#include <list>
#include "MyTime.h"
class MyTick{
public:
	virtual bool checkActive()
	{
		if (nowTime == 0) 
		{
			nowTime = MyTime::getNowMSecs();
			return true;
		}
		else if (nowTime + lastTime <= MyTime::getNowMSecs())
		{
			nowTime = MyTime::getNowMSecs();
			return true;
		}
		return false;
	}
	virtual void tick() = 0;
	double nowTime;
	unsigned long lastTime;
	MyTick()
	{
		nowTime = lastTime = 0;
		state = 0;
	}
	MyTick(unsigned long lastTime):lastTime(lastTime)
	{
		nowTime = 0;
		state = 0;
	}
	int state;
};
template<typename LOGIC>
class TickLogic:public MyTick{
public:
	typedef void(LOGIC::*FUNC)();
	struct stLogic:public MyTick{
		FUNC func;
		stLogic()
		{
			func = NULL;
		}
		void tick(){}
	};
	std::list<stLogic> logics;
	typedef typename std::list<stLogic>::iterator TICKS_ITER;
	TickLogic(){}
	TickLogic(int lastTime):MyTick(lastTime){}
	void addTickLogic(unsigned long lastTime,FUNC func)
	{
		stLogic logic;
		logic.lastTime = lastTime;
		logic.func = func;
		logics.push_back(logic);
	}
	virtual void tick()
	{
		for (TICKS_ITER iter = logics.begin(); iter != logics.end();)
		{
			if (iter->checkActive())
			{
				(((LOGIC*)this)->*(iter->func))();
			}
			++iter;
		}
	}
};
class TickManager:public thread::Thread{
public:
	void run()
	{
		while(this->isAlive())
		{
			this->usleep(3000);
			checkNewTicks();
			for (TICKS_ITER iter = ticks.begin(); iter != ticks.end();)
			{
				MyTick *tick = *iter;
				if (tick && tick->state == -1)
				{
					iter = ticks.erase(iter);
					continue;
				}
				else if (tick && tick->checkActive())
				{
					tick->tick();
				}
				++iter;
			}
		}
	}
	std::list<MyTick*> ticks;
	std::list<MyTick*> newticks;
	typedef std::list<MyTick*>::iterator TICKS_ITER;

	static TickManager& getMe()
	{
		static TickManager me;
		return me;
	}
	void checkNewTicks()
	{
		thread::Mutex_scope_lock scope(mutex);
		for (TICKS_ITER iter = newticks.begin(); iter != newticks.end();++iter)
		{
			ticks.push_back(*iter);
		}
		newticks.clear();
	}
	void addTick(MyTick *tick)
	{
		thread::Mutex_scope_lock scope(mutex);
		newticks.push_back(tick);
	}
	void removeTick(MyTick *tick)
	{
		tick->state = -1;
	}
	thread::Mutex mutex;
};

#define theTick TickManager::getMe()