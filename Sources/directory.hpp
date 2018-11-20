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
         @throws std::invalid_argument if dirName is not a directory
         */
        explicit Directory(const std::string& dirName);
        ~Directory() noexcept = default;
        Directory(const Directory&) = delete;
        Directory& operator=(const Directory&) = delete;
        Directory(Directory&&) = default;
        Directory& operator=(Directory&&) noexcept = default;

        /*!
         Obtain the iterators.
         */
        const_iterator begin() const noexcept    { return const_iterator(_dir_name); }
        const_iterator end() const noexcept      { return const_iterator(_dir_name, true); }

        /*!
         Return the name of the directory.
         */
        const std::string& name() const noexcept { return _dir_name; }

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
        std::string _dir_name;

    public:
		class const_iterator
        : public std::iterator< std::forward_iterator_tag, std::string, ptrdiff_t,
                                const std::string*, const std::string& >
		{
		public:
			const_iterator(const std::string& dirName, bool endFlag = false);
			~const_iterator() noexcept;
			reference operator*() const noexcept { return _currentValue; }
			pointer operator->() const noexcept  { return &_currentValue; }
			const_iterator& operator++();
			const_iterator operator++(int);
			bool operator==(const const_iterator& rhs) const noexcept {
                return ((_dirName == rhs._dirName) && (_currentValue == rhs._currentValue));
            }
			bool operator!=(const const_iterator& rhs) const noexcept {
                return ! operator==(rhs);
            }

		private:
			void*       _dirptr;        // This is a DIR*, void* used to avoid need
			std::string _currentValue;  // to include dirent.h in the header.
			std::string _dirName;
		};
    };


} } }

#endif
