/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "error/error.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/lookup_visitor.hpp"

using namespace fmt::literals;

namespace nmodl {
namespace error {

using visitor::AstLookupVisitor;

std::map<ErrorCases, std::string> error_messages = {
    {IncompatibleSolver,
     "\"{}\" solving method used at [{}] not handled. Supported methods are cnexp, euler, "
     "derivimplicit and sparse"},
    {IncompatibleBlockName, "\"{}\" {}construct found at [{}] is not handled"},
    {IncompatibleBlock, "{}construct found at [{}] is not handled"},
    {GlobalVar,
     "\"{}\" variable found at [{}] should be defined as a RANGE variable instead of GLOBAL to "
     "enable backend transformations"},
    {PointerVar,
     "\"{}\" POINTER found at [{}] should be defined as BBCOREPOINTER to use it in CoreNeuron"},
    {Bbcore_read, "\"bbcore_read\" function not defined in any VERBATIM block"},
    {Bbcore_write, "\"bbcore_write\" function not defined in any VERBATIM block"},
};

std::vector<ast::AstNodeType> unhandled_ast_types = {ast::AstNodeType::SOLVE_BLOCK,
                                                     ast::AstNodeType::TERMINAL_BLOCK,
                                                     ast::AstNodeType::PARTIAL_BLOCK,
                                                     ast::AstNodeType::MATCH_BLOCK,
                                                     ast::AstNodeType::BEFORE_BLOCK,
                                                     ast::AstNodeType::AFTER_BLOCK,
                                                     ast::AstNodeType::CONSTANT_BLOCK,
                                                     ast::AstNodeType::CONSTRUCTOR_BLOCK,
                                                     ast::AstNodeType::DESTRUCTOR_BLOCK,
                                                     ast::AstNodeType::DISCRETE_BLOCK,
                                                     ast::AstNodeType::FUNCTION_TABLE_BLOCK,
                                                     ast::AstNodeType::INDEPENDENT_BLOCK,
                                                     ast::AstNodeType::GLOBAL_VAR,
                                                     ast::AstNodeType::POINTER_VAR,
                                                     ast::AstNodeType::BBCORE_POINTER_VAR};


void UnhandledAstNodes::unhandled_solve_method(const std::shared_ptr<ast::Ast>& ast_node) {
    auto solve_block_ast_node = std::dynamic_pointer_cast<ast::SolveBlock>(ast_node);
    auto method = solve_block_ast_node->get_method();
    if (method == nullptr) {
        return;
    }
    auto unhandled_solver_method = handled_solvers.find(method->get_node_name()) ==
                                   handled_solvers.end();
    if (unhandled_solver_method) {
        auto unhandled_solve_method_error =
            Error(IncompatibleSolver, ast_node, method->get_node_name(), *method->get_token());
        errors.push_back(unhandled_solve_method_error);
    }
}

template <typename T>
void UnhandledAstNodes::unhandled_construct_with_name(const std::shared_ptr<ast::Ast>& ast_node) {
    auto real_type_block = std::dynamic_pointer_cast<T>(ast_node);
    auto unhandled_construct_with_name_error = Error(IncompatibleBlockName,
                                                     ast_node,
                                                     real_type_block->get_node_name(),
                                                     *real_type_block->get_token());
    errors.push_back(unhandled_construct_with_name_error);
}

template <typename T>
void UnhandledAstNodes::unhandled_construct_without_name(
    const std::shared_ptr<ast::Ast>& ast_node) {
    auto real_type_block = std::dynamic_pointer_cast<T>(ast_node);
    auto unhandled_construct_without_name_error =
        Error(IncompatibleBlock, ast_node, *real_type_block->get_token());
    errors.push_back(unhandled_construct_without_name_error);
}

void UnhandledAstNodes::global_var_to_range(Ast* node, const std::shared_ptr<ast::Ast>& ast_node) {
    auto global_var = std::dynamic_pointer_cast<ast::GlobalVar>(ast_node);
    if (node->get_symbol_table()->lookup(global_var->get_node_name())->get_write_count() > 0) {
        auto global_var_to_range_error =
            Error(GlobalVar, ast_node, global_var->get_node_name(), *global_var->get_token());
        errors.push_back(global_var_to_range_error);
    }
}

void UnhandledAstNodes::pointer_to_bbcorepointer(const std::shared_ptr<ast::Ast>& ast_node) {
    auto pointer_var = std::dynamic_pointer_cast<ast::PointerVar>(ast_node);
    auto pointer_to_bbcorepointer_error =
        Error(PointerVar, ast_node, pointer_var->get_node_name(), *pointer_var->get_token());
    errors.push_back(pointer_to_bbcorepointer_error);
}

void UnhandledAstNodes::no_bbcore_readwrite(Ast* node) {
    auto verbatim_nodes = AstLookupVisitor().lookup(node, AstNodeType::VERBATIM);
    auto found_bbcore_read = false;
    auto found_bbcore_write = false;
    for (const auto& it: verbatim_nodes) {
        auto verbatim = std::dynamic_pointer_cast<ast::Verbatim>(it);

        auto verbatim_statement = verbatim->get_statement();
        auto verbatim_statement_string = verbatim_statement->get_value();

        // Parse verbatim block to find out if there is a token "bbcore_read" or
        // "bbcore_write". If there is not, then the function is not defined or
        // is commented.
        parser::CDriver driver;

        driver.scan_string(verbatim_statement_string);
        auto tokens = driver.all_tokens();

        for (const auto& token: tokens) {
            if (token == "bbcore_read") {
                found_bbcore_read = true;
            }
            if (token == "bbcore_write") {
                found_bbcore_write = true;
            }
        }
    }
    if (!found_bbcore_read) {
        auto bbcore_read_error = Error(Bbcore_read);
        errors.push_back(bbcore_read_error);
    }
    if (!found_bbcore_write) {
        auto bbcore_write_error = Error(Bbcore_write);
        errors.push_back(bbcore_write_error);
    }
}


bool UnhandledAstNodes::error_checking(ast::Ast* node) {
    std::vector<std::shared_ptr<ast::Ast>> unhandled_ast_nodes =
        AstLookupVisitor().lookup(node, unhandled_ast_types);
    for (const auto& ast_node: unhandled_ast_nodes) {
        auto node_type = ast_node->get_node_type();
        switch (node_type) {
        case AstNodeType::SOLVE_BLOCK:
            unhandled_solve_method(ast_node);
            break;
        case AstNodeType::DISCRETE_BLOCK:
            unhandled_construct_with_name<DiscreteBlock>(ast_node);
            break;
        case AstNodeType::PARTIAL_BLOCK:
            unhandled_construct_with_name<PartialBlock>(ast_node);
            break;
        case AstNodeType::BEFORE_BLOCK:
            unhandled_construct_without_name<BeforeBlock>(ast_node);
            break;
        case AstNodeType::AFTER_BLOCK:
            unhandled_construct_without_name<AfterBlock>(ast_node);
            break;
        case AstNodeType::MATCH_BLOCK:
            unhandled_construct_without_name<MatchBlock>(ast_node);
            break;
        case AstNodeType::CONSTANT_BLOCK:
            unhandled_construct_without_name<ConstantBlock>(ast_node);
            break;
        case AstNodeType::CONSTRUCTOR_BLOCK:
            unhandled_construct_without_name<ConstructorBlock>(ast_node);
            break;
        case AstNodeType::DESTRUCTOR_BLOCK:
            unhandled_construct_without_name<DestructorBlock>(ast_node);
            break;
        case AstNodeType::INDEPENDENT_BLOCK:
            unhandled_construct_without_name<IndependentBlock>(ast_node);
            break;
        case AstNodeType::FUNCTION_TABLE_BLOCK:
            unhandled_construct_without_name<FunctionTableBlock>(ast_node);
            break;
        case AstNodeType::GLOBAL_VAR:
            global_var_to_range(node, ast_node);
            break;
        case AstNodeType::POINTER_VAR:
            pointer_to_bbcorepointer(ast_node);
            break;
        case AstNodeType::BBCORE_POINTER_VAR:
            no_bbcore_readwrite(node);
            break;
        }
    }
    return !errors.empty();
}

void UnhandledAstNodes::print_errors() {
    for (const auto& error: errors) {
        switch (error.error_case) {
        case IncompatibleSolver: {
            std::string error_message = error_messages[error.error_case];
            fmt::format_args args = fmt::make_format_args(error.name, error.token.position());
            error_message = vformat(error_message, args);
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case IncompatibleBlockName: {
            std::string error_message = error_messages[error.error_case];
            fmt::format_args args = fmt::make_format_args(error.name,
                                                          error.node->get_nmodl_name(),
                                                          error.token.position());
            error_message = vformat(error_message, args);
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case IncompatibleBlock: {
            std::string error_message = error_messages[error.error_case];
            fmt::format_args args = fmt::make_format_args(error.node->get_nmodl_name(),
                                                          error.token.position());
            error_message = vformat(error_message, args);
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case GlobalVar: {
            std::string error_message = error_messages[error.error_case];
            fmt::format_args args = fmt::make_format_args(error.name, error.token.position());
            error_message = vformat(error_message, args);
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case PointerVar: {
            std::string error_message = error_messages[error.error_case];
            fmt::format_args args = fmt::make_format_args(error.name, error.token.position());
            error_message = vformat(error_message, args);
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case Bbcore_read: {
            std::string error_message = error_messages[error.error_case];
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        case Bbcore_write: {
            std::string error_message = error_messages[error.error_case];
            logger->error(CODE_INCOMPATIBILITY + error_message);
        } break;
        }
    }
}

}  // namespace error
}  // namespace nmodl
