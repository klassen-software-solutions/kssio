//
//  binary_file.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_binary_file_hpp
#define kssio_binary_file_hpp

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include "iterator.hpp"
#include "utility.hpp"

namespace kss { namespace io { namespace file {

    /*!
     Binary file class. While the C++ stream objects can be used for binary data, they
     are really intended for and optimized for the streaming of string data. This class,
     which is really a wrapper around the C FILE* type, is designed specifically to
     handle the reading and writing of binary data.
     */
    class BinaryFile {
    public:
        /*! 
         Modes used when opening a file. Note that reading, writing, and appending are mutually
         exclusive while updating may be or'ed with any of them in order to open the file
         for both input and output. This is done as a series of constants instead of as
         an enumeration in order to allow the bitwise addition to be used.
         */
        using mode_t = uint8_t;
        static constexpr mode_t reading = 0x8;
        static constexpr mode_t writing = 0x4;
        static constexpr mode_t appending = 0x2;
        static constexpr mode_t updating = 0x1;

        /*!
         Open/close a file. Note that the default constructor will not be a usable object.
         It's only purpose will be as a temporary placeholder until another file is move
         assigned into it.

         @throws std::invalid_argument if fp is nullptr or filedes is not a non-negative
            integer or if openMode is invalid.
         @throws std::system_error if the underlying C routines return an error code.
         */
        BinaryFile() = default;
        explicit BinaryFile(const std::string& filename, mode_t openMode = reading);
        explicit BinaryFile(FILE* fp);
        explicit BinaryFile(int filedes);
        BinaryFile(BinaryFile&& f);
        BinaryFile(const BinaryFile&) = delete;
        ~BinaryFile() noexcept;

        BinaryFile& operator=(BinaryFile&& f) noexcept;
        BinaryFile& operator=(const BinaryFile&) = delete;

        /*!
         Read/write the data in a file. Note that the "fully" versions will either read or
         write the number of bytes given or will fail with an eof exception.

         @param buf A buffer to read the data into or write the data from.
         @param n The number of bytes to read or write.
         @return the number of bytes actually read or written.
         @throws kss::io::Eof if the "fully" read reaches the end of the file before
            reading n bytes.
         @throws std::invalid_argument if buf is nullptr or n is 0.
         @throws std::system_error if the underlying C routines return an error code.
         */
        size_t read(void* buf, size_t n);
        size_t write(const void* buf, size_t n);
        void readFully(void* buf, size_t n);
        void writeFully(const void* buf, size_t n);
        void flush();

        /*!
         Report on and change the position in the file. Note that seek and move are limited
         by the size of a long int. Position and set_position can be used to move anywhere
         in a file.
         @throws std::system_error if the underlying C routines return an error code.
         */
        bool eof() const noexcept;  ///< Returns true if the file has reached its end.
        off_t tell() const;         ///< Return the current position in the file.
        void seek(off_t pos);       ///< Set the current position in the file.
        void move(off_t offset);    ///< Move the file position forward or backwards.
        void rewind();              ///< Same as seek(0) but may be more efficient.
        void fastForward();         ///< Seeks to the end of the file.

        /*!
         Returns true if the file is valid and is open for the given mode.
         @throws std::invalid_argument if mode does not match a valid open mode
         @throws std::system_error if there is a problem with an underlying C call
         */
        bool isOpenFor(mode_t mode) const;

        /*!
         Direct access to the internal file handle. Use with care.
         */
        FILE* handle() noexcept { return _fp; }

    private:
        FILE*   _fp = nullptr;
        bool    _autoclose = false;
    };


    /*!
     Read a data structure from a binary file. Note that it is required that Record
     be an object that can be read using a simple read of sizeof(Struct) bytes. In
     other words it should be a simple structure containing simple types, or something
     very similar.
     @throws std::invalid_argument if f is not a valid, open file
     @throws std::system_error if the file read fails.
     */
    template <class Record>
    Record read(BinaryFile& f, const Record& arg_type = Record()) {
        if (!f.isOpenFor(BinaryFile::reading)) {
            throw std::invalid_argument("file is not open for reading");
        }

        const auto pos = f.tell();
        Record rec;
        f.readFully(&rec, sizeof(rec));

        // postconditions
        if (!(f.tell() == (pos + sizeof(rec)))) {
            _KSSIO_POSTCONDITIONS_FAILED
        }
        return rec;
    }

    /*!
     Write a data structure to a binary file. Note that it is required that Record
     be an object that can be written using a simple write of sizeof(Struct) bytes. In
     other words it should be a simple structure containing simple types, or something
     very similar.
     @throws std::invalid_argument if f is not a valid, open file
     @throws std::system_error if the file read/write fails.
     */
    template <class Record>
    void write(BinaryFile& f, const Record& rec) {
        if (!f.isOpenFor(BinaryFile::writing)) {
            throw std::invalid_argument("file is not open for writing");
        }

        const auto pos = f.tell();
        f.writeFully(&rec, sizeof(rec));

        // postconditions
        if (!(f.isOpenFor(BinaryFile::appending) || (f.tell() == (pos + sizeof(rec))))) {
            _KSSIO_POSTCONDITIONS_FAILED
        }
    }


    /*!
     Record based file handling. This template class is built on top of the BinaryFile
     class but restricting it to only write records of a given type.
     
     This class is intended to work with records that are essentially struct objects
     containing just primitives. Specifically they are read/written directly to their
     space in memory.
     */
    template <class Record>
    class FileOf : private BinaryFile {
    public:
        /*!
         Iterator types for reading (input iterator) and writing (output iterator).
         */
        using input_iterator = kss::io::stream::InputIterator<FileOf<Record>, Record>;
        using output_iterator = kss::io::stream::OutputIterator<FileOf<Record>, Record>;

