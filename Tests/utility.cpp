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
#include <iostream>
#include <vector>

#include <kss/io/utility.hpp>

#include "ksstest.hpp"

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
    make_pair("byteorder", [](TestSuite&) {
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
            KSS_ASSERT(*((uint32_t*)&hf) == 0x461dd400);
            KSS_ASSERT(*((uint32_t*)&nf) == 0x00d41d46);
            KSS_ASSERT(*((uint64_t*)&hd) == 0x40c3ba8147ae147b);
            KSS_ASSERT(*((uint64_t*)&nd) == 0x7b14ae4781bac340);
        }
    }),
    make_pair("byteorder on a sequence", [](TestSuite&) {
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
    make_pair("byte packing & unpacking", [](TestSuite&) {
        KSS_ASSERT((pack<uint16_t, 2>((uint8_t[]) { 0U, 0U })) == 0U);
        KSS_ASSERT((pack<uint32_t, 4>((uint8_t[]) { 100U, 10U, 8U, 200U })) == 1678379208U);
        KSS_ASSERT((pack<uint64_t, 6>((uint8_t[]) { 0x0a, 0xf6, 0xb1, 0x16, 0x60, 0xcd })) == 0x0af6b11660cd);

        uint8_t ar[6];
        KSS_ASSERT((unpack<uint16_t, 2>(0, ar)) == ar);
        KSS_ASSERT(equal(ar, ar+2, (uint8_t[]) { 0U, 0U }));

        unpack<uint32_t, 4>(1678379208U, ar);
        KSS_ASSERT(equal(ar, ar+4, (uint8_t[]) { 100U, 10U, 8U, 200U }));

        unpack<uint64_t, 6>(0x0af6b11660cd, ar);
        KSS_ASSERT(equal(ar, ar+6, (uint8_t[]) { 0x0a, 0xf6, 0xb1, 0x16, 0x60, 0xcd }));
    }),
    make_pair("mime types", [](TestSuite&) {
        int i = 0;
        string s;
        KSS_ASSERT(guessMimeType(i) == "text/plain");
        KSS_ASSERT(guessMimeType<long>() == "text/plain");
        KSS_ASSERT(guessMimeType(s) == "text/plain");
    })
});

static TestSuite ts1("stream::utility", {
    make_pair("capture", [](TestSuite&) {
        auto s = capture(cout, []{
            cout << "hello world!" << endl;
        });
        KSS_ASSERT(s == "hello world!\n");

        string errS;
        s = capture(cout, [&]{
            errS = capture(cerr, []{
                cout << "this is a test" << endl;
                cerr << "something wrong?";
                cout << "done!";
            });
        });
        KSS_ASSERT(s == "this is a test\ndone!");
        KSS_ASSERT(errS == "something wrong?");

    })
});
