//
//  mythread.cpp
//  NewThread
//
//  Created by 季 金龙 on 14-4-13.
//  Copyright (c) 2014年 季 金龙. All rights reserved.
//

#include "mythread.h"
#include <pthread.h>
#include <string>
#include <signal.h>
#include <unistd.h>
namespace mythread {
    class PosixMutex:public IMutex{
    public:
        PosixMutex()
        {
            pthread_mutex_init(&_mutex, NULL);
        }
        virtual void lock()
        {
            pthread_mutex_lock(&_mutex);
        }
        virtual void unLock()
        {
            pthread_mutex_unlock(&_mutex);
        }
        ~PosixMutex()
        {
            pthread_mutex_destroy(&_mutex);
        }
        pthread_mutex_t _mutex;
    };
    IMutex * IMutex::create()
    {
        IMutex * m = new PosixMutex;
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
    
    class PosixReadWriteMutex:public IReadWriteMutex{
    public:
        PosixReadWriteMutex():_read_count(0),_write_count(0)
        {
            pthread_rwlock_init(&_mutex, NULL);
        }
        virtual void readLock()
        {
            pthread_rwlock_rdlock(&_mutex);
            ++_read_count;
        }
        virtual void writeLock()
        {
            pthread_rwlock_wrlock(&_mutex);
            ++_write_count;
            ++_read_count;
        }
        virtual void unLock()
        {
            pthread_rwlock_unlock(&_mutex);
            --_read_count;
        }
        virtual ~PosixReadWriteMutex()
        {
            pthread_rwlock_destroy(&_mutex);
        }
    private:
        unsigned int _read_count;
		unsigned int _write_count;
        pthread_rwlock_t _mutex;
    };
    IReadWriteMutex * IReadWriteMutex::create()
    {
        IReadWriteMutex * mutex = new PosixReadWriteMutex();
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
    
    class PosixCondition:public ICondition{
    public:
        PosixCondition()
        {
            ::pthread_cond_init(&_cond, NULL);
        }
        ~PosixCondition()
        {
            ::pthread_cond_destroy(&_cond);
        }
        void broadcast()
        {
            ::pthread_cond_broadcast(&_cond);
        }
        void singal()
        {
            ::pthread_cond_signal(&_cond);

        }
        void wait(IMutex *mutex)
        {
            PosixMutex * pm = static_cast<PosixMutex*>(mutex);
            ::pthread_cond_wait(&_cond, &pm->_mutex);
        }
    private:
        pthread_cond_t _cond;
    };
    
    ICondition * ICondition::create()
    {
        ICondition *cond = new PosixCondition();
        return cond;
    }
    
    
    class PosixThread:public IThread{
    public:
        PosixThread(const std::string &name,const bool joinable)
        :_threadName(name),_alive(false),_complete(false),_joinable(joinable)
        {
            _thread = 0;
        }
        ~PosixThread()
        {
            join();
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
            if (_alive)
            {
                return true;
            }
            
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            if (!_joinable) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            
            if (0 != ::pthread_create(&_thread, &attr, threadFunc, this))
            {
                return false;
            }
            
            pthread_attr_destroy(&attr);
            _mutex.lock();
            while (!_alive)
                _cond.wait(&_mutex);
            _mutex.unLock();
            return true;
        }
        virtual void join()
        {
            if (0 != _thread && _joinable)
            {
                ::pthread_join(_thread, NULL);
                _thread = 0;
                _mutex.lock();
                while (_alive)
                    _cond.wait(&_mutex);
                _mutex.unLock();
            }
        }
        virtual void final()
        {
            _complete = true;
        }
        virtual bool isFinal()
        {
            return _complete;
        }
        static void* threadFunc(void *arg)
        {
            PosixThread *thread = (PosixThread *)arg;
            
            thread->_mutex.lock();
            thread->_alive = true;
            thread->_cond.broadcast();
            thread->_mutex.unLock();
            
            sigset_t sig_mask;
            sigfillset(&sig_mask);
            pthread_sigmask(SIG_SETMASK, &sig_mask, NULL);
            
            //mysql_thread_init();
            
            thread->run();
            
            //mysql_thread_end();
            
            thread->_mutex.lock();
            thread->_alive = false;
            thread->_cond.broadcast();
            thread->_mutex.unLock();
            
            if (!thread->isJoinable())
            {
                thread->_mutex.lock();
                while (thread->_alive)
                    thread->_cond.wait(&thread->_mutex);
                thread->_mutex.unLock();
                delete (thread);
            }
            
            pthread_exit(NULL);
        }
    private:
        std::string _threadName;
		PosixMutex _mutex;
		volatile bool _alive;
		volatile bool _complete;
		pthread_t _thread;
		bool _joinable;
		PosixCondition _cond;
    };
    IThread * IThread::create(const std::string &name,const bool joinAble)
    {
        IThread *thd = new PosixThread(name,joinAble);
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
        ::sleep(sec);
    }
    
    void myusleep(const unsigned int msec)
    {
        ::usleep(msec);
    }
}
