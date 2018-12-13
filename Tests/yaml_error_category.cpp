//
//  yaml_error_category.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-12-01.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <yaml.h>
#include <kss/io/yaml_error_category.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::io::stream::yaml;
using namespace kss::test;


static TestSuite ts("stream::yaml::yamlErrorCategory", {
    make_pair("error codes", [](TestSuite&) {
        const auto& cat = yamlErrorCategory();
        KSS_ASSERT(string(cat.name()) == "yaml");

        const error_code ec(YAML_EMITTER_ERROR, cat);
        KSS_ASSERT(ec.category() == cat);
        KSS_ASSERT(ec.value() == YAML_EMITTER_ERROR);
        KSS_ASSERT(!ec.message().empty());
    })
});
