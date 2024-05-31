//
// Created by julius on 5/31/24.
//
#include "svf-ex.h"
using namespace std;
using namespace SVF;
const char* ExtendedPAG::getClassName(const SVFValue* paramClass){
    JNINativeInterface_ j{};
    const JNICallOffset offset_FindClass = (unsigned long) (&j.FindClass) - (unsigned long) (&j);
    for (const auto &item: customAndersen->getPts(pag->getValueNode(paramClass))) {
        if (auto GetMethodId = jniCallsiteDummyNodes[item]) {
            if (GetMethodId->second == offset_FindClass) {
                auto paramClassName = GetMethodId->first->getArgOperand(1);
                auto llvmParamClassName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramClassName);
                if (const char* className = getStringParameterInitializer(llvmParamClassName)){
                    return className;
                }
            }
        }
    }
    return nullptr;
}

const SVFCallInst* ExtendedPAG::retrieveGetFieldID(const llvm::CallBase* getOrSetField){
    JNINativeInterface_ j{};
    const JNICallOffset offset_GetFieldID = (unsigned long) (&j.GetFieldID) - (unsigned long) (&j);
    auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getOrSetField);
    auto call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    NodeID fieldIDParam = pag->getValueNode(call->getArgOperand(2));
    for (const auto &item: customAndersen->getPts(fieldIDParam)) {
        if (auto GetFieldId = jniCallsiteDummyNodes[item]) {
            if (GetFieldId->second == offset_GetFieldID) {
                return GetFieldId->first;

            }
        }
    }
    return nullptr;
}
set<long>* ExtendedPAG::getReturnPTSForJNICallsite(const llvm::CallBase* callsite) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(callsite);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    JNINativeInterface_ j{};
    const JNICallOffset offset_GetMethodID = (unsigned long) (&j.GetMethodID) - (unsigned long) (&j);
    const JNICallOffset offset_FindClass = (unsigned long) (&j.FindClass) - (unsigned long) (&j);

    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);
    auto baseNode = pag->getValueNode(base);

    set<long>* basePTS = getPTS(baseNode);

    std::vector<std::set<long>*> argumentsPTS;
    for (int i = 3; i < call->getNumArgOperands(); i++){
        auto arg = call->getArgOperand(i);
        auto argNode = pag->getValueNode(arg);
        set<long>* argPTS = getPTS(argNode);
        argumentsPTS.push_back(argPTS);
    }

    set<long>* returnPts = new set<long>();
    NodeID methodIDParam = pag->getValueNode(call->getArgOperand(2));
    for (const auto &item: customAndersen->getPts(methodIDParam)) {
        if (auto GetMethodId = jniCallsiteDummyNodes[item]) {
            if (GetMethodId->second == offset_GetMethodID) {
                cout << "yeet" << endl;

                auto paramClass = GetMethodId->first->getArgOperand(1);
                auto paramMethodName = GetMethodId->first->getArgOperand(2);
                auto paramMethodSig = GetMethodId->first->getArgOperand(3);
                auto llvmParamMethodName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramMethodName);
                auto llvmParamMethodSig = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramMethodSig);
                const char* methodName = getStringParameterInitializer(llvmParamMethodName);
                cout << methodName << endl;
                const char* methodSig = getStringParameterInitializer(llvmParamMethodSig);
                cout << methodSig << endl;
                if (methodName && methodSig) {
                    if (const char* className = getClassName(paramClass)){
                        cout << "requesting return PTS for function " << methodName << endl;
                        auto pts = callback_ReportArgPTSGetReturnPTS(basePTS, className, methodName, methodSig, argumentsPTS);
                        returnPts->insert(pts->begin(), pts->end());
                    }
                }
            }
        }
    }
    return returnPts;

}


