//
//  simple_json_writer.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-04-14.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_simple_json_writer_h
#define kssio_simple_json_writer_h

#include <cassert>
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace kss { namespace io { namespace stream { namespace json {

	/*!
	 This namespace provides methods for efficiently creating JSON output. The
	 requirements for this project were as follows:

	 1. Single-file header-only implementation suitable for embedding into other projects.
	 2. No dependancies other than a modern C++ compiler (i.e. no third-party libraries).
	 3. Ability to stream JSON data without having to have the entire thing in memory.
	 4. Keys can be assumed to be strings.
	 5. Values will be assumed to be numbers if they "look like" a number.
	 6. Children will always be in arrays.

	 The intention is not a full-grown JSON package (i.e. it is not the equivalent
	 of kss::util::json::Document), but rather to be able to create JSON output
	 efficiently.
	 */
	namespace simple_writer {

		using namespace std;

		struct Node;

		/*!
		 A JSON child is represented by a key and a node generator function. The function
		 should return a node* with each call. The pointer must remain valid until the
		 next call. When there are no more children, it should return nullptr.

		 Note that the generator can be a lambda, or it could be a generator class that
		 has a method "node* operator()() { ... }". In either case it will need to maintain
		 its own state so that it will know what to return on each call.
		 */
		using node_generator_fn = function<Node*(void)>;
		using array_child_t = pair<string, node_generator_fn>;

		/*!
		 A JSON node is represented by a key/value pair mapping combined with an optional
		 child generator. In the mapping values that appear to be numeric will not be
		 quoted, all others will assumed to be strings and will be escaped and quoted.
		 */
		struct Node {
			map<string, string>				attributes;
			mutable vector<array_child_t>	arrays;

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
				attributes.clear();
				arrays.clear();

                // postconditions
                assert(attributes.empty());
                assert(arrays.empty());
			}
		};

		
		// Don't call anything in this "namespace" manually.
		struct _private {

			// This is based on code found at
			// https://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
			static inline bool isNumber(const string& s) {
				return !s.empty() && find_if(s.begin(), s.end(), [](char c) { return !(isdigit(c) || c == '.'); }) == s.end();
			}

			// The following is based on code found at
			// https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c/33799784#33799784
			static string encodeJson(const string &s) {
				// If it is a number we need to escapes or quotes.
				if (isNumber(s)) {
					return s;
				}

				// Otherwise we must escape it and quote it.
				ostringstream o;
				o << '"';
				for (auto c = s.cbegin(); c != s.cend(); c++) {
					switch (*c) {
						case '"': o << "\\\""; break;
						case '\\': o << "\\\\"; break;
						case '\b': o << "\\b"; break;
						case '\f': o << "\\f"; break;
						case '\n': o << "\\n"; break;
						case '\r': o << "\\r"; break;
						case '\t': o << "\\t"; break;
						default:
							if ('\x00' <= *c && *c <= '\x1f') {
								o << "\\u"
								<< std::hex << std::setw(4) << std::setfill('0') << (int)*c;
							} else {
								o << *c;
							}
					}
				}
				o << '"';
				return o.str();
			}

			static ostream& indent(ostream& strm, int indentLevel, int extraSpaces = 0) {
                assert(indentLevel >= 0);
				for (auto i = 0; i < (indentLevel*4)+extraSpaces; ++i) {
					strm << ' ';
				}
				return strm;
			}

			static ostream& writeWithIndent(ostream& strm,
                                            const Node& json,
                                            int indentLevel,
                                            bool needTrailingComma)
			{
                // preconditions
                assert(indentLevel >= 0);

				indent(strm, indentLevel);
				strm << '{' << endl;

				auto lastIt = --json.attributes.end();
				for (auto it = json.attributes.begin(); it != json.attributes.end(); ++it) {
					indent(strm, indentLevel, 2);
					strm << '"' << it->first << "\": " << encodeJson(it->second);
					if (it != lastIt || json.arrays.size() > 0) strm << ',';
					strm << endl;
				}

				const auto last = --json.arrays.end();
				for (auto it = json.arrays.begin(); it != json.arrays.end(); ++it) {
					writeChildInArray(strm, indentLevel, *it, it == last);
				}

				indent(strm, indentLevel);
				strm << '}';
				if (needTrailingComma) strm << ',';
				strm << endl;

				return strm;
			}

			static ostream& writeChildInArray(ostream& strm,
                                              int indentLevel,
                                              array_child_t& child,
                                              bool isLastChild)
            {
                // preconditions
                assert(indentLevel >= 0);

				indent(strm, indentLevel, 2);
				strm << '"' << child.first << "\": [" << endl;

                Node* json = child.second();
				while (json) {
                    Node tmp = *json;
                    Node* next = child.second();
					writeWithIndent(strm, tmp, indentLevel+1, (next != nullptr));
					json = next;
				}

				indent(strm, indentLevel, 2);
				strm << "]";
                if (!isLastChild) { strm << ','; }
				strm << endl;
				return strm;
			}
		};



		/*!
		 Write a JSON object to a stream.
		 @returns the stream
		 @throws any exceptions that the stream writing may throw.
		 */
        inline ostream& write(ostream& strm, const Node& json) {
			return _private::writeWithIndent(strm, json, 0, false);
		}
	}

} } } }

#endif
