#pragma once
#include "mynet.h"
#include "pointer.h"
#include "MyTick.h"
#include "MyConnection.h"
#include <list>
class MyPool:public mynet::EventPool,public MyTick{
public:
	static MyPool& getMe()
	{
		static MyPool pool;
		return pool;
	}
	MyPool()
	{
		theTick.addTick(this);
	}
	void add(mynet::Connection *conn)
	{
		for (POINTERS_ITER iter = pointers.begin(); iter != pointers.end();++iter)
		{
			if (conn == iter->pointer()) return;
		}
		pointers.push_back(Pointer<mynet::Connection>(conn));
	}
	void remove(mynet::Connection *conn)
	{
		for (POINTERS_ITER iter = pointers.begin(); iter != pointers.end();++iter)
		{
			if (conn == iter->pointer())
			{
				pointers.erase(iter);
				return;
			}
		}
	}
	std::list<Pointer<mynet::Connection> > pointers;
	typedef std::list<Pointer<mynet::Connection> >::iterator POINTERS_ITER;

	void tick()
	{
		for (POINTERS_ITER iter = pointers.begin(); iter != pointers.end();)
		{
			MyConnection *tick = (MyConnection*)iter->pointer();
		    if (tick && tick->checkActive())
			{
				tick->tick();
			}
			++iter;
		}
	}
	~MyPool()
	{
		theTick.removeTick(this);
	}
};

#define thePool MyPool::getMe()