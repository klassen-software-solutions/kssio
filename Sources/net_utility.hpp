//
//  net_utility.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-04-11.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_net_utility_hpp
#define kssio_net_utility_hpp

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>

#include <arpa/inet.h>

namespace kss {
    namespace io {
        namespace net {

            /*!
             Return true if this hardware is big endian (also equivalent to network byte
             order).
             */
            bool isBigEndian() noexcept;

            /*!
             Convert single precision floating point values. We assume that float conforms
             to the IEEE 754 binary32 standard.
             */
            float htonf(float hostfloat) noexcept;
            float ntohf(float netfloat) noexcept;;

            /*!
             Convert double precision floating point values. We assume that double conforms
             to the IEEE 754 binary64 standard.
             */
            double htond(double hostdouble) noexcept;
            double ntohd(double netdouble) noexcept;

            /*!
             C++ wrappers around the specific functions/macros for byte order conversion.
             */
            inline uint16_t hton(uint16_t hostshort) noexcept   { return htons(hostshort); }
            inline uint32_t hton(uint32_t hostlong) noexcept    { return htonl(hostlong); }
            inline float    hton(float hostfloat) noexcept      { return htonf(hostfloat); }
            inline double   hton(double hostdouble) noexcept    { return htond(hostdouble); }

            inline uint16_t ntoh(uint16_t netshort) noexcept    { return ntohs(netshort); }
            inline uint32_t ntoh(uint32_t netlong) noexcept     { return ntohl(netlong); }
            inline float    ntoh(float netfloat) noexcept       { return ntohf(netfloat); }
            inline double   ntoh(double netdouble) noexcept     { return ntohd(netdouble); }

            /*!
             Byte packing. Given an array of N bytes, pack it into a single unsigned integer
             of type UInt. They are packed such that byte 0 in the array is the most
             significantbyte and byte N-1 is the least significant.

             Note that UInt must be large enough to hold N bytes. In debug mode this is
             checked via an assertion. In release mode the result is undefined.

             @param ar the array to be packed
             @return the packed value
             */
            template <class UInt, size_t N>
            UInt pack(const uint8_t ar[N]) noexcept {
                static_assert(N <= sizeof(UInt), "UInt must contain at least N bytes");

                UInt result = ar[N-1];
                for (size_t i = 0; i < N-1; ++i) {
                    result += ((UInt)ar[i]) << ((N-i-1)*8);
                }
                return result;
            }

            /*!
             Byte unpacking. Given an unsigned integer value of type UInt unpack the N
             least significant bytes into an array of N bytes. (Any bytes more significant
             than N should be 0.) They are unpacked such that byte 0 in the array will
             contain the most significant representable byte and byte N-1 will be the least
             significant.

             Note that N must be large enough to store the full value. (I.e. Although
             UInt may be capable of storing more than N bytes, the most significant bytes
             over N should be 0.) In debug mode this is checked via an assertion. In release
             mode the result is undefined.

             In addition UInt must be large enough to handle N bytes. In debug mode this
             will be checked via an assertion but in release mode the result is undefined.

             @param value The input value.
             @param ar The output array.
             @return a reference to ar.
             */
            template <class UInt, size_t N>
            uint8_t* unpack(UInt value, uint8_t ar[N]) noexcept {
                static_assert(N <= sizeof(UInt), "UInt must contain at least N bytes");
                assert(value <= (std::numeric_limits<UInt>::max() >> ((sizeof(UInt)-N)*8)));

                for (size_t i = 0; i < N; ++i) {
                    size_t shift = (N-i-1)*8U;
                    ar[i] = (uint8_t)((value & (UInt(0xFF) << shift)) >> shift);
                }

                return ar;
            }

            /*!
             Guess the mime type for a given object. This will return text/plain for all
             types T unless they are specialized to return something else.
             */
            template <class T>
            std::string guess_mime_type(const T& t = T()) {
                return "text/plain";
            }
        }
    }
}

#endif
