//
//  directory.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/stat.h>

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
}


static TestSuite ts("file::directory", {
    make_pair("getCwd", [] {
        string cwd = kss::io::file::getCwd();
        KSS_ASSERT(endsWith(cwd, "/kssio")                              // when run from make check
                   || endsWith(cwd, "/kssio/Build/Products/Debug")      // when run from Xcode (Debug)
                   || endsWith(cwd, "/kssio/Build/Products/Release"));  // when run from Xcode (Release)
    }),
    make_pair("Directory", [] {
        string cwd = kss::io::file::getCwd();
        string srcdir = "Tests";            // when run from make check
        if (endsWith(cwd, "/kssio/Build/Products/Debug")
            || endsWith(cwd, "/kssio/Build/Products/Release"))
        {
            srcdir = "../../../Tests";      // when run from Xcode (Debug or Release)
        }

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
        KSS_ASSERT(dircount >= 2);
        KSS_ASSERT(filecount >= 15);

        // Iterators in the for loop format.
        dircount = filecount = 0;
        for (const string& fn : d) {
            string name = d.name() + "/" + fn;
            if (isFile(name)) { ++filecount; }
            if (isDirectory(name)) { ++dircount; }
        }
        KSS_ASSERT(dircount >= 2);
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
    })
});
