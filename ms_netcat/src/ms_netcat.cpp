// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include <exception>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <vector>
#include <cstdio>
#include <string>
#include <system_error>

#if !defined(_WIN32)
    #error "ms_netcat is Windows-only."
#endif

#include <conio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <nx/kit/debug.h>
#include <nx/kit/utils.h>

static std::string getLastSocketError(const std::string& message)
{
    return message + ": " + std::system_category().message(WSAGetLastError());
}

[[noreturn]] static void throwSocketError(const std::string& message)
{
    throw std::exception(getLastSocketError(message).c_str());
}

struct SocketSubsystem
{
    SocketSubsystem()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            throwSocketError("WSAStartup() failed");
    }

    ~SocketSubsystem()
    {
        WSACleanup();
    }
};

struct AddrInfo
{
    addrinfo* data = nullptr;

    AddrInfo(const std::string& host, int port)
    {
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        const auto r = getaddrinfo(
            host.c_str(),
            nx::kit::utils::toString(port).c_str(),
            &hints,
            &data);

        if (r != 0)
            throwSocketError("getaddrinfo() failed");
    }

    ~AddrInfo()
    {
        if (data)
            freeaddrinfo(data);
    }
};

struct Socket
{
    int fd = -1;
    bool connected = false;

    Socket()
    {
        fd = (int) ::socket(PF_INET, SOCK_STREAM, /*protocol*/ IPPROTO_TCP);
        if (fd < 0)
            throwSocketError("Unable to create a socket: socket() failed");
    }

    ~Socket()
    {
        if (fd >= 0)
        {
            std::cerr << "\n"; //< Newline after the logged keystrokes.
            if (connected)
            {
                if (::shutdown(fd, SD_BOTH) < 0)
                {
                    // Cannot throw an exception in the destructor.
                    NX_PRINT << getLastSocketError("Unable to shutdown the socket: shutdown() failed");
                }
            }
            ::closesocket(fd);
        }
    }

    void connect(const std::string& host, int port)
    {
        const AddrInfo addrInfo(host, port);
        if (::connect(fd, addrInfo.data->ai_addr, (int) addrInfo.data->ai_addrlen) != 0)
            throwSocketError("Unable to connect to the server: connect() failed");
        connected = true;
    }

    void send(char c)
    {
        if (!NX_KIT_ASSERT(connected))
            return;
        if (::send(fd, &c, /*len*/ 1, /*flags*/ 0) < 0)
            throwSocketError("Unable to send a byte to the server: send() failed");   
    }
};

static void netcat(const std::string& host, int port)
{
    [[maybe_unused]] SocketSubsystem socketSubsystem;
    Socket socket;
    socket.connect(host, port);
    NX_PRINT << "Connected to " << host << ":" << port << ". "
        << "Press keys to send keystrokes, ^C to exit:";

    for (;;)
    {
        const int key = _getch();
        NX_KIT_ASSERT(key != EOF); //< _getch() never returns EOF.
        if (key == '\x03') //< ^C.
        {
            std::cerr << "^C\n";
            break;
        }
        std::cout << nx::kit::utils::toString((char) key) << " ";
        socket.send((char) key);
    }

    NX_PRINT << "Disconnecting from the server.";
}

static void printHelp()
{
    std::cout <<
R"(
Connects to the specified host and port via a TCP socket, and sends alphanumeric keystrokes to it.

Equivalent of Linux/Cygwin's command:
    stty -icanon && nc <host> <port>

Usage:
 )" << nx::kit::utils::getProcessName() << R"( <host> <port>
)";
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 1 || (
            argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0
                || strcmp(argv[1], "/?") == 0)))
        {
            printHelp();
            exit(0);
        }

        if (argc != 3)
        {
            std::cerr << "ERROR: Expected 2 args. Run with -h, --help or /? for usage help.\n";
            exit(1);
        }

        int port;
        if (!nx::kit::utils::fromString(argv[2], &port) || port < 1 || port > 65535)
        {
            std::cerr << "ERROR: Invalid port value " << nx::kit::utils::toString(argv[2])
                << ": expected an integer in range [1, 65535].\n";
            exit(1);
        }

        const std::string host = argv[1];

        netcat(host, port);
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(1);
    }
    catch (...)
    {
        std::cerr << "ERROR: Unknown exception.\n";
        exit(1);
    }

    return 0;
}
