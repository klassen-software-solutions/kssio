//
//  yamlstream.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-05-04.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <system_error>
#include <typeinfo>

#include <yaml.h>

#include "_contract.hpp"
#include "_raii.hpp"
#include "_substring.hpp"
#include "_tokenizer.hpp"
#include "yaml_error_category.hpp"
#include "yamlstream.hpp"

using namespace std;
using namespace kss::io::stream::yaml;

namespace contract = kss::io::contract;

using kss::io::_private::finally;
using kss::io::_private::substring_t;
using kss::io::_private::Tokenizer;


// MARK: internal utilities
namespace {

    // Throw a yaml system error.
    inline void throwYaml(yaml_error_type_t err, const char* fnName) {
        throw system_error(err, yamlErrorCategory(), fnName);
    }

    // Duplicate and add a node into the document.
    void duplicate_and_add_node(yaml_document_t* doc,
                                const yaml_document_t* srcdoc,
                                const yaml_node_t* node,
                                bool useFlowStyle)
    {
        switch (node->type) {
            case YAML_SCALAR_NODE:
                if (!yaml_document_add_scalar(doc,
                                              node->tag,
                                              node->data.scalar.value,
                                              (int)node->data.scalar.length,
                                              node->data.scalar.style))
                {
                    throwYaml(YAML_COMPOSER_ERROR, "yaml_document_add_scalar");
                }
                break;
            case YAML_SEQUENCE_NODE:
                if (!yaml_document_add_sequence(doc, node->tag,
                                                (useFlowStyle
                                                 ? YAML_FLOW_SEQUENCE_STYLE
                                                 : node->data.sequence.style)))
                {
                    throwYaml(YAML_COMPOSER_ERROR, "yaml_document_add_sequence");
                }
                break;
            case YAML_MAPPING_NODE:
                if (!yaml_document_add_mapping(doc,
                                               node->tag,
                                               (useFlowStyle
                                                ? YAML_FLOW_MAPPING_STYLE
                                                : node->data.mapping.style)))
                {
                    throwYaml(YAML_COMPOSER_ERROR, "yaml_document_add_mapping");
                }
                break;
            default:
                assert(0);
                break;
        }
    }

    // Copy a document. Adapted from the libyaml example source code.
    void copy_document(yaml_document_t *document_to,
                       const yaml_document_t *document_from,
                       bool useFlowStyle)
    {
        try {
            yaml_node_t *node = nullptr;

            if (!yaml_document_initialize(document_to,
                                          document_from->version_directive,
                                          document_from->tag_directives.start,
                                          document_from->tag_directives.end,
                                          document_from->start_implicit,
                                          document_from->end_implicit))
            {
                throwYaml(YAML_MEMORY_ERROR, "yaml_document_initialize");
            }

            // First we make a duplicate of all the nodes.
            for (node = document_from->nodes.start; node < document_from->nodes.top; ++node) {
                duplicate_and_add_node(document_to, document_from, node, useFlowStyle);
            }

            // Then we fix up the children of any sequence or mapping nodes.
            int nodeId = 1;
            for (node = document_from->nodes.start;
                 node < document_from->nodes.top;
                 ++node, ++nodeId)
            {
                if (node->type == YAML_SEQUENCE_NODE) {
                    for (yaml_node_item_t* item = node->data.sequence.items.start;
                         item < node->data.sequence.items.top;
                         ++item)
                    {
                        if (!yaml_document_append_sequence_item(document_to, nodeId, *item)) {
                            throwYaml(YAML_COMPOSER_ERROR, "yaml_document_append_sequence_item");
                        }
                    }
                }
                else if (node->type == YAML_MAPPING_NODE) {
                    for (yaml_node_pair_t* pair = node->data.mapping.pairs.start;
                         pair < node->data.mapping.pairs.top;
                         pair++)
                    {
                        if (!yaml_document_append_mapping_pair(document_to,
                                                               nodeId,
                                                               pair->key,
                                                               pair->value))
                        {
                            throwYaml(YAML_COMPOSER_ERROR, "yaml_document_append_mapping_pair");
                        }
                    }
                }
            }
        }
        catch (const exception&) {
            // If an error occurs we need to cleanup the document.
            yaml_document_delete(document_to);
            throw;
        }
    }

    // Returns 1 if two nodes are the same and 0 otherwise. Adapted from the libyaml sample code.
    int compare_nodes(const yaml_document_t* document1, const yaml_node_t* node1,
                      const yaml_document_t* document2, const yaml_node_t* node2);

