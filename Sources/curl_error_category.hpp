//
//  curl_error_category.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_curl_error_category_hpp
#define kssio_curl_error_category_hpp

#include <system_error>

namespace kss {
    namespace io {
        namespace net {

            /*!
             Catgegory used to identify error codes from libcurl.
             */
            const std::error_category& curlErrorCategory() noexcept;
            
        }
    }
}

#endif
