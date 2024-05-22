//
// Created by julius on 4/10/24.
//

#include "detectJNICalls.h"
using namespace llvm;


const char* getStringParameterInitializer(Value* arg) {
    if (auto strConst = llvm::dyn_cast<llvm::ConstantExpr>(arg)) {
        errs() << *strConst << "\n";
        auto strGlobal = llvm::dyn_cast<llvm::GlobalVariable>(strConst->getOperand(0));
        auto initializer = strGlobal->getInitializer();
        ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
        StringRef s = str->getAsCString();
        return s.data();
    }
    if (const GetElementPtrInst* loadStr = llvm::dyn_cast<llvm::GetElementPtrInst>(arg)) {
        auto strVal = loadStr->getOperand(0);
        auto strConst = llvm::dyn_cast<llvm::ConstantDataArray>(strVal);
        auto strGlobal = llvm::dyn_cast<llvm::GlobalVariable>(strVal);
        auto initializer = strGlobal->getInitializer();
        ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
        StringRef s = str->getAsCString();
        return s.data();
    }
    if (auto strGlobal = llvm::dyn_cast<GlobalVariable>(arg)) {
        auto initializer = strGlobal->getInitializer();
        ConstantDataSequential *str = llvm::dyn_cast<ConstantDataSequential>(initializer);
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
// for a jclass operand passed to a JNI function, tries to find the name of the jclass,
// assuming it was obtained using FindClass.
//
// jclass cls = (*env)->FindClass(env, "Lorg/opalj/fpcf/fixtures/xl/llvm/controlflow/bidirectional/CreateJavaInstanceFromNative;");
// jmethodID methodID = (*env)->GetMethodID(env, cls, "myJavaFunction", "(Ljava/lang/Object;)V");
// (*env)->CallVoidMethod(env, instance, methodID, x);
const char* getClassName(const Module* module, Value* classOperand, const Instruction* usage, const llvm::Function& F) {
    JNINativeInterface_ j{};
    const JNICallOffset offset_FindClass = (unsigned long) (&j.FindClass) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectClass = (unsigned long) (&j.GetObjectClass) - (unsigned long) (&j);
    AllocaInst* classAddr = dyn_cast<AllocaInst>(getPointerOperand(classOperand)) ;
    if (!classAddr) {
        errs() << "could not determine class \n";
        return nullptr;
    }
    const CallBase* getClassCall = getDefiningCallsite(classAddr, usage,  F);
    if (!getClassCall) {
        errs() << "could not determine origin of class\n";
        return nullptr;
    }
    JNICallOffset getClassJNIOffset;
    if (!isJNIEnvOffsetCall(module, getClassCall, getClassJNIOffset)) {
        errs() << "could not determine origin of class\n";
        return nullptr;
    }

    if (getClassJNIOffset == offset_FindClass)
    {
        Value *classNameOperand = getClassCall->getArgOperand(1);
        return getStringParameterInitializer(classNameOperand);
    } else if (getClassJNIOffset == offset_GetObjectClass) {
        errs() << "class definition computed dynamically with GetObjectClass\n";
        return nullptr;
    } else {
        return nullptr;
    }
}

// if cb is CallVoidMethod, resolves target class and method name
//
// jclass cls = (*env)->FindClass(env, "Lorg/opalj/fpcf/fixtures/xl/llvm/controlflow/bidirectional/CreateJavaInstanceFromNative;");
// jmethodID methodID = (*env)->GetMethodID(env, cls, "myJavaFunction", "(Ljava/lang/Object;)V");
// (*env)->CallVoidMethod(env, instance, methodID, x);
void DetectNICalls::handleJniCall(const Module* module, JNICallOffset offset, const CallBase* cb, const Function& F) {
    JNINativeInterface_ j{};
    errs() << "processing jni call " << *cb << "\n";
    const JNICallOffset offset_GetMethodID = (unsigned long) (&j.GetMethodID) - (unsigned long) (&j);
    const JNICallOffset offset_FindClass = (unsigned long) (&j.FindClass) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectClass = (unsigned long) (&j.GetObjectClass) - (unsigned long) (&j);

    Value* obj = cb->getOperand(1);
    Value* methodIDLoad = dyn_cast<LoadInst>(cb->getOperand(2));
    if (!methodIDLoad) return;
    AllocaInst* methodIDAddr = dyn_cast<AllocaInst>(getPointerOperand(methodIDLoad)) ;
    if (!methodIDAddr) return;
    const CallBase* getMethodIDCall = getDefiningCallsite(methodIDAddr, cb, F);
    JNICallOffset getMethodIdOffset;
    if (!isJNIEnvOffsetCall(module, getMethodIDCall, getMethodIdOffset)) return;
    if (getMethodIdOffset != offset_GetMethodID) {
        errs() << "very weird behavior. unexpected methodID parameter\n";
        return;
    }
    //     jmethodID (JNICALL *GetMethodID)
    //      (JNIEnv *env, jclass clazz, const char *name, const char *sig);
    Value* classOperand = getMethodIDCall->getArgOperand(1);
    Value* methodNameOperand = getMethodIDCall->getArgOperand(2);
    Value* methodSigOperand = getMethodIDCall->getArgOperand(3);
    const char* methodName = getStringParameterInitializer(methodNameOperand);
    const char* methodSig = getStringParameterInitializer(methodSigOperand);
    if (!methodName){
        errs() << "method name " << methodName << "\n";
        return;
    }
    const char* className = getClassName(module, classOperand, getMethodIDCall, F);
    if (!className){
        errs() << "found invoke with unknown className\n" << methodName << "\n";
    }

    JNIInvocation* invoke = new JNIInvocation(methodName, methodSig, className, cb);
    detectedJNIInvocations[cb] = invoke;
}

void DetectNICalls::handleGetOrSetField(const llvm::Module *module, JNICallOffset offset, const llvm::CallBase *cb, const Function& F) {
    JNINativeInterface_ j{};
    const JNICallOffset offset_GetFieldID = (unsigned long) (&j.GetFieldID) - (unsigned long) (&j);
    Value* obj = cb->getOperand(1);
    Value* fieldIDLoad = dyn_cast<LoadInst>(cb->getOperand(2));
    if (!fieldIDLoad) return;

    AllocaInst* fieldIDAddr = dyn_cast<AllocaInst>(getPointerOperand(fieldIDLoad)) ;
    if (!fieldIDAddr) return;
    const CallBase* getFieldIDCall = getDefiningCallsite(fieldIDAddr, cb, F);
    JNICallOffset getFieldIDOffset;
    if (!isJNIEnvOffsetCall(module, getFieldIDCall, getFieldIDOffset)) return;
    if (getFieldIDOffset != offset_GetFieldID) {
        errs() << "very weird behavior. unexpected MethodID parameter for GetField\n";
        return;
    }

    Value* classOperand = getFieldIDCall->getArgOperand(1);
    Value* methodNameOperand = getFieldIDCall->getArgOperand(2);
    Value* methodSigOperand = getFieldIDCall->getArgOperand(3);
    const char* fieldName = getStringParameterInitializer(methodNameOperand);
    const char* fieldSig = getStringParameterInitializer(methodSigOperand);
    if (!fieldName){
        errs() << "method name " << fieldName << "\n";
        return;
    }
    const char* className = getClassName(module, classOperand, cb, F);
    if (!className){
        errs() << "found invoke with unknown className\n";
    }

    if (offset == (unsigned long) (&j.SetObjectField) - (unsigned long) (&j)) {
        detectedJNIFieldSets[cb] = new JNISetField(fieldName, className, cb);
    } else {
        detectedJNIFieldGets[cb] = new JNIGetField(fieldName, className, cb);
    }

}
// NewObject or AllocObject. AllocObject doesn't take a constructor methodID.
void DetectNICalls::handleNewObject(const Module* module, JNICallOffset offset, const CallBase* cb, const llvm::Function& F)  {
    Value* classOperand = cb->getArgOperand(1);
    const char* className = getClassName(module, classOperand, cb, F);
    if (className){
        JNIAllocation* alo = new JNIAllocation(className, cb);
        detectedJNIAllocations[cb] = alo;
    }
}
void DetectNICalls::processFunction(const llvm::Function &F) {
    JNINativeInterface_ j{};

    const JNICallOffset offset_CallVoidMethod = (unsigned long) (&j.CallVoidMethod) - (unsigned long) (&j);
    const JNICallOffset offset_CallObjectMethod = (unsigned long) (&j.CallObjectMethod) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectField = (unsigned long) (&j.GetObjectField) - (unsigned long) (&j);
    const JNICallOffset offset_SetObjectField = (unsigned long) (&j.SetObjectField) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectArrayElement = (unsigned long) (&j.GetObjectArrayElement) - (unsigned long) (&j);
    const JNICallOffset offset_SetObjectArrayElement = (unsigned long) (&j.SetObjectArrayElement) - (unsigned long) (&j);

    const JNICallOffset offset_NewObject = (unsigned long) (&j.NewObject) - (unsigned long) (&j);
    const JNICallOffset offset_AllocObject = (unsigned long) (&j.AllocObject) - (unsigned long) (&j);
    const Module* module = F.getParent();
    for (const BasicBlock &B : F) {
        for (const Instruction &I: B) {
            if (auto *cb = dyn_cast<CallBase>(&I)) {
                JNICallOffset offset;
                if (isJNIEnvOffsetCall(module, cb, offset)) {
                    if (offset == offset_CallObjectMethod || offset == offset_CallVoidMethod) {
                        // Found JNI callMethod, now try to resolve
                        //    jobject (JNICALL *CallObjectMethod)
                        //      (JNIEnv *env, jobject obj, jmethodID methodID, ...);
                        handleJniCall(module, offset, cb, F);
                    } else if (offset == offset_NewObject || offset == offset_AllocObject) {
                        handleNewObject(module, offset, cb, F);
                    } else if (offset == offset_GetObjectField || offset == offset_SetObjectField) {
                        handleGetOrSetField(module, offset, cb, F);
                    } else if (offset == offset_GetObjectArrayElement) {
                        detectedJNIArrayElementGets[cb] = new JNIGetArrayElement(cb);
                    }
                }
            }
        }
    }

}