    int compare_nodes(const yaml_document_t *document1, int index1,
                      const yaml_document_t *document2, int index2)
    {
        return compare_nodes(document1, yaml_document_get_node(const_cast<yaml_document_t*>(document1), index1),
                             document2, yaml_document_get_node(const_cast<yaml_document_t*>(document2), index2));
    }

    int compare_nodes(const yaml_document_t* document1, const yaml_node_t* node1,
                      const yaml_document_t* document2, const yaml_node_t* node2)
    {
        int k;

        assert(node1);
        assert(node2);

        if (node1->type != node2->type) {
            return 0;
        }

        if (strcmp((char *)node1->tag, (char *)node2->tag) != 0) {
            return 0;
        }

        switch (node1->type) {
            case YAML_SCALAR_NODE:
                if (node1->data.scalar.length != node2->data.scalar.length) {
                    return 0;
                }
                if (strncmp((char *)node1->data.scalar.value,
                            (char *)node2->data.scalar.value,
                            node1->data.scalar.length) != 0)
                {
                    return 0;
                }
                break;
            case YAML_SEQUENCE_NODE:
                if ((node1->data.sequence.items.top - node1->data.sequence.items.start) !=
                    (node2->data.sequence.items.top - node2->data.sequence.items.start))
                {
                    return 0;
                }
                for (k = 0;
                     k < (node1->data.sequence.items.top - node1->data.sequence.items.start);
                     k ++)
                {
                    if (!compare_nodes(document1, node1->data.sequence.items.start[k],
                                       document2, node2->data.sequence.items.start[k]))
                    {
                        return 0;
                    }
                }
                break;
            case YAML_MAPPING_NODE:
                if ((node1->data.mapping.pairs.top - node1->data.mapping.pairs.start) !=
                    (node2->data.mapping.pairs.top - node2->data.mapping.pairs.start))
                {
                    return 0;
                }
                for (k = 0;
                     k < (node1->data.mapping.pairs.top - node1->data.mapping.pairs.start);
                     k ++)
                {
                    if (!compare_nodes(document1, node1->data.mapping.pairs.start[k].key,
                                       document2, node2->data.mapping.pairs.start[k].key))
                    {
                        return 0;
                    }
                    if (!compare_nodes(document1, node1->data.mapping.pairs.start[k].value,
                                       document2, node2->data.mapping.pairs.start[k].value))
                    {
                        return 0;
                    }
                }
                break;
            default:
                assert(0);
                break;
        }
        return 1;
    }

    // Returns 1 if two documents are the same and 0 otherwise. Adapted from the libyaml sample code.
    int compare_documents(const yaml_document_t *document1, const yaml_document_t *document2) {
        int k;

        if ((document1->version_directive && !document2->version_directive)
            || (!document1->version_directive && document2->version_directive)
            || (document1->version_directive && document2->version_directive
                && (document1->version_directive->major != document2->version_directive->major
                    || document1->version_directive->minor != document2->version_directive->minor)))
        {
            return 0;
        }

        if ((document1->tag_directives.end - document1->tag_directives.start) !=
            (document2->tag_directives.end - document2->tag_directives.start))
        {
            return 0;
        }
        for (k = 0; k < (document1->tag_directives.end - document1->tag_directives.start); k ++) {
            if ((strcmp((char *)document1->tag_directives.start[k].handle,
                        (char *)document2->tag_directives.start[k].handle) != 0)
                || (strcmp((char *)document1->tag_directives.start[k].prefix,
                           (char *)document2->tag_directives.start[k].prefix) != 0))
            {
                return 0;
            }
        }

        if ((document1->nodes.top - document1->nodes.start) !=
            (document2->nodes.top - document2->nodes.start))
        {
            return 0;
        }
        
        if (document1->nodes.top != document1->nodes.start) {
            if (!compare_nodes(document1, 1, document2, 1)) {
                return 0;
            }
        }
        
        return 1;
    }

    // Handler used by the stream writing.
    int emitter_write_handler(void *data, unsigned char *buffer, size_t size) {
        ostream* os = static_cast<ostream*>(data);
        size_t maxSingleWrite = numeric_limits<streamsize>::max();
        const char* pos = (const char*)buffer;
        while (size > maxSingleWrite) {
            os->write(pos, streamsize(maxSingleWrite));
            size -= maxSingleWrite;
            pos += maxSingleWrite;
        }
        os->write(pos, streamsize(size));
        return 1;
    }

