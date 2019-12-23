//
//  interface.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2016-09-28.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_interface_hpp
#define kssio_interface_hpp

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <net/if.h>
#include <kss/util/all.h>

#include "utility.hpp"


namespace kss {
    namespace io {
        namespace net {

            /*!
             Class to hold an description of an IPV4 address.
             */
            class IpV4Address : public kss::util::AddRelOps<IpV4Address> {
            public:
                /*!
                 Construct an address.
                 @throws invalid_argument if the string version cannot be parsed into an IPV4 address.
                 */
                explicit IpV4Address(uint32_t addr = 0) : _addr(addr) {}
                explicit IpV4Address(const std::string& addrStr);
                IpV4Address(const IpV4Address&) = default;
                IpV4Address(IpV4Address&&) = default;
                ~IpV4Address() = default;

                IpV4Address& operator=(const IpV4Address&) = default;
                IpV4Address& operator=(IpV4Address&&) = default;

                /*!
                 Assignment/copy.
                 @throws invalid_argument if the string version cannot be parsed into an IPV4 address.
                 @return a reference to this
                 */
                IpV4Address& operator=(const std::string& addrStr) {
                    return operator=(IpV4Address(addrStr));
                }

                /*!
                 Allowable casts. Note that the bool cast will return true if the address
                 is all zeros. The string cast will return a string in the expected "dot"
                 format.
                 */
                explicit operator bool() const noexcept { return (_addr != 0U); }
                explicit operator std::string() const;

                /*!
                 Returns a hash value for the address.
                 */
                size_t hash() const noexcept { return std::hash<uint32_t>()(_addr); }

                /*!
                 Comparators. Two addresses are equal if all four numbers are the same. The
                 ordering is defined by a lexicographical comparison of the four numbers.
                 The remaining four operators are defined by add_rel_ops.
                 */
                bool operator==(const IpV4Address& rhs) const noexcept {
                    return (_addr == rhs._addr);
                }
                bool operator<(const IpV4Address& rhs) const noexcept {
                    return (_addr < rhs._addr);
                }

            private:
                uint32_t _addr = 0;
            };


            /*!
             Class to hold and description a MAC (HW) address.
             */
            class MacAddress : public kss::util::AddRelOps<MacAddress> {
            public:
                /*!
                 Construct an address from the C structure interface or from a string
                 description.
                 @throws invalid_argument if the string version cannot be parsed into a MAC address.
                 */
                explicit MacAddress(uint64_t addr = 0U) : _addr(addr) {}
                explicit MacAddress(const std::string& addrStr);
                MacAddress(const MacAddress&) = default;
                MacAddress(MacAddress&&) = default;
                ~MacAddress() = default;

                /*!
                 Assignment/copy.
                 @throws invalid_argument if the string version cannot be parsed into a MAC addres.
                 */
                MacAddress& operator=(const MacAddress&) = default;
                MacAddress& operator=(MacAddress&&) = default;
                MacAddress& operator=(const std::string& addrStr) {
                    return operator=(MacAddress(addrStr));
                }

                /*!
                 Allowable casts. Note that the bool cast will return true if the address
                 is all zeros. The string cast will return a string in the lowercase hex
                 digits, separated by ":" characters format.
                 */
                explicit operator bool() const noexcept { return (_addr != 0U); }
                explicit operator std::string() const;

                /*!
                 Returns a hash value for the address.
                 */
                size_t hash() const noexcept { return std::hash<uint64_t>()(_addr); }

                /*!
                 Comparators. Two addresses are equal if all four numbers are the same. The
                 ordering is defined by a lexicographical comparison of the four numbers.
                 */
                bool operator==(const MacAddress& rhs) const noexcept {
                    return (_addr == rhs._addr);
                }
                bool operator<(const MacAddress& rhs) const noexcept {
                    return (_addr < rhs._addr);
                }

            private:
                uint64_t _addr = 0;
            };


