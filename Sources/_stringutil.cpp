//
//  _stringutil.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-05.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cctype>
#include <cstring>
#include "_stringutil.hpp"

using namespace std;


namespace {
    int kss_ends_with(const char* str, const char* suffix) {
        /* All strings - event empty ones - end with an empty suffix. */
        if (!suffix)
            return 1;
        size_t suflen = strlen(suffix);
        if (!suflen)
            return 1;

        /* Empty strings, or strings shorter than the suffix, cannot end with the suffix. */
        if (!str)
            return 0;
        size_t slen = strlen(str);
        if (slen < suflen)
            return 0;

        /* Compare the end of the string to see if it matches the suffix. */
        return (strncmp(str+slen-suflen, suffix, suflen) == 0);
    }
}

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

bool kss::io::_private::endsWith(const string &str, const string &suffix) noexcept {
    return kss_ends_with(str.c_str(), suffix.c_str());
}
