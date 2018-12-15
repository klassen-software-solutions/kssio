//
//  add_rel_ops.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-04-26.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// "borrowed" from kssutil

#ifndef kssio_add_rel_ops_h
#define kssio_add_rel_ops_h

#include <utility>

namespace kss { namespace io { namespace _private {

    /*!
     By subclassing your type T from add_rel_ops<T> this will add the "missing" operators
     provided that T defines operator== and operator<.

     This is similar to "using namespace rel_ops;" but does not have the potential
     side effects of adding a namespace.
     */
    template <class T>
    struct AddRelOps {
        inline bool operator!=(const T& t) const noexcept {
            const T* self = static_cast<const T*>(this);
            return !(*self == t);
        }

        inline bool operator<=(const T& t) const noexcept {
            const T* self = static_cast<const T*>(this);
            return (*self < t || *self == t);
        }

        inline bool operator>(const T& t) const noexcept {
            const T* self = static_cast<const T*>(this);
            return (!(*self == t) && !(*self < t));
        }

        inline bool operator>=(const T& t) const noexcept {
            const T* self = static_cast<const T*>(this);
            return !(*self < t);
        }
    };

}}}

#endif
