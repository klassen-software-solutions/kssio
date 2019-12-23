//
//  eai_error_category.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-08-21.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <kss/contract/all.h>

#include "eai_error_category.hpp"

using namespace std;

namespace contract = kss::contract;

namespace {
    class EaiErrorCategory : public error_category {
    public:
        const char* name() const noexcept override {
            return "eai";
        }

        string message(int val) const override {
            const char* msg = gai_strerror(val);
            contract::postconditions({
                KSS_EXPR(msg != nullptr)
            });
            return string(msg);
        }
    };
}


const error_category& kss::io::net::eaiErrorCategory() noexcept {
    static EaiErrorCategory instance;
    return instance;
}

error_code kss::io::net::eaiErrorCode(int error) noexcept {
    return (error == EAI_SYSTEM
            ? error_code(errno, system_category())
            : error_code(error, eaiErrorCategory()));
}
