#ifndef _MFW_UTIL_THREAD_
#define _MFW_UTIL_THREAD_

#include <stdint.h>
#include <pthread.h>
#include <vector>
#include <stdexcept>
#include <tr1/memory>
#include <tr1/functional>
#include "util_noncopyable.h"
using namespace std;

namespace mfw
{

class UtilThread
{
public:
	static uint32_t getThreadId();
};

class CMutex : public noncopyable
{
public:
	CMutex();
	~CMutex();

	void lock();
	void unlock();

	friend class CNotifier;
	friend class condition_variable;

private:
	pthread_mutex_t m_mutexHandle;
};

class CNotifier : public noncopyable
{
public:
	CNotifier();
	~CNotifier();

	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }
	void signal();
	void broadcast();
	bool wait();
	bool timedwait(uint64_t ms);
	bool untilwait(uint64_t time);

private:
	pthread_cond_t m_condHandle;
	CMutex m_mutex;
};

class condition_variable: public noncopyable
{
public:
	explicit condition_variable(CMutex& mutex);
	~condition_variable();

	void notify_one();
	void notify_all();
	bool wait();
	bool wait_for(uint64_t rel_time_ms);
	bool wait_until(uint64_t abs_time_ms);

private:
	pthread_cond_t m_cond;
	CMutex& m_mutex;
};

template <typename T>
class CLockGuard : public noncopyable
{
public:
	explicit CLockGuard(T &lock) : m_lock(lock), m_bOwned(true) { m_lock.lock(); }
	~CLockGuard() { unlock(); }

	void lock()
	{
		if (!m_bOwned)
		{
			m_lock.lock();
			m_bOwned = true;
		}
	}

	void unlock()
	{
		if (m_bOwned)
		{
			m_lock.unlock();
			m_bOwned = false;
		}
	}

private:
	T &m_lock;
	bool m_bOwned;
};

class CRWLock
{
public:
    CRWLock();
    ~CRWLock();

    void rlock();
    void wlock();
    void unlock();

    class ReadLock
    {
    public:
        explicit ReadLock(CRWLock &rw, bool bAcquired = false)
            : m_rw(rw), m_bAcquired(bAcquired) { acquire(); }
        ~ReadLock() { release(); }
        void acquire()
        {
            if (!m_bAcquired)
            {
                m_rw.rlock();
                m_bAcquired = true;
            }
        }
        void release()
        {
            if (m_bAcquired)
            {
                m_rw.unlock();
                m_bAcquired = false;
            }
        }
    private:
        CRWLock &m_rw;
        bool m_bAcquired;
    };

    class WriteLock
    {
    public:
        explicit WriteLock(CRWLock &rw, bool bAcquired = false)
            : m_rw(rw), m_bAcquired(bAcquired) { acquire(); }
        ~WriteLock() { release(); }
        void acquire()
        {
            if (!m_bAcquired)
            {
                m_rw.wlock();
                m_bAcquired = true;
            }
        }
        void release()
        {
            if (m_bAcquired)
            {
                m_rw.unlock();
                m_bAcquired = false;
            }
        }
    private:
        CRWLock &m_rw;
        bool m_bAcquired;
    };
private:
    pthread_rwlock_t m_rwlockHandle;
};

class CThread : public noncopyable
{
public:
	explicit CThread(const string &sId) : m_sIdentifier(sId), m_threadHandle(), m_bIsRunning(false), m_bTerminateFlag(false), m_iInterval(0) {}
	~CThread();
	void setIdentifier(const string &sId) { m_sIdentifier = sId; }
	void setRoutine(tr1::function<void(CThread &)> routine) { m_routine = routine; }
	void setCleanUp(tr1::function<void(CThread &)> cleanup) { m_cleanup = cleanup; }
	void setIntervalRoutine(tr1::function<void(CThread &)> routine, uint32_t iInterval) { m_routine = routine; m_iInterval = iInterval; }
	void setTerminate(tr1::function<void()> terminate) { m_terminate = terminate; }
	bool isRunning() const { return m_bIsRunning; }
	bool isTerminate() const { return m_bTerminateFlag; }
	void start();
	void terminate();
	void join();
	bool wait();
	bool timedwait(uint64_t ms);

protected:
	static void *threadEntry(void *arg);

protected:
	string m_sIdentifier;
	pthread_t m_threadHandle;
	bool m_bIsRunning;
	bool m_bTerminateFlag;
	uint32_t m_iInterval;
	tr1::function<void(CThread &)> m_routine;
	tr1::function<void(CThread &)> m_cleanup;
	tr1::function<void()> m_terminate;
	CNotifier m_notifier;
};

typedef tr1::shared_ptr<CThread> CThreadPtr;

class CThreadBase
{
	CThread m_thread;
public:
	CThreadBase() : m_thread("")
	{
		m_thread.setRoutine(tr1::bind(&CThreadBase::run, this));
	}
	virtual ~CThreadBase() {}

	virtual void run() = 0;

	void start(const string &sIdentifier)
	{
		m_thread.setIdentifier(sIdentifier);
		m_thread.start();
	}

	CThread &getThread() { return m_thread; }
};

class CThreadPool : public noncopyable
{
public:
	explicit CThreadPool(const string &sId) :  m_sIdentifier(sId), m_iThreadNum(1) {}
	~CThreadPool();
	void setThreadNum(uint32_t iThreadNum) { m_iThreadNum = iThreadNum; }
	void setRoutine(tr1::function<void(CThread &)> routine) { m_routine = routine; }
	void setTerminate(tr1::function<void()> terminate) { m_terminate = terminate; }
	void start();
	void terminate();
	void join();

protected:
	string m_sIdentifier;
	uint32_t m_iThreadNum;
	tr1::function<void(CThread &)> m_routine;
	tr1::function<void()> m_terminate;
	vector<CThreadPtr> m_vThread;
};

}

#endif
