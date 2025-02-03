// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <queue>
#include <thread>
#include <future>

namespace ms::vampires_nx_vms_plugin {

/** Opens a socket for reading the incoming characters. Not thread-safe. */
class SocketReader final
{
public:
    SocketReader() noexcept;
    ~SocketReader();

    /** Opens the socket and starts listening to connections. */
    bool startListening(int port) noexcept;

    /** Reads a character from the buffer if there is one, without blocking. */
    std::optional<char> getChar() noexcept;

    /** Clears the buffer. */
    void clear() noexcept;

private:
    std::vector<char> receiveAvailableBytes() noexcept;
    void closeSocket() noexcept;

private:
    bool m_hasReceivedData = false;
    int m_port = -1;
    int m_socketFd = -1;
    std::future<int> m_dataFdFuture;
    std::optional<int> m_dataFd;
    std::queue<char> m_buffer;
};

} // namespace ms::vampires_nx_vms_plugin
