#ifndef __UTILS_SOCKET_H__
#define __UTILS_SOCKET_H__

#if (defined _WIN32) || (defined WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T
typedef int socklen_t;
#else

#include <sys/types.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;

#endif

#include <stdint.h>
#include "compiler.h"

namespace p2p {
    class Socket {
    public:
        Socket();
        ~Socket();

        virtual void quit();

        ssize_t send(const char *buf, size_t len);
        ssize_t recv(char *buf, size_t len);

    protected:
        SOCKET _socket;
    };

    class Sender : public Socket {
    public:
        Sender();

        virtual void quit() override;
        uint32_t prepare();
        bool accept();

    private:
        SOCKET _socketLoc;
    };

    class Reciever : public Socket {
    public:
        bool connect(uint32_t address);
    };
}

#endif
