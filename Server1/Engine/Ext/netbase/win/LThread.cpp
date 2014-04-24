/**
 * \brief ʵ����zThread
 */
#include "LThread.h"
//#include <mysql.h>
#include "algorithm"
/**
 * \brief �̺߳���
 *
 * �ں��������������߳������ʵ�ֵĻص�����
 *
 * \param arg �����̵߳Ĳ���
 * \return �����߳̽�����Ϣ
 */
DWORD WINAPI LThread::threadFunc(void *arg)
{
  LThread *thread = (LThread *)arg;

  thread->mlock.lock();
  thread->alive = true;
  thread->mlock.unlock();

  //mysql_thread_init();

  //�����̵߳����ص�����
  thread->run();

  //mysql_thread_end();

  thread->mlock.lock();
  thread->alive = false;
  thread->mlock.unlock();

  //�������joinable,��Ҫ�����߳���Դ
  if (!thread->isJoinable())
  {
    SAFE_DELETE(thread);
  }
  else
  {
    CloseHandle(thread->m_hThread);
    thread->m_hThread = NULL;
  }

  return 0;
}

/**
 * \brief �����߳�,�����߳�
 *
 * \return �����߳��Ƿ�ɹ�
 */
bool LThread::start()
{
  DWORD dwThread;

  //�߳��Ѿ���������,ֱ�ӷ���
  if (alive)
  {
    printf("�߳� %s �Ѿ���������,���ڳ��������߳�",getThreadName().c_str());
    return true;
  }

  if (NULL == (m_hThread=CreateThread(NULL,0,LThread::threadFunc,this,0,&dwThread))) 
  {
    printf("�����߳� %s ʧ��",getThreadName().c_str());
    return false;
  }

  //Zebra::logger->debug("�����߳� %s �ɹ�",getThreadName().c_str());

  return true;
}

/**
 * \brief �ȴ�һ���߳̽���
 *
 */
void LThread::join()
{
  //Zebra::logger->debug("LThread::join");
  WaitForSingleObject(m_hThread,INFINITE);
}

/**
 * \brief ���캯��
 *
 */
LThreadGroup::LThreadGroup() : vts(),rwlock()
{
}

/**
 * \brief ��������
 *
 */
LThreadGroup::~LThreadGroup()
{
  joinAll();
}

/**
 * \brief ���һ���̵߳�������
 * \param thread ����ӵ��߳�
 */
void LThreadGroup::add(LThread *thread)
{
  LRWLockScopeWrlock scope_wrlock(rwlock);
  Container::iterator it = std::find(vts.begin(),vts.end(),thread);
  if (it == vts.end())
    vts.push_back(thread);
}

/**
 * \brief ����index�±��ȡ�߳�
 * \param index �±���
 * \return �߳�
 */
LThread *LThreadGroup::getByIndex(const Container::size_type index)
{
  LRWLockScopeRdlock scope_rdlock(rwlock);
  if (index >= vts.size())
    return NULL;
  else
    return vts[index];
}

/**
 * \brief ����[]�����,����index�±��ȡ�߳�
 * \param index �±���
 * \return �߳�
 */
LThread *LThreadGroup::operator[] (const Container::size_type index)
{
  LRWLockScopeRdlock scope_rdlock(rwlock);
  if (index >= vts.size())
    return NULL;
  else
    return vts[index];
}

/**
 * \brief �ȴ������е������߳̽���
 */
void LThreadGroup::joinAll()
{
  LRWLockScopeWrlock scope_wrlock(rwlock);
  while(!vts.empty())
  {
    LThread *pThread = vts.back();
    vts.pop_back();
    if (pThread)
    {
      pThread->final();
      pThread->join();
      SAFE_DELETE(pThread);
    }
  }
}

/**
 * \brief �������е�����Ԫ�ص��ûص�����
 * \param cb �ص�����ʵ��
 */
void LThreadGroup::execAll(Callback &cb)
{
  LRWLockScopeRdlock scope_rdlock(rwlock);
  for(Container::iterator it = vts.begin(); it != vts.end(); ++it)
  {
    cb.exec(*it);
  }
}

