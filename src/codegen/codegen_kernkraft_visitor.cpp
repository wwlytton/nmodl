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
    print_data_structures();
    print_nrn_cur();
    print_nrn_state();
    codegen = false;
}

}  // namespace codegen
}  // namespace nmodl
