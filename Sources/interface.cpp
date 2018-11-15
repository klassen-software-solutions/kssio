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
#include <net/if_dl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "_raii.hpp"
#include "_tokenizer.hpp"
#include "interface.hpp"
#include "utility.hpp"



using namespace std;
using namespace kss::io::net;
using kss::io::_private::finally;
using kss::io::_private::Tokenizer;


///
/// MARK: IpV4Address implementation
///

namespace {
    inline void throwInvalidIp(const string& addrStr) {
        throw invalid_argument("'" + addrStr + "' is not a valid IP address");
    }
}


IpV4Address IpV4Address::_fromSockAddr(const struct sockaddr *saddr) {
    IpV4Address ret;
    if (saddr) {
        assert(saddr->sa_family == AF_INET);
        const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(saddr);
        ret._addr = ntoh((uint32_t)sin->sin_addr.s_addr);
    }
    return ret;
}

IpV4Address::IpV4Address(const string& addrStr) {
    Tokenizer tok(addrStr, ".", false);
    uint8_t ar[4];

    for (size_t i = 0; i < 4U; ++i) {
        if (tok.eof()) { throwInvalidIp(addrStr); }

        string s;
        tok >> s;
        if (s.empty()) { throwInvalidIp(addrStr); }

        size_t numCharsParsed = 0;
        const unsigned long ul = stoul(s, &numCharsParsed, 10);
        if (numCharsParsed == 0) { throwInvalidIp(addrStr); }
        if (ul > 255) { throwInvalidIp(addrStr); }

        ar[i] = (uint8_t)ul;
    }

    if (!tok.eof()) { throwInvalidIp(addrStr); }

    _addr = pack<uint32_t, 4>(ar);
}


IpV4Address::operator string() const {
    uint8_t ar[4];
    unpack<uint32_t, 4>(_addr, ar);

    ostringstream oss;
    for (size_t i = 0; i < 3; ++i) oss << (int)ar[i] << ".";
    oss << (int)ar[3];
    return oss.str();
}


///
/// MARK: MacAddress implementation
///

namespace {
    inline void throwInvalidMac(const string& addrStr) {
        throw invalid_argument("'" + addrStr + "' is not a valid MAC address");
    }
}

MacAddress MacAddress::_fromSockAddr(const struct sockaddr *saddr) {
    MacAddress ret;
    if (saddr) {
        assert(saddr->sa_family == AF_LINK);
        const sockaddr_dl* sdl = reinterpret_cast<const sockaddr_dl*>(saddr);
        if (sdl->sdl_alen == 6) {   // Ignore items that cannot be a IPV4 (which all have
            uint8_t ar[6];          // mac address of 6 bytes).
            memcpy(ar, LLADDR(sdl), 6);
            ret._addr = pack<uint64_t, 6>(ar);
        }
    }
    return ret;
}

MacAddress::MacAddress(const string& addrStr) {
    Tokenizer tok(addrStr, ":", false);
    uint8_t ar[6];

    for (size_t i = 0; i < 6U; ++i) {
        if (tok.eof()) { throwInvalidMac(addrStr); }

        string s;
        tok >> s;
        if (s.empty()) { throwInvalidMac(addrStr); }

        size_t numCharsParsed = 0;
        const unsigned long ul = stoul(s, &numCharsParsed, 16);
        if (numCharsParsed == 0) { throwInvalidMac(addrStr); }
        if (ul > 255) { throwInvalidMac(addrStr); }

        ar[i] = (uint8_t)ul;
    }

    if (!tok.eof()) { throwInvalidMac(addrStr); }
    
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

    // Throw an exception based on the current value of errno.
    inline void throw_system_error() {
        throw system_error(errno, system_category());
    }

    // Obtain a map from interface names to mac addresses currently available.
    typedef unordered_map<string, MacAddress> mac_addr_map_t;
    typedef pair<const struct ifaddrs*, const mac_addr_map_t*> os_impl_t;

    mac_addr_map_t getMacAddresses() {
        mac_addr_map_t ret;
        struct ifaddrs* addrs = nullptr;
        finally cleanup([&]{
            if (addrs) { freeifaddrs(addrs); }
        });

        if (getifaddrs(&addrs) == -1) { throw_system_error(); }
        for (auto cur = addrs; cur; cur = cur->ifa_next) {
            if (cur->ifa_addr != nullptr && cur->ifa_addr->sa_family == AF_LINK) {
                MacAddress ma = MacAddress::_fromSockAddr(cur->ifa_addr);
                if ((bool)ma == true) {
                    ret[cur->ifa_name] = ma;
                }
            }
        }
        return ret;
    }

    // Note that the callback in for_each_interface should return true if we should
    // continue to the next value and false if it should stop iterating. In addition
    // it is safe to perform a move from the network interface when calling this
    // function.
    void forEachInterface(function<bool(NetworkInterface&)> cb) {
        struct ifaddrs* addrs = nullptr;
        finally cleanup([&]{
            if (addrs) { freeifaddrs(addrs); }
        });

        if (getifaddrs(&addrs) == -1) { throw_system_error(); }
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
}

NetworkInterface& NetworkInterface::operator=(NetworkInterface&&) noexcept = default;
NetworkInterface& NetworkInterface::operator=(const NetworkInterface& rhs) {
    if (&rhs != this) {
        *_impl = *rhs._impl;
    }
    return *this;
}

NetworkInterface NetworkInterface::_fromOsImpl(void *osimpl) {
    NetworkInterface ret;
    os_impl_t* data = reinterpret_cast<os_impl_t*>(osimpl);

    const ifaddrs* rep = data->first;
    ret._impl->name = rep->ifa_name;
    ret._impl->flags = (unsigned)rep->ifa_flags;
    ret._impl->v4Address = IpV4Address::_fromSockAddr(rep->ifa_addr);
    ret._impl->v4NetMask = IpV4Address::_fromSockAddr(rep->ifa_netmask);
    if (ret.broadcast()) {
        ret._impl->v4Broadcast = IpV4Address::_fromSockAddr(rep->ifa_broadaddr);
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
