//
//  interface.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2016-09-28.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <string>
#include <stdexcept>

#include <kss/io/interface.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::net;
using namespace kss::test;


static TestSuite ts("net::interface", {
    make_pair("IpV4Address", [] {
        IpV4Address a1;
        KSS_ASSERT((bool)a1 == false);
        KSS_ASSERT((string)a1 == "0.0.0.0");

        IpV4Address a2("100.10.10.3");
        KSS_ASSERT((bool)a2 == true);
        KSS_ASSERT((string)a2 == "100.10.10.3");

        KSS_ASSERT(throwsException<invalid_argument>( []{ IpV4Address a("invalid"); }));
        KSS_ASSERT(throwsException<invalid_argument>( []{ IpV4Address a("bad.10.10.3"); }));
        KSS_ASSERT(throwsException<invalid_argument>( []{ IpV4Address a("100.bad.10.3"); }));
        KSS_ASSERT(throwsException<invalid_argument>( []{ IpV4Address a("100.10.bad.3"); }));
        KSS_ASSERT(throwsException<invalid_argument>( []{ IpV4Address a("100.10.10.bad"); }));

        IpV4Address a3("100.10.8.200");
        IpV4Address a4("100.10.8.200");
        KSS_ASSERT(a3 != a1);
        KSS_ASSERT(a3 == a4);
        KSS_ASSERT(a3 < a2);
        KSS_ASSERT(a3 <= a2 && a3 <= a4);
        KSS_ASSERT(a2 > a3);
        KSS_ASSERT(a2 >= a3);

        a4 = a2;
        KSS_ASSERT(a2 == a4);
        KSS_ASSERT(a2 >= a4);
    }),
    make_pair("MacAddress", [] {
        MacAddress a1;
        KSS_ASSERT((bool)a1 == false);
        KSS_ASSERT((string)a1 == "00:00:00:00:00:00");

        MacAddress a2("0a:f6:b1:16:60:cd");
        KSS_ASSERT((bool)a2 == true);
        KSS_ASSERT((string)a2 == "0a:f6:b1:16:60:cd");

        KSS_ASSERT(throwsException<invalid_argument>([] { MacAddress a("invalid"); }));
        KSS_ASSERT(throwsException<invalid_argument>([] { MacAddress a("bad:f6:b1:16:60:cd"); }));
        KSS_ASSERT(throwsException<invalid_argument>([] { MacAddress a("0a:f6:b1:bad:60:cd"); }));

        MacAddress a3("0a:1c:42:00:00:09");
        MacAddress a4("0a:1c:42:00:00:09");
        KSS_ASSERT(a3 != a1);
        KSS_ASSERT(a3 == a4);
        KSS_ASSERT(a3 < a2);
        KSS_ASSERT(a3 <= a2 && a3 <= a4);
        KSS_ASSERT(a2 > a3);
        KSS_ASSERT(a2 >= a3);

        a4 = a2;
        KSS_ASSERT(a2 == a4);
        KSS_ASSERT(a2 >= a4);
    }),
    make_pair("NetworkInterface", [] {
        // Find the loopback interface.
        auto ni = findInterface("lo");
        if (!ni) { ni = findInterface("lo0"); }
        KSS_ASSERT(bool(ni) == true);
        KSS_ASSERT(string(ni.v4Address()) == "127.0.0.1");
        KSS_ASSERT(string(ni.v4NetMask()) == "255.0.0.0");
        KSS_ASSERT(!ni.v4Broadcast());
        KSS_ASSERT(!ni.hwAddress());
        KSS_ASSERT(ni.up());
        KSS_ASSERT(ni.loopback());
        KSS_ASSERT(ni.running());
        KSS_ASSERT(!ni.broadcast());

        // Find an active, non-loopback interface.
        ni = findActiveInterface();
        KSS_ASSERT(bool(ni) == true);
        KSS_ASSERT(bool(ni.v4Address()) == true);
        KSS_ASSERT(bool(ni.v4NetMask()) == true);
        KSS_ASSERT(bool(ni.v4Broadcast()) == true);
        KSS_ASSERT(bool(ni.hwAddress()) == true);
        KSS_ASSERT(ni.up());
        KSS_ASSERT(!ni.loopback());
        KSS_ASSERT(ni.running());
        KSS_ASSERT(ni.broadcast());

        // Find the mac address of an active, non-loopback interface.
        MacAddress ma = findMacAddress();
        KSS_ASSERT(bool(ma) == true);

        // Find all interface names.
        auto names = findAllInterfaceNames();
        KSS_ASSERT(names.size() >= 2);

        // Find all interfaces.
        auto interfaces = findAllInterfaces();
        KSS_ASSERT(interfaces.size() == names.size());
    })
});
