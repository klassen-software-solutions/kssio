//
//  jsonstream.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2016-11-05.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cctype>
#include <sstream>
#include <typeinfo>

#include "_contract.hpp"
#include "_json.h"
#include "jsonstream.hpp"
#include "utility.hpp"

using namespace std;
using namespace kss::io::stream::json;

namespace contract = kss::io::contract;


// MARK: Node Implementation

struct Node::Impl {
    Json::Value value;
};


// Move semantics.
Node::Node(Node&& n) {
    _impl = n._impl;
    n._impl = nullptr;

    contract::postconditions({
        // Note, cannot assert that _impl != nullptr, as n may have been that to start
        KSS_EXPR(n._impl == nullptr)
    });
}

Node::Node(const Node& n) {
    if (!n._impl) {
        _impl = nullptr;
    }
    else {
        _impl = new Impl();
        _impl->value = n._impl->value;
    }

    contract::postconditions({
        KSS_EXPR(*this == n)
    });
}

Node& Node::operator=(Node&& n) noexcept {
    if (&n != this) {
        if (_impl) { delete _impl; }
        _impl = n._impl;
        n._impl = nullptr;
    }

    contract::postconditions({
        // Note, cannot assert that _impl != nullptr, as n may have been that to start
        KSS_EXPR(n._impl == nullptr)
    });
    return *this;
}

Node& Node::operator=(const Node& n) {
    if (&n != this) {
        if (_impl) { delete _impl; }
        if (!n._impl) {
            _impl = nullptr;
        }
        else {
            _impl = new Impl();
            _impl->value = n._impl->value;
        }
    }

    contract::postconditions({
        KSS_EXPR(*this == n)
    });
    return *this;
}

// Cleanup the node.
Node::~Node() noexcept {
    if (_impl) { delete _impl; }
}

// Determine the node type.
bool Node::isScalar() const noexcept {
    return (_impl && (_impl->value.isBool()
                      || _impl->value.isInt()
                      || _impl->value.isInt64()
                      || _impl->value.isUInt()
                      || _impl->value.isUInt64()
                      || _impl->value.isDouble()
                      || _impl->value.isString()));
}

bool Node::isArray() const noexcept {
    return (_impl && (_impl->value.isArray()));
}

bool Node::isObject() const noexcept {
    return (_impl && (_impl->value.isObject()));
}


// Determine if a node is empty.
bool Node::empty() const noexcept {
    if (!_impl) { return true; }
    return _impl->value.empty();
}


// Check for equality.
bool Node::equal(const Node& n) const noexcept {
    if (&n == this) { return true; }
    if (!_impl) {
        if (!n._impl) { return true; }
        return false;
    }
    if (!n._impl) {
        if (_impl) { return false; }
    }
    return (_impl->value == n._impl->value);
}

// Value that refers to no node.
const Node Node::none;


// Check the type of a node
bool Node::isString() const noexcept {
    if (!_impl) { return false; }
    return _impl->value.isConvertibleTo(Json::ValueType::stringValue);
}

bool Node::isBool() const noexcept {
    if (!_impl) { return false; }
    return _impl->value.isConvertibleTo(Json::ValueType::booleanValue);
}

bool Node::isDouble() const noexcept {
    if (!_impl) { return false; }
    return _impl->value.isConvertibleTo(Json::ValueType::realValue);
}

bool Node::isLong() const noexcept {
    if (!_impl) { return false; }
    return _impl->value.isConvertibleTo(Json::ValueType::intValue);
}

bool Node::isUnsignedLong() const noexcept {
    if (!_impl) { return false; }
    return _impl->value.isConvertibleTo(Json::ValueType::uintValue);
}

Node::operator std::string() const {
    if (!isString()) { throw bad_cast(); }
    return _impl->value.asString();
}

Node::operator bool() const {
    if (!isBool()) { throw bad_cast(); }
    return _impl->value.asBool();
}

