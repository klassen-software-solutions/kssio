//
//  http_error_category.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_http_error_category_hpp
#define kssio_http_error_category_hpp

#include <system_error>

namespace kss {
    namespace io {
        namespace net {

            /*!
             List of the standard HTTP status codes. See
             http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
             for a complete discussion of their meaning.
             */
            enum class HttpStatusCode {
                // Informational 1xx
                Continue = 100,
                SwitchingProtocols = 101,

                // Successful 2xx
                OK = 200,
                Created = 201,
                Accepted = 202,
                NonAuthoritativeInformation = 203,
                NoContent = 204,
                ResetContent = 205,
                PartialContent = 206,

                // Redirection 3xx
                MultipleChoices = 300,
                MovedPermanently = 301,
                Found = 302,
                SeeOther = 303,
                NotModified = 304,
                UseProxy = 305,
                UnusedButReserved1 = 306,
                TemporaryRedirect = 307,

                // Client error 4xx
                BadRequest = 400,
                Unauthorized = 401,
                PaymentRequired = 402,
                Forbidden = 403,
                NotFound = 404,
                MethodNotAllowed = 405,
                NotAcceptable = 406,
                ProxyAuthenticationRequred = 407,
                RequestTimeout = 408,
                Conflict = 409,
                Gone = 410,
                LengthRequired = 411,
                PreconditionFailed = 412,
                RequestEntityTooLarge = 413,
                RequestURITooLong = 414,
                UnsupportedMediaType = 415,
                RequestedRangeNotSatisfiable = 416,
                ExpectationFailed = 417,

                // Server error 5xx
                InternalServerError = 500,
                NotImplemented = 501,
                BadGateway = 502,
                ServiceUnavailable = 503,
                GatewayTimeout = 504,
                HTTPVersionNotSupported = 505
            };

            /*!
             The category used to identify HTTP error codes.
             */
            const std::error_category& httpErrorCategory() noexcept;
        }
    }
}

namespace std {

    /*!
     Specialization of is_error_code_enum to signify that http_error is an error code
     enumeration.
     */
    template<> struct is_error_code_enum<kss::io::net::HttpStatusCode> : true_type {};

    /*!
     Override of make_error_code to handle an http_error.
     */
    inline error_code make_error_code(kss::io::net::HttpStatusCode sc) {
        return error_code(static_cast<int>(sc), kss::io::net::httpErrorCategory());
    }
}

#endif
