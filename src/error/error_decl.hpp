/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

namespace nmodl {
namespace error {

enum ErrorCases {
    IncompatibleSolver, IncompatibleBlock, IncompatibleBlockName, GlobalVar, PointerVar, Bbcore_readwrite
};

/// Array of all the ast::AstNodeType that are unhandled
/// by the NMODL \c C++ code generator
std::vector <ast::AstNodeType> unhandled_ast_types = {AstNodeType::SOLVE_BLOCK,
                                                      AstNodeType::TERMINAL_BLOCK,
                                                      AstNodeType::PARTIAL_BLOCK,
                                                      AstNodeType::MATCH_BLOCK,
                                                      AstNodeType::BEFORE_BLOCK,
                                                      AstNodeType::AFTER_BLOCK,
                                                      AstNodeType::CONSTANT_BLOCK,
                                                      AstNodeType::CONSTRUCTOR_BLOCK,
                                                      AstNodeType::DESTRUCTOR_BLOCK,
                                                      AstNodeType::DISCRETE_BLOCK,
                                                      AstNodeType::FUNCTION_TABLE_BLOCK,
                                                      AstNodeType::INDEPENDENT_BLOCK,
                                                      AstNodeType::GLOBAL_VAR,
                                                      AstNodeType::POINTER_VAR,
                                                      AstNodeType::BBCORE_POINTER_VAR};

/// Set of handled solvers by the NMODL \c C++ code generator
const std::set <std::string> handled_solvers{codegen::naming::CNEXP_METHOD,
                                             codegen::naming::EULER_METHOD,
                                             codegen::naming::DERIVIMPLICIT_METHOD,
                                             codegen::naming::SPARSE_METHOD};

}
}
