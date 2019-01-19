//
//  fileutil.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2013-02-07.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_fileutil_hpp
#define kssio_fileutil_hpp

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <system_error>

#include <sys/stat.h>

namespace kss {
    namespace io {
        namespace file {

            namespace _private {
                bool doStat(const std::string& path, bool followLinks, mode_t mode);
            }

            /*!
             Returns true if the file exists and false otherwise.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            bool exists(const std::string& path, bool followLinks = true);

            /*!
             Returns true if the path exists and is a regular file.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isFile(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFREG);
            }

            /*!
             Returns true if the path exists and is a directory.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isDirectory(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFDIR);
            }

            /*!
             Returns true if the path exists and is a symbolic link.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isSymbolicLink(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFLNK);
            }

            /*!
             Returns true if the path exists and is a pipe.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isPipe(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFIFO);
            }

            /*!
             Returns true if the path exists and is a character special file.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isCharacterSpecial(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFCHR);
            }

            /*!
             Returns true if the path exists and is a block special file.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isBlockSpecial(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFBLK);
            }

            /*!
             Returns true if the path exists and is a socket.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isSocket(const std::string& path, bool followLinks = true) {
                return kss::io::file::_private::doStat(path, followLinks, S_IFSOCK);
            }

            /*!
             Returns true if the path exists and is a whiteout. Note that this does not
             exist on some architectures (e.g. Linux). For those architectures this will
             always return false.
             @param path the path to be examined
             @param followLinks if true we follow symbolic links
             @throws std::invalid_argument if path is empty
             @throws std::system_error if the underlying C call fails
             */
            inline bool isWhiteout(const std::string& path, bool followLinks = true) {
#if defined(S_IFWHT)
                return kss::io::file::_private::doStat(path, followLinks, S_IFWHT);
#else
                if (path.empty()) {
                    throw std::invalid_argument("path is empty");
                }
                return false;
#endif
            }

            /*!
             Return a temporary name/file/stream with the given prefix.

             Warning: temporaryFilename can be a security risk as in some architectures the
             pattern of the returned name can be guessed and there is a pause between obtaining
             the name and, presumbably, opening the file. For situations where this is a
             potential problem use temporaryFile instead which handles the potential race
             condition, instead of temporaryFilename or temporaryFStream both of which have the
             potential race condition.
             
             @return the string version returns a suitable filename. The file version returns
                  a file handle opened for reading and writing. The stream version returns
                  an fstream opened for reading and writing.
             @throws std::invalid_argument if prefix is empty
             @throws system_error if the underlying C routines return an error condition.
             */
            std::string  temporaryFilename(const std::string& prefix);
            FILE*        temporaryFile(const std::string& prefix);
            std::fstream temporaryFStream(const std::string& prefix);


            /*!
             Returns the base name (everything after the final "/") of the given path.
             If path is empty, or if it consists of just "/" characters, then an empty
             string is returned.

             Note that unlike basename(3) this is re-entrant.
             */
            std::string basename(const std::string& path);


            /*!
             Returns the directory (everything up to and including the final "/") of
             the given path. If the path is empty or contains no "/" characters, then "./"
             is returned to signify the current directory.

             Note that this differs slightly from dirname(3) (includes the trailing '/')
             and is re-entrant.
             */
            std::string dirname(const std::string& path);


            /*!
             Provide a guard around a, presumably open, unix file descriptor. This will close 
             the file in its destructor. Note that it will check for a -1 filedes hence you can 
             pass open in directly.
             */
            class FiledesGuard {
            public:
                explicit FiledesGuard(int filedes);
                ~FiledesGuard() noexcept;
                int filedes() const noexcept { return _filedes; }
            private:
                int _filedes;
            };

            /*!
             Provide a guard around a, presumably open, file. This will close the file in its
             destructor. Note that it will check for a NULL file hence you can pass fopen in
             directly.
             */
            class FileGuard {
            public:
                explicit FileGuard(FILE* f);
                ~FileGuard() noexcept;
                FILE* file() const noexcept { return _file; }
            private:
                FILE* _file;
            };

            /*!
             Write a file throwing exceptions if there is a problem. This will open the
             given filename for writing, creating it if necessary, then call the given
             function to actually perform the work.

             @param filename the name of the file to write
             @param fn a lambda that will write the contents to the stream
             @throws std::invalid_argument if filename is empty
             @throws std::system_error if there is a problem while opening or writing
             @throws any exception that fn throws
             */
            void writeFile(const std::string& filename,
                           const std::function<void(std::ofstream&)>& fn);

            /*!
             Process a file throwing exceptions if there is a problem.

             @param filename the name of the file to process
             @param fn a lambda that will process the given stream
             @throws std::invalid_argument if filename is empty
             @throws std::system_error if there is a problem while opening or reading.
             @throws any exception that fn throws
             */
            void processFile(const std::string& filename,
                             const std::function<void(std::ifstream&)>& fn);

            /*!
             This functional allows an input stream to be processed line by line when its
             operator is called. While it can be used on its own with any input stream, it
             is specifically designed to work as an input to process_file. That is, you can
             pass an instance of this class as the second argument to process_file in
             order to process the file one line at a time.
             */
            class LineByLine {
            public:
                typedef std::function<void(const std::string&)> line_processing_fn;

                explicit LineByLine(const line_processing_fn& fn) : LineByLine('\n', fn) {}
                LineByLine(char eolDelimiter, const line_processing_fn& fn);
                void operator()(std::istream& strm);

            private:
                const char          _delim;
                line_processing_fn  _fn;
            };

            /*!
             This functional allows an input stream to be processed one character at a time.
             While it can be used on its own with any input stream, it is specifically designed
             to work as an input to process_file. That is, you can pass an instance of this
             class as the second argument to process_file in order to process the file one
             character at a time.
             */
            class CharByChar {
            public:
                typedef std::function<void(char)> char_processing_fn;

                explicit CharByChar(const char_processing_fn& fn) : _fn(fn) {}
                void operator()(std::istream& strm);

            private:
                char_processing_fn _fn;
            };

            /*!
             Copy a file.
             @throws std::invalid_argument if either filename is empty
             @throws std::system_error if a problem occurs.
             */
            void copyFile(const std::string& sourceFilename,
                          const std::string& destinationFilename);
        }
    }
}

#endif
