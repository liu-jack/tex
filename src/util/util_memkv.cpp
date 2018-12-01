#include "util_memkv.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <map>
#include <iostream>
using namespace std;

#define CHUNK_MIN_SIZE	64
#define CHUNK_MAX_SIZE	(64 * 1024)
#define CMEMKVDBG(x) do { cout << __FILE__ << ":" << __LINE__ << "|" << x << endl; } while (0)

namespace mfw
{

enum CMemKVDataFlag {
    CMemKVDataFlag_Dirty = 1,
    CMemKVDataFlag_Deleted = 2,
};

struct __attribute__ ((packed)) CMemKVGlobal {
    uint64_t	iMemSize;
    uint32_t	iChunkSize;
};

struct __attribute__ ((packed)) CMemKVChunkHead {
    uint32_t	iPrevChunk;
    uint32_t	iNextChunk;
    uint16_t	iUsedSize;
};

struct __attribute__ ((packed)) CMemKVDataHead {
    uint32_t		iVersion;
    uint16_t		iKeySize;
    uint32_t		iValueSize;
    uint32_t		iSyncTime;
    uint32_t		iAccessTime;
    uint8_t			iDataFlags;
};

static CMemKVChunkHead *getChunkHead(char *pChunk)
{
    return reinterpret_cast<CMemKVChunkHead *>(pChunk);
}

static CMemKVDataHead *getDataHead(char *pChunk)
{
    return reinterpret_cast<CMemKVDataHead *>(pChunk + sizeof(CMemKVChunkHead));
}

static uint32_t getFirstDataOffset()
{
    return sizeof(CMemKVChunkHead) + sizeof(CMemKVDataHead);
}

static uint32_t getFollowDataOffset()
{
    return sizeof(CMemKVChunkHead);
}

class CDataNode
{
public:
    CDataNode() : m_pMemKV(NULL), m_iChunk(0), m_pChunk(NULL), m_pDataHead(NULL) {}
    explicit CDataNode(CMemKV *pMemKV, uint32_t iChunk) : m_pMemKV(pMemKV), m_iChunk(iChunk), m_pChunk(NULL), m_pDataHead(NULL)
    {
        m_pChunk = getChunkAddr(m_iChunk);
        m_pDataHead = getDataHead(m_pChunk);
    }
    void setNode(CMemKV *pMemKV, uint32_t iChunk)
    {
        m_pMemKV = pMemKV;
        m_iChunk = iChunk;
        m_pChunk = getChunkAddr(m_iChunk);
        m_pDataHead = getDataHead(m_pChunk);
    }

    char *getChunkAddr(uint32_t iChunk);
    bool getKey(string &sKey);
    bool getValue(string &sValue);

    bool setKeyValue(const string &sKey, const string &sValue);
    bool setUsedSize(const string &sKey, const string &sValue);
    void clearUsedSize();

    bool checkVersion(uint32_t iVersion)
    {
        return iVersion == 0 || iVersion == getVersion();
    }
    uint32_t getVersion()
    {
        return m_pDataHead->iVersion;
    }
    void setVersion(uint32_t iVersion)
    {
        m_pDataHead->iVersion = iVersion;
    }
    uint32_t getSyncTime()
    {
        return m_pDataHead->iSyncTime;
    }
    void setSyncTime(uint32_t iSyncTime)
    {
        m_pDataHead->iSyncTime = iSyncTime;
    }
    uint32_t getAccessTime()
    {
        return m_pDataHead->iAccessTime;
    }
    void setAccessTime(uint32_t iAccessTime)
    {
        m_pDataHead->iAccessTime = iAccessTime;
    }

    bool isDirty() const
    {
        return (m_pDataHead->iDataFlags & CMemKVDataFlag_Dirty) != 0;
    }
    void setDirty()
    {
        m_pDataHead->iDataFlags |= CMemKVDataFlag_Dirty;
    }
    void clearDirty()
    {
        m_pDataHead->iDataFlags &= ~CMemKVDataFlag_Dirty;
    }

