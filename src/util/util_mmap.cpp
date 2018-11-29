#include "util_mmap.h"
#include "util_file.h"
#include <fcntl.h>
#include <unistd.h>

namespace mfw
{

bool CMmap::mmap(void *addr, uint64_t length, int prot, int flags, int fd, uint64_t offset, bool bCreate)
{
	void *ptr = ::mmap64(addr, length, prot, flags, fd, offset);
	if (ptr == MAP_FAILED)
	{
		return false;
	}
	m_pAddr = ptr;
	m_iSize = length;
	m_bCreate = bCreate;
	return true;
}

bool CMmap::mmap(const string &sFile, uint64_t iSize)
{
	bool bCreate = UtilFile::isFileExists(sFile) ? false : true;
	if (!UtilFile::makeDirectoryRecursive(UtilFile::getFileDirname(sFile)))
	{
		return false;
	}

	int fd = open(sFile.c_str(), O_CREAT | O_RDWR, 0666);
	if (fd < 0)
	{
		return false;
	}

	char c = 0;
	if (lseek64(fd, iSize - 1, SEEK_SET) < 0
			|| read(fd, &c, 1) < 0
			|| lseek64(fd, iSize - 1, SEEK_SET) < 0
			|| write(fd, &c, 1) != 1)
	{
		close(fd);
		return false;
	}

	if (!mmap(NULL, iSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, bCreate))
	{
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

bool CMmap::munmap()
{
	bool ret = true;
	if (isValid())
	{
		if (::munmap(m_pAddr, m_iSize) != 0)
		{
			ret = false;
		}
		reset();
	}
	return ret;
}

void CMmap::msync(bool bSync)
{
	if (isValid())
	{
		if (bSync)
		{
			::msync(m_pAddr, m_iSize, MS_SYNC | MS_INVALIDATE);
		}
		else
		{
			::msync(m_pAddr, m_iSize, MS_ASYNC | MS_INVALIDATE);
		}
	}
}

}
