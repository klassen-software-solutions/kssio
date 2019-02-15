//
//  rolling_file.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-02-07.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <sstream>

#include <kss/io/_stringutil.hpp>
#include <kss/io/directory.hpp>
#include <kss/io/fileutil.hpp>
#include <kss/io/rolling_file.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::io::file;
using namespace kss::test;
using kss::io::_private::startsWith;
using kss::io::_private::endsWith;

namespace {
    class MyTestSuite : public TestSuite, public HasBeforeEach, public HasAfterEach {
    public:
        MyTestSuite(const string& name, TestSuite::test_case_list_t fns)
        : TestSuite(name, fns)
        {}

        void beforeEach() override {
            testDirectory = temporaryFilename("/tmp/KSSIORollingFileTest");
            ensurePath(testDirectory);
        }

        void afterEach() override {
            removePath(testDirectory, true);
        }

        string testDirectory;
    };

    void write110(ofstream& strm) {
        for (int i = 0; i < 110; ++i) {
            strm << "X";
        }
    }

    class MyListener : public RollingFileListener {
    public:
        ~MyListener() override = default;
        
        void willOpen(RollingFile&, const string&) override { ++willOpenCount; }
        void didClose(RollingFile&, const string&) override { ++didCloseCount; }

        void didOpen(RollingFile&, const string&, ofstream& strm) override {
            strm << "onOpen";
            ++didOpenCount;
        }

        void willClose(RollingFile&, const string&, ofstream& strm) override {
            strm << "onClose";
            ++willCloseCount;
        }

        int willOpenCount = 0;
        int didOpenCount = 0;
        int willCloseCount = 0;
        int didCloseCount = 0;
    };
}

static MyTestSuite ts("file::rolling_file", {
    make_pair("Single write fills file", [] {
        KSS_ASSERT(isEqualTo<size_t>(3, [] {
            MyTestSuite& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
            RollingFile file(100, ts.testDirectory + "/rollingFile");
            file.write([] (ofstream& strm) { write110(strm); });
            file.write([] (ofstream& strm) { write110(strm); });
            file.write([] (ofstream& strm) { write110(strm); });
            Directory d(ts.testDirectory);
            return d.size();
        }));
    }),
    make_pair("Listeners", [] {
        MyTestSuite& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
        MyListener listener;
        RollingFile file(100, ts.testDirectory + "/rollingFile");
        file.setListener(listener);
        file.write([] (ofstream& strm) { write110(strm); });
        file.write([] (ofstream& strm) { write110(strm); });
        file.write([] (ofstream& strm) { write110(strm); });
        KSS_ASSERT(listener.didOpenCount == 3);
        KSS_ASSERT(listener.willOpenCount == 3);
        KSS_ASSERT(listener.didCloseCount == 3);
        KSS_ASSERT(listener.willCloseCount == 3);

        Directory d(ts.testDirectory);
        KSS_ASSERT(d.size() == 3);
        for (const auto& fn : d) {
            stringstream output;
            processFile(d.name() + "/" + fn, [&output](ifstream& strm) {
                string s;
                strm >> s;
                output << s;
            });
            KSS_ASSERT(startsWith(output.str(), "onOpen"));
            KSS_ASSERT(endsWith(output.str(), "onClose"));
        }
    })
});
