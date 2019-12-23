//
//  directory.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

#include <dirent.h>
#include <unistd.h>

#include <sys/param.h>
#include <kss/contract/all.h>
#include <kss/util/all.h>

#include "directory.hpp"
#include "fileutil.hpp"

using namespace std;
using namespace kss::io::file;

namespace contract = kss::contract;

using kss::util::Finally;
using kss::util::strings::endsWith;
using kss::util::strings::Tokenizer;


namespace {
    constexpr const char* separator = "/";

	void ensurePathSoFar(const string& pathName, mode_t permissions) {
        assert(!pathName.empty());
		if (!isDirectory(pathName)) {
			if (::mkdir(pathName.c_str(), permissions) == -1) {
                throw system_error(errno, system_category(), "mkdir");
			}
#if defined(__linux)
			// For some reason on Linux mkdir does not always seem to set the permissions.
			if (::chmod(pathName.c_str(), permissions) == -1) {
                throw system_error(errno, system_category(), "chmod");
			}
#endif
		}
	}
}

Directory::Directory(const string& dirName, bool ignoreHidden)
: directoryName(dirName), ignoreHidden(ignoreHidden)
{
    contract::parameters({
        KSS_EXPR(!dirName.empty()),
        KSS_EXPR(isDirectory(dirName))
    });

    contract::postconditions({
        KSS_EXPR(isDirectory(directoryName))
    });
}

// Get the current directory.
string kss::io::file::getCwd() {
    char cwd[MAXPATHLEN+1];
	if (!::getcwd(cwd, MAXPATHLEN)) {
        throw system_error(errno, system_category(), "getcwd");
	}

    contract::postconditions({
        KSS_EXPR(strlen(cwd) > 0)
    });
    return string(cwd);
}


void kss::io::file::ensurePath(const string& dir, mode_t permissions) {
    contract::parameters({
        KSS_EXPR(!dir.empty())
    });

	if (!isDirectory(dir)) {
		// If the path already exists, but is not a directory, we do not attmept to
		// replace it, but throw an exception.
		if (exists(dir)) {
			throw system_error(EEXIST, system_category(), dir + " is not a directory");
		}

		// Parse the path into its components, ensuring each directory exists in turn.
		Tokenizer pathComponents(dir, separator);
        string pathSoFar = (dir[0] == separator[0] ? "/" : "");
		for (const auto& component : pathComponents) {
            pathSoFar += component;
			ensurePathSoFar(pathSoFar, permissions);
            pathSoFar += separator;
		}
	}

    contract::postconditions({
        KSS_EXPR(isDirectory(dir))
    });
}

namespace {
    void removePathSimple(const string& dir) {
        if (rmdir(dir.c_str()) == -1) {
            throw system_error(errno, system_category(), "rmdir");
        }
    }

    void removePathRecursive(const string& dir) {
        assert(!dir.empty());
        assert(exists(dir));

        // First we remove all the entries.
        {
            Directory d(dir);
            for (const auto& entry : d) {
                assert(entry != "." && entry != "..");
                const string fullPathToEntry = dir + "/" + entry;
                if (isDirectory(fullPathToEntry)) {
                    removePathRecursive(fullPathToEntry);
                }
                else {
                    if (unlink(fullPathToEntry.c_str()) == -1) {
                        throw system_error(errno, system_category(), "unlink");
                    }
                }
            }
        }

        // Then we remove the directory.
        removePathSimple(dir);
    }
}

void kss::io::file::removePath(const string &dir, bool recursive) {
    contract::parameters({
        KSS_EXPR(!dir.empty()),
        KSS_EXPR(!exists(dir) || isDirectory(dir))
    });

    if (exists(dir)) {
        if (recursive) {
            removePathRecursive(dir);
        }
        else {
            removePathSimple(dir);
        }
    }

    contract::postconditions({
        KSS_EXPR(!exists(dir))
    });
}


//// MARK: directory

namespace {
    DIR* openDir(const string& pathName) {
        // preconditions
        assert(!pathName.empty());

        DIR* dir = opendir(pathName.c_str());
        if (!dir) {
            throw system_error(errno, system_category(), "opendir");
        }

        // postconditions
        assert(dir != nullptr);
        return dir;
    }

    // Note that entry should not be used by the caller, it is just space provided that
    // must not go out of scope before you are done using *result.
    bool readDir(DIR* dir, dirent* entry, dirent** result, bool ignoreHidden) {
        // preconditions
        assert(dir != nullptr);
        assert(entry != nullptr);
        assert(result != nullptr);

        bool tryAgain = false;
        do {
#if defined(__linux)
            errno = 0;
            dirent* d = readdir(dir);
            if (errno) {
                throw system_error(errno, system_category(), "readdir");
            }
            *result = d;
#else
            const int err = readdir_r(dir, entry, result);
            if (err) {
                throw system_error(err, system_category(), "readdir_r");
            }
#endif

            // If we have an entry but it is either "." or "..", then we skip it
            // and try again. This is the fix for bug 16.
            tryAgain = false;
            if (*result != nullptr) {
                if (!strcmp((*result)->d_name, ".") || !strcmp((*result)->d_name, "..")) {
                    tryAgain = true;
                }

                // Optionally ignore hidden files. This is the fix for issue 17.
                if (ignoreHidden && (*result)->d_name[0] == '.') {
                    tryAgain = true;
                }
            }
        } while (tryAgain);

        return (*result != nullptr);
    }
}

