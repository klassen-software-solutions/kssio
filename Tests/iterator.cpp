//
//  iterator.cpp
//  kssio
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction other
// 	than to recognize that others are allowed to do the same.
//
//  Created by Steven W. Klassen on 2014-10-24.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

#include <kss/io/iterator.hpp>
#include <kss/io/utility.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io::stream;
using namespace kss::test;

using kss::io::InvalidState;


namespace {
    class container {
    public:
        container() { _current = 0; }
        void increment() { ++_current; }

    protected:
        long _current;
    };

    // subclass for testing the input_iterator
    class input_container : public container {
    public:
        typedef InputIterator<input_container, unsigned> const_iterator;
        const_iterator begin() { return const_iterator(*this); }
        const_iterator end() { return const_iterator(); }

        input_container& operator>>(unsigned& value) {
            if (_current > 5) throw std::runtime_error("too many >> calls");
            ++_current;
            value = unsigned(_current);
            return *this;
        }

        bool eof() const {
            if (_current > 5) throw std::runtime_error("too many next()s");
            if (_current == 5) return true;
            return false;
        }
    };

    // class for testing the output_iterator
    class output_container : public container {
    public:
        typedef OutputIterator<output_container, unsigned> iterator;
        iterator begin() { return iterator(*this); }

        output_container& operator<<(const unsigned& value) {
            ++_current;
            return *this;
        }

        long counter() const noexcept { return _current; }
    };
}


static TestSuite ts("stream::iterator", {
    make_pair("InputIterator basic tests", [] {
        input_container c;
        input_container::const_iterator it = c.begin();
        input_container::const_iterator last = c.end();
        for (unsigned i = 1; i <= 5; ++i) {
            KSS_ASSERT(it != last);
            KSS_ASSERT(*it == i);
            ++it;
        }
        KSS_ASSERT(it == last);
        KSS_ASSERT(c.begin() == last);
    }),
    make_pair("InputIterator type declarations", [] {
        input_container c;
        auto it = c.begin();
        input_container::const_iterator::difference_type d = 1;
        input_container::const_iterator::value_type v = *it;
        input_container::const_iterator::pointer p = &v;
        input_container::const_iterator::reference r = v;
        KSS_ASSERT(typeid(d) == typeid(ptrdiff_t));
        KSS_ASSERT(typeid(v) == typeid(unsigned));
        KSS_ASSERT(typeid(p) == typeid(const unsigned*));
        KSS_ASSERT(typeid(r) == typeid(const unsigned&));
    }),
    make_pair("InputIterator copies", [] {
        input_container c1, c2;
        input_container::const_iterator it1 = c1.begin(), it2 = c2.begin();
        input_container::const_iterator last1 = c1.end(), last2 = c2.end();
        KSS_ASSERT(it1 != it2 && !(it1 == it2));
        KSS_ASSERT(it1 != last1 && !(it1 == last1));
        KSS_ASSERT(last1 == last2 && !(last1 != last2));

        input_container::const_iterator it = it2;
        KSS_ASSERT(it == it2);
        KSS_ASSERT(it != it1);

        KSS_ASSERT(*it == 1 && *it2 == 1);  // Note that two input iterators from the same container
        ++it2;                              // will "interfere" with each other in terms of reading
        ++it;                               // values from the container.
        KSS_ASSERT(*it == 3 && *it2 == 2);
        ++it;
        ++it;
        KSS_ASSERT(*it == 5 && *it2 == 2);
        ++it2;
        KSS_ASSERT(it2 == last2);

        // Test that we pass on exceptions when we iterate past end().
        KSS_ASSERT(throwsException<InvalidState>([&] { ++last1; }));
        KSS_ASSERT(throwsException<InvalidState>([&] { ++it2; }));
        ++it;
        KSS_ASSERT(it == last2);
        KSS_ASSERT(throwsException<InvalidState>([&] { ++it; }));

        // Test that we pass on exceptions from operator>>() and eof().
        for (int i = 0; i < 10; ++i) c1.increment();
        KSS_ASSERT(throwsException<runtime_error>([&] { ++it1; }));
    }),
    make_pair("InputIterator prefix iteration combined with dereference", [] {
        input_container c;
        auto it = c.begin();
        for (unsigned i = 1; i < 5;) {
            KSS_ASSERT(*++it == ++i);
        }
    }),
    make_pair("InputIterator postfix iteration combined with dereference", [] {
        input_container c;
        unsigned i = 1;
        for (auto it = c.begin(), last = c.end(); it != last;) {
            KSS_ASSERT(*it++ == i++);
        }
    }),
    make_pair("InputIterator swap will handle end()", [] {
        input_container c, c1;
        auto first = c.begin(), last = c.end(), first1 = c1.begin();
        swap(first, last);
        KSS_ASSERT(first == c.end());
        swap(first, first1);
        KSS_ASSERT(first1 == c.end());
    }),
    make_pair("InputIterator will work with STL algorithms", [] {
        unsigned ar[10];
        input_container c;
        for (size_t i = 0; i < 10; ++i) ar[i] = 999;
        copy(c.begin(), c.end(), ar);
        for (size_t i = 0; i < 5; ++i) {
            KSS_ASSERT(ar[i] == (unsigned)i+1);
        }
        for (size_t i = 5; i < 10; ++i) {
            KSS_ASSERT(ar[i] == 999);
        }
    }),
    make_pair("InputIterator is similar to an istream_iterator", [] {
        istringstream iss("one\ntwo\nthree\nfour");
        typedef InputIterator<istream, string> my_istream_iterator;
        my_istream_iterator begin = my_istream_iterator(iss);
        my_istream_iterator end = my_istream_iterator();
        vector<string> v(begin, end);
        KSS_ASSERT(v.size() == 4);
        KSS_ASSERT(v[0] == "one");
        KSS_ASSERT(v[1] == "two");
        KSS_ASSERT(v[2] == "three");
        KSS_ASSERT(v[3] == "four");
    }),
    make_pair("OutputIterator", [] {
        output_container c;
        long ar[] { 1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L, 9L, 10L };

        copy(ar, ar+10, c.begin());
        KSS_ASSERT(c.counter() == 10);

        OutputIterator<output_container, unsigned> it = c.begin();
        copy(ar, ar+10, it);
        KSS_ASSERT(c.counter() == 20);

        OutputIterator<output_container, unsigned> it2 = move(it);
        copy(ar, ar+10, it2);
        KSS_ASSERT(c.counter() == 30);

        output_container c2;
        it = c.begin();
        OutputIterator<output_container, unsigned> it3 = c2.begin();
        swap(it, it3);
        copy(ar, ar+10, it3);
        KSS_ASSERT(c2.counter() == 0 && c.counter() == 40);
    })
});
