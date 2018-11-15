//
//  fileutil.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2013-12-30.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <system_error>

#include <unistd.h>

#include "fileutil.hpp"

using namespace std;
using namespace kss::io::file;


namespace {
    inline void throwSystemError(int err, const char* fnname) {
        throw system_error(err, system_category(), fnname);
    }
}

bool kss::io::file::_private::doStat(const string &path, bool followLinks, mode_t mode) {
    struct stat st;
    const int res = (followLinks ? stat(path.c_str(), &st) : lstat(path.c_str(), &st));
    if (!res) {
        return ((st.st_mode & mode) == mode);
    }
    else if ((errno == ENOENT) || (errno == ENOTDIR)) {
        errno = 0;
        return false;
    }

    throwSystemError(errno, (followLinks ? "lstat" : "stat"));
    return false;
}

bool kss::io::file::exists(const string &path, bool followLinks) {
    struct stat st;
    const int res = (followLinks ? stat(path.c_str(), &st) : lstat(path.c_str(), &st));
    if (!res) {
        return true;
    }
    else if ((errno == ENOENT) || (errno == ENOTDIR)) {
        errno = 0;
        return false;
    }

    throwSystemError(errno, (followLinks ? "lstat" : "stat"));
    return false;
}


//// MARK: TEMPORARY FILES

namespace {
    // Construct the template from the prefix. Note that for Linux versions mktemp requires
    // that the template end in "XXXXXX" so six characters is our limit for the uniqueness.
    unique_ptr<char, void(*)(void*)> createTemplate(const string& prefix) {
        char* templ = (char*)malloc(prefix.length()+7);
        if (!templ) {
            throw bad_alloc();
        }

        unique_ptr<char, void(*)(void*)> templguard(templ, free);
        strcpy(templ, prefix.c_str());
        strcat(templ, "XXXXXX");
        return templguard;
    }
}

string kss::io::file::temporaryFilename(const string &prefix) {
    // Note that the Linux version sets templ to an empty string on failure
    // while BSD versions return NULL. In either case errno gets set.
    auto templguard = createTemplate(prefix);
    char* templ = templguard.get();
    int fd = mkstemp(templ);
    if (fd == -1) {
        throwSystemError(errno, "mkstemp");
    }

    close(fd);
    unlink(templ);
    return string(templ);
}

FILE* kss::io::file::temporaryFile(const string &prefix) {
    auto templguard = createTemplate(prefix);
    char* templ = templguard.get();
    int fd = mkstemp(templ);
    if (fd == -1) {
        throwSystemError(errno, "mkstemp");
    }

    FILE* f = fdopen(fd, "w+");
    if (!f) {
        throwSystemError(errno, "fdopen");
    }
    return f;
}

fstream kss::io::file::temporaryFStream(const string &prefix) {
    string fn = temporaryFilename(prefix);
    return fstream(fn.c_str(), fstream::in | fstream::out | fstream::trunc);
}


//// MARK: PATH MANIPULATION

string kss::io::file::basename(const string &path) {
    const auto dirSepPos = path.find_last_of('/');
    if (dirSepPos == string::npos) {
        return path;
    }
    else {
        return path.substr(dirSepPos+1);
    }
}

string kss::io::file::dirname(const string &path) {
    const auto dirSepPos = path.find_last_of('/');
    if (dirSepPos == string::npos) {
        return "./";
    }
    else if (dirSepPos == 0) {
        return "/";
    }
    else {
        return path.substr(0, dirSepPos+1);
    }
}

//// MARK: FILE_GUARD IMPLEMENTATION

FileGuard::~FileGuard() noexcept {
    if (_file) {
        fclose(_file);
    }
}

FiledesGuard::~FiledesGuard() noexcept {
    if (_filedes != -1) {
        close(_filedes);
    }
}


// MARK: File Processing with exceptions

namespace {
    inline void throwProcessingError(const string& filename, const string& what_arg) {
        throw system_error(errno, system_category(), what_arg + " " + filename);
    }

    template <class Stream, class Fn>
    void doProcess(const string& filename, const string& mode, const Fn& fn) {
        errno = 0;
        Stream strm(filename);
        if (!strm.is_open()) {
            throwProcessingError(filename, "Failed to open");
        }

        fn(strm);
        if (strm.bad()) {
            throwProcessingError(filename, "Failed while " + mode);
        }
    }
}

void kss::io::file::writeFile(const string& filename, const function<void(ofstream&)>& fn) {
    doProcess<ofstream>(filename, "writing", fn);
}

void kss::io::file::processFile(const string& filename, const function<void(ifstream &)>& fn) {
    doProcess<ifstream>(filename, "processing", fn);
}

void LineByLine::operator()(istream& strm) {
    string line;
    while (getline(strm, line, _delim)) {
        _fn(line);
    }
}

void CharByChar::operator()(istream& strm) {
    char ch = 0;
    while (strm.get(ch)) {
        _fn(ch);
    }
}

void kss::io::file::copyFile(const string& sourceFilename, const string& destinationFilename) {
    errno = 0;
    FileGuard ifile(fopen(sourceFilename.c_str(), "r"));
    if (!ifile.file()) {
        throwProcessingError(sourceFilename, "Failed to open");
    }

    ofstream ostrm(destinationFilename);
    if (!ostrm.is_open()) {
        throwProcessingError(destinationFilename, "Failed to create");
    }

    char buffer[BUFSIZ];
    while (!feof(ifile.file())) {
        const auto n = fread(buffer, 1, sizeof(buffer), ifile.file());
        if (n > 0) {
            ostrm.write(buffer, streamsize(n));
        }
    }

    if (ostrm.bad()) {
        throwProcessingError(destinationFilename, "Failed while writing");
    }
}
