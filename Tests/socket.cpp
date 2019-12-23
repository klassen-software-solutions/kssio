//
//  socket.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-08-27.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstring>
#include <system_error>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <kss/io/socket.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::net;
using namespace kss::test;

namespace {
    // Unblock the socket.
    void unblock(int sock) noexcept {
        if (sock != -1) {
            shutdown(sock, SHUT_RDWR);
        }
    }

    // Block a port and return the socket.
    int block(int port) {
        const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            throw system_error(errno, system_category(), "socket");
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (::bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
            // If it fails to bind, it is already blocked enough for our purposes.
            unblock(sock);
            return -1;
        }

        return sock;
    }
}

static TestSuite ts("net::socket", {
    make_pair("bindToPort", [] {
        vector<int> blockedSockets;
        blockedSockets.push_back(block(5000));
        blockedSockets.push_back(block(5001));
        blockedSockets.push_back(block(5002));

        const int sock1 = ::socket(AF_INET, SOCK_STREAM, 0);
        KSS_ASSERT(sock1 != -1);
        blockedSockets.push_back(sock1);
        const int port = bindToPort(sock1, nextAvailablePort, nullptr, 0, 5000, 5010);
        KSS_ASSERT(port >= 5003);

        KSS_ASSERT(throwsException<invalid_argument>([] {
            bindToPort(-2, nextAvailablePort);
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            bindToPort(5555, nextAvailablePort);
        }));

        const int sock2 = ::socket(AF_INET, SOCK_STREAM, 0);
        KSS_ASSERT(sock2 != -1);
        blockedSockets.push_back(sock2);
        KSS_ASSERT(throwsException<system_error>([&] {
            bindToPort(sock2, 5001);
        }));
        KSS_ASSERT(throwsException<system_error>([&] {
            bindToPort(sock2, nextAvailablePort, nullptr, 0, 5000, 5003);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            bindToPort(sock2, -3);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            bindToPort(sock2, nextAvailablePort, nullptr, 0, -2);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            bindToPort(sock2, nextAvailablePort, nullptr, 0, 5000, -2);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            bindToPort(sock2, nextAvailablePort, nullptr, 0, 5010, 5000);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            bindToPort(sock2, nextAvailablePort, nullptr, 0, 5000, maxPossiblePortAsInt+1);
        }));

        for (const auto sock : blockedSockets) {
            unblock(sock);
        }
    }),
    make_pair("findNextAvailablePort", [] {
        vector<int> blockedSockets;
        blockedSockets.push_back(block(6000));
        blockedSockets.push_back(block(6001));
        blockedSockets.push_back(block(6002));

        KSS_ASSERT(findNextAvailablePort(6000) >= 6003);
        KSS_ASSERT(throwsException<system_error>([] {
            findNextAvailablePort(6000, 6002);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([] {
            findNextAvailablePort(0, -2);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([] {
            findNextAvailablePort(6000, -2);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([] {
            findNextAvailablePort(6010, 6000);
        }));
        KSS_ASSERT(throwsException<invalid_argument>([] {
            findNextAvailablePort(6000, maxPossiblePortAsInt+1);
        }));

        for (const auto sock : blockedSockets) {
            unblock(sock);
        }
    })
});
