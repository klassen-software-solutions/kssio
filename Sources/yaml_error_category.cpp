//
//  yaml_error_category.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-12-01.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <mutex>
#include <unordered_map>

#include <yaml.h>

#include "yaml_error_category.hpp"

using namespace std;
using namespace kss::io::stream::yaml;

namespace {
    class YamlErrorCategory : public error_category {
    public:
        const char* name() const noexcept override {
            return "yaml";
        }

        string message(int val) const override {
            static unordered_map<yaml_error_type_t, string> strings;
            static once_flag flag;
            call_once(flag, [] {
                strings.reserve(8);
                strings[YAML_NO_ERROR] = "No error";
                strings[YAML_MEMORY_ERROR] = "Cannot allocate or reallocate a block of memory";
                strings[YAML_READER_ERROR] = "Cannot read or decode the input stream";
                strings[YAML_SCANNER_ERROR] = "Cannot scan the input stream";
                strings[YAML_PARSER_ERROR] = "Cannot parse the input stream";
                strings[YAML_COMPOSER_ERROR] = "Cannot compose a YAML document";
                strings[YAML_WRITER_ERROR] = "Cannot write to the output stream";
                strings[YAML_EMITTER_ERROR] = "Cannot emit a YAML stream";
            });

            // Search for the string. If not found return an unknown.
            auto it = strings.find(static_cast<yaml_error_type_t>(val));
            return (it == strings.end() ? "unknown error" : it->second.c_str());
        }
    };
}


const error_category& kss::io::stream::yaml::yamlErrorCategory() noexcept {
    static YamlErrorCategory instance;
    return instance;
}
