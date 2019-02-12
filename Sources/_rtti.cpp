//
//  _rtti.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-01-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// "Borrowed" from kssutil
//

#include <stdexcept>

#include <cxxabi.h>

#include "_rtti.hpp"


using namespace std;
using namespace kss::io::_private::rtti;


// Determine the readable version of a manged name.
string kss::io::_private::rtti::demangle(const string& tname) {
    int status;
    char* val = abi::__cxa_demangle(tname.c_str(), 0, 0, &status);
    if (val == nullptr) {
        throw runtime_error("Failed to demangle '" + tname + "'");
    }
	return string(val);
}
