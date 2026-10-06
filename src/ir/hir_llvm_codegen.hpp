#pragma once

#include "hir.hpp"
#include <map>
#include <string>

namespace lanner::hir {

class LLVMCodegen {
public:
    std::string generate(const Module& module) const;

private:
    std::string llvmType(const Type& type) const;
    std::string cmpPredicate(const Type& type, const std::string& op) const;
    std::string binaryOpcode(const Type& type, const std::string& op) const;
    std::string castInstruction(const Type& source, const Type& dest) const;
    void emitFunction(const Module& module, const Function& fn, std::string& out) const;
};

} // namespace lanner::hir