            /*!
             Class to describe a network interface. Note that at present this only considers
             IPV4 interfaces.
             */
            class NetworkInterface {
            public:
                /*!
                 Construct a network interface description.
                 */
                NetworkInterface();
                NetworkInterface(const NetworkInterface& ni);
                NetworkInterface(NetworkInterface&& ni);
                ~NetworkInterface() noexcept;

                /*!
                 Assignment/copy.
                 */
                NetworkInterface& operator=(const NetworkInterface& rhs);
                NetworkInterface& operator=(NetworkInterface&& ni) noexcept;

                /*!
                 A network interface cast to bool will be true if it contains a name
                 and false otherwise.
                 */
                operator bool() const noexcept { return !name().empty(); }

                /*!
                 Accessors. Note that the meaning of the bits in flag() are the same as those
                 found in the ifreq.ifr_flags item. There are convenience methods defined
                 for the most common items, but you may need to check for flags manually
                 to handles flags not found in all POSIX architectures.
                 */
                const std::string& name() const noexcept;
                unsigned flags() const noexcept;
                const IpV4Address& v4Address() const noexcept;
                const IpV4Address& v4NetMask() const noexcept;
                const IpV4Address& v4Broadcast() const noexcept;
                const MacAddress& hwAddress() const noexcept;

                /*!
                 Convenience accessors for the flags common to all POSIX systems. For other
                 flags for a given hardware you will need to examine the bits in flags()
                 manually.
                 */
                bool up() const noexcept        { return (bool)(flags() & IFF_UP); }
                bool loopback() const noexcept  { return (bool)(flags() & IFF_LOOPBACK); }
                bool running() const noexcept   { return (bool)(flags() & IFF_RUNNING); }
                bool broadcast() const noexcept { return (bool)(flags() & IFF_BROADCAST); }
                bool multicast() const noexcept { return (bool)(flags() & IFF_MULTICAST); }

                // Construct from the underlying OS implementation. This should never
                // be called manually and should be considered a private method.
                static NetworkInterface _fromOsImpl(void* osimpl);

            private:
                struct Impl;
                std::unique_ptr<Impl> _impl;
            };


            /*!
             Look for the network interface of the given name and return it.
             @return the network interface. If no interface for this name is found, then
             an empty interface (will be false when cast to a bool) will be returned.
             @throws system_error if one of the underlying system C calls fails.
             */
            NetworkInterface findInterface(const std::string& name);

            /*!
             Look for the first non-loopback, active interface and return it.
             @return the network interface. If none was found an empty interface will be returned.
             */
            NetworkInterface findActiveInterface();

            /*!
             Return the MAC address of the first active, non-loopback, network interface
             on this system. Returns an empty address if there are no active, non-loopback
             interfaces.

             @throws system_error if one of the underlying system C calls fails.
             */
            MacAddress findMacAddress();

            /*!
             Return a vector of all the network interface names found on this hardware.
             Note that while it likely isn't going to happen in practice, it is at least
             in theory possible for this to return an empty vector.

             @throws system_error if one of the underlying system C calls fails.
             */
            std::vector<std::string> findAllInterfaceNames();

            /*!
             Return a vector of all network interfaces found on this hardware.
             Note that while it likely isn't going to happen in practice, it is at least
             in theory possible for this to return an empty vector.

             @throws system_error if one of the underlying system C calls fails.
             */
            std::vector<NetworkInterface> findAllInterfaces();
        }
    }
}


/*!
 Specialize hashes for the addresses to allow them to be used in unordered_map or
 unordered_set containers.
 */
namespace std {
    template <>
    class hash<kss::io::net::IpV4Address> {
    public:
        size_t operator()(const kss::io::net::IpV4Address& addr) noexcept {
            return addr.hash();
        }
    };

    template <> class hash<kss::io::net::MacAddress> {
    public:
        size_t operator()(const kss::io::net::MacAddress& addr) noexcept { return addr.hash(); }
    };
}

#endif 
