//
//  yamlstream.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2014-05-04.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <sstream>
#include <string>

#include <kss/io/yamlstream.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::io::stream::yaml;
using namespace kss::test;


const char* test1 = R"(
- Mark McGwire
- Sammy Sosa
- Ken Griffey
)";

const char* test1_canonical = R"(---
!!seq [
  !!str "Mark McGwire",
  !!str "Sammy Sosa",
  !!str "Ken Griffey",
]
)";

const char* test1_pretty = R"(- Mark McGwire
- Sammy Sosa
- Ken Griffey
)";

const char* test1_flow = "[Mark McGwire, Sammy Sosa, Ken Griffey]\n";


const char* test2 = R"(
american:
 - Boston Red Sox
 - Detroit Tigers
 - New York Yankees
national:
 - New York Mets
 - Chicago Cubs
 - Atlanta Braves
)";

const char* test2_canonical = R"(---
!!map {
  ? !!str "american"
  : !!seq [
    !!str "Boston Red Sox",
    !!str "Detroit Tigers",
    !!str "New York Yankees",
  ],
  ? !!str "national"
  : !!seq [
    !!str "New York Mets",
    !!str "Chicago Cubs",
    !!str "Atlanta Braves",
  ],
}
)";

const char* test2_pretty = R"(american:
- Boston Red Sox
- Detroit Tigers
- New York Yankees
national:
- New York Mets
- Chicago Cubs
- Atlanta Braves
)";

const char* test2_flow = "{american: [Boston Red Sox, Detroit Tigers, New York Yankees], national: [New York Mets, Chicago Cubs, Atlanta Braves]}\n";


const char* test3 = R"(
---
time: 20:03:20
player: Sammy Sosa
action: strike (miss)
...
---
time: 20:03:47
player: Sammy Sosa
action: grand slam
...
)";

const char* test3_canonical = R"X(---
!!map {
  ? !!str "time"
  : !!str "20:03:20",
  ? !!str "player"
  : !!str "Sammy Sosa",
  ? !!str "action"
  : !!str "strike (miss)",
}
)X";

const char* test3_pretty = R"(time: 20:03:20
player: Sammy Sosa
action: strike (miss)
)";

const char* test3_flow = "{time: '20:03:20', player: Sammy Sosa, action: strike (miss)}\n";


const char* test4 = R"(
# Ranking of 1998 home runs\n"
---
- Mark McGwire
- Sammy Sosa
- Ken Griffey

# Team ranking
---
- Chicago Cubs
- St Louis Cardinals

)";


const char* test5 = R"(
---
!<tag:clarkevans.com,2002:invoice>
invoice: 34843
date   : 2001-01-23
bill-to: &id001
  given  : Chris
  family : Dumars
  address:
    lines: |
      458 Walkman Dr.
      Suite #292
    city    : Royal Oak
    state   : MI
    postal  : 48046
ship-to: *id001
product:
  - sku         : BL394D
    quantity    : 4
    description : Basketball
    price       : 450.00
  - sku         : BL4438H
    quantity    : 1
    description : Super Hoop
    price       : 2392.00
tax  : 251.42
total: 4443.52
comments:
  Late afternoon is best.
  Backup contact is Nancy
  Billsmer @ 338-4338.
)";

