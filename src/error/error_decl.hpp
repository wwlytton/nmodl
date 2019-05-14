/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <set>

#include "src/codegen/codegen_naming.hpp"

namespace nmodl {
namespace error {

enum ErrorCases {
    IncompatibleSolver, IncompatibleBlockName, IncompatibleBlock, GlobalVar, PointerVar, Bbcore_read, Bbcore_write
};

/// Array of all the ast::AstNodeType that are unhandled
/// by the NMODL \c C++ code generator
std::vector <ast::AstNodeType> unhandled_ast_types = {ast::AstNodeType::SOLVE_BLOCK,
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

/// Set of handled solvers by the NMODL \c C++ code generator
const std::set <std::string> handled_solvers{codegen::naming::CNEXP_METHOD,
                                             codegen::naming::EULER_METHOD,
                                             codegen::naming::DERIVIMPLICIT_METHOD,
                                             codegen::naming::SPARSE_METHOD};

}
}
