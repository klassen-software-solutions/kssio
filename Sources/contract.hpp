//
//  contract.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-17.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
//  "borrowed" from ksscontract
//

#ifndef ksscontract_contract_hpp
#define ksscontract_contract_hpp

#include <cassert>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>

namespace kss { namespace contract {

    namespace _private {
        struct Expression {
            bool        result = false;
            const char* expr = nullptr;
            const char* functionName = nullptr;
            const char* fileName = nullptr;
            unsigned    lineNo = 0;
        };

        inline const char* localBasename(const std::string& path) noexcept {
            const size_t pos = path.find_last_of('/');
            return (pos == std::string::npos ? path.c_str() : path.c_str() + pos + 1);
        }

        template <bool IsThrowing>
        void performCheck(const char* conditionType,
                          const kss::contract::_private::Expression& exp)
        {
            if (!exp.result) {
                assert(exp.expr != nullptr);
                assert(exp.functionName != nullptr);
                assert(exp.fileName != nullptr);

                if (IsThrowing) {
                    throw std::invalid_argument(std::string(conditionType)
                                                + " failed: '" + exp.expr
                                                + "' in " + exp.functionName
                                                + ", file " + localBasename(exp.fileName)
                                                + ", line " + std::to_string(exp.lineNo));
                }
                else {
                    std::cerr << conditionType << " failed: '" << exp.expr << "'" << std::endl;
                    std::cerr << "   in " << exp.functionName << std::endl;
                    std::cerr << "   file: " << exp.fileName << ", line: " << exp.lineNo << std::endl;
                    std::terminate();
                }
            }
        }
    }


    /*!
     This macro is used to create the Expression objects used as inputs to the
     condition checking methods of this file.
     */
#   define KSS_EXPR(expr) kss::contract::_private::Expression {(expr), #expr, __PRETTY_FUNCTION__, __FILE__, __LINE__}


    /*!
     The parameter check is a form of precondition that is presumed to be checking
     the incoming parameter arguments. One difference between this and the other
     conditions, is that instead of terminating execution, this throws an
     exception.

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             parameter(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws std::invalid_argument if the expression fails
     @throws any other exception that the expression may throw
     */
    inline void parameter(const kss::contract::_private::Expression& exp) {
        kss::contract::_private::performCheck<true>("Parameter", exp);
    }

    /*!
     This is a short-hand way of calling parameter multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             parameters({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue);
             });

     @throws std::invalid_argument (actually kss::contract::InvalidArgument) if one or
            more of the expressions fail
     @throws any other exception that the expressions may throw
     */
    inline void parameters(std::initializer_list<kss::contract::_private::Expression> exps) {
        for (const auto& exp : exps) {
            parameter(exp);
        }
    }


    /*!
     Check that a precondition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             precondition(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws any exception that the expression may throw
     */
    inline void precondition(const kss::contract::_private::Expression& exp) {
        kss::contract::_private::performCheck<false>("Precondition", exp);
    }

    /*!
     This is a short-hand way of calling precondition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             preconditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue);
             });

     @throws any exception that the expressions may throw
     */
    inline void preconditions(std::initializer_list<kss::contract::_private::Expression> exps) {
        for (const auto& exp : exps) {
            precondition(exp);
        }
    }


    /*!
     Check that a condition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             condition(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws any exception that the expression may throw
     */
    inline void condition(const kss::contract::_private::Expression& exp) {
        kss::contract::_private::performCheck<false>("Condition", exp);
    }

    /*!
     This is a short-hand way of calling condition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             conditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue);
             });

     @throws std::invalid_argument (actually kss::contract::InvalidArgument) if one or
            more of the expressions fail.
     @throws any exception that the expressions may throw
     */
    inline void conditions(std::initializer_list<kss::contract::_private::Expression> exps) {
        for (const auto& exp : exps) {
            precondition(exp);
        }
    }


    /*!
     Check that a postcondition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             postcondition(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws any exception that the expression may throw
     */
    inline void postcondition(const kss::contract::_private::Expression& exp) {
        kss::contract::_private::performCheck<false>("Postcondition", exp);
    }

    /*!
     This is a short-hand way of calling postcondition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             postconditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue);
             });

     @throws any exception that the expressions may throw
     */
    inline void postconditions(std::initializer_list<kss::contract::_private::Expression> exps) {
        for (const auto& exp : exps) {
            postcondition(exp);
        }
    }

}}

#endif
