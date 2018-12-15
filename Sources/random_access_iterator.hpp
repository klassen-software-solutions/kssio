//
//  random_access_iterator.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-10-24.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
//  "borrowed" from kssutil
//

#ifndef kssio_random_access_iterator_hpp
#define kssio_random_access_iterator_hpp

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "add_rel_ops.hpp"
#include "utility.hpp"

namespace kss { namespace io { namespace _private {

    /*!
     Copy version of the random access iterator. The comments for random_access_iterator
     follow for this one as well.

     This iterator is for containers that don't necessarily contain their elements but
     may generate them on an as-needed basis. Specifically if operator[] returns a copy
     of an element instead of a reference to one, then this iterator will be needed.

     Note that this iterator is virtually the same as the ConstRandomAccessIterator
     except that the pointer type will be a std::shared_ptr<const value_type> and
     there is no reference type.
     */
    template <typename Container>
    class CopyRandomAccessIterator
    : public std::iterator<
        std::random_access_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        std::shared_ptr<const typename Container::value_type>,
        void >,
      public AddRelOps<CopyRandomAccessIterator<Container>>
    {
    public:
        CopyRandomAccessIterator() = default;
        CopyRandomAccessIterator(const CopyRandomAccessIterator& rhs) = default;
        ~CopyRandomAccessIterator() noexcept = default;
        CopyRandomAccessIterator& operator=(const CopyRandomAccessIterator& it) noexcept = default;

        /*!
         Construct the iterator.
         @param isEnd if true the iterator will be set to the end of the container
         */
        explicit CopyRandomAccessIterator(const Container& container, bool isEnd = false)
        : _container(&container)
        {
            _pos = (isEnd ? container.size() : 0);

            // postconditions
            if (!(_container != nullptr)) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }

        /*!
         Two iterators are considered equal if they are both pointing to the same position
         of the same container. Note that this "missing" comparators are defined by
         subclassing from AddRelOps. Note that the preconditions are not checked except
         in debug mode as we expect these to be called a lot. This is a known and accepted
         departure from our normal coding rules.
         */
        bool operator==(const CopyRandomAccessIterator& rhs) const noexcept {
            return (_container == rhs._container && _pos == rhs._pos);
        }

        bool operator<(const CopyRandomAccessIterator& rhs) const noexcept {
            // preconditions
            assert(_container == rhs._container);

            return (_pos < rhs._pos);
        }

        /*!
         Dereference the iterators. Note that to obtain maximum efficiency (since this
         is expected to be done a lot) we do not check if _container and _pos are
         valid, except in debug mode. This is a known and accepted departure from our normal
         coding standards. If they are not, you can expect a SEGV or something similarly bad
         to happen.
         @throws any exception that Container::operator[] throws
         */
        typename Container::value_type operator*() const {
            // preconditions
            assert(_container != nullptr);
            assert(_pos < _container->size());

            return (*_container)[_pos];
        }

        std::shared_ptr<const typename Container::value_type> operator->() const {
            // preconditions
            assert(_container != nullptr);
            assert(_pos < _container->size());

            return std::shared_ptr<const typename Container::value_type>(new typename Container::value_type((*_container)[_pos]));
        }

        typename Container::value_type operator[](size_t i) const {
            // preconditions
            assert(_container != nullptr);
            assert((_pos+i) < _container->size());

            return (*_container)[_pos+i];
        }

        /*!
         Pointer arithmetic. For efficiency reasons we do not check if the resulting
         position is valid, except in debug mode. This is a known and accepted departure
         from our normal coding standards.
         */
        CopyRandomAccessIterator& operator++() noexcept {
            // preconditions
            assert(_container != nullptr);
            assert(_pos < _container->size());

            ++_pos;

            // postconditions
            assert(_container != nullptr);
            assert(_pos <= _container->size());
            return *this;
        }

        CopyRandomAccessIterator operator++(int) noexcept {
            CopyRandomAccessIterator tmp(*this);
            operator++();
            return tmp;
        }

        CopyRandomAccessIterator& operator--() noexcept {
            // preconditions
            assert(_container != nullptr);
            assert(_pos > 0);
            assert(_pos <= _container->size());

            --_pos;

            // postconditions
            assert(_container != nullptr);
            assert(_pos >= 0);
            assert(_pos < _container->size());
            return *this;
        }

        CopyRandomAccessIterator operator--(int) noexcept {
            CopyRandomAccessIterator tmp(_pos);
            operator--();
            return tmp;
        }

        CopyRandomAccessIterator& operator+=(typename Container::difference_type n) noexcept {
            // preconditions
            assert(_container != nullptr);

            (n >= 0 ? _pos += size_t(n) : _pos -= size_t(-n));

            // postconditions
            assert(_container != nullptr);
            assert(_pos <= _container->size());
            return *this;
        }

        CopyRandomAccessIterator& operator-=(typename Container::difference_type n) noexcept {
            // preconditions
            assert(_container != nullptr);

            (n >= 0 ? _pos -= size_t(n) : _pos += size_t(-n));

            // postconditions
            assert(_container != nullptr);
            assert(_pos <= _container->size());
            return *this;
        }

        CopyRandomAccessIterator operator+(typename Container::difference_type n) const noexcept
        {
            CopyRandomAccessIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        CopyRandomAccessIterator operator-(typename Container::difference_type n) const noexcept
        {
            CopyRandomAccessIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        typename Container::difference_type operator-(const CopyRandomAccessIterator& rhs) const noexcept
        {
            // preconditions
            assert(_container == rhs._container);
            
            return typename Container::difference_type(_pos >= rhs._pos ? _pos - rhs._pos : -(rhs._pos - _pos));
        }

        /*!
         Swap with another iterator.
         */
        void swap(CopyRandomAccessIterator& b) noexcept {
            if (this != &b) {
                std::swap(_container, b._container);
                std::swap(_pos, b._pos);
            }
        }

    private:
        const Container*    _container = nullptr;
        size_t              _pos = 0;
    };


    template <typename Container>
    CopyRandomAccessIterator<Container> operator+(typename Container::difference_type n,
                                                  const CopyRandomAccessIterator<Container>& c) noexcept
    {
        return (c + n);
    }

}}}


namespace std {
    /*!
     Swap two iterators.
     Note this is added to the std namespace so that the stl code will pick it up when necessary.

     @throws std::invalid_argument if the two iterators are not from the same container.
     */
    template <typename Container>
    void swap(kss::io::_private::CopyRandomAccessIterator<Container>& a,
              kss::io::_private::CopyRandomAccessIterator<Container>& b)
    {
        a.swap(b);
    }
}

#endif
