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

bool CodegenKernkraft::optimize_ion_variable_copies() {
    return false;
}

void CodegenKernkraft::visit_binary_expression(ast::BinaryExpression* node) {
    if (!codegen) {
        return;
    }

    auto op = node->get_op().eval();
    auto lhs = node->get_lhs();
    auto rhs = node->get_rhs();
    if (op == "^") {
        printer->add_text("/*elided_pow*/");
        lhs->accept(this);
 //       printer->add_text(", ");
 //       rhs->accept(this);
 //       printer->add_text(")");
    } else {
        lhs->accept(this);
        printer->add_text(" " + op + " ");
        rhs->accept(this);
    }
}


std::string CodegenKernkraft::float_variable_name(SymbolType& symbol, bool use_instance) {
    auto name = symbol->get_name();
    return "{}[{}]"_format(name, "id");
}

std::string CodegenKernkraft::ion_shadow_variable_name(SymbolType& symbol) {
    return "{}[{}]"_format(symbol->get_name(), "id");
}

std::string CodegenKernkraft::int_variable_name(IndexVariableInfo& symbol,
                                               const std::string& name,
                                               bool use_instance) {
    auto position = position_of_int_var(name);
    auto num_int = int_variables_size();
    std::string offset;

    // clang-format off
    if (symbol.is_index) {
        offset = std::to_string(position);
        if (use_instance) {
            /*
            return "inst->{}[{}]"_format(name, offset);
             */
            std::cout << "Warning! Kernkraft does not support integer arrays such as " << name << std::endl;
            return "";
        }
        return "indexes[{}]"_format(offset);
    }
    if (symbol.is_integer) {
        if (use_instance) {
            /*
            offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id*{}+{}"_format(num_int, position);
            return "inst->{}[{}]"_format(name, offset);
             */
            std::cout << "Warning! Kernkraft does not support integer arrays such as " << name << std::endl;
            return "";
        }
        offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id";
        return "indexes[{}]"_format(offset);
    }
    offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "{}"_format(position);
    if (use_instance) {
        printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
        printer->add_text("/*[KR INFO] ");
        printer->add_text("inst->{}[indexes[{}]]"_format(name, offset));
        printer->add_line("*/");
        return "{}[id]"_format(name, offset);
    }
    auto data = symbol.is_vdata ? "_vdata" : "_data";
    return "nt->{}[indexes[{}]]"_format(data, offset);
    // clang-format on
}

std::string CodegenKernkraft::global_variable_name(SymbolType& symbol) {
    return "{}"_format(symbol->get_name());
}

bool CodegenKernkraft::statement_to_skip(ast::Statement* node) {
    return node->is_local_list_statement() || CodegenCVisitor::statement_to_skip(node);
}

void CodegenKernkraft::visit_if_statement(ast::IfStatement* node) {
    if (!codegen) {
        return;
    }
    //printer->add_text("if (");
    //node->get_condition()->accept(this);
    //printer->add_text(") ");
    //print_vector_elements(node->get_elseifs(), "");
    auto elses = node->get_elses();
    if (elses) {
        elses->accept(this);
    } else {
        node->get_statement_block()->accept(this);
    }
}


void CodegenKernkraft::visit_else_if_statement(ast::ElseIfStatement* node) {
    if (!codegen) {
        return;
    }
    //printer->add_text(" else if (");
    //node->get_condition()->accept(this);
    //printer->add_text(") ");
    //node->get_statement_block()->accept(this);
}


void CodegenKernkraft::visit_else_statement(ast::ElseStatement* node) {
    if (!codegen) {
        return;
    }
    //printer->add_text(" else ");
    node->visit_children(this);
}