    // Write a document to an output stream.
    ostream& write_to_stream(ostream& os,
                             const yaml_document_t* d,
                             int canonical,
                             unsigned indent,
                             bool useFlowStyle)
    {
        yaml_emitter_t emitter;
        if (!yaml_emitter_initialize(&emitter)) {
            throw bad_alloc();
        }

        finally deleteEmitter([&]{ yaml_emitter_delete(&emitter); });
        yaml_emitter_set_canonical(&emitter, canonical);
        yaml_emitter_set_unicode(&emitter, 1);
        yaml_emitter_set_width(&emitter, -1);
        if (!canonical && !useFlowStyle) {
            if (indent < 1 || indent > 10) {
                throw range_error("indent must be betwen 1 and 10");
            }
            yaml_emitter_set_indent(&emitter, int(indent));
        }
        yaml_emitter_set_output(&emitter, emitter_write_handler, &os);

        if (!yaml_emitter_open(&emitter)) {
            throwYaml(YAML_EMITTER_ERROR, "yaml_emitter_open");
        }

        // Since the emitter destroys the doc, we need to make a copy.
        yaml_document_t doc;
        copy_document(&doc, d, useFlowStyle);
        if (!yaml_emitter_dump(&emitter, &doc)) {
            yaml_document_delete(&doc);
            throwYaml(YAML_EMITTER_ERROR, "yaml_emitter_dump");
        }

        if (!yaml_emitter_close(&emitter)) {
            yaml_document_delete(&doc);
            throwYaml(YAML_EMITTER_ERROR, "yaml_emitter_close");
        }

        yaml_emitter_delete(&emitter);
        return os;
    }
}

// MARK: private impl classes

// Internal representation of a yaml node.
struct Node::NodeImpl {
    NodeImpl() = default;
    NodeImpl(const yaml_document_t* d, const yaml_node_t* n) : doc(d), node(n) {}

    NodeImpl(const yaml_document_t* d, int nodeId) : doc(d) {
        node = yaml_document_get_node(const_cast<yaml_document_t*>(d), nodeId);
        if (!node) {
            throw out_of_range("Invalid nodeId for this document");
        }
    }

    NodeImpl(const NodeImpl& n) = delete;
    NodeImpl& operator=(const NodeImpl& n) = delete;

    ~NodeImpl() noexcept = default;

    const yaml_document_t*  doc = nullptr;
    const yaml_node_t*      node = nullptr;
};

// Internal representation of a yaml document.
struct Document::DocumentImpl {
    DocumentImpl() {
        memset(&doc, 0, sizeof(yaml_document_t));
        haveDoc = false;
    }

    ~DocumentImpl() noexcept {
        if (haveDoc) {
            yaml_document_delete(&doc);
        }
    }

    DocumentImpl(const DocumentImpl& d) = delete;
    DocumentImpl& operator=(const DocumentImpl& d) = delete;

    yaml_document_t doc;
    bool            haveDoc = false;
};


// MARK: node and related classes

Node::Node() = default;
Node::Node(Node&& n) = default;
Node& Node::operator=(Node&& n) noexcept = default;
Node::~Node() noexcept = default;

Node::Node(const Node& n) {
    if (!n._impl) {
        _impl.reset(nullptr);
    }
    else {
        _impl.reset(new NodeImpl());
        _impl->doc = n._impl->doc;
        _impl->node = n._impl->node;
    }

    contract::postconditions({
        KSS_EXPR((bool)n._impl
                 ? (_impl->doc == n._impl->doc && _impl->node == n._impl->node)
                 : _impl.get() == nullptr)
    });
}

Node::Node(Node::NodeImpl* impl) {
    _impl.reset(impl);

    contract::postconditions({
        KSS_EXPR(_impl.get() == impl)
    });
}

Node& Node::operator=(const Node& n) {
    if (&n != this) {
        if (!n._impl) {
            _impl.reset(nullptr);
        }
        else {
            _impl.reset(new NodeImpl());
            _impl->doc = n._impl->doc;
            _impl->node = n._impl->node;
        }
    }

    contract::postconditions({
        KSS_EXPR((bool)n._impl
                 ? (_impl->doc == n._impl->doc && _impl->node == n._impl->node)
                 : _impl.get() == nullptr)
    });
    return *this;
}

// Returns the node tag.
string Node::tag() const noexcept {
    return (!_impl || !_impl->node->tag
            ? string()
            : string((const char*)_impl->node->tag));
}

// Determine the node type.
bool Node::isScalar() const noexcept {
    return (static_cast<bool>(_impl) && (_impl->node->type == YAML_SCALAR_NODE));
}
bool Node::isSequence() const noexcept {
    return (static_cast<bool>(_impl) && (_impl->node->type == YAML_SEQUENCE_NODE));
}
bool Node::isMapping() const noexcept {
    return (static_cast<bool>(_impl) && (_impl->node->type == YAML_MAPPING_NODE));
}

