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

#include "_contract.hpp"
#include "fileutil.hpp"

using namespace std;
using namespace kss::io::file;

namespace contract = kss::io::_private::contract;


namespace {
    inline void throwSystemError(int err, const char* fnname) {
        throw system_error(err, system_category(), fnname);
    }
}

bool kss::io::file::_private::doStat(const string &path, bool followLinks, mode_t mode) {
    contract::parameters({
        KSS_EXPR(!path.empty())
    });

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
    contract::parameters({
        KSS_EXPR(!path.empty())
    });

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
        contract::parameters({
            KSS_EXPR(!prefix.empty())
        });

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

FiledesGuard::FiledesGuard(int filedes) : _filedes(filedes) {
    contract::postconditions({
        KSS_EXPR(_filedes == filedes)
    });
}

FiledesGuard::~FiledesGuard() noexcept {
    if (_filedes != -1) {
        close(_filedes);
    }
}

FileGuard::FileGuard(FILE* f) : _file(f) {
    contract::postconditions({
        KSS_EXPR(_file == f)
    });
}

FileGuard::~FileGuard() noexcept {
    if (_file) {
        fclose(_file);
    }
}


// MARK: File Processing with exceptions

namespace {
    inline void throwProcessingError(const string& filename, const string& what_arg) {
        throw system_error(errno, system_category(), what_arg + " " + filename);
    }

    template <class Stream, class Fn>
    void doProcess(const string& filename, const string& mode, const Fn& fn) {
        contract::parameters({
            KSS_EXPR(!filename.empty())
        });

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

LineByLine::LineByLine(char eolDelimiter, const line_processing_fn& fn)
: _delim(eolDelimiter), _fn(fn)
{
    contract::postconditions({
        KSS_EXPR(_delim == eolDelimiter)
    });
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
    contract::parameters({
        KSS_EXPR(!sourceFilename.empty()),
        KSS_EXPR(!destinationFilename.empty())
    });

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
    ostrm.flush();

    if (ostrm.bad()) {
        throwProcessingError(destinationFilename, "Failed while writing");
    }

    contract::postconditions({
        KSS_EXPR(isFile(sourceFilename)),
        KSS_EXPR(isFile(destinationFilename))
    });

#if !defined(NDEBUG)
    // This postcondition is expensive on all but the smallest files. Hence we break
    // from our standard and only perform this check in debug mode. Also note that this
    // is not a full check, i.e. we are not comparing the file contents, we are only
    // ensuring they are the correct size.
    size_t srcCharCount = 0;
    size_t dstCharCount = 0;
    processFile(sourceFilename, CharByChar([&srcCharCount](char) { ++srcCharCount; }));
    processFile(destinationFilename, CharByChar([&dstCharCount](char) { ++dstCharCount; }));
    contract::postconditions({
        KSS_EXPR(srcCharCount == dstCharCount)
    });
#endif
}
