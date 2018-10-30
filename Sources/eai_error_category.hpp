//
//  eai_error_category.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-08-21.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_eai_error_category_hpp
#define kssio_eai_error_category_hpp

#include <system_error>

namespace kss {
    namespace io {
        namespace net {
            
            /*!
             This method returns an std::error_category that describes the error codes returned
             by the getaddrinfo method. It is used to allow C++ network wrappers to throw
             std::system_error exceptions with the EAI error codes.
             */
            const std::error_category& eaiErrorCategory() noexcept;
            
            /*!
             This method returns an error code given a EAI error code number. The returned
             error code will be for a system_category() error if the EAI error code is
             EAI_SYSTEM and will be for an eai_category() error for all other EAI error codes.
             */
            std::error_code eaiErrorCode(int error) noexcept;
        }
    }
}

#endif 
