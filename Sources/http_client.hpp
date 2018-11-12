//
//  http_client.h
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_http_client_hpp
#define kssio_http_client_hpp

#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>

#include "http_error_category.hpp"
#include "utility.hpp"


namespace kss {
    namespace io {
        namespace net {

            /*!
             HTTP headers, either sent or received, are handled by a map.
             */
            using http_header_t = std::unordered_map<std::string, std::string>;


            /*!
             HTTP response callback interface. A class to be used to read a response from
             the asynchronous methods needs to subclass from this interface. If any of
             the methods are not overridden, their default action is to do nothing.
             */
            class HttpResponseListener {
            public:
                /*!
                 Called if the request fails before we can even obtain an HTTP status. For
                 example, if we cannot connect to the sever. 
                 */
                virtual void httpResponseError(const std::error_code& err) {}

                /*!
                 Called when the response header has been completely read. Note that the 
                 header will have move semantics so that you may move instead of copy
                 its contents into your desired location.
                 */
                virtual void httpResponseHeaderReceived(HttpStatusCode status,
                                                        http_header_t&& header) {}

                /*!
                 Called before we start receiving the response in order to determine the
                 stream to write the response to. If this returns a nullptr (which is the
                 default) then the response will be ignored. Otherwise it is written to
                 this stream.
                 */
                virtual std::ostream* httpResponseOutputStream() { return nullptr; }

                /*!
                 Called when the entire operation, both the request and its response, have
                 been completed. At this point it will be safe for you to perform any
                 required cleanup, including the destruction of this object itself.
                 */
                virtual void httpResponseCompleted() {}
            };


            /*!
             HTTP/HTTPS client connection. This is largely a wrapper around libcurl
             written in a fashion that will minimize the copying of data by allowing
             stream access both for sending and receiving.
             */
            class HttpClient {
            public:
                static constexpr size_t noLimit = std::numeric_limits<size_t>::max();

                /*!
                 Construct a client with a connection to the given url. Note that there is
                 a "lazy connection" in that the connection to the url is not made until
                 an attempt is made to actually send something.
                 
                 The default constructor will create a client placeholder. But that will not
                 be useful until a valid one is "moved" into it.

                  - url the fully qualified url
                  - protocol, machine, port the items used to make a url.
                  - maxQueueSize, the maximum number of asynchronous operations allowed
                        to be pending.
                 */
                HttpClient();
                explicit HttpClient(const std::string& url, size_t maxQueueSize = noLimit);
                HttpClient(const std::string& protocol,
                           const std::string& machine,
                           unsigned port,
                           size_t maxQueueSize = noLimit);
                HttpClient(HttpClient&& c);
                HttpClient(const HttpClient&) = delete;
                ~HttpClient() noexcept;

                HttpClient& operator=(HttpClient&& c) noexcept;
                HttpClient& operator=(const HttpClient&) = delete;

                /*!
                 Synchronous actions. Note that if no connection to the server can be made,
                 these can cause long blocks (up to the connection timeout) in the current
                 thread. Also note that the get method will create a copy of its results
                 instead of using the more efficient stream access.
                 
                 - request_header the header values (other than the defaults) to be sent
                 - path the path that will be added to the url (may be empty if the url is sufficient)
                 - data the data to be sent
                 - response_header if not null the response header will be set into this object
                 @return any response other than the response header
                 @throws invalid_argument if path is not empty and does not start with a '/'
                 @throws system_error if there is a problem writing the request, or in
                    reading the response, or if the HTTP response is not an OK response.
                 */
                std::string get(const std::string& path,
                                http_header_t&& requestHeader = http_header_t(),
                                http_header_t* responseHeader = nullptr)
                {
                    return doSync(Operation::Get, path, std::move(requestHeader),
                                  nullptr, responseHeader);
                }
                void head(const std::string& path,
                          http_header_t&& requestHeader = http_header_t(),
                          http_header_t* responseHeader = nullptr)
                {
                    doSync(Operation::Head, path, std::move(requestHeader),
                           nullptr, responseHeader);
                }
                void put(const std::string& path,
                         http_header_t&& requestHeader,
                         std::istream& data,
                         http_header_t* responseHeader = nullptr)
                {
                    doSync(Operation::Put, path, std::move(requestHeader),
                           &data, responseHeader);
                }
                void post(const std::string& path,
                          http_header_t&& requestHeader,
                          std::istream& data,
                          http_header_t* responseHeader = nullptr)
                {
                    doSync(Operation::Post, path, std::move(requestHeader),
                           &data, responseHeader);
                }

