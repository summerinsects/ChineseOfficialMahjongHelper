
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

#define CALENDAR_YEAR_START 1839

    /**
     * 农历数据
     * 编码结构：
     *           +-+----17---13------------0+
     *           |O|OOOOO|LLLL|MMMMMMMMMMMMM|
     *           +-+-----+----+-------------+
     *            春节偏移  闰月    每月大小
     *
     * 例如2017年数据为0x38dd4a，即 0b0'011100'0110'1110101001010
     *    这年闰六月(0110)，闰六月的信息在第7位上；七月至十二月的信息依次后移了1位，分布在第8~13位上
     *    具体信息：二、四、闰六、八、十、十一、十二月为大月
     *    春节偏移(0|11100)，在公历1月28日
     *
     * 再如2018年数据为0x600d4a，即 0b0'110000'0000'0110101001010
     *    这年无闰月(0000)，正月至十二月的信息分布在第1~12位上；第13位数据为0，不作处理
     *    具体信息：二、四、七、九、十一、十二月为大月
     *    春节偏移(1|10000)，在公历2月16日
     */

    static const uint32_t ChineseCalendarData[] = {
        0x5c0752,
        0x460ea5, 0x2e754a, 0x54054b, 0x3cea97, 0x640aab, 0x4e055a, 0x36ab55, 0x5e0ba9, 0x4a0752, 0x309aa5,  // 1840~1849
        0x580b25, 0x431a4b, 0x68094d, 0x500aad, 0x3af56a, 0x6205b4, 0x4c0ba9, 0x34bd52, 0x5c0d92, 0x460d25,  // 1850~1859
        0x2e7a4d, 0x540956, 0x3d12b5, 0x640ad6, 0x5006d4, 0x36ada9, 0x5e0ec9, 0x4a0e92, 0x328d26, 0x560527,  // 1860~1869
        0x3f4a57, 0x66095b, 0x520b5a, 0x3ad6d4, 0x620754, 0x4c0749, 0x34b693, 0x5a0a93, 0x44052b, 0x2c6a5b,  // 1870~1879
        0x54096d, 0x3ceb6a, 0x640daa, 0x500ba4, 0x38bb49, 0x5e0d49, 0x480a95, 0x30952b, 0x58052d, 0x3e0aad,  // 1880~1889
        0x2a556a, 0x520daa, 0x3cdda4, 0x620ea4, 0x4c0d4a, 0x34aa95, 0x5a0a97, 0x440556, 0x2c6ab5, 0x540ad5,  // 1890~1899
        0x3f16d2, 0x660752, 0x500ea5, 0x3ab64a, 0x60064b, 0x480a9b, 0x329556, 0x5a056a, 0x440b59, 0x2c5752,  // 1900~1909
        0x540752, 0x3cdb25, 0x640b25, 0x4c0a4b, 0x34b4ab, 0x5c02ad, 0x46056b, 0x2e4b69, 0x560da9, 0x42fd92,  // 1910~1919
        0x680e92, 0x500d25, 0x38ba4d, 0x600a56, 0x4a02b6, 0x3095b5, 0x5a06d4, 0x440ea9, 0x2e5e92, 0x540e92,  // 1920~1929
        0x3ccd26, 0x62052b, 0x4c0a57, 0x34b2b6, 0x5c0b5a, 0x4806d4, 0x306ec9, 0x560749, 0x3ef693, 0x660a93,  // 1930~1939
        0x50052b, 0x36ca5b, 0x5e0aad, 0x4a056a, 0x329b55, 0x5a0ba4, 0x440b49, 0x2c5a93, 0x540a95, 0x3af52d,  // 1940~1949
        0x620536, 0x4c0aad, 0x36b5aa, 0x5c05b2, 0x460da5, 0x307d4a, 0x580d4a, 0x3f0a95, 0x640a97, 0x500556,  // 1950~1959
        0x38cab5, 0x5e0ad5, 0x4a06d2, 0x328ea5, 0x5a0ea5, 0x44064a, 0x2a6c97, 0x520a9b, 0x3cf55a, 0x62056a,  // 1960~1969
        0x4c0b69, 0x36b752, 0x5e0b52, 0x460b25, 0x2e964b, 0x560a4b, 0x3f14ab, 0x6402ad, 0x4e056d, 0x38cb69,  // 1970~1979
        0x600da9, 0x4a0d92, 0x329d25, 0x5a0d25, 0x455a4d, 0x680a56, 0x5202b6, 0x3ac5b5, 0x6206d5, 0x4c0ea9,  // 1980~1989
        0x36be92, 0x5e0e92, 0x480d26, 0x2e6a56, 0x540a57, 0x3f14d6, 0x66035a, 0x4e06d5, 0x38b6c9, 0x600749,  // 1990~1999
        0x4a0693, 0x30952b, 0x58052b, 0x420a5b, 0x2c555a, 0x52056a, 0x3afb55, 0x640ba4, 0x4e0b49, 0x34ba93,  // 2000~2009
        0x5c0a95, 0x46052d, 0x2e8aad, 0x540ab5, 0x3f35aa, 0x6605d2, 0x500da5, 0x38dd4a, 0x600d4a, 0x4a0c95,  // 2010~2019
        0x32952e, 0x580556, 0x420ab5, 0x2c55b2, 0x5406d2, 0x3acea5, 0x620725, 0x4c064b, 0x34ac97, 0x5a0cab,  // 2020~2029
        0x46055a, 0x2e6ad6, 0x560b69, 0x3f7752, 0x660b52, 0x500b25, 0x38da4b, 0x5e0a4b, 0x4804ab, 0x30a55b,  // 2030~2039
        0x5805ad, 0x420b6a, 0x2c5b52, 0x540d92, 0x3cfd25, 0x620d25, 0x4c0a55, 0x34b4ad, 0x5c04b6, 0x4405b5,  // 2040~2049
        0x2e6daa, 0x560ec9, 0x431e92, 0x660e92, 0x500d26, 0x38ca56, 0x5e0a57, 0x480556, 0x3086d5, 0x580755,  // 2050~2059
        0x440749, 0x2a6e93, 0x520693, 0x3af52b, 0x62052b, 0x4a0a5b, 0x34b55a, 0x5c056a, 0x460b65, 0x2e974a,  // 2060~2069
        0x560b4a, 0x3f1a95, 0x660a95, 0x4e052d, 0x36caad, 0x5e0ab5, 0x4a05aa, 0x308ba5, 0x580da5, 0x440d4a,  // 2070~2079
        0x2c7c95, 0x520c96, 0x3af94e, 0x620556, 0x4c0ab5, 0x34b5b2, 0x5c06d2, 0x460ea5, 0x308e4a, 0x54068b,  // 2080~2089
        0x3d0c97, 0x6404ab, 0x4e055b, 0x36cad6, 0x5e0b6a, 0x4a0752, 0x329725, 0x580b45, 0x420a8b, 0x2a549b,  // 2090~2099
        0x5204ab
    };

    static FORCE_INLINE uint8_t Chinese_GetLeapMonth(uint32_t v) {
        return static_cast<uint8_t>((v >> 13) & 0xf);
    }

    static FORCE_INLINE bool Chinese_IsMajorMonth(uint32_t v, uint8_t m) {
        return v & (1 << m);
    }

    static FORCE_INLINE void SpringFestivalInGregorian(uint32_t v, uint8_t &m, uint8_t &d) {
        m = ((v >> 22) & 1) + 1;
        d = (v >> 17) & 0x1f;
    }

    static ChineseDate Gregorian2Chinese(const GregorianDate &date) {
        int year = date.year;
        int month = date.month;
        int day = date.day;
        if (year < 1840 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31) {
            return ChineseDate{};
        }

        bool ly = Gregorian_IsLeapYear(year);
        uint32_t v = ChineseCalendarData[year - CALENDAR_YEAR_START];  // 当年的信息

        uint8_t sfm, sfd;
        SpringFestivalInGregorian(v, sfm, sfd);  // 春节所在的公历日期

        uint8_t rem, mx = 1, lm = 0;
        bool mm;
        if (month > sfm) {  // 春节所在的公历月份已过
            if (sfm == 2) {
                // 春节在公历2月（概率较大），公历2月剩余天数
                rem = (ly ? 29 : 28) - sfd;
            }
            else {
                // 春节在公历1月（概率较小），公历1月剩余天数
                rem = 31 - sfd;

                // 如果公历2月已经过完，累加
                if (month > 2) {
                    rem += (ly ? 29 : 28);
                }
            }

            // 农历先按每月30天算，每经过一个公历的大月，农历就要增加1日
            // 公历3~7月逢奇数是大月
            uint8_t i = 3;
            while (i < month && i < 8) {
                ++mx;
                if ((i++) & 1) {
                    ++rem;
                }
            }

            // 公历8~12月逢偶数是大月
            while (i < month && i < 13) {
                ++mx;
                if ((++i) & 1) {
                    ++rem;
                }
            }

            rem += day;  // 当前月剩余天数

            // 全部剩余天数凑农历整月
            while (rem >= 30) {
                ++mx;
                rem -= 30;
            }

            // 每经过一个农历小月，增加1日
            for (i = 1; i < mx; ++i) {
                mm = Chinese_IsMajorMonth(v, i - 1);
                if (!mm) {
                    ++rem;
                    if (rem >= 30) {
                        ++mx;
                        rem -= 30;
                    }
                }
            }

            mm = Chinese_IsMajorMonth(v, mx - 1);

            // 当前月是农历小月，并且仍然剩余29天，进位月份
            // 农历历法规定冬至必须在农历十一月里，
            // 即便是冬至（非常早）在公历12月21号，农历（非常晚）十一月三十，公历12月31号最多也只是农历十二月十几
            // 不会出现农历从十二月进位到次年正月的情况
            if (!mm && rem == 29) {
                mm = Chinese_IsMajorMonth(v, mx);
                ++mx;
                rem = 0;
            }

            lm = Chinese_GetLeapMonth(v);
        }
        else if (month == sfm) {
            // 与春节同月份
            if (day >= sfd) {
                // 无论春节在1月还是2月，当前月不可能再次进入新的农历月
                // 1月的春节最早也要在大寒附近，此时已经没有足够的天数过完整个农历正月，不会产生进位
                // 2月的春节即便在1日，逢公历闰年的29日也只能到农历的正月廿九，不能进入二月初一
                rem = day - sfd;
                mm = Chinese_IsMajorMonth(v, 0);
                lm = Chinese_GetLeapMonth(v);
            }
            else {
                // 春节之前，农历倒退一年
                --year;
                rem = sfd - day;
            }
        }
        else {
            // 春节所在的月份未到，农历倒退一年，这种情况仅当春节在公历2月时出现
            --year;
            rem = sfd + (31 - day);
        }

        // 倒退的情况
        if (year != date.year) {
            v = ChineseCalendarData[year - CALENDAR_YEAR_START];  // 上一年的信息

            // 根据是否有闰月，从第12或第13个农历月起倒退
            lm = Chinese_GetLeapMonth(v);
            mx = (lm == 0) ? 12 : 13;
            do {
                mm = Chinese_IsMajorMonth(v, mx - 1);
                if (mm) {
                    // 大月可退30天
                    if (rem <= 30) {
                        rem = 30 - rem;
                        break;
                    }
                    // 倒退整个农历月
                    rem -= 30;
                }
                else {
                    // 小月可退29天
                    if (rem <= 29) {
                        rem = 29 - rem;
                        break;
                    }
                    // 倒退整个农历月
                    rem -= 29;
                }
                --mx;
            } while (1);
        }

        ChineseDate res{};
        res.year = year;
        res.month = mx;
        res.day = rem + 1;
        res.major = mm;
        if (lm != 0 && mx > lm) {  // 闰月序号校正
            --res.month;
            if (mx == lm + 1) {
                res.leap = true;
            }
        }
        return res;
    }

    void ChineseDate_NextMonth(ChineseDate &date) {
        uint32_t v = ChineseCalendarData[date.year - CALENDAR_YEAR_START];
        uint8_t lm = Chinese_GetLeapMonth(v);
        uint8_t mx = date.month, num = 12;
        if (lm != 0) {
            num = 13;
            if (mx > lm || (mx == lm && date.leap)) {
                ++mx;
            }
        }

        ++mx;
        if (mx <= num) {
            date.month = mx;
            date.leap = false;
            if (lm != 0 && mx > lm) {  // 闰月序号校正
                --date.month;
                if (mx == lm + 1) {
                    date.leap = true;
                }
            }
            date.major = Chinese_IsMajorMonth(v, mx - 1);
        }
        else {
            // 下一年
            ++date.year;
            date.month = 1;
            v = ChineseCalendarData[date.year - CALENDAR_YEAR_START];
            date.leap = false;
            date.major = Chinese_IsMajorMonth(v, 0);
        }
        date.day = 1;
    }

    /**
     * 24节气编码
     * 由于24节气在几十年范围均在2日内波动，每1位表示一个节气，再配合校正参数得到具体日期
     * 以4年为一个周期，可发现大致如下规律：
     * 对于3~12月的节气：
     *   开始时都为n号(持续约36年一致)
     *   闰年首先退到n-1(持续约9个周期36年)
     *   闰年+1也退到n-1(持续约9个周期36年)
     *   闰年+2也退到n-1(持续约9个周期36年)
     *   闰年+3也退到n-1，至此统一为n-1(持续约36年一致，直到再遇闰年首先退一天)
     * 对于1~2月的节气：
     *   闰年+1退
     *   闰年+2退
     *   闰年+3退
     *   闰年退
     * 维持以上规律直到遇整百年不闰，会向后延一天
     *
     * 例如：2018年数据为0xc3b6ec，即0b0'1100'001110'110110'111011'00
     *   第1位为小寒值0，具体日期为：(基数5)+0+(1900年起偏移1)-(小寒第一次总体偏移1931年起1)=5日
     *   第3位为立春值1，具体日期为：(基数4)+1+(1900年起偏移1)-(立春第一次总体偏移1859年起1)-(立春第二次总体偏移1999年起1)=4日
     */
    static const uint32_t SolarTermsData[] = {
        0xfffffb,
        0x80244f, 0x816578, 0xbd6d79, 0xfffff9, 0x80244b, 0x816578, 0xbd6d78, 0xfffff9, 0x80200b, 0x816468,  // 1840~1849
        0x9d6d78, 0xfffff9, 0x00200b, 0x816468, 0x996d78, 0xbdff79, 0x00200b, 0x812440, 0x996d78, 0xbdfd7d,  // 1850~1859
        0x00200f, 0x812444, 0x996d7c, 0xbdfd7d, 0x00200f, 0x812444, 0x896d7c, 0xbdfd7d, 0x00000f, 0x802444,  // 1860~1869
        0x8365fc, 0xbf6dfd, 0x02028f, 0x8226c4, 0xc367fc, 0xff6ffd, 0x42028d, 0xc222c0, 0xc367fc, 0xff6ffd,  // 1870~1879
        0x42028d, 0xc22280, 0xc366ec, 0xdf6ffc, 0x42028d, 0x422280, 0xc366ec, 0xdb6ffc, 0x00000d, 0x422280,  // 1880~1889
        0xc326c4, 0xdb6ffc, 0x00000d, 0x422280, 0xc326c4, 0xdb6ffc, 0x00000d, 0x420280, 0xc326c4, 0xcb6ffc,  // 1890~1899
        0x3c0804, 0x7e9a82, 0xfebec6, 0xfffffe, 0x3c080f, 0x7e9a82, 0xfebac6, 0xfffffe, 0x3c080f, 0x7e9a82,  // 1900~1909
        0xfebac6, 0xfffffe, 0x3c080f, 0x7e9a80, 0xfeba82, 0xffbeee, 0x18080e, 0x3c9880, 0x7eba82, 0xffbeee,  // 1910~1919
        0x18080e, 0x3c9800, 0x7eba82, 0xffbec6, 0x18080e, 0x3c9800, 0x7e9a82, 0xfebec6, 0x00010e, 0x3c0900,  // 1920~1929
        0x7edb92, 0xfeffd7, 0x00411f, 0x3c4911, 0x7edb93, 0xfefbd7, 0x00411f, 0x3c4911, 0x7edbb3, 0xfefbf7,  // 1930~1939
        0x00413f, 0x3c4931, 0x7fdbb3, 0xfffbf7, 0x01403f, 0x194931, 0x7ddbb1, 0xfffbb3, 0x01002f, 0x194930,  // 1940~1949
        0x7ddd31, 0xffffb3, 0x01042f, 0x194d30, 0x3ddd31, 0x7fdfb3, 0x01040f, 0x194d30, 0x3d4d31, 0x7fdfb3,  // 1950~1959
        0x000447, 0x014570, 0x3d4d71, 0x7fdff3, 0x000447, 0x014570, 0x3d4d71, 0x7fdff3, 0x002047, 0x016570,  // 1960~1969
        0xbd6d71, 0xfffff3, 0x802047, 0x816470, 0xbd6d71, 0xfdfffb, 0x80200f, 0x812478, 0xb96d79, 0xfdfdfb,  // 1970~1979
        0x80200f, 0x812478, 0x996d79, 0xfdfd79, 0x80000b, 0x812468, 0x996d78, 0xbded79, 0x00000b, 0x802448,  // 1980~1989
        0x9b6578, 0xbf6d79, 0x02000b, 0x822440, 0x836778, 0xbf6f79, 0x02020b, 0x822240, 0x8367f8, 0xbf6ffd,  // 1990~1999
        0x02028f, 0x8222c4, 0x8377fc, 0xbf7ffd, 0x42928f, 0xc2b2c4, 0xc3b6fc, 0xfffffd, 0x40928f, 0xc2b284,  // 2000~2009
        0xc3b6fc, 0xfbfffd, 0x40908f, 0xc29284, 0xc3b6fc, 0xdbfffd, 0x40900d, 0xc29280, 0xc3b6ec, 0xdbf7fc,  // 2010~2019
        0x40000d, 0x429280, 0xc2b6cc, 0xdbf7fc, 0x00000d, 0x429280, 0xc6b2c4, 0xc7f7fc, 0x04000d, 0x469280,  // 2020~2029
        0xe6b2c4, 0xe7f7fe, 0x24080f, 0x669a82, 0xe6bac6, 0xe7befe, 0x24080f, 0x669a82, 0xe6bac6, 0xe7befe,  // 2030~2039
        0x24080f, 0x649882, 0xfeba86, 0xffbefe, 0x38080f, 0x7c9802, 0xfe9a86, 0xffbefe, 0x18000f, 0x7c0800,  // 2040~2049
        0xfedb82, 0xfeffce, 0x18410e, 0x7c4900, 0xfedb82, 0xfefbce, 0x18410e, 0x3c4900, 0x7edb82, 0xfefbce,  // 2050~2059
        0x00410e, 0x3c4900, 0x7edb82, 0xfefbc6, 0x00410e, 0x3c4900, 0x7fdbb2, 0xfffbf7, 0x01003f, 0x3d4931,  // 2060~2069
        0x7dddb3, 0xfffff7, 0x01043f, 0x394d31, 0x7dddb3, 0xffdfb7, 0x01043f, 0x394d31, 0x7dcd33, 0xffdfb7,  // 2070~2079
        0x01043f, 0x194531, 0x7d4d33, 0xffdfb3, 0x00040f, 0x194530, 0x7d4d31, 0xffdfb3, 0x00000f, 0x194530,  // 2080~2089
        0x3d4d31, 0x7fdfb3, 0x00000f, 0x014530, 0x3d4d31, 0x7fdfb3, 0x000007, 0x010530, 0x3d4d31, 0x7fdfb3,  // 2090~2099
        0xfffff7
    };

    // 校正参数
    // 日期基数
    static const uint8_t SolarTermsBaseDay[] = {
        5, 20,
        4, 18, 5, 20, 4, 20,
        5, 21, 5, 21, 7, 22,
        7, 23, 7, 23, 8, 23,
        7, 22, 7, 21
    };

    // 1900年起偏移，每位表示一个节气，值为1时日期需要+1
    static const uint32_t SolarTermsOffset = 0xc367fd;

    // 总体第一次偏移，年份大于或等于相应值时日期需要-1
    static const int16_t SolarTermsOffsetY0[] = {
        1931, 2031,
        1859, 1975, 1930, 1938, 1960, 1870,
        1928, 1872, 1950, 2032, 2002, 1968,
        1930, 2004, 1942, 1870, 2026, 2042,
        2042, 2030, 1874, 1970
    };

    // 总体第二次偏移，年份大于或等于相应值时日期需要-1
    static const int16_t SolarTermsOffsetY1[] = {
        2067, 0x00,
        1999, 0x00, 2066, 2066, 0x00, 1998,
        2050, 1994, 2070, 0x00, 0x00, 0x00,
        2050, 0x00, 2066, 1990, 0x00, 0x00,
        0x00, 0x00, 2004, 0x00
    };

    static uint8_t GetSolarTerm(int y, uint8_t i) {
        const uint32_t v = SolarTermsData[y - CALENDAR_YEAR_START];
        uint8_t st = SolarTermsBaseDay[i] + ((v >> i) & 1);
        if (y >= 1900) st += ((SolarTermsOffset >> i) & 1);
        if (y >= SolarTermsOffsetY0[i]) --st;
        if (SolarTermsOffsetY1[i] != 0 && y >= SolarTermsOffsetY1[i]) --st;
        return st;
    }

    static std::pair<int, int> GetSolarTermsInGregorianMonth(int y, int m) {
        uint8_t idx = ((m - 1) << 1);
        return std::make_pair(GetSolarTerm(y, idx), GetSolarTerm(y, idx + 1));
    }

    static const char *SolarTermsText[] = {
        __UTF8("小寒"), __UTF8("大寒"),
        __UTF8("立春"), __UTF8("雨水"), __UTF8("惊蛰"), __UTF8("春分"), __UTF8("清明"), __UTF8("谷雨"),
        __UTF8("立夏"), __UTF8("小满"), __UTF8("芒种"), __UTF8("夏至"), __UTF8("小暑"), __UTF8("大暑"),
        __UTF8("立秋"), __UTF8("处暑"), __UTF8("白露"), __UTF8("秋分"), __UTF8("寒露"), __UTF8("霜降"),
        __UTF8("立冬"), __UTF8("小雪"), __UTF8("大雪"), __UTF8("冬至")
    };

    static const char *CelestialStem[10] = {
        __UTF8("甲"), __UTF8("乙"), __UTF8("丙"), __UTF8("丁"), __UTF8("戊"), __UTF8("己"), __UTF8("庚"), __UTF8("辛"), __UTF8("壬"), __UTF8("癸")
    };

    static const char *TerrestrialBranch[12] = {
        __UTF8("子"), __UTF8("丑"), __UTF8("寅"), __UTF8("卯"), __UTF8("辰"), __UTF8("巳"), __UTF8("午"), __UTF8("未"), __UTF8("申"), __UTF8("酉"), __UTF8("戌"), __UTF8("亥")
    };

    static const char *ChineseZodiac[12] = {
        __UTF8("鼠"), __UTF8("牛"), __UTF8("虎"), __UTF8("兔"), __UTF8("龙"), __UTF8("蛇"), __UTF8("马"), __UTF8("羊"), __UTF8("猴"), __UTF8("鸡"), __UTF8("狗"), __UTF8("猪")
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
            if (date.leap) str.append(__UTF8("闰"));
            str.append(Chinese_MonthText[date.month - 1]);
            str.append(date.major ? __UTF8("大") : __UTF8("小"));
            return str;
        }
    }

    static std::string GetChineseDateTextLong(const calendar::ChineseDate &date) {
        int cz = (date.year + 8) % 12;
        std::string str;
        str.append(CelestialStem[(date.year + 6) % 10]);
        str.append(TerrestrialBranch[cz]);
        str.append(__UTF8("（"));
        str.append(ChineseZodiac[cz]);
        str.append(__UTF8("）年"));
        if (date.leap) str.append(__UTF8("闰"));
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
        /*  9月 */ VICTORY_MEMORIAL_DAY, TEACHERS_DAY, MARTYRS_DAY,
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
        /*  9月 */ __UTF8("抗战胜利"), __UTF8("教师节"), __UTF8("烈士纪念日"),
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
        case 0x091E: if (date.year >= 2014) return std::make_pair(MARTYRS_DAY, 2); break;
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
        if (date.leap) return std::make_pair(CHINESE_FESTIVAL_NONE, 0);
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
        case 0x0C1D: if (!date.major) return std::make_pair(SPRING_FESTIVAL_EVE, 3); break;
        case 0x0C1E: return std::make_pair(SPRING_FESTIVAL_EVE, 3);
        default: break;
        }
        return std::make_pair(CHINESE_FESTIVAL_NONE, 0);
    }
}
