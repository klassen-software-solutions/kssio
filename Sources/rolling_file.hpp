//
//  rolling_file.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2019-02-06.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_rolling_file_hpp
#define kssio_rolling_file_hpp

#include <fstream>
#include <functional>
#include <ios>
#include <string>

namespace kss { namespace io { namespace file {

    class RollingFile;

    /*!
     The RollingFileListener class is an interface (pure virtual class) that can be used
     to receive notifications when a file is opened or closed. This is useful if you wish
     to write a prefix or suffix to each file, or perhaps to update header values before
     the file is actually closed. It can also be used just to find out when events occur.

     Note that all the methods default to do nothing, hence subclasses need only
     override the ones of interest.

     Also note that any exceptions thrown will be passed up through the RollingFile
     class.
     */
    class RollingFileListener {
    public:
        /*!
         This method will be called just before a file is opened.
         @param rf the RollingFile that is calling the method
         @param fileName the full name of the file about to be opened
         */
        virtual void willOpen(RollingFile& rf, const std::string& fileName) {}

        /*!
         This method will be called just after a file is opened. It may be used to
         writing any desired initial data (say for example, a header section).
         @param rf the RollingFile that is calling the method
         @param fileName the full name of the current file
         @param strm the output stream of the current file
         */
        virtual void didOpen(RollingFile& rf,
                             const std::string& fileName,
                             std::ofstream& strm)
        {}

        /*!
         This method will be called just before a file is closed. It may be used to
         write any desired trailing data, or to update any current data (say, for example
         to overwrite portions of any header information).
         @param rf the RollingFile that is calling the method
         @param fileName the full name of the current file
         @param strm the output stream of the current file
         */
        virtual void willClose(RollingFile& rf,
                               const std::string& fileName,
                               std::ofstream& strm)
        {}

        /*!
         This method will be called just after a file is closed.
         @param rf the RollingFile that is calling the method
         @param fileName the full name of the file that was just closed
         */
        virtual void didClose(RollingFile& rf, const std::string& fileName) {}
    };


    /*!
     The RollingFile class writes files with a maximum size. When the size is passed, the
     file is closed and a new file is opened. Note that the filenames will be of the form

     @code
     <fileNamePrefix><counter>[.<fileNameSuffix>]
     @endcode

     where counter is automatically generated, starting with 000001. If the value
     exceeds 999999, the RollingFile will continue to work, but the filenames will
     grow longer.
     */
    class RollingFile {
    public:

        /*!
         Start a rolling file with the given prefix, suffix (extension), and maximum,
         file size. Note that the file is created here but will be empty until the
         first write takes place.

         @param fileWrapSize a new file is started after a write takes the size over this
         @param fileNamePrefix the prefix, including path, of the files to be written
         @param fileNameSuffix if provided, will be appended after a "dot"
         @throws std::invalid_argument if fileWrapSize is not positive or if
            fileNamePrefix is empty.
         */
        RollingFile(std::streamsize fileWrapSize,
                    const std::string& fileNamePrefix,
                    const std::string& fileNameSuffix = "");

        /*!
         Note that if there is an error while closing the current file, it will be logged
         via syslog, but the destructor will not pass on the exception. If you need to
         handle any exceptions in this case, you should call the close() method.
         */
        ~RollingFile();

        // Moving is allowed, copying is not.
        RollingFile(RollingFile&&) = default;
        RollingFile& operator=(RollingFile&&) = default;

        /*!
         Set the optional listener if you need to be informed about when the individual
         files are opened and closed. Note that the object given here must remain valid
         at least for the life of the RollingFile object.
         */
        void setListener(RollingFileListener& listener);

        /*!
         Write to the current file. If after this write the file is larger than the
         fileWrapSize, it will be closed and a file new will be started. Note that if
         you seek within the file as part of fn(), you must be certain to seek to the
         end of the file before you return from fn(), otherwise the size computation
         will be incorrect and the next write will not occur in the correct place.

         @param fn the function that will be called and passed the current output stream
         @throws std::system_error if there is a problem closing or opening the file
         @throws any exception that fn may throw
         @throws any exception that the listener methods may throw
         */
        void write(const std::function<void(std::ofstream& strm)>& fn);

        /*!
         Close the final file. This is not truly necessary (the destructor will do this
         automatically), but if you wish to receive any exceptions that may occur while
         closing the file, including those that may be thrown by the listener, you should
         call this method.

         Note that after calling this method, you should not call write() again.

         @throws any exception that may be thrown by the listener willClose() or
            didClose() methods.
         */
        void close();

    private:
        const std::streamsize   maximumFileSize;
        const std::string       prefix;
        const std::string       suffix;
        unsigned                nextFileIndex = 0U;
        bool                    hasPermanentlyClosed = false;
        std::string             currentFileName;
        std::ofstream*          currentStream = nullptr;
        RollingFileListener*    listener = nullptr;
    };
}}}

#endif
