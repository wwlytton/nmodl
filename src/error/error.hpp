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
    virtual void print_errors() = 0;
};
// if user wants to define new error:
// 1. add case to ErrorCases
// 2. add string to std::map<ErrorCases, std::string>
// 3. create visitor to handle the error or add case to switch of lookupvisitor

class UnhandledAstNodes
    : public ErrorHandler {  // should be derived by ErrorVisitor and called Unhandled visitor

    const std::string CODE_INCOMPATIBILITY = "Code Incompatibility :: ";

    std::vector<Error> errors;
    // visit_solve_blocks();   // create Error and add it to the error vector
    void unhandled_solve_method(const std::shared_ptr<ast::Ast>& ast_node);

    template <typename T>
    void unhandled_construct_with_name(const std::shared_ptr<ast::Ast>& ast_node);

    template <typename T>
    void unhandled_construct_without_name(const std::shared_ptr<ast::Ast>& ast_node);

    void global_var_to_range(Ast* node, const std::shared_ptr<ast::Ast>& ast_node);

    void pointer_to_bbcorepointer(const std::shared_ptr<ast::Ast>& ast_node);

    void no_bbcore_readwrite(Ast* node);

    // visit_discrete_block();
    //.
    //.
    //.
    // visit_global_var();
    // visit_pointer_var();
    // visit_bbcore_pointer_var();
    // instead of different visitors, I can use the same lookup that takes unhandled_ast_types from
    // error_decl.hpp

  public:
    bool error_checking(ast::Ast* ast);  // run all visitors (check if there is any way to simply
                                         // run all the defined private visitors) also prints error
                                         // in the end returns true or false based on error.empty()
    // bool visit_unhandled_nodes(ast::Ast* ast, std::string);
    // bool visit_unhandled_nodes(ast::Ast* ast, stream);
    void print_errors() override;  // based on the error_case print the message and the related
                                   // members of Error
    // print_errors(std::string prefix);   // print_errors with some prefix in the beggining
};

}  // namespace error
}  // namespace nmodl
