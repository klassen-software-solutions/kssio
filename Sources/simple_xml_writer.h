//
//  simple_xml_writer.h
//  kssio
//
//  Created by Steven W. Klassen on 2018-04-20.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_simple_xml_writer_h
#define kssio_simple_xml_writer_h

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace kss { namespace io { namespace stream { namespace xml {

	/*!
	 This namespace provides methods for efficiently creating XML output. The requirements
	 for this project were as follows:

	 1. Single-file header-only implementation suitable for embedding into other projects.
	 2. No dependancies other than a modern C++ compiler (i.e. no third-party libraries).
	 3. Ability to stream XML data without having to have the entire thing in memory.

	 The intention is not a full-grown XML package, but rather to be able to create XML
	 output efficiently.
	 */
	namespace simple_writer {

		using namespace std;

		struct Node;

		/*!
		 An XML child is represented by a node generator function. The function must return
		 a node* with each call. The returned pointer must remain valid until the next call.
		 When there are no more children to be produced, it should return nullptr. This
		 design was chosen to allow the XML to be streamed without all being in memory at
		 once.

		 Note that the generator can be a lambda, or it could be a generator class that has
		 a method "node* operator()() { ... }". In either case it will need to maintain
		 its own state so that it will know what to return on each call.
		 */
		using node_generator_fn = function<Node*(void)>;

		/*!
		 An XML node is represented by a key/value pair mapping (which become the attributes)
		 combined with an optional child generator (which become the children).
		 */
		struct Node {
			string								name;
			map<string, string>					attributes;
			string								text;
			mutable vector<node_generator_fn>	children;

			/*!
			 Convenience access for setting attributes.
			 */
			string& operator[](const string& key) {
                assert(!key.empty());
				return attributes[key];
			}

			/*!
			 Reset the node to an empty one.
			 */
			void clear() noexcept {
				name.clear();
				attributes.clear();
				text.clear();
				children.clear();

                // postconditions
                assert(name.empty());
                assert(attributes.empty());
                assert(text.empty());
                assert(children.empty());
			}
		};


		// Don't call anything in this "namespace" manually.
		struct _private {

			static ostream& indent(ostream& strm, int indentLevel) {
                assert(indentLevel >= 0);
				for (auto i = 0; i < indentLevel; ++i) { strm << "  "; }
				return strm;
			}

			// Based on code found at
			// https://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
			static string encode(const string& data, int indentLevel = -1) {
                // preconditions
                assert(!data.empty());

				string buffer;
				buffer.reserve(data.size());
				for(size_t pos = 0; pos != data.size(); ++pos) {
					switch(data[pos]) {
						case '&':  buffer.append("&amp;");       break;
						case '\"': buffer.append("&quot;");      break;
						case '\'': buffer.append("&apos;");      break;
						case '<':  buffer.append("&lt;");        break;
						case '>':  buffer.append("&gt;");        break;
						case '\n':
							if (indentLevel > 0) {
								buffer.append("\n");
								while (indentLevel--) buffer.append("  ");
							}
							break;
						default:   buffer.append(&data[pos], 1); break;
					}
				}

                // postconditions
                assert(buffer.size() >= data.size());
				return buffer;
			}

            static ostream& writeWithIndent(ostream& strm, const Node& n, int indentLevel) {
                // preconditions
				assert(!n.name.empty());
				assert(indentLevel >= 0);
				assert(n.text.empty() || n.children.empty());

				// Start the node.
				const bool singleLine = (n.text.empty() && n.children.empty());
				indent(strm, indentLevel);
				strm << '<' << n.name;

				// Write the attributes.
				for (auto& attr : n.attributes) {
					assert(!attr.first.empty());
					assert(!attr.second.empty());
					strm << ' ' << attr.first << "=\"" << encode(attr.second) << '"';
				}
				strm << (singleLine ? "/>" : ">") << endl;

				if (!singleLine) {
					// Write the contents.
					if (!n.text.empty()) {
						indent(strm, indentLevel+1);
						strm << encode(n.text, indentLevel+1) << endl;
					}

					// Write the children.
					for (auto fn : n.children) {
                        Node* child = fn();
						while (child) {
                            writeWithIndent(strm, *child, indentLevel+1);
							child = fn();
						}
					}

					// End the node.
					indent(strm, indentLevel);
					strm << "</" << n.name << '>' << endl;
				}

				return strm;
			}
		};


		/*!
		 Write an XML object to a stream.
		 @returns the stream
		 @throws any exceptions that the stream writing may throw.
		 */
        inline ostream& write(ostream& strm, const Node& root) {
			strm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
            return _private::writeWithIndent(strm, root, 0);
		}
	}
}}}}

#endif
