//
//  _raii.hpp
//  ("borrowed" from kssutil - not part of the kssio public API)
//
//  Created by Steven W. Klassen on 2016-08-26.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_raii_h
#define kssio_raii_h

#include <functional>

namespace kss {
    namespace io {
        namespace _private {

            /**
             RAII (Resource acquisition is initialization) is used to implement the RAII
             pattern without always having to create a custom class.

             You give this class two lambdas. The first is the setup code which is run
             by the class constructor. The second is the cleanup code which is run by the
             class destructor. Hence the RAII pattern, the setup runs when the object
             comes into scope and the cleanup when it goes out of scope.

             This is structured like the following:

             <pre>
             {
                 RAII myRAII([&]{
                    cout << "this gets run at setup" << endl;
                 }, [&]{
                    cout << "this gets run at cleanup" << endl;
                 });

                 cout << "this is my application code" << endl;
             }
             </pre>

             This would produce the following output:

             <pre>
                this gets run at setup
                this is my application code
                this gets run at cleanup
             </pre>
             */
            class RAII {
            public:
                typedef std::function<void()> lambda;

                /**
                 Construct an instance of the guard giving it a lambda. Note that the init lambda
                 will execute immediately (in the constructor) while the cleanup lambda will be
                 executed in the destructor.
                 */
                RAII(const lambda& initCode, const lambda& cleanupCode) : _cleanupCode(cleanupCode) {
                    initCode();
                }

                ~RAII() {
                    _cleanupCode();
                }

                RAII& operator=(const RAII&) = delete;

            private:
                lambda _cleanupCode;
            };


            /**
             Provide a "finally" block. This is a shortcut to an RAII that contains no
             initialization code.
             */
            class finally : public RAII {
            public:
                explicit finally(const lambda& code) : RAII([]{}, code) {}
            };

        }
    }
}

#endif
