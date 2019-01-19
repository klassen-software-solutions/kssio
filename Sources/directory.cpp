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

#include "_contract.hpp"
#include "_raii.hpp"
#include "_stringutil.hpp"
#include "_tokenizer.hpp"
#include "directory.hpp"
#include "fileutil.hpp"

using namespace std;
using namespace kss::io::file;

namespace contract = kss::io::_private::contract;

using kss::io::_private::endsWith;
using kss::io::_private::finally;
using kss::io::_private::Tokenizer;


namespace {
    constexpr const char* separator = "/";

	// Attempt to change to the given directory. Throws an exception if it fails.
	void changeDirectory(const string& dirName) {
        assert(!dirName.empty());
		if (::chdir(dirName.c_str()) == -1) {
            throw system_error(errno, system_category(), "chdir");
		}

        // postconditions
        assert(endsWith(getCwd(), dirName));
	}

	// Attempt to create a directory and change to it. Throws an exception if it fails.
	void ensureAndChangeDirectory(const string& localDirName, mode_t permissions) {
        assert(!localDirName.empty());
		if (!isDirectory(localDirName)) {
			if (::mkdir(localDirName.c_str(), permissions) == -1) {
                throw system_error(errno, system_category(), "mkdir");
			}
#if defined(__linux)
			// For some reason on Linux mkdir does not always seem to set the permissions.
			if (::chmod(localDirName.c_str(), permissions) == -1) {
                throw system_error(errno, system_category(), "chmod");
			}
#endif
		}
		changeDirectory(localDirName);

        // postconditions
        assert(endsWith(getCwd(), localDirName));
	}
}

Directory::Directory(const string& dirName) : _dir_name(dirName) {
    contract::parameters({
        KSS_EXPR(!dirName.empty()),
        KSS_EXPR(isDirectory(dirName))
    });

    contract::postconditions({
        KSS_EXPR(isDirectory(_dir_name))
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


// Ensure the path exists.
void kss::io::file::ensurePath(const string& dir, mode_t permissions) {
    contract::parameters({
        KSS_EXPR(!dir.empty())
    });

    const string startingDirectory = getCwd();
	if (!isDirectory(dir)) {
		// If the path already exists, but is not a directory, we do not attmept to
		// replace it, but throw an exception.
		if (exists(dir)) {
			throw system_error(EEXIST, system_category(), dir + " is not a directory");
		}

		// Determine where we start checking from and that we will return to the
		// current directory when we are all done.
		finally cleanup([&]{
			if (startingDirectory != getCwd()) {
				changeDirectory(startingDirectory);
			}
		});
		if (dir[0] == separator[0]) {
			changeDirectory("/");
		}

		// Parse the path into its components, ensuring each directory exists in turn.
		Tokenizer pathComponents(dir, separator);
		for (const auto& component : pathComponents) {
			ensureAndChangeDirectory(component, permissions);
		}
	}

    contract::postconditions({
        KSS_EXPR(isDirectory(dir)),
        KSS_EXPR(getCwd() == startingDirectory)
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
    bool readDir(DIR* dir, dirent* entry, dirent** result) {
        // preconditions
        assert(dir != nullptr);
        assert(entry != nullptr);
        assert(result != nullptr);

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
        return (*result != nullptr);
    }
}

// Determine the number of entries in the directory.
Directory::size_type Directory::size() const {
    contract::preconditions({
        KSS_EXPR(isDirectory(_dir_name))
    });

    Directory::size_type count = 0;
    DIR* dir = openDir(_dir_name);
    finally cleanup([&]{
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
        if (readDir(dir, &entry, &result)) {
            ++count;
        }
    } while (result != nullptr);
    return count;
}

// Determine if a directory is empty.
bool Directory::empty() const {
    contract::preconditions({
        KSS_EXPR(isDirectory(_dir_name))
    });

    DIR* dir = openDir(_dir_name);
    finally cleanup([&]{
        if (dir) {
            closedir(dir);
        }
    });
    contract::conditions({
        KSS_EXPR(dir != nullptr)
    });

    dirent entry;
    dirent* result = nullptr;
    return !readDir(dir, &entry, &result);
}

// Determine if two directories have the same entries.
bool Directory::operator==(const Directory& rhs) const {
    contract::preconditions({
        KSS_EXPR(isDirectory(_dir_name)),
        KSS_EXPR(isDirectory(rhs._dir_name))
    });

    // If these are the same objects, or they point to the same directories, then this is
    // trivially true.
    if ((&rhs == this) || (_dir_name == rhs._dir_name)) {
        return true;
    }

    // Otherwise we must traverse the directories, comparing their entries.
    DIR* dir1 = nullptr;
    DIR* dir2 = nullptr;
    finally cleanup([&]{
        if (dir2 != nullptr) { closedir(dir2); }
        if (dir1 != nullptr) { closedir(dir1); }
    });

    dir1 = openDir(_dir_name);
    dir2 = openDir(rhs._dir_name);
    contract::conditions({
        KSS_EXPR(dir1 != nullptr),
        KSS_EXPR(dir2 != nullptr)
    });

    dirent d1;
    dirent d2;
    dirent* de1 = nullptr;
    dirent* de2 = nullptr;
    do {
        const bool ret1 = readDir(dir1, &d1, &de1);
        const bool ret2 = readDir(dir2, &d2, &de2);
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
Directory::const_iterator::const_iterator(const string& dirName, bool endFlag)
: _dirptr(NULL), _currentValue(""), _dirName(dirName)
{
    contract::preconditions({
        KSS_EXPR(isDirectory(dirName))
    });

    // Things are starting at the end (empty current value) so if
    // the end flag is set we are done.
    if (endFlag == true) { return; }

    // Otherwise we open the directory and obtain its first value.
    _dirptr = openDir(dirName);

    dirent d;
    dirent* de = nullptr;
    if (readDir(static_cast<DIR*>(_dirptr), &d, &de)) {
        _currentValue = de->d_name;
    }

    contract::postconditions({
        KSS_EXPR(_dirptr != nullptr),
        KSS_EXPR(isDirectory(_dirName))
    });
}

// Destroy the iterator.
Directory::const_iterator::~const_iterator() noexcept
{
    if (_dirptr != nullptr) {
        closedir(static_cast<DIR*>(_dirptr));
    }
}

// Move to the next value.
Directory::const_iterator& Directory::const_iterator::operator++() {
    contract::preconditions({
        KSS_EXPR(_dirptr != nullptr),
        KSS_EXPR(!_currentValue.empty())
    });

    const auto prevValue = _currentValue;
    dirent d;
    dirent* de = nullptr;
    if (readDir(static_cast<DIR*>(_dirptr), &d, &de)) {
        _currentValue = de->d_name;
    }
    else {
        _currentValue = "";
    }

    contract::postconditions({
        KSS_EXPR(_currentValue != prevValue)
    });
    return *this;
}

Directory::const_iterator Directory::const_iterator::operator++(int) {
    const_iterator tmp = *this;
    ++*this;
    return tmp;
}
