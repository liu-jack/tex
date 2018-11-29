#include "util_crc32.h"
#include "util_encode.h"
#include "zlib/zlib.h"
#include <stdio.h>
#include <netinet/in.h>

namespace mfw
{

uint32_t UtilCRC32::crc32(const string &s)
{
	uint32_t crc = ::crc32(0, Z_NULL, 0);
	crc = ::crc32(crc, (const Bytef *)s.c_str(), s.size());
	return crc;
}

uint32_t UtilCRC32::crc32file(const string &sFile)
{
	FILE *fp = fopen(sFile.c_str(), "rb");
	if (fp == NULL)
	{
		return 0;
	}

	uint32_t crc = ::crc32(0, Z_NULL, 0);
	char buf[4096];
	int n = 0;
	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
	{
		crc = ::crc32(crc, (const Bytef *)buf, n);
	}
	fclose(fp);
	return crc;
}

string UtilCRC32::crc32bin(const string &s)
{
	uint32_t crc = htonl(crc32(s));
	return string((char *)&crc, sizeof(crc));
}

string UtilCRC32::crc32filebin(const string &sFile)
{
	uint32_t crc = htonl(crc32file(sFile));
	return string((char *)&crc, sizeof(crc));
}

string UtilCRC32::crc32hex(const string &s)
{
	string sCRC32Bin = crc32bin(s);
	return UtilEncode::toHex(sCRC32Bin);
}

string UtilCRC32::crc32filehex(const string &sFile)
{
	string sCRC32Bin = crc32filebin(sFile);
	return UtilEncode::toHex(sCRC32Bin);
}

}