const char* test5_canonical = R"(---
!<tag:clarkevans.com,2002:invoice> {
  ? !!str "invoice"
  : !!str "34843",
  ? !!str "date"
  : !!str "2001-01-23",
  ? !!str "bill-to"
  : &id001 !!map {
    ? !!str "given"
    : !!str "Chris",
    ? !!str "family"
    : !!str "Dumars",
    ? !!str "address"
    : !!map {
      ? !!str "lines"
      : !!str "458 Walkman Dr.\nSuite #292\n",
      ? !!str "city"
      : !!str "Royal Oak",
      ? !!str "state"
      : !!str "MI",
      ? !!str "postal"
      : !!str "48046",
    },
  },
  ? !!str "ship-to"
  : *id001,
  ? !!str "product"
  : !!seq [
    !!map {
      ? !!str "sku"
      : !!str "BL394D",
      ? !!str "quantity"
      : !!str "4",
      ? !!str "description"
      : !!str "Basketball",
      ? !!str "price"
      : !!str "450.00",
    },
    !!map {
      ? !!str "sku"
      : !!str "BL4438H",
      ? !!str "quantity"
      : !!str "1",
      ? !!str "description"
      : !!str "Super Hoop",
      ? !!str "price"
      : !!str "2392.00",
    },
  ],
  ? !!str "tax"
  : !!str "251.42",
  ? !!str "total"
  : !!str "4443.52",
  ? !!str "comments"
  : !!str "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.",
}
)";

const char* test5_canonical_node = R"(---
!<tag:clarkevans.com,2002:invoice> {
  ? !!str "invoice"
  : !!str "34843",
  ? !!str "date"
  : !!str "2001-01-23",
  ? !!str "bill-to"
  : !!map {
    ? !!str "given"
    : !!str "Chris",
    ? !!str "family"
    : !!str "Dumars",
    ? !!str "address"
    : !!map {
      ? !!str "lines"
      : !!str "458 Walkman Dr.\nSuite #292\n",
      ? !!str "city"
      : !!str "Royal Oak",
      ? !!str "state"
      : !!str "MI",
      ? !!str "postal"
      : !!str "48046",
    },
  },
  ? !!str "ship-to"
  : !!map {
    ? !!str "given"
    : !!str "Chris",
    ? !!str "family"
    : !!str "Dumars",
    ? !!str "address"
    : !!map {
      ? !!str "lines"
      : !!str "458 Walkman Dr.\nSuite #292\n",
      ? !!str "city"
      : !!str "Royal Oak",
      ? !!str "state"
      : !!str "MI",
      ? !!str "postal"
      : !!str "48046",
    },
  },
  ? !!str "product"
  : !!seq [
    !!map {
      ? !!str "sku"
      : !!str "BL394D",
      ? !!str "quantity"
      : !!str "4",
      ? !!str "description"
      : !!str "Basketball",
      ? !!str "price"
      : !!str "450.00",
    },
    !!map {
      ? !!str "sku"
      : !!str "BL4438H",
      ? !!str "quantity"
      : !!str "1",
      ? !!str "description"
      : !!str "Super Hoop",
      ? !!str "price"
      : !!str "2392.00",
    },
  ],
  ? !!str "tax"
  : !!str "251.42",
  ? !!str "total"
  : !!str "4443.52",
  ? !!str "comments"
  : !!str "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.",
}
)";

const char* test5_pretty2 = R"(!<tag:clarkevans.com,2002:invoice>
invoice: 34843
date: 2001-01-23
bill-to: &id001
  given: Chris
  family: Dumars
  address:
    lines: |
      458 Walkman Dr.
      Suite #292
    city: Royal Oak
    state: MI
    postal: 48046
ship-to: *id001
product:
- sku: BL394D
  quantity: 4
  description: Basketball
  price: 450.00
- sku: BL4438H
  quantity: 1
  description: Super Hoop
  price: 2392.00
tax: 251.42
total: 4443.52
comments: Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.
)";

const char* test5_pretty4 = R"(!<tag:clarkevans.com,2002:invoice>
invoice: 34843
date: 2001-01-23
bill-to: &id001
    given: Chris
    family: Dumars
    address:
        lines: |
            458 Walkman Dr.
            Suite #292
        city: Royal Oak
        state: MI
        postal: 48046
ship-to: *id001
product:
-   sku: BL394D
    quantity: 4
    description: Basketball
    price: 450.00
-   sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00
tax: 251.42
total: 4443.52
comments: Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.
)";

