//
//  binary_file.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>

#include <fcntl.h>

#include "binary_file.hpp"

using namespace std;
using namespace kss::io::file;


// MARK: BinaryFile

namespace {
    // Convert the open mode enum to a string.
    string toOpenModeString(BinaryFile::mode_t openMode) {
        switch (openMode) {
            case BinaryFile::reading:                           return "rb";
            case BinaryFile::writing:                           return "wb";
            case BinaryFile::appending:                         return "ab";
            case BinaryFile::reading | BinaryFile::updating:    return "rb+";
            case BinaryFile::writing | BinaryFile::updating:    return "wb+";
            case BinaryFile::appending | BinaryFile::updating:  return "ab+";
            default:
                throw invalid_argument("invalid openMode");
        }
    }

    // Convert the open mode of a filedes to a string.
    string toOpenModeString(int filedes) noexcept {
        int flags = fcntl(filedes, F_GETFL);
        if ((flags & O_APPEND) && (flags & O_WRONLY)) return "ab";
        if ((flags & O_APPEND) && (flags & O_RDWR)) return "ab+";
        if (flags & O_WRONLY) return "wb";
        if (flags & O_RDWR) return "wb+";
        return "rb";
    }
}

BinaryFile::BinaryFile(const string& filename, mode_t openMode) {
    contract::parameters({
        KSS_EXPR(!filename.empty())
    });

    _fp = fopen(filename.c_str(), toOpenModeString(openMode).c_str());
    if (!_fp) {
        throw system_error(errno, system_category(), "fopen");
    }
    _autoclose = true;

    contract::postconditions({
        KSS_EXPR(_fp != nullptr),
        KSS_EXPR(_autoclose == true),
        KSS_EXPR(isOpenFor(openMode)),
        KSS_EXPR((openMode & appending) ? true : tell() == 0)
    });
}

BinaryFile::BinaryFile(FILE* fp) {
    contract::parameters({
        KSS_EXPR(fp != nullptr)
    });

    _fp = fp;
    _autoclose = false;

    contract::postconditions({
        KSS_EXPR(_fp != nullptr),
        KSS_EXPR(_autoclose == false),
        KSS_EXPR(isOpenFor(appending) ? true : tell() == 0)
    });
}

BinaryFile::BinaryFile(int filedes) {
    contract::parameters({
        KSS_EXPR(filedes >= 0)
    });

    _fp = fdopen(filedes, toOpenModeString(filedes).c_str());
    if (!_fp) {
        throw system_error(errno, system_category(), "fopen");
    }
    _autoclose = false;

    contract::postconditions({
        KSS_EXPR(_fp != nullptr),
        KSS_EXPR(_autoclose == false),
        KSS_EXPR(isOpenFor(appending) ? true : tell() == 0)
    });
}

BinaryFile::BinaryFile(BinaryFile&& f) {
    operator=(std::move(f));
}

BinaryFile::~BinaryFile() noexcept {
    if (_fp && _autoclose) {
        fclose(_fp);
    }
}

BinaryFile& BinaryFile::operator=(BinaryFile&& f) noexcept {
    if (&f != this) {
        _fp = f._fp;
        _autoclose = f._autoclose;
        f._fp = nullptr;
    }
    return *this;
}


// Reading and writing.
namespace {
    size_t singleRead(void* buf, size_t n, FILE* fp) {
        assert(buf != nullptr);
        assert(n != 0);
        assert(fp != nullptr);

        size_t bytesread = fread(buf, 1, n, fp);
        if (bytesread != n && ferror(fp)) {
            throw system_error(EIO, system_category(), "fread");
        }
        return bytesread;
    }

    size_t singleWrite(const void* buf, size_t n, FILE* fp) {
        assert(buf != nullptr);
        assert(n != 0);
        assert(fp != nullptr);

        size_t byteswritten = fwrite(buf, 1, n, fp);
        if (byteswritten != n && ferror(fp)) {
            throw system_error(EIO, system_category(), "fwrite");
        }
        return byteswritten;
    }
}

size_t BinaryFile::read(void *buf, size_t n) {
    contract::parameters({
        KSS_EXPR(buf != nullptr),
        KSS_EXPR(n != 0)
    });
    contract::preconditions({
        KSS_EXPR(isOpenFor(reading))
    });

    const auto pos = tell();
    const auto nRead = singleRead(buf, n, _fp);

    contract::postconditions({
        KSS_EXPR(tell() == (pos + nRead))
    });
    return nRead;
}

