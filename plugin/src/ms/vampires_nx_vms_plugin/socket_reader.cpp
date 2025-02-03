// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include "socket_reader.h"

#include <system_error>

#if defined(_WIN32)
    #include <WinSock2.h>
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include <nx/kit/debug.h>

namespace ms::vampires_nx_vms_plugin {

using namespace std::chrono_literals;

#if !defined(_WIN32)
    #error "Currently this socket wrapper is Windows-only."
#endif

using nx::kit::utils::format;
using nx::kit::utils::toString;

/** Allows to be called as `return error("%1...", args);`. */
template<typename... Args>
static bool error(Args&&... args)  noexcept
{
    NX_PRINT << "ERROR: " << format(std::forward<decltype(args)>(args)...) << ": " +
        std::system_category().message(WSAGetLastError());
    return false;
}

SocketReader::SocketReader() noexcept
{
}

SocketReader::~SocketReader()
{
    NX_PRINT << "\n####### Closing the connection";
    closeSocket();
}

void SocketReader::closeSocket() noexcept
{
    if (m_socketFd >= 0)
    {
        if (closesocket(m_socketFd) != 0)
            error("Unable to close the socket");
        m_socketFd = -1;
        m_dataFd.reset();
        m_hasReceivedData = false;
    }
}

static void waitForConnection(int socketFd, std::promise<int> dataFdPromise) noexcept
{
    sockaddr_in clientAddr;
    int len = sizeof(sockaddr_in);
    const int dataFd = (int) accept(socketFd, (sockaddr*) &clientAddr, &len);
    if (dataFd < 0)
    {
        error("Unable to accept on the socket");
        return;
    }

    NX_PRINT << format("\n####### Connection accepted from %d.%d.%d.%d\n",
        clientAddr.sin_addr.S_un.S_un_b.s_b1,
        clientAddr.sin_addr.S_un.S_un_b.s_b2,
        clientAddr.sin_addr.S_un.S_un_b.s_b3,
        clientAddr.sin_addr.S_un.S_un_b.s_b4);

    dataFdPromise.set_value(dataFd);
}

static void printWelcomeMessage(int port) noexcept
{
    NX_PRINT << format(
R"(

###################################################################################################
ATTENTION: Waiting for incoming connection at port %d.

Execute the following command in another terminal:
    Linux or Cygwin:
        stty -icanon && nc localhost %d
    Git Bash or cmd:
        ms_netcat localhost %d
)", port, port, port, port);
}

bool SocketReader::startListening(int port) noexcept
{
    if (!NX_KIT_ASSERT(port > 0) || !NX_KIT_ASSERT(port <= 65535))
        return false;
    if (!NX_KIT_ASSERT(m_socketFd < 0) || !NX_KIT_ASSERT(!m_dataFd))
        return false;

    m_port = port;

    if ((m_socketFd = (int) socket(PF_INET, SOCK_STREAM, /*protocol*/ 0)) < 0)
        return error("Socket creation failed");

    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(m_port);

    if (bind(m_socketFd, (sockaddr*) &localAddr, sizeof(localAddr)) < 0)
        return error("Unable to bind on the socket");

    if (listen(m_socketFd, /*backlog*/ 100) < 0)
        return error("Unable to listen on the socket");

    printWelcomeMessage(m_port);

    // Create a thread which will wait for the connection and return its data fd via the `future`.
    std::promise<int> dataFdPromise;
    m_dataFdFuture = dataFdPromise.get_future();
    std::thread(waitForConnection, m_socketFd, std::move(dataFdPromise)).detach();

    return true;
}

std::vector<char> SocketReader::receiveAvailableBytes() noexcept
{
    std::vector<char> bytes;
    for (;;) //< Looping to allow more bytes to arrive while we are reading the previous ones.
    {
        static constexpr int kBufferSize = 256;
        const int oldBytesCount = (int) bytes.size();
        bytes.resize(oldBytesCount + kBufferSize);
        const int r = recv(*m_dataFd, &bytes[oldBytesCount], kBufferSize, /*flags*/ 0);
        if (r > 0)
        {
            bytes.resize(oldBytesCount + r);
            continue;
        }
        if (r == 0)
        {
            NX_PRINT << "Connection was closed by the sender - please reconnect.";
            closeSocket();
            startListening(m_port);
            return {};
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            error("Unable to read from the socket");
            return {};
        }
        // No data was read on this iteration.
        bytes.resize(oldBytesCount);
        break;
    }
    return bytes;
}

std::optional<char> SocketReader::getChar() noexcept
{
    if (!m_dataFd)
    {
        if (m_dataFdFuture.wait_for(1us) != std::future_status::ready)
            return std::nullopt;
        m_dataFd = m_dataFdFuture.get(); //<< The socket has connected.

        u_long argp = 1;
        if (ioctlsocket(*m_dataFd, FIONBIO, &argp) < 0)
            error("Unable to set the socket to non-blocking mode");
    }

    const std::vector<char> bytes = receiveAvailableBytes();

    // Copy received bytes to the buffer, removing consecutive keystrokes to avoid inertia.
    char prevByte = m_buffer.empty() ? '\0' : m_buffer.back();
    for (const char byte: bytes)
    {
        if (byte == prevByte)
            continue;
        m_buffer.push(byte);
        prevByte = byte;
    }
    if (m_buffer.empty())
        return std::nullopt;

    const char c = m_buffer.front();
    if (!m_hasReceivedData)
    {
        NX_PRINT << "\n####### Received first keystroke: " << toString(c);
        m_hasReceivedData = true;
    }
    m_buffer.pop();
    return c;
}

void SocketReader::clear() noexcept
{
    std::queue<char> empty;
    std::swap(m_buffer, empty);
}

} // namespace ms::vampires_nx_vms_plugin