// Determine the number of entries in the directory.
Directory::size_type Directory::size() const {
    contract::preconditions({
        KSS_EXPR(isDirectory(directoryName))
    });

    Directory::size_type count = 0;
    DIR* dir = openDir(directoryName);
    Finally cleanup([&]{
        if (dir) {
            closedir(dir);
        }
    });
    contract::conditions({
        KSS_EXPR(dir != nullptr)
    });

    dirent entry;
    dirent* result = nullptr;
    do {
        if (readDir(dir, &entry, &result, ignoreHidden)) {
            ++count;
        }
    } while (result != nullptr);
    return count;
}

// Determine if a directory is empty.
bool Directory::empty() const {
    contract::preconditions({
        KSS_EXPR(isDirectory(directoryName))
    });

    DIR* dir = openDir(directoryName);
    Finally cleanup([&]{
        if (dir) {
            closedir(dir);
        }
    });
    contract::conditions({
        KSS_EXPR(dir != nullptr)
    });

    dirent entry;
    dirent* result = nullptr;
    return !readDir(dir, &entry, &result, ignoreHidden);
}

// Determine if two directories have the same entries.
bool Directory::operator==(const Directory& rhs) const {
    contract::preconditions({
        KSS_EXPR(isDirectory(directoryName)),
        KSS_EXPR(isDirectory(rhs.directoryName))
    });

    // If these are the same objects, then this is trivially true.
    if (&rhs == this) {
        return true;
    }

    // If these point to the same directories, and have the same ignoreHidden setting,
    // then this is trivially true.
    if ((directoryName == rhs.directoryName) && (ignoreHidden == rhs.ignoreHidden)) {
        return true;
    }

    // Otherwise we must traverse the directories, comparing their entries.
    DIR* dir1 = nullptr;
    DIR* dir2 = nullptr;
    Finally cleanup([&]{
        if (dir2 != nullptr) { closedir(dir2); }
        if (dir1 != nullptr) { closedir(dir1); }
    });

    dir1 = openDir(directoryName);
    dir2 = openDir(rhs.directoryName);
    contract::conditions({
        KSS_EXPR(dir1 != nullptr),
        KSS_EXPR(dir2 != nullptr)
    });

    dirent d1;
    dirent d2;
    dirent* de1 = nullptr;
    dirent* de2 = nullptr;
    do {
        const bool ret1 = readDir(dir1, &d1, &de1, ignoreHidden);
        const bool ret2 = readDir(dir2, &d2, &de2, rhs.ignoreHidden);
        if (ret1 != ret2) {
            return false;
        }
        if (ret1 && strncmp(de1->d_name, de2->d_name, NAME_MAX) != 0) {
            return false;
        }
    } while (de1 != nullptr && de2 != nullptr);

    return true;
}



//// MARK: DIRECTORY::CONST_ITERATOR IMPLEMENTATION

// Construct the iterator.
Directory::const_iterator::const_iterator(const string& dirName,
                                          bool ignoreHidden,
                                          bool endFlag)
: dirptr(NULL), currentValue(""), directoryName(dirName), ignoreHidden(ignoreHidden)
{
    contract::preconditions({
        KSS_EXPR(isDirectory(dirName))
    });

    // Things are starting at the end (empty current value) so if
    // the end flag is set we are done.
    if (endFlag == true) { return; }

    // Otherwise we open the directory and obtain its first value.
    dirptr = openDir(dirName);

    dirent d;
    dirent* de = nullptr;
    if (readDir(static_cast<DIR*>(dirptr), &d, &de, ignoreHidden)) {
        currentValue = de->d_name;
    }

    contract::postconditions({
        KSS_EXPR(dirptr != nullptr),
        KSS_EXPR(isDirectory(directoryName))
    });
}

// Destroy the iterator.
Directory::const_iterator::~const_iterator() noexcept
{
    if (dirptr != nullptr) {
        closedir(static_cast<DIR*>(dirptr));
    }
}

// Move to the next value.
Directory::const_iterator& Directory::const_iterator::operator++() {
    contract::preconditions({
        KSS_EXPR(dirptr != nullptr),
        KSS_EXPR(!currentValue.empty())
    });

    const auto prevValue = currentValue;
    dirent d;
    dirent* de = nullptr;
    if (readDir(static_cast<DIR*>(dirptr), &d, &de, ignoreHidden)) {
        currentValue = de->d_name;
    }
    else {
        currentValue = "";
    }

    contract::postconditions({
        KSS_EXPR(currentValue != prevValue)
    });
    return *this;
}

Directory::const_iterator Directory::const_iterator::operator++(int) {
    const_iterator tmp = *this;
    ++*this;
    return tmp;
}
