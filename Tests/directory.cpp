//
//  directory.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include <kss/io/_raii.hpp>
#include <kss/io/_stringutil.hpp>
#include <kss/io/directory.hpp>
#include <kss/io/fileutil.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::io::file;
using namespace kss::test;

using kss::io::_private::endsWith;
using kss::io::_private::finally;

namespace {
	mode_t getPermissions(const string& path) {
		struct stat buf;
		if (::stat(path.c_str(), &buf) == -1) {
			return 0;
		}
		else {
			return buf.st_mode & 0777;
		}
	}

    class MyTestSuite : public TestSuite, public HasBeforeAll {
    public:
        MyTestSuite(const string& name, test_case_list_t fns) : TestSuite(name, fns) {}

        void beforeAll() override {
            currentWorkingDirectory = getCwd();

            // Determine the location of the project directory
            projectDirectory = ".";    // when run from make check
            if (endsWith(currentWorkingDirectory, "/kssio/Build/Products/Debug")
                || endsWith(currentWorkingDirectory, "/kssio/Build/Products/Release"))
            {
                // when run from Xcode
                projectDirectory = "../../..";
            }

            testSourcesDirectory = projectDirectory + "/Tests";
        }

        string currentWorkingDirectory;
        string testSourcesDirectory;
        string projectDirectory;
    };
}


static MyTestSuite ts("file::directory", {
    make_pair("getCwd", [] {
        auto& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
        const auto cwd = ts.currentWorkingDirectory;
        KSS_ASSERT(endsWith(cwd, "/kssio")                              // when run from make check
                   || endsWith(cwd, "/kssio/Build/Products/Debug")      // when run from Xcode (Debug)
                   || endsWith(cwd, "/kssio/Build/Products/Release"));  // when run from Xcode (Release)
    }),
    make_pair("Directory", [] {
        auto& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
        const auto srcdir = ts.testSourcesDirectory;

        // Test the iterators.
        size_t dircount = 0;
        size_t filecount = 0;
        Directory d(srcdir);
        for (Directory::const_iterator it = d.begin(), last = d.end(); it != last; ++it) {
            string name = d.name() + "/" + *it;
            if (isFile(name)) { ++filecount; }
            if (isDirectory(name)) { ++dircount; }
        }

        KSS_ASSERT(d.name() == srcdir);
        KSS_ASSERT(d.size() > 0);
        KSS_ASSERT(!d.empty());
        KSS_ASSERT(dircount == 0);
        KSS_ASSERT(filecount >= 15);

        // Iterators in the for loop format.
        dircount = filecount = 0;
        for (const string& fn : d) {
            string name = d.name() + "/" + fn;
            if (isFile(name)) { ++filecount; }
            if (isDirectory(name)) { ++dircount; }
        }
        KSS_ASSERT(dircount == 0);
        KSS_ASSERT(filecount >= 15);

        // Two iterators at the same time.
        {
            auto it1 = d.begin();
            auto it2 = d.begin();
            auto last = d.end();
            while (it1 != last) {
                KSS_ASSERT(*it1 == *it2);
                ++it1;
                ++it2;
            }
        }

        Directory d2(d.name());
        Directory d3(d.name() + "/../Tests");
        Directory d4("/tmp");
        KSS_ASSERT(d == d2);
        KSS_ASSERT(d == d3);
        KSS_ASSERT(d != d4);
    }),
    make_pair("ensurePath", [] {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

        finally cleanup([]{
            system("rm -rf /tmp/kssutiltest");
            system("rm -rf aaa");
            system("rm -rf hi");
            system("rm -f tmp/kssutiltest_file.dat");
        });

        string path = "/tmp/kssiotest/one/two/three/four";
        ensurePath(path);
        KSS_ASSERT(isDirectory(path));
        KSS_ASSERT(getPermissions(path) == 0755);

        path = "aaa/bbb";
        ensurePath(path);
        KSS_ASSERT(isDirectory(path));
        KSS_ASSERT(isDirectory(getCwd() + "/" + path));
        KSS_ASSERT(getPermissions(path) == 0755);

        path = "hi";
        ensurePath(path, 0700);
        KSS_ASSERT(isDirectory(path));
        KSS_ASSERT(isDirectory(getCwd() + "/" + path));
        KSS_ASSERT(getPermissions(path) == 0700);

        KSS_ASSERT(throwsException<system_error>([] {
            ensurePath("/no/permission");
        }));

        system("touch /tmp/kssutiltest_file.dat");
        KSS_ASSERT(throwsException<system_error>([] {
            ensurePath("/tmp/kssutiltest_file.dat");
        }));
#pragma GCC diagnostic pop
    }),
    make_pair("Bug16 Improper handling of empty directories", [] {
        KSS_ASSERT(isTrue([&] {
            const string path = temporaryFilename("/tmp/KSSIODirectoryTest");
            finally cleanup([&path] {
                removePath(path, true);
            });
            ensurePath(path);
            Directory d(path);
            return d.empty();
        }));
    }),
    make_pair("Enhancement17 Ignore hidden files", [] {
        // The project directory will have at least the hidden entries of ".git",
        // ".gitignore", and ".prereqs".
        auto& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
        Directory allEntries(ts.projectDirectory);
        Directory nonHiddenEntries(ts.projectDirectory, true);
        KSS_ASSERT(allEntries.size() >= (nonHiddenEntries.size() + 3));

        // The two directory objects will not be considered equivalent, even though
        // they have the same directory.
        KSS_ASSERT(allEntries != nonHiddenEntries);
    }),
    make_pair("removePath", [] {
        // Empty directory can be removed.
        KSS_ASSERT(isTrue([] {
            string path = temporaryFilename("/tmp/KSSIORemovePathTest");
            ensurePath(path);
            assert(isDirectory(path));
            removePath(path);
            return !exists(path);
        }));

        // An exception is thrown if the path name is empty.
        KSS_ASSERT(throwsException<invalid_argument>([] {
            removePath("");
        }));

        // An exception is thrown if the path refers to something other than a directory.
        KSS_ASSERT(throwsException<invalid_argument>([] {
            string fileName = temporaryFilename("/tmp/KSSIORemovePathTest");
            finally cleanup([&] {
                if (isFile(fileName)) {
                    unlink(fileName.c_str());
                }
            });

            ofstream strm(fileName);
            strm << "Hi!\n";
            strm.close();
            assert(isFile(fileName));
            removePath(fileName);
        }));

        // Non empty directory throws exception unless recursive is specified.
        {
            const auto path = temporaryFilename("/tmp/KSSIORemovePathTest");
            ensurePath(path);
            assert(isDirectory(path));

            const auto fileName = path + "/someFile";
            ofstream strm(fileName);
            strm << "Hi!\n";
            strm.close();
            assert(isFile(fileName));

            KSS_ASSERT(throwsException<system_error>([&] { removePath(path); }));
            removePath(path, true);
            KSS_ASSERT(!exists(path));
        }

        // Hidden files count as making the directory non-empty.
        {
            const auto path = temporaryFilename("/tmp/KSSIORemovePathTest");
            ensurePath(path);
            assert(isDirectory(path));

            const auto fileName = path + "/.someFile";
            ofstream strm(fileName);
            strm << "Hi!\n";
            strm.close();
            assert(isFile(fileName));

            KSS_ASSERT(throwsException<system_error>([&] { removePath(path); }));
            removePath(path, true);
            KSS_ASSERT(!exists(path));
        }
    })
});
