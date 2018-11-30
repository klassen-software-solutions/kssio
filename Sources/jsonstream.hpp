//
//  jsonstream.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2016-11-05.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_jsonstream_h
#define kssio_jsonstream_h

#include <iostream>
#include <memory>
#include <vector>


namespace kss { namespace io { namespace stream { namespace json {

    class Document;

    /*!
     Read the next Document from the input stream. This may throw any error that
     an istream may throw in addition to the following.
     @throws std::bad_alloc if we could not allocate space for the document.
     */
    std::istream& operator>>(std::istream& is, Document& d);

    /*!
     Representation of a single JSON node

     Objects of this type will only be valid as long as the Document class that
     they belong to remains in scope. (They are really just views into the Document
     implementation.)
     */
    class Node {
    public:
        Node(Node&& n);
        Node(const Node& n);
        Node& operator=(Node&& n) noexcept;
        Node& operator=(const Node& n);
        ~Node() noexcept;

        /*!
         Determine the type of the node.
         */
        bool isScalar() const noexcept;
        bool isArray() const noexcept;
        bool isObject() const noexcept;

        /*!
         Returns true if the node has no content.
         */
        bool empty() const noexcept;

        /*!
         Two nodes are equal if their types and content are equal.
         */
        bool equal(const Node& n) const noexcept;
        bool operator==(const Node& n) const noexcept { return equal(n); }
        bool operator!=(const Node& n) const noexcept { return !equal(n); }

        /*!
         A "special" Node that does not point to anything. Note that empty() will
         return true for this Node and isScalar(), isArray(), and isObject()
         will all return false. The key purpose of this Node is for algorithms
         and methods to return if they are to return a non-existance result.
         */
        static const Node none;

        /*!
         Check the type of a node.
         */
        bool isString() const noexcept;
        bool isBool() const noexcept;
        bool isDouble() const noexcept;
        bool isLong() const noexcept;
        bool isUnsignedLong() const noexcept;

        /*!
         Scalar node access. These methods are only valid if isScalar() will return true.
         The "is" methods can be used to determine if the node can be represented
         as the equivalent type. (i.e. If this fails then the corresponding cast
         will throw the bad_cast exception.)
         @throws std::bad_cast if the node is not of a suitable type.
         */
        explicit operator std::string() const;
        explicit operator bool() const;
        explicit operator double() const;
        explicit operator long() const;
        explicit operator unsigned long() const;

    private:
        friend class Document;
        friend class ArrayNode;
        friend class ObjectNode;

        /*!
         The representation and the construction of a node are hidden in order
         to ensure that the libyaml details are not in the API itself. The Document
         class is "friended" as that is the only object allowed to create nodes.
         */
        struct Impl;
        Impl* _impl;

        Node() : _impl(nullptr) {}
        explicit Node(Impl* impl) : _impl(impl) {}

        friend std::istream& operator>>(std::istream& is, Document& d);
    };


    /*!
     Representation of a single JSON document

     Note that unless specified as a noexcept method, the methods in the class may
     throw the following exceptions in addition to any specified in the individual
     method descriptions.

     @throws std::bad_alloc if a memory allocation fails
     @throws std::system_error if an underlying system call fails.

     @warning Note that this class uses a lazy-copy, reference counting scheme. This
      implies that two copies of the same document are not necessarily thread-safe. If
      you need separate copies in separate threads, and are planning to modify one or
      more of them, you will need to protect them appropriately, or call ensure_unique
      to ensure that they are truly separate copies before accessing them in separate
      threads.
     */
    class Document {
    public:

        /*!
         Construct an empty document.
         */
        Document();

        /*!
         Construct a document from a JSON string.
         @throws std::invalid_argument if jsonStr is empty
         @throws kss::io::ParsingError if the string cannot be parsed as a JSON document.
         */
        explicit Document(const std::string& jsonStr);

        /*!
          Copy a document from another one. Note that we use a lazy copy scheme where
          no physical duplicate is made until necessary. (i.e. until one of the instances
          wants to make a change, which has not yet been implemented) Hence we don't bother
          to include a move semantics version as that would not be any more efficient.
          If you need to truly ensure that a copy has been made (e.g. if you want to use
          it in multiple threads then you should call ensure_unique to force a true copy.
         */
        Document(const Document& d);
        Document& operator=(const Document& d);

        /*!
         Move a document.
         */
        Document(Document&& d);
        Document& operator=(Document&& d) noexcept;

        /*!
         Ensure the object instance is unique. If necessary, this will create a
         new, deep-copy, implementation of the object.
         */
        void ensureUnique();

        /*!
         Destroy a document and free its resources.
         */
        ~Document() noexcept;

        /*!
         Two documents are considered equal if their contents are equal.
         */
        bool equal(const Document& d) const noexcept;
        bool operator==(const Document& d) const noexcept { return equal(d); }
        bool operator!=(const Document& d) const noexcept { return !equal(d); }

        /*!
         Returns true if the document is empty and false otherwise.
         */
        bool empty() const noexcept;

        /*!
         Destroy everything in the document, returning it to an empty state.
         */
        void clear() noexcept;

        /*!
         Return the root node of the document.
         */
        Node root() const;

    private:
        friend class Node;
        struct Impl;
        std::shared_ptr<Impl> _impl;

        friend std::istream& operator>>(std::istream& is, Document& d);
    };

} } } }

#endif
