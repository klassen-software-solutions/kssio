//
//  file_tree_walk.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2019-02-16.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_file_tree_walk_hpp
#define kssio_file_tree_walk_hpp

#include <fstream>
#include <functional>
#include <string>

#include <sys/stat.h>

namespace kss { namespace io { namespace file {

    namespace ftw {
        /*!
         This is the type of lambda to provide when you want fileTreeWalk() to give you only
         the path to each file.
         */
        using name_only_fn = std::function<void(const std::string& pathToFile)>;

        /*!
         This is the type of lambda to provide when you want fileTreeWalk() to give you the
         path and a reference to the stat of the file.
         */
        using name_and_stat_fn = std::function<void(const std::string& pathToFile,
                                                    const struct stat& fileStat)>;

        /*!
         This is the type of lambda to provide when you want fileTreeWalk() to give you the
         path and an already opened read-only stream for the file.
         */
        using name_and_stream_fn = std::function<void(const std::string& pathToFile,
                                                      std::ifstream& strm)>;
    }


    /*!
     Perform a file tree walk starting at the given directory. This function will
     visit every file, calling the given lambda for each.

     \note that this method has three different forms, depending on what you want to do
     with the file.

     1) If the lambda takes only a const string reference, then the only
     thing you will be told is the path to the file from the starting directory. This
     version will follow all directories but will only report files or symbolic links
     to files.

     2) If it takes a string and a `struct stat` reference, then you will be given the
     path and a const reference to the file stat structure. The reference will be guaranteed
     to be valid at least until the end of the call to the lambda. This version will
     follow all directories and will report all non-directory entries.

     3) If it takes a string and a `std::ifstream` reference, then you will be given the
     path and a read-only reference to the already opened file. This reference will be
     guaranteed to be valid at least until the end of the call to the lambda. This
     version will follow all directories but will only report files or symbolic links
     to files.

     @param pathToDirectory The path to the directory you want to walk.
     @param ignoreHiddenFiles (optional) If specified ignore any files that start with `.`
     @param fn The lambda that should be called for each file found in the walk.

     @throws invalid_argument if pathToDirectory is empty or names something
        that is not a directory.
     @throws system_error if there is a problem with one of the underlying C calls. This
        error will include the name of the C call (e.g. `fts_read`) as well as the name
        of the file that it was looking at when the error occurred. If no such file is
        relevant, then only the name of the C call will be included.
     @throws any exception that fn may throw
     */
    void fileTreeWalk(const std::string& pathToDirectory,
                      bool ignoreHiddenFiles,
                      const ftw::name_only_fn& fn);

    inline void fileTreeWalk(const std::string& pathToDirectory, const ftw::name_only_fn& fn) {
        fileTreeWalk(pathToDirectory, false, fn);
    }


    void fileTreeWalk(const std::string& pathToDirectory,
                      bool ignoreHiddenFiles,
                      const ftw::name_and_stat_fn& fn);

    inline void fileTreeWalk(const std::string& pathToDirectory,
                             const ftw::name_and_stat_fn& fn)
    {
        fileTreeWalk(pathToDirectory, false, fn);
    }


    void fileTreeWalk(const std::string& pathToDirectory,
                      bool ignoreHiddenFiles,
                      const ftw::name_and_stream_fn& fn);

    inline void fileTreeWalk(const std::string& pathToDirectory,
                             const ftw::name_and_stream_fn& fn)
    {
        fileTreeWalk(pathToDirectory, false, fn);
    }

}}}

#endif
