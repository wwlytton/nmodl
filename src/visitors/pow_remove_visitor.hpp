/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <stack>

#include "ast/ast.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include "visitors/visitor.hpp"

namespace nmodl {

    class PowRemoveVisitor: public AstVisitor {
    private:
        /// statement block containing current call to pow
        ast::StatementBlock* caller_block = nullptr;

        /// statement containing current call to pow
        std::shared_ptr<ast::Statement> caller_statement;

        /// map to track statements being prepended before pow call
        std::map<std::shared_ptr<ast::Statement>,
                std::vector<std::shared_ptr<ast::ExpressionStatement>>>
                prepend_statements;

        /// count the number of times a new variable was introduced
        int newvar_count = 0;

        /// map to track replaced pow calls
        std::map<ast::Expression*, std::string> replaced_pow_expr;

    public:
        PowRemoveVisitor() = default;

        void visit_binary_expression(ast::BinaryExpression *node) override;

        void visit_statement_block(ast::StatementBlock* node) override;

        void visit_wrapped_expression(ast::WrappedExpression* node) override;

        void visit_function_call(ast::FunctionCall *node) override;
    };
}
