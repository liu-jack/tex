#include "util_tea.h"
#include "util_crypto.h"
#include <netinet/in.h>
#include <stdexcept>

#define TEA_KEY_SIZE		16
#define TEA_BLOCK_SIZE		8
#define TEA_ROUNDS		32
#define TEA_DELTA		0x9e3779b9

#define XTEA_ROUNDS		TEA_ROUNDS
#define XTEA_DELTA		TEA_DELTA

typedef unsigned char u8;
typedef unsigned int u32;

#define u32_in(x) ntohl(*(const u32 *)(x))
#define u32_out(to, from) (*(u32 *)(to) = htonl(from))

struct tea_ctx {
	u32 KEY[4];
};

static int tea_setkey(tea_ctx *ctx, const u8 *in_key, unsigned int key_len)
{
	if (key_len != 16)
	{
		return -1;
	}

	ctx->KEY[0] = u32_in (in_key);
	ctx->KEY[1] = u32_in (in_key + 4);
	ctx->KEY[2] = u32_in (in_key + 8);
	ctx->KEY[3] = u32_in (in_key + 12);

	return 0;

}

static void tea_encrypt(tea_ctx *ctx, const u8 *src, u8 *dst)
{
	u32 y, z, n, sum = 0;
	u32 k0, k1, k2, k3;

	y = u32_in (src);
	z = u32_in (src + 4);

	k0 = ctx->KEY[0];
	k1 = ctx->KEY[1];
	k2 = ctx->KEY[2];
	k3 = ctx->KEY[3];

	n = TEA_ROUNDS;

	while (n-- > 0) {
		sum += TEA_DELTA;
		y += ((z << 4) + k0) ^ (z + sum) ^ ((z >> 5) + k1);
		z += ((y << 4) + k2) ^ (y + sum) ^ ((y >> 5) + k3);
	}

	u32_out (dst, y);
	u32_out (dst + 4, z);
}

static void tea_decrypt(tea_ctx *ctx, const u8 *src, u8 *dst)
{
	u32 y, z, n, sum;
	u32 k0, k1, k2, k3;

	y = u32_in (src);
	z = u32_in (src + 4);

	k0 = ctx->KEY[0];
	k1 = ctx->KEY[1];
	k2 = ctx->KEY[2];
	k3 = ctx->KEY[3];

	sum = TEA_DELTA << 5;

	n = TEA_ROUNDS;

	while (n-- > 0) {
		z -= ((y << 4) + k2) ^ (y + sum) ^ ((y >> 5) + k3);
		y -= ((z << 4) + k0) ^ (z + sum) ^ ((z >> 5) + k1);
		sum -= TEA_DELTA;
	}

	u32_out (dst, y);
	u32_out (dst + 4, z);
}

static void xtea_encrypt(tea_ctx *ctx, const u8 *src, u8 *dst)
{
	u32 y, z, sum = 0;
	u32 limit = XTEA_DELTA * XTEA_ROUNDS;

	y = u32_in (src);
	z = u32_in (src + 4);

	while (sum != limit) {
		y += (z << 4 ^ z >> 5) + (z ^ sum) + ctx->KEY[sum&3];
		sum += XTEA_DELTA;
		z += (y << 4 ^ y >> 5) + (y ^ sum) + ctx->KEY[sum>>11 &3];
	}

	u32_out (dst, y);
	u32_out (dst + 4, z);

}

static void xtea_decrypt(tea_ctx *ctx, const u8 *src, u8 *dst)
{
	u32 y, z, sum;

	y = u32_in (src);
	z = u32_in (src + 4);

	sum = XTEA_DELTA * XTEA_ROUNDS;

	while (sum) {
		z -= (y << 4 ^ y >> 5) + (y ^ sum) + ctx->KEY[sum>>11 & 3];
		sum -= XTEA_DELTA;
		y -= (z << 4 ^ z >> 5) + (z ^ sum) + ctx->KEY[sum & 3];
	}

	u32_out (dst, y);
	u32_out (dst + 4, z);
}

namespace mfw
{

string UtilTEA::tea_encrypt(const string &sData, const string &sKey)
{
	string sResult;
	if (!tea_encrypt(sData, sKey, sResult))
	{
		throw std::runtime_error("tea encrypt");
	}
	return sResult;
}

string UtilTEA::tea_decrypt(const string &sData, const string &sKey)
{
	string sResult;
	if (!tea_decrypt(sData, sKey, sResult))
	{
		throw std::runtime_error("tea decrypt");
	}
	return sResult;
}

bool UtilTEA::tea_encrypt(const string &sData, const string &sKey, string &sResult)
{
	tea_ctx ctx;
	if (tea_setkey(&ctx, (unsigned char *)sKey.c_str(), sKey.size()) != 0)
	{
		return false;
	}

	return UtilCrypto::cbc_encrypt(sData, (UtilCrypto::block_encrypt_func)::tea_encrypt, TEA_BLOCK_SIZE, &ctx, sResult);
}

bool UtilTEA::tea_decrypt(const string &sData, const string &sKey, string &sResult)
{
	tea_ctx ctx;
	if (tea_setkey(&ctx, (unsigned char *)sKey.c_str(), sKey.size()) != 0)
	{
		return false;
	}

	return UtilCrypto::cbc_decrypt(sData, (UtilCrypto::block_decrypt_func)::tea_decrypt, TEA_BLOCK_SIZE, &ctx, sResult);
}

string UtilTEA::xtea_encrypt(const string &sData, const string &sKey)
{
	string sResult;
	if (!xtea_encrypt(sData, sKey, sResult))
	{
		throw std::runtime_error("xtea encrypt");
	}
	return sResult;
}

string UtilTEA::xtea_decrypt(const string &sData, const string &sKey)
{
	string sResult;
	if (!xtea_decrypt(sData, sKey, sResult))
	{
		throw std::runtime_error("xtea decrypt");
	}
	return sResult;
}

bool UtilTEA::xtea_encrypt(const string &sData, const string &sKey, string &sResult)
{
	tea_ctx ctx;
	if (tea_setkey(&ctx, (unsigned char *)sKey.c_str(), sKey.size()) != 0)
	{
		return false;
	}

	return UtilCrypto::cbc_encrypt(sData, (UtilCrypto::block_encrypt_func)::xtea_encrypt, TEA_BLOCK_SIZE, &ctx, sResult);
}

bool UtilTEA::xtea_decrypt(const string &sData, const string &sKey, string &sResult)
{
	tea_ctx ctx;
	if (tea_setkey(&ctx, (unsigned char *)sKey.c_str(), sKey.size()) != 0)
	{
		return false;
	}

	return UtilCrypto::cbc_decrypt(sData, (UtilCrypto::block_decrypt_func)::xtea_decrypt, TEA_BLOCK_SIZE, &ctx, sResult);
}

}
