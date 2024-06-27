//
// Created by julius on 5/31/24.
//
#include "main.h"
using namespace std;
using namespace SVF;
// from a list of callsites, return the latest possible callsite that is executed before instruciton "user"
const CallBase * getImmediateDominator(set<const llvm::CallBase*>* candidateCalls, const llvm::CallBase* user, DominatorTree* tree){
    const llvm::CallBase* immediateDominator = nullptr;
    for (const auto &def: *candidateCalls) {
        if (immediateDominator == nullptr && tree->dominates(def, user)) {
            // def is the first instruction from the set that dominates user
            immediateDominator = def;
        } else {
            if (immediateDominator != nullptr) {
                if (tree->dominates(immediateDominator, def) && tree->dominates(def, user)) {
                    immediateDominator = def;
                }
            }
        }
    }
    return immediateDominator;
}

const char* ExtendedPAG::getClassName(const SVFValue* paramClass, const llvm::CallBase* user, DominatorTree* tree){
    JNINativeInterface_ j{};
    const JNICallOffset offset_FindClass = (unsigned long) (&j.FindClass) - (unsigned long) (&j);
    set<const llvm::CallBase*> candidates;
    for (const auto &item: customAndersen->getPts(pag->getValueNode(paramClass))) {
        if (auto GetMethodId = jniCallsiteDummyNodes[item]) {
            if (GetMethodId->second == offset_FindClass) {
                const llvm::CallBase* getFieldCall = llvm::dyn_cast<llvm::CallBase>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetMethodId->first));
                candidates.insert(getFieldCall);

            }
        }
    }
    const llvm::CallBase* definingCall = getImmediateDominator(&candidates, user, tree);
    if (definingCall){
        auto svfCall = SVFUtil::dyn_cast<SVFCallInst>(LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(definingCall));
        auto paramClassName = svfCall->getArgOperand(1);
        auto llvmParamClassName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramClassName);
        if (const char* className = getStringParameterInitializer(llvmParamClassName)){
            return className;
        }
    }
    return nullptr;
}
const SVFCallInst* ExtendedPAG::retrieveGetFieldID(const llvm::CallBase* getOrSetField, DominatorTree* tree){
    JNINativeInterface_ j{};
    const JNICallOffset offset_GetFieldID = (unsigned long) (&j.GetFieldID) - (unsigned long) (&j);
    auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getOrSetField);
    auto call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    NodeID fieldIDParam = pag->getValueNode(call->getArgOperand(2));
    set<const llvm::CallBase*> candidates;
    for (const auto &item: customAndersen->getPts(fieldIDParam)) {
        if (auto GetFieldId = jniCallsiteDummyNodes[item]) {
            if (GetFieldId->second == offset_GetFieldID) {
                const llvm::CallBase* getFieldCall = llvm::dyn_cast<llvm::CallBase>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetFieldId->first));
                candidates.insert(getFieldCall);
            }
        }
    }
    const llvm::CallBase* definingCall = getImmediateDominator(&candidates, getOrSetField, tree);
    if (definingCall){
        return SVFUtil::dyn_cast<SVFCallInst>(LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(definingCall));
    }
    return nullptr;
}
set<long>* ExtendedPAG::getReturnPTSForJNICallsite(const llvm::CallBase* callsite, DominatorTree* tree) {
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
    set<const llvm::CallBase*> candidatesGetMethodId;

    for (const auto &item: customAndersen->getPts(methodIDParam)) {
        if (auto GetMethodId = jniCallsiteDummyNodes[item]) {
            if (GetMethodId->second == offset_GetMethodID) {
                const llvm::CallBase *getMethodIdCall = llvm::dyn_cast<llvm::CallBase>(
                        LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetMethodId->first));
                candidatesGetMethodId.insert(getMethodIdCall);
            }
        }
    }
    const llvm::CallBase* definingGetMethodIdCall = getImmediateDominator(&candidatesGetMethodId, callsite, tree);
    if (definingGetMethodIdCall) {
        auto getMethodId = SVFUtil::dyn_cast<SVFCallInst>(
                LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(definingGetMethodIdCall));

        cout << "yeet" << endl;

        auto paramClass = getMethodId->getArgOperand(1);
        auto paramMethodName = getMethodId->getArgOperand(2);
        auto paramMethodSig = getMethodId->getArgOperand(3);
        auto llvmParamMethodName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramMethodName);
        auto llvmParamMethodSig = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(paramMethodSig);
        const char *methodName = getStringParameterInitializer(llvmParamMethodName);
        cout << methodName << endl;
        const char *methodSig = getStringParameterInitializer(llvmParamMethodSig);
        cout << methodSig << endl;
        if (methodName && methodSig) {
            const char* className = getClassName(paramClass, callsite, tree);
            cout << "requesting return PTS for function " << methodName << endl;
            auto pts = callback_ReportArgPTSGetReturnPTS(basePTS, className, methodName, methodSig, argumentsPTS);
            returnPts->insert(pts->begin(), pts->end());
        }
    }
    return returnPts;

}


