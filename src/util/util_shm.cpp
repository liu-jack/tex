#include "util_shm.h"

namespace mfw
{

bool CSharedMemory::init(key_t key, uint64_t iSize)
{
	if (!detach())
	{
		return false;
	}

	m_iShmId = shmget(key, iSize, IPC_CREAT | IPC_EXCL | 0666);
	if (m_iShmId < 0)
	{
		m_bCreate = false;
		m_iShmId = shmget(key, iSize, 0666);
		if (m_iShmId < 0)
		{
			return false;
		}
	}
	else
	{
		m_bCreate = true;
	}

	m_pAddr = shmat(m_iShmId, NULL, 0);
	if (m_pAddr == (void *)-1)
	{
		return false;
	}

	m_iSize = iSize;
	return true;
}

bool CSharedMemory::detach()
{
	bool ret = true;
	if (isValid())
	{
		if (shmdt(m_pAddr) != 0)
		{
			ret = false;
		}
		reset();
	}
	return ret;
}

bool CSharedMemory::del()
{
	bool ret = true;
	if (isValid())
	{
		if (shmctl(m_iShmId, IPC_RMID, 0) != 0)
		{
			ret = false;
		}
		reset();
	}
	return ret;
}

}