const char* test5_flow = "!<tag:clarkevans.com,2002:invoice> {invoice: 34843, date: 2001-01-23, bill-to: &id001 {given: Chris, family: Dumars, address: {lines: \"458 Walkman Dr.\\nSuite #292\\n\", city: Royal Oak, state: MI, postal: 48046}}, ship-to: *id001, product: [{sku: BL394D, quantity: 4, description: Basketball, price: 450.00}, {sku: BL4438H, quantity: 1, description: Super Hoop, price: 2392.00}], tax: 251.42, total: 4443.52, comments: Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.}\n";


const char* test_type_reads = R"(
# String tests (node 0)
- This is a test

# Boolean tests (nodes 1-10)
- true
- TRUE
- 1
- t
- T
- false
- FALSE
- 0
- f
- F

# Floating point tests (nodes 11-17)
- 0.
- 10.01
- -10.5
- -10
- 10
- 1.5e-10
- -2e+2

# Signed integer tests (nodes 18-20)
- -1
- -100
- -1837377238

# Unsigned integer tests (nodes 21-24)
- 0
- 1
- 100
- 1837377238

)";


static unsigned count_documents(const string& testname, const string& s) {
    Document d;
    unsigned numDocsRead = 0;
    istringstream ss(s);
    while (!ss.eof() && !ss.fail()) {
        ss >> d;
        if (!d.empty())
            ++numDocsRead;
    }
    return numDocsRead;
}

static Document read_first_doc(const string& s) {
    istringstream iss(s);
    Document d;
    iss >> d;
    return d;
}


