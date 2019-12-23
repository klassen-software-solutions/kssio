//
//  file_tree_walk.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2019-02-16.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <system_error>

#include <fts.h>
#include <sys/types.h>
#include <kss/contract/all.h>
#include <kss/util/all.h>

#include "file_tree_walk.hpp"
#include "fileutil.hpp"

using namespace std;
using namespace kss::io::file;
namespace contract = kss::contract;
using kss::util::Finally;


namespace {
    using name_and_ftsent_fn = function<void(const string&, FTSENT*)>;

    bool isHidden(const FTSENT* entry) noexcept {
        assert(entry->fts_namelen > 0);
        const string name(entry->fts_name, entry->fts_namelen);
        if (name == "." || name == "..") {
            return false;
        }
        return (name[0] == '.');
    }

    void doFileTreeWalk(const string& pathToDirectory,
                        bool ignoreHiddenFiles,
                        bool includeStatWithOutput,
                        const name_and_ftsent_fn& fn)
    {
        contract::parameters({
            KSS_EXPR(!pathToDirectory.empty()),
            KSS_EXPR(isDirectory(pathToDirectory))
        });

        FTS* fts = nullptr;
        Finally cleanup([&] {
            if (fts) {
                if (fts_close(fts) == -1) {
                    throw system_error(errno, system_category(), "fts_close");
                }
            }
        });

        char* pathArgv[] = { const_cast<char*>(pathToDirectory.c_str()), nullptr };
        int options = FTS_NOCHDIR | (includeStatWithOutput
                                     ? FTS_PHYSICAL | FTS_COMFOLLOW
                                     : FTS_LOGICAL | FTS_NOSTAT);

        errno = 0;
        fts = fts_open(pathArgv, options, nullptr);
        if (!fts) {
            throw system_error(errno, system_category(), "fts_open");
        }

        string ignoreUntilWeVisit;
        FTSENT* entry = fts_read(fts);
        while (entry) {
            const string pathName(entry->fts_path, entry->fts_pathlen);

            switch (entry->fts_info) {
                case FTS_D:
                    // Directory being visited in pre-order. If we are ignoring hidden
                    // files and the directory starts with a dot, then we ignore all
                    // further entries until we visit this again in post-order.
                    if (!ignoreUntilWeVisit.empty()) {
                        break;
                    }
                    if (ignoreHiddenFiles && isHidden(entry)) {
                        ignoreUntilWeVisit = string(entry->fts_name, entry->fts_namelen);
                    }
                    break;

                case FTS_DP:
                    // Directory being visited in post-order. If we are currently ignoring
                    // until we see this directory, then we stop ignoring entries.
                    if (!strncmp(ignoreUntilWeVisit.c_str(), entry->fts_name, entry->fts_namelen)) {
                        ignoreUntilWeVisit.clear();
                    }
                    break;

                case FTS_DOT:
                    // Directory entries, nothing to report just continue to the next.
                    break;

                case FTS_DNR:
                case FTS_ERR:
                case FTS_NS:
                    // Error conditions, throw an exception.
                    throw system_error(entry->fts_errno, system_category(),
                                       "fts_read failed on "
                                       + string(entry->fts_path, entry->fts_pathlen));

                default:
                    // Unless we should ignore it, pass it to the lambda.
                    if (!ignoreUntilWeVisit.empty()) {
                        break;
                    }
                    if (ignoreHiddenFiles && isHidden(entry)) {
                        break;
                    }
                    fn(string(entry->fts_path, entry->fts_pathlen), entry);
            }

            entry = fts_read(fts);
        }

        if (errno != 0) {
            throw system_error(errno, system_category(), "fts_read");
        }
    }
}


void kss::io::file::fileTreeWalk(const string& pathToDirectory,
                                 bool ignoreHiddenFiles,
                                 const ftw::name_only_fn& fn)
{
    doFileTreeWalk(pathToDirectory, ignoreHiddenFiles, false, [&](const string& pathToFile, FTSENT*) {
        contract::conditions({
            KSS_EXPR(!pathToFile.empty()),
            KSS_EXPR(!ignoreHiddenFiles || basename(pathToFile)[0] != '.')
        });
        if (isFile(pathToFile)) {
            fn(pathToFile);
        }
    });
}


void kss::io::file::fileTreeWalk(const string& pathToDirectory,
                                 bool ignoreHiddenFiles,
                                 const ftw::name_and_stat_fn& fn)
{
    doFileTreeWalk(pathToDirectory, ignoreHiddenFiles, true, [&](const string& pathToFile, FTSENT* ent) {
        contract::conditions({
            KSS_EXPR(!pathToFile.empty()),
            KSS_EXPR(!ignoreHiddenFiles || basename(pathToFile)[0] != '.'),
            KSS_EXPR(ent != nullptr),
            KSS_EXPR(ent->fts_statp != nullptr)
        });
        fn(pathToFile, *(ent->fts_statp));
    });
}


void kss::io::file::fileTreeWalk(const string& pathToDirectory,
                                 bool ignoreHiddenFiles,
                                 const ftw::name_and_stream_fn& fn)
{
    doFileTreeWalk(pathToDirectory, ignoreHiddenFiles, false, [&](const string& pathToFile, FTSENT*) {
        contract::conditions({
            KSS_EXPR(!pathToFile.empty()),
            KSS_EXPR(!ignoreHiddenFiles || basename(pathToFile)[0] != '.')
        });

        if (isFile(pathToFile)) {
            errno = 0;
            ifstream strm(pathToFile);
            if (!strm.is_open()) {
                throw system_error(errno, system_category(), "Failed to open " + pathToFile);
            }

            fn(pathToFile, strm);
            if (strm.bad()) {
                throw system_error(errno, system_category(), "Failed while reading " + pathToFile);
            }
        }
    });
}
