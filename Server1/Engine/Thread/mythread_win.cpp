//
//  mythread.cpp
//  NewThread
//
//  Created by 季 金龙 on 14-4-13.
//  Copyright (c) 2014年 季 金龙. All rights reserved.
//
#include "mythread.h"
#include <windows.h>
#include "algorithm"
namespace mythread {
    class WinMutex:public IMutex{
    public:
        WinMutex()
        {
            _mutex = CreateMutex(NULL,FALSE,NULL);
        }
        virtual void lock()
        {
			if( WaitForSingleObject(_mutex,10000) == WAIT_TIMEOUT )
			{
				
			}
        }
        virtual void unLock()
        {
            ReleaseMutex(_mutex);;
        }
        ~WinMutex()
        {
            CloseHandle(_mutex);
        }
        HANDLE _mutex;
    };
    IMutex * IMutex::create()
    {
        IMutex * m = new WinMutex;
        return m;
    }
    SafeLockMutex::SafeLockMutex(IMutex *mutex):_mutex(mutex)
    {
        mutex->lock();
    }
    SafeLockMutex::~SafeLockMutex()
    {
        _mutex->unLock();
    }
    
    class WinReadWriteMutex:public IReadWriteMutex{
    public:
        WinReadWriteMutex():_read_count(0),_write_count(0)
        {
            _mutex = CreateMutex(NULL,FALSE,NULL);
        }
        virtual void readLock()
        {
            WaitForSingleObject(_mutex,INFINITE);
            ++_read_count;
        }
        virtual void writeLock()
        {
            WaitForSingleObject(_mutex,INFINITE);
            ++_write_count;
            ++_read_count;
        }
        virtual void unLock()
        {
            ReleaseMutex(_mutex);
        }
        virtual ~WinReadWriteMutex()
        {
            CloseHandle(_mutex);
        }
    private:
        unsigned int _read_count;
		unsigned int _write_count;
        HANDLE _mutex;
    };
    IReadWriteMutex * IReadWriteMutex::create()
    {
        IReadWriteMutex * mutex = new WinReadWriteMutex();
        return mutex;
    }
    SafeLockReadMutex::SafeLockReadMutex(IReadWriteMutex *mutex):_mutex(mutex)
    {
        _mutex->readLock();
    }
    SafeLockReadMutex::~SafeLockReadMutex()
    {
        _mutex->unLock();
    }
    SafeLockWriteMutex::SafeLockWriteMutex(IReadWriteMutex *mutex):_mutex(mutex)
    {
        _mutex->writeLock();
    }
    SafeLockWriteMutex::~SafeLockWriteMutex()
    {
        _mutex->unLock();
    }
    
    class WinCondition:public ICondition{
    public:
        WinCondition()
        {
            _cond = CreateEvent(NULL,FALSE,FALSE,NULL);
        }
        ~WinCondition()
        {
            CloseHandle(_cond);
        }
        void broadcast()
        {
            SetEvent(_cond);
        }
        void singal()
        {
            SetEvent(_cond);
        }
        void wait(IMutex *mutex)
        {
           WaitForSingleObject(_cond,INFINITE);
        }
    private:
        HANDLE _cond;
    };
    
    ICondition * ICondition::create()
    {
        ICondition *cond = new WinCondition();
        return cond;
    }
    
    
    class WinThread:public IThread{
    public:
        WinThread(const std::string &name,const bool joinable)
        :_threadName(name),_alive(false),_complete(false),_joinable(joinable)
        {
            _thread = 0;
        }
        ~WinThread()
        {
            join();
			if (_thread)
			{
				CloseHandle(_thread);
			}
        }
        virtual bool isJoinable()
        {
            return _joinable;
        }
        virtual bool isAlive()
        {
            return _alive;
        }
        virtual bool start(Runnable * logic)
        {
            _logic = logic;
            unsigned long dwThread;

			//线程已经创建运行,直接返回
			if (_alive)
			{
				return true;
			}

			if (NULL == (_thread=CreateThread(NULL,0,WinThread::threadFunc,this,0,&dwThread))) 
			{
			return false;
			}
			return true;
        }
        virtual void join()
        {
           WaitForSingleObject(_thread,INFINITE);
        }
        virtual void final()
        {
            _complete = true;
        }
        virtual bool isFinal()
        {
            return _complete;
        }
        static unsigned long WINAPI threadFunc(void *arg)
        {
            WinThread *thread = (WinThread *)arg;
            
            thread->_mutex.lock();
			thread->_alive = true;
			thread->_mutex.unLock();

			//运行线程的主回调函数
			thread->run();

			thread->_mutex.lock();
			thread->_alive = false;
			thread->_mutex.unLock();

			//如果不是joinable,需要回收线程资源
			if (!thread->isJoinable())
			{
				delete thread;
			}
			else
			{
				CloseHandle(thread->_thread);
				thread->_thread = NULL;
			}
			return 0;
        }
    private:
        std::string _threadName;
		WinMutex _mutex;
		volatile bool _alive;
		volatile bool _complete;
		HANDLE _thread;
		bool _joinable;
		WinCondition _cond;
    };
    IThread * IThread::create(const std::string &name,const bool joinAble)
    {
        IThread *thd = new WinThread(name,joinAble);
        return thd;
    }
    
    
    
    ThreadGroup::ThreadGroup()
	{
        _readwriteMutex = IReadWriteMutex::create();
	}
    
	ThreadGroup::~ThreadGroup()
	{
		joinAll();
        delete _readwriteMutex;
        _readwriteMutex = NULL;
	}
    
	void ThreadGroup::add(IThread *thread)
	{
		SafeLockWriteMutex scope(_readwriteMutex);
		Threads::iterator it = std::find(vts.begin(),vts.end(),thread);
		if (it == vts.end())
			vts.push_back(thread);
	}
    
	IThread *ThreadGroup::getByIndex(const Threads::size_type index)
	{
		SafeLockReadMutex scope(_readwriteMutex);
		if (index >= vts.size())
			return NULL;
		else
			return vts[index];
	}
    
	void ThreadGroup::joinAll()
	{
		SafeLockReadMutex scope(_readwriteMutex);
		while(!vts.empty())
		{
			IThread *pThread = vts.back();
			vts.pop_back();
			if (pThread)
			{
				pThread->final();
				pThread->join();
				delete (pThread);
			}
		}
	}
    
	void ThreadGroup::execAll(Callback &cb)
	{
		SafeLockReadMutex scope(_readwriteMutex);
		for(Threads::iterator it = vts.begin(); it != vts.end(); ++it)
		{
			cb.exec(*it);
		}
	}
    
    void mysleep(const unsigned int sec)
    {
        ::Sleep(1000 * sec);
    }
    
    void myusleep(const unsigned int msec)
    {
        ::Sleep(msec/1000);
    }
}
