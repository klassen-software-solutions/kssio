//
//  utility.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include <kss/io/utility.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::net;
using namespace kss::io::stream;
using namespace kss::test;

namespace {
    template <class T>
    bool testByteOrderSequence(T hval, T nval) {
        vector<T> vec(100, hval);
        hton(vec.begin(), vec.end());
        for (const T& v : vec) {
            if (v != nval) {
                return false;
            }
        }

        ntoh(vec.begin(), vec.end());
        for (const T& v : vec) {
            if (v != hval) {
                return false;
            }
        }

        return true;
    }
}

static TestSuite ts("net::utility", {
    make_pair("byteorder", [] {
        uint16_t hu16 = 10101;
        uint16_t nu16 = hton(hu16);
        uint32_t hu32 = 1010101;
        uint32_t nu32 = hton(hu32);
        float hf = 10101.F;
        float nf = hton(hf);
        double hd = 1010101.e-2;
        double nd = hton(hd);

        KSS_ASSERT(ntoh(nu16) == hu16);
        KSS_ASSERT(ntoh(nu32) == hu32);
        KSS_ASSERT(ntoh(nf) == hf);
        KSS_ASSERT(ntoh(nd) == hd);

        if (isBigEndian()) {
            KSS_ASSERT(hu16 == nu16);
            KSS_ASSERT(hu32 == nu32);
            KSS_ASSERT(hf == nf);
            KSS_ASSERT(hd == nd);
        }
        else {
            KSS_ASSERT(hu16 == 0x2775);
            KSS_ASSERT(nu16 == 0x7527);
            KSS_ASSERT(hu32 == 0x000f69b5);
            KSS_ASSERT(nu32 == 0xb5690f00);
            KSS_ASSERT(isEqualTo<uint32_t>(0x461dd400, [&] {
                uint32_t tmp;
                memcpy(&tmp, &hf, 4);
                return tmp;
            }));
            KSS_ASSERT(isEqualTo<uint32_t>(0x00d41d46, [&] {
                uint32_t tmp;
                memcpy(&tmp, &nf, 4);
                return tmp;
            }));
            KSS_ASSERT(isEqualTo<uint64_t>(0x40c3ba8147ae147b, [&] {
                uint64_t tmp;
                memcpy(&tmp, &hd, 8);
                return tmp;
            }));
            KSS_ASSERT(isEqualTo<uint64_t>(0x7b14ae4781bac340, [&] {
                uint64_t tmp;
                memcpy(&tmp, &nd, 8);
                return tmp;
            }));
        }
    }),
    make_pair("byteorder on a sequence", [] {
        uint16_t hu16 = 10101;
        uint16_t nu16 = hton(hu16);
        uint32_t hu32 = 1010101;
        uint32_t nu32 = hton(hu32);
        float hf = 10101.F;
        float nf = hton(hf);
        double hd = 1010101.e-2;
        double nd = hton(hd);

        KSS_ASSERT(testByteOrderSequence(hu16, nu16));
        KSS_ASSERT(testByteOrderSequence(hu32, nu32));
        KSS_ASSERT(testByteOrderSequence(hf, nf));
        KSS_ASSERT(testByteOrderSequence(hd, nd));
    }),
    make_pair("byte packing & unpacking", [] {
        KSS_ASSERT(isEqualTo<uint16_t>(0, [] {
            uint8_t ar[] = { 0U, 0U };
            return pack<uint16_t>(ar);
        }));
        KSS_ASSERT(isEqualTo<uint32_t>(1678379208U, [] {
            uint8_t ar[] = { 100U, 10U, 8U, 200U };
            return pack<uint32_t>(ar);
        }));
        KSS_ASSERT(isEqualTo<uint64_t>(0x0af6b11660cd, [] {
            uint8_t ar[] = { 0x0a, 0xf6, 0xb1, 0x16, 0x60, 0xcd };
            return pack<uint64_t, 6>(ar);
        }));

        uint8_t ar[6];
        auto* ptr = unpack<uint16_t>(0, ar);
        KSS_ASSERT(ptr == ar);
        KSS_ASSERT(isTrue([&] {
            uint8_t ans[] = { 0U, 0U };
            return equal(ar, ar+2, ans);
        }));

        KSS_ASSERT(isTrue([&] {
            uint8_t ans[] = { 100U, 10U, 8U, 200U };
            unpack<uint32_t>(1678379208U, ar);
            return equal(ar, ar+4, ans);
        }));

        KSS_ASSERT(isTrue([&] {
            uint8_t ans[] = { 0x0a, 0xf6, 0xb1, 0x16, 0x60, 0xcd };
            unpack<uint64_t, 6>(0x0af6b11660cd, ar);
            return equal(ar, ar+6, ans);
        }));

    }),
    make_pair("mime types", [] {
        int i = 0;
        string s;
        KSS_ASSERT(guessMimeType(i) == "text/plain");
        KSS_ASSERT(guessMimeType<long>() == "text/plain");
        KSS_ASSERT(guessMimeType(s) == "text/plain");
    })
});

static TestSuite ts1("stream::utility", {
    make_pair("capture", [] {
        ostringstream strm;
        auto s = capture(strm, [&]{
            strm << "hello world!" << endl;
        });
        KSS_ASSERT(s == "hello world!\n");
    })
});
