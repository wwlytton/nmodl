/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_kernkraft_visitor.hpp"


using namespace fmt::literals;

namespace nmodl {
namespace codegen {


/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/


std::string CodegenKernkraft::backend_name() {
    return "C-Kernkraft (api-compatibility)";
}


std::string CodegenKernkraft::float_variable_name(SymbolType& symbol, bool use_instance) {
    auto name = symbol->get_name();
    return "{}[{}]"_format(name, "id");
}

void CodegenKernkraft::print_global_function_common_code(BlockType type) {
    // do nothing
    std::string method = compute_method_name(type);
    auto args = "NrnThread* nt, Memb_list* ml, int type";
    printer->start_block("void {}({})"_format(method, args));
}

void CodegenKernkraft::print_codegen_routines() {
    codegen = true;
    //print_data_structures();
    print_mechanism_range_var_structure();
    printer->add_line( "double voltage[N]"); //TODO: find a better way
    print_nrn_cur();
    print_nrn_state();
    codegen = false;
}

void CodegenKernkraft::print_mechanism_range_var_structure() {
    auto float_type = default_float_data_type();
    auto int_type = default_int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables */");
//    printer->start_block("struct {} "_format(instance_struct()));
    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto type = get_range_var_float_type(var);
        auto qualifier = is_constant_variable(name) ? k_const() : "";
        printer->add_line("{}{} {}{}[N];"_format(qualifier, type, ptr_type_qualifier(), name));
    }
    // Kerncraft does not support integer arrays
    /*
    for (auto& var: codegen_int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            auto qualifier = var.is_constant ? k_const() : "";
            printer->add_line(
                "{}{}* {}{};"_format(qualifier, int_type, ptr_type_qualifier(), name));
        } else {
            auto qualifier = var.is_constant ? k_const() : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->add_line("{}{}* {}{};"_format(qualifier, type, ptr_type_qualifier(), name));
        }
    }
    if (channel_task_dependency_enabled()) {
        for (auto& var: codegen_shadow_variables) {
            auto name = var->get_name();
            printer->add_line("{}* {}{};"_format(float_type, ptr_type_qualifier(), name));
        }
    }
    */
  //  printer->end_block();
  //  printer->add_text(";");
    printer->add_newline();
}

void CodegenKernkraft::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for cpu backend, just get loop bounds
    printer->add_line("int start = 0;");
    printer->add_line("int end = nodecount;");
    printer->add_line("int id");
    printer->add_line("double v");
}

void CodegenKernkraft::print_channel_iteration_block_begin(BlockType type) {
    print_channel_iteration_block_parallel_hint(type);
    printer->start_block("for (id = start; id < end; id++) ");
}

void CodegenKernkraft::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }
    codegen = true;

    printer->add_newline(2);
    printer->add_line("/** update state */");
    //print_global_function_common_code(BlockType::State);
    print_channel_iteration_tiling_block_begin(BlockType::State);
    print_channel_iteration_block_begin(BlockType::State);
    //print_post_channel_iteration_common_code();

    printer->add_line("/*[INFO] elision of 1 indirect access: */");
    printer->add_line("/*[INFO] node_id = node_index[id]; */");
    printer->add_line("/*[INFO] v = voltage[node_id]; */");
    printer->add_line("v = voltage[id];");
/*
    /// todo : eigen solver node also emits IonCurVar variable in the functor
    /// but that shouldn't update ions in derivative block
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(this);
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(block.get(), false, false);
    }

    auto write_statements = ion_write_statements(BlockType::State);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::State);
        printer->add_line(text);
    }
    print_channel_iteration_block_end();
    if (!shadow_statements.empty()) {
        print_shadow_reduction_block_begin();
        print_shadow_reduction_statements();
        print_shadow_reduction_block_end();
    }
    print_channel_iteration_tiling_block_end();

    print_kernel_data_present_annotation_block_end();
    printer->end_block(1);
*/
    codegen = false;
}

}  // namespace codegen
}  // namespace nmodl