set<long>* ExtendedPAG::getPTSForField(const llvm::CallBase* getField, DominatorTree* tree) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getField);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);

    auto baseNode = pag->getValueNode(base);

    set<long>* basePTS = getPTS(baseNode);
    if (auto GetFieldId = retrieveGetFieldID(getField, tree)){
        auto paramClass = GetFieldId->getArgOperand(1);
        auto llvmParamFieldName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetFieldId->getArgOperand(2));

        const char* fieldName = getStringParameterInitializer(llvmParamFieldName);
        cout << "requesting return PTS for field " << fieldName << endl;
        const char* className = getClassName(paramClass, getField, tree);
        return callback_GetFieldPTS(basePTS, className, fieldName);
    } else {
        cout << inst->toString() << endl;
        cout << "failed to get field name for " << inst->toString() << endl;
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
void ExtendedPAG::reportSetField(const llvm::CallBase* setField, DominatorTree* tree) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(setField);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);
    auto baseNode = pag->getValueNode(base);
    set<long>* basePTS = getPTS(baseNode);

    auto arg = call->getArgOperand(3);
    auto argNode = pag->getValueNode(arg);
    set<long>* argPTS = getPTS(argNode);

    if (auto GetFieldId = retrieveGetFieldID(setField, tree)){
        auto paramClass = GetFieldId->getArgOperand(1);
        auto llvmParamFieldName = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(GetFieldId->getArgOperand(2));

        const char* fieldName = getStringParameterInitializer(llvmParamFieldName);
        const char* className = getClassName(paramClass, setField, tree);
        cout << "adding setting PTS for field " << fieldName << endl;
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
    const JNICallOffset offset_NewGlobalRef = (unsigned long) (&j.NewGlobalRef) - (unsigned long) (&j);
    const JNICallOffset offset_SetObjectArrayElement = (unsigned long) (&j.SetObjectArrayElement) - (unsigned long) (&j);
    const JNICallOffset offset_NewObject = (unsigned long) (&j.NewObject) - (unsigned long) (&j);
    const JNICallOffset offset_AllocObject = (unsigned long) (&j.AllocObject) - (unsigned long) (&j);

    auto name =  function->getName();
    if (name == "JNICustomFilter"){
        cout << "process " << name << endl;
    }
    llvm::Function* f_llvm = const_cast<Function *>(llvm::dyn_cast<llvm::Function>(
            LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(function)));
    llvm::DominatorTree* domTree = new llvm::DominatorTree(*f_llvm);
    for (const auto &item: detectedJniCalls->detectedJNICalls) {
        JNICallOffset offset = item.second;
        if (item.first->getFunction() == f_llvm) {
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            if (offset == offset_CallObjectMethod || offset == offset_CallVoidMethod) {
                auto retPTS = getReturnPTSForJNICallsite(item.first, domTree);
                NodeID callsiteNode = pag->getValueNode(inst);
                addPTS(callsiteNode, *retPTS);
            } else if (offset == offset_NewObject || offset == offset_AllocObject) {
                handleJNIAllocSite(item.first, domTree);
            } else if (offset == offset_GetObjectField) {
                set<long>* fieldPTS = getPTSForField(item.first, domTree);
                NodeID callsiteNode = pag->getValueNode(inst);
                cout << "adding " << fieldPTS->size() << " PTS to field " << endl;
                addPTS(callsiteNode, *fieldPTS);
            } else if (offset == offset_SetObjectField) {
                reportSetField(item.first, domTree);

            } else if (offset == offset_GetObjectArrayElement) {
                getPTSForArray(item.first);
                NodeID callsiteNode = pag->getValueNode(inst);
                NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
                set<NodeID> *s = new set<NodeID>();
                s->insert(dummyNode);
                additionalPTS[callsiteNode] = s;
            } else if (offset == offset_NewGlobalRef) {
                const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
                const SVFValue* obj = call->getArgOperand(1);
                cout << obj->toString()<<endl;
                auto baseNode = pag->getValueNode(obj);
                if (const SVFInstruction*  loadInst = SVFUtil::dyn_cast<SVFInstruction>(obj)) {
                    cout << loadInst->toString() << endl;
                    const Value* v = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(obj);
                    if (auto l = llvm::dyn_cast<LoadInst>(v)) {
                        auto pointer = l->getPointerOperand();
                        auto svfVal = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(pointer);
                        //baseNode = pag->getValueNode(svfVal);
                    }

                }
                NodeID callsiteNode = pag->getValueNode(inst);
                if (auto e = additionalPTS[callsiteNode]){
                    e->insert(baseNode);
                } else {
                    set<NodeID>* newset = new set<NodeID>();
                    newset->insert(baseNode);
                    additionalPTS[callsiteNode] = newset;
                }
            }
        }
    }
    delete domTree;


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