// Determine if a node is empty.
bool Node::empty() const noexcept {
    if (!_impl) {
        return true;
    }

    switch (_impl->node->type) {
        case YAML_SCALAR_NODE:
            return (_impl->node->data.scalar.length == 0);
        case YAML_SEQUENCE_NODE:
        {
            const yaml_node_t* node = _impl->node;
            return (node->data.sequence.items.start >= node->data.sequence.items.top);
        }
        case YAML_MAPPING_NODE:
        {
            const yaml_node_t* node = _impl->node;
            return (node->data.mapping.pairs.start >= node->data.mapping.pairs.top);
        }
        default:
            return true;
    }
}

// Returns the canonical representation of the node.
namespace {
    string escape(const string& s) {
        string s2 = s;
        while (s2.find("\n") != string::npos) {
            substring_t(s2, "\n") = "\\n";
        }
        return s2;
    }

    void append_node(ostream& os, const yaml_document_t* doc, const yaml_node_t* n,
                     unsigned indent, bool firstIndent);

    void append_tag(ostream& os,
                    const yaml_char_t* tag,
                    const char* defaultTag,
                    const char* defaultTagToPrint)
    {
        if (!strcmp((const char*)tag, defaultTag)) {
            os << defaultTagToPrint;
        }
        else {
            os << "!<" << tag << ">";
        }
    }

    void append_null(ostream& os,
                     const yaml_document_t* doc,
                     const yaml_node_t* n,
                     unsigned indent,
                     bool firstIndent)
    {
        if (firstIndent) {
            while (indent--) {
                os << "  ";
            }
        }
        append_tag(os, n->tag, YAML_NULL_TAG, "!!null");
    }

    void append_scalar(ostream& os,
                       const yaml_document_t* doc,
                       const yaml_node_t* n,
                       unsigned indent, bool
                       firstIndent)
    {
        if (firstIndent) {
            while (indent--) {
                os << "  ";
            }
        }
        append_tag(os, n->tag, YAML_STR_TAG, "!!str");
        os << " \"";
        string s((const char*)n->data.scalar.value, n->data.scalar.length);
        os << escape(s);
        os << "\"";
    }

    void append_sequence(ostream& os,
                         const yaml_document_t* doc,
                         const yaml_node_t* n,
                         unsigned indent,
                         bool firstIndent)
    {
        if (firstIndent) {
            for (unsigned u = indent; u > 0; --u) {
                os << "  ";
            }
        }

        append_tag(os, n->tag, YAML_SEQ_TAG, "!!seq");
        os << " [" << endl;
        for (const yaml_node_item_t* item = n->data.sequence.items.start;
             item < n->data.sequence.items.top;
             ++item)
        {
            append_node(os, doc,
                        yaml_document_get_node(const_cast<yaml_document_t*>(doc), *item),
                        indent+1, true);
            os << "," << endl;
        }

        while (indent--) {
            os << "  ";
        }
        os << "]";
    }

    void append_mapping(ostream& os,
                        const yaml_document_t* doc,
                        const yaml_node_t* n,
                        unsigned indent,
                        bool firstIndent)
    {
        if (firstIndent) {
            for (unsigned u = indent; u > 0; --u) {
                os << "  ";
            }
        }

        append_tag(os, n->tag, YAML_MAP_TAG, "!!map");
        os << " {" << endl;
        for (const yaml_node_pair_t* pair = n->data.mapping.pairs.start;
             pair < n->data.mapping.pairs.top;
             ++pair)
        {
            for (unsigned u = indent+1; u > 0; --u) {
                os << "  ";
            }
            os << "? ";
            append_node(os, doc,
                        yaml_document_get_node(const_cast<yaml_document_t*>(doc), pair->key),
                        indent+1, false);
            os << endl;

            for (unsigned u = indent+1; u > 0; --u) {
                os << "  ";
            }
            os << ": ";
            append_node(os, doc,
                        yaml_document_get_node(const_cast<yaml_document_t*>(doc), pair->value),
                        indent+1, false);
            os << "," << endl;
        }
        while (indent--) {
            os << "  ";
        }
        os << "}";
    }

    void append_node(ostream& os,
                     const yaml_document_t* doc,
                     const yaml_node_t* n,
                     unsigned indent,
                     bool firstIndent)
    {
        switch (n->type) {
            case YAML_NO_NODE:      append_null(os, doc, n, indent, firstIndent);       break;
            case YAML_SCALAR_NODE:  append_scalar(os, doc, n, indent, firstIndent);     break;
            case YAML_SEQUENCE_NODE:append_sequence(os, doc, n, indent, firstIndent);   break;
            case YAML_MAPPING_NODE: append_mapping(os, doc, n, indent, firstIndent);    break;
            default:                                                                    break;
        }
    }
}

