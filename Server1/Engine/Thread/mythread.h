//
//  mythread.h
//  NewThread
//
//  Created by 季 金龙 on 14-4-13.
//  Copyright (c) 2014年 季 金龙. All rights reserved.
//

#ifndef NewThread_mythread_h
#define NewThread_mythread_h
#include <vector>
#include <string>
namespace mythread {
    class IMutex{
    public:
        static IMutex * create();
        virtual void lock() = 0;
        virtual void unLock() = 0;
        virtual ~IMutex(){}
    };
    class SafeLockMutex{
    public:
        SafeLockMutex(IMutex *mutex);
        ~SafeLockMutex();
    private:
        IMutex *_mutex;
    };
    class IReadWriteMutex{
    public:
        virtual void readLock() = 0;
        virtual void writeLock() = 0;
        virtual void unLock() = 0;
        virtual ~IReadWriteMutex(){}
        static IReadWriteMutex * create();
    };
    class SafeLockReadMutex{
    public:
        SafeLockReadMutex(IReadWriteMutex *mutex);
        ~SafeLockReadMutex();
    private:
        IReadWriteMutex * _mutex;
    };
    class SafeLockWriteMutex{
    public:
        SafeLockWriteMutex(IReadWriteMutex *mutex);
        ~SafeLockWriteMutex();
    private:
        IReadWriteMutex * _mutex;
    };
    class ICondition{
    public:
        static ICondition * create();
        virtual void broadcast() = 0;
        virtual void singal() = 0;
        virtual void wait(IMutex *mutex) = 0;
        virtual ~ICondition(){}
    };
    class Runnable{
    public:
        virtual void run(){}
        virtual ~Runnable(){}
		virtual bool isAlive(){return true;}
		virtual void stop() = 0;
		virtual void start(){}
    };
    class IThread{
    public:
        IThread()
        {
            _logic = NULL;
        }
        static IThread * create(const std::string &name="START",const bool joinAble = true);
        virtual bool isJoinable() = 0;
        virtual bool isAlive()  = 0;
        virtual bool start(Runnable * logic) = 0;
        virtual void join() = 0;
        virtual void final() = 0;
        virtual bool isFinal() = 0;
        virtual void run()
        {
            if (_logic) _logic->run();
        }
        virtual ~IThread(){}
    protected:
        Runnable *_logic;
    };
	template<typename LOGIC>
    class Thread{
    public:
        Thread()
        {
           _ithread = IThread::create();
        }
        void start()
		{
			_logic.start();
			_ithread->start(&_logic);
		}
        virtual ~Thread(){
			if (_ithread) delete _ithread;
			_ithread = NULL;
		}
		IThread * getThread(){
			return _ithread;
		}
		void stop()
		{
			_logic.stop();
		}
    protected:
        LOGIC _logic;
		IThread *_ithread;
    };
    class ThreadGroup{
    public:
        struct Callback{
            virtual void exec(IThread *thd) = 0;
            virtual ~Callback(){}
        };
        typedef std::vector<IThread *> Threads;
        ThreadGroup();
        ~ThreadGroup();
        void add(IThread * thd);
        IThread* getByIndex(const Threads::size_type index);
        void joinAll();
        void execAll(Callback &cb);
        const Threads::size_type size()
        {
            return vts.size();
        }
    private:
        Threads vts;
        IReadWriteMutex *_readwriteMutex;
    };
    void mysleep(const unsigned int sec);
    void myusleep(const unsigned int msec);
    
    template<class T>
    class shared_ptr{
        class _counter{
        public:
            _counter(int u,T * t):use(u),t(t){
                mutex = IMutex::create();
            }
            ~_counter(){
                delete mutex;
                mutex = NULL;
            }
            int use;
            T * t;
            IMutex * mutex;
            bool release()
            {
                if (mutex)
                {
                    SafeLockMutex scope(mutex);
                    use--;
                    if(use == 0)
                    {
                        delete t;
                        return true;
                    }
                }
                return false;
            }
            void retain()
            {
                SafeLockMutex scope(mutex);
                use++;
            }
        };
    public:
        shared_ptr(T *t):pc(new _counter(1,t)){
            this->pt = t;
            state = 0;
        }
        
        shared_ptr(const shared_ptr<T> &rhs){
            this->pc = rhs.pc;
            this->pt = rhs.pt;
            retain();
            state = 0;
        }
        void release()
        {
            if (pc && pc->release())
            {
                delete pc;
                pc = NULL;
            }
        }
        ~shared_ptr(){
            release();
        }
        void retain()
        {
            pc->retain();
        }
        shared_ptr<T>& operator=(const shared_ptr<T> rhs){
            this->pt = rhs.pt;
            this->pc = rhs.pc;
            this->state = rhs.state;
            retain();
            return *this;
        }
        T& operator *(){ return *pt; }
        T* operator ->() { return pt; }
        
        T* pointer()
        {
            return pt;
        }
        void setState(char state)
        {
            this->state = state;
        }
        bool checkState(char state)
        {
            return this->state == state;
        }
    private:
        T *pt;
        char state;
        _counter* pc;
    };
    
}

#endif