void CodegenKernkraft::print_statement_block(ast::StatementBlock* node,
                                                  bool open_brace,
                                                  bool close_brace ) {
    if (open_brace) {
        //printer->start_block();
    }

    auto statements = node->get_statements();
    for (const auto& statement: statements) {

        //auto expressionss = AstLookupVisitor(ast::AstNodeType::EXPRESSION_STATEMENT).lookup(statement.get());
        //printer->add_line("/* >>> {} {}*/"_format(statement.get()->get_node_type_name(),expressionss.size()));

        if (statement_to_skip(statement.get())) {
            continue;
        }
        /// not necessary to add indent for verbatim block (pretty-printing)
        if (!statement->is_verbatim()) {
            printer->add_indent();
        }

        // check that this is a "terminal" expression, i.e. no children expressions
        // TODO: this should be improved, because it will silently elide calls to exp functions in statements that
        //  must have child expressions, such as e.g. an if statement.
        auto expressions = AstLookupVisitor(ast::AstNodeType::EXPRESSION_STATEMENT).lookup(statement.get());
        if (expressions.size() <= 1) {
            auto functions = AstLookupVisitor(ast::AstNodeType::FUNCTION_CALL).lookup(statement.get());
            for (const auto &f : functions) {
                if (f->get_node_name() == "exp") {
                    printer->add_line("/*[KR INFO] elision of {} calls to exp below*/"_format(functions.size()));
                }
            }
        }

        statement->accept(this);

        if (need_semicolon(statement.get())) {
            printer->add_text(";");
        }
        printer->add_newline();
    }

    if (close_brace) {
        //printer->end_block();
    }
}

void CodegenKernkraft::print_function_call(ast::FunctionCall* node) {
    auto name = node->get_node_name();
    auto function_name = name;

    if (name == "exp" || name == "fabs" || name == "pow") {
        auto arguments = node->get_arguments();
        printer->add_text("/*elided_{}*/"_format(function_name));

        arguments[0]->accept(this); //always just print first argument
        //print_vector_elements(arguments, ", ");
        //printer->add_text(")");
    } else {
        CodegenCVisitor::print_function_call(node);
    }
}

std::string CodegenKernkraft::get_variable_name(const std::string& name, bool use_instance) {
    std::string varname = update_if_ion_variable_name(name);

    // clang-format off
    auto symbol_comparator = [&varname](const SymbolType& sym) {
        return varname == sym->get_name();
    };

    auto index_comparator = [&varname](const IndexVariableInfo& var) {
        return varname == var.symbol->get_name();
    };

    /// shadow variable
    auto s = std::find_if(codegen_shadow_variables.begin(), codegen_shadow_variables.end(),
                          symbol_comparator);
    if (s != codegen_shadow_variables.end()) {
        return ion_shadow_variable_name(*s);
    }

    /// global variable
    auto g = std::find_if(codegen_global_variables.begin(), codegen_global_variables.end(),
                          symbol_comparator);
    if (g != codegen_global_variables.end()) {
        return global_variable_name(*g);
    }

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return naming::NTHREAD_DT_VARIABLE;
    } else {
        return CodegenCVisitor::get_variable_name(name, use_instance);
    }
}

void CodegenKernkraft::print_unique_local_variables(ast::StatementVector statements) {
    // declare a set to hold local variables
    struct compare_var_names {
        bool operator()( std::shared_ptr<ast::AST> x, std::shared_ptr<ast::AST> y){
            return x->get_node_name() < y->get_node_name();
        }
    };
    std::set<std::shared_ptr<ast::LocalVar>, compare_var_names> unique_local_variables;

    // fill up the set with all local variables
    for (const auto& statement: statements) {
        auto local_variables_statements = AstLookupVisitor(ast::AstNodeType::LOCAL_LIST_STATEMENT).lookup(statement.get());
        if (!local_variables_statements.empty()) {
            for (const auto& lvs : local_variables_statements) {
                auto local_variables = dynamic_cast<ast::LocalListStatement*>(lvs.get())->variables;
                std::copy(local_variables.begin(),
                          local_variables.end(),
                          std::inserter(unique_local_variables, unique_local_variables.begin()));
            }
        }
    }

    if (!unique_local_variables.empty()) {
        // create a new LocalListStatement that holds all the unique variables
        std::vector<std::shared_ptr<ast::LocalVar>> local_var_vec;
        std::copy(unique_local_variables.begin(),
                  unique_local_variables.end(),
                  std::back_inserter(local_var_vec));

        auto unified_local_variables_statement = new ast::LocalListStatement(local_var_vec);

        // print it
        CodegenCVisitor::visit_local_list_statement(unified_local_variables_statement);
        printer->add_line(";");
    }
}