string Node::canonical() const noexcept {
    if (!_impl) {
        return string();
    }

    ostringstream oss;
    oss << "---" << endl;
    append_node(oss, _impl->doc, _impl->node, 0, true);
    oss << endl;

    contract::postconditions({
        KSS_EXPR(!oss.str().empty())
    });
    return oss.str();
}

// Check for equality.
bool Node::equal(const Node& n) const noexcept {
    if (&n == this) {
        return true;
    }
    if (!_impl) {
        return (n._impl.get() == nullptr);
    }
    if (!n._impl) {
        if (_impl) return false;
    }
    return (compare_nodes(_impl->doc, _impl->node, n._impl->doc, n._impl->node) == 1);
}

// Return the value of the node.
string Node::value() const {
    if (!isScalar()) {
        throw bad_cast();
    }
    if (_impl->node->data.scalar.length == 0) {
        return string();
    }

    contract::postconditions({
        KSS_EXPR(_impl->node->data.scalar.value != nullptr)
    });
    return string((const char*)_impl->node->data.scalar.value);
}

Node::operator bool() const {
    const auto s = value();
    if (s == "1" || s == "t" || s == "T" || s == "true" || s == "TRUE") {
        return true;
    }
    if (s == "0" || s == "f" || s == "F" || s == "false" || s == "FALSE") {
        return false;
    }
    throw bad_cast();
}

namespace {
    // "borrowed" from the old C version of kssutil
    template<class T>
    inline T lexical_cast_via_fn_call(const string& s, T (*fn)(const char*, char**)) {
        char* endptr;
        errno = 0;
        T t = (*fn)(s.c_str(), &endptr);
        if (errno || *endptr != '\0') {
            throw bad_cast();
        }
        return t;
    }

    template<class T>
    inline T lexical_cast_via_fn_call2(const string& s, T (*fn)(const char*, char**, int)) {
        char* endptr;
        errno = 0;
        T t = (*fn)(s.c_str(), &endptr, 0);
        if (errno || *endptr != '\0') {
            throw bad_cast();
        }
        return t;
    }
}

Node::operator float() const {
    return lexical_cast_via_fn_call<float>(value(), strtof);
}

Node::operator double() const {
    return lexical_cast_via_fn_call<double>(value(), strtod);
}

Node::operator int() const {
    const auto s = value();
    const auto l = lexical_cast_via_fn_call2<long>(s, strtol);
    if (l > numeric_limits<int>::max() || l < numeric_limits<int>::min()) {
        throw out_of_range(s + " is out of range for an int");
    }
    return static_cast<int>(l);
}

Node::operator unsigned() const {
    const auto s = value();
    const auto ul = lexical_cast_via_fn_call2<unsigned long>(s, strtoul);
    if (ul > numeric_limits<unsigned>::max()) {
        throw out_of_range(s + " is too large for unsigned");
    }
    return static_cast<unsigned>(ul);
}

Node::operator long() const {
    return lexical_cast_via_fn_call2<long>(value(), strtol);
}

Node::operator unsigned long() const {
    return lexical_cast_via_fn_call2<unsigned long>(value(), strtoul);
}


// Return the ith child of a sequence node.
Node Node::operator[](size_t i) const {
    if (!isSequence()) {
        throw std::bad_cast();
    }
    
    const yaml_node_item_t* start = _impl->node->data.sequence.items.start;
    const yaml_node_item_t* end = _impl->node->data.sequence.items.top;
    contract::conditions({
        KSS_EXPR(end >= start)
    });

    ptrdiff_t len = (end - start);
    if (len <= 0 || i >= size_t(len)) {
        return none;
    }

    contract::postconditions({
        KSS_EXPR(_impl->doc != nullptr),
        KSS_EXPR(_impl->node->data.sequence.items.start != nullptr),
        KSS_EXPR(i < len)
    });
    return Node(new Node::NodeImpl(_impl->doc, _impl->node->data.sequence.items.start[i]));
}

// Access a child of a mapping node.
Node Node::operator[](const string& s) const {
    if (!isMapping()) {
        throw std::bad_cast();
    }

    for (const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start;
         p < _impl->node->data.mapping.pairs.top;
         ++p)
    {
        Node k(new NodeImpl(_impl->doc, p->key));
        if (k.isScalar() && k.value() == s) {
            return Node(new NodeImpl(_impl->doc, p->value));
        }
    }
    return Node::none;
}

// Path based access to a node.
static const string ROOT("---start-at-root-node---");
static const string WILDCARD("*");

static bool tokenIsIndex(const string& token, size_t& idx) {
    try {
        idx = stoul(token);
        return true;
    }
    catch (const invalid_argument&) {
        return false;
    }
}

