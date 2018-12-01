#ifndef _SDP_TEST_HPP_
#define _SDP_TEST_HPP_

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "sdp/Sdp.h"
using namespace std;
using namespace mfw;

namespace Test
{
enum NUMBER {
    NUMBER_1 = 1,
    NUMBER_2,
};
static inline const char *etos(NUMBER e)
{
    switch (e) {
    case NUMBER_1:
        return "NUMBER_1";
    case NUMBER_2:
        return "NUMBER_2";
    default:
        return "";
    }
}
static inline bool stoe(const string &s, NUMBER &e)
{
    if (s == "NUMBER_1") {
        e = NUMBER_1;
        return true;
    }
    if (s == "NUMBER_2") {
        e = NUMBER_2;
        return true;
    }
    return false;
}

struct Student {
    uint64_t iUid;
    string sName;
    uint32_t iAge;
    map<string, string> mSecret;

    Student():
        iUid(0),
        iAge(0)
    {
    }
    const char *getName() const
    {
        return "Student";
    }
    template <typename T>
    void visit(T &t, bool bOpt)
    {
        if (!bOpt) {
            if (iUid != 0) t.visit(0, false, "iUid", iUid);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
            if (iAge != 0) t.visit(2, false, "iAge", iAge);
            if (!mSecret.empty()) t.visit(3, false, "mSecret", mSecret);
        } else {
            t.visit(0, false, "iUid", iUid);
            t.visit(1, false, "sName", sName);
            t.visit(2, false, "iAge", iAge);
            t.visit(3, false, "mSecret", mSecret);
        }
    }
    template <typename T>
    void visit(T &t, bool bOpt) const
    {
        if (!bOpt) {
            if (iUid != 0) t.visit(0, false, "iUid", iUid);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
            if (iAge != 0) t.visit(2, false, "iAge", iAge);
            if (!mSecret.empty()) t.visit(3, false, "mSecret", mSecret);
        } else {
            t.visit(0, false, "iUid", iUid);
            t.visit(1, false, "sName", sName);
            t.visit(2, false, "iAge", iAge);
            t.visit(3, false, "mSecret", mSecret);
        }
    }
    void swap(Student &b)
    {
        using std::swap;
        swap(iUid, b.iUid);
        swap(sName, b.sName);
        swap(iAge, b.iAge);
        swap(mSecret, b.mSecret);
    }
    bool operator== (const Student &rhs) const
    {
        return iUid == rhs.iUid
               && sName == rhs.sName
               && iAge == rhs.iAge
               && mSecret == rhs.mSecret;
    }
    bool operator!= (const Student &rhs) const
    {
        return !((*this) == rhs);
    }
};

struct Teacher {
    uint32_t iId;
    string sName;

    Teacher():
        iId(0)
    {
    }
    const char *getName() const
    {
        return "Teacher";
    }
    template <typename T>
    void visit(T &t, bool bOpt)
    {
        if (!bOpt) {
            if (iId != 0) t.visit(0, false, "iId", iId);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
        } else {
            t.visit(0, false, "iId", iId);
            t.visit(1, false, "sName", sName);
        }
    }
    template <typename T>
    void visit(T &t, bool bOpt) const
    {
        if (!bOpt) {
            if (iId != 0) t.visit(0, false, "iId", iId);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
        } else {
            t.visit(0, false, "iId", iId);
            t.visit(1, false, "sName", sName);
        }
    }
    void swap(Teacher &b)
    {
        using std::swap;
        swap(iId, b.iId);
        swap(sName, b.sName);
    }
    bool operator== (const Teacher &rhs) const
    {
        return iId == rhs.iId
               && sName == rhs.sName;
    }
    bool operator!= (const Teacher &rhs) const
    {
        return !((*this) == rhs);
    }
};

struct Teachers {
    vector<Test::Teacher> vTeacher;

    Teachers() {}
    const char *getName() const
    {
        return "Teachers";
    }
    template <typename T>
    void visit(T &t, bool bOpt)
    {
        if (!bOpt) {
            if (!vTeacher.empty()) t.visit(0, false, "vTeacher", vTeacher);
        } else {
            t.visit(0, false, "vTeacher", vTeacher);
        }
    }
    template <typename T>
    void visit(T &t, bool bOpt) const
    {
        if (!bOpt) {
            if (!vTeacher.empty()) t.visit(0, false, "vTeacher", vTeacher);
        } else {
            t.visit(0, false, "vTeacher", vTeacher);
        }
    }
    void swap(Teachers &b)
    {
        using std::swap;
        swap(vTeacher, b.vTeacher);
    }
    bool operator== (const Teachers &rhs) const
    {
        return vTeacher == rhs.vTeacher;
    }
    bool operator!= (const Teachers &rhs) const
    {
        return !((*this) == rhs);
    }
};

struct Class {
    uint32_t iId;
    string sName;
    vector<Test::Student> vStudent;
    vector<char> vData;

    Class():
        iId(0)
    {
    }
    const char *getName() const
    {
        return "Class";
    }
    template <typename T>
    void visit(T &t, bool bOpt)
    {
        if (!bOpt) {
            t.visit(0, true, "iId", iId);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
            if (!vStudent.empty()) t.visit(2, false, "vStudent", vStudent);
            if (!vData.empty()) t.visit(3, false, "vData", vData);
        } else {
            t.visit(0, true, "iId", iId);
            t.visit(1, false, "sName", sName);
            t.visit(2, false, "vStudent", vStudent);
            t.visit(3, false, "vData", vData);
        }
    }
    template <typename T>
    void visit(T &t, bool bOpt) const
    {
        if (!bOpt) {
            t.visit(0, true, "iId", iId);
            if (!sName.empty()) t.visit(1, false, "sName", sName);
            if (!vStudent.empty()) t.visit(2, false, "vStudent", vStudent);
            if (!vData.empty()) t.visit(3, false, "vData", vData);
        } else {
            t.visit(0, true, "iId", iId);
            t.visit(1, false, "sName", sName);
            t.visit(2, false, "vStudent", vStudent);
            t.visit(3, false, "vData", vData);
        }
    }
    void swap(Class &b)
    {
        using std::swap;
        swap(iId, b.iId);
        swap(sName, b.sName);
        swap(vStudent, b.vStudent);
        swap(vData, b.vData);
    }
    bool operator== (const Class &rhs) const
    {
        return iId == rhs.iId
               && sName == rhs.sName
               && vStudent == rhs.vStudent
               && vData == rhs.vData;
    }
    bool operator!= (const Class &rhs) const
    {
        return !((*this) == rhs);
    }
};

}

namespace std
{
inline void swap(Test::Student &a, Test::Student &b)
{
    a.swap(b);
}
inline void swap(Test::Teacher &a, Test::Teacher &b)
{
    a.swap(b);
}
inline void swap(Test::Teachers &a, Test::Teachers &b)
{
    a.swap(b);
}
inline void swap(Test::Class &a, Test::Class &b)
{
    a.swap(b);
}
}

#endif
