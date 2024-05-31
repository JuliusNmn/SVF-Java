//
// Created by julius on 4/10/24.
//

#include "detectJNICalls.h"
using namespace llvm;


const char* getStringParameterInitializer(const Value* arg) {
    if (auto strConst = llvm::dyn_cast<llvm::ConstantExpr>(arg)) {
        errs() << *strConst << "\n";
        auto strGlobal = llvm::dyn_cast<llvm::GlobalVariable>(strConst->getOperand(0));
        auto initializer = strGlobal->getInitializer();
        const ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
        StringRef s = str->getAsCString();
        return s.data();
    }
    if (const GetElementPtrInst* loadStr = llvm::dyn_cast<llvm::GetElementPtrInst>(arg)) {
        auto strVal = loadStr->getOperand(0);
        auto strConst = llvm::dyn_cast<llvm::ConstantDataArray>(strVal);
        auto strGlobal = llvm::dyn_cast<llvm::GlobalVariable>(strVal);
        auto initializer = strGlobal->getInitializer();
        const ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
        StringRef s = str->getAsCString();
        return s.data();
    }
    if (auto strGlobal = llvm::dyn_cast<GlobalVariable>(arg)) {
        auto initializer = strGlobal->getInitializer();
        const ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
        StringRef s = str->getAsCString();
        return s.data();
    }
    return nullptr;
}


JNICallOffset getOffset(const Module* module, const llvm::GetElementPtrInst *gepi) {
    APInt ap_offset(32, 0, false);
    gepi->accumulateConstantOffset(module->getDataLayout(), ap_offset);
    JNICallOffset offset = ap_offset.getSExtValue();
    return offset;
}

// return JNIEnv struct offset of a function call, or -1 if the CallSite is not a JNIEnv call
bool isJNIEnvOffsetCall(const Module* module, const CallBase* cb, JNICallOffset& offset) {
    auto calledOperand = cb->getCalledOperand();
    LoadInst* loadFunctionAddressInst = dyn_cast<LoadInst>(calledOperand);
    if (!loadFunctionAddressInst) return false;
    GetElementPtrInst* offsetBase = dyn_cast<GetElementPtrInst>(loadFunctionAddressInst->getPointerOperand());
    if (!offsetBase) return false;
    offset = getOffset(module, offsetBase);
    //errs() << *calledOperand << "\n";
    //errs() << *offsetBase << "\n";

    return true;

}
// for a pointer P, get the last function call where the return value is stored in P, e.g:
// %8 = alloca %struct._jmethodID*, align 8
// store %struct._jmethodID* %23, %struct._jmethodID** %8, align 8
// %23 = call %struct._jmethodID* %20( ...
// before inst usage
const CallBase* getDefiningCallsite(AllocaInst* pointer, const Instruction* usage, const llvm::Function& F) {
    const CallBase* definition;
    for (const auto &BB: F){
        for (const auto &item: BB) {
            if (&item == usage) {
                return definition;
            }
            const StoreInst* storeinst = dyn_cast<StoreInst>(&item);
            if (!storeinst) continue;
            if (storeinst->getPointerOperand() != pointer) continue;
            const Value* methodIDValue = storeinst->getValueOperand();
            auto def = dyn_cast<CallBase>(methodIDValue);
            if (def) {
                definition = def;
            }
        }
    }
}
void DetectNICalls::processFunction(const llvm::Function &F) {
    JNINativeInterface_ j{};
    const Module* module = F.getParent();
    for (const BasicBlock &B : F) {
        for (const Instruction &I: B) {
            if (auto *cb = dyn_cast<CallBase>(&I)) {
                JNICallOffset offset;
                if (isJNIEnvOffsetCall(module, cb, offset)) {
                    detectedJNICalls[cb] = offset;
                }
            }
        }
    }

}
