#ifndef _MFW_BYTE_CONVERTER_H_
#define _MFW_BYTE_CONVERTER_H_

# if defined (BYTE_ORDER)
#   if (BYTE_ORDER == LITTLE_ENDIAN)
#     define MFW_LITTLE_ENDIAN 0x0123
#   elif (BYTE_ORDER == BIG_ENDIAN)
#     define MFW_BIG_ENDIAN 0x3210
#   else
#     error: unknown BYTE_ORDER!
#   endif /* BYTE_ORDER */
# elif defined (_BYTE_ORDER)
#   if (_BYTE_ORDER == _LITTLE_ENDIAN)
#     define MFW_LITTLE_ENDIAN 0x0123
#   elif (_BYTE_ORDER == _BIG_ENDIAN)
#     define MFW_BIG_ENDIAN 0x3210
#   else
#     error: unknown _BYTE_ORDER!
#   endif /* _BYTE_ORDER */
# elif defined (__BYTE_ORDER)
#   if (__BYTE_ORDER == __LITTLE_ENDIAN)
#     define MFW_LITTLE_ENDIAN 0x0123
#   elif (__BYTE_ORDER == __BIG_ENDIAN)
#     define MFW_BIG_ENDIAN 0x3210
#   else
#     error: unknown __BYTE_ORDER!
#   endif /* __BYTE_ORDER */
# else /* ! BYTE_ORDER && ! __BYTE_ORDER */
   // We weren't explicitly told, so we have to figure it out . . .
#   if defined (i386) || defined (__i386__) || defined (_M_IX86) || \
      defined (vax) || defined (__alpha) || defined (__LITTLE_ENDIAN__) ||\
      defined (ARM) || defined (_M_IA64) || \
      defined (_M_AMD64) || defined (__amd64)
     // We know these are little endian.
#     define MFW_LITTLE_ENDIAN 0x0123
#   else
    // Otherwise, we assume big endian.
#     define MFW_BIG_ENDIAN 0x3210
#   endif
# endif /* ! BYTE_ORDER && ! __BYTE_ORDER */

#include <algorithm>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <string.h>
#include <list>
#include <map>

using namespace std;

namespace mfw
{

template<size_t T>
inline void convert(char* val)
{
    std::swap(*val, *(val + T - 1));
    convert < T - 2 > (val + 1);
}

template<> inline void convert<0>(char*) {}

template<> inline void convert<1>(char*) {}

template<typename T>
inline void apply(T* val)
{
    convert<sizeof(T)>((char*)(val));
}

#if defined(MFW_BIG_ENDIAN)
template<typename T> inline void littleEndian(T& val) { mfw::apply<T>(&val); }
template<typename T> inline void bigEndian(T&) { }
#else
template<typename T> inline void littleEndian(T&) { }
template<typename T> inline void bigEndian(T& val) { mfw::apply<T>(&val); }
#endif

template<typename T> void littleEndian(T*);

template<typename T> void bigEndian(T*);

inline void littleEndian(unsigned char&) { }

inline void littleEndian(char&)  { }

inline void bigEndian(unsigned char&) { }

inline void bigEndian(char&) { }

class ByteBufferException
{
public:
    ByteBufferException(bool add_, size_t pos_, size_t esize_, size_t size_)
        : _add(add_), _pos(pos_), _esize(esize_), _size(size_)
    {
        PrintPosError();
    }

    void PrintPosError() const;

private:
    bool _add;
    size_t _pos;
    size_t _esize;
    size_t _size;
};

template<class T>
struct Unused
{
    Unused() {}
};

class ByteBuffer
{
public:
    const static size_t DEFAULT_SIZE = 0x1000; // TODO:FixMe

    ByteBuffer(): _rpos(0), _wpos(0)
    {
        _storage.reserve(DEFAULT_SIZE);
    }

    ByteBuffer(size_t res): _rpos(0), _wpos(0)
    {
        _storage.reserve(res);
    }

    ByteBuffer(const ByteBuffer& buf): _rpos(buf._rpos), _wpos(buf._wpos), _storage(buf._storage) { }

    void clear()
    {
        _storage.clear();
        _rpos = _wpos = 0;
    }

    template <typename T> 
	void put(size_t pos, T value)
    {
        bigEndian(value);
        put(pos, (uint8_t*)&value, sizeof(value));
    }

    ByteBuffer& operator<<(bool value)
    {
        append<int8_t>(value?1:0);
        return *this;
    }

