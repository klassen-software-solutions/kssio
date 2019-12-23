//
//  eai_error_category.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-08-21.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <iostream>
#include <netdb.h>
#include <kss/io/eai_error_category.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::net;
using namespace kss::test;


static TestSuite ts("net::eai_error_category", {
    make_pair("eaiErrorCategory", [] {
        const error_category& cat = eaiErrorCategory();
        KSS_ASSERT(cat.name() == string("eai"));
        KSS_ASSERT(!cat.message(EAI_FAIL).empty());
        KSS_ASSERT(!cat.message(-1).empty());
    }),
    make_pair("eaiErrorCode", [] {
        error_code ec = eaiErrorCode(EAI_FAMILY);
        KSS_ASSERT(ec.category() == eaiErrorCategory());
        KSS_ASSERT(ec.value() == EAI_FAMILY);

        errno = ENOMEM;
        ec = eaiErrorCode(EAI_SYSTEM);
        KSS_ASSERT(ec.category() == system_category());
        KSS_ASSERT(ec.value() == ENOMEM);
    })
});
