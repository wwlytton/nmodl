/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_kernkraft_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include "visitors/lookup_visitor.hpp"

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

void CodegenKernkraft::print_statement_block(ast::StatementBlock* node,
                                                  bool open_brace,
                                                  bool close_brace ) {
    if (open_brace) {
        printer->start_block();
    }

    auto statements = node->get_statements();
    for (const auto& statement: statements) {
        if (statement_to_skip(statement.get())) {
            continue;
        }
        /// not necessary to add indent for verbatim block (pretty-printing)
        if (!statement->is_verbatim()) {
            printer->add_indent();
        }

        auto functions = AstLookupVisitor(ast::AstNodeType::FUNCTION_CALL).lookup(statement.get());
        for(const auto& f : functions) {
            if(f->get_node_name() == "exp" ){
                printer->add_line("/*[KR INFO] elision of {} calls to exp below*/"_format(functions.size()));
            }
        }

        statement->accept(this);

        if (need_semicolon(statement.get())) {
            printer->add_text(";");
        }
        printer->add_newline();
    }

    if (close_brace) {
        printer->end_block();
    }
}

void CodegenKernkraft::print_function_call(ast::FunctionCall* node) {
    auto name = node->get_node_name();
    auto function_name = name;

    if (name == "exp") {
        auto arguments = node->get_arguments();
    //    printer->add_text("{}("_format(function_name));

        if (defined_method(name)) {
            printer->add_text(internal_method_arguments());
            if (!arguments.empty()) {
                printer->add_text(", ");
            }
        }

        print_vector_elements(arguments, ", ");
        //printer->add_text(")");
    } else {
        CodegenCVisitor::print_function_call(node);
    }
}

std::string CodegenKernkraft::get_variable_name(const std::string& name, bool use_instance) {
    std::string varname = update_if_ion_variable_name(name);

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return naming::NTHREAD_DT_VARIABLE;
    } else {
        return CodegenCVisitor::get_variable_name(name, use_instance);
    }
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
    printer->add_line( "double voltage[N];");
 //   print_nrn_cur();
    print_nrn_state();
    codegen = false;
}

void CodegenKernkraft::print_mechanism_range_var_structure() {
    auto float_type = default_float_data_type();
    auto int_type = default_int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables */");

    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto type = get_range_var_float_type(var);
        auto qualifier = is_constant_variable(name) ? k_const() : "";
        printer->add_line("{} {}[N];"_format(type, name));
    }
    // N.B. Kerncraft does not support integer arrays

    printer->add_newline();
}

void CodegenKernkraft::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for cpu backend, just get loop bounds
    printer->add_line("double v;");
    printer->add_line("double dt;");
}

void CodegenKernkraft::print_channel_iteration_block_begin(BlockType type) {
    print_channel_iteration_block_parallel_hint(type);
    printer->start_block("for (int id = 0; id < N; id++) ");
}

void CodegenKernkraft::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }
    codegen = true;

    printer->add_newline(2);
    printer->add_line("/** update state */");

    print_channel_iteration_tiling_block_begin(BlockType::State);
    print_channel_iteration_block_begin(BlockType::State);
    print_post_channel_iteration_common_code();ptr_type_qualifier(),

    printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
    printer->add_line("/*[KR INFO] node_id = node_index[id]; */");
    printer->add_line("/*[KR INFO] v = voltage[node_id]; */");
    printer->add_line("v = voltage[id];");

    if (ion_variable_struct_required()) {
        print_ion_variable();
    }


    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }


    if (info.nrn_state_block) {
        info.nrn_state_block->accept(this);
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

    codegen = false;
}

}  // namespace codegen
}  // namespace nmodl

