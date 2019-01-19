//
//  _stringutil.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-05.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_stringutil_hpp
#define kssio_stringutil_hpp

#include <string>

namespace kss {
    namespace io {
        namespace _private {

            // Items "borrowed" from kssutil strings.
            std::string& ltrim(std::string& s) noexcept;
            std::string& ltrim(std::string& s, char c) noexcept;
            std::string& rtrim(std::string& s) noexcept;
            std::string& rtrim(std::string& s, char c) noexcept;

            inline std::string& trim(std::string& s) noexcept {
                return ltrim(rtrim(s));
            }

            inline std::string& trim(std::string& s, char c) noexcept {
                return ltrim(rtrim(s, c), c);
            }

            inline bool startsWith(const std::string& str, const std::string& prefix) noexcept
            {
                return (str.substr(0, prefix.size()) == prefix);
            }

            bool endsWith(const std::string& str, const std::string& suffix) noexcept;
        }
    }
}

#endif /* _stringutil_hpp */
