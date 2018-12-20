//
//  http_client.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-05.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>

#include <syslog.h>
#include <curl/curl.h>

#include "_action_queue.hpp"
#include "_contract.hpp"
#include "_raii.hpp"
#include "_stringutil.hpp"
#include "_tokenizer.hpp"
#include "curl_error_category.hpp"
#include "http_client.hpp"


using namespace std;
using namespace kss::io;
using namespace kss::io::net;

namespace contract = kss::io::contract;

using kss::io::_private::ActionQueue;
using kss::io::_private::finally;
using kss::io::_private::startsWith;
using kss::io::_private::Tokenizer;
using kss::io::_private::rtrim;
using kss::io::_private::trim;


// MARK: HttpClient

namespace {
    struct request {
        short                   op = 0;
        string                  uri;
        http_header_t           header;
        istream*                data = nullptr;
        HttpResponseListener*   callback = nullptr;

        request() = default;
        request(const request&) = delete;
        request(request&& req) {
            op = req.op;
            uri = move(req.uri);
            header = move(req.header);
            data = req.data;
            req.data = nullptr;
            callback = req.callback;
            req.callback = nullptr;
        }
    };

    struct response {
        const request*  req = nullptr;
        HttpStatusCode  status = HttpStatusCode::OK;
        http_header_t   headers;
        ostream*        data = nullptr;
        bool            haveStartedHeaderRead = false;
        bool            haveStartedDataRead = false;
    };

    // Callback for curl to read the request data.
    size_t readRequestCallback(char* buffer, size_t size, size_t nitems, void* userdata) noexcept
    {
        // Note that we are using preconditions instead of parameters since we cannot
        // allow this to throw exceptions. These should confirm that curl is calling
        // the callbacks properly.
        contract::preconditions({
            KSS_EXPR(buffer != nullptr),
            KSS_EXPR((size * nitems) != 0),
            KSS_EXPR(userdata != nullptr)
        });

        streamsize n = 0;
        try {
            request* req = reinterpret_cast<request*>(userdata);
            if (req->data) {
                if (!(*req->data)) {
                    return 0;
                }
                req->data->read(buffer, streamsize(size*nitems));
                n = req->data->gcount();
            }
        }
        catch (const exception& e) {
            syslog(LOG_WARNING, "Exception in %s: %s", __func__, e.what());
        }
        return (size_t)n;
    }

    // Callback to write header data into the response. If there are multiple headers
    // of the same name, we append the additional ones in a comma separated lists
    // preserving the order they were found. In addition we create a fake entry called
    // "Response" which contains the HTTP version and response code.
    size_t readHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) noexcept
    {
        // Note that we are using preconditions instead of parameters since we cannot
        // allow this to throw exceptions. These should confirm that curl is calling
        // the callbacks properly.
        contract::preconditions({
            KSS_EXPR(buffer != nullptr),
            KSS_EXPR((size * nitems) != 0),
            KSS_EXPR(userdata != nullptr)
        });

        try {
            response* resp = reinterpret_cast<response*>(userdata);
            if (!resp->haveStartedHeaderRead) {
                resp->haveStartedHeaderRead = true;
            }

            string headerdata(buffer, size*nitems);
            trim(headerdata);

            if (startsWith(headerdata, "HTTP/")) {              // response code
                Tokenizer t(headerdata);
                string http, responseCode;
                t >> http;
                t >> responseCode;
                resp->status = static_cast<HttpStatusCode>(stoi(responseCode));
                resp->headers["Response"] = headerdata;
            }
            else if (!headerdata.empty()) {                     // remaining header lines
                size_t colonPos = headerdata.find(':');
                if (colonPos == string::npos) {
                    syslog(LOG_WARNING, "Bad header, skipping: %s", headerdata.c_str());
                }
                else {
                    string key = headerdata.substr(0, colonPos);
                    string value = headerdata.substr(colonPos+1);
                    trim(key);
                    trim(value);

                    http_header_t::iterator it = resp->headers.find(key);
                    if (it == resp->headers.end()) {
                        resp->headers[key] = value;
                    }
                    else {
                        it->second += ("," + value);
                    }
                }
            }
        }
        catch (const invalid_argument& e) {
            syslog(LOG_WARNING, "Could not parse the status code, skipping: %s", e.what());
        }
        catch (const Eof& e) {
            syslog(LOG_WARNING, "Could not find the status code, skipping: %s", e.what());
        }
        catch (const exception& e) {
            syslog(LOG_WARNING, "Exception in %s: %s", __func__, e.what());
        }
        return size*nitems;
    }

    // Callback to write response data into the response stream.
    size_t readResponseCallback(char* ptr, size_t size, size_t nmemb, void* userdata) noexcept {
        // Note that we are using preconditions instead of parameters since we cannot
        // allow this to throw exceptions. These should confirm that curl is calling
        // the callbacks properly.
        contract::preconditions({
            KSS_EXPR(ptr != nullptr),
            KSS_EXPR((size * nmemb) != 0),
            KSS_EXPR(userdata != nullptr)
        });

        try {
            // If we are just starting the data read, we can send out the header response.
            response* resp = reinterpret_cast<response*>(userdata);
            if (!resp->haveStartedDataRead) {
                resp->haveStartedDataRead = true;
                if (resp->req->callback) {
                    resp->req->callback->httpResponseHeaderReceived(resp->status,
                                                                    move(resp->headers));
                }
            }

            // Read the next chunk of data.
            if (resp->req->callback) {
                ostream* os = resp->req->callback->httpResponseOutputStream();
                if (os) {
                    os->write(ptr, streamsize(size*nmemb));
                }
            }
        }
        catch (const exception& e) {
            syslog(LOG_WARNING, "Exception in %s: %s", __func__, e.what());
        }
        return size*nmemb;
    }
}

