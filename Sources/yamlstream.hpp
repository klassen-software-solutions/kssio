//
//  yamlstream.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2014-05-04.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_yamlstream_hpp
#define kssio_yamlstream_hpp

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "random_access_iterator.hpp"
#include "utility.hpp"


/*!
 @file This file provides classes and overrides for the serialization of YAML types.
 
 Note that the underlying YAML handling is performed by the libyaml C library. If this
 library is not found on your system, the classes in this file will not work.
 
 */

namespace kss { namespace io { namespace stream { namespace yaml {

    // Forward declarations needed for the "friend" statements.
    class Document;
    class PrettyPrint;
    class FlowPrint;

    /*!
     Read the next Document from the input stream. This may throw any error that
     an istream may throw in addition to the following.
     @throws std::system_error with a group code of KSS_ERROR_YAML if there is a problem parsing the input.
     @throws std::bad_alloc if we could not allocate space for the document.
     */
    std::istream& operator>>(std::istream& is, Document& d);

    /*!
     Write a Document to the output stream. This may throw any errors that an ostream
     may throw in addition to the following one.
     @throws std::system_error with a group code of KSS_ERROR_YAML if there is a problem producing the output.
     */
    std::ostream& operator<<(std::ostream& os, const Document& d);
    std::ostream& operator<<(std::ostream& os, const PrettyPrint& d);
    std::ostream& operator<<(std::ostream& os, const FlowPrint& d);


    /*!
     Representation of a single yaml node

     Objects of this type will only be valid as long as the Document class that
     they belong to remains in scope. (They are really just views into the Document
     implementation.)
     */
    class Node {
    public:
        /*!
         Copy or move a node.
         */
        Node(Node&& n);
        Node(const Node& n);
        Node& operator=(Node&& n) noexcept;
        Node& operator=(const Node& n);

        ~Node() noexcept;

        /*!
         Returns the node tag.
         */
        std::string tag() const noexcept;

        /*!
         Returns the canonical string representation of the node.
         */
        std::string canonical() const noexcept;

        /*!
         Determine the type of the node.
         */
        bool isScalar() const noexcept;
        bool isSequence() const noexcept;
        bool isMapping() const noexcept;

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
         return true for this Node and isScalar(), isSequence(), and isMapping()
         will all return false. The key purpose of this Node is for algorithms
         and methods to return if they are to return a non-existance result.
         */
        static const Node none;

        /*!
         Scalar node access. These methods are only valid if isScalar() will return true.
         @throws std::bad_cast if the node is not a scalar node or if the string value
            cannot be represented by the requested type.
         */
        std::string value() const;
        explicit operator std::string() const { return value(); }
        explicit operator bool() const;
        explicit operator float() const;
        explicit operator double() const;
        explicit operator int() const;
        explicit operator unsigned() const;
        explicit operator long() const;
        explicit operator unsigned long() const;

        /*!
         Sequence node access. This will return the ith node of the sequence if it exists
         or none if it does not. Note that there is no way to check on the valid values
         of i beforehand with this method. See Sequence below for more robust handling of a
         sequence node.
         @throws std::bad_cast if the node is not a sequence node
         */
        Node operator[](size_t i) const;

        /*!
         Mapping node access. This will return the node that contains a Scalar node
         with the given text as its key. Returns none if no such key is found. Note
         that there is no way to check on the valid values for the keys beforehand
         with this method. See Mapping below for a more robust handling of a mapping
         node.
         */
        Node operator[](const std::string& s) const;

        /*!
         Path based access. Note that the path begins relative to this node.
         See Document for a further description on the path syntax.

         @throws std::invalid_argument if the path is not syntactically correct
         @throws std::bad_cast if the path does not "match" the nodes at some point.
         @return a vector of Nodes that match the path, empty if there are none.
         */
        std::vector<Node> select(const std::string& path) const;

        /*!
         Similar to select() but only returns a single node or none if there is no match.
         */
        Node find(const std::string& path) const;

        /*!
         Similar to select() but only returns the string value of the node at the end of
         the path or an empty string if there is none.
         */
        std::string value(const std::string& path) const;


    private:
        friend class Document;
        friend class Sequence;
        friend class Mapping;

        struct NodeImpl;
        std::unique_ptr<NodeImpl> _impl;

        Node();
        explicit Node(NodeImpl* impl);
    };


    /*!
     Container style access for a sequence node. This class wraps a sequence style
     node to provide a container style interface, including the ability to query the
     size, to obtain the individual children, and to iterate over the values using
     a random access iterator.

     Note that this class will only be valid as long as the Document of the wrapped
     Node is valid.
     */
    class Sequence {
    public:
        /*!
         Container type declarations.
         */
        using value_type = Node;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using const_iterator = kss::io::_private::CopyRandomAccessIterator<Sequence>;

