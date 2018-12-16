//
//  fileutil.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2013-02-07.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <kss/io/fileutil.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::io::file;
using namespace kss::test;

static int sockfd = -1;

static void teardown_fileutil_test(void) {
    unlink("/tmp/kssutiltest/_socketlink");
    close(sockfd);
    unlink("/tmp/kssutiltest/_socket");
    unlink("/tmp/kssutiltest/_fifolink");
    unlink("/tmp/kssutiltest/_fifo");
    unlink("/tmp/kssutiltest/_etc");
    unlink("/tmp/kssutiltest/_hosts");
    unlink("/tmp/kssutiltest/_not_exist");
    rmdir("/tmp/kssutiltest");
}

static void setup_fileutil_test(void) {
    teardown_fileutil_test();
    mkdir("/tmp/kssutiltest", S_IRWXU);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    symlink("/probably/does/not/exist", "/tmp/kssutiltest/_not_exist");
    symlink("/etc/hosts", "/tmp/kssutiltest/_hosts");
    symlink("/etc", "/tmp/kssutiltest/_etc");
    mkfifo("/tmp/kssutiltest/_fifo", 0);
    symlink("/tmp/kssutiltest/_fifo", "/tmp/kssutiltest/_fifolink");
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/kssutiltest/_socket");
    ::bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    symlink("/tmp/kssutiltest/_socket", "/tmp/kssutiltest/_socketlink");
#pragma GCC diagnostic pop
}

namespace {
    inline bool startsWith(const std::string& str, const std::string& prefix) noexcept {
        return (str.substr(0, prefix.size()) == prefix);
    }
}


