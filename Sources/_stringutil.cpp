//
//  _stringutil.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-05.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cctype>
#include "_stringutil.hpp"

using namespace std;


string& kss::io::_private::ltrim(string& s) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && isspace(s[i])) { ++i; }
    if (i > 0) { s.erase(0, i); }
    return s;
}

string& kss::io::_private::ltrim(string& s, char c) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && s[i] == c) { ++i; }
    if (i > 0) { s.erase(0, i); }
    return s;
}

string& kss::io::_private::rtrim(string& s, char c) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && s[len-i-1] == c) { ++i; }
    if (i > 0) { s.erase(len-i); }
    return s;
}

string& kss::io::_private::rtrim(string& s) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && isspace(s[len-i-1])) { ++i; }
    if (i > 0) { s.erase(len-i); }
    return s;
}


