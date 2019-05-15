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
    IncompatibleSolver,
    IncompatibleBlockName,
    IncompatibleBlock,
    GlobalVar,
    PointerVar,
    Bbcore_read,
    Bbcore_write
};

extern std::map<ErrorCases, std::string> error_messages;

/// Array of all the ast::AstNodeType that are unhandled
/// by the NMODL \c C++ code generator
extern std::vector<ast::AstNodeType> unhandled_ast_types;

/// Set of handled solvers by the NMODL \c C++ code generator
const std::set<std::string> handled_solvers{codegen::naming::CNEXP_METHOD,
                                            codegen::naming::EULER_METHOD,
                                            codegen::naming::DERIVIMPLICIT_METHOD,
                                            codegen::naming::SPARSE_METHOD};

}  // namespace error
}  // namespace nmodl