static TestSuite ts("file::fileutil", {
    make_pair("basic tests", [](TestSuite&) {
        setup_fileutil_test();

        KSS_ASSERT(!exists("/probably/does/not/exist")
                   && !exists("/probably/does/not/exist", false));
        KSS_ASSERT(!exists("/tmp/kssutiltest/_not_exist")
                   && exists("/tmp/kssutiltest/_not_exist", false));

        KSS_ASSERT(!isSymbolicLink("/tmp/kssutiltest/_not_exist")
                   && isSymbolicLink("/tmp/kssutiltest/_not_exist", false));
        KSS_ASSERT(exists("/etc/hosts") && exists("/tmp/kssutiltest/_hosts", false));

        KSS_ASSERT(isFile("/etc/hosts") && isFile("/tmp/kssutiltest/_hosts", false));
        KSS_ASSERT(!isSymbolicLink("/etc/hosts") && !isSymbolicLink("/tmp/kssutiltest/_hosts"));
        KSS_ASSERT(!isSymbolicLink("/etc/hosts", false)
                   && isSymbolicLink("/tmp/kssutiltest/_hosts", false));

        KSS_ASSERT(exists("/etc") && isDirectory("/etc") && !isFile("/etc"));
        KSS_ASSERT(exists("/tmp/kssutiltest/_etc", true)
                   && !isDirectory("/tmp/kssutiltest/_etc", false)
                   && isSymbolicLink("/tmp/kssutiltest/_etc", false));

        KSS_ASSERT(exists("/tmp/kssutiltest/_fifo") && isPipe("/tmp/kssutiltest/_fifo"));
        KSS_ASSERT(exists("/tmp/kssutiltest/_fifolink", false)
                   && !isPipe("/tmp/kssutiltest/_fifolink", false)
                   && isSymbolicLink("/tmp/kssutiltest/_fifolink", false));

        // ischar - don't know how to create these in order to test them
        // isblock
        // iswhite

        KSS_ASSERT(exists("/tmp/kssutiltest/_socket") && isSocket("/tmp/kssutiltest/_socket"));
        KSS_ASSERT(exists("/tmp/kssutiltest/_socketlink", false)
                   && !isSocket("/tmp/kssutiltest/_socketlink", false)
                   && isSymbolicLink("/tmp/kssutiltest/_socketlink", false));

        teardown_fileutil_test();
    }),
    make_pair("temporaryFile", [](TestSuite&) {
        string prefix = "/tmp/kssutil";
        {
            string name = temporaryFilename(prefix);
            KSS_ASSERT(startsWith(name, prefix));
            KSS_ASSERT(!exists(name));
        }
        {
            FileGuard f1(temporaryFile(prefix));
            FileGuard f2(temporaryFile(prefix));
            KSS_ASSERT(f1.file() != nullptr);
            KSS_ASSERT(f2.file() != nullptr);
            KSS_ASSERT(f1.file() != f2.file());
        }
        {
            fstream f1 = temporaryFStream(prefix);
            KSS_ASSERT(f1.is_open());
            f1 << "hi";
            KSS_ASSERT(f1.good());

            fstream f2 = temporaryFStream(prefix);
            KSS_ASSERT(f2.is_open());
            f2 << "there";
            KSS_ASSERT(f2.good());
            KSS_ASSERT(f1.rdbuf() != f2.rdbuf());
        }
    }),
    make_pair("path manipulation", [](TestSuite&) {
        // basename
        KSS_ASSERT(basename("") == "");
        KSS_ASSERT(basename("/////") == "");
        KSS_ASSERT(basename("/this/is/a/directory/") == "");
        KSS_ASSERT(basename("helloworld.txt") == "helloworld.txt");
        KSS_ASSERT(basename("/this/is/a/path/file.ext") == "file.ext");
        KSS_ASSERT(basename("dir/file.ext") == "file.ext");
        KSS_ASSERT(basename("/file.ext") == "file.ext");

        // dirname
        KSS_ASSERT(dirname("") == "./");
        KSS_ASSERT(dirname("/////") == "/////");
        KSS_ASSERT(dirname("/this/is/a/directory/") == "/this/is/a/directory/");
        KSS_ASSERT(dirname("helloworld.txt") == "./");
        KSS_ASSERT(dirname("/this/is/a/path/file.ext") == "/this/is/a/path/");
        KSS_ASSERT(dirname("dir/file.ext") == "dir/");
        KSS_ASSERT(dirname("/file.ext") == "/");
    }),
    make_pair("FileGuard", [](TestSuite&){
        int data = 10;
        string prefix = "/tmp/fileguard";
        {
            FILE* f = fopen(temporaryFilename(prefix).c_str(), "w+");
            {
                FileGuard fg(f);
                KSS_ASSERT(fg.file() == f);
                KSS_ASSERT(fwrite(&data, sizeof(int), 1, f) == 1);  // file is open
            }
            // Unfortunately there is nothing that we can actually test here, other than that
            // we got this far without exceptions. I tried the following...
            //
            // KSS_ASSERT(fwrite(&data, sizeof(int), 1, f) < 1);
            //
            // which seems to work on MacOS and Debian Linux, but fails on CentOS Linux (the
            // fwrite returns a value of 1 implying that it actually wrote the file even
            // though it was closed). When I think about it, it is not a valid test on any
            // architecture as after the fclose that is performed by file_guard, the status of
            // f is undefined.
            KSS_ASSERT(true);
        }
        {
            int fd = open(temporaryFilename(prefix).c_str(), O_CREAT | O_RDWR, S_IWGRP | S_IWOTH);
            {
                FiledesGuard fg(fd);
                KSS_ASSERT(fg.filedes() == fd);
                KSS_ASSERT(write(fd, &data, sizeof(int)) == sizeof(int));   // file is open
            }
            KSS_ASSERT(write(fd, &data, sizeof(int)) == -1);                // file is closed
        }
    }),
    make_pair("file processing", [](TestSuite&) {
        // Create a test file. (This also acts as a test of writeFile.)
        const string filename = temporaryFilename("/tmp/fileprocessing");
        const string filename2 = temporaryFilename("/tmp/fileprocessing");
        writeFile(filename, [](ofstream& strm) {
            strm << "1 2 3 4" << endl;
            strm << "one two three four" << endl;
        });

        // File processing without errors.
        int count = 0;
        processFile(filename, [&](ifstream& strm) {
            string line;
            while (getline(strm, line)) {
                ++count;
            }
        });
        KSS_ASSERT(count == 2);

        count = 0;
        processFile(filename, LineByLine([&](const string&) {
            ++count;
        }));
        KSS_ASSERT(count == 2);

        count = 0;
        processFile(filename, LineByLine(' ', [&](const string&) {
            ++count;
        }));
        KSS_ASSERT(count == 7);

        count = 0;
        processFile(filename, CharByChar([&](char) {
            ++count;
        }));
        KSS_ASSERT(count == 27);

        // File writing with errors on open.
        KSS_ASSERT(throwsException<system_error>([] {
            writeFile("/no/such/file", [](ofstream&){});
        }));

        // File processing with errors on open.
        KSS_ASSERT(throwsException<system_error>([] {
            processFile("/no/such/file", [](ifstream&){});
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            processFile("/no/such/file", LineByLine([](const string&){}));
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            processFile("/no/such/file", LineByLine(' ', [](const string&){}));
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            processFile("/no/such/file", CharByChar([](char){}));
        }));

        // File processing with errors on read.
        KSS_ASSERT(throwsException<system_error>([&] {
            processFile(filename, [](ifstream& strm) { strm.setstate(ifstream::badbit); });
        }));
        KSS_ASSERT(throwsException<runtime_error>([&] {
            processFile(filename, [](ifstream&) { throw runtime_error("my error"); });
        }));

        // File copying.
        KSS_ASSERT(throwsException<system_error>([&] {
            copyFile("/no/such/file", filename);
        }));
        KSS_ASSERT(throwsException<system_error>([&] {
            copyFile(filename, "/no/such/directory");
        }));

        copyFile(filename, filename2);
        KSS_ASSERT(isFile(filename2));
        stringstream s;
        processFile(filename2, LineByLine([&](const string& line) {
            s << line << endl;
        }));
        KSS_ASSERT(s.str() == "1 2 3 4\none two three four\n");

        // Cleanup
        unlink(filename.c_str());
        unlink(filename2.c_str());
    }),
    make_pair("empty path arguments", [](TestSuite&) {
        KSS_ASSERT(throwsException<invalid_argument>([]{ exists(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isFile(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isDirectory(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isSymbolicLink(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isPipe(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isCharacterSpecial(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isBlockSpecial(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isSocket(""); }));
        KSS_ASSERT(throwsException<invalid_argument>([]{ isWhiteout(""); }));
    })
});
