# kssio
C++ I/O library (networking, files, and streams)

## Prerequisites

This library has no prerequisites other than a C++14 compiler and the standard library.

## Contributing

If you wish to make changes to this library that you believe will be useful to others, you can
contribute to the project. If you do, there are a number of policies you should follow:

* Check the issues to see if there are already similar bug reports or enhancements.
* Feel free to add bug reports and enhancements at any time.
* Don't work on things that are not part of an approved project.
* Don't create projects - they are only created the by owner.
* Projects are created based on conversations on the wiki.
* Feel free to initiate or join conversations on the wiki.

In addition, code should follow our coding standards as described below:

### General Coding Principles

1. Be very familiar with [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).
If you choose to write code that breaks one of these, be prepared to justify your decision.
2. Favour reliability and readability over strict formatting guidelines, but if you choose to break a 
formatting guideline, be prepared to justify your decision.
3. Write unit tests that cover your code. The code must pass the unit tests before your pull request will be accepted.
4. Follow the "programming by contract" patterns. 
5. Follow the C++14 standard. (We won't switch to C++17 until 2020.)
6. Don't work on the master branch - create your own suitably named branch to work on.
7. Don't merge your branch into master - create a pull request so we have an opportunity to review your work first.
8. Eliminate all compiler warnings. If you absolutely cannot eliminate a warning, then suppress it
using the apprpriate #pragma, but be prepared to justify your decision.

### General Code Formatting

#### Header files

* Everything in the header files should be in the namespace `kss::io` or a sub-space within it.
* Items in the header files that should be considered private, but for technical reasons cannot be should
either be placed in the namespace `kss::io::_private` or should individually begine with an underscore.
* C++ header files should use the extention '.hpp' and be all lower case.
* C header files should use the extention '.h' and be all lower case.
* C/C++ mixed header files should use '.h' and must have appropriate `#if defined(__cplusplus)` statements.
* Header files do not have to be limited to a single class. But everything in them should be
reasonably related.
* Names of headers files that are for compiling only (i.e. that do not need to be installed with the library) should 
start with an underscore.
* Header files must be protected using an `#ifdef` of the form `kssio_<filenamebase>_hpp`.
* Keep implementation details out of the header except for very simple classes and structures. Make use of 
the [PIMPL Pattern](https://en.cppreference.com/w/cpp/language/pimpl) when suitable.
* Include documenting comments with the public portion of an API. Use a syntax that will work both for 
Doxygen and Xcode Documentation.
* Do not use `using namespace ...` in your header files except where they are reasonably scope limited.

#### Source files

* C++ source files should use the extension '.cpp', be all lower case, and should match their '.hpp'.
* C source files should use the extension '.c', be all lower case, and should match their '.h'.
* Indent using 4 spaces (no tabs except where required) at the usual locations.
* Keep lines to no more than 95 characters in length. (It should be possible to view two source
files side by side using a 12pt font on a reasonably sized monitor.)
* Keep methods and other blocks to no more than 50 lines in length. (It should be possible to
view an entire block on a reasonable laptop screen.)
* Prefer the use of anonymous namespaces over the "static" keyword.
* When including header files group them in the order of "standard c++ libraries" (e.g. `#include <string>`), 
"standard OS libraries" (e.g. `#include <sys/wait.h>`), 
"third party libraries" (e.g. `#include <curl/curl.h>`, 
and finally "local includes" (e.g. `#include "binary_file.hpp"`).
* Local includes should use the form `#include "filename"`. All other includes should use the 
form `#include <filename>`.
* Use `using namespace ...`, `namespace = ...` and `use something_t = ...` to reduce the need to 
explicitly refer to namespaces in your code.
* Don't repeat the "documentation" comments that are already in the header file in the source file.
(Our Doxygen configuration ignores the source files and only examines the header files on the
assumption that the API does not depend on the implementation.)
* Don't comment the obvious. It should be possible to determine what your code does by 
examining the code itself. Comments should primarily be used to describe why you chose a
specific implementation, or why you are breaking from a standard, or just to make the code more 
navigable in the IDEs (e.g. MARK: and TODO:).

#### Naming convensions

* Namespaces: all lower case and single word.
* Classes: CamelCase except for certain cases where we want to make them look like a keyword.
* Variables (local and member): camelCase.
* Member variables that would conflict with a method name: _underscorePrefixedCamelCase.
* "Private" items: This refers to things that should be treated as private but for technical reasons cannot be 
explicitly made private. They should either be placed in a sub-namespace named `_private` or just given a 
single underscore prefix.
* Type aliases: lowercase_t (e.g. `use requestlist_t = list<Request>;`)
* Macros: ALL_UPPERCASE - but keep macros scarce. Work hard to avoid them if possible,
and be prepared to justify their use.

#### Braces

Always use braces in decision and looping constructs. For single line items these may be on the same line,
but do not have to be. For multiple line items they should not be combined on the same line. Also
don't put the opening brace on a separate line, unless the statement requires more than one line.

The following are acceptable:

    if (somethingIsTrue) { ...do something... }

    if (somethingIsTrue) {
        ...do something... 
    }

    if (somethingIsTrue 
        && thisVariable == "hello world"
        && userHasPermissionToDoThis(action))
    {
        ...do something...
    }

The following are not acceptable:

    if (somethingIsTrue) doSomething();
    
    if (somethingIsTrue) { doSomething(); doSomethingElse(); }

    if (somethingIsTrue)
    {
        ...do something...
    }

#### Whitespace

Do not use newlines liberally. In particular, do not use them to make your code "double spaced."
Newlines should be used to draw the eye to logically grouped sections of the code.