vector<Node> Node::select(const string& path) const {
    contract::parameters({
        KSS_EXPR(!path.empty())
    });
    contract::preconditions({
        KSS_EXPR(_impl->doc != nullptr)
    });

    // Step 1, parse the path into a list of tokens. Note that if the first character is '/'
    // we insert a special "start at root node" token, then parse the rest of the string.
    // Once done we scan the tokens to check that none are empty (which would be invalid).
    vector<string> pathTokens;
    {
        size_t startPos = 0;
        if (path[0] == '/') {
            pathTokens.push_back(ROOT);
            ++startPos;
        }

        Tokenizer t(path, "/", false, startPos);
        pathTokens.insert(pathTokens.end(), t.begin(), t.end());
        for (const string& tok : pathTokens) {
            if (tok.empty()) {
                throw invalid_argument("path should have no empty sections");
            }
        }
    }
    contract::conditions({
        KSS_EXPR(!pathTokens.empty())
    });

    // Step 2, traverse the path until we reach the end, collecting the matching nodes.
    vector<const yaml_node_t*> nodesToConsider;
    {
        nodesToConsider.push_back(_impl->node);
        vector<const yaml_node_t*> newNodesToConsider;
        yaml_document_t* doc = const_cast<yaml_document_t*>(_impl->doc);

        for (const string& tok : pathTokens) {
            size_t idx;
            assert(newNodesToConsider.empty());

            if (nodesToConsider.empty()) {          // A special case, our path has found an empty set so
                break;                              // we return an empty set.
            }

            if (tok == ROOT) {                      // Start with the root node.
                newNodesToConsider.push_back(yaml_document_get_node(doc, 1));
            }
            else if (tok == WILDCARD) {             // Wildcards must be either sequence or mapping nodes, consider them all
                for (auto n : nodesToConsider) {
                    if (n->type == YAML_SEQUENCE_NODE) {
                        for (int k = 0; k < (n->data.sequence.items.top - n->data.sequence.items.start); ++k) {
                            newNodesToConsider.push_back(yaml_document_get_node(doc, n->data.sequence.items.start[k]));
                        }
                    }
                    else if (n->type == YAML_MAPPING_NODE) {
                        for (int k = 0; k < (n->data.mapping.pairs.top - n->data.mapping.pairs.start); ++k) {
                            newNodesToConsider.push_back(yaml_document_get_node(doc, n->data.mapping.pairs.start[k].value));
                        }
                    }
                    else {
                        throw std::bad_cast();
                    }
                }
            }
            else if (tokenIsIndex(tok, idx)) {    // Index specifies a single child of a sequence
                for (auto n : nodesToConsider) {
                    if (n->type != YAML_SEQUENCE_NODE) {
                        throw std::bad_cast();
                    }
                    assert(n->data.sequence.items.top >= n->data.sequence.items.start);
                    size_t max = size_t(n->data.sequence.items.top - n->data.sequence.items.start);
                    if (idx < max) {
                        newNodesToConsider.push_back(yaml_document_get_node(doc, n->data.sequence.items.start[idx]));
                    }
                }
            }
            else {                                  // Otherwise it must be the name of a child of a mapping
                for (auto n : nodesToConsider) {
                    for (int k = 0; k < (n->data.mapping.pairs.top - n->data.mapping.pairs.start); ++k) {
                        yaml_node_t* key = yaml_document_get_node(doc, n->data.mapping.pairs.start[k].key);
                        if (key->type == YAML_SCALAR_NODE
                            && !strncmp((const char*)key->data.scalar.value, tok.c_str(), key->data.scalar.length))
                        {
                            newNodesToConsider.push_back(yaml_document_get_node(doc, n->data.mapping.pairs.start[k].value));
                        }
                    }
                }
            }

            nodesToConsider.swap(newNodesToConsider);
            newNodesToConsider.clear();
        }
    }

    // Step 3, whatever is currently in our nodes to consider will be transformed into our
    // Node wrappers and returned.
    vector<Node> matchingNodes;
    for (const yaml_node_t* nptr : nodesToConsider) {
        matchingNodes.push_back(Node(new NodeImpl(_impl->doc, nptr)));
    }
    return matchingNodes;
}


Node Node::find(const string& path) const {
    vector<Node> matches = select(path);
    if (matches.empty()) {
        return none;
    }
    return matches.front();
}

string Node::value(const string& path) const {
    vector<Node> matches = select(path);
    if (matches.empty()) {
        return string();
    }
    return matches.front().value();
}

// Value that refers to no node.
const Node Node::none;





////
//// SEQUENCE
////

