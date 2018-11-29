#include "util_thread.h"
#include "util_singleton.h"
#include "util_time.h"
#include "util_string.h"
#include "util_log.h"
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace mfw
{

struct LocalThreadIdStorage
{
	uint32_t iThreadId;

	LocalThreadIdStorage() : iThreadId(0) {}
};

uint32_t UtilThread::getThreadId()
{
	LocalThreadIdStorage *pLocalThreadIdStorage = ThreadSingleton<LocalThreadIdStorage>::get();
	if (pLocalThreadIdStorage->iThreadId == 0)
	{
		pLocalThreadIdStorage->iThreadId = syscall(SYS_gettid);
	}
	return pLocalThreadIdStorage->iThreadId;
}

CMutex::CMutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	int ret = pthread_mutex_init(&m_mutexHandle, &attr);
	pthread_mutexattr_destroy(&attr);
	if (ret != 0)
	{
		throw std::runtime_error("pthread_mutex_init");
	}
}

CMutex::~CMutex()
{
	pthread_mutex_destroy(&m_mutexHandle);
}

void CMutex::lock()
{
	int ret = pthread_mutex_lock(&m_mutexHandle);
    if (ret != 0)
	{
		throw std::runtime_error("pthread_mutex_lock:" + UtilString::tostr(ret));
	}
}

void CMutex::unlock()
{
	if (pthread_mutex_unlock(&m_mutexHandle) != 0)
	{
		throw std::runtime_error("pthread_mutex_unlock");
	}
}

CNotifier::CNotifier()
{
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	int ret = pthread_cond_init(&m_condHandle, &attr);
	pthread_condattr_destroy(&attr);
	if (ret != 0)
	{
		throw std::runtime_error("pthread_cond_init");
	}
}

CNotifier::~CNotifier()
{
	pthread_cond_destroy(&m_condHandle);
}

void CNotifier::signal()
{
	pthread_cond_signal(&m_condHandle);
}

void CNotifier::broadcast()
{
	pthread_cond_broadcast(&m_condHandle);
}

bool CNotifier::wait()
{
	int ret = pthread_cond_wait(&m_condHandle, &m_mutex.m_mutexHandle);
	if (ret != 0)
	{
		return false;
	}
	return true;
}

bool CNotifier::timedwait(uint64_t ms)
{
	ms += UtilTime::getNowMS();
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = ms % 1000 * 1000 * 1000;
	int ret = pthread_cond_timedwait(&m_condHandle, &m_mutex.m_mutexHandle, &ts);
	if (ret != 0 && ret != ETIMEDOUT)
	{
		return false;
	}
	return true;
}

bool CNotifier::untilwait(uint64_t time)
{
	struct timespec ts;
	ts.tv_sec = time / 1000;
	ts.tv_nsec = time % 1000 * 1000 * 1000;
	int ret = pthread_cond_timedwait(&m_condHandle, &m_mutex.m_mutexHandle, &ts);
	if (ret != 0 && ret != ETIMEDOUT)
	{
		return false;
	}
	return true;
}

condition_variable::condition_variable(CMutex& mutex)
	: m_mutex(mutex) {
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	int ret = pthread_cond_init(&m_cond, &attr);
	pthread_condattr_destroy(&attr);
	if (ret != 0) {
		throw std::runtime_error("pthread_cond_init");
	}
}

condition_variable::~condition_variable() {
	pthread_cond_destroy(&m_cond);
}

void condition_variable::notify_one() {
	pthread_cond_signal(&m_cond);
}

void condition_variable::notify_all() {
	pthread_cond_broadcast(&m_cond);
}

bool condition_variable::wait() {
	int ret = pthread_cond_wait(&m_cond, &m_mutex.m_mutexHandle);
	if (ret != 0) {
		return false;
	}
	return true;
}

bool condition_variable::wait_for(uint64_t rel_time_ms) {
	rel_time_ms += UtilTime::getNowMS();
	struct timespec ts;
	ts.tv_sec = rel_time_ms / 1000;
	ts.tv_nsec = rel_time_ms % 1000 * 1000 * 1000;
	int ret = pthread_cond_timedwait(&m_cond, &m_mutex.m_mutexHandle, &ts);
	if (ret != 0 && ret != ETIMEDOUT) {
		return false;
	}
	return true;
}

