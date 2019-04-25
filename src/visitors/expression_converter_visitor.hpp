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

    class ExpressionConverter: public AstVisitor {
    private:
        /// map to track replaced pow calls
        std::map<ast::BinaryExpression*, ast::FunctionCall*> replaced_pow_expr;

    public:
        ExpressionConverter() = default;

        void visit_binary_expression(ast::BinaryExpression *node) override;

        void visit_wrapped_expression(ast::WrappedExpression* node) override;
    };
}