        /*!
         Open/close a file. Note that the default constructor will not be a usable object.
         It's only purpose will be as a temporary placeholder until another file is move
         assigned into it.
         @throws std::invalid_argument if openMode is not valid or if fp is nullptr or
            filedes is not a non-negative integer or if the file does not appear to contain
            the correct type as determined by its current size on opening and the sizeof the 
            encoding of the record type.
         @throws std::system_error if the underlying C routines return an error code
         */
        FileOf() = default;

        explicit FileOf(const std::string& filename, mode_t openMode = BinaryFile::reading)
        : BinaryFile(filename, openMode)
        {
            if (!(isOpenFor(BinaryFile::appending) || (position() == 0))) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }

        explicit FileOf(FILE* fp) : BinaryFile(fp) {
            if (!(isOpenFor(BinaryFile::appending) || (position() == 0))) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }

        explicit FileOf(int filedes) : BinaryFile(filedes) {
            if (!(isOpenFor(BinaryFile::appending) || (position() == 0))) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }

        FileOf(FileOf&& f) = default;
        FileOf(const FileOf&) = delete;

        FileOf& operator=(FileOf&&) noexcept = default;
        FileOf& operator=(const FileOf&) = delete;

        ~FileOf() noexcept = default;


        /*!
         Read/write data in the file. Note that the position in the file refers to the
         record number, starting with 0, and not the raw position in bytes. This will work
         on any size file, performing multiple seek and move calls if necessary (if the
         position would go farther than a long int allows). At the end of each call, the
         new position in the file will be at the end of the record that was just read
         or written. The versions that do not specify a record number will read or write
         at the current position in the file.

         @throws std::invalid_argument if recNo is not in the file or one past the end (for writing).
         @throws std::system_error if the underlying C routines return an error code
         */
        Record read() {
            // preconditions
            if (!(isOpenFor(reading))) {
                _KSSIO_PRECONDITIONS_FAILED
            }

            const auto pos = position();
            Record r = kss::io::file::read(*this, Record());

            // postconditions
            if (!(position() == (pos+1))) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
            return r;
        }

        inline Record read(size_t recNo) {
            setPosition(recNo);
            return read();
        }

        inline FileOf& operator>>(Record& r) {
            r = std::move(read());
            return *this;
        }

        void write(const Record& r) {
            // preconditions
            if (!(isOpenFor(writing))) {
                _KSSIO_PRECONDITIONS_FAILED
            }

            const auto pos = position();
            kss::io::file::write(*this, r);

            // postconditions
            if (!(position() == (pos+1))) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }

        /*!
         Write a specific record. Note that this should not be called if the file
         was opened for appending, as then recNo would be ignored and all writes
         will append to the end of the file.
         */
        inline void write(const Record& r, size_t recNo) {
            // preconditions
            if (!(isOpenFor(writing))
                || !(!isOpenFor(appending)))
            {
                _KSSIO_PRECONDITIONS_FAILED
            }

            setPosition(recNo);
            write(r);
        }

        inline FileOf& operator<<(const Record& r) {
            write(r);
            return *this;
        }

        inline void flush() {
            return BinaryFile::flush();
        }

        /*!
         Iterator access for reading. These are only applicable if the file was opened
         for reading. Note that begin() and ibegin() will also reposition the
         file to the beginning. You should not mix the iterator calls with other reading/
         writing methods as that will mess up the position of the file and lead to
         undefined results. After using the iterators you should call set_position()
         before using another read/write method. Also note that these are all essentially
         "const" iterators. Namely changing the values contained in them does not make
         any change in the underlying file.
         @throws std::system_error if the underlying C routines return an error code
         */
        input_iterator begin() {
            // preconditions
            if (!(isOpenFor(BinaryFile::reading))) {
                _KSSIO_PRECONDITIONS_FAILED
            }

            rewind();
            return input_iterator(*this);
        }

        input_iterator end()    { return input_iterator(); }
        input_iterator ibegin() { return begin(); }
        input_iterator iend()   { return end(); }

        /*!
         Iterator access for writing. This requires that the file be opened for writing
         or appending. You should not mix the iterator calls with other reading/writing
         methods as that will mess up the position of the file and lead to undefined
         results. After using the iterators you should call setPosition() before using
         another read/write method.
         @throws std::logic_error (specifically InvaidState) if the file was not
            opened for writing
         @throws std::system_error if the underlying C routines return an error code.
         */
        output_iterator obegin() {
            // preconditions
            if (!(isOpenFor(BinaryFile::writing) || isOpenFor(BinaryFile::appending))) {
                _KSSIO_PRECONDITIONS_FAILED
            }

            fastForward();
            return output_iterator(*this);
        }

        /*!
         Report on and move the position in the file. Note that the position is in terms of
         the record number, not the position in bytes.
         @throws std::invalid_argument if the position is not within the file or just at its end
         @throws std::system_error if the underlying C routines return an error code
         */
        size_t position() const noexcept {
            return (size_t)(BinaryFile::tell() / sizeof(Record));
        }

        void setPosition(size_t recNo)  {
            BinaryFile::seek(recNo * sizeof(Record));

            // postconditions
            if (!(position() == recNo)) {
                _KSSIO_POSTCONDITIONS_FAILED
            }
        }
        void rewind() noexcept      { BinaryFile::rewind(); }
        void fastForward()          { BinaryFile::fastForward(); }
        bool eof() const noexcept   { return BinaryFile::eof(); }
    };

} } }

#endif
