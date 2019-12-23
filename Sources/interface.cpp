//
//  interface.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2016-09-28.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_map>
#include <utility>

#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>

#if defined(__APPLE__)
#   include <net/if_dl.h>
#endif

#if defined(__linux)
#   include <unistd.h>
#   include <sys/ioctl.h>
#endif

#include <kss/contract/all.h>
#include <kss/util/all.h>

#include "interface.hpp"
#include "utility.hpp"


using namespace std;
using namespace kss::io::net;

namespace contract = kss::contract;

using kss::util::Finally;
using kss::util::strings::Tokenizer;


///
/// MARK: IpV4Address implementation
///

namespace {
    inline void throwInvalidIp(const string& addrStr) {
        throw invalid_argument("'" + addrStr + "' is not a valid IP address");
    }

    uint32_t ipFromSockAddr(const struct sockaddr *saddr) {
        uint32_t ret = 0;
        if (saddr) {
#if defined(__APPLE__)
            contract::conditions({
                KSS_EXPR(saddr->sa_family == AF_INET)
            });
#endif

            const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(saddr);
            ret = ntoh((uint32_t)sin->sin_addr.s_addr);
        }
        return ret;
    }
}


IpV4Address::IpV4Address(const string& addrStr) {
    contract::parameters({
        KSS_EXPR(!addrStr.empty())
    });

    Tokenizer tok(addrStr, ".", false);
    uint8_t ar[4];

    auto it = tok.begin();
    for (size_t i = 0; i < 4U; ++i) {
        if (it == tok.end()) { throwInvalidIp(addrStr); }
        if (it->empty()) { throwInvalidIp(addrStr); }

        size_t numCharsParsed = 0;
        const unsigned long ul = stoul(*it, &numCharsParsed, 10);
        if (numCharsParsed == 0) { throwInvalidIp(addrStr); }
        if (ul > 255) { throwInvalidIp(addrStr); }

        ar[i] = (uint8_t)ul;
        ++it;
    }
    if (it != tok.end()) { throwInvalidIp(addrStr); }

    _addr = pack<uint32_t, 4>(ar);
}


IpV4Address::operator string() const {
    uint8_t ar[4];
    unpack<uint32_t, 4>(_addr, ar);

    ostringstream oss;
    for (size_t i = 0; i < 3; ++i) {
        oss << (int)ar[i] << ".";
    }
    oss << (int)ar[3];

    contract::postconditions({
        KSS_EXPR(IpV4Address(oss.str()) == *this)
    });
    return oss.str();
}


///
/// MARK: MacAddress implementation
///

namespace {
    inline void throwInvalidMac(const string& addrStr) {
        throw invalid_argument("'" + addrStr + "' is not a valid MAC address");
    }

#if defined(__APPLE__)
    uint64_t macFromSockAddr(const struct sockaddr *saddr) {
        uint64_t ret = 0;
        if (saddr) {
            contract::conditions({
                KSS_EXPR(saddr->sa_family == AF_LINK)
            });

            const sockaddr_dl* sdl = reinterpret_cast<const sockaddr_dl*>(saddr);
            if (sdl->sdl_alen == 6) {   // Ignore items that cannot be a IPV4 (which all have
                uint8_t ar[6];          // mac address of 6 bytes).
                memcpy(ar, LLADDR(sdl), 6);
                ret = pack<uint64_t, 6>(ar);
            }
        }
        return ret;
    }
#endif
}

MacAddress::MacAddress(const string& addrStr) {
    contract::parameters({
        KSS_EXPR(!addrStr.empty())
    });

    Tokenizer tok(addrStr, ":", false);
    uint8_t ar[6];

    auto it = tok.begin();
    for (size_t i = 0; i < 6U; ++i) {
        if (it == tok.end()) { throwInvalidMac(addrStr); }
        if (it->empty()) { throwInvalidMac(addrStr); }

        size_t numCharsParsed = 0;
        const unsigned long ul = stoul(*it, &numCharsParsed, 16);
        if (numCharsParsed == 0) { throwInvalidMac(addrStr); }
        if (ul > 255) { throwInvalidMac(addrStr); }

        ar[i] = (uint8_t)ul;
        ++it;
    }
    if (it != tok.end()) { throwInvalidMac(addrStr); }
    
    _addr = pack<uint64_t, 6>(ar);
}

