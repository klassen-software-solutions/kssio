//
//  _tokenizer.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2012-12-21.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
//  "Borrowed" from kssutil.
//

#ifndef kssio_tokenizer_hpp
#define kssio_tokenizer_hpp

#include <string>
#include "iterator.hpp"

namespace kss {
    namespace io {
        namespace _private {
            
            /*!
             Provide a container-ish view of a string separated by tokens. This is loosly based
             on code published at http://stackoverflow.com/questions/236129/split-a-string-in-c.
             
             This provides either a stream based or an input iterator based view of the tokens.
             */
            class Tokenizer {
            public:
                /*!
                 Container/iterator based type definitions.
                 */
                using value_type = std::string;
                using iterator = kss::io::stream::InputIterator<Tokenizer, std::string>;

                /*!
                 Create the tokenizer for a given string and delimiter set. Note that if empty
                 tokens are not to be skipped then having two or more delimiters in a row
                 will lead to empty tokens.

                 @param s is the string to be split into tokens
                 @param delim is the set of delimiters
                 @param skipEmptyTokens if true then empty tokens are skipped.
                 @param start is the position in s to start searching
                 @param end is the position in s one after the position to end searching
                 @throws std::invalid_argument if delim is an empty string
                 @throws any exceptions that may be thrown by the std::string class
                 */
                explicit Tokenizer(const std::string& s, const std::string& delim = " \t\n\r",
                                   bool skipEmptyTokens = true,
                                   std::string::size_type start = 0,
                                   std::string::size_type end = std::string::npos);
                explicit Tokenizer(std::string&& s, const std::string& delim = " \t\n\r",
                                   bool skipEmptyTokens = true,
                                   std::string::size_type start = 0,
                                   std::string::size_type end = std::string::npos);
                ~Tokenizer() noexcept = default;

                /*!
                 Stream-type access. You should use this or iterator based, but generally not both.
                 (The iterators are written in terms of these methods.)
                 @throws kss::io::Eof if called after eof() will return true.
                 */
                Tokenizer& operator>>(std::string& token);
                bool eof() const noexcept { return (_lastPos > _end); }

                /*!
                 Iterator based access. Note that if you combine this and the stream based access
                 you will find that some tokens are "skipped" as these iterators are written in
                 terms of the stream based access.
                 */
                iterator begin() { return iterator(*this); }
                iterator end()   { return iterator(); }

            private:
                std::string             _s;
                std::string             _delim;
                bool                    _skipEmptyTokens;
                std::string::size_type  _lastPos;
                std::string::size_type  _end;
            };

        }
    }
}

#endif