// Construct the wrapper.
Sequence::Sequence(const Node& n) {
    if (!n.isSequence()) {
        throw std::bad_cast();
    }
    _impl = make_unique<Node::NodeImpl>();
    _impl->doc = n._impl->doc;
    _impl->node = n._impl->node;

    contract::postconditions({
        KSS_EXPR(_impl->doc != nullptr),
        KSS_EXPR(_impl->node != nullptr)
    });
}

Sequence::Sequence(Node&& n) {
    if (!n.isSequence()) {
        throw std::bad_cast();
    }
    _impl = move(n._impl);

    contract::postconditions({
        KSS_EXPR(_impl->doc != nullptr),
        KSS_EXPR(_impl->node != nullptr)
    });
}

Sequence::~Sequence() noexcept = default;

// Return the number of items in the sequence.
size_t Sequence::size() const noexcept {
    const yaml_node_item_t* start = _impl->node->data.sequence.items.start;
    const yaml_node_item_t* end = _impl->node->data.sequence.items.top;
    if (start >= end) {
        return 0;
    }
    return (size_t)(end - start);
}

// Return the ith node.
Node Sequence::operator[](size_t i) const {
    return Node(new Node::NodeImpl(_impl->doc, _impl->node->data.sequence.items.start[i]));
}
Node Sequence::at(size_t i) const {
    if (i >= size()) {
        throw std::out_of_range("i must be less than size()");
    }
    return operator[](i);
}



////
//// MAPPING
////

// Construct the wrapper.
Mapping::Mapping(const Node& n) {
    if (!n.isMapping()) {
        throw std::bad_cast();
    }
    _impl = make_unique<Node::NodeImpl>();
    _impl->doc = n._impl->doc;
    _impl->node = n._impl->node;

    contract::postconditions({
        KSS_EXPR(_impl->doc != nullptr),
        KSS_EXPR(_impl->node != nullptr)
    });
}

Mapping::Mapping(Node&& n) {
    if (!n.isMapping()) {
        throw std::bad_cast();
    }
    _impl = move(n._impl);

    contract::postconditions({
        KSS_EXPR(_impl->doc != nullptr),
        KSS_EXPR(_impl->node != nullptr)
    });
}

Mapping::~Mapping() noexcept = default;

// Return the number of pairs in the map.
size_t Mapping::size() const noexcept {
    const yaml_node_pair_t* start = _impl->node->data.mapping.pairs.start;
    const yaml_node_pair_t* end = _impl->node->data.mapping.pairs.top;
    if (start >= end) {
        return 0;
    }
    return (size_t)(end - start);
}

// Return the ith pair.
pair<Node, Node> Mapping::operator[](size_t i) const {
    const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start + i;
    return make_pair(Node(new Node::NodeImpl(_impl->doc, p->key)),
                     Node(new Node::NodeImpl(_impl->doc, p->value)));
}
pair<Node, Node> Mapping::at(size_t i) const {
    if (i >= size()) {
        throw out_of_range("i must be less than size()");
    }
    return operator[](i);
}

// Count how many matches there are.
size_t Mapping::count(const string& key) const {
    size_t c = 0;
    for (const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start;
         p < _impl->node->data.mapping.pairs.top;
         ++p)
    {
        Node k(new Node::NodeImpl(_impl->doc, p->key));
        if (k.isScalar() && k.value() == key) {
            ++c;
        }
    }
    return c;
}
size_t Mapping::count(const Node& key) const {
    size_t c = 0;
    for (const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start;
         p < _impl->node->data.mapping.pairs.top;
         ++p)
    {
        Node k(new Node::NodeImpl(_impl->doc, p->key));
        if (k == key) {
            ++c;
        }
    }
    return c;
}

// Return the value for the given key.
Node Mapping::operator[](const string& key) const {
    for (const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start;
         p < _impl->node->data.mapping.pairs.top;
         ++p)
    {
        Node k(new Node::NodeImpl(_impl->doc, p->key));
        if (k.isScalar() && k.value() == key) {
            return Node(new Node::NodeImpl(_impl->doc, p->value));
        }
    }
    return Node::none;
}
Node Mapping::operator[](const Node& key) const {
    for (const yaml_node_pair_t* p = _impl->node->data.mapping.pairs.start;
         p < _impl->node->data.mapping.pairs.top;
         ++p)
    {
        Node k(new Node::NodeImpl(_impl->doc, p->key));
        if (k == key) {
            return Node(new Node::NodeImpl(_impl->doc, p->value));
        }
    }
    return Node::none;
}