static TestSuite ts("stream::yaml::yamlstream", {
    make_pair("construction, destruction, and copying", [](TestSuite&) {
        Document d;
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

        KSS_ASSERT(Node::none.tag().empty());
        KSS_ASSERT(Node::none.canonical().empty());
        KSS_ASSERT(Node::none.empty());
        KSS_ASSERT(!Node::none.isScalar());
        KSS_ASSERT(!Node::none.isSequence());
        KSS_ASSERT(!Node::none.isMapping());
    }),
    make_pair("whitespace variation & canonical writing", [](TestSuite&) {
        const char* test1a =
        "-   Mark McGwire\n"
        "- Sammy Sosa\n"
        "-    Ken Griffey\n";

        Document d1 = read_first_doc(test1);
        Document d2 = read_first_doc(test1a);
        KSS_ASSERT(d1 == d2);
        KSS_ASSERT(read_first_doc(test1_canonical) == d1);

        ostringstream oss;
        oss << d1;
        KSS_ASSERT(oss.str() == test1_canonical);
        KSS_ASSERT(!d1.empty() && d1 == d2);

        oss.str(string());
        oss << d2;
        KSS_ASSERT(oss.str() == test1_canonical);

        oss.str(string());
        oss << read_first_doc(test2);
        KSS_ASSERT(oss.str() == test2_canonical);

        oss.str(string());
        oss << read_first_doc(test3);
        KSS_ASSERT(oss.str() == test3_canonical);

        oss.str(string());
        oss << read_first_doc(test4);
        KSS_ASSERT(oss.str() == test1_canonical);

        oss.str(string());
        oss << read_first_doc(test5);
        KSS_ASSERT(oss.str() == test5_canonical);
    }),
    make_pair("PrettyPrint", [](TestSuite&) {
        ostringstream oss;
        oss << PrettyPrint(read_first_doc(test1));
        KSS_ASSERT(oss.str() == test1_pretty);

        oss.str(string());
        oss << PrettyPrint(read_first_doc(test2));
        KSS_ASSERT(oss.str() == test2_pretty);

        oss.str(string());
        oss << PrettyPrint(read_first_doc(test3));
        KSS_ASSERT(oss.str() == test3_pretty);

        oss.str(string());
        oss << PrettyPrint(read_first_doc(test4));
        KSS_ASSERT(oss.str() == test1_pretty);

        oss.str(string());
        Document d = read_first_doc(test5);
        oss << PrettyPrint(d);
        KSS_ASSERT(oss.str() == test5_pretty2);

        oss.str(string());
        oss << PrettyPrint(d, 4);
        KSS_ASSERT(oss.str() == test5_pretty4);

        oss.str(string());
        KSS_ASSERT(throwsException<range_error>([&] {
            oss << PrettyPrint(d, 0);
        }));
        KSS_ASSERT(throwsException<range_error>([&] {
            oss << PrettyPrint(d, 11);
        }));
    }),
    make_pair("FlowPrint", [](TestSuite&) {
        ostringstream oss;
        oss << FlowPrint(read_first_doc(test1));
        KSS_ASSERT(oss.str() == test1_flow);

        oss.str(string());
        oss << FlowPrint(read_first_doc(test2));
        KSS_ASSERT(oss.str() == test2_flow);

        oss.str(string());
        oss << FlowPrint(read_first_doc(test3));
        KSS_ASSERT(oss.str() == test3_flow);

        oss.str(string());
        oss << FlowPrint(read_first_doc(test4));
        KSS_ASSERT(oss.str() == test1_flow);

        oss.str(string());
        Document d = read_first_doc(test5);
        oss << FlowPrint(d);
        KSS_ASSERT(oss.str() == test5_flow);
    }),
    make_pair("root node access", [](TestSuite&) {
        Document d = read_first_doc(test1);
        Node n = d.root();
        KSS_ASSERT(n.tag() == "tag:yaml.org,2002:seq");
        KSS_ASSERT(!n.empty());
        KSS_ASSERT(!n.isScalar());
        KSS_ASSERT(n.isSequence());
        KSS_ASSERT(!n.isMapping());
        KSS_ASSERT(n.canonical() == test1_canonical);

        d = read_first_doc(test2);
        n = d.root();
        KSS_ASSERT(n.tag() == "tag:yaml.org,2002:map");
        KSS_ASSERT(!n.empty());
        KSS_ASSERT(!n.isScalar());
        KSS_ASSERT(!n.isSequence());
        KSS_ASSERT(n.isMapping());
        KSS_ASSERT(n.canonical() == test2_canonical);

        d = read_first_doc(test3);
        n = d.root();
        KSS_ASSERT(n.tag() == "tag:yaml.org,2002:map");
        KSS_ASSERT(n.isMapping());
        KSS_ASSERT(n.canonical() == test3_canonical);

        d = read_first_doc(test4);
        n = d.root();
        KSS_ASSERT(n.tag() == "tag:yaml.org,2002:seq");
        KSS_ASSERT(n.isSequence());
        KSS_ASSERT(n.canonical() == test1_canonical);

        d = read_first_doc(test5);
        n = d.root();
        KSS_ASSERT(n.tag() == "tag:clarkevans.com,2002:invoice");
        KSS_ASSERT(n.isMapping());
        KSS_ASSERT(n.canonical() == test5_canonical_node);
    }),
    make_pair("Sequence", [](TestSuite&) {
        Document d = read_first_doc(test1);
        Node n = Node(d.root());
        KSS_ASSERT(!n.empty());
        KSS_ASSERT(n[0].value() == "Mark McGwire");
        KSS_ASSERT(n[1].value() == "Sammy Sosa");
        KSS_ASSERT(n[2].value() == "Ken Griffey");
        KSS_ASSERT(n[3] == Node::none);

        Document d4 = read_first_doc(test4);
        KSS_ASSERT(d4.root() == d.root());

        Sequence seq(n);
        KSS_ASSERT(!seq.empty() && seq.size() == 3);
        Sequence::const_iterator it = seq.begin();
        Sequence::const_iterator last = seq.end();
        size_t i = 0;
        for (; it != last; ++it, ++i) {
            KSS_ASSERT(it->value() == n[i].value());
            KSS_ASSERT(seq.at(i) == n[i]);
        }
        KSS_ASSERT(seq.front() == n[0]);
        KSS_ASSERT(seq.back() == n[2]);
        KSS_ASSERT(throwsException<out_of_range>([&] {
            (void)seq.at(3);
        }));

        i = 0;
        for (Node nn : seq) {
            KSS_ASSERT(nn.value() == n[i].value());
            ++i;
        }

        KSS_ASSERT(seq.begin()->isScalar());
        KSS_ASSERT((*(seq.begin())).isScalar());
    }),
    make_pair("Mapping", [](TestSuite&) {
        Document d = read_first_doc(test2);
        Node n = d.root();
        KSS_ASSERT(!n.empty());
        KSS_ASSERT(n["american"] != Node::none);
        KSS_ASSERT(n["american"].isSequence());
        KSS_ASSERT(n["american"][0].value() == "Boston Red Sox");
        KSS_ASSERT(n["american"][1].value() == "Detroit Tigers");
        KSS_ASSERT(n["american"][2].value() == "New York Yankees");
        KSS_ASSERT(n["american"][3] == Node::none);
        KSS_ASSERT(n["national"] != Node::none);
        KSS_ASSERT(n["national"].isSequence());
        KSS_ASSERT(n["national"][0].value() == "New York Mets");
        KSS_ASSERT(n["national"][1].value() == "Chicago Cubs");
        KSS_ASSERT(n["national"][2].value() == "Atlanta Braves");

        KSS_ASSERT(n["not there"] == Node::none);

        {
            Mapping map(n);
            KSS_ASSERT(!map.empty());
            KSS_ASSERT(map.size() == 2);
            Node national = Node::none;
            for (size_t i = 0; i < map.size(); ++i) {
                pair<Node, Node> p = map[i];
                KSS_ASSERT(p.first.value() == "american" || p.first.value() == "national");
                if (p.first.value() == "national")
                    national = p.first;
                KSS_ASSERT(map.at(i) == p);
            }

            KSS_ASSERT(throwsException<out_of_range>([&] { map.at(3); }));
            KSS_ASSERT(map[national].isSequence());
            KSS_ASSERT(map[national][0].value() == "New York Mets");
            KSS_ASSERT(map.contains("national"));
            KSS_ASSERT(map.count("national") == 1);
            KSS_ASSERT(map.contains(national));
            KSS_ASSERT(map.count(national) == 1);
            KSS_ASSERT(!map.contains("canadian"));
            KSS_ASSERT(map.count("canadian") == 0);
            KSS_ASSERT(map["national"] != Node::none);
            KSS_ASSERT(map[national] != Node::none);
            KSS_ASSERT(map.at("national") != Node::none);
            KSS_ASSERT(map.at(national) != Node::none);
            KSS_ASSERT(map["canadian"] == Node::none);
            KSS_ASSERT(map.at("canadian") == Node::none);
            KSS_ASSERT(map.find("national") != map.end());
            KSS_ASSERT(map.find(national) != map.end());
            KSS_ASSERT(map.find("canadian") == map.end());
        }

        {
            d = read_first_doc(test5);
            Mapping map(d.root());
            KSS_ASSERT(map.size() == 8);
            KSS_ASSERT(map["bill-to"] == map["ship-to"]);

            Mapping::const_iterator it = map.begin();
            Mapping::const_iterator last = map.end();
            size_t i = 0;
            for (; it != last; ++it, ++i) {
                pair<Node, Node> p = *it;
                KSS_ASSERT(*it == map[i]);
            }

            i = 0;
            for (auto p : map) {
                KSS_ASSERT(p.first == map[i].first);
                KSS_ASSERT(p.second == map[i].second);
                ++i;
            }

            KSS_ASSERT(map.begin()->first.value() == "invoice");
        }
    }),
    make_pair("scalar nodes (type casting)", [](TestSuite&) {
        Document d = read_first_doc(test_type_reads);
        Sequence n(d.root());
        KSS_ASSERT(n.size() == 25);

        // All nodes can be read as strings.
        for (size_t i = 0; i < n.size(); ++i) {
            KSS_ASSERT(string(n[i]) != "");
        }

        // Nodes 1-5 are true booleans, 6-10 are false booleans.
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)bool(n[0]); }));
        for (size_t i = 1; i <= 5; ++i) {
            KSS_ASSERT(bool(n[i]) == true);
            KSS_ASSERT(bool(n[i+5]) == false);
        }

        // The remaining nodes are all numbers. 11-17 are floating point, 18-20 are signed
        // integers, and 22-24 are unsigned integers.
        KSS_ASSERT(double(n[12]) == 10.01);
        KSS_ASSERT(double(n[19]) == -100.);
        KSS_ASSERT(long(n[19]) == -100);
        KSS_ASSERT((unsigned long)n[23] == 100);
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)double(n[0]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)long(n[0]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)(unsigned long)(n[0]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)double(n[1]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)long(n[1]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)(unsigned long)(n[1]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)long(n[12]); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)(unsigned long)(n[12]); }));
        for (size_t i = 11; i <= 24; ++i) {
            KSS_ASSERT(double(n[i]) != -555.);
            if (i >= 18) {
                KSS_ASSERT(long(n[i]) != 555);
                KSS_ASSERT((unsigned long)n[i] != 555); // note that -val is considered a
                                                        // valid unsigned int according to
                                                        // strtoul
            }
        }

        // Checks for scalar node type casting.
        KSS_ASSERT(d.root().isSequence() && !d.root().isScalar());
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)d.root().value(); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)bool(d.root()); }));

        d = read_first_doc(test3);
        KSS_ASSERT(d.root().isMapping() && !d.root().isScalar());
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)d.root().value(); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)long(d.root()); }));
    }),
    make_pair("document path tests", [](TestSuite&) {
        Document d = read_first_doc(test5);
        KSS_ASSERT(d.root()["bill-to"]["family"].value() == "Dumars");
        KSS_ASSERT(long(d.root()["product"][1]["quantity"]) == 1L);
        KSS_ASSERT(d.root()["not there"] == Node::none);
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)d.root()["product"][3].value(); }));

        vector<Node> products = d.select("/product/*");
        KSS_ASSERT(products.size() == 2);
        KSS_ASSERT(products[0].isMapping());
        KSS_ASSERT(products[0].find("description").value() == "Basketball");
        KSS_ASSERT(d.value("/product/1/description") == "Super Hoop");
        KSS_ASSERT(products[1].value("/product/1/description") == "Super Hoop");

        KSS_ASSERT(throwsException<bad_cast>([&] { (void)d.select("/bill-to/0/given"); }));
        KSS_ASSERT(throwsException<invalid_argument>([&] { (void)d.select("//bill-to/*"); }));
        KSS_ASSERT(throwsException<bad_cast>([&] { (void)d.value("/bill-to/address"); }));

        KSS_ASSERT(d.select("bill-to/surname").empty());
        KSS_ASSERT(d.find("bill-to/surname") == Node::none);
        KSS_ASSERT(d.value("bill-to/surname") == string());

        vector<Node> productDescriptions = d.select("/product/*/description");
        KSS_ASSERT(productDescriptions.size() == 2);
        KSS_ASSERT(productDescriptions[0].value() == "Basketball");
        KSS_ASSERT(productDescriptions[1].value() == "Super Hoop");
    }),
    make_pair("mime type", [](TestSuite&) {
        KSS_ASSERT(kss::io::net::guessMimeType<Document>() == "application/x-yaml");
        KSS_ASSERT(kss::io::net::guessMimeType(Document()) == "application/x-yaml");
    })
});
