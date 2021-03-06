//
//  version.cpp
//  unittest
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/io/version.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("version", {
    make_pair("version", [] {
        KSS_ASSERT(!kss::io::version().empty());
        KSS_ASSERT(!kss::io::license().empty());
    })
});