    bool isDeleted() const
    {
        return (m_pDataHead->iDataFlags & CMemKVDataFlag_Deleted) != 0;
    }
    void setDeleted()
    {
        m_pDataHead->iDataFlags |= CMemKVDataFlag_Deleted;
    }

private:
    bool prepareOffset(char *&pChunk, uint32_t &iOffset);
    bool moveToNextChunk(char *&pChunk, uint32_t &iOffset);
    bool copyDataOut(char *&pChunk, uint32_t &iOffset, uint32_t iSize, string *psData);
    bool copyDataIn(const string &sData, char *&pChunk, uint32_t &iOffset);

private:
    CMemKV *m_pMemKV;
    uint32_t m_iChunk;
    char *m_pChunk;
    CMemKVDataHead *m_pDataHead;
};

char *CDataNode::getChunkAddr(uint32_t iChunk)
{
    return m_pMemKV->getChunkAddr(iChunk);
}

bool CDataNode::getKey(string &sKey)
{
    char *pReadingChunk = m_pChunk;
    uint32_t iReadingOffset = getFirstDataOffset();
    sKey.reserve(m_pDataHead->iKeySize);
    if (!copyDataOut(pReadingChunk, iReadingOffset, m_pDataHead->iKeySize, &sKey)) {
        return false;
    }
    return true;
}

bool CDataNode::getValue(string &sValue)
{
    char *pReadingChunk = m_pChunk;
    uint32_t iReadingOffset = getFirstDataOffset();
    if (!copyDataOut(pReadingChunk, iReadingOffset, m_pDataHead->iKeySize, NULL)) {
        return false;
    }

    sValue.reserve(m_pDataHead->iValueSize);
    if (!copyDataOut(pReadingChunk, iReadingOffset, m_pDataHead->iValueSize, &sValue)) {
        return false;
    }
    return true;
}

bool CDataNode::setKeyValue(const string &sKey, const string &sValue)
{
    char *pWritingChunk = m_pChunk;
    uint32_t iWritingOffset = getFirstDataOffset();
    if (!copyDataIn(sKey, pWritingChunk, iWritingOffset)) {
        return false;
    }
    if (!copyDataIn(sValue, pWritingChunk, iWritingOffset)) {
        return false;
    }
    m_pDataHead->iKeySize = sKey.size();
    m_pDataHead->iValueSize = sValue.size();
    return true;
}

bool CDataNode::setUsedSize(const string &sKey, const string &sValue)
{
    CMemKVChunkHead *pHead = getChunkHead(m_pChunk);
    uint32_t iSize = sKey.size() + sValue.size();
    uint32_t iAvailSize = m_pMemKV->m_iChunkSize - getFirstDataOffset();
    if (iSize <= iAvailSize) {
        pHead->iUsedSize = sizeof(CMemKVDataHead) + iSize;
        return true;
    }

    pHead->iUsedSize = sizeof(CMemKVDataHead) + iAvailSize;
    iSize -= iAvailSize;
    while (iSize != 0) {
        if (pHead->iNextChunk == 0) {
            assert(false);
            return false;
        }
        char *pChunk = getChunkAddr(pHead->iNextChunk);
        pHead = getChunkHead(pChunk);
        iAvailSize = m_pMemKV->m_iChunkSize - getFollowDataOffset();
        if (iSize <= iAvailSize) {
            pHead->iUsedSize = iSize;
            return true;
        } else {
            pHead->iUsedSize = iAvailSize;
            iSize -= iAvailSize;
        }
    }
    return false;
}

void CDataNode::clearUsedSize()
{
    char *pChunk = m_pChunk;
    CMemKVChunkHead *pHead = getChunkHead(pChunk);
    pHead->iUsedSize = 0;
    while (pHead->iNextChunk) {
        pChunk = getChunkAddr(pHead->iNextChunk);
        pHead = getChunkHead(pChunk);
        pHead->iUsedSize = 0;
    }
}

bool CDataNode::prepareOffset(char *&pChunk, uint32_t &iOffset)
{
    if (iOffset > m_pMemKV->m_iChunkSize) {
        assert(false);
        return false;
    }
    if (iOffset == m_pMemKV->m_iChunkSize) {
        if (!moveToNextChunk(pChunk, iOffset)) {
            return false;
        }
    }
    return true;
}

bool CDataNode::moveToNextChunk(char *&pChunk, uint32_t &iOffset)
{
    CMemKVChunkHead *pHead = getChunkHead(pChunk);
    if (pHead->iNextChunk == 0) {
        assert(false);
        return false;
    }
    pChunk = getChunkAddr(pHead->iNextChunk);
    iOffset = getFollowDataOffset();
    return true;
}

bool CDataNode::copyDataOut(char *&pChunk, uint32_t &iOffset, uint32_t iSize, string *psData)
{
    if (iSize == 0) {
        return true;
    }
    if (!prepareOffset(pChunk, iOffset)) {
        return false;
    }

    while (iSize != 0) {
        CMemKVChunkHead *pHead = getChunkHead(pChunk);
        if (pHead->iUsedSize == 0) {
            assert(false);
            return false;
        }

        uint32_t iOffsetEnd = sizeof(CMemKVChunkHead) + pHead->iUsedSize;
        if (iOffset >= iOffsetEnd) {
            assert(false);
            return false;
        }

        uint32_t iAvailSize = iOffsetEnd - iOffset;
        if (iSize <= iAvailSize) {
            if (psData) {
                psData->append(pChunk + iOffset, pChunk + iOffset + iSize);
            }
            iOffset += iSize;
            return true;
        } else {
            if (psData) {
                psData->append(pChunk + iOffset, pChunk + iOffsetEnd);
            }
            if (!moveToNextChunk(pChunk, iOffset)) {
                return false;
            }
            iSize -= iAvailSize;
        }
    }
    return false;
}

bool CDataNode::copyDataIn(const string &sData, char *&pChunk, uint32_t &iOffset)
{
    if (sData.empty()) {
        return true;
    }
    if (!prepareOffset(pChunk, iOffset)) {
        return false;
    }

    const char *pData = sData.c_str();
    uint32_t iLeftSize = sData.size();
    while (iLeftSize != 0) {
        uint32_t iAvailSize = m_pMemKV->m_iChunkSize - iOffset;
        if (iLeftSize <= iAvailSize) {
            memcpy(pChunk + iOffset, pData, iLeftSize);
            iOffset += iLeftSize;
            return true;
        } else {
            memcpy(pChunk + iOffset, pData, iAvailSize);
            pData += iAvailSize;
            iLeftSize -= iAvailSize;
            if (!moveToNextChunk(pChunk, iOffset)) {
                return false;
            }
        }
    }
    return false;
}

bool CMemKV::init(void *pMemBase, uint64_t iMemSize, uint32_t iChunkSize, bool bCreate)
{
    if (bCreate) {
        return create(pMemBase, iMemSize, iChunkSize);
    }
    return attach(pMemBase, iMemSize);
}

bool CMemKV::create(void *pMemBase, uint64_t iMemSize, uint32_t iChunkSize)
{
    if (pMemBase == NULL || reinterpret_cast<uint64_t>(pMemBase) % 8 != 0) {
        return false;
    }
    if (iChunkSize < CHUNK_MIN_SIZE || iChunkSize > CHUNK_MAX_SIZE || iChunkSize % 8 != 0 || iMemSize / iChunkSize < 2 || iMemSize % iChunkSize != 0) {
        return false;
    }

    memset(pMemBase, 0, iMemSize);
    CMemKVGlobal *pGlobal = reinterpret_cast<CMemKVGlobal *>(pMemBase);
    pGlobal->iMemSize = iMemSize;
    pGlobal->iChunkSize = iChunkSize;

    m_pMemBase = reinterpret_cast<char *>(pMemBase);
    m_iMemSize = iMemSize;
    m_iChunkSize = iChunkSize;
    m_iChunkNum = m_iMemSize / m_iChunkSize;

    for (uint32_t iChunk = 1; iChunk < m_iChunkNum; ++iChunk) {
        m_listFreeChunk.push_back(iChunk);
    }
    return true;
}

bool CMemKV::attach(void *pMemBase, uint64_t iMemSize)
{
    if (pMemBase == NULL || reinterpret_cast<uint64_t>(pMemBase) % 8 != 0) {
        return false;
    }
    if (iMemSize < sizeof(CMemKVGlobal)) {
        return false;
    }

    CMemKVGlobal *pGlobal = reinterpret_cast<CMemKVGlobal *>(pMemBase);
    if (pGlobal->iMemSize != iMemSize) {
        return false;
    }
    uint32_t iChunkSize = pGlobal->iChunkSize;
    if (iChunkSize < CHUNK_MIN_SIZE || iChunkSize > CHUNK_MAX_SIZE || iChunkSize % 8 != 0 || iMemSize / iChunkSize < 2 || iMemSize % iChunkSize != 0) {
        return false;
    }

    m_pMemBase = reinterpret_cast<char *>(pMemBase);
    m_iMemSize = pGlobal->iMemSize;
    m_iChunkSize = pGlobal->iChunkSize;
    m_iChunkNum = m_iMemSize / m_iChunkSize;
    return load();
}

bool CMemKV::has(const string &sKey)
{
    uint32_t iChunk = getChunkByKey(sKey);
    if (iChunk == 0) {
        return false;
    }
    return true;
}

CMemKVStatus CMemKV::get(uint32_t iNow, const string &sKey, uint32_t &iVersion, string &sValue)
{
    uint32_t iChunk = getChunkByKey(sKey);
    if (iChunk == 0) {
        return CMemKVStatus_Key_Missing;
    }
    CDataNode node(this, iChunk);
    if (node.isDeleted()) {
        iVersion = node.getVersion();
        return CMemKVStatus_Deleted;
    }
    if (!node.getValue(sValue)) {
        return CMemKVStatus_Internal_Error;
    }
    iVersion = node.getVersion();
    node.setAccessTime(iNow);
    updateKeyNode(sKey, iChunk);
    return CMemKVStatus_OK;
}

CMemKVStatus CMemKV::setInternal(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue, bool bDirty, bool bDeleted, bool bIsInit)
{
    CDataNode oldnode;
    uint32_t iOldChunk = getChunkByKey(sKey);
    if (iOldChunk != 0) {
        oldnode.setNode(this, iOldChunk);
        if (oldnode.isDirty()) {
            bDirty = true;
        }
        if (!bDirty && bDeleted) {
            return erase(sKey, iVersion, false);
        }
    }

    if (bDeleted && (iOldChunk == 0 || oldnode.isDeleted())) {
        return CMemKVStatus_OK;
    }
    if (bIsInit && iOldChunk != 0) {
        return CMemKVStatus_Invalid_Operation;
    }

    // check version
    uint32_t iRequiredVersion = 0;
    uint32_t iNewVersion = 0;
    if (bDeleted) {
        iRequiredVersion = oldnode.getVersion();
    } else if (iOldChunk == 0 || oldnode.isDeleted()) {
        iRequiredVersion = 1;
    } else {
        iRequiredVersion = oldnode.getVersion();
    }
    if (!bIsInit && iVersion != 0 && iVersion != iRequiredVersion) {
        return CMemKVStatus_Ver_Mismatch;
    }
    if (bIsInit) {
        iNewVersion = iVersion;
    } else {
        iNewVersion = iRequiredVersion + 1;
    }
    if (iNewVersion < 2) {
        iNewVersion = 2;
    }

    uint32_t iNewSyncTime = 0;
    if (bDirty) {
        if (iOldChunk != 0) {
            iNewSyncTime = oldnode.getSyncTime();
            if (iNewSyncTime == 0) {
                iNewSyncTime = iNow;
            }
        } else {
            iNewSyncTime = iNow;
        }
    }

    uint32_t iChunkNum = caclChunkNum(sKey, sValue);
    if (!makeSpace(iChunkNum)) {
        return CMemKVStatus_No_Space;
    }

    uint32_t iNewChunk = allocateChunk(iChunkNum);
    if (iNewChunk == 0) {
        return CMemKVStatus_Internal_Error;
    }

    CDataNode newnode(this, iNewChunk);
    newnode.setVersion(iNewVersion);
    newnode.setSyncTime(iNewSyncTime);
    newnode.setAccessTime(iNow);
    if (bDirty) {
        newnode.setDirty();
    }
    if (bDeleted) {
        newnode.setDeleted();
    }
    if (!newnode.setKeyValue(sKey, sValue)) {
        return CMemKVStatus_Internal_Error;
    }
    if (!newnode.setUsedSize(sKey, sValue)) {
        return CMemKVStatus_Internal_Error;
    }
    for (uint32_t i = 0; i < iChunkNum; ++i) {
        m_listFreeChunk.pop_front();
    }

    if (iOldChunk != 0) {
        recycleChunk(iOldChunk);
    }

    updateKeyNode(sKey, iNewChunk);
    return CMemKVStatus_OK;
}

CMemKVStatus CMemKV::setInitVersion(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue)
{
    return setInternal(iNow, sKey, iVersion, sValue, false, false, true);
}

CMemKVStatus CMemKV::set(uint32_t iNow, const string &sKey, uint32_t iVersion, const string &sValue, bool bDirty)
{
    return setInternal(iNow, sKey, iVersion, sValue, bDirty, false);
}

static string g_sDummyEmptyString;
CMemKVStatus CMemKV::del(uint32_t iNow, const string &sKey, uint32_t iVersion, bool bDirty)
{
    return setInternal(iNow, sKey, iVersion, g_sDummyEmptyString, bDirty, true);
}

CMemKVStatus CMemKV::erase(const string &sKey, uint32_t iVersion, bool bCheckDeleteFlag)
{
    uint32_t iChunk = getChunkByKey(sKey);
    if (iChunk == 0) {
        return CMemKVStatus_OK;
    }
    CDataNode node(this, iChunk);
    if (!node.checkVersion(iVersion)) {
        return CMemKVStatus_Ver_Mismatch;
    }
    if (bCheckDeleteFlag && !node.isDeleted()) {
        return CMemKVStatus_Invalid_Operation;
    }
    recycleChunk(iChunk);
    updateKeyNode(sKey, 0);
    return CMemKVStatus_OK;
}

uint32_t CMemKV::size()
{
    return m_mKeyNode.size();
}

bool CMemKV::getSyncTask(uint32_t iSyncTime, string &sKey, uint64_t &iUniqueVersion)
{
    if (m_queueSyncTask.empty()) {
        return false;
    }
    if (iSyncTime < m_queueSyncTask.begin()->second) {
        return false;
    }
    sKey = m_queueSyncTask.begin()->first;
    unordered_map<string, NodeInfo>::const_iterator it = m_mKeyNode.find(sKey);
    assert(it != m_mKeyNode.end());
    const NodeInfo &stNodeInfo = it->second;
    iUniqueVersion = stNodeInfo.iUniqueVersion;
    return true;
}

void CMemKV::clearSyncTask(uint32_t iNow, const string &sKey, uint64_t iUniqueVersion)
{
    unordered_map<string, NodeInfo>::iterator it = m_mKeyNode.find(sKey);
    if (it == m_mKeyNode.end()) {
        return;
    }

    NodeInfo &stNodeInfo = it->second;
    CDataNode node(this, stNodeInfo.iChunk);
    if (iUniqueVersion != stNodeInfo.iUniqueVersion) {
        assert(node.isDirty());
        node.setSyncTime(iNow);
        stNodeInfo.iSyncTime = iNow;
        m_queueSyncTask.add(sKey, stNodeInfo.iSyncTime);
        return;
    }

    if (node.isDeleted()) {
        erase(sKey, 0, false);
        return;
    }

    node.clearDirty();
    node.setSyncTime(0);
    stNodeInfo.bIsDirty = false;
    stNodeInfo.iSyncTime = 0;
    m_queueSyncTask.del(sKey);
}

void CMemKV::delaySyncTask(const string &sKey, uint32_t iSyncTime)
{
    unordered_map<string, NodeInfo>::iterator it = m_mKeyNode.find(sKey);
    if (it == m_mKeyNode.end()) {
        return;
    }

    NodeInfo &stNodeInfo = it->second;
    CDataNode node(this, stNodeInfo.iChunk);
    assert(node.isDirty());

    node.setSyncTime(iSyncTime);
    stNodeInfo.iSyncTime = iSyncTime;
    m_queueSyncTask.add(sKey, stNodeInfo.iSyncTime);
}

bool CMemKV::load()
{
    map<uint32_t, pair<uint32_t, uint32_t> > mChunkPrevNext;
    for (uint32_t iChunk = 1; iChunk < m_iChunkNum; ++iChunk) {
        char *pChunk = getChunkAddr(iChunk);
        CMemKVChunkHead *pHead = getChunkHead(pChunk);
        if (pHead->iUsedSize == 0) {
            m_listFreeChunk.push_back(iChunk);
            continue;
        }
        if (pHead->iUsedSize > m_iChunkSize - sizeof(CMemKVChunkHead)) {
            assert(false);
            return false;
        }
        if (pHead->iPrevChunk >= m_iChunkNum || pHead->iNextChunk >= m_iChunkNum) {
            assert(false);
            return false;
        }

        if (pHead->iPrevChunk == 0) {
            // check data head
            if (pHead->iUsedSize < sizeof(CMemKVDataHead)) {
                assert(false);
                return false;
            }
        }
        if (pHead->iNextChunk != 0) {
            // this chunk contains data only
            if (pHead->iUsedSize != m_iChunkSize - sizeof(CMemKVChunkHead)) {
                assert(false);
                return false;
            }
        }
        mChunkPrevNext[iChunk] = make_pair(pHead->iPrevChunk, pHead->iNextChunk);
    }

    vector<uint32_t> vRecoverChunk;
    for (map<uint32_t, pair<uint32_t, uint32_t> >::const_iterator first = mChunkPrevNext.begin(), last = mChunkPrevNext.end(); first != last; ++first) {
        uint32_t iChunk = first->first;
        uint32_t iPrevChunk = first->second.first;
        uint32_t iNextChunk = first->second.second;

        if (iPrevChunk != 0) {
            map<uint32_t, pair<uint32_t, uint32_t> >::const_iterator it = mChunkPrevNext.find(iPrevChunk);
            if (it == mChunkPrevNext.end()) {
                vRecoverChunk.push_back(iChunk);
                continue;
            }
            if (it->second.second != iChunk) {
                assert(false);
                return false;
            }
        }
        if (iNextChunk != 0) {
            map<uint32_t, pair<uint32_t, uint32_t> >::const_iterator it = mChunkPrevNext.find(iNextChunk);
            if (it == mChunkPrevNext.end()) {
                vRecoverChunk.push_back(iChunk);
                continue;
            }
            if (it->second.first != iChunk) {
                assert(false);
                return false;
            }
        }
    }

    for (unsigned i = 0; i < vRecoverChunk.size(); ++i) {
        uint32_t iChunk = vRecoverChunk[i];
        char *pChunk = getChunkAddr(iChunk);
        CMemKVChunkHead *pHead = getChunkHead(pChunk);

        uint32_t iNextChunk = pHead->iNextChunk;
        while (iNextChunk != 0) {
            CMEMKVDBG("recovering chunk " << iNextChunk);
            char *pNextChunk = getChunkAddr(iNextChunk);
            CMemKVChunkHead *pNextHead = getChunkHead(pNextChunk);
            pNextHead->iUsedSize = 0;
            mChunkPrevNext.erase(iNextChunk);
            iNextChunk = pNextHead->iNextChunk;
        }

        uint32_t iPrevChunk = pHead->iPrevChunk;
        while (iPrevChunk != 0) {
            CMEMKVDBG("recovering chunk " << iPrevChunk);
            char *pPrevChunk = getChunkAddr(iPrevChunk);
            CMemKVChunkHead *pPrevHead = getChunkHead(pPrevChunk);
            pPrevHead->iUsedSize = 0;
            mChunkPrevNext.erase(iPrevChunk);
            iPrevChunk = pPrevHead->iPrevChunk;
        }

        CMEMKVDBG("recovering chunk " << iChunk);
        mChunkPrevNext.erase(iChunk);
    }

    map<string, pair<uint32_t, uint32_t> > mKey2ChunkId_Version;
    for (map<uint32_t, pair<uint32_t, uint32_t> >::const_iterator first = mChunkPrevNext.begin(), last = mChunkPrevNext.end(); first != last; ++first) {
        uint32_t iChunk = first->first;
        uint32_t iPrevChunk = first->second.first;
        if (iPrevChunk != 0) {
            continue;
        }

        CDataNode node(this, iChunk);
        string sKey;
        if (!node.getKey(sKey)) {
            assert(false);
            return false;
        }

        map<string, pair<uint32_t, uint32_t> >::iterator it = mKey2ChunkId_Version.find(sKey);
        if (it == mKey2ChunkId_Version.end()) {
            mKey2ChunkId_Version[sKey] = make_pair(iChunk, node.getVersion());
            continue;
        }
        if (it->second.second < node.getVersion()) {
            CMEMKVDBG("duplicate version " << sKey << ",old " << it->second.second << ",new " << node.getVersion() << ",chunk old " << it->second.first << ",new " << iChunk);
            recycleChunk(it->second.first);
            it->second = make_pair(iChunk, node.getVersion());
        }
    }

    for (map<string, pair<uint32_t, uint32_t> >::const_iterator first = mKey2ChunkId_Version.begin(), last = mKey2ChunkId_Version.end(); first != last; ++first) {
        const string &sKey = first->first;
        uint32_t iChunk = first->second.first;
        if (!updateKeyNode(sKey, iChunk)) {
            assert(false);
            return false;
        }
    }
    return true;
}

uint64_t CMemKV::getNewUniqueVersion()
{
    ++m_iLastUniqueVersion;
    if (m_iLastUniqueVersion == 0) {
        ++m_iLastUniqueVersion;
    }
    return m_iLastUniqueVersion;
}

bool CMemKV::updateKeyNode(const string &sKey, uint32_t iChunk)
{
    unordered_map<string, NodeInfo>::iterator it = m_mKeyNode.find(sKey);
    if (iChunk == 0) {
        // remove
        if (it != m_mKeyNode.end()) {
            NodeInfo &stNodeInfo = m_mKeyNode[sKey];
            if (stNodeInfo.hasSyncTask()) {
                m_queueSyncTask.del(sKey);
            }
            if (stNodeInfo.hasAccessLRU()) {
                m_queueAccessLRU.del(sKey);
            }
            m_mKeyNode.erase(it);
        }
        return true;
    }

    CDataNode node(this, iChunk);
    if (it == m_mKeyNode.end()) {
        // new
        NodeInfo &stNodeInfo = m_mKeyNode[sKey];
        stNodeInfo.iChunk = iChunk;
        stNodeInfo.bIsDirty = node.isDirty();
        stNodeInfo.iSyncTime = node.isDirty() ? node.getSyncTime() : 0;
        stNodeInfo.iAccessTime = node.getAccessTime();
        stNodeInfo.iUniqueVersion = getNewUniqueVersion();
        if (stNodeInfo.hasSyncTask()) {
            m_queueSyncTask.add(sKey, stNodeInfo.iSyncTime);
        }
        if (stNodeInfo.hasAccessLRU()) {
            m_queueAccessLRU.add(sKey, stNodeInfo.iAccessTime);
        }
        return true;
    }

    NodeInfo &stNodeInfo = it->second;
    if (stNodeInfo.iChunk == iChunk) {
        // GET operation, update access time only
        if (stNodeInfo.bIsDirty != node.isDirty()) {
            assert(false);
            return false;
        }
        if (stNodeInfo.hasAccessLRU() && stNodeInfo.iAccessTime != node.getAccessTime()) {
            stNodeInfo.iAccessTime = node.getAccessTime();
            m_queueAccessLRU.add(sKey, stNodeInfo.iAccessTime);
        }
        return true;
    }

    stNodeInfo.iChunk = iChunk;
    stNodeInfo.bIsDirty = node.isDirty();
    stNodeInfo.iSyncTime = node.isDirty() ? node.getSyncTime() : 0;
    stNodeInfo.iAccessTime = node.getAccessTime();
    stNodeInfo.iUniqueVersion = getNewUniqueVersion();
    if (stNodeInfo.hasSyncTask()) {
        m_queueSyncTask.add(sKey, stNodeInfo.iSyncTime);
    } else {
        m_queueSyncTask.del(sKey);
    }

    if (stNodeInfo.hasAccessLRU()) {
        m_queueAccessLRU.add(sKey, stNodeInfo.iAccessTime);
    } else {
        m_queueAccessLRU.del(sKey);
    }
    return true;
}

uint32_t CMemKV::getChunkByKey(const string &sKey)
{
    unordered_map<string, NodeInfo>::const_iterator it = m_mKeyNode.find(sKey);
    if (it == m_mKeyNode.end()) {
        return 0;
    }
    return it->second.iChunk;
}

uint32_t CMemKV::caclChunkNum(const string &sKey, const string &sValue)
{
    uint32_t iSize = sKey.size() + sValue.size();
    uint32_t iFirstDataSize = m_iChunkSize - getFirstDataOffset();
    if (iSize <= iFirstDataSize) {
        return 1;
    }

    uint32_t iFollowDataSize = m_iChunkSize - getFollowDataOffset();
    return 1 + (iSize - iFirstDataSize + iFollowDataSize - 1) / iFollowDataSize;
}

bool CMemKV::makeSpace(uint32_t iChunkNum)
{
    while (m_listFreeChunk.size() < iChunkNum && !m_queueAccessLRU.empty()) {
        const pair<string, uint32_t> &pairKeyAccessTime = *m_queueAccessLRU.begin();
        const string &sKey = pairKeyAccessTime.first;
        CMEMKVDBG("discards " << sKey);

        unordered_map<string, NodeInfo>::iterator it = m_mKeyNode.find(sKey);
        if (it == m_mKeyNode.end()) {
            assert(false);
            return false;
        }
        const NodeInfo &stNodeInfo = it->second;
        if (stNodeInfo.hasSyncTask()) {
            m_queueSyncTask.del(sKey);
        }
        if (stNodeInfo.hasAccessLRU()) {
            m_queueAccessLRU.del(sKey);
        }
        m_mKeyNode.erase(it);

        uint32_t iChunk = stNodeInfo.iChunk;
        assert(iChunk != 0);
        recycleChunk(iChunk);
    }
    return m_listFreeChunk.size() >= iChunkNum;
}

void CMemKV::recycleChunk(uint32_t iChunk)
{
    CDataNode node(this, iChunk);
    node.clearUsedSize();

    m_listFreeChunk.push_back(iChunk);
    char *pChunk = getChunkAddr(iChunk);
    CMemKVChunkHead *pHead = getChunkHead(pChunk);
    while (pHead->iNextChunk) {
        m_listFreeChunk.push_back(pHead->iNextChunk);
        pChunk = getChunkAddr(pHead->iNextChunk);
        pHead = getChunkHead(pChunk);
    }
}

uint32_t CMemKV::allocateChunk(uint32_t iChunkNum)
{
    if (iChunkNum > m_listFreeChunk.size()) {
        return 0;
    }

    deque<uint32_t>::iterator itFreeChunk = m_listFreeChunk.begin();
    for (uint32_t i = 0; i < iChunkNum; ++i, ++itFreeChunk) {
        uint32_t iChunk = *itFreeChunk;
        char *pChunk = getChunkAddr(iChunk);
        CMemKVChunkHead *pHead = getChunkHead(pChunk);
        memset(pHead, 0, sizeof(CMemKVChunkHead));
        if (i == 0) {
            CMemKVDataHead *pDataHead = getDataHead(pChunk);
            memset(pDataHead, 0, sizeof(CMemKVDataHead));
        } else {
            deque<uint32_t>::iterator itPrev = itFreeChunk;
            --itPrev;
            pHead->iPrevChunk = *itPrev;
        }
        if (i != iChunkNum - 1) {
            deque<uint32_t>::iterator itNext = itFreeChunk;
            ++itNext;
            pHead->iNextChunk = *itNext;
        }
    }
    return m_listFreeChunk.front();
}

void CMemKV::getStatInfo(StatInfo &stStatInfo)
{
    stStatInfo.iMemSize = m_iMemSize;
    stStatInfo.iChunkSize = m_iChunkSize;
    stStatInfo.iChunkNum = m_iChunkNum;
    stStatInfo.iFreeChunkNum = m_listFreeChunk.size();
    stStatInfo.iUsedChunkNum = m_iChunkNum - m_listFreeChunk.size();
    stStatInfo.iFreeSize = stStatInfo.iFreeChunkNum * stStatInfo.iChunkSize;
    stStatInfo.iUsedSize = stStatInfo.iUsedChunkNum * stStatInfo.iChunkSize;
    stStatInfo.iDataNum = m_mKeyNode.size();
    stStatInfo.iDirtyNum = m_queueSyncTask.size();
    stStatInfo.iAccessLRUNum = m_queueAccessLRU.size();
}

}

