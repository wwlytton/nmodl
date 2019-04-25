/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/expression_converter_visitor.hpp"

namespace nmodl {
    using namespace ast;

void ExpressionConverter::visit_wrapped_expression(WrappedExpression* node) {
    node->visit_children(this);
    auto e = node->get_expression();
    if (e->is_binary_expression()) {
        auto expression = static_cast<BinaryExpression*>(e.get());
        if (replaced_pow_expr.find(expression) != replaced_pow_expr.end()) {
            auto var = replaced_pow_expr[expression];
            node->set_expression(std::make_shared<FunctionCall>(*var));
        }
    }
}

void ExpressionConverter::visit_binary_expression(BinaryExpression *node){
    node->visit_children(this);

    auto op = node->get_op().eval();

    if (op == "^") {

        auto lhs = node->get_lhs();
        auto rhs = node->get_rhs();

        /// Create new assignment statements and insert them into a map that tracks
        /// the statements to be prepended to the expression containing pow

        std::vector<std::shared_ptr<ast::Expression>> pow_args ;
        pow_args.push_back(lhs);
        pow_args.push_back(rhs);

        auto pow_name = new String("pow");
        auto pow_id = new Name(pow_name);
        auto fun = new FunctionCall(pow_id->clone(), pow_args);

        replaced_pow_expr[node] = fun->clone();

    } else {
        AstVisitor::visit_binary_expression(node);
    }
}

} // namespace nmodl
