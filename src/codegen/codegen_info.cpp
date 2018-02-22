#include "codegen/codegen_info.hpp"

using namespace codegen;


/// if any ion has write variable
bool CodegenInfo::ion_has_write_variable() {
    for (const auto& ion : ions) {
        if (!ion.writes.empty()) {
            return true;
        }
    }
    return false;
}


/// if given variable is ion write variable
bool CodegenInfo::is_ion_write_variable(std::string name) {
    for (const auto& ion : ions) {
        for (auto& var : ion.writes) {
            if (var == name) {
                return true;
            }
        }
    }
    return false;
}


/// if given variable is ion read variable
bool CodegenInfo::is_ion_read_variable(std::string name) {
    for (const auto& ion : ions) {
        for (auto& var : ion.reads) {
            if (var == name) {
                return true;
            }
        }
    }
    return false;
}


/// if either read or write variable
bool CodegenInfo::is_ion_variable(std::string name) {
    if (is_ion_read_variable(name) || is_ion_write_variable(name)) {
        return true;
    }
    return false;
}


/// if a current
bool CodegenInfo::is_current(std::string name) {
    for (auto& var : currents) {
        if (var == name) {
            return true;
        }
    }
    return false;
}