//
//  http_error_category.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <functional>
#include <mutex>
#include <unordered_map>

#include "http_error_category.hpp"

using namespace std;
using namespace kss::io::net;

namespace {
    struct HttpHash : private hash<int> {
        result_type operator()(HttpStatusCode sc) const noexcept {
            return hash<int>::operator()(static_cast<int>(sc));
        }
    };

    class HttpErrorCategory : public error_category {
    public:
        const char* name() const noexcept override {
            return "http";
        }

        string message(int val) const override {
            static unordered_map<HttpStatusCode, string, HttpHash> strings;
            static once_flag flag;

            call_once(flag, [] {
                // This is not very nice, but should make things more optimal. Don't forget to
                // change this number if the HTTP committee adds more codes to support.
                strings.reserve(41);

                // Informational 1xx
                strings[HttpStatusCode::Continue] = "Continue";
                strings[HttpStatusCode::SwitchingProtocols] = "Switching Protocols";

                // Successful 2xx
                strings[HttpStatusCode::OK] = "OK";
                strings[HttpStatusCode::Created] = "Created";
                strings[HttpStatusCode::Accepted] = "Accepted";
                strings[HttpStatusCode::NonAuthoritativeInformation] = "Non-Authoritative Information";
                strings[HttpStatusCode::NoContent] = "No Content";
                strings[HttpStatusCode::ResetContent] = "Reset Content";
                strings[HttpStatusCode::PartialContent] = "Partial Content";

                // Redirection 3xx
                strings[HttpStatusCode::MultipleChoices] = "Multiple Choices";
                strings[HttpStatusCode::MovedPermanently] = "Moved Permanently";
                strings[HttpStatusCode::Found] = "Found";
                strings[HttpStatusCode::SeeOther] = "See Other";
                strings[HttpStatusCode::NotModified] = "Not Modified";
                strings[HttpStatusCode::UseProxy] = "Use Proxy";
                strings[HttpStatusCode::UnusedButReserved1] = "(Unused)";
                strings[HttpStatusCode::TemporaryRedirect] = "Temporary Redirect";

                // Client error 4xx
                strings[HttpStatusCode::BadRequest] = "Bad Request";
                strings[HttpStatusCode::Unauthorized] = "Unauthorized";
                strings[HttpStatusCode::PaymentRequired] = "Payment Required";
                strings[HttpStatusCode::Forbidden] = "Forbidden";
                strings[HttpStatusCode::NotFound] = "Not Found";
                strings[HttpStatusCode::MethodNotAllowed] = "Method Not Allowed";
                strings[HttpStatusCode::NotAcceptable] = "Not Acceptable";
                strings[HttpStatusCode::ProxyAuthenticationRequred] = "Proxy Authentication Required";
                strings[HttpStatusCode::RequestTimeout] = "Request Timeout";
                strings[HttpStatusCode::Conflict] = "Conflict";
                strings[HttpStatusCode::Gone] = "Gone";
                strings[HttpStatusCode::LengthRequired] = "Length Required";
                strings[HttpStatusCode::PreconditionFailed] = "Precondition Failed";
                strings[HttpStatusCode::RequestEntityTooLarge] = "Request Entity Too Large";
                strings[HttpStatusCode::RequestURITooLong] = "Request-URI Too Long";
                strings[HttpStatusCode::UnsupportedMediaType] = "Unsupported Media Type";
                strings[HttpStatusCode::RequestedRangeNotSatisfiable] = "Requested Range Not Satisfiable";
                strings[HttpStatusCode::ExpectationFailed] = "Expectation Failed";

                // Server error 5xx
                strings[HttpStatusCode::InternalServerError] = "Internal Server Error";
                strings[HttpStatusCode::NotImplemented] = "Not Implemented";
                strings[HttpStatusCode::BadGateway] = "Bad Gateway";
                strings[HttpStatusCode::ServiceUnavailable] = "Service Unavailable";
                strings[HttpStatusCode::GatewayTimeout] = "Gateway Timeout";
                strings[HttpStatusCode::HTTPVersionNotSupported] = "HTTP Version Not Supported";
            });

            // Search for the string. If not found return an unknown.
            auto it = strings.find(static_cast<HttpStatusCode>(val));
            return (it == strings.end() ? "unknown error" : it->second.c_str());
         }
    };
}


const error_category& kss::io::net::httpErrorCategory() noexcept {
    static HttpErrorCategory instance;
    return instance;
}
