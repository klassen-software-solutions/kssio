//
//  testutils.hpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-02-19.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#ifndef testutils_h
#define testutils_h

#include <string>

#include <kss/io/_stringutil.hpp>
#include <kss/io/directory.hpp>

#include "ksstest.hpp"

// A test suite that adds information about where the program is running and the
// relative location of the project directory.
class TestSuiteWithDirectories : public kss::test::TestSuite, public kss::test::HasBeforeAll {
public:
    TestSuiteWithDirectories(const std::string& name, test_case_list_t fns) : TestSuite(name, fns) {}

    void beforeAll() override {
        using kss::io::_private::endsWith;

        currentWorkingDirectory = kss::io::file::getCwd();

        // Determine the location of the project directory
        projectDirectory = ".";    // when run from make check
        if (endsWith(currentWorkingDirectory, "/kssio/Build/Products/Debug")
            || endsWith(currentWorkingDirectory, "/kssio/Build/Products/Release"))
        {
            // when run from Xcode
            projectDirectory = "../../..";
        }
    }

    std::string currentWorkingDirectory;
    std::string projectDirectory;
};

#endif
