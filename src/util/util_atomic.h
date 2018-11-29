#ifndef _MFW_UTIL_ATOMIC_
#define _MFW_UTIL_ATOMIC_

#include <stdint.h>

namespace mfw
{

template <typename T>
class CAtomic
{
public:
	CAtomic() : m_val() {}
	explicit CAtomic(T v) : m_val(v) {}

	T get() const { return m_val; }
	void set(T v) { m_val = v; }
	T inc(T v = 1) { return add_and_fetch(v); }
	T dec(T v = 1) { return add_and_fetch(v); }
	// operator T() const { return m_val; }

	T fetch_and_add(T v) { return __sync_fetch_and_add(&m_val, v); }
	T fetch_and_sub(T v) { return __sync_fetch_and_sub(&m_val, v); }
	T fetch_and_or(T v) { return __sync_fetch_and_or(&m_val, v); }
	T fetch_and_and(T v) { return __sync_fetch_and_and(&m_val, v); }
	T fetch_and_xor(T v) { return __sync_fetch_and_xor(&m_val, v); }
	T fetch_and_nand(T v) { return __sync_fetch_and_nand(&m_val, v); }

	T add_and_fetch(T v) { return __sync_add_and_fetch(&m_val, v); }
	T sub_and_fetch(T v) { return __sync_sub_and_fetch(&m_val, v); }
	T or_and_fetch(T v) { return __sync_or_and_fetch(&m_val, v); }
	T and_and_fetch(T v) { return __sync_and_and_fetch(&m_val, v); }
	T xor_and_fetch(T v) { return __sync_xor_and_fetch(&m_val, v); }
	T nand_and_fetch(T v) { return __sync_nand_and_fetch(&m_val, v); }

	bool bool_compare_and_swap(T oldv, T newv) { return __sync_bool_compare_and_swap(&m_val, oldv, newv); }
	T val_compare_and_swap(T oldv, T newv) { return __sync_val_compare_and_swap(&m_val, oldv, newv); }

private:
	T m_val;
};

}

#endif
