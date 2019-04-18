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
    printer->add_line( "double node_area[N];");
    printer->add_line( "double shadow_rhs[N];");
    printer->add_line( "double shadow_d[N];");
    print_nrn_cur();
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
        printer->add_line("{} {}[N];"_format(type, name));
    }
//    if (channel_task_dependency_enabled()) {
        for (auto& var: codegen_shadow_variables) {
            auto name = var->get_name();
            printer->add_line("{} {}[N];"_format(float_type, name));
        }
//    }
    // N.B. Kerncraft does not support integer arrays

    printer->add_newline();
}

void CodegenKernkraft::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for cpu backend, just get loop bounds
    printer->add_line("double v;");
    printer->add_line("double dt;");
    printer->add_line("double lg;");
    printer->add_line("double rhs;");
    printer->add_line("double mfactor;");
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
    print_post_channel_iteration_common_code();

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

void CodegenKernkraft::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    codegen = true;
    if (info.conductances.empty()) {
        print_nrn_current(info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    //print_global_function_common_code(BlockType::Equation);
    print_channel_iteration_tiling_block_begin(BlockType::Equation);
    print_channel_iteration_block_begin(BlockType::Equation);
    print_post_channel_iteration_common_code();

    print_nrn_cur_kernel(info.breakpoint_node);

    print_nrn_cur_matrix_shadow_update();
    print_channel_iteration_block_end();

    /* MUST BE HANDLED AS A SEPARATE KERNEL
    if (nrn_cur_reduction_loop_required()) {
        print_shadow_reduction_block_begin();
        print_nrn_cur_matrix_shadow_reduction();
        print_shadow_reduction_statements();
        print_shadow_reduction_block_end();
    }
     */

    print_channel_iteration_tiling_block_end();
    print_kernel_data_present_annotation_block_end();
    //printer->end_block(1);
    codegen = false;
}

void CodegenKernkraft::print_nrn_cur_kernel(ast::BreakpointBlock* node) {

    printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
    printer->add_line("/*[KR INFO] node_id = node_index[id]; */");
    printer->add_line("/*[KR INFO] v = voltage[node_id]; */");
    printer->add_line("v = voltage[id];");

    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    auto read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.conductances.empty()) {
        //print_nrn_cur_non_conductance_kernel(); //not supported by kernkraft
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    auto write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
        printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
        printer->add_text("/*[KR INFO] ");
        printer->add_text("mfactor = 1.e2/{};"_format(area));
        printer->add_text("*/");
        printer->add_line("");
        printer->add_line("mfactor = 1.e2/node_area[id];");
        printer->add_line("lg = lg*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }
}

void CodegenKernkraft::print_nrn_cur_conductance_kernel(ast::BreakpointBlock* node) {

    auto block = node->get_statement_block();
    print_statement_block(block.get(), false, false);

    if (!info.currents.empty()) {
        std::string sum;
        for (const auto& current: info.currents) {
            auto var = breakpoint_current(current);
            sum += get_variable_name(var);
            if (&current != &info.currents.back()) {
                sum += "+";
            }
        }
        printer->add_line("rhs = {};"_format(sum));
    }

    if (!info.conductances.empty()) {
        std::string sum;
        for (const auto& conductance: info.conductances) {
            auto var = breakpoint_current(conductance.variable);
            sum += get_variable_name(var);
            if (&conductance != &info.conductances.back()) {
                sum += "+";
            }
        }
        printer->add_line("lg = {};"_format(sum));
    }

    for (const auto& conductance: info.conductances) {
        if (!conductance.ion.empty()) {
            auto lhs = "ion_di" + conductance.ion + "dv";
            auto rhs = get_variable_name(conductance.variable);
            auto statement = ShadowUseStatement{lhs, "+=", rhs};
            auto text = process_shadow_update_statement(statement, BlockType::Equation);
            printer->add_line(text);
        }
    }
}

void CodegenKernkraft::print_nrn_cur_matrix_shadow_update() {
    if (channel_task_dependency_enabled()) {
        auto rhs = get_variable_name("ml_rhs");
        auto d = get_variable_name("ml_d");
        printer->add_line("{} = rhs;"_format(rhs));
        printer->add_line("{} = lg;"_format(d));
    } else {
        if (info.point_process) {
            printer->add_line("shadow_rhs[id] = rhs;");
            printer->add_line("shadow_d[id] = lg;");
        } else {
            auto rhs_op = operator_for_rhs();
            auto d_op = operator_for_d();
            print_atomic_reduction_pragma();
            printer->add_line("vec_rhs[node_id] {} rhs;"_format(rhs_op));
            print_atomic_reduction_pragma();
            printer->add_line("vec_d[node_id] {} lg;"_format(d_op));
        }
    }
}

}  // namespace codegen
}  // namespace nmodl