        /*!
         Wrap a sequence node.
         @throws std::bad_cast if the underlying node is not a sequence node
         */
        explicit Sequence(const Node& n);
        explicit Sequence(Node&& n);
        ~Sequence() noexcept;

        bool empty() const noexcept { return (size() == 0); }
        size_t size() const noexcept;

        /*!
         Returns the ith node in this sequence. The result is undefined if i >= size().
         @throws std::bad_alloc if the memory could not be allocated
         @throws std:out_of_range if i >= size() (at method only)
         */
        Node operator[](size_t i) const;
        Node at(size_t i) const;
        Node front() const { return operator[](0); }
        Node back() const  { return operator[](size()-1); }

        /*!
         Iterator based access. Each iterator will dereference to a Node.
         */
        const_iterator begin() const noexcept { return const_iterator(*this, false); }
        const_iterator end() const noexcept { return const_iterator(*this, true); }

    private:
        std::unique_ptr<Node::NodeImpl> _impl;
    };


    /*!
     Container style access for a mapping node. This class wraps a mapping style
     node to provide a map-like container interface, including the ability to query
     the size, check for the existance of children, obtain individual childen using
     both keyname and key node keys and to iterate over the values using a random
     access interator. The random access iterator is available due to the nature
     of the underlying libyaml library. This also implies that although this is
     providing a map-like API, it is not providing map-like efficiency. Specifically
     each access is based on searching an array hence has O(n) efficiency.

     Note that this class will only be valid as long as the Document of the wrapped
     Node is valid.
     */
    class Mapping {
    public:
        /*!
         Container type declarations.
         */
        using key_type = Node;
        using mapped_type = Node;
        using value_type = std::pair<key_type, mapped_type>;
        using difference_type = ptrdiff_t;
        using size_type = size_t;
        using const_iterator = kss::io::_private::CopyRandomAccessIterator<Mapping>;

        /*!
         Wrap a mapping node.
         @throws std::bad_cast if the underlying node is not a sequence node
         */
        explicit Mapping(const Node& n);
        explicit Mapping(Node&& n);
        ~Mapping() noexcept;

        /*!
         Returns the number of pairs in this mapping.
         */
        bool empty() const noexcept { return (size() == 0); }
        size_t size() const noexcept;

        /*!
         Returns the ith mapping pair in this mapping. Note that the first value of
         the pair will be the key and the second value will be the value. If i >= size()
         the result is undefined (for operator[]) and an exception is thrown for at.
         @throws std::bad_alloc if the necessary memory could not be allocated
         @throws std::out_of_range if i >= size() (at method only)
         */
        std::pair<Node, Node> operator[](size_t i) const;
        std::pair<Node, Node> at(size_t i) const;

        /*!
         Returns true if the mapping contains the given key. Note that the string
         version implies a scalar node containing that string.
         @throws std::bad_alloc if the temporary memory needed could not be allocated
         */
        bool contains(const std::string& key) const { return (operator[](key) != Node::none); }
        bool contains(const Node& key) const        { return (operator[](key) != Node::none); }
        size_t count(const std::string& key) const;
        size_t count(const Node& key) const;

        /*!
         Returns the value associated with the given key.
         @returns Node::none if the key does not exist.
         @throws std::bad_alloc if the memory could not be allocated
         */
        Node operator[](const std::string& key) const;
        Node operator[](const Node& key) const;
        Node at(const std::string& key) const  { return operator[](key); }
        Node at(const Node& key) const         { return operator[](key); }

        /*!
         Iterator based access. Each iterator will dereference to a pair of nodes. Note that
         the find iterators will return end() if no such node was found.
         @throws std::bad_alloc if the necesary memory could not be allocated
         */
        const_iterator begin() const   { return const_iterator(*this, false); }
        const_iterator end() const     { return const_iterator(*this, true); }
        const_iterator find(const std::string& key) const;
        const_iterator find(const Node& key) const;

    private:
        std::unique_ptr<Node::NodeImpl> _impl;
    };


    /*!
     Representation of a single yaml document

     Note that unless specified as a noexcept method, the methods in the class may
     throw the following exceptions in addition to any specified in the individual
     method descriptions.

     @throws std::bad_alloc if a memory allocation fails
     @throws std::system_error if an underlying system call or libyaml call fails

     @warning Note that this class uses a lazy-copy, reference counting scheme. This
      implies that two copies of the same document are not necessarily thread-safe. If
      you need separate copies in separate threads, and are planning to modify one or
      more of them, you will need to protect them appropriately, or call ensureUnique
      to ensure that they are truly separate copies before accessing them in separate
      threads.
     */
    class Document {
    public:
        Document();
        explicit Document(const std::string& yamlStr);
        Document(const Document& d);
        Document& operator=(const Document& d);
        ~Document() noexcept;

