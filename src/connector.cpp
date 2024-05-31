//
// Created by julius on 5/31/24.
//
#include "svf-ex.h"
using namespace std;
using namespace SVF;


long ExtendedPAG::getTotalSizeOfAdditionalPTS() {
    long sum = 0;
    for (const auto &pts: additionalPTS) {
        sum += pts.second->size();
    }
    return sum;
}
ExtendedPAG::ExtendedPAG(SVFModule* module, SVFIR* pag): pag(pag) {
    this->module = module;
    const llvm::Module* lm = LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule();
    detectedJniCalls = new DetectNICalls();
    for (const auto &svfFunction: *module) {
        const llvm::Function* llvmFunction = llvm::dyn_cast<llvm::Function>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfFunction));
        detectedJniCalls->processFunction(*llvmFunction);
    }
    for (const auto &call: detectedJniCalls->detectedJNICalls) {
        auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(call.first);
        NodeID callsiteNode = pag->getValueNode(inst);
        NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
        SVFCallInst* cb = SVFUtil::dyn_cast<SVFCallInst>(inst);
        jniCallsiteDummyNodes[dummyNode] = new pair<SVFCallInst*, JNICallOffset >(cb, call.second);
        set<NodeID>* s = new set<NodeID>();
        s->insert(dummyNode);
        additionalPTS[callsiteNode] = s;
    }
    updateAndersen();
}


// main entry point for native detector.
// called from java analysis when PTS for native function call args (+ base) are known
// gets PTS for return value, requests PTS from java analysis on demand
// also reports argument points to sets for all transitively called jni callsites (CallMethod + SetField)

set<long>* ExtendedPAG::processNativeFunction(const SVFFunction* function, set<long> callBasePTS, vector<set<long>> argumentsPTS) {

    // (env, this, arg0, arg1, ...)
    auto name =  function->getName();
    auto args = pag->getFunArgsList(function);
    auto baseNode = pag->getValueNode(args[1]->getValue());
    if (args.size() != argumentsPTS.size() + 2) {
        cout << "native function arg count (" << args.size() << ") != size of arguments PTS (" << argumentsPTS.size() << ") + 2 " << endl;
        assert(args.size() == argumentsPTS.size() + 2 && "invalid arg count");
    }
    addPTS(baseNode, callBasePTS);


    for (int i = 0; i < argumentsPTS.size(); i++) {
        auto argNode = pag->getValueNode(args[i + 2]->getValue());
        auto argPTS = argumentsPTS[i];
        addPTS(argNode, argPTS);
    }
    solve(function);


    NodeID retNode = pag->getReturnNode(function);
    set<long>* javaPTS = getPTS(retNode);
    customAndersen->getPTACallGraph()->dump("cg");

    return javaPTS;
}

void ExtendedPAG::handleJNIAllocSite(const llvm::CallBase* allocsite) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(allocsite);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    NodeID callsiteNode = pag->getValueNode(inst);
    if (javaAllocNodeToSVFDummyNode.find(callsiteNode) == javaAllocNodeToSVFDummyNode.end()){
        if (const char* className = getClassName(call->getArgOperand(1))) {
            long id = callback_GenerateNativeAllocSiteId(className, nullptr);
            set<long> *s = new set<long>();
            s->insert(id);
            addPTS(callsiteNode, *s);
            javaAllocNodeToSVFDummyNode[callsiteNode] = id;
            cout << "JNI alloc site  id " << id << " nodeId "
                 << callsiteNode << endl;
        }
    }
}