struct HttpClient::Impl {
    string      url;
    ActionQueue pendingActions;

    // Items protected by the sending lock.
    mutex       curlLock;
    CURL*       curl = nullptr;

    Impl(size_t maxQueueSize) : pendingActions(ActionQueue(maxQueueSize)) {}
    void sendRequest(request& req);
};

// Send a request.
namespace {

    template <class Param>
    inline void setopt(CURL* curl, CURLoption opt, Param value) {
        CURLcode ccerr = curl_easy_setopt(curl, opt, value);
        if (ccerr) {
            throw system_error(ccerr, curlErrorCategory(), "curl_easy_setop");
        }
    }

    inline void perform(CURL* curl) {
        CURLcode ccerr = curl_easy_perform(curl);
        if (ccerr) {
            throw system_error(ccerr, curlErrorCategory(), "curl_easy_perform");
        }
    }
}

void HttpClient::Impl::sendRequest(request &req) {

    lock_guard<mutex> l(curlLock);
    {
        contract::preconditions({
            KSS_EXPR(curl != nullptr),
        });

        curl_slist* hdrs = nullptr;

        finally cleanup([&]{
            curl_slist_free_all(hdrs);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
        });

        // Options common to all methods.
        curl_easy_reset(curl);
        setopt(curl, CURLOPT_URL, req.uri.c_str());
        setopt(curl, CURLOPT_FAILONERROR, 0);

        // Handle the operation.
        switch (req.op) {
            case (short)Operation::Get:
                setopt(curl, CURLOPT_HTTPGET, 1);
                break;
            case (short)Operation::Head:
                setopt(curl, CURLOPT_NOBODY, 1);
                break;
            case (short)Operation::Put:
                setopt(curl, CURLOPT_UPLOAD, 1);
                break;
            case (short)Operation::Post:
                setopt(curl, CURLOPT_POST, 1);
                break;
            default:
                throw system_error(make_error_code(HttpStatusCode::BadRequest),
                                   "unrecognized HTTP operation");
        }

        // Setup the headers.
        if (!req.header.empty()) {
            for (const auto& h : req.header) {
                hdrs = curl_slist_append(hdrs, (h.first + ": " + h.second).c_str());
            }
            setopt(curl, CURLOPT_HTTPHEADER, hdrs);
        }

        // Setup the callback that will send the request data.
        if (req.data) {
            setopt(curl, CURLOPT_READFUNCTION, readRequestCallback);
            setopt(curl, CURLOPT_READDATA, &req);
        }

        // Setup the callbacks that will read the response.
        response resp;
        resp.req = &req;
        setopt(curl, CURLOPT_HEADERFUNCTION, readHeaderCallback);
        setopt(curl, CURLOPT_HEADERDATA, &resp);
        setopt(curl, CURLOPT_WRITEFUNCTION, readResponseCallback);
        setopt(curl, CURLOPT_WRITEDATA, &resp);

        // Perform the operation. Note that if no data was read then we must call the
        // header callback at this point.
        perform(curl);
        if (resp.haveStartedHeaderRead && !resp.haveStartedDataRead && resp.req->callback) {
            resp.req->callback->httpResponseHeaderReceived(resp.status, move(resp.headers));
        }
    }
}


// Constructors
HttpClient::HttpClient() {
    _impl = nullptr;
}

HttpClient::HttpClient(const string& url, size_t maxQueueSize) : _impl(new Impl(maxQueueSize)) {
    _impl->curl = curl_easy_init();
    if (!_impl->curl) {
        delete _impl;
        throw system_error(CURLE_FAILED_INIT, curlErrorCategory(), "curl_easy_init");
    }
    
    _impl->url = url;
    rtrim(_impl->url, '/');
}

