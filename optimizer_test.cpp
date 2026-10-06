#include "src/ir/hir.hpp"
#include "src/ir/hir_optimizer.hpp"
#include <iostream>
#include <cmath>

using namespace lanner::hir;

int main() {
    Function f;
    f.name = "constant";
    f.returnType = {ScalarKind::Int, 64, false, "u64", "i64"};
    f.blocks.push_back(BasicBlock{"entry", {}});

    Instruction a; a.op=Opcode::ConstInt; a.type=f.returnType; a.result="%a"; a.intValue=40;
    Instruction b; b.op=Opcode::ConstInt; b.type=f.returnType; b.result="%b"; b.intValue=2;
    Instruction add; add.op=Opcode::Binary; add.type=f.returnType; add.result="%sum"; add.lhs="%a"; add.rhs="%b"; add.lhsType=f.returnType; add.rhsType=f.returnType; add.operatorName="+";
    Instruction ret; ret.op=Opcode::Return; ret.type=f.returnType; ret.lhs="%sum";
    f.blocks[0].instructions = {a,b,add,ret};

    Module m; m.functions.push_back(std::move(f));
    optimize(m, OptimizationLevel::O2);
    const auto& insts = m.functions[0].blocks[0].instructions;
    if (insts.size() != 2) return 1;
    if (insts[0].op != Opcode::ConstInt || insts[0].intValue != 42) return 2;
    if (insts[1].op != Opcode::Return || insts[1].lhs != "%sum") return 3;

    Function signedCast;
    signedCast.name = "signed_cast";
    signedCast.returnType = {ScalarKind::Int, 64, true, "i64", "i64"};
    signedCast.blocks.push_back(BasicBlock{"entry", {}});

    Instruction neg8;
    neg8.op = Opcode::ConstInt;
    neg8.type = {ScalarKind::Int, 8, true, "i8", "i8"};
    neg8.result = "%neg8";
    neg8.intValue = static_cast<std::uint64_t>(static_cast<std::int64_t>(-7));

    Instruction widen;
    widen.op = Opcode::Cast;
    widen.sourceType = neg8.type;
    widen.type = signedCast.returnType;
    widen.result = "%wide";
    widen.lhs = "%neg8";

    Instruction signedRet;
    signedRet.op = Opcode::Return;
    signedRet.type = signedCast.returnType;
    signedRet.lhs = "%wide";
    signedCast.blocks[0].instructions = {neg8, widen, signedRet};

    Module signedModule;
    signedModule.functions.push_back(std::move(signedCast));
    optimize(signedModule, OptimizationLevel::O2);
    const auto& signedInsts = signedModule.functions[0].blocks[0].instructions;
    if (signedInsts.size() != 2) return 4;
    if (signedInsts[0].op != Opcode::ConstInt || signedInsts[0].intValue != static_cast<std::uint64_t>(static_cast<std::int64_t>(-7))) return 5;
    if (signedInsts[1].op != Opcode::Return || signedInsts[1].lhs != "%wide") return 6;


    Function boundsFn;
    boundsFn.name = "fixed_bounds";
    boundsFn.returnType = {ScalarKind::Int, 32, true, "i32", "i32"};
    boundsFn.blocks.push_back(BasicBlock{"entry", {}});
    Instruction idx; idx.op=Opcode::ConstInt; idx.type={ScalarKind::Int,64,false,"usize","i64"}; idx.result="%idx"; idx.intValue=1;
    Instruction check; check.op=Opcode::BoundsCheck; check.type={ScalarKind::Bool,1,false,"bool","i1"}; check.rhs="%idx"; check.rhsType=idx.type; check.aggregateIndex=4; check.aggregateIndexConstant=true;
    Instruction ret42; ret42.op=Opcode::Return; ret42.type=boundsFn.returnType; ret42.lhs="%idx";
    boundsFn.blocks[0].instructions = {idx, check, ret42};
    Module boundsModule; boundsModule.functions.push_back(std::move(boundsFn));
    optimize(boundsModule, OptimizationLevel::O2);
    const auto& boundsInsts = boundsModule.functions[0].blocks[0].instructions;
    if (boundsInsts.size() != 2 || boundsInsts[1].op != Opcode::Return) return 7;
    for (const auto& inst : boundsInsts) if (inst.op == Opcode::BoundsCheck) return 8;

    Function loadsFn;
    loadsFn.name = "forward_load";
    loadsFn.returnType = {ScalarKind::Int,32,true,"i32","i32"};
    loadsFn.blocks.push_back(BasicBlock{"entry", {}});
    Instruction v; v.op=Opcode::ConstInt; v.type=loadsFn.returnType; v.result="%v"; v.intValue=7;
    Instruction st; st.op=Opcode::StoreLocal; st.type=loadsFn.returnType; st.slot="x"; st.lhs="%v";
    Instruction ld; ld.op=Opcode::LoadLocal; ld.type=loadsFn.returnType; ld.result="%ld"; ld.slot="x";
    Instruction add2; add2.op=Opcode::Binary; add2.type=loadsFn.returnType; add2.result="%sum"; add2.lhs="%ld"; add2.rhs="%v"; add2.lhsType=loadsFn.returnType; add2.rhsType=loadsFn.returnType; add2.operatorName="+";
    Instruction ret2; ret2.op=Opcode::Return; ret2.type=loadsFn.returnType; ret2.lhs="%sum";
    loadsFn.blocks[0].instructions = {v, st, ld, add2, ret2};
    Module loadsModule; loadsModule.functions.push_back(std::move(loadsFn));
    optimize(loadsModule, OptimizationLevel::O2);
    const auto& loadInsts = loadsModule.functions[0].blocks[0].instructions;
    for (const auto& inst : loadInsts) if (inst.op == Opcode::LoadLocal) return 9;
    bool rewrote = false;
    for (const auto& inst : loadInsts) if (inst.op == Opcode::Binary && inst.lhs == "%v") rewrote = true;
    if (!rewrote) return 10;

    Function floatFn;
    floatFn.name = "float_fold";
    floatFn.returnType = {ScalarKind::Float, 64, true, "f64", "double"};
    floatFn.blocks.push_back(BasicBlock{"entry", {}});
    Instruction fa; fa.op=Opcode::ConstFloat; fa.type=floatFn.returnType; fa.result="%fa"; fa.floatValue=1.5;
    Instruction fb; fb.op=Opcode::ConstFloat; fb.type=floatFn.returnType; fb.result="%fb"; fb.floatValue=2.5;
    Instruction fadd; fadd.op=Opcode::Binary; fadd.type=floatFn.returnType; fadd.result="%fadd";
    fadd.lhs="%fa"; fadd.rhs="%fb"; fadd.lhsType=floatFn.returnType; fadd.rhsType=floatFn.returnType; fadd.operatorName="+";
    Instruction fret; fret.op=Opcode::Return; fret.type=floatFn.returnType; fret.lhs="%fadd";
    floatFn.blocks[0].instructions = {fa, fb, fadd, fret};
    Module floatModule; floatModule.functions.push_back(std::move(floatFn));
    optimize(floatModule, OptimizationLevel::O2);
    const auto& floatInsts = floatModule.functions[0].blocks[0].instructions;
    if (floatInsts.size() != 2) return 13;
    bool sawFloat42 = false;
    for (const auto& inst : floatInsts) if (inst.op == Opcode::ConstFloat && inst.result == "%fadd" && std::abs(inst.floatValue - 4.0) < 1e-12) sawFloat42 = true;
    if (!sawFloat42) return 14;

    Function floatCmp;
    floatCmp.name = "float_cmp";
    floatCmp.returnType = {ScalarKind::Int, 32, true, "i32", "i32"};
    floatCmp.blocks.push_back(BasicBlock{"entry", {}});
    Instruction ca; ca.op=Opcode::ConstFloat; ca.type=floatModule.functions[0].returnType; ca.result="%ca"; ca.floatValue=7.0;
    Instruction cb; cb.op=Opcode::ConstFloat; cb.type=floatModule.functions[0].returnType; cb.result="%cb"; cb.floatValue=3.0;
    Instruction ccmp; ccmp.op=Opcode::Compare; ccmp.type={ScalarKind::Bool,1,false,"bool","i1"}; ccmp.result="%cc";
    ccmp.lhs="%ca"; ccmp.rhs="%cb"; ccmp.lhsType=ca.type; ccmp.rhsType=cb.type; ccmp.operatorName=">";
    Instruction ccast; ccast.op=Opcode::Cast; ccast.sourceType=ccmp.type; ccast.type=floatCmp.returnType; ccast.result="%ci"; ccast.lhs="%cc";
    Instruction cret; cret.op=Opcode::Return; cret.type=floatCmp.returnType; cret.lhs="%ci";
    floatCmp.blocks[0].instructions = {ca, cb, ccmp, ccast, cret};
    Module cmpModule; cmpModule.functions.push_back(std::move(floatCmp));
    optimize(cmpModule, OptimizationLevel::O2);
    const auto& cmpInsts = cmpModule.functions[0].blocks[0].instructions;
    if (cmpInsts.size() != 2) return 15;
    if (cmpInsts[0].op != Opcode::ConstInt || cmpInsts[0].intValue != 1) return 16;

    Function aggregateLoadFn;
    aggregateLoadFn.name = "aggregate_load_preserved";
    aggregateLoadFn.returnType = loadsFn.returnType;
    aggregateLoadFn.blocks.push_back(BasicBlock{"entry", {}});
    Type owningAgg{ScalarKind::Aggregate, 0, false, "[]i32", "{ ptr, i64, i64, ptr }"};
    Instruction av; av.op=Opcode::ZeroValue; av.type=owningAgg; av.result="%av";
    Instruction ast; ast.op=Opcode::StoreLocal; ast.type=owningAgg; ast.slot="agg"; ast.lhs="%av";
    Instruction ald; ald.op=Opcode::LoadLocal; ald.type=owningAgg; ald.result="%ald"; ald.slot="agg";
    Instruction aidx; aidx.op=Opcode::AggregateIndex; aidx.type=loadsFn.returnType; aidx.result="%aidx"; aidx.lhs="%ald"; aidx.aggregateIndex=0; aidx.aggregateIndexConstant=true;
    Instruction aret; aret.op=Opcode::Return; aret.type=loadsFn.returnType; aret.lhs="%aidx";
    aggregateLoadFn.blocks[0].instructions = {av, ast, ald, aidx, aret};
    Module aggregateModule; aggregateModule.functions.push_back(std::move(aggregateLoadFn));
    optimize(aggregateModule, OptimizationLevel::O2);
    bool aggregateLoadRetained = false;
    for (const auto& inst : aggregateModule.functions[0].blocks[0].instructions) {
        if (inst.op == Opcode::LoadLocal) aggregateLoadRetained = true;
    }
    if (!aggregateLoadRetained) return 12;

    Function callBarrierFn;
    callBarrierFn.name = "call_barrier";
    callBarrierFn.returnType = loadsFn.returnType;
    callBarrierFn.blocks.push_back(BasicBlock{"entry", {}});
    Instruction cv; cv.op=Opcode::ConstInt; cv.type=loadsFn.returnType; cv.result="%cv"; cv.intValue=9;
    Instruction cst; cst.op=Opcode::StoreLocal; cst.type=loadsFn.returnType; cst.slot="x"; cst.lhs="%cv";
    Instruction call; call.op=Opcode::Call; call.type={ScalarKind::Void,0,false,"void","void"}; call.callee="mutate";
    Instruction cld; cld.op=Opcode::LoadLocal; cld.type=loadsFn.returnType; cld.result="%cld"; cld.slot="x";
    Instruction crt; crt.op=Opcode::Return; crt.type=loadsFn.returnType; crt.lhs="%cld";
    callBarrierFn.blocks[0].instructions = {cv, cst, call, cld, crt};
    Module callModule; callModule.functions.push_back(std::move(callBarrierFn));
    optimize(callModule, OptimizationLevel::O2);
    const auto& callInsts = callModule.functions[0].blocks[0].instructions;
    bool retainedLoad = false;
    for (const auto& inst : callInsts) if (inst.op == Opcode::LoadLocal) retainedLoad = true;
    if (!retainedLoad) return 11;

    std::cout << "optimizer_test: PASS\n";
    return 0;
}
