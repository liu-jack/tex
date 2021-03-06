#ifndef _MFW_UTIL_TIME_
#define _MFW_UTIL_TIME_

#include <stdint.h>
#include <string>
using namespace std;

const uint32_t SECOND_PER_MINUTE	= 60;
const uint32_t MINUTE_PER_HOUR		= 60;
const uint32_t SECOND_PER_HOUR		= 3600;
const uint32_t HOUR_PER_DAY			= 24;
const uint32_t SECOND_PER_DAY		= 86400;

namespace mfw
{

class UtilTime
{
public:
    static uint32_t getNow();
    static uint64_t getNowMS();
    static uint64_t getNowUS();

    static string formatTime(uint32_t iTime, const char *fmt = "%Y-%m-%d %H:%M:%S");
    static uint32_t parseTime(const string &sTime, const char *fmt = "%Y-%m-%d %H:%M:%S");
    static void parseTime(const string &sTime, struct tm &stTm, const char *fmt = "%Y-%m-%d %H:%M:%S");

    // 获取小时数
    static uint32_t getHour(uint32_t iTime, int32_t offset = 0);

    // eg: 2014-08-20 transfer to 20140820
    static uint32_t getDate(uint32_t iTime, int32_t offset = 0);
    //@param iDate format: 20140820
    static uint32_t fromDate(uint32_t iDate, uint32_t iHour = 0);

    // eg："23:09:12" transfer to 230912
    static uint32_t parseDayTime(const string &sTime);
    // get hour,min,sec ,eg:230912
    static uint32_t getDayTime(uint32_t iTime);

    static uint32_t getTimeByDayOffset(uint32_t iTime, uint32_t iDayOffset, uint32_t iDayTimeOffset);
    static bool isInDayTimeRange(uint32_t iTime, uint32_t from, uint32_t end);
};

// 毫秒级定时器
class UtilTimer
{
public:
    UtilTimer(uint64_t iInterval, uint64_t iCurrTime)
        : m_iInterval(iInterval),
          m_iLastTriggerTime(iCurrTime)
    {
    }

    void reset(uint64_t iInterval, uint64_t iCurrTime);
    //检测定时器是否到时间
    bool operator()(uint64_t iCurrTime);

private:
    bool isReady(uint64_t iCurrTime);

private:
    uint64_t m_iInterval;
    uint64_t m_iLastTriggerTime;
};

}

#endif
