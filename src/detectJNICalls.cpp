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
    if (!offsetBase) {
        BitCastInst* bitcast = dyn_cast<BitCastInst>(loadFunctionAddressInst->getPointerOperand());
        if (!bitcast) return false;

        offsetBase = dyn_cast<GetElementPtrInst>(bitcast->getOperand(0));
        if (!offsetBase) return false;

    }
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

    static std::map<std::string, JNICallOffset>* wrapper_function_names = nullptr;
    if (!wrapper_function_names){
        wrapper_function_names = new std::map<std::string, JNICallOffset>;
        (*wrapper_function_names)["_ZN7JNIEnv_9FindClassEPKc"] = (unsigned long) (&j.FindClass) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_11GetMethodIDEP7_jclassPKcS3_"] = (unsigned long) (&j.GetMethodID) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_16GetStaticFieldIDEP7_jclassPKcS3_"] = (unsigned long) (&j.GetStaticFieldID) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_20GetStaticObjectFieldEP7_jclassP9_jfieldID"] = (unsigned long) (&j.GetStaticObjectField) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_16CallObjectMethodEP8_jobjectP10_jmethodIDz"] = (unsigned long) (&j.CallObjectMethod) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_9NewObjectEP7_jclassP10_jmethodIDz"] = (unsigned long) (&j.NewObject) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_10GetFieldIDEP7_jclassPKcS3_"] = (unsigned long) (&j.GetFieldID) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_14SetObjectFieldEP8_jobjectP9_jfieldIDS1_"] = (unsigned long) (&j.SetObjectField) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_11SetIntFieldEP8_jobjectP9_jfieldIDi"] = (unsigned long) (&j.SetIntField) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_14GetObjectClassEP8_jobject"] = (unsigned long) (&j.GetObjectClass) - (unsigned long) (&j);
        (*wrapper_function_names)["_ZN7JNIEnv_14GetObjectFieldEP8_jobjectP9_jfieldID"] = (unsigned long) (&j.GetObjectField) - (unsigned long) (&j);

    }
    // don't add callsites inside wrapper functions
    if ((*wrapper_function_names)[F.getName().str()])
        return;
    for (const BasicBlock &B : F) {
        for (const Instruction &I: B) {
            if (auto *cb = dyn_cast<CallBase>(&I)) {
                JNICallOffset offset;
                if (auto callee = cb->getCalledFunction()){
                    auto name = cb->getCalledFunction()->getName().str();
                    if (name.find("_ZN7JNIEnv") == 0){
                        if (auto offset = (*wrapper_function_names)[name]){
                            detectedJNICalls[cb] = offset;
                        } else {
                            std::cout << "unknown JNI function " << name << std::endl;
                        }
                    }

                }

                if (isJNIEnvOffsetCall(module, cb, offset)) {
                    detectedJNICalls[cb] = offset;
                }
            }
        }
    }

}
