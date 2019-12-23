//
//  socket.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-16.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cerrno>
#include <cstring>
#include <system_error>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <kss/contract/all.h>

#include "socket.hpp"

using namespace std;
using namespace kss::io::net;

namespace contract = kss::contract;


namespace {
    // Attempt to bind to a port. If successful we return the port number, otherwise
    // we throw an exception.
    int attemptToBind(int sock, int port, struct sockaddr* addr, socklen_t addrLen) {
        switch (addr->sa_family) {
            case AF_INET: {
                struct sockaddr_in* a = reinterpret_cast<struct sockaddr_in*>(addr);
                a->sin_port = htons(port);
                break;
            }
            case AF_INET6: {
                struct sockaddr_in6* a = reinterpret_cast<struct sockaddr_in6*>(addr);
                a->sin6_port = htons(port);
                break;
            }
            default:
                throw system_error(EAFNOSUPPORT, system_category(),
                                   "only AF_INET and AF_INET6 are currently supported");
        }

        if (::bind(sock, addr, addrLen) == -1) {
            throw system_error(errno, system_category(), "bind");
        }
        return port;
    }

    // Throw an exception if port is not a valid port number.
    void verifyPortNumber(const string& paramName, int port) {
        if (port <= 0) {
            throw invalid_argument(paramName + " must be positive");
        }
        if (port > maxPossiblePortAsInt) {
            throw invalid_argument(paramName
                                   + " must be no more than "
                                   + to_string(maxPossiblePortAsInt));
        }
    }
}


int kss::io::net::bindToPort(int sock, int port,
                             struct sockaddr* addr, size_t addrLen,
                             int startingPort, int endingPort)
{
    contract::parameters({
        KSS_EXPR(sock > 0),
        KSS_EXPR(addrLen <= numeric_limits<socklen_t>::max())
    });

    // Sanity checks on the inputs.
    const int requestedPort = port;
    if (port != nextAvailablePort) {
        verifyPortNumber("port", port);
    }
    if (port == nextAvailablePort) {
        verifyPortNumber("startingPort", startingPort);
        verifyPortNumber("endingPort", endingPort);
        if (startingPort > endingPort) {
            throw invalid_argument("startingPort must be less than or equal to endingPort");
        }
    }

    // Setup the default sockaddr if needed.
    struct sockaddr_in defaultAddr;
    if (addr == nullptr) {
        memset(&defaultAddr, 0, sizeof(defaultAddr));
        defaultAddr.sin_family = AF_INET;
        defaultAddr.sin_addr.s_addr = INADDR_ANY;
        addr = reinterpret_cast<struct sockaddr*>(&defaultAddr);
        addrLen = sizeof(defaultAddr);
    }

    if (port != nextAvailablePort) {
        // Attempt to bind to exactly the port specified.
        port = attemptToBind(sock, port, addr, (socklen_t)addrLen);
    }
    else {
        // Attempt to find a port we can bind to.
        for (port = startingPort; port <= endingPort; ++port) {
            try {
                return attemptToBind(sock, port, addr, (socklen_t)addrLen);
            }
            catch (const system_error& e) {
                if (e.code() == error_code(EADDRINUSE, system_category())) {
                    continue;
                }
                else {
                    throw;
                }
            }
        }

        // If we get here, then we failed on all possible ports.
        throw system_error(EADDRINUSE, system_category(),
                           "ports in [" + to_string(startingPort)
                           + "," + to_string(endingPort)
                           + "] unavailable");
    }

    contract::postconditions({
        KSS_EXPR(requestedPort == nextAvailablePort
                 ? port >= startingPort && port <= endingPort
                 : port == requestedPort)
    });

    return port;
}


int kss::io::net::findNextAvailablePort(int startingPort, int endingPort) {
    // We determine the next available port by using bindToPort. If successful, then we
    // shutdown the socket and return the port number.
    const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw system_error(errno, system_category(), "socket");
    }

    const int port = bindToPort(sock, nextAvailablePort, nullptr, 0, startingPort, endingPort);
    shutdown(sock, SHUT_RDWR);
    close(sock);

    contract::postconditions({
        KSS_EXPR(port >= startingPort && port <= endingPort)
    });

    return port;
}
