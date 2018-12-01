#ifndef _MFW_UTIL_NONCOPYABLE_
#define _MFW_UTIL_NONCOPYABLE_

namespace mfw
{

class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable &);
    const noncopyable &operator=(const noncopyable &);
};

}

#endif
