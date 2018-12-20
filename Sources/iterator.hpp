//
//  iterator.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-10-24.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//


#ifndef kssio_iterator_hpp
#define kssio_iterator_hpp

#include <algorithm>
#include <cstddef>
#include <iterator>
#include "utility.hpp"

namespace kss {
    namespace io {
        namespace stream {

            /*!
             The InputIterator class provides an input iterator for any class that acts
             like an input stream. Specifically it needs to support the following operations:

                 Stream& operator>>(T& t);   // Obtain the next available value.
                 bool eof() const;           // Indicates "end of file", ie. no more elements

             The type T needs to support a default constructor and copy and move semantics.
             */
            template <typename Stream, typename T>
            class InputIterator
            : public std::iterator<std::input_iterator_tag, T, ptrdiff_t, const T*, const T&>
            {
            public:

                /*!
                 Construct/assign the iterator. Typically the containers begin() and end()
                 methods would be defined along the lines of the following.

                     using iterator = kss::io::stream::InputIterator<Stream, T>;
                     iterator begin() { return iterator(*this); }
                     iterator end() { return iterator(); }

                 The default constructor creates an iterator that points to nothing other
                 than a special "end" flag.

                 The container version of the constructor creates an iterator and attempts
                 to obtain the next value from the container.

                 @throws any exception that container::operator>> or container::eof() may throw
                 @throws any exception that the T copy and move semantics may throw
                 */
                InputIterator() : _stream(nullptr) {}
                explicit InputIterator(Stream& stream) {
                    if (stream.eof()) {
                        _stream = nullptr;
                    }
                    else {
                        _stream = &stream;
                        operator++();
                    }
                }

                InputIterator(const InputIterator& it) = default;
                InputIterator(InputIterator&& it)
                : _stream(it._stream), _value(std::move(it._value))
                {
                    it._stream = nullptr;
                }
                InputIterator& operator=(const InputIterator& it) = default;
                InputIterator& operator=(InputIterator&& it) noexcept {
                    if (&it != this) {
                        _stream = it._stream;
                        _value = std::move(it._value);
                    }
                    return *this;
                }

                /*!
                 Determine iterator equality. Any "end" iterators are considered equal, even if
                 they have different streams. Any other iterators are equal so long as they
                 are referencing the same stream. And iterator that is not an "end" iterator
                 is not equal to an "end" iterator.

                 This may sound complex, but it is consistent with the operation of
                 std::istream_operator (the typical example of an input iterator).
                 */
                bool operator==(const InputIterator& it) const noexcept {
                    if (&it == this) { return true; }
                    if (_stream == it._stream) { return true; }
                    return false;
                }
                bool operator!=(const InputIterator& it) const noexcept {
                    return !operator==(it);
                }

                /*!
                 Dereference the iterator. This is undefined if we try to dereference the end()
                 value.
                 */
                const T& operator*() const { return _value; }
                const T* operator->() const { return &_value; }

                /*!
                 Increment the iterator. If the containers eof() is true then this will become
                 an instance of the end iterator. Otherwise we call the containers >> operator
                 and store the result.

                 If for some reason eof() cannot be known until operator>> is called, then you
                 should have your operator>> throw a kss::eof exception if it reaches the end.
                 This allows us to avoid having to write "lookahead" code in these situations.

                 @throws runtime_error if this is already the end iterator.
                 @throws any exception that container::eof() or container::operator>>(T&) may throw
                 */
                InputIterator& operator++() {
                    if (!_stream) {
                        throw kss::io::InvalidState("This iterator is already at the end state.");
                    }

                    if (_stream->eof()) {
                        _stream = nullptr;
                    }
                    else {
                        try {
                            *_stream >> _value;
                        }
                        catch (const kss::io::Eof&) {
                            // Not an error, just means we were at the end of the input.
                            _stream = nullptr;
                        }
                    }
                    return *this;
                }
                InputIterator operator++(int) {
                    InputIterator it = *this;
                    operator++();
                    return it;
                }

                /*!
                 Swap with another iterator.
                 */
                void swap(InputIterator& b) noexcept {
                    if (this != &b) {
                        std::swap(_stream, b._stream);
                        std::swap(_value, b._value);
                    }
                }

            private:
                Stream* _stream;
                T       _value;
            };


            /*!
             The output_iterator class provides an output iterator for any object that acts
             like an output stream. Specifically it needs to support the following operation:
             
                 Stream& operator<<(const T& t);
               
             Like all output iterators this one will never reach an "end" but will keep inserting
             items into the container or stream.
             */
            template <typename Stream, typename T>
            class OutputIterator
            : public std::iterator<std::output_iterator_tag, void, void, void, void>
            {
            public:

                /*!
                 Construct/assign the iterator. Typically the container's begin() method would
                 be defined like the following.
                 
                   using iterator = kss::io::stream::OutputIterator<Stream, T>;
                   iterator begin() { return iterator(*this); }
                 
                 @param stream the stream that will get the elements inserted.
                 */
                explicit OutputIterator(Stream& stream) : _stream(&stream) {}
                OutputIterator(const OutputIterator& it) : _stream(it._stream) {}
                OutputIterator(OutputIterator&& it) {
                    _stream = it._stream;
                    it._stream = nullptr;
                }

                ~OutputIterator() noexcept = default;
                OutputIterator& operator=(const OutputIterator& it) = default;
                OutputIterator& operator=(OutputIterator&& it) noexcept {
                    if (&it != this) {
                        _stream = it._stream;
                        it._stream = nullptr;
                    }
                    return *this;
                }

                /*!
                 Assigning a value to the iterator inserts it into the container.
                 @param t the item to be inserted
                 @throws any exception that Stream::operator<<(t) will throw.
                 */
                OutputIterator& operator=(const T& t) {
                    *_stream << t;
                    return *this;
                }

                /*!
                 The remaining iterator operators are placeholders that do nothing.
                 */
                OutputIterator& operator*() noexcept { return *this; }
                OutputIterator& operator++() noexcept { return *this; }
                OutputIterator& operator++(int) noexcept { return *this; }

                /*!
                 Swap with another iterator.
                 */
                void swap(OutputIterator& b) noexcept {
                    if (this != &b) {
                        std::swap(_stream, b._stream);
                    }
                }

            private:
                Stream* _stream;
            };
        }
    }
}

/*!
 Swap two iterators.
 Note this is added to the std namespace so that the stl code will pick it up when necessary.

 @throws std::invalid_argument if the two iterators are not from the same container.
 */
namespace std {
    template <typename Stream, typename T>
    void swap(kss::io::stream::InputIterator<Stream, T>& a,
              kss::io::stream::InputIterator<Stream, T>& b)
    {
        a.swap(b);
    }

    template <typename Stream, typename T>
    void swap(kss::io::stream::OutputIterator<Stream, T>& a,
              kss::io::stream::OutputIterator<Stream, T>& b)
    {
        a.swap(b);
    }
}

#endif
