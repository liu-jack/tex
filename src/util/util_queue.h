#ifndef _MFW_UTIL_QUEUE_
#define _MFW_UTIL_QUEUE_

#include "util_thread.h"
#include <stdint.h>
#include <queue>
#include <set>
#include <tr1/unordered_map>
using namespace std;
using std::tr1::unordered_map;

namespace mfw
{

template <typename T>
class CThreadQueue
{
public:
    bool dequeue(T &t, int64_t ms = -1)
    {
        CLockGuard<CNotifier> lock(m_notifier);
        if (m_queue.empty()) {
            if (ms == 0) {
                return false;
            } else if (ms < 0) {
                m_notifier.wait();
            } else {
                m_notifier.timedwait(ms);
            }
        }
        if (m_queue.empty()) {
            return false;
        }
        swap(m_queue.front(), t);
        m_queue.pop();
        return true;
    }

    void enqueue(const T &t)
    {
        CLockGuard<CNotifier> lock(m_notifier);
        m_queue.push(t);
        m_notifier.signal();
    }

    uint32_t size()
    {
        CLockGuard<CNotifier> lock(m_notifier);
        return m_queue.size();
    }

    void wakeup()
    {
        CLockGuard<CNotifier> lock(m_notifier);
        m_notifier.broadcast();
    }

private:
    queue<T> m_queue;
    CNotifier m_notifier;
};

template <typename T, typename TimeType, typename TimeCompare = std::less<TimeType>, typename Hash = tr1::hash<T> >
class CTimeQueue
{
    struct TimeData {
        pair<T, TimeType> pairKeyTime;
        uint32_t	iSalt;

        bool operator==(const TimeData &b) const
        {
            return pairKeyTime == b.pairKeyTime && iSalt == b.iSalt;
        }
    };

    struct TimeDataCompare {
        TimeCompare stTimeCompare;
        bool operator()(const TimeData &a, const TimeData &b) const
        {
            if (stTimeCompare(a.pairKeyTime.second, b.pairKeyTime.second)) {
                return true;
            }
            if (stTimeCompare(b.pairKeyTime.second, a.pairKeyTime.second)) {
                return false;
            }
            return a.iSalt < b.iSalt;
        }
    };

    typedef unordered_map<T, TimeData, Hash> TimeDataType;
    typedef multiset<TimeData, TimeDataCompare> TimeQueueType;

    struct CTimeQueueIterator {
        typedef pair<T, TimeType> value_type;
        typedef const value_type & reference;
        typedef const value_type * pointer;
        typedef CTimeQueueIterator _Self;

        typename TimeQueueType::const_iterator _it;
        explicit CTimeQueueIterator(typename TimeQueueType::const_iterator it) : _it(it) {}

        reference operator*() const
        {
            return _it->pairKeyTime;
        }
        pointer operator->() const
        {
            return &_it->pairKeyTime;
        }
        _Self &operator++()
        {
            ++_it;
            return *this;
        }
        _Self operator++(int)
        {
            _Self tmp = *this;
            ++_it;
            return tmp;
        }
        _Self &operator--()
        {
            --_it;
            return *this;
        }
        _Self operator--(int)
        {
            _Self tmp = *this;
            --_it;
            return tmp;
        }
        bool operator==(const _Self &x)
        {
            return _it == x._it;
        }
        bool operator!=(const _Self &x)
        {
            return _it != x._it;
        }
    };

public:
    typedef CTimeQueueIterator const_iterator;

public:

    CTimeQueue()
        : m_iLastSalt(0)
    {}

    bool contains(const T &key)
    {
        return m_mTimeData.find(key) != m_mTimeData.end();
    }

    void add(const T &key, const TimeType &time)
    {
        typename TimeDataType::iterator it = m_mTimeData.find(key);
        if (it != m_mTimeData.end()) {
            TimeData &data = it->second;
            if (data.pairKeyTime.second != time) {
                removeTimeQueue(data);
                data.pairKeyTime.second = time;
                data.iSalt = getNewSalt();
                addTimeQueue(data);
            }
        } else {
            TimeData &data = m_mTimeData[key];
            data.pairKeyTime.first = key;
            data.pairKeyTime.second = time;
            data.iSalt = getNewSalt();
            addTimeQueue(data);
        }
    }

    void del(const T &key)
    {
        typename TimeDataType::iterator it = m_mTimeData.find(key);
        if (it != m_mTimeData.end()) {
            TimeData &data = it->second;
            removeTimeQueue(data);
            m_mTimeData.erase(it);
        }
    }

    const_iterator begin() const
    {
        return const_iterator(m_setTimeQueue.begin());
    }
    const_iterator end() const
    {
        return const_iterator(m_setTimeQueue.end());
    }
    bool empty() const
    {
        return m_setTimeQueue.empty();
    }
    uint32_t size() const
    {
        return m_setTimeQueue.size();
    }

    bool peek(T &key, TimeType &time)
    {
        if (m_setTimeQueue.empty()) {
            return false;
        }
        const TimeData &stTimeData = *m_setTimeQueue.begin();
        key = stTimeData.pairKeyTime.first;
        time = stTimeData.pairKeyTime.second;
        return true;
    }

    bool pop(T &key, TimeType &time)
    {
        bool ret = peek(key, time);
        if (ret) {
            del(key);
        }
        return ret;
    }

    bool peek_timeout(TimeType now, T &key, TimeType &time)
    {
        if (m_setTimeQueue.empty()) {
            return false;
        }

        const TimeData &stTimeData = *m_setTimeQueue.begin();
        if (TimeCompare()(now, stTimeData.pairKeyTime.second)) {
            return false;
        }
        key = stTimeData.pairKeyTime.first;
        time = stTimeData.pairKeyTime.second;
        return true;
    }

    bool pop_timeout(TimeType now, T &key, TimeType &time)
    {
        bool ret = peek_timeout(now, key, time);
        if (ret) {
            del(key);
        }
        return ret;
    }

private:
    uint32_t getNewSalt()
    {
        ++m_iLastSalt;
        if (m_iLastSalt == 0) {
            ++m_iLastSalt;
        }
        return m_iLastSalt;
    }

    void removeTimeQueue(const TimeData &data)
    {
        for (typename TimeQueueType::iterator first = m_setTimeQueue.lower_bound(data), last = m_setTimeQueue.end(); first != last; ++first) {
            if (*first == data) {
                m_setTimeQueue.erase(first);
                break;
            }
        }
    }
    void addTimeQueue(const TimeData &data)
    {
        m_setTimeQueue.insert(data);
    }

private:
    uint32_t		m_iLastSalt;
    TimeDataType	m_mTimeData;
    TimeQueueType	m_setTimeQueue;
};

}

#endif