                /*!
                 Asynchronous actions. These will cause requests to be written to an internal
                 queue and another thread will pick up the items and send them. In addition,
                 instead of sending and receiving strings you provide a stream for the input
                 data and read the response, other than the headers, from another stream.
                 
                 Note that the request_data streams and the response_callback items must remain
                 valid (in scope) until the request is actually processed.

                 - requestHeader the header values (other than the defaults) to be sent
                 - path the path that will be added to the url (may be empty if the url is sufficient)
                 - request_data a stream containing the request data
                 - cb the callback for processing the response. Note that this reference
                    must remain valid at least until the asynchronous call has been
                    completed.
                 @throws system_error if anything goes wrong writing the request. If this error
                    is EAGAIN it may be mean following, all of which imply trying again later:
                      - Another thread has called wait. No additional operations will be
                        accepted until the wait has completed.
                      - There are too many pending asynchronous operations (as defined by
                        maxQueueSize in the constructor). No additional operations will be
                        accepted until some of the existing ones have completed.
                 */
                void asyncGet(const std::string& path,
                              http_header_t&& requestHeader,
                              HttpResponseListener& cb)
                {
                    doAsync(Operation::Get, path, std::move(requestHeader), nullptr, cb);
                }
                void asyncHead(const std::string& path,
                               http_header_t&& requestHeader,
                               HttpResponseListener& cb)
                {
                    doAsync(Operation::Head, path, std::move(requestHeader), nullptr, cb);
                }
                void asyncPut(const std::string& path,
                              http_header_t&& requestHeader,
                              std::istream& requestData,
                              HttpResponseListener& cb)
                {
                    doAsync(Operation::Put, path, std::move(requestHeader), &requestData, cb);
                }
                void asyncPost(const std::string& path,
                               http_header_t&& requestHeader,
                               std::istream& requestData,
                               HttpResponseListener& cb)
                {
                    doAsync(Operation::Post, path, std::move(requestHeader), &requestData, cb);
                }

                /*!
                 Wait for any pending asynchronous operations to complete. You can use this
                 to ensure that everything is done before the client goes out of scope and
                 the destructor is called. Otherwise the sending thread may be shut down
                 and some operations lost. (Which may be acceptable for some applications.)
                
                 Note that attempts to make async calls (i.e. to add requests to the queue)
                 will fail with a system_error EAGAIN code until the wait has completed.
                 
                 Note that this is a thread interruption point.
                 */
                void wait();

            private:
                struct Impl;
                Impl* _impl;

                enum class Operation : short { Get = 1, Put = 2, Post = 3, Head = 4 };
                std::string doSync(Operation op, const std::string& path,
                                   http_header_t&& req_hdr, std::istream* req_data,
                                   http_header_t* resp_hdr);
                void doAsync(Operation op, const std::string& path,
                             http_header_t&& req_hdr, std::istream* req_data,
                             HttpResponseListener& cb);
            };


            namespace _private {
                // Callback used for the post convenience method. You should not use this directly.
                // This will log any errors when the header is received, will ignore any response,
                // and will delete itself when the request and response has concluded.
                class InternalPostResponseCallback : public HttpResponseListener {
                public:
                    explicit InternalPostResponseCallback(bool verb) : verbose(verb) {}
                    
                    void httpResponseError(const std::error_code& err) override;
                    void httpResponseHeaderReceived(HttpStatusCode status,
                                                    http_header_t&& header) override;
                    void httpResponseCompleted() override;

                    std::stringstream _requestData;

                private:
                    bool verbose = true;
                };
            }

            /*!
             HTTP/HTTPS post convenience function. This will post an object asynchronously
             by setting the Content-Type header using guess_mime_type then by streaming
             the object into the request_data stream of the async_post client method.
             
             Note that this is a "fire and forget" method. It does not process the response
             from the server other than to log a warning to syslog if something goes wrong.
             
             This will work for any type T such that guess_mime_type<T> provides the
             correct Content-Type and for which operator<<(ostream&, T) exists. In order 
             to use this with your own objects you will need to ensure that operator<<
             is written and that guess_mime_type is specialized to handle your type.
             
             Note that t will be written to a stringstream before the request is actually
             made. If t would be a very large request we recommend that you use async_post
             with an appropriate input stream instead.

             @param c the HTTP client object used to send the request
             @param path the path of the request (may be empty if the client url is sufficient)
             @param t the object to serialize to the stream and send.
             @param verbose if false then syslog will not be called, even for errors
             @throws system_error with EAGAIN if the queue is currently in wait mode, or if
                there are too many pending asynchronous operations. In either event, you
                can try again later.
             */
            template <class T>
            void post(HttpClient& c, const std::string& path, const T& t, bool verbose=true) {
                http_header_t headers {
                    std::make_pair("Content-Type", guessMimeType(t))
                };

                auto* cb = new _private::InternalPostResponseCallback(verbose);
                cb->_requestData << t;
                c.asyncPost(path, std::move(headers), cb->_requestData, *cb);
            }
        }
    }
}

#endif
