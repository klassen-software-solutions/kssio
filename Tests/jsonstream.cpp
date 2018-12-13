//
//  jsonstream.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2016-11-05.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <sstream>
#include <kss/io/jsonstream.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::io::stream::json;
using namespace kss::test;

namespace {
    const char* test0 = R"({ "key": "value" })";

    const char* test1 = R"([ "Mark McGwire", "Sammy Sosa", "Ken Griffey" ])";

    const char* test2 = R"(
    {
        "american": [ "Boston Red Sox", "Detroit Tigers", "New York Yankees" ],
        "national": [ "New York Mets", "Chicago Cubs", "Atlanta Braves" ]
    }
    )";

    const char* test3 = R"JSON(
        { "time": "20:03:20", "player": "Sammy Sosa", "action": "strike (miss)" }
        { "time": "20:03:47", "player": "Sammy Sosa", "action": "grand slam" }
    )JSON";

    const char* test4 = R"(
        ["Mark McGwire", "Sammy Sosa", "Ken Griffey"]
        ["Chicago Cubs", "St Louis Cardinals"]
    )";

    const char* test5 = R"(
    {
        "invoice": 34843,
        "date": "2001-01-23",
        "bill-to": {
            "given": "Chris",
            "family": "Dumars",
            "address": {
                "lines": [
                    "458 Walkman Dr.",
                    "Suite #292"
                    ],
                "city": "Royal Oak",
                "state": "MI",
                "postal": "48046"
            }
        },
        "ship-to": "--same as bill to--",
        "product": [{"sku": "BL394D", "quantity": 4, "description": "Basketball", "price": 450.00},
                    {"sku": "BL4438H", "quantity": 1, "description": "Super Hoop", "price": 2392.00}],
        "tax": 251.42,
        "total": 4443.52,
        "comments": ["Late afternoon is best",
                     "Backup contact is Nancy",
                     "Billsmer @ 338-4338"]
    }
    )";


    unsigned count_documents(const string& testname, const string& s) {
        Document d;
        unsigned numDocsRead = 0;
        istringstream ss(s);
        while (!ss.eof() && !ss.fail()) {
            ss >> d;
            if (!ss.fail()) {
                ++numDocsRead;
            }
        }
        return numDocsRead;
    }
}

static TestSuite ts("stream::json::jsonstream", {
    make_pair("construction, destruction and copying", [](TestSuite&) {
        Document d(test0);
        KSS_ASSERT(!d.empty());

        istringstream ss(test1);
        ss >> d;
        KSS_ASSERT(!d.empty());
        KSS_ASSERT(d == d);

        Document d2 = d;
        KSS_ASSERT(d2 == d);
        d2.clear();
        KSS_ASSERT(d2.empty() && !d.empty());
        KSS_ASSERT(d2 != d);

        d2 = d;
        d2.ensureUnique();
        KSS_ASSERT(d2 == d);

        ss.clear();
        ss.seekg(0);
        ss >> d2;
        KSS_ASSERT(!d2.empty());
        KSS_ASSERT(d2 == d);

        KSS_ASSERT(count_documents("test1", test1) == 1);
        KSS_ASSERT(count_documents("test2", test2) == 1);
        KSS_ASSERT(count_documents("test3", test3) == 2);
        KSS_ASSERT(count_documents("test4", test4) == 2);
        KSS_ASSERT(count_documents("test5", test5) == 1);

        KSS_ASSERT(Node::none.empty());
        KSS_ASSERT(!Node::none.isScalar());
        KSS_ASSERT(!Node::none.isArray());
        KSS_ASSERT(!Node::none.isObject());
    }),
    make_pair("mime type", [](TestSuite&) {
        KSS_ASSERT(kss::io::net::guessMimeType<Document>() == "application/json");
        KSS_ASSERT(kss::io::net::guessMimeType(Document()) == "application/json");
    })
});