MacAddress::operator string() const {
    uint8_t ar[6];
    unpack<uint64_t, 6>(_addr, ar);

    ostringstream oss;
    oss << setfill('0') << setw(2);
    for (size_t i = 0; i < 5; ++i) {
        oss << setbase(16) << setfill('0') << setw(2) << (int)ar[i];
        oss << setw(0) << ":";
    }
    oss << setbase(16) << setfill('0') << setw(2) << (int)ar[5];

    contract::postconditions({
        KSS_EXPR(MacAddress(oss.str()) == *this)
    });
    return oss.str();
}

///
/// MARK: network_interface implementation
///

// Internal representation.
struct NetworkInterface::Impl {
    string      name;
    unsigned    flags = 0U;
    IpV4Address v4Address;
    IpV4Address v4NetMask;
    IpV4Address v4Broadcast;
    MacAddress  hwAddress;
};


namespace {

    // Obtain a map from interface names to mac addresses currently available.
    typedef unordered_map<string, MacAddress> mac_addr_map_t;
    typedef pair<const struct ifaddrs*, const mac_addr_map_t*> os_impl_t;

    mac_addr_map_t getMacAddresses() {
        mac_addr_map_t ret;
#if defined(__APPLE__)
        struct ifaddrs* addrs = nullptr;
        Finally cleanup([&]{
            if (addrs) { freeifaddrs(addrs); }
        });

        if (getifaddrs(&addrs) == -1) {
            throw system_error(errno, system_category(), "getifaddrs");
        }
        for (auto cur = addrs; cur; cur = cur->ifa_next) {
            if (cur->ifa_addr != nullptr && cur->ifa_addr->sa_family == AF_LINK) {
                MacAddress ma(macFromSockAddr(cur->ifa_addr));
                if ((bool)ma == true) {
                    ret[cur->ifa_name] = ma;
                }
            }
        }
#elif defined(__linux)
        // This implementation is based on example code found at
        // https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program
        const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock == -1) {
            throw system_error(errno, system_category(), "socket");
        }
        Finally cleanup([&] {
            if (sock != -1) { close(sock); }
        });

        struct ifreq ifr;
        struct ifconf ifc;
        char buf[1024];

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
            throw system_error(errno, system_category(), "ioctl");
        }

        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
        for (; it != end; ++it) {
            strcpy(ifr.ifr_name, it->ifr_name);
            if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
                throw system_error(errno, system_category(), "ioctl");
            }

            if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
                throw system_error(errno, system_category(), "ioctl");
            }

            MacAddress ma(pack<uint64_t, 6>(reinterpret_cast<const uint8_t*>(ifr.ifr_hwaddr.sa_data)));
            if ((bool)ma == true) {
                ret[ifr.ifr_name] = ma;
            }
        }
#else
#       error Not implemented for this OS
#endif
        return ret;
    }

    // Note that the callback in for_each_interface should return true if we should
    // continue to the next value and false if it should stop iterating. In addition
    // it is safe to perform a move from the network interface when calling this
    // function.
    void forEachInterface(function<bool(NetworkInterface&)> cb) {
        struct ifaddrs* addrs = nullptr;
        Finally cleanup([&]{
            if (addrs) { freeifaddrs(addrs); }
        });

        if (getifaddrs(&addrs) == -1) {
            throw system_error(errno, system_category(), "getifaddrs");
        }
        auto macAddressMap = getMacAddresses();
        for (auto cur = addrs; cur; cur = cur->ifa_next) {
            if (cur->ifa_addr != nullptr && cur->ifa_addr->sa_family == AF_INET) {
                os_impl_t data;
                data.first = cur;
                data.second = &macAddressMap;

                auto ni = NetworkInterface::_fromOsImpl(reinterpret_cast<void*>(&data));
                if (!cb(ni)) { break; }
            }
        }
    }
}


