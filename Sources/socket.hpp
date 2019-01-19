//
//  socket.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-16.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

// This file contains objects use for socket communications. It assumes a C library with
// standard POSIX sockets as the core of its implementation.

#ifndef kssio_socket_hpp
#define kssio_socket_hpp

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <sys/socket.h>

namespace kss {
    namespace io {
        namespace net {

            constexpr int nextAvailablePort = -2;
            constexpr int defaultStartingPort = 5000;
            constexpr int maxPossiblePortAsInt = std::min((int)std::numeric_limits<uint16_t>::max(),
                                                          std::numeric_limits<int>::max()-1);


            /*!
             Bind to a port for a given socket type and family. If the requested port
             is kss::network::nextAvailablePort, then this will search for an available
             port, starting at startingPort (5000 by default), and bind.

             @param socket the socket we wish to bind
             @param port the port we wish to bind to, may be nextAvailablePort to automatically
                choose one.
             @param addr detas of the address we wish to bind. The port of this address
             will be changed as part of the binding. If NULL, then an address that assumes
             the AF_INET family and INADDR_ANY address will be used.
             @param addrLen the length of whatever was passed to addr.
             @param startingPort the first port to try to connect to. This is ignored unless
                port is set to nextAvailablePort.
             @param endingPort the last port number to try to connect to. This is ignored unless
                port is set to nextAvailablePort.
             @return the port that we bound to

             @throws std::invalid_argument if sock is invalid (not positive)
             @throws std::invalid_argument if port is not nextAvailablePort and port is invalid
             @throws std::invalid_argument if port is nextAvailablePort and startingPort or
                maxPort are invalid or if startingPort > maxPort
             @throws std::system_error if we were unable to bind
             */
            int bindToPort(int socket,
                           int port,
                           struct sockaddr* addr = nullptr,
                           size_t addrLen = 0,
                           int startingPort = defaultStartingPort,
                           int endingPort = maxPossiblePortAsInt);

            /*!
             Return the next available port for creating a service, starting at startingPort
             (5000 by default) and ending with endingPort (maxPossiblePortAsInt by default.

             Warning: Using this method introduces a security risk in the form of a race
             condition, and is primarily intended for software testing, not production use.
             Specifically the port that is returned could be used by another process in the
             time between this call and when you actually use the port. Use bindToPort() if
             possible to avoid this race condition.

             @param startingPort the first port to try to connect to.
             @param endingPort the last port number to try to connect to.
             @return the port that we identified as available.

             @throws std::invalid_argument if port is nextAvailablePort and startingPort or
                maxPort are invalid or if startingPort > maxPort
             @throws std::system_error if we were unable to find a suitable port
             */
            int findNextAvailablePort(int startingPort = defaultStartingPort,
                                      int endingPort = maxPossiblePortAsInt);
        }
    }
}

#endif
