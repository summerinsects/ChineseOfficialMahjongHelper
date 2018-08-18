
namespace calendar {

    // ref: http://howardhinnant.github.io/date_algorithms.html
    static FORCE_INLINE bool Gregorian_IsLeapYear(int y) {
        /**
        *  +------------+---------------------------------------------------
        *  | percentage | executed
        *  +------------+---------------------------------------------------
        *  | 75%        | y % 4 == 0, typically optimized to !(y & 0x3)
        *  | 24%        | (y % 4 == 0) && (y % 100 != 0)
        *  |  1%        | (y % 4 == 0) && (y % 100 != 0) && (y % 400 == 0)
        *  +------------+---------------------------------------------------
        */
        return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    }

    static FORCE_INLINE int Gregorian_LastDayOfMonth(int y, int m) {
        /**
        *  +------------+---------------------------------------------------
        *  | percentage | executed
        *  +------------+---------------------------------------------------
        *  | (91+2/3)%  | m != 2
        *  | 6.25%      | (m != 2) && (y % 4 == 0)
        *  |  2%        | (m != 2) && (y % 4 == 0) && (y % 100 != 0)
        *  |  (1/12)%   | (m != 2) && (y % 4 == 0) && (y % 100 != 0) && (y % 400 == 0)
        *  +------------+---------------------------------------------------
        */
        static const int8_t table[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        return ((m != 2 || !Gregorian_IsLeapYear(y)) ? table[m - 1] : 29);
    }

    static FORCE_INLINE int Gregorian_CaculateWeekDay(int y, int m, int d) {
        // 基姆拉尔森计算公式[0,6]->[Sun, Sat]
        if (m == 1 || m == 2) {
            m += 12;
            --y;
        }
        return (1 + d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    }

    /**
     * 农历数据
     * 编码结构：
     *           +------16---12---8---4----0+
     *           |DDDDDD|L|NNNNNNNNNNNN|LLLL|
     *           +------+-+------------+----+
     *    春节偏移-16 闰月大小 常规月大小 闰几月
     *
     * 例如2033年数据为0x1e4afb，即‭0001 1110 0100 1010 1111 1011
     *    这年闰十一月小（最后4位的1011，及第17位的0）
     *    ‬二、五、七、九、十、十一、十二月为大月（从16-4位的0100 1010 1111）
     *    春节偏移（01111）=15，15+16=31，在公历1月31号
     */
    static const int ChineseCalendarData[] = {

        0x32ab50,  // 1899
        0x1e4bd8, 0x444ae0, 0x2ea570, 0x1a54d5, 0x3ed260, 0x26d950, 0x136554, 0x3856a0, 0x229ad0, 0x0c55d2,  // 1900-1909
        0x324ae0, 0x1ca5b6, 0x42a4d0, 0x2ad250, 0x15d255, 0x3ab540, 0x24d6a0, 0x0eada2, 0x3495b0, 0x214977,  // 1910-1919
        0x464970, 0x2ea4b0, 0x18b4b5, 0x3e6a50, 0x286d40, 0x11ab54, 0x382b60, 0x229570, 0x0e52f2, 0x324970,  // 1920-1929
        0x1c6566, 0x40d4a0, 0x2aea50, 0x146e95, 0x3a5ad0, 0x262b60, 0x1186e3, 0x3492e0, 0x1fc8d7, 0x44c950,  // 1930-1939
        0x2ed4a0, 0x17d8a6, 0x3cb550, 0x2856a0, 0x13a5b4, 0x3825d0, 0x2292d0, 0x0cd2b2, 0x32a950, 0x1ab557,  // 1940-1949
        0x406ca0, 0x2ab550, 0x175355, 0x3a4da0, 0x24a5b0, 0x114573, 0x3652b0, 0x1ea9a8, 0x42e950, 0x2e6aa0,  // 1950-1959
        0x18aea6, 0x3cab50, 0x284b60, 0x12aae4, 0x38a570, 0x225260, 0x0af263, 0x30d950, 0x1c5b57, 0x4056a0,  // 1960-1969
        0x2a96d0, 0x164dd5, 0x3c4ad0, 0x24a4d0, 0x0ed4d4, 0x34d250, 0x1ed558, 0x42b540, 0x2cb6a0, 0x1995a6,  // 1970-1979
        0x3e95b0, 0x2849b0, 0x12a974, 0x38a4b0, 0x22b27a, 0x466a50, 0x306d40, 0x1aaf46, 0x40ab60, 0x2a9570,  // 1980-1989
        0x164af5, 0x3c4970, 0x2664b0, 0x0e74a3, 0x32ea50, 0x1e6b58, 0x4455c0, 0x2cab60, 0x1896d5, 0x3e92e0,  // 1990-1999
        0x28c960, 0x10d954, 0x36d4a0, 0x20da50, 0x0c7552, 0x3056a0, 0x1aabb7, 0x4225d0, 0x2c92d0, 0x14cab5,  // 2000-2009
        0x3aa950, 0x24b4a0, 0x0ebaa4, 0x32ad50, 0x1e55d9, 0x444ba0, 0x2ea5b0, 0x195176, 0x3e52b0, 0x28a930,  // 2010-2019
        0x127954, 0x366aa0, 0x20ad50, 0x0c5b52, 0x324b60, 0x1aa6e6, 0x40a4e0, 0x2ad260, 0x14ea65, 0x38d530,  // 2020-2029
        0x245aa0, 0x0e76a3, 0x3496d0, 0x1e4afb, 0x444ad0, 0x2ea4d0, 0x19d0b6, 0x3cd250, 0x26d520, 0x10dd45,  // 2030-2039
        0x36b5a0, 0x2056d0, 0x0c55b2, 0x3249b0, 0x1ca577, 0x40a4b0, 0x2aaa50, 0x15b255, 0x3a6d20, 0x22ada0,  // 2040-2049
        0x0f4b63, 0x349370, 0x2049f8, 0x444970, 0x2e64b0, 0x1968a6, 0x3cea50, 0x266b20, 0x11a6c4, 0x36aae0,  // 2050-2059
        0x22a2e0, 0x0ad2e3, 0x30c960, 0x1ad557, 0x40d4a0, 0x28da50, 0x145d55, 0x3a56a0, 0x24a6d0, 0x0e55d4,  // 2060-2069
        0x3452d0, 0x1ea9b8, 0x44a950, 0x2cb4a0, 0x16b6a6, 0x3cad50, 0x2855a0, 0x10aba4, 0x36a5b0, 0x2252b0,  // 2070-2079
        0x0cb273, 0x306930, 0x1a7337, 0x406aa0, 0x2aad50, 0x154b55, 0x3a4b60, 0x24a570, 0x1054e4, 0x32d160,  // 2080-2089
        0x1ce968, 0x42d520, 0x2cdaa0, 0x176aa6, 0x3c56d0, 0x284ae0, 0x12a9d4, 0x36a2d0, 0x20d150, 0x0af252,  // 2090-2099
        0x30d520  // 2100
    };

    static FORCE_INLINE int Chinese_GetLeapMonth(int y) {
        return (ChineseCalendarData[y - 1899] & 0xf);
    }

    static FORCE_INLINE bool Chinese_IsLeapMonthLong(int y) {
        return (ChineseCalendarData[y - 1899] & 0x10000);
    }

    static FORCE_INLINE bool Chinese_IsNormalMonthLong(int y, int m) {
        return (ChineseCalendarData[y - 1899] & (0x10000 >> m));
    }

    static FORCE_INLINE GregorianDate SpringFestervalInGregorian(int y) {
        int d = 16 + ((ChineseCalendarData[y - 1899] >> 17) & 0x3F);
        int m = 1;
        if (d > 31) d -= 31, ++m;
        return GregorianDate{ y, m, d };
    }

    static ChineseDate Gregorian2Chinese(const GregorianDate &gd) {
        // 年份限定、上限
        if (gd.year < 1900 || gd.year > 2100) {
            return ChineseDate();
        }

        static const int8_t GregorianDaysTable[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        // 年
        int year = gd.year;
        int offset = 0;
        GregorianDate sf = SpringFestervalInGregorian(year);  // 春节所在公历的日期
        if (gd.month > sf.month) {  // 春节所在的月份已过
            offset = GregorianDaysTable[sf.month] - sf.day;  // 春节之后的天数
            if (Gregorian_IsLeapYear(year)) ++offset;  // 公历闰年
            for (int m = sf.month + 1; m < gd.month; ++m) offset += GregorianDaysTable[m];  // 随后完整的月天数
            offset += gd.day;  // 当前月已过的天数
        }
        else if (gd.month == sf.month) {  // 与春节同月份
            if (gd.day >= sf.day) {  // 日期已过
                offset = gd.day - sf.day;
            }
            else {  // 日期未过，说明春节未过，农历倒退一年
                --year;
            }
        }
        else {  // 春节所在的月份未过，农历倒退一年
            --year;
        }

        if (year != gd.year) {  // 倒退一年的情况
            sf = SpringFestervalInGregorian(year);  // 春节所在公历的日期

            // 当前日期与去年元旦的间隔天数
            offset = (Gregorian_IsLeapYear(year) ? 366 : 365);  // 去年一整年的天数
            for (int m = 1; m < gd.month; ++m) {
                offset += GregorianDaysTable[m];
            }
            offset += gd.day;

            // 去年春节与去年元旦的间隔天数
            for (int m = 1; m < sf.month; ++m) {
                offset -= GregorianDaysTable[m];
            }
            offset -= sf.day;
        }

        // 月
        int month = 1;
        bool isInLeapMonth = false;
        int leapMonth = Chinese_GetLeapMonth(year); // 闰哪个月
        int lastDay = 0;
        do {
            if (leapMonth > 0 && month == leapMonth + 1) {  // 插入闰月
                lastDay = Chinese_IsLeapMonthLong(year) ? 30 : 29;
                if (offset < lastDay) {
                    isInLeapMonth = true;
                    --month;
                    break;
                }
                offset -= lastDay;
            }
            lastDay = Chinese_IsNormalMonthLong(year, month) ? 30 : 29;
            if (offset < lastDay) {
                break;
            }
            offset -= lastDay;
            ++month;
        } while (month < 13);

        assert(month < 13);

        // 日
        int day = offset + 1;

        ChineseDate date;
        date.year = year;
        date.month = month;
        date.day = day;
        date.is_leap = isInLeapMonth;
        date.is_long = (lastDay == 30);
        return date;
    }

    /**
     * 24节气编码
     * 每4位表示一个月的两个节气，从高到低分别为1-12月
     * 在每个月的4位中，高两位表示本月第1个节气，低两位表示本月第1个节气
     * 每月前一个节气偏移：1-6月+3，7-12月+6
     * 每月后一个节气偏移：1-5月+18，6月+19，7-12月+21
     * 特别的：1975年及以前的年份，偏移在上一步基础上再+1
     *
     * 例如：2018年数据为0xa5babe66aa55
     *   0xa为1月，(0xa>>2)&0x3=2，2+3=5，那么5号为小寒，0xa&0x3=2，2+18=20，20号为大寒
     */
    static const uint64_t SolarTermsData[] = {
        0x90a5aa155a50, 0xa0a6ba566a54, 0xa4aabe666a55, 0xa5fafe6aaa55, 0xe5a5aa155a50,  // 1900-1904
        0xa0a6ba566a54, 0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5a5aa155a50, 0xa0a6ba566a54,  // 1905-1909
        0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5a5aa155a50, 0x90a6ba566a54, 0xa0a6ba666a55,  // 1910-1914
        0xa5babe66aa55, 0xa5a5aa155640, 0x90a6aa565a50, 0xa0a6ba666a54, 0xa5babe66aa55,  // 1915-1919
        0xa5a5aa155640, 0x90a5aa565a50, 0xa0a6ba666a54, 0xa4aabe66aa55, 0xa5a5aa155640,  // 1920-1924
        0x90a5aa565a50, 0xa0a6ba566a54, 0xa4aabe666a55, 0xa5a5a9155500, 0x90a5aa155a50,  // 1925-1929
        0xa0a6ba566a54, 0xa4aabe666a55, 0xa5a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54,  // 1930-1934
        0xa4aaba666a55, 0xa5a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55,  // 1935-1939
        0xa5a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55, 0xa5a569155500,  // 1940-1944
        0x90a5aa155640, 0x90a6ba565a54, 0xa0a6ba666a55, 0xa56569115500, 0x50a5aa155640,  // 1945-1949
        0x90a5aa565a54, 0xa0a6ba666a55, 0xa56569115500, 0x50a5aa155640, 0x90a5aa565a50,  // 1950-1954
        0xa0a6ba566a54, 0xa55569115500, 0x50a5aa155640, 0x90a5aa155a50, 0xa0a6ba566a54,  // 1955-1959
        0xa45569111500, 0x50a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa45569111500,  // 1960-1964
        0x50a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa45565111500, 0x50a5a9155500,  // 1965-1969
        0x90a5aa155a50, 0xa0a6ba566a54, 0xa45565111500, 0x50a569155500, 0x90a5aa155a50,  // 1970-1974
        0xa0a6ba565a54, 0xf9a6ba666a55, 0xa5fabe66aa55, 0xe5faff6aaba5, 0xf5fbffabafa9,  // 1975-1979
        0xf9a6ba666a55, 0xa5fabe66aa55, 0xe5faff6aab95, 0xe5faffabafa9, 0xf5a6ba566a55,  // 1980-1984
        0xa5babe66aa55, 0xa5faff6aab95, 0xe5faff6bafa5, 0xf5a6ba566a54, 0xa5aabe666a55,  // 1985-1989
        0xa5fafe6aab95, 0xe5faff6aafa5, 0xf5a6ba566a54, 0xa4aabe666a55, 0xa5fafe6aaa55,  // 1990-1994
        0xe5faff6aafa5, 0xf5a6ba566a54, 0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5faff6aafa5,  // 1995-1999
        0xf5a6ba566a54, 0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5faff6aafa5, 0xf5a6ba566a54,  // 2000-2004
        0xa4aaba666a55, 0xa5fabe66aa55, 0xe5faff6aafa5, 0xf5a6ba565a54, 0xa4a6ba666a55,  // 2005-2009
        0xa5fabe66aa55, 0xe5faff6aaba5, 0xf5a6aa565a54, 0xa4a6ba566a55, 0xa5fabe66aa55,  // 2010-2014
        0xe5faff6aab95, 0xe5a5aa565a54, 0xa0a6ba566a55, 0xa5babe66aa55, 0xa5fafe6aab95,  // 2015-2019
        0xe5a5aa155a54, 0xa0a6ba566a54, 0xa5aabe666a55, 0xa5fafe6aab95, 0xe5a5aa155a50,  // 2020-2024
        0xa0a6ba566a54, 0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5a5aa155a50, 0xa0a6ba566a54,  // 2025-2029
        0xa4aaba666a55, 0xa5fafe6aaa55, 0xe5a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55,  // 2030-2034
        0xa5fabe66aa55, 0xe5a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55, 0xa5fabe66aa55,  // 2035-2039
        0xe5a5aa155a50, 0xa0a6aa565a54, 0xa4a6ba666a55, 0xa5fabe66aa55, 0xe5a5aa155650,  // 2040-2044
        0xa0a5aa565a54, 0xa4a6ba566a55, 0xa5fabe66aa55, 0xe5a5a9155640, 0x90a5aa155a54,  // 2045-2049
        0xa0a6ba566a55, 0xa5aabe666a55, 0xa5a5a9155640, 0x90a5aa155a54, 0xa0a6ba566a55,  // 2050-2054
        0xa5aaba666a55, 0xa5a5a9155640, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa5aaba666a55,  // 2055-2059
        0xa5a5a9155500, 0x90a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55, 0xa5a5a9155500,  // 2060-2064
        0x90a5aa155a50, 0xa0a6ba566a54, 0xa4aaba666a55, 0xa5a569115500, 0x90a5aa155a50,  // 2065-2069
        0xa0a6aa565a54, 0xa4aaba666a55, 0xa5a569115500, 0x90a5aa155650, 0xa0a6aa565a54,  // 2070-2074
        0xa4a6ba566a55, 0xa5a569115500, 0x90a5aa155650, 0xa0a5aa165a54, 0xa4a6ba566a55,  // 2075-2079
        0xa5a569115500, 0x90a5a9155640, 0xa0a5aa155a54, 0xa0a6ba566a55, 0xa55569111500,  // 2080-2084
        0x50a5a9155640, 0x90a5aa155a54, 0xa0a6ba566a55, 0xa55565111500, 0x50a5a9155640,  // 2085-2089
        0x90a5aa155a50, 0xa0a6ba566a54, 0xa55565111500, 0x50a5a9155500, 0x90a5aa155a50,  // 2090-2094
        0xa0a6ba566a54, 0xa45565111500, 0x50a5a9115500, 0x90a5aa155a50, 0xa0a6ba566a54,  // 2095-2099
        0xa4aaba666a55  // 2100
    };

    // 校正参数
    static const int8_t SolarTermsOffset1[] = { 3, 3, 3, 3, 3, 3, 6, 6, 6, 6, 6, 6 };
    static const int8_t SolarTermsOffset2[] = { 18, 18, 18, 18, 18, 19, 21, 21, 21, 21, 21, 21 };

    static std::pair<int, int> GetSolarTermsInGregorianMonth(int y, int m) {
        uint64_t data = (SolarTermsData[y - 1900] >> ((12 - m) * 4)) & 0xF;
        int st1 = ((data >> 2) & 0x3) + SolarTermsOffset1[m - 1];
        int st2 = (data & 0x3) + SolarTermsOffset2[m - 1];
        if (y <= 1975) {
            ++st1, ++st2;
        }
        return std::make_pair(st1, st2);
    }

    static const char *SolarTermsText[] = {
        __UTF8("小寒"), __UTF8("大寒"),
        __UTF8("立春"), __UTF8("雨水"), __UTF8("惊蛰"), __UTF8("春分"), __UTF8("清明"), __UTF8("谷雨"),
        __UTF8("立夏"), __UTF8("小满"), __UTF8("芒种"), __UTF8("夏至"), __UTF8("小暑"), __UTF8("大暑"),
        __UTF8("立秋"), __UTF8("处暑"), __UTF8("白露"), __UTF8("秋分"), __UTF8("寒露"), __UTF8("霜降"),
        __UTF8("立冬"), __UTF8("小雪"), __UTF8("大雪"), __UTF8("冬至")
    };

    static const char *SexagesimalCycleName[60] = {
        __UTF8("甲子"), __UTF8("乙丑"), __UTF8("丙寅"), __UTF8("丁卯"), __UTF8("戊辰"), __UTF8("己巳"), __UTF8("庚午"), __UTF8("辛未"), __UTF8("壬申"), __UTF8("癸酉"),
        __UTF8("甲戌"), __UTF8("乙亥"), __UTF8("丙子"), __UTF8("丁丑"), __UTF8("戊寅"), __UTF8("己卯"), __UTF8("庚辰"), __UTF8("辛巳"), __UTF8("壬午"), __UTF8("癸未"),
        __UTF8("甲申"), __UTF8("乙酉"), __UTF8("丙戌"), __UTF8("丁亥"), __UTF8("戊子"), __UTF8("己丑"), __UTF8("庚寅"), __UTF8("辛卯"), __UTF8("壬辰"), __UTF8("癸巳"),
        __UTF8("甲午"), __UTF8("乙未"), __UTF8("丙申"), __UTF8("丁酉"), __UTF8("戊戌"), __UTF8("己亥"), __UTF8("庚子"), __UTF8("辛丑"), __UTF8("壬寅"), __UTF8("癸卯"),
        __UTF8("甲辰"), __UTF8("乙巳"), __UTF8("丙午"), __UTF8("丁未"), __UTF8("戊申"), __UTF8("己酉"), __UTF8("庚戌"), __UTF8("辛亥"), __UTF8("壬子"), __UTF8("癸丑"),
        __UTF8("甲寅"), __UTF8("乙卯"), __UTF8("丙辰"), __UTF8("丁巳"), __UTF8("戊午"), __UTF8("己未"), __UTF8("庚申"), __UTF8("辛酉"), __UTF8("壬戌"), __UTF8("癸亥")
    };

    static const char *Chinese_MonthText[] = {
        __UTF8("正月"), __UTF8("二月"), __UTF8("三月"), __UTF8("四月"), __UTF8("五月"), __UTF8("六月"), __UTF8("七月"), __UTF8("八月"), __UTF8("九月"), __UTF8("十月"), __UTF8("十一月"), __UTF8("十二月")
    };

    static const char *Chinese_DayText[] = {
        __UTF8("初一"), __UTF8("初二"), __UTF8("初三"), __UTF8("初四"), __UTF8("初五"), __UTF8("初六"), __UTF8("初七"), __UTF8("初八"), __UTF8("初九"), __UTF8("初十"),
        __UTF8("十一"), __UTF8("十二"), __UTF8("十三"), __UTF8("十四"), __UTF8("十五"), __UTF8("十六"), __UTF8("十七"), __UTF8("十八"), __UTF8("十九"), __UTF8("二十"),
        __UTF8("廿一"), __UTF8("廿二"), __UTF8("廿三"), __UTF8("廿四"), __UTF8("廿五"), __UTF8("廿六"), __UTF8("廿七"), __UTF8("廿八"), __UTF8("廿九"), __UTF8("三十")
    };

    static std::string GetChineseDateTextShort(const calendar::ChineseDate &date) {
        if (date.day > 1) {
            return Chinese_DayText[date.day - 1];
        }
        else {
            std::string str;
            if (date.is_leap) str.append(__UTF8("闰"));
            str.append(Chinese_MonthText[date.month - 1]);
            str.append(date.is_long ? __UTF8("大") : __UTF8("小"));
            return str;
        }
    }

    static std::string GetChineseDateTextLong(const calendar::ChineseDate &date) {
        std::string str = SexagesimalCycleName[(date.year + 56) % 60];
        str.append(__UTF8("年"));
        if (date.is_leap) str.append(__UTF8("闰"));
        str.append(Chinese_MonthText[date.month - 1]);
        str.append(Chinese_DayText[date.day - 1]);
        return str;
    }

    enum GregorianDateFestival {
        GREGORIAN_FESTIVAL_NONE,
        /*  1月 */ NEW_YEARS_DAY,
        /*  2月 */ VALENTINES_DAY,
        /*  3月 */ WOMENS_DAY, CHINA_ARBOR_DAY, WORLD_CONSUMER_RIGHTS_DAY,
        /*  4月 */ APRIL_FOOLS_DAY, COLD_FOOD_FESTIVAL,
        /*  5月 */ LABOUR_DAY, CHINESE_YOUTH_DAY, MOTHERS_DAY,
        /*  6月 */ CHILDRENS_DAY, FATHERS_DAY,
        /*  7月 */ PARTYS_DAY,
        /*  8月 */ ARMY_DAY,
        /*  9月 */ VICTORY_MEMORIAL_DAY, TEACHERS_DAY,
        /* 10月 */ NATIONAL_DAY,
        /* 11月 */ THANKSGIVING_DAY,
        /* 12月 */ NATIONAL_MEMORIAL_DAY, CHRISTMAS_EVE, CHRISTMAS_DAY
    };
    static const char *GregorianDateFestivalNames[] = {
        nullptr,
        /*  1月 */ __UTF8("元旦"),
        /*  2月 */ __UTF8("情人节"),
        /*  3月 */ __UTF8("妇女节"), __UTF8("植树节"), __UTF8("消费者权益日"),
        /*  4月 */ __UTF8("愚人节"), __UTF8("寒食节"),
        /*  5月 */ __UTF8("劳动节"), __UTF8("青年节"), __UTF8("母亲节"),
        /*  6月 */ __UTF8("儿童节"), __UTF8("父亲节"),
        /*  7月 */ __UTF8("建党节"),
        /*  8月 */ __UTF8("建军节"),
        /*  9月 */ __UTF8("抗战胜利"), __UTF8("教师节"),
        /* 10月 */ __UTF8("国庆节"),
        /* 11月 */ __UTF8("感恩节"),
        /* 12月 */ __UTF8("国家公祭日"), __UTF8("平安夜"), __UTF8("圣诞节")
    };

    static std::pair<GregorianDateFestival, int> GetGregorianFestival(const calendar::GregorianDate &date, int offset, const std::pair<int, int> &solarTerms) {
        int md = date.month << 8 | date.day;
        switch (md) {
        case 0x0101: return std::make_pair(NEW_YEARS_DAY, 3);
        case 0x020E: return std::make_pair(VALENTINES_DAY, 2);
        case 0x0308: if (date.year > 1975) return std::make_pair(WOMENS_DAY, 3); break;
        case 0x030C: if (date.year >= 1979) return std::make_pair(CHINA_ARBOR_DAY, 2); break;
        case 0x030F: if (date.year > 1983) return std::make_pair(WORLD_CONSUMER_RIGHTS_DAY, 2); break;
        case 0x0401: return std::make_pair(APRIL_FOOLS_DAY, 2);
        case 0x0501: if (date.year > 1949) return std::make_pair(LABOUR_DAY, 3); break;
        case 0x0504: if (date.year > 1949) return std::make_pair(CHINESE_YOUTH_DAY, 2); break;
        case 0x0601: if (date.year > 1949) return std::make_pair(CHILDRENS_DAY, 2); break;
        case 0x0701: if (date.year >= 1938) return std::make_pair(PARTYS_DAY, 2); break;
        case 0x0801: if (date.year >= 1933) return std::make_pair(ARMY_DAY, 2); break;
        case 0x0903: if (date.year >= 1945) return std::make_pair(VICTORY_MEMORIAL_DAY, 2); break;
        case 0x090A: if (date.year >= 1985) return std::make_pair(TEACHERS_DAY, 2); break;
        case 0x0A01: if (date.year >= 1949) return std::make_pair(NATIONAL_DAY, 3); break;
        case 0x0C0D: if (date.year >= 2014) return std::make_pair(NATIONAL_MEMORIAL_DAY, 2); break;
        case 0x0C18: return std::make_pair(CHRISTMAS_EVE, 2);
        case 0x0C19: return std::make_pair(CHRISTMAS_DAY, 2);
        default:
            switch (date.month) {
            case 4: if (date.day == solarTerms.first - 1) return std::make_pair(COLD_FOOD_FESTIVAL, 2); break;
            case 5: if (date.year >= 1913 && date.day == (offset > 0 ? 15 - offset : 8)) return std::make_pair(MOTHERS_DAY, 1); break;
            case 6: if (date.year >= 1910 && date.day == (offset > 0 ? 22 - offset : 15)) return std::make_pair(FATHERS_DAY, 1); break;
            case 11: if (date.day == (offset < 5 ? 26 - offset : 33 - offset)) return std::make_pair(THANKSGIVING_DAY, 1); break;
            default: break;
            }
            break;
        }
        return std::make_pair(GREGORIAN_FESTIVAL_NONE, 0);
    }

    enum ChineseDateFestival {
        CHINESE_FESTIVAL_NONE,
        /* 正　月 */ SPRING_FESTIVAL, LANTERN_FESTIVAL,
        /* 二　月 */ DRAGON_HEAD_RAISING_FESTIVAL,
        /* 三　月 */ SHANGSI_FESTIVAL,
        /* 四　月 */ BUDDHAS_BIRTHDAY ,
        /* 五　月 */ DRAGON_BOAT_FESTIVAL,
        /* 六　月 */
        /* 七　月 */ DOUBLE_SEVENTH_FESTIVAL, GHOST_FESTIVAL,
        /* 八　月 */ MID_AUTUMN_FESTIVAL,
        /* 九　月 */ DOUBLE_NINTH_FESTIVAL,
        /* 十　月 */ WINTER_CLOTHING_FESTIVAL, XIA_YUAN_FESTIVAL,
        /* 十一月 */
        /* 十二月 */ LABA_FESTIVAL, NORTHERN_OFF_YEAR, SOUTHERN_OFF_YEAR, SPRING_FESTIVAL_EVE
    };
    static const char *ChineseDateFestivalNames[] = {
        nullptr,
        /* 正　月 */ __UTF8("春节"), __UTF8("元宵节"),
        /* 二　月 */ __UTF8("龙头节"),
        /* 三　月 */ __UTF8("上巳节"),
        /* 四　月 */ __UTF8("佛诞"),
        /* 五　月 */ __UTF8("端午节"),
        /* 六　月 */
        /* 七　月 */ __UTF8("七夕节"), __UTF8("中元节"),
        /* 八　月 */ __UTF8("中秋节"),
        /* 九　月 */ __UTF8("重阳节"),
        /* 十　月 */ __UTF8("寒衣节"), __UTF8("下元节"),
        /* 十一月 */
        /* 十二月 */ __UTF8("腊八节"), __UTF8("北方小年"), __UTF8("南方小年"), __UTF8("除夕")
    };
    static std::pair<ChineseDateFestival, int> GetChineseFestival(const calendar::ChineseDate &date) {
        if (date.is_leap) return std::make_pair(CHINESE_FESTIVAL_NONE, 0);
        int md = date.month << 8 | date.day;
        switch (md) {
        case 0x0101: return std::make_pair(SPRING_FESTIVAL, 3);
        case 0x010F: return std::make_pair(LANTERN_FESTIVAL, 2);
        case 0x0202: return std::make_pair(DRAGON_HEAD_RAISING_FESTIVAL, 1);
        case 0x0303: return std::make_pair(SHANGSI_FESTIVAL, 1);
        case 0x0408: return std::make_pair(BUDDHAS_BIRTHDAY, 1);
        case 0x0505: return std::make_pair(DRAGON_BOAT_FESTIVAL, 3);
        case 0x0707: return std::make_pair(DOUBLE_SEVENTH_FESTIVAL, 1);
        case 0x070F: return std::make_pair(GHOST_FESTIVAL, 1);
        case 0x080F: return std::make_pair(MID_AUTUMN_FESTIVAL, 3);
        case 0x0909: return std::make_pair(DOUBLE_NINTH_FESTIVAL, 1);
        case 0x0A01: return std::make_pair(WINTER_CLOTHING_FESTIVAL, 1);
        case 0x0A0F: return std::make_pair(XIA_YUAN_FESTIVAL, 1);
        case 0x0C08: return std::make_pair(LABA_FESTIVAL, 1);
        case 0x0C17: return std::make_pair(NORTHERN_OFF_YEAR, 1);
        case 0x0C18: return std::make_pair(SOUTHERN_OFF_YEAR, 1);
        case 0x0C1D: if (!date.is_long) return std::make_pair(SPRING_FESTIVAL_EVE, 3); break;
        case 0x0C1E: return std::make_pair(SPRING_FESTIVAL_EVE, 3);
        default: break;
        }
        return std::make_pair(CHINESE_FESTIVAL_NONE, 0);
    }
}
