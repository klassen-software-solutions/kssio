//
//  http_error_category.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2015-02-08.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/io/http_error_category.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::io::net;
using namespace kss::test;


static TestSuite ts("net::http_error_category", {
    make_pair("status codes", [](TestSuite&) {
        const auto& cat = httpErrorCategory();
        KSS_ASSERT(string(cat.name()) == "http");

        const error_code ec = make_error_code(HttpStatusCode::NoContent);
        KSS_ASSERT(ec.category() == cat);
        KSS_ASSERT(ec.value() == static_cast<int>(HttpStatusCode::NoContent));
    })
});
