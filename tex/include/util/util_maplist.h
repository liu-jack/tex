#ifndef _MFW_UTIL_MAP_LIST_H_
#define _MFW_UTIL_MAP_LIST_H_

#include <stdint.h>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <list>

namespace mfw
{
// 兼顾查找与遍历效率， 查找时使用unordered_map, 遍历使用list
template <typename KeyT, typename ValueT>
class MapListContainer
{
public:
    typedef std::pair<KeyT, ValueT> ListValueT;
    typedef std::list<ListValueT> ListType;
    typedef std::tr1::unordered_map<KeyT, typename ListType::iterator> MapType;

    typedef typename MapType::iterator MapIter;
    typedef typename ListType::iterator ListIter;

public:
    MapListContainer(uint32_t iPerLoopNum) :m_loopNum(iPerLoopNum)
    {
        m_loopCursor = m_list.end();
    }

    uint32_t size()
    {
        return m_list.size();
    }

    void addElement(const KeyT &key, const ValueT& val)
    {
        if (m_map.find(key) != m_map.end()) {
            return;
        }

        ListValueT value(key, val);
        m_list.push_back(value);
        m_map[key] = --m_list.end();
    }

    void delElement(const KeyT &key)
    {
        MapIter it = m_map.find(key);
        if (it == m_map.end()) {
            return;
        }

        // 删除时改变游标
        if (m_loopCursor == it->second) {
            m_loopCursor++;
        }

        m_list.erase(it->second);
        m_map.erase(it);
    }

    ValueT* getByKey(const KeyT &key)
    {
        MapIter it = m_map.find(key);
        if (it != m_map.end()) {
            return &(it->second->second);
        }

        return NULL;
    }

    // loop中不允许删除元素
    void loop(std::tr1::function<void(KeyT&, ValueT&)> func)
    {
        if (m_list.empty() || !func) {
            return;
        }

        uint32_t iNum = 0;
        ListIter startIt = m_loopCursor;
        if (m_loopCursor == m_list.end()) {
            m_loopCursor = m_list.begin();
        }
        do {
            func(m_loopCursor->first, m_loopCursor->second);
            iNum++;

            if (++m_loopCursor == m_list.end()) {
                if (startIt == m_list.end()) {
                    break;
                } else {
                    m_loopCursor = m_list.begin();
                }
            }
        } while(iNum < m_loopNum && m_loopCursor != startIt);
    }

    // loop中不允许删除元素
    void loopAll(std::tr1::function<void(KeyT&, ValueT&)> func)
    {
        if (m_list.empty() || !func) {
            return;
        }

        for (ListIter it = m_list.begin(); it != m_list.end(); ++it) {
            func(it->first, it->second);
        }
    }

private:
    MapType m_map;
    ListType m_list;

    ListIter m_loopCursor;
    uint32_t m_loopNum;
};

};
#endif