    ByteBuffer& operator<<(uint8_t value)
    {
        append<uint8_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint16_t value)
    {
        append<uint16_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint32_t value)
    {
        append<uint32_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint64_t value)
    {
        append<uint64_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(int8_t value)
    {
        append<int8_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(int16_t value)
    {
        append<int16_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(int32_t value)
    {
        append<int32_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(int64_t value)
    {
        append<int64_t>(value);
        return *this;
    }

    ByteBuffer& operator<<(float value)
    {
        append<float>(value);
        return *this;
    }

    ByteBuffer& operator<<(double value)
    {
        append<double>(value);
        return *this;
    }

    ByteBuffer& operator<<(const std::string& value)
    {
        append(value);
        return *this;
    }

    ByteBuffer& operator<<(const char* str)
    {
        append(str, str ? strlen(str) : 0);
        return *this;
    }

    ByteBuffer& operator>>(bool& value)
    {
        value = read<int8_t>() > 0 ? true : false;
        return *this;
    }

    ByteBuffer& operator>>(uint8_t& value)
    {
        value = read<uint8_t>();
        return *this;
    }

    ByteBuffer& operator>>(uint16_t& value)
    {
        value = read<uint16_t>();
        return *this;
    }

    ByteBuffer& operator>>(uint32_t& value)
    {
        value = read<uint32_t>();
        return *this;
    }

    ByteBuffer& operator>>(uint64_t& value)
    {
        value = read<uint64_t>();
        return *this;
    }

    ByteBuffer& operator>>(int8_t& value)
    {
        value = read<int8_t>();
        return *this;
    }

    ByteBuffer& operator>>(int16_t& value)
    {
        value = read<int16_t>();
        return *this;
    }

    ByteBuffer& operator>>(int32_t& value)
    {
        value = read<int32_t>();
        return *this;
    }

    ByteBuffer& operator>>(int64_t& value)
    {
        value = read<int64_t>();
        return *this;
    }

    ByteBuffer& operator>>(float& value)
    {
        value = read<float>();
        return *this;
    }

    ByteBuffer& operator>>(double& value)
    {
        value = read<double>();
        return *this;
    }

    ByteBuffer& operator>>(std::string& value)
    {
        value.clear();

        uint16_t length_ = read<uint16_t>();

        //assert(rpos()+length_  <= size());
        value.reserve(length_);

        for (uint16_t i = 0u; i < length_; i++)
        {
            value += read<char>();
        }

        return *this;
    }

    template<class T>
    ByteBuffer& operator>>(Unused<T> const&)
    {
        read_skip<T>();
        return *this;
    }

    uint8_t operator[](size_t pos) const
    {
        return read<uint8_t>(pos);
    }

    size_t rpos() const { return _rpos; }

    size_t rpos(size_t rpos_)
    {
        _rpos = rpos_;
        return _rpos;
    }

    size_t wpos() const { return _wpos; }

    size_t wpos(size_t wpos_)
    {
        _wpos = wpos_;
        return _wpos;
    }

    template<typename T>
    void read_skip() { read_skip(sizeof(T)); }

    void read_skip(size_t skip)
    {
        if (_rpos + skip > size())
        {
            throw ByteBufferException(false, _rpos, skip, size());
        }

        _rpos += skip;
    }

    template <typename T> T read()
    {
        T r = read<T>(_rpos);
        _rpos += sizeof(T);

        return r;
    }

    template <typename T> T read(size_t pos) const
    {
        if (pos + sizeof(T) > size())
        {
            throw ByteBufferException(false, pos, sizeof(T), size());
        }

        T val = *((T const*)&_storage[pos]);
        bigEndian(val);
        return val;
    }

    void read(uint8_t* dest, size_t len)
    {
        if (_rpos  + len > size())
        {
            throw ByteBufferException(false, _rpos, len, size());
        }

        memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    const uint8_t* contents() const { return &_storage[0]; }

    size_t size() const { return _storage.size(); }
    size_t left() const { return size() - _rpos; }

    bool empty() const { return _storage.empty(); }

    void resize(size_t newsize)
    {
        _storage.resize(newsize);
        _rpos = 0;
        _wpos = size();
    }

    void reserve(size_t ressize)
    {
        if (ressize > size())
        {
            _storage.reserve(ressize);
        }
    }

    void append(const std::string& str)
    {
        assert(str.size() <= 0xFFFF);
        append<uint16_t>(str.size());

        append((const uint8_t*)str.c_str(), str.size());
    }

    void append(const char* src, size_t cnt)
    {
        assert(cnt <= 0xFFFF);
        append<uint16_t>(cnt);

        return append((const uint8_t*)src, cnt);
    }

    template<class T> void append(const T* src, size_t cnt)
    {
        return append((const uint8_t*)src, cnt * sizeof(T));
    }

    void append(const uint8_t* src, size_t cnt)
    {
        if (!cnt)
            return;

        assert(size() < 10000000);

        if (_storage.size() < _wpos + cnt)
        {
            _storage.resize(_wpos + cnt);
        }

        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
    }

    void append(const ByteBuffer& buffer)
    {
        if (buffer.wpos())
        {
            append(buffer.contents(), buffer.wpos());
        }
    }

    void put(size_t pos, const uint8_t* src, size_t cnt)
    {
        if (pos + cnt > size())
        {
            throw ByteBufferException(true, pos, cnt, size());
        }

        memcpy(&_storage[pos], src, cnt);
    }

    void print_storage() const;
    void textlike() const;
    void hexlike() const;
	
private:
    template <typename T> void append(T value)
    {
        bigEndian(value);
        append((uint8_t*)&value, sizeof(value));
    }

protected:
    size_t _rpos, _wpos;
    std::vector<uint8_t> _storage;
};

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::vector<T> const& v)
{
    assert(v.size() <= 0xFFFF);
    b << (uint16_t)v.size();
    for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }

    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::vector<T>& v)
{
    uint16_t vsize1;
    b >> vsize1;
    int32_t vsize = vsize1;
    v.clear();
    v.reserve(vsize);

    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }

    return b;
}

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::list<T> const& v)
{
    assert(v.size() <= 0xFFFF);
    b << (uint16_t)v.size();
    for (typename std::list<T>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }

    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::list<T>& v)
{
    uint16_t vsize1;
    b >> vsize1;
    int32_t vsize = vsize1;
    v.clear();
    v.reserve(vsize);

    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator<<(ByteBuffer& b, std::map<K, V>& m)
{
    assert(m.size() <= 0xFFFFF);
    b << (uint16_t)m.size();
    for (typename std::map<K, V>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator>>(ByteBuffer& b, std::map<K, V>& m)
{
    uint16_t msize1;
    b >> msize1;

    int32_t msize = msize1;
    m.clear();

    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }

    return b;
}

}

#endif // _MFW_BYTE_CONVERTER_H_
