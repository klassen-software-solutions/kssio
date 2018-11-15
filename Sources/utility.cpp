//
//  utility.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-04-11.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstddef>
#include <cstring>

#include "utility.hpp"

using namespace std;
using namespace kss::io::net;


// MARK: namespace kss::io::net

// This is a simple, compile-time means of determining the endianness of our architecture.
// At least I think the compiler should optimize this down to either 1 or 0.
static const uint16_t endian_i = 1;
#define IS_BIGENDIAN() ((*(char*)&endian_i) == 0)


bool kss::io::net::isBigEndian() noexcept {
    return IS_BIGENDIAN();
}


// The conversion for a float is done by making use of what seems to be a fairly
// safe assumption, namely that IEEE 754 4-byte floating point values get bytes swapped
// in the same order as a 4-byte integer. Note that the memcpy calls are needed as
// merely casting the types may reinterpret the bytes.
float kss::io::net::htonf(float hostfloat) noexcept {
    if (IS_BIGENDIAN()) {
        return hostfloat;
    }
    else {
        uint32_t u = 0;
        float f = 0;

        static_assert(sizeof(float) == sizeof(uint32_t),
                      "float and uint32_t must be the same size");
        memcpy(&u, &hostfloat, sizeof(uint32_t));
        u = htonl(u);
        memcpy(&f, &u, sizeof(uint32_t));
        return f;
    }
}

float kss::io::net::ntohf(float netfloat) noexcept {
    if (IS_BIGENDIAN()) {
        return netfloat;
    }
    else {
        uint32_t u = 0;
        float f = 0;

        assert(sizeof(float) == sizeof(uint32_t));
        memcpy(&u, &netfloat, sizeof(uint32_t));
        u = ntohl(u);
        memcpy(&f, &u, sizeof(uint32_t));
        return f;
    }
}



// The conversion for a double relies on a double swap of two uint32_t values. I no longer
// recall where I first obtain this code, but it was published on the internet at one
// point.
double kss::io::net::htond(double hostdouble) noexcept {
    if (IS_BIGENDIAN()) {
        return hostdouble;
    }
    else {
        static_assert(sizeof(double) == (sizeof(uint32_t)*2), "A double must be 8 bytes.");

        uint32_t u1 = 0;
        uint32_t u2 = 0;
        double d = 0;

        memcpy(&u1, &hostdouble, sizeof(uint32_t));
        memcpy(&u2, ((uint8_t*)&hostdouble)+sizeof(uint32_t), sizeof(uint32_t));
        u1 = ntohl(u1);
        u2 = ntohl(u2);
        memcpy(&d, &u2, sizeof(uint32_t));
        memcpy(((uint8_t*)&d)+sizeof(uint32_t), &u1, sizeof(uint32_t));
        return d;
    }
}

double kss::io::net::ntohd(double netdouble) noexcept {
    if (IS_BIGENDIAN()) {
        return netdouble;
    }
    else {
        static_assert(sizeof(double) == (sizeof(uint32_t)*2), "A double must be 8 bytes.");

        uint32_t u1 = 0;
        uint32_t u2 = 0;
        double d = 0;

        memcpy(&u1, &netdouble, sizeof(uint32_t));
        memcpy(&u2, ((uint8_t*)&netdouble)+sizeof(uint32_t), sizeof(uint32_t));
        u1 = htonl(u1);
        u2 = htonl(u2);
        memcpy(&d, &u2, sizeof(uint32_t));
        memcpy(((uint8_t*)&d)+sizeof(uint32_t), &u1, sizeof(uint32_t));
        return d;
    }
}
