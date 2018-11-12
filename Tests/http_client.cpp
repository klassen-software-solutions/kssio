//
//  http_client.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-08.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include <unistd.h>
#include <curl/curl.h>

#include <kss/io/curl_error_category.hpp>
#include <kss/io/http_client.hpp>

#include "ksstest.hpp"


using namespace std;
using namespace kss::io::net;
using namespace kss::test;

namespace {
    class OurResponseCallback : public HttpResponseListener {
    public:
        void httpResponseError(const error_code& err) override { code = err; }
        ostream* httpResponseOutputStream() override { return &ss; }
        void httpResponseHeaderReceived(HttpStatusCode status, http_header_t&& hdr) override {
            header = move(hdr);
            stat = status;
        }

        http_header_t   header;
        stringstream    ss;
        HttpStatusCode  stat;
        error_code      code;
    };

#if 0
    class TestSuiteWithServer : public TestSuite, public HasBeforeAll, public HasAfterAll {
    public:
        TestSuiteWithServer(const string& name, test_case_list fns) : TestSuite(name, fns) {}

        void beforeAll() override {
            pid = fork();
            if (pid == 0) {
                int ret = system("Tests/http_test_server.py >> http_test_server.log 2>&1");
                if (ret) {
                    throw runtime_error("system returned: " + to_string(ret));
                }
            }
        }

        void afterAll() override {
            if (pid != -1 && pid != 0) {
                kill(pid, SIGTERM);     // the parent
                kill(pid+1, SIGTERM);   // the shell
                kill(pid+2, SIGTERM);   // the pythong script
            }
        }

    private:
        int pid;
    };
#endif
}