Node::operator double() const {
    if (!isDouble()) { throw bad_cast(); }
    return _impl->value.asDouble();
}

Node::operator long() const {
    if (!isLong()) { throw bad_cast(); }
    return _impl->value.asInt64();
}

Node::operator unsigned long() const {
    if (!isUnsignedLong()) { throw bad_cast(); }
    return _impl->value.asUInt64();
}


// MARK: Document Implementation

struct Document::Impl {
    Node rootNode;
};

// Construct an empty document.
Document::Document() : _impl(new Impl()) {
}

// Construct from a string.
Document::Document(const string& jsonStr) : _impl(new Impl()) {
    contract::parameters({
        KSS_EXPR(!jsonStr.empty())
    });

    istringstream strm(jsonStr);
    Json::CharReaderBuilder builder;
    Json::Value v;
    string errors;
    if (!Json::parseFromStream(builder, strm, &v, &errors)) {
        throw ParsingError(errors);
    }
    _impl->rootNode._impl = new Node::Impl();
    _impl->rootNode._impl->value.swap(v);

    contract::parameters({
        KSS_EXPR(!this->empty())
    });
}


// Copy a document.
Document::Document(const Document& d) {
    _impl = d._impl;

    contract::postconditions({
        KSS_EXPR(*this == d)
    });
}


Document& Document::operator=(const Document& d) {
    if (&d != this) {
        _impl = d._impl;
    }

    contract::postconditions({
        KSS_EXPR(*this == d)
    });
    return *this;
}

// Ensure the implementation is unique, replacing it with a new one if necessary.
void Document::ensureUnique() {
    if (!_impl.unique()) {
        shared_ptr<Impl> newImpl(new Impl());
        newImpl->rootNode = _impl->rootNode;
        _impl = newImpl;
    }

    contract::postconditions({
        KSS_EXPR(_impl.unique())
    });
}

Document::~Document() noexcept = default;

// Determine document equality.
bool Document::equal(const Document& d) const noexcept {
    if (&d == this) {
        return true;
    }
    if (_impl.get() == d._impl.get()) {
        return true;
    }
    return (_impl->rootNode == d._impl->rootNode);
}

// Determine if a document is empty.
bool Document::empty() const noexcept {
    return _impl->rootNode.empty();
}

// Destroy everything in the document.
void Document::clear() noexcept {
    if (!empty()) {
        _impl = shared_ptr<Impl>(new Impl());
    }
    contract::postconditions({
        KSS_EXPR(empty())
    });
}

// Return the root node.
Node Document::root() const {
    return _impl->rootNode;
}


// MARK: Stream Reading and Writing

namespace {
    void eat_whitespace(istream& is) {
        while (!is.eof() && !is.fail()) {
            const int c = is.peek();
            if (isspace(c)) {
                is.get();
            } else {
                break;
            }
        }
    }
}

istream& kss::io::stream::json::operator>>(istream& is, Document& d) {

    // Since the Json::Reader class will not recognize the end of a JSON object within a stream
    // (it reads the entire stream then tries to parse), we need to read it one line at a time
    // and see if it is parseable. This is obviously not the most efficient way to do things,
    // but it will have to do until we find a better Json reader.
    bool readOne = false;
    string line;
    string buf;
    Json::CharReaderBuilder builder;
    Json::Value v;
    string errors;
    eat_whitespace(is);
    while (getline(is, line)) {
        buf += line;
        buf += "\n";
        istringstream iss(buf);
        if (Json::parseFromStream(builder, iss, &v, &errors)) {
            d.clear();
            if (!d._impl->rootNode._impl) {
                d._impl->rootNode._impl = new Node::Impl();
            }
            d._impl->rootNode._impl->value.swap(v);
            readOne = true;
            break;
        }
    }

    if (!readOne) {
        is.setstate(ios::failbit);
    }
    return is;
}
