#ifndef _MFW_UTIL_MEMKV_
#define _MFW_UTIL_MEMKV_

#include <stdint.h>
#include <string>
#include <list>
#include "util_queue.h"
using namespace std;

namespace mfw
{

enum CMemKVStatus {
    CMemKVStatus_OK,
    CMemKVStatus_Key_Missing,
    CMemKVStatus_Deleted,
    CMemKVStatus_No_Space,
    CMemKVStatus_Ver_Mismatch,
    CMemKVStatus_Invalid_Operation,
    CMemKVStatus_Internal_Error,
};

class CMemKV
{
public:
    CMemKV() : m_pMemBase(NULL), m_iMemSize(0), m_iChunkSize(0), m_iChunkNum(0), m_iLastUniqueVersion(0) {}

    bool init(void *pMemBase, uint64_t iMemSize, uint32_t iChunkSize, bool bCreate);
    bool create(void *pMemBase, uint64_t iMemSize, uint32_t iChunkSize);
    bool attach(void *pMemBase, uint64_t iMemSize);
    bool has(const string &sKey);
    CMemKVStatus get(uint32_t iNow, const string &sKey, uint32_t &iVersion, string &sValue);
    CMemKVStatus setInitVersion(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue);
    CMemKVStatus set(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue, bool bDirty);
    CMemKVStatus del(uint32_t iNow, const string &sKey, uint32_t iVersion, bool bDirty);
    CMemKVStatus erase(const string &sKey, uint32_t iVersion, bool bCheckDeleteFlag);
    uint32_t size();

    bool getSyncTask(uint32_t iSyncTime, string &sKey, uint64_t &iUniqueVersion);
    void clearSyncTask(uint32_t iNow, const string &sKey, uint64_t iUniqueVersion);
    void delaySyncTask(const string &sKey, uint32_t iSyncTime);

    struct StatInfo {
        uint64_t    iMemSize;
        uint32_t    iChunkSize;
        uint32_t    iChunkNum;
        uint32_t    iFreeChunkNum;
        uint32_t    iUsedChunkNum;
        uint64_t    iFreeSize;
        uint64_t    iUsedSize;
        uint32_t    iDataNum;
        uint32_t    iDirtyNum;
        uint32_t    iAccessLRUNum;
    };
    void getStatInfo(StatInfo &stStatInfo);

private:
    struct NodeInfo {
        uint32_t	iChunk;
        bool		bIsDirty;
        uint32_t	iSyncTime;
        uint32_t	iAccessTime;
        uint64_t	iUniqueVersion;

        bool hasSyncTask() const
        {
            return iSyncTime != 0;
        }
        bool hasAccessLRU() const
        {
            return !bIsDirty && iAccessTime != 0;
        }
    };

    struct Iterator {
        typedef ptrdiff_t                           difference_type;
        typedef std::forward_iterator_tag           iterator_category;
        typedef const string                        value_type;
        typedef value_type&                         reference;
        typedef value_type*                         pointer;

        unordered_map<string, NodeInfo>::iterator   iter;

        explicit Iterator(unordered_map<string, NodeInfo>::iterator it) : iter(it) {}
        Iterator(const Iterator &it) : iter(it.iter) {}
        Iterator() {}

        reference operator*() const
        {
            return iter->first;
        }

        pointer operator->() const
        {
            return &iter->first;
        }

        Iterator &operator++()
        {
            ++iter;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator== (const Iterator &x) const
        {
            return iter == x.iter;
        }
        bool operator!= (const Iterator &x) const
        {
            return iter != x.iter;
        }
    };

public:
    typedef Iterator iterator;
    iterator begin()
    {
        return Iterator(m_mKeyNode.begin());
    }
    iterator end()
    {
        return Iterator(m_mKeyNode.end());
    }

private:
    bool load();
    uint64_t getNewUniqueVersion();
    bool updateKeyNode(const string &sKey, uint32_t iChunk);
    char *getChunkAddr(uint32_t iChunk)
    {
        return m_pMemBase + iChunk * m_iChunkSize;
    }
    uint32_t getChunkByKey(const string &sKey);
    CMemKVStatus setInternal(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue, bool bDirty, bool bDeleted, bool bIsInit = false);

    uint32_t caclChunkNum(const string &sKey, const string &sValue);
    bool makeSpace(uint32_t iChunkNum);
    void recycleChunk(uint32_t iChunk);
    uint32_t allocateChunk(uint32_t iChunkNum);

private:
    friend class CDataNode;

    char *m_pMemBase;
    uint64_t m_iMemSize;
    uint32_t m_iChunkSize;
    uint32_t m_iChunkNum;
    uint64_t m_iLastUniqueVersion;
    deque<uint32_t> m_listFreeChunk;
    unordered_map<string, NodeInfo> m_mKeyNode;
    CTimeQueue<string, uint32_t> m_queueSyncTask;
    CTimeQueue<string, uint32_t> m_queueAccessLRU; // non-dirty data only
};

}

#endif
