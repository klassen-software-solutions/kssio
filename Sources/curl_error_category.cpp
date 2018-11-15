//
//  curl_error_category.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <mutex>
#include <string>
#include <stdexcept>

#include <curl/curl.h>

#include "_raii.hpp"
#include "curl_error_category.hpp"

using namespace std;
using namespace kss::io::_private;
using namespace kss::io::net;

namespace {
    class CurlErrorCategory : public error_category {
    public:
        const char* name() const noexcept override {
            return "curl";
        }

        string message(int val) const override {
            return curl_easy_strerror(CURLcode(val));
        }
    };

    static RAII curlSetupAndCleanup([]{
        const auto error = curl_global_init(CURL_GLOBAL_ALL);
        if (error) {
            throw runtime_error("Could not init CURL.");
        }
    }, []{
        curl_global_cleanup();
    });
}


const error_category& kss::io::net::curlErrorCategory() noexcept {
    static CurlErrorCategory instance;
    return instance;
}