bool condition_variable::wait_until(uint64_t abs_time_ms) {
	struct timespec ts;
	ts.tv_sec = abs_time_ms / 1000;
	ts.tv_nsec = abs_time_ms % 1000 * 1000 * 1000;
	int ret = pthread_cond_timedwait(&m_cond, &m_mutex.m_mutexHandle, &ts);
	if (ret != 0 && ret != ETIMEDOUT) {
		return false;
	}
	return true;
}

CRWLock::CRWLock()
{
	pthread_rwlock_init(&m_rwlockHandle, NULL);
}

CRWLock::~CRWLock()
{
	pthread_rwlock_destroy(&m_rwlockHandle);
}

void CRWLock::rlock()
{
	int rc = pthread_rwlock_rdlock(&m_rwlockHandle);
	if (rc != 0)
	{
		throw std::runtime_error("pthread_rwlock_rdlock");
	}
}

void CRWLock::wlock()
{
	int rc = pthread_rwlock_wrlock(&m_rwlockHandle);
	if (rc != 0)
	{
		throw std::runtime_error("pthread_rwlock_wrlock");
	}
}

void CRWLock::unlock()
{
	int rc = pthread_rwlock_unlock(&m_rwlockHandle);
	if (rc != 0)
	{
		throw std::runtime_error("pthread_rwlock_unlock");
	}
}

CThread::~CThread()
{
	if (isRunning())
	{
		terminate();
		join();
	}
}

void CThread::start()
{
	if (isRunning() || !m_routine)
	{
		return;
	}

	m_bIsRunning = true;
	m_bTerminateFlag = false;
	if (pthread_create(&m_threadHandle, NULL, threadEntry, this) != 0)
	{
		m_bIsRunning = false;
		throw std::runtime_error("pthread_create");
	}
	MFW_DEBUG("start thread " << m_sIdentifier << ": " << m_threadHandle);
}

void CThread::terminate()
{
	if (!isRunning() || isTerminate())
	{
		return;
	}

	m_bTerminateFlag = true;
	m_notifier.broadcast();
	if (m_terminate)
	{
		m_terminate();
	}
}

void CThread::join()
{
    if (!isRunning()) return;

	void *retval = NULL;
	pthread_join(m_threadHandle, &retval);

	m_bIsRunning = false;
}

bool CThread::wait()
{
	CLockGuard<CNotifier> lock(m_notifier);
	if (isTerminate())
	{
		return false;
	}
	return m_notifier.wait();
}
bool CThread::timedwait(uint64_t ms)
{
	CLockGuard<CNotifier> lock(m_notifier);
	if (isTerminate())
	{
		return false;
	}
	return m_notifier.timedwait(ms);
}

void *CThread::threadEntry(void *arg)
{
	CThread *thread = static_cast<CThread *>(arg);
	try
	{
		if (thread->m_iInterval == 0)
		{
			thread->m_routine(*thread);
		}
		else
		{
			while (!thread->isTerminate())
			{
				thread->timedwait(thread->m_iInterval);
				if (thread->isTerminate())
				{
					break;
				}
				thread->m_routine(*thread);
			}
		}
		if (thread->m_cleanup)
		{
			thread->m_cleanup(*thread);	
		}
	}
	catch (std::exception &e)
	{
		MFW_ERROR("thread exit with exception: " << thread->m_sIdentifier << "," << thread->m_threadHandle << "," << e.what());
	}
	catch (...)
	{
		MFW_ERROR("thread exit with unknown exception: " << thread->m_sIdentifier << "," << thread->m_threadHandle);
	}
	MFW_DEBUG("end thread " << thread->m_sIdentifier << ": " << thread->m_threadHandle);
	return NULL;
}

CThreadPool::~CThreadPool()
{
	terminate();
	join();

    m_vThread.clear();
}

void CThreadPool::start()
{
	if (!m_vThread.empty())
	{
		return;
	}

	for (uint32_t i = 0; i < m_iThreadNum; ++i)
	{
		CThreadPtr thread(new CThread(m_sIdentifier + "-" + UtilString::tostr(i)));
		thread->setRoutine(m_routine);
		thread->start();
		m_vThread.push_back(thread);
	}
}

void CThreadPool::terminate()
{
	for (unsigned i = 0; i < m_vThread.size(); ++i)
	{
		m_vThread[i]->terminate();
	}
	if (m_terminate)
	{
		m_terminate();
	}
}

void CThreadPool::join()
{
	for (unsigned i = 0; i < m_vThread.size(); ++i)
	{
		m_vThread[i]->join();
	}
}

}
