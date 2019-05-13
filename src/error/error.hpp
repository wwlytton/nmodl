/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iostream>
#include <map>

#include "error/error_decl.hpp"

namespace nmodl {
namespace error {

const std::map<ErrorCases, std::string> {
    (IncompatibleSolver, "\"{}\" solving method used at [{}] not handled. Supported methods are cnexp, euler, derivimplicit and sparse"),
};

class Error{
    ErrorCases error_case;
    ast::Ast* node;
    std::string name;
    ModToken token;
};

// if user wants to define new error:
// 1. add case to ErrorCases
// 2. add string to std::map<ErrorCases, std::string>
// 3. create visitor to handle the error or add case to switch of lookupvisitor

class ErrorVisitor {
    std::vector<Error> errors;
    visit_solve_blocks();   // create Error and add it to the error vector
    visit_discrete_block();
    .
    .
    .
    visit_global_var();
    visit_pointer_var();
    visit_bbcore_pointer_var();
    // instead of different visitors, I can use the same lookup that takes unhandled_ast_types from error_decl.hpp
    print_errors();     // based on the error_case print the message and the related members of Error
    print_errors(std::string prefix);   // print_errors with some prefix in the beggining
public:
    bool visit_program();    // run all visitors (check if there is any way to simply run all the defined private visitors)
                             // also prints error in the end
                             // returns true or false based on error.empty()
    bool visit_program(std::string);
    bool visit_program(stream);
};

}
}