        /*!
         Ensure the object instance is unique. If necessary, this will create a
         new, deep-copy, implementation of the object.
         */
        void ensureUnique();

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

        /*!
         Path based access. These are equivalent to calling the same methods on the
         root node of the Document.

         The path description follows a syntax of tokens separated by the '/' character.
         If the path starts with '/' then it is relative to the document root, even
         if called from a node, otherwise it is relative to the node. (Of course when
         called from the Document it will always be relative to the root node.)

         Each token in the patch will be one of the following:
         - the '*' character - this matches all nodes at the current level (sequence or mapping)
         - an unsigned integer - this matches that indexed node of a sequence style node
         - other strings - will match the named child of a mapping style node

         For example, suppose we have the following yaml:

         products:
          - sku: 1234
            name: chocolate bar
            price: 1.25
            number_in_stock: 12
          - sku: 456
            name: coke
            price: 2.50
            number_in_stock: 3
         staff:
          - name: jim
            position: sales
          - name: james
            position: sales
          - name: sue
            position: management

         Then the path "/products/1/name" would match the node "chocolate bar",
         and the path "/products/ * /name" (ignore the spaces) would match "chocolate bar" and "coke",
         and the path "/ * /name" (ignore the spaces) would match those plus "jim", "james" and "sue",
         and the path "/products/name" will throw a bad_cast as the child of the products node
         is a sequence node and not a mapping node.

         Note that in this path syntax the keys of the mapping nodes must be scalar nodes.
         I apologize in advance for not supporting the ypath "standard" but after much
         searching I was unable to find an actual description of that "standard", much
         less existing C/C++ code to support it, so I developed my own.

         In terms of efficiency, it is more efficient to use a path search than to follow
         the path manually using the Node::operator[] methods as the path is followed in
         the underlying implementation without creating the Node wrapper objects until
         the end.

         @throws std::invalid_argument if the path is not syntactically correct
         @throws std::bad_cast if the path does not "match" the nodes at some point.

         - select returns a vector of Nodes that match the path, empty if there are none.
         - find returns a single Node that matches the path or none.
         - value returns the string value of the single Node that patches the path, or
           an empty string if there is none.
         */
        std::vector<Node> select(const std::string& path) const { return root().select(path); }
        Node find(const std::string& path) const                { return root().find(path); }
        std::string value(const std::string& path) const        { return root().value(path); }

    private:
        friend class Node;
        struct DocumentImpl;                   // Hidden from the public to keep libyaml C declarations
        std::shared_ptr<DocumentImpl> _impl;   // out of the application code.

        friend std::istream& kss::io::stream::yaml::operator>>(std::istream& is, Document& d);
        friend std::ostream& kss::io::stream::yaml::operator<<(std::ostream& os,
                                                               const Document& d);
        friend std::ostream& kss::io::stream::yaml::operator<<(std::ostream& os,
                                                               const PrettyPrint& d);
        friend std::ostream& kss::io::stream::yaml::operator<<(std::ostream& os,
                                                               const FlowPrint& d);
    };


    /*!
     The PrettyPrint "class" acts as a custom io manipulator that changes the ostream
     output from canonical to a nicer print. Use it by including it in your stream,
     e.g. if d is a Document, then cout << PrettyPrint(d) will output a nicer print
     with a 2-space indentation and cout << PrettyPrint(d, 4) will do the same with
     a 4-space indentation.
     */
    class PrettyPrint {
    public:
        explicit PrettyPrint(const Document& d, unsigned indent = 2) : _d(d), _indent(indent) {}

    private:
        const Document& _d;
        unsigned        _indent;
        friend std::ostream& kss::io::stream::yaml::operator<<(std::ostream& os,
                                                               const PrettyPrint& d);
    };

    /*!
     The FlowPrint "class" acts as a custom io manipulator that changes the ostream
     output from canonical to the flow format. Use it by including it in your stream.
     e.g. if d is a document, then cout << FlowPrint(d) will output the document
     using the flow format for sequences and mappings.
     */
    class FlowPrint {
    public:
        explicit FlowPrint(const Document& d) : _d(d) {}

    private:
        const Document& _d;
        friend std::ostream& kss::io::stream::yaml::operator<<(std::ostream& os,
                                                               const FlowPrint& d);
    };

}}}}

// Override the mime type guess.
template <>
inline std::string kss::io::net::guessMimeType(const kss::io::stream::yaml::Document&) {
    return "application/x-yaml";
}

#endif