void BinaryFile::readFully(void *buf, size_t n) {
    contract::parameters({
        KSS_EXPR(buf != nullptr),
        KSS_EXPR(n != 0)
    });
    contract::preconditions({
        KSS_EXPR(isOpenFor(reading))
    });

    const auto startPos = tell();
    size_t remain = n;
    uint8_t* pos = static_cast<uint8_t*>(buf);
    while (remain > 0 && !feof(_fp) && !ferror(_fp)) {
        size_t bytesread = singleRead(pos, remain, _fp);
        remain -= bytesread;
        pos += bytesread;
    }
    if (remain > 0) {
        if (ferror(_fp)) {
            throw system_error(EIO, system_category(), "fread");
        }
        if (feof(_fp)) {
            throw kss::io::Eof();
        }
    }

    contract::postconditions({
        KSS_EXPR(tell() == (startPos + n))
    });
}

size_t BinaryFile::write(const void *buf, size_t n) {
    contract::parameters({
        KSS_EXPR(buf != nullptr),
        KSS_EXPR(n != 0)
    });
    contract::preconditions({
        KSS_EXPR(isOpenFor(writing))
    });

    const auto pos = tell();
    size_t bytesWritten = singleWrite(buf, n, _fp);

    contract::postconditions({
        KSS_EXPR(isOpenFor(appending) ? true : tell() == (pos + bytesWritten))
    });
    return bytesWritten;
}

void BinaryFile::writeFully(const void *buf, size_t n) {
    contract::parameters({
        KSS_EXPR(buf != nullptr),
        KSS_EXPR(n != 0)
    });
    contract::preconditions({
        KSS_EXPR(isOpenFor(writing))
    });

    const auto startingPos = tell();
    size_t remain = n;
    const uint8_t* pos = static_cast<const uint8_t*>(buf);
    while (remain > 0 && !ferror(_fp)) {
        size_t bytesWritten = singleWrite(pos, remain, _fp);
        remain -= bytesWritten;
        pos += bytesWritten;
    }
    if (ferror(_fp)) {
        throw system_error(EIO, system_category(), "fwrite");
    }

    contract::postconditions({
        KSS_EXPR(isOpenFor(appending) ? true : tell() == (startingPos + n))
    });
}

void BinaryFile::flush() {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    const auto pos = tell();
    if (fflush(_fp) != 0) {
        throw system_error(errno, system_category(), "fflush");
    }

    contract::postconditions({
        KSS_EXPR(tell() == pos)
    });
}


// Position in the file.
bool BinaryFile::eof() const noexcept {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    return (feof(_fp) != 0);
}

off_t BinaryFile::tell() const {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    const auto p = ftello(_fp);
    if (p == -1) {
        throw system_error(errno, system_category(), "ftello");
    }
    return p;
}

void BinaryFile::seek(off_t sp) {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    if (fseeko(_fp, sp, SEEK_SET) == -1) {
        throw system_error(errno, system_category(), "fseeko");
    }

    contract::postconditions({
        KSS_EXPR(tell() == sp)
    });
}

void BinaryFile::move(off_t offset) {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    const auto pos = tell();
    if (fseeko(_fp, offset, SEEK_CUR) == -1) {
        throw system_error(errno, system_category(), "fseeko");
    }

    contract::postconditions({
        KSS_EXPR(tell() == (pos + offset))
    });
}

void BinaryFile::rewind() {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    errno = 0;
    ::rewind(_fp);
    if (errno) {
        throw system_error(errno, system_category(), "rewind");
    }

    contract::postconditions({
        KSS_EXPR(tell() == 0)
    });
}

void BinaryFile::fastForward() {
    contract::preconditions({
        KSS_EXPR(_fp != nullptr)
    });

    if (fseek(_fp, 0, SEEK_END) == -1) {
        throw system_error(errno, system_category(), "fseek");
    }
}

bool BinaryFile::isOpenFor(mode_t mode) const {
    if (_fp) {
        const int fd = fileno(_fp);
        contract::condition(KSS_EXPR(fd >= 0));

        const int flags = fcntl(fd, F_GETFL);
        if ((mode & reading) && (flags & O_WRONLY)) {
            return false;
        }
        if ((mode & writing) && (flags & O_RDONLY)) {
            return false;
        }
        if ((mode & appending) && !(flags & O_APPEND)) {
            return false;
        }
        if ((mode & updating) && !(flags & O_RDWR)) {
            return false;
        }
        return true;
    }
    return false;
}
