//
//  simple_json_writer.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-14.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <sstream>

//#include <kss/io/jsonstream.hpp>
#include <kss/io/simple_json_writer.hpp>

#include "ksstest.hpp"


using namespace std;
//using namespace kss::util::json;
using namespace kss::io::stream::json::simple_writer;

namespace {
	struct ParamGenerator {
        Node* operator()() {
			if (current > 3) return nullptr;
			json.clear();
			json["type"] = "test generator";
			json["value"] = to_string(++current);
			return &json;
		}
	private:
		int current = 0;
        Node json;
	};
}

#if 0
static kss::testing::TestSet ts("json::simple_writer", {
	[]{
		KSS_TEST_GROUP("test without children");
		node json;
		json["tests"] = "3";
		json["failures"] = "1";
		json["errors"] = "0";
		json["time"] = "0.035s";
		json["timestamp"] = "2011-10-31T18:52:42Z";
		json["special_chars"] = "'one' & 'two'";

		stringstream strm;
		write(strm, json);

		Document d;
		strm >> d;
		// For now all we can test is that we can parse the JSON. These tests will
		// become more complete as our JSON streaming classes become more complete.
		KSS_ASSERT(d.root().isObject());
		KSS_ASSERT(!d.root().empty());
	},
	[]{
		KSS_TEST_GROUP("test with children");
		node json;
		json["tests"] = "3";
		json["failures"] = "1";
		json["errors"] = "0";
		json["time"] = "0.035s";
		json["timestamp"] = "2011-10-31T18:52:42Z";
		json["special_chars"] = "'one' & \"two\"";
		json.arrays = {
			make_pair("testsuites", []()->node*{
				static int counter = 0;
				static node json;
				if (counter++ >= 5) {
					return nullptr;
				}
				else {
					json.clear();
					json["counter"] = to_string(counter);
					return &json;
				}
			}),
			make_pair("params", ParamGenerator())
		};

		stringstream strm;
		write(strm, json);

		Document d;
		strm >> d;
		// For now all we can test is that we can parse the JSON. These tests will
		// become more complete as our JSON streaming classes become more complete.
		KSS_ASSERT(d.root().isObject());
		KSS_ASSERT(!d.root().empty());
	},
	[]{
		KSS_TEST_GROUP("test with grandchildren");
		node json;
		json["tests"] = "3";
		json["failures"] = "1";
		json["errors"] = "0";
		json["time"] = "0.035s";
		json["timestamp"] = "2011-10-31T18:52:42Z";
		json["special_chars"] = "'one' & \"two\"";
		json.arrays = {
			make_pair("testsuites", []()->node*{
				static int counter = 0;
				static node json;
				if (counter++ >= 5) {
					return nullptr;
				}
				else {
					json.clear();
					json["counter"] = to_string(counter);
					if (counter == 2) {
						json.arrays = {
							make_pair("grandchildren", ParamGenerator())
						};
					}
					return &json;
				}
			})
		};

		stringstream strm;
		write(strm, json);

		Document d;
		strm >> d;
		// For now all we can test is that we can parse the JSON. These tests will
		// become more complete as our JSON streaming classes become more complete.
		KSS_ASSERT(d.root().isObject());
		KSS_ASSERT(!d.root().empty());
	}
});
#endif