// Find a given key and return an iterator to it.
Mapping::const_iterator Mapping::find(const string& key) const {
    Mapping::const_iterator it = begin();
    Mapping::const_iterator last = end();
    while (it != last) {
        if (it->first.isScalar() && it->first.value() == key) {
            return it;
        }
        ++it;
    }
    assert(it == last);
    return it;
}
Mapping::const_iterator Mapping::find(const Node& key) const {
    Mapping::const_iterator it = begin();
    Mapping::const_iterator last = end();
    while (it != last) {
        if (it->first == key) {
            return it;
        }
        ++it;
    }
    assert(it == last);
    return it;
}


////
//// DOCUMENT
////

// Construct an empty document.
Document::Document() : _impl(new DocumentImpl()) {
}

/*!
 Construct a document from a yaml string.
 */
Document::Document(const std::string& yamlStr) : _impl(new DocumentImpl()) {
    // Attempt to parse the string.
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        throw bad_alloc();
    }

    yaml_parser_set_input_string(&parser,
                                 (const unsigned char*)yamlStr.c_str(),
                                 yamlStr.length());

    if (!yaml_parser_load(&parser, &_impl->doc)) {
        yaml_parser_delete(&parser);
        throw system_error(parser.error, yamlErrorCategory(), "yaml_parser_load");
    }

    _impl->haveDoc = true;
    yaml_parser_delete(&parser);
}

// Copy a document.
Document::Document(const Document& d) {
    _impl = d._impl;
}

Document& Document::operator=(const Document& d) {
    if (&d != this) {
        _impl = d._impl;
    }
    return *this;
}

Document::~Document() noexcept = default;


// Ensure the given implementation is unique, replacing it with a new one when necessary.
void Document::ensureUnique() {
    if (!_impl.unique()) {
        shared_ptr<DocumentImpl> newImpl(new DocumentImpl());
        if (_impl->haveDoc) {
            copy_document(&newImpl->doc, &_impl->doc, false);
            newImpl->haveDoc = _impl->haveDoc;
        }
        _impl = newImpl;
    }

    contract::postconditions({
        KSS_EXPR(_impl.unique())
    });
}


// Determine if two documents are equal.
bool Document::equal(const Document& d) const noexcept {
    if (&d == this) {
        return true;
    }
    if (_impl.get() == d._impl.get()) {
        return true;
    }
    if (compare_documents(&_impl->doc, &d._impl->doc) == 1) {
        return true;
    }
    return false;
}

// Determine if a document is empty.
bool Document::empty() const noexcept {
    if (!_impl->haveDoc) {
        return true;
    }
    if (!yaml_document_get_root_node(&_impl->doc)) {
        return true;
    }
    return false;
}

// Destroy everything in the document.
void Document::clear() noexcept {
    if (_impl->haveDoc) {
        _impl = shared_ptr<DocumentImpl>(new DocumentImpl());
    }

    contract::postconditions({
        KSS_EXPR(empty())
    });
}

// Return the root node.
Node Document::root() const {
    return Node(new Node::NodeImpl(&_impl->doc, 1));
}

// Read the next document.
istream& kss::io::stream::yaml::operator>>(istream& is, Document& d) {
    // Read the next document into a string.
    d.clear();
    ostringstream nextDocStr;
    bool haveReadSomething = false;
    {
        char buffer[BUFSIZ];
        
        while (!is.eof() && !is.fail()) {
            buffer[0] = '\0';
            is.getline(buffer, BUFSIZ-1);
            if (!is.fail()) {
                if (buffer[0] == '#' || buffer[0] == '\0') { // Skip empty lines and full line comments.
                    continue;
                }
                
                if (!strncmp(buffer, "...", 3)) {            // Found the end of document marker.
                    break;
                }

                if (!strncmp(buffer, "---", 3)) {            // Found the start of document marker.
                    if (strlen(buffer) > 3) {
                        nextDocStr << buffer << endl;
                    }
                    if (haveReadSomething) {                 // And we have a doc, so stop.
                        break;
                    }
                    haveReadSomething = true;
                    continue;                                // If we don't have a doc, go to the next line.
                }
                
                nextDocStr << buffer << endl;
                haveReadSomething = true;
            }
        }
    }
    
    // Parse the string using the libyaml C library.
    if (haveReadSomething) {
        d = Document(nextDocStr.str());
    }
    
    return is;
}


// Write the document.
ostream& kss::io::stream::yaml::operator<<(ostream& os, const Document& d) {
    return write_to_stream(os, &d._impl->doc, 1, 0, false);
}

ostream& kss::io::stream::yaml::operator<<(ostream& os, const PrettyPrint& d) {
    return write_to_stream(os, &d._d._impl->doc, 0, d._indent, false);
}

ostream& kss::io::stream::yaml::operator<<(ostream& os, const FlowPrint& d) {
    return write_to_stream(os, &d._d._impl->doc, 0, 0, true);
}
