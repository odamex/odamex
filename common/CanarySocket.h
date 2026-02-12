#pragma once

#include <functional>
#include <vector>

#ifdef _WIN32
#   define  WIN32_LEAN_AND_MEAN
#   include <winsock2.h>
#   define  CANARY_SOCKET_INT SOCKET
#   define  CANARY_BAD_SOCKET INVALID_SOCKET

// Ugh.  Why, Microsoft, why?
#   ifdef max
#       undef max
#   endif
#else
#   include <netinet/in.h>
#   include <sys/socket.h>
#   define  CANARY_SOCKET_INT int
#   define  CANARY_BAD_SOCKET -1
#endif

/// This class is a collection of "Canaries" that are actually TCP connections that let the server
/// know if any remote clients disconnect suddenly and ungracefully.
///
/// The only data communicated over the TCP connection is a UDP port number in network byte order,
/// immediately after the connection is established.  From that point forward the connection goes
/// slient and is monitored for being closed by the client itself or by the client's host OS.

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

        /// Callback to notify the server code about a new client.
        /// The callback must accept a reference to sockaddr_in and return the player ID associated
        /// with the given address.
        using CallbackType = std::function<int (sockaddr_in& address)>;

        template <typename Callable>
        void SetConnectCallback(Callable i_func) { m_connectCallback = i_func; }

        iterator end() { return m_canaries.end(); }

        /// Call out "Bring out your dead!" once per tic.  Will also handle new clients and make callbacks if needed.
        /// Returns an iterator to the first dead canary if there are any.  Put it on the cart once done with
        /// it and repeat until you reach end().
        iterator FindDead();
        iterator PutOnCart(iterator i_deadCanaryIter) { return m_canaries.erase(i_deadCanaryIter); }

    protected:

        std::vector<PlayerSocketType> m_canaries;
        CallbackType                  m_connectCallback;
        CANARY_SOCKET_INT             m_serverSocket;
};

/// This class is the client's mechanism for establishing the "Canary" on the server side.
class CanarySocketClient
{
    public:
        ~CanarySocketClient();

        /// Establish a connection to the Canary server.  Connect to the given i_toAddress,
        /// and supply the local UDP i_dataAddress that this client uses to communicate game traffic.
        /// Returns true if a new connection was successfully established, false otherwise.
        bool Connect(const sockaddr_in& i_toAddress, const sockaddr_in& i_dataAddress);

    protected:
        CANARY_SOCKET_INT m_socket { CANARY_BAD_SOCKET };
};