// Constructors and assignment.

NetworkInterface::NetworkInterface() : _impl(new Impl()) {}
NetworkInterface::~NetworkInterface() noexcept = default;
NetworkInterface::NetworkInterface(NetworkInterface&& ni) = default;

NetworkInterface::NetworkInterface(const NetworkInterface& ni) : _impl(new Impl()) {
    *_impl = *ni._impl;

    contract::postconditions({
        KSS_EXPR(ni == *this)
    });
}

NetworkInterface& NetworkInterface::operator=(NetworkInterface&&) noexcept = default;
NetworkInterface& NetworkInterface::operator=(const NetworkInterface& rhs) {
    if (&rhs != this) {
        *_impl = *rhs._impl;
    }

    contract::postconditions({
        KSS_EXPR(rhs == *this)
    });
    return *this;
}

NetworkInterface NetworkInterface::_fromOsImpl(void *osimpl) {
    NetworkInterface ret;
    os_impl_t* data = reinterpret_cast<os_impl_t*>(osimpl);

    const ifaddrs* rep = data->first;
    ret._impl->name = rep->ifa_name;
    ret._impl->flags = (unsigned)rep->ifa_flags;
    ret._impl->v4Address = IpV4Address(ipFromSockAddr(rep->ifa_addr));
    ret._impl->v4NetMask = IpV4Address(ipFromSockAddr(rep->ifa_netmask));
    if (ret.broadcast()) {
        ret._impl->v4Broadcast = IpV4Address(ipFromSockAddr(rep->ifa_broadaddr));
    }

    const mac_addr_map_t* addrMap = data->second;
    auto it = addrMap->find(ret._impl->name);
    if (it != addrMap->end()) { ret._impl->hwAddress = it->second; }
    return ret;
}

// Accessors.

const string& NetworkInterface::name() const noexcept               { return _impl->name; }
unsigned NetworkInterface::flags() const noexcept                   { return _impl->flags; }
const IpV4Address& NetworkInterface::v4Address() const noexcept     { return _impl->v4Address; }
const IpV4Address& NetworkInterface::v4NetMask() const noexcept     { return _impl->v4NetMask; }
const IpV4Address& NetworkInterface::v4Broadcast() const noexcept   { return _impl->v4Broadcast; }
const MacAddress& NetworkInterface::hwAddress() const noexcept      { return _impl->hwAddress; }



///
/// MARK: other method implementations
///

NetworkInterface kss::io::net::findInterface(const string& name) {
    NetworkInterface ret;
    forEachInterface([&](NetworkInterface& ni) {
        if (ni.name() == name) {
            ret = move(ni);
            return false;
        }
        return true;
    });
    return ret;
}

NetworkInterface kss::io::net::findActiveInterface() {
    NetworkInterface ret;
    forEachInterface([&](NetworkInterface& ni) {
        if (ni.up() && ni.running() && !ni.loopback()) {
            ret = move(ni);
            return false;
        }
        return true;
    });
    return ret;
}

MacAddress kss::io::net::findMacAddress() {
    return findActiveInterface().hwAddress();
}

vector<string> kss::io::net::findAllInterfaceNames() {
    vector<string> v;
    forEachInterface([&](NetworkInterface& ni) {
        v.push_back(ni.name());
        return true;
    });
    return v;
}

vector<NetworkInterface> kss::io::net::findAllInterfaces() {
    vector<NetworkInterface> v;
    forEachInterface([&](NetworkInterface& ni) {
        v.push_back(move(ni));
        return true;
    });
    return v;
}
