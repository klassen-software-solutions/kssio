//
//  yaml_error_category.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-12-01.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef yaml_error_category_hpp
#define yaml_error_category_hpp

#include <system_error>

namespace kss { namespace io { namespace stream { namespace yaml {

    /*!
     Category used to identify error codes from libyaml.
     */
    const std::error_category& yamlErrorCategory() noexcept;
    
}}}}

#endif /* yaml_error_category_hpp */
