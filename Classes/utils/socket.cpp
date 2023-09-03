#include "socket.h"

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#if (defined _WIN32) || (defined WIN32)
#   define PLATFORM_IS_WINDOWS 1
#else
#   define PLATFORM_IS_WINDOWS 0
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <net/if.h>
#   include <sys/ioctl.h>
#   include <string.h>

static FORCE_INLINE int closesocket(int s) {
    return ::shutdown(s, SHUT_RDWR);
}
#endif

#include <stdio.h>

namespace p2p {

    Socket::Socket() :_socket(INVALID_SOCKET) {
#if PLATFORM_IS_WINDOWS
        WSADATA data;
        WORD ver = MAKEWORD(2, 2);
        int ret = ::WSAStartup(ver, &data);
        if (ret != 0) {
            //LOG_DEBUG("WSAStartup failed: last error %d", ::WSAGetLastError());
        }
#endif
    }

    Socket::~Socket() {
        quit();
#if PLATFORM_IS_WINDOWS
        ::WSACleanup();
#endif
    }

    void Socket::quit() {
        if (_socket != INVALID_SOCKET) {
            ::closesocket(_socket);
            _socket = INVALID_SOCKET;
        }
    }

    ssize_t Socket::recv(char *buf, size_t len) const {
        return ::recv(_socket, buf, len, 0);
    }

    ssize_t Socket::send(const char *buf, size_t len) const {
        return ::send(_socket, buf, len, 0);
    }

    Sender::Sender() : _socketLoc(INVALID_SOCKET) {
    }

    static std::string getLocalIP() {
#if PLATFORM_IS_WINDOWS
        char hostName[260];
        ::gethostname(hostName, sizeof(hostName));
        struct hostent *host = ::gethostbyname(hostName);
        return ::inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
#else
        std::string ret = "0.0.0.0";

        int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));

        static const char *names[] = { "wlan0", "en0", "eth0", "bridge100" };
        for (size_t i = 0, cnt = sizeof(names) / sizeof(*names); i < cnt; ++i) {
            memset(&ifr, 0, sizeof(ifr));
            strcpy(ifr.ifr_name, names[i]);
            ::ioctl(sock, SIOCGIFADDR, &ifr);
            char *ip = ::inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);
            if (strcmp(ip, "0.0.0.0") != 0) {
                ret = ip;
                break;
            }
        }

        ::shutdown(sock, SHUT_RDWR);

        return ret;
#endif
    }

    void Sender::quit() {
        if (_socketLoc != INVALID_SOCKET) {
            ::closesocket(_socketLoc);
            _socketLoc = INVALID_SOCKET;
        }

        Socket::quit();
    }

    std::string Sender::prepare() {
        _socketLoc = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_socketLoc == INVALID_SOCKET) {
            return 0;
        }

        std::string ip = getLocalIP();

        struct sockaddr_in sin = { 0 };
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = ::inet_addr(ip.c_str()); //INADDR_ANY;

        if (SOCKET_ERROR == ::bind(_socketLoc, (struct sockaddr *)&sin, sizeof(sin))) {
            return 0;
        }

        struct sockaddr_in addrLoc; 
        socklen_t addrLen = sizeof(addrLoc);
        ::getsockname(_socketLoc, (struct sockaddr *)&addrLoc, &addrLen);

        if (SOCKET_ERROR == ::listen(_socketLoc, 1)) {
            return 0;
        }

        return ip + ":" + std::to_string(ntohs(addrLoc.sin_port));
    }

    bool Sender::accept() {
        struct sockaddr_in addrRom; 
        socklen_t addrLen = sizeof(addrRom);
        _socket = ::accept(_socketLoc, (struct sockaddr *)&addrRom, &addrLen);
        return (_socket != INVALID_SOCKET);
    }

    bool Reciever::connect(const char *address) {
        _socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_socket == INVALID_SOCKET) {
            return false;
        }

        const char *p = strchr(address, ':');
        if (p == nullptr) {
            return false;
        }

        char serverIp[64];
        memcpy(serverIp, address, p - address);
        serverIp[p - address] = '\0';

        uint16_t port = static_cast<uint16_t>(atoi(p + 1));

        struct sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = ::inet_addr(serverIp);
        serverAddr.sin_port = htons(port);

        int ret = ::connect(_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
        return (ret != SOCKET_ERROR);
    }
}