void CodegenKernkraft::print_global_function_common_code(BlockType type) {
    // do nothing
    std::string method = compute_method_name(type);
    auto args = "NrnThread* nt, Memb_list* ml, int type";
    printer->start_block("void {}({})"_format(method, args));
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

    /// ion_* variables
    for (auto& var: codegen_int_variables) {
        // N.B. Kerncraft does not support integer arrays
        if (!var.is_vdata && !var.is_integer && !var.is_index) {
            auto name = var.symbol->get_name();
            printer->add_line("{} {}[N];"_format(float_type, name));
        }
    }

    auto& globals = info.global_variables;
    auto& constants = info.constant_variables;

    if (!globals.empty()) {
        for (const auto& var: globals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!constants.empty()) {
        for (const auto& var: constants) {
            auto name = var->get_name();
            auto value_ptr = var->get_value();
            printer->add_line("{} {};"_format(float_type, name));
            codegen_global_variables.push_back(var);
        }
    }

    printer->add_line( "double voltage[N];");
    //printer->add_line( "double node_area[N];");
    printer->add_line( "double shadow_rhs[N];");
    printer->add_line( "double shadow_d[N];");
    printer->add_line( "double vec_rhs[N];");
    printer->add_line( "double vec_d[N];");

    printer->add_line("double v;");
    printer->add_line("double dt;");
    printer->add_line("double lg;");
    printer->add_line("double rhs;");
    printer->add_line("double mfactor;");
    printer->add_line("double celsius;");

    printer->add_line("double FARADAY;");
    printer->add_line("double PI;");
    printer->add_line("double R;");

}

void CodegenKernkraft::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for cpu backend, just get loop bounds
    if (ion_variable_struct_required()) {
        //should loop over members of ion variable and declare them here
        for (const auto& ion: info.ions) {
            for (const auto& r : ion.reads) {
                auto name = read_ion_variable_name(r).second;
                printer->add_line("double {}[N];"_format(name));
            }
            for (const auto& w : ion.writes) {
                auto name = write_ion_variable_name(w).second;
                printer->add_line("double {}[N];"_format(w));
            }
        }
    }
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

    auto statements = info.nrn_state_block->get_solve_statements();
    print_unique_local_variables(statements);

    print_channel_iteration_tiling_block_begin(BlockType::State);
    print_channel_iteration_block_begin(BlockType::State);
    print_post_channel_iteration_common_code();

    printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
    printer->add_line("/*[KR INFO] node_id = node_index[id]; */");
    printer->add_line("/*[KR INFO] v = voltage[node_id]; */");
    printer->add_line("v = voltage[id];");


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

    auto statements = info.breakpoint_node->get_statement_block()->get_statements();
    print_unique_local_variables(statements);

    print_channel_iteration_tiling_block_begin(BlockType::Equation);
    print_channel_iteration_block_begin(BlockType::Equation);
    print_post_channel_iteration_common_code();

    print_nrn_cur_kernel(info.breakpoint_node);

    print_nrn_cur_matrix_shadow_update();
    print_channel_iteration_block_end();

    /*//MUST BE HANDLED AS A SEPARATE KERNEL
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
            printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
            printer->add_text("/*[KR INFO] ");
            printer->add_text("vec_rhs[node_id] {} rhs;"_format(rhs_op));
            printer->add_line("*/");
            print_atomic_reduction_pragma();
            printer->add_line("vec_rhs[id] {} rhs;"_format(rhs_op));
            printer->add_line("/*[KR INFO] elision of 1 indirect access: */");
            printer->add_text("/*[KR INFO] ");
            printer->add_text("vec_d[node_id] {} rhs;"_format(rhs_op));
            printer->add_line("*/");
            print_atomic_reduction_pragma();
            printer->add_line("vec_d[id] {} lg;"_format(d_op));
        }
    }
}

void CodegenKernkraft::print_codegen_routines() {
    codegen = true;
    print_mechanism_range_var_structure();
    print_nrn_cur();
    //print_nrn_state();
    codegen = false;
}

}  // namespace codegen
}  // namespace nmodl

