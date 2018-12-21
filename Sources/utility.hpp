//
//  utility.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-04-11.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_net_utility_hpp
#define kssio_net_utility_hpp

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

#include <arpa/inet.h>

namespace kss {
    namespace io {

        namespace _private {
            // This is not part of the public API. Don't use anything in this namespace
            // directly.

            // Use these macros if a condition fails in a header file. (In a cpp file
            // use the contract API.)
#           define _KSSIO_PRECONDITIONS_FAILED { std::cerr << "preconditions failed " << __FILE__ << ", " << __LINE__ << std::endl; std::terminate(); }
#           define _KSSIO_POSTCONDITIONS_FAILED { std::cerr << "postconditions failed " << __FILE__ << ", " << __LINE__ << std::endl; std::terminate(); }


            /*!
             By subclassing your type T from add_rel_ops<T> this will add the "missing" operators
             provided that T defines operator== and operator<.

             This is similar to "using namespace rel_ops;" but does not have the potential
             side effects of adding a namespace.

             This is "borrowed" from kssutil.
             */
            template <class T>
            struct AddRelOps {
                inline bool operator!=(const T& t) const noexcept {
                    const T* self = static_cast<const T*>(this);
                    return !(*self == t);
                }

                inline bool operator<=(const T& t) const noexcept {
                    const T* self = static_cast<const T*>(this);
                    return (*self < t || *self == t);
                }

                inline bool operator>(const T& t) const noexcept {
                    const T* self = static_cast<const T*>(this);
                    return (!(*self == t) && !(*self < t));
                }

                inline bool operator>=(const T& t) const noexcept {
                    const T* self = static_cast<const T*>(this);
                    return !(*self < t);
                }
            };

            /*!
             A checked version of duration_cast. Note that this will be less efficient
             than std::chrono::duration_cast. In particular it is not a constexpr plus
             it needs to perform checks before the cast can be made.

             This is based on code found at
             https://stackoverflow.com/questions/44635941/how-to-check-for-overflow-in-duration-cast?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

             "borrowed" from kssutil

             @throws std::overflow_error if the cast would not be representable in the target duration.
             */
            template <class ToDuration, class Rep, class Period>
            ToDuration checked_duration_cast(const std::chrono::duration<Rep, Period>& dtn) {
                using namespace std::chrono;
                using S = duration<long double, typename ToDuration::period>;
                constexpr S minimumAllowed = ToDuration::min();
                constexpr S maximumAllowed = ToDuration::max();
                const S s = dtn;
                if (s < minimumAllowed || s > maximumAllowed) {
                    throw std::overflow_error("checked_duration_cast");
                }
                return duration_cast<ToDuration>(s);
            }


            /*!
             Returns a time_point constructed to contain the current time as defined by
             the time point's clock.
             @throws kss::util::time::checked_duration_cast if the duration value for
             TimePoint::clock::time_point::duration cannot be represented as a duration
             value for TimePoint::duration.

             "borrowed" from kssutil
             */
            template <class TimePoint>
            inline TimePoint now(const TimePoint& = TimePoint()) {
                using Clock = typename TimePoint::clock;
                using Duration = typename TimePoint::duration;
                const auto dur = Clock::now().time_since_epoch();
                return TimePoint(checked_duration_cast<Duration>(dur));
            }

        }


        /*!
         Thrown by operations that attempt to read something past its end. This is intended
         to be used to handle cases where we cannot efficiently determine the end until we
         attempt the read and allow it to fail. For example, the input_iterator class
         will trap this exception if thrown by the operator>>() method in order to handle
         the cases were eof() cannot be determined until operator>>() is called.
         */
        class Eof : public std::exception {
        public:
            virtual const char* what() const noexcept { return "eof"; }
        };

        /*!
         Thrown by classes when they are called in methods not allowed in their current state.
         A typical example is the kss::iterators classes which throw this if you reference
         an iterator that is not currently dereferenceable (e.g. because it == end() for
         example.
         */
        class InvalidState : public std::logic_error {
        public:
            explicit InvalidState(const char* what_arg) : std::logic_error(what_arg) {}
            explicit InvalidState(const std::string& what_arg) : std::logic_error(what_arg) {}
        };

        /*!
         Thrown by operations that fail to properly parse input from file or stream
         based reading.
         */
        class ParsingError : public std::runtime_error {
        public:
            explicit ParsingError(const char* what_arg) : std::runtime_error(what_arg) {}
            explicit ParsingError(const std::string& what_arg) : std::runtime_error(what_arg) {}
        };


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
             In-place conversion of a series of values to/from network and host byte ordering.
             */
            template <class InputIterator>
            void hton(InputIterator first, InputIterator last) {
                if (!isBigEndian()) {
                    while (first != last) {
                        *first = hton(*first);
                        ++first;
                    }
                }
            }

            template <class InputIterator>
            void ntoh(InputIterator first, InputIterator last) {
                if (!isBigEndian()) {
                    while (first != last) {
                        *first = ntoh(*first);
                        ++first;
                    }
                }
            }


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
            std::string guessMimeType(const T& t = T()) {
                return "text/plain";
            }
        }

        namespace stream {

            /*!
             Temporarily redirect and stream and "capture" it in a string. On exit the stream
             is restored again. This is typically used to capture the output of cout or cerr
             during unit tests.

             This method is based on examples and discussions found at
             https://stackoverflow.com/questions/1881589/redirected-cout-stdstringstream-not-seeing-eol

             @return the captured string data
             @throws anything that may be thrown by the stream libraries.
             */
            std::string capture(std::ostream& os, const std::function<void()>& fn);

        }
    }
}

#endif
