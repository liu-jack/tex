#ifndef _MFW_UTIL_STL_
#define _MFW_UTIL_STL_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <tr1/memory>
#include <tr1/functional>

using namespace std;

namespace mfw
{

namespace impl
{
template <typename T, int level>
struct extractMapValueType {
};

template <typename T>
struct extractMapValueType <T, 1> {
    typedef typename T::mapped_type type;
};

template <typename T>
struct extractMapValueType <const T, 1> {
    typedef const typename T::mapped_type type;
};

template <typename T>
struct extractMapValueType <T, 2> {
    typedef typename extractMapValueType <typename extractMapValueType<T, 1>::type, 1>::type type;
};

template <typename T>
struct extractMapValueType <T, 3> {
    typedef typename extractMapValueType <typename extractMapValueType<T, 1>::type, 2>::type type;
};

template <typename T>
struct extractMapValueType <T, 4> {
    typedef typename extractMapValueType <typename extractMapValueType<T, 1>::type, 3>::type type;
};

template <typename T>
struct extractMapValueType <T, 5> {
    typedef typename extractMapValueType <typename extractMapValueType<T, 1>::type, 4>::type type;
};

template <typename T>
struct selectMapIterator {
    typedef typename T::iterator type;
};

template <typename T>
struct selectMapIterator <const T> {
    typedef typename T::const_iterator type;
};
}

class UtilSTL
{
public:
    template <typename Map, typename FKey>
    static typename impl::extractMapValueType<Map, 1>::type *findMapPtr(Map &m, const FKey &key)
    {
        typename impl::selectMapIterator<Map>::type it = m.find(key);
        if (it == m.end()) {
            return NULL;
        }
        return &it->second;
    }

    template <typename Map, typename FKey, typename FKey2>
    static typename impl::extractMapValueType<Map, 2>::type *findMapPtr2(Map &m, const FKey &key, const FKey2 &key2)
    {
        typename impl::selectMapIterator<Map>::type it = m.find(key);
        if (it == m.end()) {
            return NULL;
        }
        return findMapPtr(it->second, key2);
    }

    template <typename Map, typename FKey, typename FKey2, typename FKey3>
    static typename impl::extractMapValueType<Map, 3>::type *findMapPtr3(Map &m, const FKey &key, const FKey2 &key2, const FKey3 &key3)
    {
        typename impl::selectMapIterator<Map>::type it = m.find(key);
        if (it == m.end()) {
            return NULL;
        }
        return findMapPtr2(it->second, key2, key3);
    }

    template <typename Map, typename FKey, typename FKey2, typename FKey3, typename FKey4>
    static typename impl::extractMapValueType<Map, 4>::type *findMapPtr4(Map &m, const FKey &key, const FKey2 &key2, const FKey3 &key3, const FKey4 &key4)
    {
        typename impl::selectMapIterator<Map>::type it = m.find(key);
        if (it == m.end()) {
            return NULL;
        }
        return findMapPtr3(it->second, key2, key3, key4);
    }

    template <typename Map, typename FKey, typename FKey2, typename FKey3, typename FKey4, typename FKey5>
    static typename impl::extractMapValueType<Map, 5>::type *findMapPtr5(Map &m, const FKey &key, const FKey2 &key2, const FKey3 &key3, const FKey4 &key4, const FKey5 &key5)
    {
        typename impl::selectMapIterator<Map>::type it = m.find(key);
        if (it == m.end()) {
            return NULL;
        }
        return findMapPtr4(it->second, key2, key3, key4, key5);
    }

    template <typename Tp, typename Alloc, typename Key>
    static Tp *findVectorPtr(vector<Tp, Alloc> &v, const Key &key)
    {
        typename vector<Tp, Alloc>::iterator it = find(v.begin(), v.end(), key);
        return it == v.end() ? NULL : &*it;
    }

    template <typename Tp, typename Alloc, typename Key>
    static const Tp* findVectorPtr(const vector<Tp, Alloc> &v, const Key &key)
    {
        typename vector<Tp, Alloc>::const_iterator it = find(v.begin(), v.end(), key);
        return it == v.end() ? NULL : &*it;
    }

    template <typename Tp, typename Alloc>
    static void sortUniq(vector<Tp, Alloc> &v)
    {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
    }

    template <typename Tp, typename Alloc, typename Key>
    static bool eraseVectorOne(vector<Tp, Alloc> &v, const Key &key)
    {
        typename vector<Tp, Alloc>::iterator it = find(v.begin(), v.end(), key);
        if (it != v.end()) {
            v.erase(it);
            return true;
        }
        return false;
    }

    template <typename Tp, typename Alloc, typename Key>
    static bool eraseVectorAll(vector<Tp, Alloc> &v, const Key &key)
    {
        bool ret = false;
        for (unsigned i = 0; i < v.size(); ) {
            if (v[i] == key) {
                v.erase(v.begin() + i);
                ret = true;
            } else {
                ++i;
            }
        }
        return ret;
    }

    template <typename Tp, typename Alloc>
    static bool eraseVectorByPtr(vector<Tp, Alloc> &v, const Tp *ptr)
    {
        if (ptr != NULL && !v.empty()) {
            typename vector<Tp, Alloc>::iterator it = v.begin() + (ptr - &*v.begin());
            v.erase(it);
            return true;
        }
        return false;
    }

    template <typename Tp, typename Alloc, typename Tp2, typename Alloc2>
    static void appendVector(vector<Tp, Alloc> &v, const vector<Tp2, Alloc2> &v2)
    {
        v.insert(v.end(), v2.begin(), v2.end());
    }

    template <typename T, typename RetType>
    static RetType funcOperatorNewCreator()
    {
        return RetType(new T());
    }

    template <typename T, typename RetType>
    static tr1::function<RetType ()> makeOperatorNewCreator()
    {
        return tr1::bind(&UtilSTL::funcOperatorNewCreator<T, RetType>);
    }
};

}

#endif