set<long>* ExtendedPAG::getPTSForField(const llvm::CallBase* getField) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getField);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);

    auto baseNode = pag->getValueNode(base);

    set<long>* basePTS = getPTS(baseNode);
    if (auto GetFieldId = retrieveGetFieldID(getField)){
        auto paramClass = GetFieldId->getArgOperand(1);
        auto llvmParamFieldName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetFieldId->getArgOperand(2));

        const char* fieldName = getStringParameterInitializer(llvmParamFieldName);
        const char* className = getClassName(paramClass);
        cout << "requesting return PTS for field " << fieldName << endl;
        return callback_GetFieldPTS(basePTS, className, fieldName);
    }


}
set<long>* ExtendedPAG::getPTSForArray(const llvm::CallBase* getArrayElement) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getArrayElement);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);

    auto baseNode = pag->getValueNode(base);

    set<long>* basePTS = getPTS(baseNode);


    cout << "requesting return PTS for array. pts size " << basePTS->size() << endl;
    return callback_GetArrayElementPTS(basePTS);

}
void ExtendedPAG::reportSetField(const llvm::CallBase* setField) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(setField);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);
    auto baseNode = pag->getValueNode(base);
    set<long>* basePTS = getPTS(baseNode);

    auto arg = call->getArgOperand(3);
    auto argNode = pag->getValueNode(arg);
    set<long>* argPTS = getPTS(argNode);

    if (auto GetFieldId = retrieveGetFieldID(setField)){
        auto paramClass = GetFieldId->getArgOperand(1);
        auto llvmParamFieldName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetFieldId->getArgOperand(2));

        const char* fieldName = getStringParameterInitializer(llvmParamFieldName);
        const char* className = getClassName(paramClass);
        cout << "requesting return PTS for field " << fieldName << endl;
        callback_SetFieldPTS(basePTS, className, fieldName, argPTS);
    }
}

// this reports callsite arguments of all jni callsites and field sets to the java analysis
// transitively (for the passed function plus any called functions)!
// adds returned PTSs to PAG for future propagation
void ExtendedPAG::runDetector(const SVFFunction* function, set<const SVFFunction*>* processed) {
    JNINativeInterface_ j{};
    const JNICallOffset offset_CallVoidMethod = (unsigned long) (&j.CallVoidMethod) - (unsigned long) (&j);
    const JNICallOffset offset_CallObjectMethod = (unsigned long) (&j.CallObjectMethod) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectField = (unsigned long) (&j.GetObjectField) - (unsigned long) (&j);
    const JNICallOffset offset_SetObjectField = (unsigned long) (&j.SetObjectField) - (unsigned long) (&j);
    const JNICallOffset offset_GetObjectArrayElement = (unsigned long) (&j.GetObjectArrayElement) - (unsigned long) (&j);
    const JNICallOffset offset_SetObjectArrayElement = (unsigned long) (&j.SetObjectArrayElement) - (unsigned long) (&j);
    const JNICallOffset offset_NewObject = (unsigned long) (&j.NewObject) - (unsigned long) (&j);
    const JNICallOffset offset_AllocObject = (unsigned long) (&j.AllocObject) - (unsigned long) (&j);

    auto name =  function->getName();
    if (name == "JNICustomFilter"){
        cout << "process " << name << endl;
    }
    const llvm::Value* f_llvm = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(function);
    for (const auto &item: detectedJniCalls->detectedJNICalls) {
        JNICallOffset offset = item.second;
        if (item.first->getFunction() == f_llvm) {
            if (offset == offset_CallObjectMethod || offset == offset_CallVoidMethod) {
                auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
                auto retPTS = getReturnPTSForJNICallsite(item.first);
                NodeID callsiteNode = pag->getValueNode(inst);
                addPTS(callsiteNode, *retPTS);
            } else if (offset == offset_NewObject || offset == offset_AllocObject) {
                handleJNIAllocSite(item.first);
            } else if (offset == offset_GetObjectField) {
                set<long>* fieldPTS = getPTSForField(item.first);
                auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
                NodeID callsiteNode = pag->getValueNode(inst);
                cout << "adding " << fieldPTS->size() << " PTS to field " << endl;
                addPTS(callsiteNode, *fieldPTS);
            } else if (offset == offset_SetObjectField) {
                reportSetField(item.first);

            } else if (offset == offset_GetObjectArrayElement) {
                auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
                getPTSForArray(item.first);
                NodeID callsiteNode = pag->getValueNode(inst);
                NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
                set<NodeID> *s = new set<NodeID>();
                s->insert(dummyNode);
                additionalPTS[callsiteNode] = s;
            }
        }
    }


    // transitively
    PTACallGraph* callgraph = customAndersen->getPTACallGraph();
    PTACallGraphNode* functionNode = callgraph->getCallGraphNode(function);
    for (const auto &callee: functionNode->getOutEdges()) {
        auto cts = callee->toString();
        auto ctst = callee->getDstNode()->toString();

        auto calledFunction = callee->getDstNode()->getFunction();
        if (calledFunction)
            if (processed->insert(calledFunction).second) {
                runDetector(calledFunction, processed);
            }
    }
}
