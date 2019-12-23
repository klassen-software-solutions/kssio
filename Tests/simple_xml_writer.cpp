//
//  simple_xml_writer.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-20.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <sstream>
#include <kss/io/simple_xml_writer.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::stream::xml;
using namespace kss::io::stream::xml::simple_writer;
using namespace kss::test;

namespace {
	struct ParamGenerator {
        Node* operator()() {
			if (current > 3) return nullptr;
			n.clear();
			n.name = "params";
			n["type"] = "test generator";
			n["value"] = to_string(++current);
			return &n;
		}
	private:
		int current = 0;
        Node n;
	};
}

static TestSuite ts("stream::xml::simple_writer", {
    make_pair("test without children", [] {
        Node root;
		root.name = "testsuites";
		root["tests"] = "3";
		root["failures"] = "1";
		root["special_chars"] = "'one' & 'two'";

		const string answer = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<testsuites failures="1" special_chars="&apos;one&apos; &amp; &apos;two&apos;" tests="3"/>
)XML";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, root);
            return strm.str();
        }));
    }),
    make_pair("test with children", [] {
        Node root;
		root.name = "testsuites";
		root["tests"] = "3";
		root["failures"] = "1";
		root["special_chars"] = "'one' & 'two'";
		root.children = {
            []()->Node*{
				static int counter = 0;
                static Node n;
				if (counter++ >= 5) {
					return nullptr;
				}
				else {
					n.clear();
					n.name = "counter";
					n["count"] = to_string(counter);
					if (counter == 2) n.text = "This node has some contents.";
					if (counter == 3) n.text = "This node has comments on this line\nand on a second line.";
					return &n;
				}
			},
            []()->Node*{
				static bool haveit = false;
                static Node n;
				if (haveit) {
					return nullptr;
				}
				else {
					haveit = true;
					n.clear();
					n.name = "empty";
					return &n;
				}
			}
		};

		const string answer = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<testsuites failures="1" special_chars="&apos;one&apos; &amp; &apos;two&apos;" tests="3">
  <counter count="1"/>
  <counter count="2">
    This node has some contents.
  </counter>
  <counter count="3">
    This node has comments on this line
    and on a second line.
  </counter>
  <counter count="4"/>
  <counter count="5"/>
  <empty/>
</testsuites>
)XML";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, root);
            return strm.str();
        }));
    }),
    make_pair("test with grandchildren", [] {
        Node root;
		root.name = "testsuites";
		root["tests"] = "3";
		root["failures"] = "1";
		root["special_chars"] = "'one' & 'two'";
		root.children = {
            []()->Node*{
				static int counter = 0;
                static Node n;
				if (counter++ >= 5) {
					return nullptr;
				}
				else {
					n.clear();
					n.name = "counter";
					n["count"] = to_string(counter);
					if (counter == 2) n.text = "This node has some contents.";
					if (counter == 3) n.text = "This node has comments on this line\nand on a second line.";
					if (counter == 4) n.children = { ParamGenerator() };
					return &n;
				}
			},
            []()->Node*{
				static bool haveit = false;
                static Node n;
				if (haveit) {
					return nullptr;
				}
				else {
					haveit = true;
					n.clear();
					n.name = "empty";
					return &n;
				}
			}
		};

		const string answer = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<testsuites failures="1" special_chars="&apos;one&apos; &amp; &apos;two&apos;" tests="3">
  <counter count="1"/>
  <counter count="2">
    This node has some contents.
  </counter>
  <counter count="3">
    This node has comments on this line
    and on a second line.
  </counter>
  <counter count="4">
    <params type="test generator" value="1"/>
    <params type="test generator" value="2"/>
    <params type="test generator" value="3"/>
    <params type="test generator" value="4"/>
  </counter>
  <counter count="5"/>
  <empty/>
</testsuites>
)XML";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, root);
            return strm.str();
        }));
    })
});
