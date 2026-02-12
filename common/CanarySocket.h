#pragma once

#include <functional>
#include <vector>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <winsock2.h>
#   define  CANARY_SOCKET_INT SOCKET
#   define  CANARY_BAD_SOCKET INVALID_SOCKET
#else
#   include <sys/socket.h>
#   define  CANARY_SOCKET_INT int
#   define  CANARY_BAD_SOCKET -1
#endif

#ifdef max
#undef max
#endif

class CanarySocketServer
{
    public:
        struct PlayerSocketType
        {
            int               id;
            CANARY_SOCKET_INT socket;
            bool              isAlive;

            PlayerSocketType(int i_id, CANARY_SOCKET_INT i_socket) :
                id      (i_id),
                socket  (i_socket),
                isAlive (true)
            {
            }
        };

        using iterator = std::vector<PlayerSocketType>::iterator;

        explicit CanarySocketServer(int i_tcpPort);
        ~CanarySocketServer();

        template <typename Callable>
        void SetConnectCallback(Callable i_func)
        {
            m_connectCallback = i_func;
        }

        iterator end()   { return m_canaries.end();   }

        // Bring out your dead!
        iterator FindDead();
        iterator PutOnCart(iterator i_deadCanaryIter) { return m_canaries.erase(i_deadCanaryIter); }

    protected:

        std::vector<PlayerSocketType> m_canaries;

        std::function<int (sockaddr_in& address)> m_connectCallback;

        CANARY_SOCKET_INT m_serverSocket;
};

class CanarySocketClient
{
    public:
        ~CanarySocketClient();
        bool Connect(const sockaddr_in& i_toAddress, const sockaddr_in& i_dataAddress);

    protected:
        CANARY_SOCKET_INT m_socket {CANARY_BAD_SOCKET};
};
