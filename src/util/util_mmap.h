#ifndef _MFW_UTIL_MMAP_
#define _MFW_UTIL_MMAP_

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <string>
using namespace std;

namespace mfw
{

class CMmap
{
public:
	CMmap() : m_pAddr((void *)-1), m_iSize(0), m_bCreate(false) {}

	bool mmap(void *addr, uint64_t length, int prot, int flags, int fd, uint64_t offset, bool bCreate);
	bool mmap(const string &sFile, uint64_t iSize);
	bool munmap();
	void msync(bool bSync);

	bool isValid() { return m_pAddr != NULL && m_pAddr != (void *)-1; }

	void *getMem() { return m_pAddr; }
	uint64_t getSize() { return m_iSize; }
	bool isCreate() { return m_bCreate; }

private:
	void reset() { m_pAddr = (void *)-1; m_iSize = 0; m_sFile.clear(); m_bCreate = false; }

private:
	void *m_pAddr;
	uint64_t m_iSize;
	string m_sFile;
	bool m_bCreate;
};

}

#endif
