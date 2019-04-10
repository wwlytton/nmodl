/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "codegen/codegen_c_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * \class CodegenKernkraft
 * \brief Visitor for printing c code with OpenMP backend
 */
class CodegenKernkraft: public CodegenCVisitor {
  protected:
    /// name of the code generation backend
    std::string backend_name() override;


    std::string float_variable_name(SymbolType& symbol, bool use_instance) override;

    void print_global_function_common_code(BlockType type) override;

    /// entry point to code generation
    void print_codegen_routines() override;

    /// structure that wraps all range and int variables required for mod file
    void print_mechanism_range_var_structure() override;

    /// nrn_state / state update function definition
    void print_nrn_state() override;

    /// backend specific block start for tiling on channel iteration
    virtual void print_channel_iteration_tiling_block_begin(BlockType type) override;

    /// backend specific channel instance iteration block start
    virtual void print_channel_iteration_block_begin(BlockType type) override;



  public:
    CodegenKernkraft(std::string mod_file,
                     std::string output_dir,
                     LayoutType layout,
                     std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, layout, float_type, ".kr.cpp") {}

    CodegenKernkraft(std::string mod_file,
                     std::stringstream& stream,
                     LayoutType layout,
                     std::string float_type)
        : CodegenCVisitor(mod_file, stream, layout, float_type) {}
};

}  // namespace codegen
}  // namespace nmodl
