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

#include <kss/io/simple_json_writer.hpp>

#include "ksstest.hpp"


using namespace std;
using namespace kss::io::stream::json::simple_writer;
using namespace kss::test;


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

static TestSuite ts("stream::json::simple_writer", {
    make_pair("test without children", [] {
        Node json;
        json["tests"] = "3";
        json["failures"] = "1";
        json["errors"] = "0";
        json["time"] = "0.035s";
        json["timestamp"] = "2011-10-31T18:52:42Z";
        json["special_chars"] = "'one' & 'two'";

        const string answer = R"JSON({
  "errors": 0,
  "failures": 1,
  "special_chars": "'one' & 'two'",
  "tests": 3,
  "time": "0.035s",
  "timestamp": "2011-10-31T18:52:42Z"
}
)JSON";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, json);
            return strm.str();
        }));
    }),
    make_pair("test with children", [] {
        Node json;
        json["tests"] = "3";
        json["failures"] = "1";
        json["errors"] = "0";
        json["time"] = "0.035s";
        json["timestamp"] = "2011-10-31T18:52:42Z";
        json["special_chars"] = "'one' & \"two\"";
        json.arrays = {
            make_pair("testsuites", []()->Node*{
                static int counter = 0;
                static Node json;
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

        const string answer = R"JSON({
  "errors": 0,
  "failures": 1,
  "special_chars": "'one' & \"two\"",
  "tests": 3,
  "time": "0.035s",
  "timestamp": "2011-10-31T18:52:42Z",
  "testsuites": [
    {
      "counter": 1
    },
    {
      "counter": 2
    },
    {
      "counter": 3
    },
    {
      "counter": 4
    },
    {
      "counter": 5
    }
  ],
  "params": [
    {
      "type": "test generator",
      "value": 1
    },
    {
      "type": "test generator",
      "value": 2
    },
    {
      "type": "test generator",
      "value": 3
    },
    {
      "type": "test generator",
      "value": 4
    }
  ]
}
)JSON";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, json);
            return strm.str();
        }));
    }),
    make_pair("test with grandchildren", [] {
        Node json;
        json["tests"] = "3";
        json["failures"] = "1";
        json["errors"] = "0";
        json["time"] = "0.035s";
        json["timestamp"] = "2011-10-31T18:52:42Z";
        json["special_chars"] = "'one' & \"two\"";
        json.arrays = {
            make_pair("testsuites", []()->Node*{
                static int counter = 0;
                static Node json;
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

        const string answer = R"JSON({
  "errors": 0,
  "failures": 1,
  "special_chars": "'one' & \"two\"",
  "tests": 3,
  "time": "0.035s",
  "timestamp": "2011-10-31T18:52:42Z",
  "testsuites": [
    {
      "counter": 1
    },
    {
      "counter": 2,
      "grandchildren": [
        {
          "type": "test generator",
          "value": 1
        },
        {
          "type": "test generator",
          "value": 2
        },
        {
          "type": "test generator",
          "value": 3
        },
        {
          "type": "test generator",
          "value": 4
        }
      ]
    },
    {
      "counter": 3
    },
    {
      "counter": 4
    },
    {
      "counter": 5
    }
  ]
}
)JSON";

        KSS_ASSERT(isEqualTo<string>(answer, [&] {
            stringstream strm;
            write(strm, json);
            return strm.str();
        }));
    })
});
