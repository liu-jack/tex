#ifndef _MFW_UTIL_SINGLETON_
#define _MFW_UTIL_SINGLETON_

#include <pthread.h>

namespace mfw
{

template<typename T>
class ThreadSingleton
{
public:
	static T *get()
	{
		pthread_once(&_isinited, init);
		T *t = (T *)pthread_getspecific(_key);
		if(t == NULL)
		{
			t = new T();
			set(t);
		}
		return t;
	}
	static void del()
	{
		delete get();
		set(NULL);
	}

private:
	static void set(T *t)
	{
		pthread_setspecific(_key, t);
	}

	static void destructor(void *p)
	{
		if(p)
		{
			delete (T*)p;
		}
	}

	static void init()
	{
		pthread_key_create(&_key, destructor);
	}

	static pthread_key_t _key;
	static pthread_once_t _isinited;
};

template <typename T>
pthread_key_t ThreadSingleton<T>::_key;

template <typename T>
pthread_once_t ThreadSingleton<T>::_isinited = PTHREAD_ONCE_INIT;

template<typename T>
class ProcessSingleton
{
public:
	static T *get()
	{
		pthread_once(&_isinited, init);
		return m_pInstance;
	}
private:
	static void init()
	{
		m_pInstance = new T();
	}

private:
	static T *m_pInstance;
	static pthread_once_t _isinited;
};

template <typename T>
T * ProcessSingleton<T>::m_pInstance = NULL;

template <typename T>
pthread_once_t ProcessSingleton<T>::_isinited = PTHREAD_ONCE_INIT;

template <typename T>
class CSingleton
{
public:
	static T *getInstance() { return ProcessSingleton<T>::get(); }
};

}

#endif
