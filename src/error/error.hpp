/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iostream>
#include <map>

#include "ast/ast.hpp"
#include "codegen/codegen_naming.hpp"
#include "error/error_decl.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace error {

using namespace ast;

class Error {
  public:
    ErrorCases error_case;
    std::shared_ptr<ast::Ast> node;
    std::string name;
    ModToken token;

    Error(ErrorCases error_case, std::shared_ptr<Ast> node, std::string name, ModToken token)
        : error_case(error_case)
        , node(node)
        , name(name)
        , token(token) {}

    Error(ErrorCases error_case, std::shared_ptr<Ast> node, ModToken token)
        : error_case(error_case)
        , node(node)
        , name("None")
        , token(token) {}

    explicit Error(ErrorCases error_case)
        : error_case(error_case) {}
};

class ErrorHandler {
    std::vector<Error> errors;

  public:
    void add_error(Error error) {
        errors.push_back(error);
    }
    std::vector<Error> get_errors() {
        return errors;
    }
    virtual bool error_checking() = 0;
    virtual void print_errors() = 0;
};
// if someone wants to define new error:
// 1. add case to ErrorCases
// 2. add string to std::map<ErrorCases, std::string>
// 3. create relevant class for this type of errors
// 4. add print_errors which is needed to print some kind of error
// 5. add error_checking that takes care of handling the error

// ErrorHandler can be added to every class and if there is an error in this class
// add it to the errors vector and then print the vector in the end
// or
// there can be defined a new error handler that takes care of traversing the AST
// and print errors if there are any

class UnhandledAstNodes: public ErrorHandler {
    const std::string CODE_INCOMPATIBILITY = "Code Incompatibility :: ";

    ast::Ast* ast_tree;

    // functions that handle errors of this type and add them to the errors vector
    void unhandled_solve_method(const std::shared_ptr<ast::Ast>& ast_node);

    template <typename T>
    void unhandled_construct_with_name(const std::shared_ptr<ast::Ast>& ast_node);

    template <typename T>
    void unhandled_construct_without_name(const std::shared_ptr<ast::Ast>& ast_node);

    void global_var_to_range(Ast* node, const std::shared_ptr<ast::Ast>& ast_node);

    void pointer_to_bbcorepointer(const std::shared_ptr<ast::Ast>& ast_node);

    void no_bbcore_readwrite(Ast* node);

  public:
    explicit UnhandledAstNodes(ast::Ast* node)
        : ast_tree(node) {}

    bool error_checking() override;  // basic function which runs all for checking some error in AST
                                     // in the end returns true or false based on error.empty()

    void print_errors() override;  // based on the error_case print the message and the related
                                   // members of Error
};

}  // namespace error
}  // namespace nmodl
