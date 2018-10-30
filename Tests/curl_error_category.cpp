//
//  curl_error_category.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-10-30.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <curl/curl.h>
#include <kss/io/curl_error_category.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::io::net;
using namespace kss::test;


static TestSuite ts("net::curl_error_category", {
    make_pair("status codes", [](TestSuite&) {
        const auto& cat = curlErrorCategory();
        KSS_ASSERT(string(cat.name()) == "curl");

        const error_code ec(CURLE_URL_MALFORMAT, cat);
        KSS_ASSERT(ec.category() == cat);
        KSS_ASSERT(ec.value() == CURLE_URL_MALFORMAT);
        KSS_ASSERT(!ec.message().empty());
    })
});
