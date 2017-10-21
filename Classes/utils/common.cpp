#include "common.h"
#include <stdarg.h>

namespace Common {

std::string format(const char *fmt, ...) {
    std::string ret;
    va_list ap;

    size_t fmtlen = strlen(fmt);
    if (LIKELY(fmtlen < INT_MAX)) {  // Ensure fmtlen is in an int
        int len = static_cast<int>(fmtlen) + 1;

        // For each %, reserve 64 characters
        for (const char *p = strchr(fmt, '%'); p != nullptr; p = strchr(p, '%')) {
            char ch = *++p;
            if (ch != '%' && ch != '\0') len += 64;  // skip %%
        }

        do {
            ret.resize(len);

            va_start(ap, fmt);
            int size = vsnprintf(&ret[0], len, fmt, ap);
            va_end(ap);

            if (LIKELY(size >= 0)) {
                if (LIKELY(size < len)) {  // Everything worked
                    ret.resize(size);
                    ret.shrink_to_fit();
                    break;
                }
                len = size + 1;  // Needed size returned
                continue;
            }
            len *= 2;  // Guess at a larger size
        } while (1);
    }

    return ret;
}

}
