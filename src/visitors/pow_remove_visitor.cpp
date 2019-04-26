/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cmake-build-debug/src/visitors/json_visitor.hpp>
#include "visitors/pow_remove_visitor.hpp"

namespace nmodl {
    using namespace ast;
    using namespace visitor;


void PowRemoveVisitor::visit_statement_block(StatementBlock *node) {

    caller_block = node;

    add_local_statement(node);

    auto& statements = node->statements;

    for(auto& statement : statements ) {
        caller_statement = statement;
        statement->accept(this);
    }

    /// if nothing was added into local statement, remove it
    LocalVarVector* local_variables = get_local_variables(node);
    if (local_variables->empty()) {
        statements.erase(statements.begin());
    }

    /// all statements from inlining needs to be added before the caller statement
    for (auto& element: prepend_statements) {
        auto it = std::find(statements.begin(), statements.end(), element.first);
        if (it != statements.end()) {
            statements.insert(it, element.second.begin(), element.second.end());
            element.second.clear();
        }
    }
}

void PowRemoveVisitor::visit_wrapped_expression(WrappedExpression* node) {
    node->visit_children(this);
    auto e = node->get_expression();
    if (e->is_binary_expression()) {
        auto expression = static_cast<BinaryExpression*>(e.get());
        if (replaced_pow_expr.find(expression) != replaced_pow_expr.end()) {
            auto var = replaced_pow_expr[expression];
            node->set_expression(std::make_shared<Name>(new String(var)));
        }
    }
    if (e->is_function_call()) {
        auto expression = static_cast<FunctionCall *>(e.get());
        auto name = expression->get_node_name();
        if (name == "pow") {
            auto expression = static_cast<FunctionCall*>(e.get());
            if (replaced_pow_expr.find(expression) != replaced_pow_expr.end()) {
                auto var = replaced_pow_expr[expression];
                node->set_expression(std::make_shared<Name>(new String(var)));
            }
        }
    }
}

void PowRemoveVisitor::visit_binary_expression(BinaryExpression *node){
    node->visit_children(this);

    auto op = node->get_op().eval();

    if (op == "^") {

        std::cout << "Visiting ^ operator" << std::endl;

        auto lhs = node->get_lhs();
        auto rhs = node->get_rhs();

        /// Create new assignment statements and insert them into a map that tracks
        /// the statements to be prepended to the expression containing pow

        std::vector<std::shared_ptr<ast::ExpressionStatement>> local_prepend_statements;

        /// For lhs of pow
        auto namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        auto newvarid = new Name(namestr->clone());
        auto newvar_lhs = new VarName(newvarid->clone(), nullptr, nullptr);
        auto newvar_assignment_expr = lhs->clone();
        /// for argument add new variable to local statement
        add_local_variable(caller_block, newvarid);

        auto expression = new BinaryExpression(newvar_lhs, BinaryOperator(ast::BOP_ASSIGN), newvar_assignment_expr);
        auto statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// For rhs of pow
        namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        newvarid = new Name(namestr->clone());
        auto newvar_rhs = new VarName(newvarid->clone(), nullptr, nullptr);
        newvar_assignment_expr = rhs->clone();
        add_local_variable(caller_block, newvarid);

        expression = new BinaryExpression(newvar_rhs, BinaryOperator(ast::BOP_ASSIGN), newvar_assignment_expr);
        statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// For result of pow
        namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        newvarid = new Name(namestr->clone());
        auto newvar = new VarName(newvarid->clone(), nullptr, nullptr);
        newvar_assignment_expr = new BinaryExpression(newvar_lhs->clone(), BinaryOperator(ast::BOP_POWER), newvar_rhs->clone());
        add_local_variable(caller_block, newvarid);

        expression = new BinaryExpression(newvar, BinaryOperator(ast::BOP_ASSIGN), newvar_assignment_expr);
        statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// insert local_prepend_statements into prepend_statements for later use
        /// Below check is necessary to handle the case where multiple power
        /// function calls were in the same statement.
        if (prepend_statements.find(caller_statement) == prepend_statements.end()) {
            // this is the first time
            prepend_statements[caller_statement] = local_prepend_statements;
        } else {
            // there was already something, we must now overwrite it!
            prepend_statements[caller_statement].insert(prepend_statements[caller_statement].end(),
                                                        local_prepend_statements.begin(),
                                                        local_prepend_statements.end());
        }
        replaced_pow_expr[node] = newvarid->get_node_name();


    } else {
        AstVisitor::visit_binary_expression(node);
    }
}

void PowRemoveVisitor::visit_function_call(FunctionCall *node){
    node->visit_children(this);

    auto name = node->get_node_name();

    if (name == "pow") {

        auto pow_arguments = node->get_arguments();
        auto lhs = pow_arguments[0];
        auto rhs = pow_arguments[1];

        auto file2 = "./tmp/pow_instance_" + std::to_string(newvar_count++) + ".ast.json";
        JSONVisitor v(file2);
        node->accept(&v);

        /// Create new assignment statements and insert them into a map that tracks
        /// the statements to be prepended to the expression containing pow

        std::vector<std::shared_ptr<ast::ExpressionStatement>> local_prepend_statements;

        /// For lhs of pow
        auto namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        auto newvarid = new Name(namestr->clone());
        auto newvar_lhs = new VarName(newvarid->clone(), nullptr, nullptr);
        auto newvar_assignment_expr = lhs->clone();
        /// for argument add new variable to local statement
        add_local_variable(caller_block, newvarid);

        auto expression = new BinaryExpression(newvar_lhs, BinaryOperator(ast::BOP_ASSIGN), newvar_assignment_expr);
        auto statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// For rhs of pow
        namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        newvarid = new Name(namestr->clone());
        auto newvar_rhs = new VarName(newvarid->clone(), nullptr, nullptr);
        newvar_assignment_expr = rhs->clone();
        add_local_variable(caller_block, newvarid);

        expression = new BinaryExpression(newvar_rhs, BinaryOperator(ast::BOP_ASSIGN), newvar_assignment_expr);
        statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// For result of pow
        namestr = new String("_repl_pow_expr_" + std::to_string(newvar_count++));
        newvarid = new Name(namestr->clone());
        auto newvar = new VarName(newvarid->clone(), nullptr, nullptr);

        std::vector<std::shared_ptr<ast::Expression>> new_args;
        new_args.push_back(std::make_shared<ast::VarName>( *newvar_lhs ));
        new_args.push_back(std::make_shared<ast::VarName>( *newvar_rhs ));

        auto pow_name = new String("pow");
        auto pow_id = new Name(pow_name);
        auto fun = new FunctionCall(pow_id->clone(), new_args);

        add_local_variable(caller_block, newvarid);

        expression = new BinaryExpression(newvar, BinaryOperator(ast::BOP_ASSIGN), fun);
        statement = std::make_shared<ExpressionStatement>(expression);

        local_prepend_statements.push_back(statement);

        /// insert local_prepend_statements into prepend_statements for later use
        if (prepend_statements.find(caller_statement) == prepend_statements.end()) {
            // this is the first time
            prepend_statements[caller_statement] = local_prepend_statements;
        } else {
            // there was already something, we must now overwrite it!
            prepend_statements[caller_statement].insert(prepend_statements[caller_statement].end(),
                                                        local_prepend_statements.begin(),
                                                        local_prepend_statements.end());
        }
        replaced_pow_expr[node] = newvarid->get_node_name();

    } else {
        AstVisitor::visit_function_call(node);
    }
}

} // namespace nmodl
