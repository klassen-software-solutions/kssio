//
//  rolling_file.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2019-02-06.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cerrno>
#include <exception>
#include <iomanip>
#include <sstream>
#include <system_error>

#include <syslog.h>

#include "_contract.hpp"
#include "_rtti.hpp"
#include "fileutil.hpp"
#include "rolling_file.hpp"

using namespace std;
using namespace kss::io::file;

namespace contract = kss::io::_private::contract;
namespace rtti = kss::io::_private::rtti;

namespace {
    // Close the current stream, calling the listeners if necessary.
    void doClose(RollingFile& rf,
                 ofstream* strm,
                 const string& fileName,
                 RollingFileListener* listener)
    {
        assert(strm != nullptr);

        if (listener != nullptr) { listener->willClose(rf, fileName, *strm); }
        delete strm;
        if (listener != nullptr) { listener->didClose(rf, fileName); }
    }

    // Find the next filename in the sequence.
    string nextFileName(const string& prefix, unsigned& nextFileIndex, const string& suffix) {
        ostringstream proposedFileName;
        proposedFileName << prefix;
        proposedFileName << setfill('0') << setw(6) << nextFileIndex;
        if (!suffix.empty()) {
            proposedFileName << ".";
            proposedFileName << suffix;
        }

        ++nextFileIndex;
        return proposedFileName.str();
    }
}


RollingFile::RollingFile(streamsize fileWrapSize,
                         const string& fileNamePrefix,
                         const string& fileNameSuffix)
: maximumFileSize(fileWrapSize), prefix(fileNamePrefix), suffix(fileNameSuffix)
{
    contract::parameters({
        KSS_EXPR(fileWrapSize > 0),
        KSS_EXPR(!fileNamePrefix.empty())
    });

    contract::postconditions({
        KSS_EXPR(maximumFileSize == fileWrapSize),
        KSS_EXPR(prefix == fileNamePrefix),
        KSS_EXPR(suffix == fileNameSuffix),
        KSS_EXPR(nextFileIndex == 0U),
        KSS_EXPR(hasPermanentlyClosed == false),
        KSS_EXPR(currentFileName.empty()),
        KSS_EXPR(currentStream == nullptr),
        KSS_EXPR(listener == nullptr)
    });
}


RollingFile::~RollingFile() {
    if (currentStream != nullptr) {
        try {
            doClose(*this, currentStream, currentFileName, listener);
        }
        catch (const exception& e) {
            syslog(LOG_ERR, "%s: failed during close, %s (%s)",
                   __PRETTY_FUNCTION__, rtti::name(e).c_str(), e.what());
         }
    }
}


void RollingFile::setListener(RollingFileListener &listener) {
    this->listener = &listener;
}


void RollingFile::write(const function<void (ofstream &)> &fn) {
    contract::preconditions({
        KSS_EXPR(hasPermanentlyClosed == false)
    });

    // If we do not have a current file, we create the next one in the sequence. Note
    // that if the new file already exists, we log a warning to syslog and then
    // continue, deleting the exiting file.
    if (currentStream == nullptr) {
        currentFileName = nextFileName(prefix, nextFileIndex, suffix);
        if (isFile(currentFileName)) {
            syslog(LOG_WARNING, "%s: %s already exists, will be replaced",
                   __PRETTY_FUNCTION__, currentFileName.c_str());
        }

        if (listener != nullptr) { listener->willOpen(*this, currentFileName); }
        errno = 0;
        currentStream = new ofstream(currentFileName, ios_base::out | ios_base::trunc);
        if (!currentStream->is_open()) {
            throw system_error(errno, system_category(), "failed to open " + currentFileName);
        }
        if (listener != nullptr) { listener->didOpen(*this, currentFileName, *currentStream); }
    }

    // Perform the writing function, then check for an error condition.
    contract::conditions({
        KSS_EXPR(currentStream != nullptr),
        KSS_EXPR(currentStream->is_open()),
        KSS_EXPR(!currentStream->bad())
    });

    errno = 0;
    fn(*currentStream);
    if (currentStream->bad()) {
        const auto error = errno;
        delete currentStream;
        currentStream = nullptr;
        throw system_error(error, system_category(), "failed wile writing to " + currentFileName);
    }

    // Check the current file position, which we will assume to be the file size. If it
    // is over the limit, we close the file.
    contract::conditions({
        KSS_EXPR(currentStream != nullptr),
        KSS_EXPR(currentStream->is_open()),
        KSS_EXPR(!currentStream->bad())
    });

    if (currentStream->tellp() > maximumFileSize) {
        doClose(*this, currentStream, currentFileName, listener);
        currentStream = nullptr;
    }
}


void RollingFile::close() {
    contract::preconditions({
        KSS_EXPR(hasPermanentlyClosed == false)
    });

    if (currentStream != nullptr) {
        doClose(*this, currentStream, currentFileName, listener);
        currentStream = nullptr;
        hasPermanentlyClosed = true;
    }
}
