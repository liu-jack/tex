#include "util_random.h"
#include "util_singleton.h"
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

namespace mfw
{

static pthread_once_t g_is_randomfd_inited = PTHREAD_ONCE_INIT;
static int g_random_fd = -1;
static void initRandomFd()
{
	g_random_fd = open("/dev/urandom", O_RDONLY);
}

class RandomGenerator
{
public:
	RandomGenerator()
	{
		update();
	}

	uint32_t gen()
	{
		if (m_iPos >= sizeof(m_aRands) / sizeof(m_aRands[0]))
		{
			update();
		}
		return m_aRands[m_iPos++];
	}

private:
	void update()
	{
		if (g_random_fd < 0)
		{
			pthread_once(&g_is_randomfd_inited, initRandomFd);
		}

		m_iPos = 0;
		read(g_random_fd, (void *)m_aRands, sizeof(m_aRands));
	}

private:
	uint32_t	m_iPos;
	uint32_t	m_aRands[10240];
};

uint32_t UtilRandom::random32()
{
	return ThreadSingleton<RandomGenerator>::get()->gen();
}

uint32_t UtilRandom::random32(uint32_t min, uint32_t max)
{
	if (min == max)
	{
		return min;
	}

	if (min > max)
	{
		std::swap(min, max);
	}

	return random32() % (max - min + 1) + min;
}

}