//static TestSuiteWithServer ts("net::http_client", {
static TestSuite ts("net::http_client", {
    make_pair("construction and moving", [](TestSuite&) {
        HttpClient c1("http", "127.0.0.1", 8080);
        KSS_ASSERT(c1.get("/hello") == "Hello World");

        HttpClient c2(move(c1));
        KSS_ASSERT(c2.get("/hello") == "Hello World");

        HttpClient c3 = move(c2);
        KSS_ASSERT(c3.get("/hello") == "Hello World");
    }),
    make_pair("sync get and head", [](TestSuite&) {
        HttpClient c("http://127.0.0.1:8080/");
        KSS_ASSERT(throwsException<system_error>([&]{
            c.get("/notthere", http_header_t({ make_pair("MyHeader", "value") }));
        }));
        KSS_ASSERT(throwsException<system_error>([&]{ c.head("/notthere"); }));

        http_header_t respHdr;
        string resp = c.get("/hello",
                            http_header_t({ make_pair("MyHeader", "value") }),
                            &respHdr);
        KSS_ASSERT(resp == "Hello World");
        KSS_ASSERT(respHdr["Server"] == "BaseHTTP/0.3 Python/2.7.10");

        c.head("/hello", http_header_t({ make_pair("MyHeader", "value") }), &respHdr);
        KSS_ASSERT(respHdr["Server"] == "BaseHTTP/0.3 Python/2.7.10");
    }),
    make_pair("async get and head", [](TestSuite&) {
        HttpClient c("http://127.0.0.1:8080/");
        OurResponseCallback cb1, cb2, cb3, cb4;
        c.asyncGet("/notthere", http_header_t(), cb1);
        c.asyncHead("/notthere", http_header_t({ make_pair("MyHeader", "value") }), cb2);
        c.asyncGet("/hello", http_header_t(), cb3);
        c.asyncHead("/hello", http_header_t(), cb4);
        c.wait();

        KSS_ASSERT(cb1.stat == HttpStatusCode::NotFound);
        KSS_ASSERT(cb2.stat == HttpStatusCode::NotFound);
        KSS_ASSERT(cb3.stat == HttpStatusCode::OK);
        KSS_ASSERT(cb3.ss.str() == "Hello World");
        KSS_ASSERT(cb3.header["Server"] == "BaseHTTP/0.3 Python/2.7.10");
        KSS_ASSERT(cb4.stat == HttpStatusCode::OK);
        KSS_ASSERT(cb4.header["Server"] == "BaseHTTP/0.3 Python/2.7.10");
    }),
    make_pair("sync post and put", [](TestSuite&) {
        HttpClient c("http://127.0.0.1:8080/");
        http_header_t respHdr;
        stringstream s1, s2, s3, s4;
        s1 << "Hello world" << endl;
        s2 << "Hello world" << endl;
        s3 << "This is a test" << endl;
        s4 << "This is a test" << endl;
        KSS_ASSERT(throwsSystemErrorWithCategory(httpErrorCategory(), [&] {
            c.post("/notthere", http_header_t(), s1, &respHdr);
        }));
        KSS_ASSERT(throwsSystemErrorWithCode(make_error_code(HttpStatusCode::NotFound), [&] {
            c.put("/notthere", http_header_t(), s2, &respHdr);
        }));

        c.post("/hello", http_header_t(), s3, &respHdr);
        KSS_ASSERT(respHdr["Server"] == "BaseHTTP/0.3 Python/2.7.10");

        c.put("/hello", http_header_t(), s4, &respHdr);
        KSS_ASSERT(respHdr["Server"] == "BaseHTTP/0.3 Python/2.7.10");
    }),
    make_pair("async post and put", [](TestSuite&) {
        HttpClient c("http://127.0.0.1:8080/");
        OurResponseCallback cb1, cb2, cb3, cb4;
        stringstream s1, s2, s3, s4;
        s1 << "Hello world" << endl;
        s2 << "Hello world" << endl;
        s3 << "This is a test" << endl;
        s4 << "This is a test" << endl;

        c.asyncPost("/notthere", http_header_t({ make_pair("MyHeader", "value") }), s1, cb1);
        c.asyncPut("/notthere", http_header_t(), s2, cb2);
        c.asyncPost("/hello", http_header_t(), s3, cb3);
        c.asyncPut("/hello", http_header_t({ make_pair("MyHeader", "value") }), s4, cb4);
        c.wait();

        KSS_ASSERT(cb1.stat == HttpStatusCode::NotFound);
        KSS_ASSERT((bool)cb1.code == false);
        KSS_ASSERT(cb2.stat == HttpStatusCode::NotFound);
        KSS_ASSERT(cb3.stat == HttpStatusCode::OK);
        KSS_ASSERT(cb3.ss.str().empty());
        KSS_ASSERT(cb3.header["Server"] == "BaseHTTP/0.3 Python/2.7.10");
        KSS_ASSERT(cb4.stat == HttpStatusCode::OK);
        KSS_ASSERT(cb4.header["Server"] == "BaseHTTP/0.3 Python/2.7.10");
    }),
    make_pair("post convenience function", [](TestSuite&) {
        HttpClient c("http://127.0.0.1:8080/");
        post(c, "/notthere", "The fact that this will fail should be ignored", false);
        post(c, "/hello", "Hello there everyone", false);
        c.wait();
    }),
    make_pair("non-existent server sync", [](TestSuite&) {
        HttpClient c("http://no-such-machine/");
        stringstream s1, s2;
        s1 << "Hello world" << endl;
        s2 << "Hello world" << endl;
        KSS_ASSERT(throwsSystemErrorWithCode(error_code(CURLE_COULDNT_RESOLVE_HOST, curlErrorCategory()), [&] {
            c.get("/hi");
        }));
        KSS_ASSERT(throwsSystemErrorWithCategory(curlErrorCategory(), [&] {
            c.head("/hi");
        }));
        KSS_ASSERT(throwsException<system_error>([&] {
            c.put("/hi", http_header_t({ make_pair("MyHeader", "value") }), s1);
        }));
        KSS_ASSERT(throwsException<system_error>([&] {
            c.post("/hi", http_header_t(), s2);
        }));
    }),
    make_pair("non-existent server async", [](TestSuite&) {
        HttpClient c("http://no-such-machine/");
        OurResponseCallback cb1, cb2, cb3, cb4;
        stringstream s1, s2;
        s1 << "Hello world" << endl;
        s2 << "Hello world" << endl;
        c.asyncGet("/hello", http_header_t(), cb1);
        c.asyncHead("/hello", http_header_t(), cb2);
        c.asyncPut("/hello", http_header_t(), s1, cb3);
        c.asyncPost("/hello", http_header_t(), s2, cb4);
        post(c, "/hello11", "This error will also be ignored.", false);
        c.wait();

        const auto err = error_code(CURLE_COULDNT_RESOLVE_HOST, curlErrorCategory());
        KSS_ASSERT(cb1.code == err);
        KSS_ASSERT(cb2.code == err);
        KSS_ASSERT(cb3.code == err);
        KSS_ASSERT(cb4.code == err);
   })
});
