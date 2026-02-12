
#include "CanarySocket.h"

#include <algorithm>
#include <iso646.h>
#include <string.h>

#ifdef _WIN32
using socklen_t = int;
#else
#   include <unistd.h>
#   define closesocket(x) close(x)
#endif

CanarySocketServer::CanarySocketServer(int i_tcpPort) :
    m_serverSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{
    if (m_serverSocket >= 0)
    {
        sockaddr_in address;

        memset (&address, 0, sizeof(address));
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port        = htons(static_cast<unsigned short>(i_tcpPort));

        if (bind(m_serverSocket,
                 reinterpret_cast<sockaddr*>(&address),
                 sizeof(address)) == 0)
        {
            if (listen(m_serverSocket, 10) == 0)
            {
                return;
            }
        }

        closesocket(m_serverSocket);
        m_serverSocket = CANARY_BAD_SOCKET;
    }
}

CanarySocketServer::~CanarySocketServer()
{
    for (auto& canary : m_canaries)
    {
        closesocket(canary.socket);
    }
}

CanarySocketServer::iterator CanarySocketServer::FindDead()
{
    if (m_serverSocket == CANARY_BAD_SOCKET)
    {
        return m_canaries.end();
    }

    fd_set sockets;
    FD_ZERO(&sockets);
    FD_SET(m_serverSocket, &sockets);
    CANARY_SOCKET_INT greatestSocketValue = m_serverSocket;

    for (auto& canary : m_canaries)
    {
        FD_SET(canary.socket, &sockets);
        greatestSocketValue = std::max(greatestSocketValue, canary.socket);
    }

    timeval noWait     = {0, 0};
    int     numSockets = select(greatestSocketValue + 1, &sockets, nullptr, nullptr, &noWait);

    if (numSockets > 0)
    {
        if (FD_ISSET(m_serverSocket, &sockets))
        {
            --numSockets;

            sockaddr_in address;
            sockaddr_in dataAddress;

            socklen_t   addressLength = sizeof(address);
            const CANARY_SOCKET_INT clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&address), &addressLength);

            dataAddress = address;

            recv(clientSocket, reinterpret_cast<char*>(&dataAddress.sin_port), sizeof(dataAddress.sin_port), MSG_WAITALL);

            const int playerId = m_connectCallback ? m_connectCallback(dataAddress) : -1;

            m_canaries.emplace_back(playerId, clientSocket);
        }

        // As we find dead canaries, move them to the end of the vector so that we can easily just erase
        // them without moving too much data.
        iterator firstDeadCanary = m_canaries.end();
        iterator iter = m_canaries.begin();
        while (iter != firstDeadCanary and numSockets > 0)
        {
            if (FD_ISSET(iter->socket, &sockets))
            {
                using std::swap;

                --numSockets;
                --firstDeadCanary;
                swap(*iter, *firstDeadCanary);
            }
            else
            {
                ++iter;
            }
        }
        return firstDeadCanary;
    }
    return m_canaries.end();
}

CanarySocketClient::~CanarySocketClient()
{
    if (m_socket >= 0)
    {
        closesocket(m_socket);
    }
}

bool CanarySocketClient::Connect(const sockaddr_in& i_toAddress, const sockaddr_in& i_dataAddress)
{
    if (m_socket == CANARY_BAD_SOCKET)
    {
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket >= 0)
        {
            if (connect(m_socket, reinterpret_cast<const sockaddr*>(&i_toAddress), sizeof(i_toAddress)) == 0)
            {
                // sin_port is already in network byte order.
                if (send(m_socket,
                         reinterpret_cast<const char*>(&i_dataAddress.sin_port),
                         sizeof(i_dataAddress.sin_port),
                         0) == sizeof(i_dataAddress.sin_port))
                {
                    return true;
                }
            }
            closesocket(m_socket);
            m_socket = CANARY_BAD_SOCKET;
        }
    }
    return false;
}
