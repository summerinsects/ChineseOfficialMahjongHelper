#ifndef _CALENDAR_H_
#define _CALENDAR_H_

namespace calendar {
    typedef unsigned int U32;
    typedef unsigned short U16;
    typedef unsigned char U8;

    static constexpr U16 CALENDAR_MIN_YEAR = 1800;
    static constexpr U16 CALENDAR_MAX_YEAR = 2299;

    struct GregorianDate {
        U16 year;
        U8 month;
        U8 day;
    };

    struct ChineseDate {
        U16 year;
        U8 month;
        U8 day;
        bool major;
        bool leap;
    };

    static inline bool Gregorian_IsLeapYear(int y) {
        // 75% y % 4 == 0
        // 24% (y % 4 == 0) && (y % 100 != 0)
        //  1% (y % 4 == 0) && (y % 100 != 0) && (y % 400 == 0)
        return (!(y & 3) && (y % 100 != 0 || y % 400 == 0));
    }

    static inline U8 Gregorian_DaysInMonth(int y, int m) {
        // (91_2/3)% m != 2
        // 6.25% (m != 2) && (y % 4 == 0)
        // 2% (m != 2) && (y % 4 == 0) && (y % 100 != 0)
        // (1/12)% (m != 2) && (y % 4 == 0) && (y % 100 != 0) && (y % 400 == 0)
        static const U8 table[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        return ((m != 2 || !Gregorian_IsLeapYear(y)) ? table[m - 1] : 29);
    }

    static inline U8 Gregorian_WeekDay(int y, int m, int d) {
        // 基姆拉尔森计算公式优化
        if (m == 1 || m == 2) {
            m += 12;
            --y;
        }

        // 每年按365天算，365%7=1，y年多出y天
        // 四年一闰，百年不闰，四百年又闰：y/4-y/100+y/400
        // 每月按30天算，30%7=2，m月多出2m天
        // 大月累积天数神奇公式(19(m'+1))>>5
        return (y + (y >> 2) - y / 100 + y / 400 + (m << 1) + (((m - 2) * 19) >> 5) + d + 3) % 7;
    }

    namespace detail {
        template <class Dummy>
        struct Impl {
            static constexpr U32 _CALENDAR_YEAR_START = CALENDAR_MIN_YEAR - 1;
            static const U32 _NEW_MOONS[];
            static const U32 _SOLAR_TERMS[];
            static const U8 _SOLAR_TERMS_BASE[];
            static constexpr U32 _SOLAR_TERMS_BASE_OFFSET_18XX = 0x83a6c8;
            static constexpr U32 _SOLAR_TERMS_BASE_OFFSET_19XX = 0x83e7f9;
            static constexpr U32 _SOLAR_TERMS_BASE_OFFSET_21XX = 0x008080;
            static constexpr U32 _SOLAR_TERMS_BASE_OFFSET_22XX = 0x828288;
            static const U8 _SOLAR_TERMS_OFFSET_TABLE_18XX[];
            static const U8 _SOLAR_TERMS_OFFSET_TABLE_19XX[];
            static const U8 _SOLAR_TERMS_OFFSET_TABLE_20XX[];
            static const U8 _SOLAR_TERMS_OFFSET_TABLE_21XX[];
            static const U8 _SOLAR_TERMS_OFFSET_TABLE_22XX[];

            static U32 _DayOffset(U16 y, U8 m, U8 d) {
                if (m < 3) {
                    m += 12;
                    --y;
                }
                return y * 365 + (y >> 2) - y / 100 + y / 400 + (m - 3) * 30 + (((m - 2) * 19) >> 5) + d;
            }

            static U8 _GetNewMoons(U16 y, U32 (&new_moons)[26], U8 start_idx);
            static ChineseDate Gregorian2Chinese(const GregorianDate &date);
            static U8 Gregorian_GetSolarTerm(U16 y, U8 i);
            static U16 Gregorian_GetSolarTerms(U16 y, U8 m);
            static U8 Gregorian_GetStemBranch(U16 y, U8 m, U8 d);
        };

        /**
         * 24节气编码
         * 注：这里适配公历，小寒为0
         *
         * 天文知识：
         * 回归年的长度为365.2422天，即365天5小时48分46秒
         * 但同一个节气，并不是简单地往后加365.2422就是下一年的该节气，
         * 因为地球公转速度不均匀、远近日点变化、章动、月球轨道影响等，不同节气的回归年是不同的。
         *
         * 以下简单计算，节气倒退1天需要多少年
         * 假如某节气今年在D号00:00:00，该节气1年后就会在D+0.2422，2年后在D+0.4844，3年后在D+0.7266，
         * 4年后本应在D+0.9688，然而被闰年扳回1天，变成D-0.0312
         * 于是可以4年为一组找规律
         * 对于4Y+1这一组，0.2422-0.0312x<0 ⇒ x>7_119/156
         * 对于4Y+2这一组，0.4844-0.0312x<0 ⇒ x>15_41/78
         * 对于4Y+3这一组，0.7266-0.0312x<0 ⇒ x>23_15/52
         * 对于4Y+4这一组，-0.0312-0.0312x<1 ⇒ x>33_2/39
         * 所以4Y+1首先倒退一天，在第8次后，便倒退1天
         *
         * 以4年为一组，同一个节气呈现出有规律的倒退：
         * 以4的倍数年当作每组的开头，将每组年份表示为(4Y,4Y+1,4Y+2,4Y+3)，(1,1,1,1)表示每组的节气
         * 对于1、2月的节气（小寒~雨水），变化过程为：
         * (1,1,1,1)→(1,0,1,1)→(1,0,0,1)→(1,0,0,0)→(0,0,0,0)
         * 对于3~12月的节气（惊蛰~冬至），变化过程为：
         * (1,1,1,1)→(0,1,1,1)→(0,0,1,1)→(0,0,0,1)→(0,0,0,0)
         * 注：之所以3~12月与1、2月的规律不同，是因为公历闰年在2月会多一天，以小寒和惊蛰为例：
         * 4Y年的小寒与4Y+1年的小寒包含了4Y年的2月29号，因此相隔366天；而4Y年的惊蛰与4Y+1年的惊蛰不包含4Y年的2月29号，因此相隔365天
         * 类似的，4Y+3年的小寒与4Y+4年的小寒相隔365天；而4Y+3年的惊蛰与4Y+4年的惊蛰相隔366天
         *
         * 每组持续约8次，也有偶尔出现7次或9次的，多是由于节气时刻在0点附近，导致前后相差一次
         * 8次*4年*(4+1)个周期=160>100，所以在一个世纪内，节气只会在2日内波动
         * 一般情况，世纪初为N+1、N之间波动，世纪末切换为N、N-1之间波动，
         * 运气好的话，世纪初统一为N，世纪末统一为N-1，整个世纪内都在N、N-1之间波动
         * 维持以上规律直到遇世纪之交的整百年不闰，会突然从N、N-1之间波动，切换为N+1、N
         *
         * 由于2000年是闰年，所以20和21世纪之间没有切换，更容易看出规律：
         * 比如清明，1943年最后一次出现6号，
         * 1944年进入(5,5,5,5)周期，持续7组：1944,1948,1952,1956,1960,1964,1972
         * 1976年进入(4,5,5,5)周期，持续8组：1976,1980,1984,1988,1992,1996,2000,2004
         * 2008年进入(4,4,5,5)周期，持续8组：2008,2012,2016,2020,2024,2028,2032,2036
         * 2040年进入(4,4,4,5)周期，持续8组：2040,2044,2048,2052,2056,2060,2064,2068
         * 2072年进入(4,4,4,4)周期，持续8组：2072,2076,2080,2084,2088,2092,2096,(2100)
         * 由于2100年不闰，扳回一天，变成(5,5,5,5)，
         * 2104年进入(4,5,5,5)周期，持续8组：2104,2108,2112,2116,2120,2124,2128,2132
         * 那么，什么时候会再度出现6号的清明节呢？
         * 根据上面的规律，(4,4,4,4)周期要从2104+32*3=2200年开始，刚好被扳回一天，再度变成(5,5,5,5)
         * 整个23世纪没有6号的清明，等到23世纪变成(4,4,4,5)周期，已经是2200+32*3=2296年，
         * 此周期第2次遇到2300年扳回一天后变成(5,5,5,6)，因此2303年才再度出现6号的清明节
         *
         * 再比如立春，1980年最后一次出现5号，
         * 1984年进入(4,4,4,4)周期，持续8组：1984,1988,1992,1996,2000,2004,2008,2012
         * 2016年进入(4,3,4,4)周期，持续8组：2016,2020,2024,2028,2032,2036,2040,2044
         * 2048年进入(4,3,3,4)周期，持续8组：2048,2052,2056,2060,2064,2068,2072,2076
         * 2080年进入(4,3,3,3)周期，持续10组：2080,2084,2088,2092,2096,(2100,2104,2108,2112,2116)
         * 由于2100年不闰，2100年会扳回一天，变成(5,4,4,4)，
         * 与3~12月节气不同的是，立春在2月初，2100当年并不会被少闰的一天所扳回
         * 2120年进入(4,4,4,4)周期，持续8组：2120,2124,2128,2132,2136,2140,2144,2148
         * 由此可见，公历的4年一闰确实是闰多了，需要百年不闰来校正，否则，24节气就会一直前移
         *
         * 因此这里设计算法用每1位表示一个节气，再配合校正参数得到具体日期
         *
         * 例如：2018年数据为0x83a6ec，即0b0'1000'001110'100110'111011'00
         *   第1位小寒值0，具体日期为：(基数5)+0=5号
         *   第3位立春值1，具体日期为：(基数4)+1-(2017年起立春偏移1)=4号
         * 对于21世纪以外的年份，除了OFFSET_TABLE外，还有世纪基数偏移BASE_OFFSET，也是每位表示+1
         * 例如：1999年数据为0xbd6d79，即0b0'1011'110101'101101'011110'01
         *   第8位谷雨值1，具体日期为：(基数19)+1+(20世纪基数偏移0)=20号
         *   第24位冬至值1，具体日期为：(基数21)+1+(20世纪基数偏移1)-(1988年起冬至偏移1)=22号
         */
        template <class Dummy>
        const U32 Impl<Dummy>::_SOLAR_TERMS[] = {
            0x000000,
            0x004930, 0x3cd931, 0x7efbb3, 0xfffff7, 0x00493f, 0x3cd931, 0x7edbb1, 0xfefff3, 0x00493f, 0x3c4930,  // 1800~1809
            0x7edbb1, 0xfefff3, 0x00412f, 0x1c4930, 0x7edbb1, 0x7efbb3, 0x00412f, 0x1c4930, 0x3edbb1, 0x7efbb3,  // 1810~1819
            0x004027, 0x184930, 0x3cdbb1, 0x7efbb3, 0x004007, 0x084930, 0x3cd931, 0x7efbb3, 0x000007, 0x104930,  // 1820~1829
            0x3cd931, 0x7efbb3, 0x000007, 0x004930, 0x3cd931, 0x7edbb3, 0x010007, 0x014930, 0x3d4931, 0x7fdbb3,  // 1830~1839
            0x000007, 0x014130, 0x3d4931, 0x7fdbb1, 0x000403, 0x014530, 0x3d4d30, 0x7fdfb1, 0x800003, 0x814460,  // 1840~1849
            0x9d4d70, 0xffdff1, 0x000003, 0x814460, 0x994d70, 0xbddf71, 0x000003, 0x810440, 0x994d78, 0xbddd79,  // 1850~1859
            0x00000b, 0x810440, 0x994d78, 0xbddd79, 0x00200b, 0x812440, 0x896d78, 0xbdfd79, 0x00000b, 0x802440,  // 1860~1869
            0x816578, 0xbd6d79, 0x00000b, 0x802440, 0x816578, 0xbd6d79, 0x000009, 0x802040, 0x81657c, 0xbd6d7d,  // 1870~1879
            0x02000d, 0x822000, 0x83646c, 0x9f6d7c, 0x42000d, 0x422000, 0xc3646c, 0xdb6d7c, 0x00000d, 0x422280,  // 1880~1889
            0xc326c4, 0xdb6ffc, 0x00000d, 0x422280, 0xc326c4, 0xdb6ffc, 0x00900d, 0x429280, 0xc3b6c4, 0xcbfffc,  // 1890~1899
            0x3c0800, 0x7e9a82, 0xfebec6, 0xfffffe, 0x3c080f, 0x7e9a82, 0xfebac6, 0xfffffe, 0x3c080f, 0x7e9a82,  // 1900~1909
            0xfebac6, 0xfffffe, 0x3c080f, 0x7e9a80, 0xfeba82, 0xffbeee, 0x18080e, 0x3c9880, 0x7eba82, 0xffbeee,  // 1910~1919
            0x18080e, 0x3c9800, 0x7eba82, 0xffbec6, 0x18080e, 0x3c9800, 0x7e9a82, 0xfebec6, 0x00000e, 0x3c0800,  // 1920~1929
            0x7e9a82, 0xfebec6, 0x00000e, 0x3c0800, 0x7e9a82, 0xfebac6, 0x00000e, 0x3c0800, 0x7e9a82, 0xfebac6,  // 1930~1939
            0x00000e, 0x3c0800, 0x7e9a82, 0xfebac6, 0x00000e, 0x180900, 0x7c9b80, 0xfebb82, 0x00000e, 0x184910,  // 1940~1949
            0x7cd911, 0xfefb93, 0x00000f, 0x184910, 0x3cd911, 0x7edb93, 0x00000f, 0x184930, 0x3c4931, 0x7edbb3,  // 1950~1959
            0x000007, 0x014130, 0x3d4931, 0x7fdbb3, 0x000007, 0x014130, 0x3d4931, 0x7fdbb3, 0x000007, 0x014530,  // 1960~1969
            0x3d4d31, 0x7fdfb3, 0x000007, 0x014430, 0x3d4d31, 0x7ddfb3, 0x000007, 0x010470, 0x394d71, 0x7dddf1,  // 1970~1979
            0x000007, 0x010470, 0x194d71, 0x7ddd71, 0x000003, 0x012460, 0x196d70, 0x3ded71, 0x000003, 0x802440,  // 1980~1989
            0x996570, 0xbd6d71, 0x000003, 0x802440, 0x816578, 0xbd6d79, 0x00000b, 0x802040, 0x816578, 0xbd6d79,  // 1990~1999
            0x02828b, 0x82a2c0, 0x83e7f8, 0xbfeff9, 0x02828b, 0x82a2c0, 0x83a6f8, 0xbfeff9, 0x00808b, 0x82a280,  // 2000~2009
            0x83a6f8, 0xbbeff9, 0x00808b, 0x828280, 0x83a6f8, 0x9beff9, 0x008009, 0x828280, 0x83a6ec, 0x9be7fc,  // 2010~2019
            0x00000d, 0x029280, 0x82b6cc, 0x9bf7fc, 0x00000d, 0x429280, 0xc2b2c4, 0xc3f7fc, 0x00000d, 0x429280,  // 2020~2029
            0xc2b2c4, 0xc3f7fc, 0x00000d, 0x429280, 0xc2b2c4, 0xc3b6fc, 0x00000d, 0x429280, 0xc2b2c4, 0xc3b6fc,  // 2030~2039
            0x00000d, 0x409080, 0xc2b284, 0xc3b6fc, 0x00000d, 0x449000, 0xc69284, 0xc7b6fc, 0x00000f, 0x640800,  // 2040~2049
            0xe69a82, 0xe6bece, 0x00000e, 0x640800, 0xe69a82, 0xe6bace, 0x00000e, 0x240800, 0x669a82, 0xe6bace,  // 2050~2059
            0x00000e, 0x3c0800, 0x7e9a82, 0xfebac6, 0x00000e, 0x3c0800, 0x7e9a82, 0xfebac6, 0x00000e, 0x3c4900,  // 2060~2069
            0x7cd982, 0xfefbc6, 0x00000e, 0x384900, 0x7cd982, 0xfedb86, 0x00000e, 0x384900, 0x7cc902, 0xfedb86,  // 2070~2079
            0x00000e, 0x184100, 0x7c4902, 0xfedb82, 0x00000e, 0x194130, 0x7d4931, 0xffdbb3, 0x00000f, 0x194530,  // 2080~2089
            0x3d4d31, 0x7fdfb3, 0x00000f, 0x014530, 0x3d4d31, 0x7fdfb3, 0x000007, 0x010530, 0x3d4d31, 0x7fdfb3,  // 2090~2099
            0x822240, 0x832678, 0xbf6f79, 0xfffffb, 0x82020f, 0x832678, 0xbb6f79, 0xffeffb, 0x82020f, 0x832678,  // 2100~2109
            0xbb6779, 0xff6f7b, 0x82020f, 0x822678, 0x9b6779, 0xff6f7b, 0x82020f, 0x822268, 0x9b6779, 0xff6f79,  // 2110~2119
            0x82020b, 0x822248, 0x936778, 0xbf6f79, 0x02020b, 0x822248, 0x832778, 0xbf6f79, 0x02020b, 0x822240,  // 2120~2129
            0x832678, 0xbf6f79, 0x00000b, 0x820240, 0x832678, 0xbf6f79, 0x00000b, 0x821200, 0x833678, 0xbb7779,  // 2130~2139
            0x00000b, 0x829280, 0x82b6f8, 0xbbf7f9, 0x00000b, 0x829280, 0x82b2f8, 0x9bf7f9, 0x00000b, 0x829280,  // 2140~2149
            0x82b2e8, 0x9bf7f9, 0x000009, 0x829280, 0x82b2cc, 0x93b7fc, 0x00000d, 0x429280, 0xc2b2cc, 0xc3b6fc,  // 2150~2159
            0x00000d, 0x429080, 0xc292c4, 0xc3b6fc, 0x00000d, 0x408080, 0xc292c4, 0xc3b6fc, 0x00000d, 0x448080,  // 2160~2169
            0xc69284, 0xc7b6fc, 0x00000d, 0x440800, 0xc69a84, 0xc6bafc, 0x00000d, 0x440800, 0xc69a84, 0xc6bafc,  // 2170~2179
            0x00000d, 0x640800, 0xe69a84, 0xe6bacc, 0x00000d, 0x640800, 0xe69a80, 0xe6bacc, 0x00000d, 0x6c4800,  // 2180~2189
            0xeeda82, 0xeeface, 0x00000e, 0x3c4900, 0x7cc982, 0xfedbc6, 0x00000e, 0x3c4100, 0x7cc982, 0xfedb86,  // 2190~2199
            0x012470, 0x396571, 0x7d6df3, 0xfffff7, 0x00247f, 0x396571, 0x7d6d73, 0xfffff7, 0x00207f, 0x396571,  // 2200~2209
            0x7d6d73, 0xfffff7, 0x00207f, 0x196571, 0x7d6d73, 0xfffff7, 0x00204f, 0x192571, 0x7d6d73, 0xfffff7,  // 2210~2219
            0x00204f, 0x112471, 0x7d6d71, 0xfffdf3, 0x00004f, 0x012470, 0x3d6d71, 0x7dedf3, 0x00004f, 0x012470,  // 2220~2229
            0x396571, 0x7d6df3, 0x000007, 0x012470, 0x396571, 0x7d6d73, 0x000007, 0x002070, 0x396571, 0x7d6d73,  // 2230~2239
            0x000007, 0x002070, 0x396571, 0x7d6d73, 0x000007, 0x002070, 0x392571, 0x7d6d73, 0x000007, 0x002040,  // 2240~2249
            0x192571, 0x7d6d73, 0x000007, 0x000240, 0x112671, 0x7d6f71, 0x000003, 0x021240, 0x033670, 0x3f7771,  // 2250~2259
            0x000003, 0x029240, 0x03b670, 0x3bf771, 0x000003, 0x829240, 0x82b278, 0xbbf779, 0x00000b, 0x829280,  // 2260~2269
            0x82b2f8, 0xbbf7f9, 0x00000b, 0x829280, 0x82b2f8, 0xbbb7f9, 0x00000b, 0x829280, 0x82b2f8, 0xbbb7f9,  // 2270~2279
            0x00000b, 0x829280, 0x8292c8, 0x9bb6f9, 0x00000b, 0x828080, 0x8292c8, 0x93b6f9, 0x000009, 0x800880,  // 2280~2289
            0x829acc, 0x83befc, 0x00000d, 0xc00880, 0xc29acc, 0xc3befc, 0x00000d, 0x440880, 0xc69a84, 0xc6bafc,  // 2290~2299
            0xfffffd
        };

        // 24节气基础日期
        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_BASE[] = {
            5, 20, 4, 18, 5, 20, 4, 19, 5, 20, 5, 21, 7, 22, 7, 22, 7, 22, 8, 23, 7, 22, 7, 21
        };

        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_OFFSET_TABLE_18XX[] = {
             0,  0, 77, 57,  0,  0, 48, 88,  0, 88, 44,  0, 96, 64,  0, 96, 36, 80,  0,  0,  0,  0, 84, 48
        };

        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_OFFSET_TABLE_19XX[] = {
            49,  0,  0, 93, 48, 56, 76,  0, 44,  0, 68,  0,  0, 84, 48,  0, 60,  0,  0,  0,  0,  0,  0, 88
        };

        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_OFFSET_TABLE_20XX[] = {
            85, 48, 17,  0, 84, 84,  0,  0, 68,  0, 88, 48, 20,  0, 68,  0, 84,  0, 44, 60, 60, 48, 24,  0
        };

        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_OFFSET_TABLE_21XX[] = {
             0, 89, 53,  0,  0,  0,  0, 40, 92,  0,  0, 72, 36,  0, 88, 40,  0,  0, 68, 88, 92, 80, 56,  0
        };

        template <class Dummy>
        const U8 Impl<Dummy>::_SOLAR_TERMS_OFFSET_TABLE_22XX[] = {
             0,  0, 89, 65,  0,  0,  0, 68,  0, 52,  0, 88, 56,  0,  0, 60,  0, 56, 96,  0,  0,  0, 92, 64
        };

        template <class Dummy>
        U8 Impl<Dummy>::Gregorian_GetSolarTerm(U16 y, U8 i) {
            const U32 v = _SOLAR_TERMS[y - _CALENDAR_YEAR_START];
            U8 s = _SOLAR_TERMS_BASE[i] + ((v >> i) & 1);
            U8 c = y / 100;
            U16 t = 0;
            if (c == 20) {
                t = _SOLAR_TERMS_OFFSET_TABLE_20XX[i];
            } else if (c == 19) {
                if ((_SOLAR_TERMS_BASE_OFFSET_19XX >> i) & 1) ++s;
                t = _SOLAR_TERMS_OFFSET_TABLE_19XX[i];
            } else if (c == 21) {
                if ((_SOLAR_TERMS_BASE_OFFSET_21XX >> i) & 1) ++s;
                t = _SOLAR_TERMS_OFFSET_TABLE_21XX[i];
            } else if (c >= 22) {
                if ((_SOLAR_TERMS_BASE_OFFSET_22XX >> i) & 1) ++s;
                t = _SOLAR_TERMS_OFFSET_TABLE_22XX[i];
            } else if (c <= 18) {
                if ((_SOLAR_TERMS_BASE_OFFSET_18XX >> i) & 1) ++s;
                t = _SOLAR_TERMS_OFFSET_TABLE_18XX[i];
            }

            if (t != 0 && y >= t + 100 * c) {
                --s;
            }

            return s;
        }

        template <class Dummy>
        U16 Impl<Dummy>::Gregorian_GetSolarTerms(U16 y, U8 m) {
            U8 i = ((m - 1) << 1);
            U16 s1 = Gregorian_GetSolarTerm(y, i), s2 = Gregorian_GetSolarTerm(y, i + 1);
            return (s1 << 8) | s2;
        }

        /**
         * 朔日信息编码
         * 编码结构：
         *           +----12-----------0+
         *           |DDDDD|MMMMMMMMMMMM|
         *           +-----+------------+
         *      首个朔日日期  其余每个朔日与前一个朔日的间隔
         * 一年最多有13个朔日，而首个朔日已经被日期标记了，所以用12位表示其余朔日已经够了
         *
         * 例如2017年数据为0x1c54a，即 0b'11100'0101'0100'1010
         *    首个朔日日期(11100)，即公历1月28日
         *    余下的朔日中，第2、4、7、9、11距前一朔日是30天，也就是大月，其余为小月
         *
         * 天文知识：
         * 月相变化的周期29.53天，称为朔望月。月球公转的周期为27.32天，称为恒星月。
         * 之所以朔望月比恒星月长，是因为月球绕地球公转了360°时，地球也在绕着太阳公转了一定的角度，
         * 月球要多转同样的角度，才能再次观察到同一月相。
         * 由于地球绕太阳公转的轨道是一个椭圆，根据开普勒第二定律（面积定律），
         * 同样的时间，地球在近日点和远日点公转经过的角度是不一样的，
         * 况且月球要多转的这点角度，也存在近地点和远地点的运行速度区别，
         * 所以并不是知道某个初一在哪天之后，简单地往后加29.53就是下一个朔日了。
         * 实事上，朔日的间隔在29.27~29.83之间波动，因此，连续大月、连续小月的情况都是有的。
         */
        template <class Dummy>
        const U32 Impl<Dummy>::_NEW_MOONS[] = {
            0x062ad,
            0x192ad, 0x0f5ac, 0x04ba9, 0x175a9, 0x0dd92, 0x01d15, 0x14516, 0x09a4e, 0x1c156, 0x102b6,  // 1800~1809
            0x055b5, 0x196d4, 0x0e6a9, 0x03e8a, 0x1568b, 0x0a527, 0x1d52b, 0x1115b, 0x072da, 0x1a36a,  // 1810~1819
            0x10754, 0x04749, 0x17345, 0x0ca93, 0x0152b, 0x1352b, 0x08a5b, 0x1b2ad, 0x1156a, 0x05b55,  // 1820~1829
            0x195a4, 0x0e549, 0x03a95, 0x15295, 0x0a52d, 0x1d536, 0x122b5, 0x075aa, 0x1a6ca, 0x0f6a5,  // 1830~1839
            0x05d4a, 0x1754a, 0x0ba97, 0x0152e, 0x14556, 0x08ab5, 0x1b355, 0x11752, 0x06ea5, 0x18aa5,  // 1840~1849
            0x0d64b, 0x02497, 0x1529b, 0x0955b, 0x1d56a, 0x12369, 0x08752, 0x1a552, 0x0f325, 0x04a4b,  // 1850~1859
            0x1724d, 0x0b2ad, 0x1e2b5, 0x135ad, 0x09da9, 0x1b5a9, 0x11592, 0x06d25, 0x19526, 0x0da4e,  // 1860~1869
            0x024ae, 0x152b6, 0x0a6b5, 0x1d6d4, 0x126a9, 0x08e92, 0x1a693, 0x0e527, 0x03a57, 0x1625b,  // 1870~1879
            0x0c2da, 0x1e36a, 0x14354, 0x09749, 0x1c349, 0x10293, 0x0552b, 0x1852b, 0x0d25b, 0x0255a,  // 1880~1889
            0x1556a, 0x0ab55, 0x1e5a4, 0x12549, 0x07a95, 0x1a295, 0x0f52e, 0x03aad, 0x162b5, 0x0c5aa,  // 1890~1899
            0x01da5, 0x146a5, 0x0ad4a, 0x1d64a, 0x11497, 0x06536, 0x19556, 0x0e2d5, 0x046b2, 0x16752,  // 1900~1909
            0x0bea5, 0x0164a, 0x1364b, 0x07497, 0x1a4ab, 0x0f55b, 0x05ad6, 0x17369, 0x0d352, 0x02b25,  // 1910~1919
            0x15525, 0x09a4b, 0x1c24d, 0x114ad, 0x0656d, 0x185b5, 0x0e5a9, 0x04d52, 0x17692, 0x0bd25,  // 1920~1929
            0x1e526, 0x13256, 0x084ae, 0x1a2b6, 0x0f6b5, 0x05da9, 0x186c9, 0x0d692, 0x02d26, 0x14527,  // 1930~1939
            0x09a57, 0x1b25b, 0x1155a, 0x06ad5, 0x19355, 0x0e749, 0x03693, 0x16293, 0x0b52b, 0x1d52d,  // 1940~1949
            0x1226d, 0x0855a, 0x1b5aa, 0x0f365, 0x05b4a, 0x1854a, 0x0da95, 0x0152b, 0x1452e, 0x09aad,  // 1950~1959
            0x1c2b5, 0x115aa, 0x06da5, 0x196a5, 0x0f54a, 0x03c95, 0x15497, 0x0b536, 0x1e55a, 0x122d5,  // 1960~1969
            0x086d2, 0x1b752, 0x106a5, 0x0464b, 0x1764b, 0x0c497, 0x01957, 0x1355b, 0x09ada, 0x1c369,  // 1970~1979
            0x12352, 0x06b25, 0x19525, 0x0e24b, 0x0349b, 0x154ad, 0x0a56d, 0x1d5b5, 0x135aa, 0x08d52,  // 1980~1989
            0x1b692, 0x10525, 0x05a4d, 0x17256, 0x0c4ae, 0x019ad, 0x146b5, 0x09daa, 0x1c6c9, 0x11693,  // 1990~1999
            0x07d26, 0x1852b, 0x0d257, 0x034b6, 0x1655a, 0x0aad5, 0x1d355, 0x13749, 0x08693, 0x1a293,  // 2000~2009
            0x0f52b, 0x04a5b, 0x172ad, 0x0c56a, 0x01b55, 0x143a5, 0x0ab4a, 0x1c54a, 0x11295, 0x0692b,  // 2010~2019
            0x1952e, 0x0d2ad, 0x0356a, 0x165b2, 0x0bda5, 0x1d6a5, 0x1364a, 0x08c96, 0x1a497, 0x0f156,  // 2020~2029
            0x04ab5, 0x172d6, 0x0d6d2, 0x01ea5, 0x146a5, 0x0964b, 0x1c24b, 0x10497, 0x05957, 0x1855b,  // 2030~2039
            0x0e35a, 0x036d4, 0x16352, 0x0bb25, 0x1e525, 0x1224b, 0x074ab, 0x1a4ad, 0x0f16d, 0x04b6a,  // 2040~2049
            0x175aa, 0x0d592, 0x02d25, 0x14525, 0x09a4d, 0x1c256, 0x114ae, 0x05aad, 0x186d5, 0x0e6aa,  // 2050~2059
            0x04e92, 0x15693, 0x0bd26, 0x1d52b, 0x12257, 0x074b6, 0x1a55a, 0x0f2d5, 0x056ca, 0x1774a,  // 2060~2069
            0x0c695, 0x0152b, 0x1452b, 0x08a5b, 0x1b2ad, 0x1156a, 0x06b55, 0x183a5, 0x0e34a, 0x03a95,  // 2070~2079
            0x16495, 0x0a92d, 0x1d14e, 0x122ad, 0x0856a, 0x1a5b2, 0x0f5a5, 0x05d4a, 0x1864a, 0x0cd16,  // 2080~2089
            0x0192e, 0x14156, 0x09ab6, 0x1b2d6, 0x116d4, 0x06ea5, 0x19725, 0x0d68b, 0x02517, 0x1549b,  // 2090~2099
            0x0a957, 0x1d15b, 0x1335a, 0x09754, 0x1c352, 0x10325, 0x05a4b, 0x1824b, 0x0d4ab, 0x0195b,  // 2100~2109
            0x1416d, 0x0ad6a, 0x1d5aa, 0x12592, 0x07d25, 0x1a526, 0x0f255, 0x034ad, 0x164b6, 0x0baad,  // 2110~2119
            0x015aa, 0x136aa, 0x08e95, 0x1b693, 0x1152a, 0x05a56, 0x17257, 0x0d536, 0x02ab5, 0x142d5,  // 2120~2129
            0x0a6ca, 0x1d74a, 0x12695, 0x0662b, 0x1952b, 0x0e29b, 0x0455a, 0x1656a, 0x0bb55, 0x0174a,  // 2130~2139
            0x1434a, 0x08a95, 0x1b515, 0x1022d, 0x0529d, 0x172ad, 0x0d5ac, 0x02b69, 0x155a5, 0x0ad4a,  // 2140~2149
            0x1d68a, 0x12516, 0x07a2e, 0x19156, 0x0e2b6, 0x035b5, 0x176d4, 0x0bea9, 0x01e8a, 0x1368b,  // 2150~2159
            0x08517, 0x1a52b, 0x0f15b, 0x052d6, 0x1835a, 0x0d354, 0x026a9, 0x15345, 0x0aa8b, 0x1c295,  // 2160~2169
            0x1152b, 0x06a5b, 0x192ad, 0x0e56a, 0x03b55, 0x175a4, 0x0cd45, 0x1e546, 0x13295, 0x0852d,  // 2170~2179
            0x1b536, 0x0f2b5, 0x055aa, 0x186ca, 0x0d6a5, 0x02d4a, 0x1554a, 0x0aa96, 0x1c297, 0x11556,  // 2180~2189
            0x06ab5, 0x192d5, 0x0f6ca, 0x03ea5, 0x162a5, 0x0b64b, 0x1e24b, 0x1229b, 0x0755b, 0x1b56a,  // 2190~2199
            0x10369, 0x06752, 0x19352, 0x0e325, 0x03a4b, 0x1524d, 0x0a2ad, 0x1d2ad, 0x125ad, 0x07da9,  // 2200~2209
            0x1a5a9, 0x10592, 0x05d25, 0x17526, 0x0ca4e, 0x014ae, 0x142b6, 0x085b5, 0x1c6d4, 0x116a9,  // 2210~2219
            0x07e92, 0x18693, 0x0d527, 0x02a57, 0x1525b, 0x0a2da, 0x1d36a, 0x13354, 0x08749, 0x1a349,  // 2220~2229
            0x0f293, 0x0452b, 0x1752b, 0x0ba5b, 0x0155a, 0x1456a, 0x09b55, 0x1c5a4, 0x11549, 0x06a95,  // 2230~2239
            0x19295, 0x0d52d, 0x02a6d, 0x152b5, 0x0b5aa, 0x1d6d2, 0x126a5, 0x08d4a, 0x1b64a, 0x0e497,  // 2240~2249
            0x0452e, 0x17556, 0x0cab5, 0x016aa, 0x14752, 0x09ea5, 0x1c325, 0x1064b, 0x05497, 0x1849b,  // 2250~2259
            0x0d55b, 0x02ad5, 0x15369, 0x0bb52, 0x1e552, 0x12325, 0x07a4b, 0x1a24d, 0x0f4ad, 0x0356b,  // 2260~2269
            0x165ad, 0x0c5a9, 0x02b52, 0x14592, 0x09d25, 0x1c526, 0x1124e, 0x054ae, 0x182b6, 0x0d6b5,  // 2270~2279
            0x03da9, 0x156a9, 0x0be92, 0x1d693, 0x12527, 0x06a57, 0x1925b, 0x0f4da, 0x04ad5, 0x16355,  // 2280~2289
            0x0c749, 0x01693, 0x14293, 0x0852b, 0x1b52d, 0x1025d, 0x0655a, 0x1856a, 0x0d365, 0x03b4a,  // 2290~2299
            0x1654a
        };

        template <class Dummy>
        U8 Impl<Dummy>::_GetNewMoons(U16 y, U32 (&new_moons)[26], U8 start_idx) {
            U32 sum, nm1;
            U8 i0, i;

            const U32 w = _DayOffset(y, 12, Gregorian_GetSolarTerm(y, 23));  // 当年的冬至

            // 当年朔日信息
            const U32 bit = _NEW_MOONS[y - _CALENDAR_YEAR_START];

            // 首个朔日
            nm1 = (bit >> 12) & 0xfff;
            sum = _DayOffset(y, 1, nm1);
            new_moons[start_idx] = sum;

            // 其余的朔日，并记录冬至之前最近的朔日
            i0 = 12;
            i = 0;
            while (i < 12) {
                sum += (bit & (1 << i)) ? 30 : 29;
                if (sum > w) {
                    i0 = i;
                }
                new_moons[++i + start_idx] = sum;
            }

            // 越界检查
            if (sum + nm1 - new_moons[start_idx] > (Gregorian_IsLeapYear(y) ? 366u : 365u)) {
                new_moons[start_idx + i] = 0;
            }

            return i0 + start_idx;
        }

        template <class Dummy>
        ChineseDate Impl<Dummy>::Gregorian2Chinese(const GregorianDate &date) {
            U16 y = date.year;
            U8 m = date.month;
            U8 d = date.day;
            ChineseDate res{};
            if (y < _CALENDAR_YEAR_START || y > CALENDAR_MAX_YEAR || m < 1 || m > 12 || d < 1 || d > 31) {
                return res;
            }

            U32 mid_terms[11]{}, new_moons[26]{};
            U32 o;
            U16 y1;
            U8 w, w1, leap, i, k;

            o = _DayOffset(y, m, d);

            // 获取当年朔日以及冬至所在月（即农历十一月）下标
            // 十一月朔日的平均日期在大雪节气（12月6~8号），
            // 随机输入一天，在大雪之前的概率为(365-31+7-1)/365*100%=(93_11/73)%
            // 所以这里从13放起，把前面的空间预留给上一年
            w = _GetNewMoons(y, new_moons, 13);

            // 待计算日期在冬至所在月之前
            if (new_moons[w] > o) {
                // 计算上一年信息
                y1 = y;
                --y;
                w1 = w;
                w = _GetNewMoons(y, new_moons, 0);

                if (new_moons[12] == 0) {
                    // 上一年只有12个月，移一位
                    for (i = 0; i < 13; ++i) {
                        new_moons[12 + i] = new_moons[13 + i];
                    }
                    --w1;
                }
            }
            // 待计算日期在冬至所在月或之后
            else {
                // 先把朔日移到开头
                for (i = 0; i < 13; ++i) {
                    new_moons[i] = new_moons[13 + i];
                }
                w -= 13;

                // 再计算下一年信息
                y1 = y + 1;
                w1 = _GetNewMoons(y1, new_moons, new_moons[12] ? 13 : 12);
            }

            // 两个冬至之间有13个朔日，需要置闰
            // 如果只有12个朔日，则无须理会中气，一概不置闰，
            // 例如：1984年的冬至~1985年的冬至之间只有12个朔日，
            // 尽管2月20号~3月20号这个月不包含中气，也不将其设置为闰月
            leap = 0;
            if (w1 - w > 12) {
                // 中气
                for (i = 0; i < 11; ++i) {
                    mid_terms[i] = _DayOffset(y1, i + 1, Gregorian_GetSolarTerm(y1, i * 2 + 1));
                }

                // 查找首个无中气月份
                for (i = 0, k = w + 2; i < 11; ++i, ++k) {
                    if (new_moons[k] <= mid_terms[i]) {
                        break;
                    }
                }

                leap = k;
            }

            // 判断日期在哪个月内
            for (i = 0, k = w; i < 13; ++i, ++k) {
                if (new_moons[k] > o) {
                    break;
                }
            }

            m = k - w;
            if (leap != 0 && k >= leap) {
                --m;
            }

            // 修正月份序号
            if (m > 2) {
                m -= 2;
                y = y1;
            }
            else {
                m += 10;
            }

            res.year = y;
            res.month = m;
            res.day = o - new_moons[k - 1] + 1;
            res.major = new_moons[k] - new_moons[k - 1] == 30;
            res.leap = leap != 0 && leap == k;

            return res;
        }

        template <class Dummy>
        U8 Impl<Dummy>::Gregorian_GetStemBranch(U16 y, U8 m, U8 d) {
            /**
             * 为方便处理2月的问题，把1月、2月当作前一年的13月14月
             * 即把3月1日当作起点计算，这样一来，闰年的2月29号就在年末，大大方便计算
             * days为已过整月累积天数，d-30m'为大月累积多出来的天数
             *  month   3   4   5   6   7   8   9  10  11  12    1   2
             *  m'=m-3  0   1   2   3   4   5   6   7   8   9   10  11
             *   days   0  31  61  92  122 153 184 214 245 275 306 337
             *  d-30m'  0   1   1   2   2   3   4   4   5   5    6   7
             * 观察到m'在5和10处规律骤变，得到规律：f(x)=⌊(x+1+⌊x/5⌋)/2⌋
             * 进一步优化：
             * 考虑到一些语言没有整除功能，加之2的幂可以用位运算，尝试用位运算优化
             * 如果不考虑向下取整，可化简f(x)=(x+1+x/5)/2=(6x+5)/10
             * 由于6/10=0.6，最接近的2的幂为分母的分数为19/32=0.59375，
             * （这里当然也可以用39/64=0.609375，77/128=0.6015625，153/256=0.59765625，
             * 甚至更大的分母，但试验证明19/32已经能满足要求了；
             * 更小的分母如5/8=0.625试过了，不行）
             * 设：f(x)=⌊(19x+b)/32⌋
             * 考虑骤然变化的f(4)=2、f(6)=4、f(9)=5、f(11)=7：
             * 因为⌊(19*4+b)/32⌋=2，所以(76+b)<96，即b<20；
             * 因为⌊(19*6+b)/32⌋=4，所以(114+b)≥128，即b≥14；
             * 因为⌊(19*9+b)/32⌋=5，所以(171+b)<192，即b<21；
             * 因为⌊(19*11+b)/32⌋=7，所以(209+b)≥224，即b≥15；
             * 综合上述结果，15≤b<20，这里取19，以便与前面的乘法合并，得到最终优化公式：f(x)=19(x+1) shr 5
             * 经代码验证，上述f(x)的表达式符合要求
             */

            /**
             * 以甲子为0，假定公元0年3月0日的干支为甲子
             * （当然假定不一定正确，最后再调整就是了，也不考虑1582年改历导致少10天的问题）
             * 一年365天，365≡5(mod 60)，每年就会前进5个，即：公元y年3月0日的干支为y*5
             * 公历闰年校正：4年一闰，总共多⌊y/4⌋天；百年不闰，少⌊y/100⌋天；400年又闰，多⌊y/400⌋天
             * 所以闰年累计天数为：⌊y/4⌋-⌊y/100⌋+⌊y/400⌋
             * 根据上面的大月累积天数公式，得出m月d日距离当年3月0日的天数为：30(m-3)+((19(m-2))>>5)+d
             * 考虑到(2N)*30|60，(2N+1)*30≡30(mod 60)，30(m-3)可优化为30((m-1) and 1)
             * 最后，通过已知1949年10月1日的干支为甲子，进行验证：
             * (1949*5+⌊1949/4⌋-⌊1949/100⌋+⌊1949/400⌋+30(9 and 1)+((19*8 shr 5)+1)=(9745+487-19+4+30+4+1) mod 60=52
             * 计算结果为52，说明假定公元0年3月1日的干支为甲子有偏差，+8使之符合事实
             * 综合起来，最终的公式为：(5y+⌊y/4⌋-⌊y/100⌋+⌊y/400⌋+30((m-1) and 1)+(19(m-2) shr 5)+d+8) mod 60
             */

            if (m == 1 || m == 2) {
                m += 12;
                --y;
            }
            return (y * 5 + (y >> 2) - y / 100 + y / 400 + ((m - 1) & 1) * 30 + (((m - 2) * 19) >> 5) + d + 8) % 60;
        }

        typedef Impl<int> MyImpl;
    }

    static inline ChineseDate Gregorian2Chinese(const GregorianDate &date) {
        return detail::MyImpl::Gregorian2Chinese(date);
    }

    static inline ChineseDate Gregorian2Chinese(U16 y, U8 m, U8 d) {
        return detail::MyImpl::Gregorian2Chinese(GregorianDate{y, m, d});
    }

    // 适配公历，小寒为0
    static inline U8 Gregorian_GetSolarTerm(U16 y, U8 i) {
        return detail::MyImpl::Gregorian_GetSolarTerm(y, i);
    }

    // 高8位为第一个节气，低8位为第二个节气
    static inline U16 Gregorian_GetSolarTerms(U16 y, U8 m) {
        return detail::MyImpl::Gregorian_GetSolarTerms(y, m);
    }

    static inline U8 Gregorian_GetStemBranch(U16 y, U8 m, U8 d) {
        return detail::MyImpl::Gregorian_GetStemBranch(y, m, d);
    }

}

#endif
