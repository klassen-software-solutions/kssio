//
//  directory.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_directory_hpp
#define kssio_directory_hpp

#include <cstddef>
#include <iterator>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>

namespace kss { namespace io { namespace file {


    /*!
      Return the current working directory as a string. This is a wrapper around
      the getcwd unix function.
      @throws std::system_error if the system call results in an error.
     */
    std::string getCwd();

	/*!
	 Ensure that a directory exists. More specifically, if dir already exists, is accessible,
	 and is a directory, then we do nothing. Otherwise we attempt to create the directory
	 with the given permissions.

	 Note that this will create all the directories needed in order to build the full
	 path specified by dir.

     @throws std::invalid_argument if dir is the empty string
	 @throws std::system_error if there is a problem creating the path.
	 */
	void ensurePath(const std::string& dir, mode_t permissions = 0755);

    /*!
     Remove a directory. More specifically, if dir already exists it is removed. If
     recursive is true, then the directory and all its contents will be removed, otherwise
     it will only be removed if it is empty. If the directory does not exist, this will
     quietly do nothing.

     @throws std::invalid_argument if dir is the empty string
     @throws std::invalid_argument if dir exists but is not a directory
     @throws std::system_error if there is a problem with the underlying C calls.
     */
    void removePath(const std::string& dir, bool recursive = false);

    /*!
      The directory class is a wrapper around the dirent C calls. It provides an
      iterator based approach to accessing directory entries.
     */
    class Directory {
    public:
        class               const_iterator; ///< This iterator follows forward iterator semantics.
        typedef std::string value_type;
        typedef size_t      size_type;

        /*!
         Construct or destroy the object. Note that constructing a directory object is
         not creating a directory on the drive. It is creating an object used to iterate
         over a directory on a drive.

         Note that if you are ignoring hidden entries, then count() and empty() are also
         affected by that settings. That is, empty() can then return true but the directory
         still will not actually be empty, it will just be empty of any non-hidden files.

         @param dirName the path to the directory to be examined
         @param ignoreHidden if true then any entry starting with a "." is ignored
         @throws std::invalid_argument if dirName is not a directory
         */
        explicit Directory(const std::string& dirName, bool ignoreHidden = false);
        ~Directory() noexcept = default;

        // Moving is allowed, copying is not.
        Directory(const Directory&) = delete;
        Directory& operator=(const Directory&) = delete;
        Directory(Directory&&) = default;
        Directory& operator=(Directory&&) noexcept = default;

        /*!
         Obtain the iterators.
         */
        const_iterator begin() const noexcept {
            return const_iterator(directoryName, ignoreHidden);
        }
        const_iterator end() const noexcept {
            return const_iterator(directoryName, ignoreHidden, true);
        }

        /*!
         Return the name of the directory.
         */
        const std::string& name() const noexcept { return directoryName; }

        /*!
         Return the number of entries in the directory.
         @throws std::sytem_error if there is a problem with the underlying C calls
         */
        size_type size() const;

        /*!
         Returns true if the directory is empty.
         @throws std::system_error if there is a problem with the underlying C calls.
         */
        bool empty() const;

        /*!
         Comparison functions. Two directories are considered equal if they contain
         exactly the same entries.
         @throws std::system_error if there is a problem with an underlying C call.
         */
        bool operator==(const Directory& rhs) const;
        bool operator!=(const Directory& rhs) const { return !operator==(rhs); }

    private:
        const std::string   directoryName;
        const bool          ignoreHidden;

    public:
		class const_iterator
        : public std::iterator< std::forward_iterator_tag, std::string, ptrdiff_t,
                                const std::string*, const std::string& >
		{
		public:
			const_iterator(const std::string& dirName,
                           bool ignoreHidden,
                           bool endFlag = false);
            const_iterator(const const_iterator&) = default;
            const_iterator(const_iterator&&) = default;
            const_iterator& operator=(const const_iterator&) = default;
            const_iterator& operator=(const_iterator&&) = default;
			~const_iterator() noexcept;
			reference operator*() const noexcept { return currentValue; }
			pointer operator->() const noexcept  { return &currentValue; }
			const_iterator& operator++();
			const_iterator operator++(int);
			bool operator==(const const_iterator& rhs) const noexcept {
                return ((directoryName == rhs.directoryName)
                        && (currentValue == rhs.currentValue));
            }
			bool operator!=(const const_iterator& rhs) const noexcept {
                return ! operator==(rhs);
            }

		private:
			void*               dirptr;        // This is a DIR*, void* used to avoid need
			std::string         currentValue;  // to include dirent.h in the header.
			const std::string   directoryName;
            const bool          ignoreHidden;
		};
    };


} } }

#endif
