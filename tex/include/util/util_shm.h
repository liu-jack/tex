#ifndef _MFW_UTIL_SHM_
#define _MFW_UTIL_SHM_

#include <stdint.h>
#include <stddef.h>
#include <sys/shm.h>

namespace mfw
{

class CSharedMemory
{
public:
	CSharedMemory() : m_pAddr((void *)-1), m_iSize(0), m_iShmId(-1), m_bCreate(false) {}

	bool init(key_t key, uint64_t iSize);
	bool detach();
	bool del();
	bool isValid() { return m_pAddr != NULL && m_pAddr != (void *)-1; }

	void *getMem() { return m_pAddr; }
	uint64_t getSize() { return m_iSize; }
	bool isCreate() { return m_bCreate; }

private:
	void reset() { m_pAddr = (void *)-1; m_iSize = 0; m_iShmId = -1; m_bCreate = false; }

private:
	void *m_pAddr;
	uint64_t m_iSize;
	int m_iShmId;
	bool m_bCreate;
};

}

#endif