HttpClient::HttpClient(const string& protocol, const string& machine, unsigned port,
                       size_t maxQueueSize)
: HttpClient(protocol + "://" + machine + ":" + to_string(port), maxQueueSize)
{
}

HttpClient::HttpClient(HttpClient&& c) {
    _impl = c._impl;
    c._impl = nullptr;
}

HttpClient::~HttpClient() noexcept {
    try {
        if (_impl) {
            _impl->pendingActions.wait();

            {
                lock_guard<mutex> l(_impl->curlLock);
                curl_easy_cleanup(_impl->curl);
            }
            delete _impl;
        }
    }
    catch (const exception& e) {
        // Best we can do is log the error and continue.
        syslog(LOG_ERR, "[%s] Exception shutting down: %s", __PRETTY_FUNCTION__, e.what());
    }
}

HttpClient& HttpClient::operator=(HttpClient&& c) noexcept {
    if (&c != this) {
        _impl = c._impl;
        c._impl = nullptr;
    }
    return *this;
}

// Synchronous actions.
namespace {
    // Handler used to read the response for our synchronous requests.
    class SynchronousResponseListener : public HttpResponseListener {
    public:
        SynchronousResponseListener() {
            _error = HttpStatusCode::OK;
        }

        void httpResponseHeaderReceived(HttpStatusCode status,
                                        http_header_t&& header) override
        {
            _error = status;
            _header = move(header);
        }

        ostream* httpResponseOutputStream() override { return &_data; }

        HttpStatusCode  _error;
        http_header_t   _header;
        stringstream    _data;
    };

    // Package the request.
    request packageRequest(short op, const string& url, const string& path,
                           http_header_t&& requestHeader, istream* requestData,
                           HttpResponseListener* cb)
    {
        contract::parameters({
            KSS_EXPR(path.empty() || (path[0] == '/'))
        });

        request req;
        req.op = op;
        req.uri = url + (path.empty() ? string("/") : path);
        req.header = move(requestHeader);
        req.data = requestData;
        req.callback = cb;
        return req;
    }

    // Examine the results and return them.
    string examineResults(HttpResponseListener& listener, http_header_t* responseHeader) {
        SynchronousResponseListener& handler = dynamic_cast<SynchronousResponseListener&>(listener);
        if (handler._error >= HttpStatusCode::BadRequest) {
            throw system_error(make_error_code(handler._error));
        }
        if (responseHeader) {
            *responseHeader = move(handler._header);
        }
        return handler._data.str();
    }
}

string HttpClient::doSync(Operation op,
                          const string &path,
                          http_header_t&& requestHeader,
                          istream* requestData,
                          http_header_t* responseHeader)
{
    SynchronousResponseListener cb;
    request req = packageRequest(static_cast<short>(op), _impl->url, path,
                                 move(requestHeader), requestData, &cb);
    finally cleanup([&] {
        if (req.callback) {
            req.callback->httpResponseCompleted();
        }
    });

    _impl->sendRequest(req);
    return examineResults(cb, responseHeader);
}

void HttpClient::doAsync(Operation op, const string& path,
                         http_header_t &&requestHeader, istream* requestData,
                         HttpResponseListener& cb)
{
    // Package up an action to send the request, and add it to the action queue.
    _impl->pendingActions.addAction([this, path, requestData,
                                     op = static_cast<short>(op),
                                     header = move(requestHeader),
                                     cb = &cb]() mutable {
        request req = packageRequest(op, _impl->url, path, move(header), requestData, move(cb));
        
        finally cleanup([&] {
            if (req.callback) {
                req.callback->httpResponseCompleted();
            }
        });

        try {
            _impl->sendRequest(req);
        }
        catch (const system_error& e) {
            // In asynchronous mode we cannot throw an exeption. Instead we register
            // the error via the callback.
            if (req.callback) {
                req.callback->httpResponseError(e.code());
            }
        }
    });
}


// Wait for all pending operations to finish.
void HttpClient::wait() {
    _impl->pendingActions.wait();
}


////
//// _internal_post_response_callback implementation
////

void kss::io::net::_private::InternalPostResponseCallback::httpResponseError(const error_code &err)
{
    if (verbose) {
        syslog(LOG_WARNING, "Failure to post message, err=%s", err.message().c_str());
    }
}

void kss::io::net::_private::InternalPostResponseCallback::httpResponseHeaderReceived(HttpStatusCode status,
                                                                                      http_header_t&& header)
{
    if (verbose) {
        syslog(LOG_WARNING, "Failure to post message, err=%u", static_cast<unsigned>(status));
    }
}

void kss::io::net::_private::InternalPostResponseCallback::httpResponseCompleted() {
    delete this;
}
