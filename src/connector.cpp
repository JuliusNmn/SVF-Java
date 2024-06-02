//
// Created by julius on 5/31/24.
//
#include "main.h"
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


void ExtendedPAG::handleJNIAllocSite(const llvm::CallBase* allocsite, DominatorTree* tree) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(allocsite);
    const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(inst);
    NodeID callsiteNode = pag->getValueNode(inst);
    if (javaAllocNodeToSVFDummyNode.find(callsiteNode) == javaAllocNodeToSVFDummyNode.end()){
        const char* className = getClassName(call->getArgOperand(1), allocsite, tree);
        long id = callback_GenerateNativeAllocSiteId(className, nullptr);
        set<long> *s = new set<long>();
        s->insert(id);
        addPTS(callsiteNode, *s);
        javaAllocNodeToSVFDummyNode[callsiteNode] = id;
        cout << "JNI alloc site  id " << id << " nodeId "
             << callsiteNode << endl;
    }
}






