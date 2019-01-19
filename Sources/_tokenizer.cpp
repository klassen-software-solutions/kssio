//
//  _tokenizer.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2012-12-22.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
//  "Borrowed" from kssutil.
//

#include "utility.hpp"
#include "_tokenizer.hpp"

using namespace std;
using namespace kss::io::_private;


Tokenizer::Tokenizer(const string& s, const string& delim, bool skipEmptyTokens,
                     string::size_type start, string::size_type end)
: _s(s), _delim(delim), _skipEmptyTokens(skipEmptyTokens)
{
    if (delim.empty()) { throw invalid_argument("delim cannot be empty"); }
    _lastPos = start;
    _end = min(end, _s.length());
    if (_lastPos == _end) { _lastPos = _end+1; }
}

Tokenizer::Tokenizer(string&& s, const string& delim, bool skipEmptyTokens,
                     string::size_type start, string::size_type end)
: _s(s), _delim(delim), _skipEmptyTokens(skipEmptyTokens)
{
    if (delim.empty()) { throw invalid_argument("delim cannot be empty"); }
    _lastPos = start;
    _end = min(end, _s.length());
    if (_lastPos == _end) { _lastPos = _end+1; }
}


// Obtain the next token.
Tokenizer& Tokenizer::operator>>(string& token) {

    // We should not be calling this if we have already reached the end.
    if (eof()) { throw kss::io::Eof(); }

    bool tryAnother = false;
    do {
        tryAnother = false;
        string::size_type pos = _s.find_first_of(_delim, _lastPos);

        if (pos >= _end) {
            // final token
            if (_lastPos < _end) {
                token = _s.substr(_lastPos, (_end - _lastPos));
            }
            else {
                // Final token is the empty token. If that isn't allowed, we have to throw
                // an exception. Otherwise we return an empty token.
                if (_skipEmptyTokens) { throw kss::io::Eof(); }
                token = string();
            }
            _lastPos = _end+1;
        }
        else if (pos == _lastPos) {
            // empty token
            ++_lastPos;
            if (_skipEmptyTokens) {
                tryAnother = true;
                continue;
            }
            token = string();
        }
        else {
            // next token
            token = _s.substr(_lastPos, (pos - _lastPos));
            _lastPos = pos + 1;
        }
    } while (tryAnother);

    return *this;
}
