//
//  file_tree_walk.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-02-19.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <unordered_set>

#include <kss/io/file_tree_walk.hpp>
#include <kss/io/fileutil.hpp>
#include <kss/test/all.h>

#include "testutils.hpp"

using namespace std;
using namespace kss::io::file;
using namespace kss::test;


static TestSuiteWithDirectories ts("file::file_tree_walk", {
    make_pair("filename only tests", [] {
        auto& ts = dynamic_cast<TestSuiteWithDirectories&>(TestSuite::get());

        // Ignoring hidden files should filter out something. (Since at least the .git
        // directory will exist and get ignored.)
        KSS_ASSERT(isTrue([&] {
            int countWithoutHiddenFiles = 0;
            int countWithHiddenFiles = 0;

            fileTreeWalk(ts.projectDirectory, [&](const string& pathToFile) {
                ++countWithHiddenFiles;
            });

            fileTreeWalk(ts.projectDirectory, true, [&](const string& pathToFile) {
                ++countWithoutHiddenFiles;
            });

            return (countWithHiddenFiles > countWithoutHiddenFiles);
        }));

        // Including hidden files should find the file named .gitignore.
        KSS_ASSERT(isTrue([&] {
            bool foundIt = false;
            fileTreeWalk(ts.projectDirectory, [&](const string& pathToFile) {
                if (basename(pathToFile) == ".gitignore") {
                    foundIt = true;
                }
            });
            return foundIt;
        }));

        // Excluding hidden files should not find the file named .gitignore.
        KSS_ASSERT(isFalse([&] {
            bool foundIt = false;
            fileTreeWalk(ts.projectDirectory, true, [&](const string& pathToFile) {
                if (basename(pathToFile) == ".gitignore") {
                    foundIt = true;
                }
            });
            return foundIt;
        }));
    }),
    make_pair("streaming tests", [] {
        // Test of reading. The stream version should return the same set of files
        // as the filename only version. And for each file we should be able to read
        // the stream. (This test relies on the fact that every file in the Tests
        // directory should include its filename as part of the file header.)
        auto& ts = dynamic_cast<TestSuiteWithDirectories&>(TestSuite::get());

        unordered_set<string> filenamesNotFound;
        const auto dirPath = ts.projectDirectory + "/Tests";
        fileTreeWalk(dirPath, [&](const string& pathToFile) {
            filenamesNotFound.insert(pathToFile);
        });

        bool foundANewFile = false;
        unordered_set<string> filenamesNotReadProperly;
        fileTreeWalk(dirPath, [&](const string& pathToFile, ifstream& strm) {
            auto it = filenamesNotFound.find(pathToFile);
            if (it == filenamesNotFound.end()) {
                foundANewFile = true;
            }
            else {
                filenamesNotFound.erase(it);
                const string filename = basename(pathToFile);
                string line;
                while (getline(strm, line)) {
                    if (line.find(filename) != string::npos) {
                        // found it
                        return;
                    }
                }
                filenamesNotReadProperly.insert(pathToFile);
            }
        });

        // The stream version should have found all the files the name only version found.
        KSS_ASSERT(filenamesNotFound.empty());

        // The stream version should not have found anything the name only version missed.
        KSS_ASSERT(foundANewFile == false);

        // Every file in the stream version should have been readable.
        KSS_ASSERT(filenamesNotReadProperly.empty());
        if (!filenamesNotReadProperly.empty()) {
            cerr << "Failed to find the name in the following files:\n";
            for (const auto& fn : filenamesNotReadProperly) {
                cerr << "  " << fn << endl;
            }
        }
    }),
    make_pair("stat tests", [] {
        auto& ts = dynamic_cast<TestSuiteWithDirectories&>(TestSuite::get());
        // The stat version should return at least a value for each file.
        KSS_ASSERT(isTrue([&] {
            const auto dirPath = ts.projectDirectory + "/Tests";
            int fileCount = 0;
            fileTreeWalk(dirPath, true, [&](const string&) {
                ++fileCount;
            });

            int statCount = 0;
            fileTreeWalk(dirPath, true, [&](const string& pathToFile, const struct stat&) {
                ++statCount;
            });

            return (statCount >= fileCount);
        }));
    })
});
