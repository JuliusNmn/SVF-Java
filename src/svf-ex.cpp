//===- svf-ex.cpp -- A driver example of SVF-------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===-----------------------------------------------------------------------===//

/*
 // 
 // A driver program of SVF including usages of SVF APIs
 //
 // Author: Yulei Sui, Julius Naeumann

 */
#include "jni.h"
#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "jni.h"
#include "jniheaders/svfjava_SVFJava.h"
#include "jniheaders/svfjava_Andersen.h"
#include "jniheaders/svfjava_SVFGBuilder.h"
#include "jniheaders/svfjava_SVFIRBuilder.h"
#include "jniheaders/svfjava_SVFJava.h"
#include "jniheaders/svfjava_SVFModule.h"
#include "jniheaders/svfjava_VFG.h"
#include "jniheaders/svfjava_SVFFunction.h"
#include "jniheaders/svfjava_PointsTo.h"
#include "jniheaders/svfjava_SVFVar.h"
#include "jniheaders/svfjava_SVFValue.h"
#include "jniheaders/svfjava_SVFIR.h"
#include "jniheaders/svfjava_SVFG.h"
#include "jniheaders/svfjava_VFGNode.h"
#include <iostream>

using namespace llvm;
using namespace std;
using namespace SVF;

/*
Creates a new java instance of a specified CppReference class, 
initialized with the address of the supplied pointer
*/
jobject createCppRefObject(JNIEnv *env, char *classname, const void *pointer) {
    jclass svfValueClass = env->FindClass(classname);
    jmethodID valueConstructor = env->GetMethodID(svfValueClass, "<init>", "(J)V");
    return env->NewObject(svfValueClass, valueConstructor, (jlong) pointer);
}

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
                                                llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

/*!
 * An example to query alias results of two LLVM values
 */
SVF::AliasResult aliasQuery(PointerAnalysis *pta, Value *v1, Value *v2) {
    SVFValue *val1 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v1);
    SVFValue *val2 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v2);

    return pta->alias(val1, val2);
}

/*!
 * An example to print points-to set of an LLVM value
 */
std::string printPts(PointerAnalysis *pta, SVFValue *svfval) {

    std::string str;
    raw_string_ostream rawstr(str);

    NodeID pNodeId = pta->getPAG()->getValueNode(svfval);
    const PointsTo &pts = pta->getPts(pNodeId);
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
         ii != ie; ii++) {
        rawstr << " " << *ii << " ";
        PAGNode *targetObj = pta->getPAG()->getGNode(*ii);
        if (targetObj->hasValue()) {
            rawstr << "(" << targetObj->getValue()->toString() << ")\t ";
        }
    }

    return rawstr.str();

}

// assumption: no cycles!
// if targetNode is null, try to find a path that satisfies NodeKinds
std::vector<const VFGNode *> *findBackwardsPath(const VFGNode *currentNode, const VFGNode *targetNode,
                                                const std::vector<VFGNode::VFGNodeK> *nodeKinds) {

    if (targetNode && (currentNode->getId() == targetNode->getId())) {
        // path exists.
        auto nodes = new std::vector<const VFGNode *>();
        nodes->push_back(targetNode);
        return nodes;
    }

    //cout << "Looking for a path ENDING AT " << currentNode->toString() << endl;
//    cout << " STARTING AT " << targetNode->toString() << endl;

    for (VFGEdge *edge: currentNode->getInEdges()) {
        VFGNode *predNode = edge->getSrcNode();
        //cout << "checking " << predNode->toString() << endl;
        VFGNode::GNodeK nextKind = predNode->getNodeKind();

        if (nodeKinds->at(0) == nextKind) {
            // Remove the matched kind from remainingKinds
            auto nextRemainingKinds = new std::vector<VFGNode::VFGNodeK>(nodeKinds->begin() + 1, nodeKinds->end());
            if (targetNode == nullptr && nextRemainingKinds->empty()) {
                auto nodes = new std::vector<const VFGNode *>();
                nodes->push_back(predNode);
                return nodes;
            }
            if (auto path = findBackwardsPath(predNode, targetNode, nextRemainingKinds)) {
                path->insert(path->begin(), currentNode);
                return path;
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}

const char* getStringParameterInitializer(const SVFValue* ptrParameter) {
    const GetElementPtrInst* loadStr = llvm::dyn_cast<llvm::GetElementPtrInst>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(ptrParameter));
    if (loadStr == nullptr) {
        return nullptr;
    }
    auto strVal = loadStr->getOperand(0);
    auto strConst = llvm::dyn_cast<llvm::ConstantDataArray>(strVal);
    auto strGlobal= llvm::dyn_cast<llvm::GlobalVariable>(strVal);
    auto initializer = strGlobal->getInitializer();
    ConstantDataSequential* str = llvm::dyn_cast<ConstantDataSequential >(initializer);
    StringRef s = str->getAsCString();
    return s.data();
}

/*!
 * An example to query/collect all successor nodes from a ICFGNode (iNode) along control-flow graph (ICFG)
 */
void traverseOnICFG(ICFG *icfg, const Instruction *inst) {
    SVFInstruction *svfinst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(inst);

    ICFGNode *iNode = icfg->getICFGNode(svfinst);
    FIFOWorkList<const ICFGNode *> worklist;
    Set<const ICFGNode *> visited;
    worklist.push(iNode);

    /// Traverse along VFG
    while (!worklist.empty()) {
        const ICFGNode *iNode = worklist.pop();
        for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
                iNode->OutEdgeEnd(); it != eit; ++it) {
            ICFGEdge *edge = *it;
            ICFGNode *succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end()) {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}

/*!
 * An example to query/collect all the uses of a definition of a value along value-flow graph (VFG)
 */
void traverseOnVFG(const SVFG *vfg, const SVFValue *svfval) {
    SVFIR *pag = SVFIR::getPAG();
    // SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);

    PAGNode *pNode = pag->getGNode(pag->getValueNode(svfval));
    const VFGNode *vNode = vfg->getDefSVFGNode(pNode);
    FIFOWorkList<const VFGNode *> worklist;
    Set<const VFGNode *> visited;
    worklist.push(vNode);

    /// Traverse along VFG
    while (!worklist.empty()) {
        const VFGNode *vNode = worklist.pop();
        for (VFGNode::const_iterator it = vNode->OutEdgeBegin(), eit =
                vNode->OutEdgeEnd(); it != eit; ++it) {
            VFGEdge *edge = *it;
            VFGNode *succNode = edge->getDstNode();
            VFGNode::GNodeK kind = succNode->getNodeKind();

            if (visited.find(succNode) == visited.end()) {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }

    /// Collect all LLVM Values
    for (Set<const VFGNode *>::const_iterator it = visited.begin(), eit = visited.end(); it != eit; ++it) {
        const VFGNode *node = *it;
        /// can only query VFGNode involving top-level pointers (starting with % or @ in LLVM IR)
        /// PAGNode* pNode = vfg->getLHSTopLevPtr(node);
        /// Value* val = pNode->getValue();
    }
}

/**
 * return true if instruction is
 * %4 = alloca %struct.JNINativeInterface_**, align 8 "
 */
bool isAllocaJNINativeInterface(const llvm::AllocaInst *ai) {
    static llvm::StringRef jniStructName("struct.JNINativeInterface_");
    auto t1 = ai->getAllocatedType();
    if (!t1->isPointerTy()) {
        return false;
    }
    auto t2 = t1->getContainedType(0);
    if (!t2->isPointerTy()) {
        return false;
    }
    auto t3 = t2->getContainedType(0);
    if (!t3->isStructTy()) {
        return false;
    }

    return t3->getStructName().equals(jniStructName);
}

int getOffset(const llvm::GetElementPtrInst *gepi) {
    APInt ap_offset(32, 0, false);
    gepi->accumulateConstantOffset(LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule()->getDataLayout(), ap_offset);
    int offset = ap_offset.getSExtValue();
    return offset;
}
// if the given parameter value was passed to the function as a parameter, return the parameter node.
const FormalParmVFGNode* getDefinitionIfFormalParameter(const SVFG* svfg, const SVFValue* param)  {
    auto pag = svfg->getPAG();
    auto paramDefNode = svfg->getDefSVFGNode(pag->getGNode(pag->getValueNode(param)));
    std::vector<VFGNode::VFGNodeK> loadParameterPath;
    loadParameterPath.push_back(VFGNode::VFGNodeK::Store);
    loadParameterPath.push_back(VFGNode::VFGNodeK::FParm);
    if (auto path = findBackwardsPath(paramDefNode, nullptr, &loadParameterPath)) {
        auto paramNode = path->at(1);
        return SVFUtil::dyn_cast<FormalParmVFGNode>(paramNode);
    }
    return nullptr;
}


// finds the definition for a parameter, if the parameter is the return value of a function.
const SVFCallInst *getDefinitionIfRetVal(SVFG *svfg, const SVFValue *parameter) {
    auto parameterSVGNode = svfg->getDefSVFGNode(svfg->getPAG()->getGNode(svfg->getPAG()->getValueNode(parameter)));
    std::vector<VFGNode::VFGNodeK> methodIdDefinitionPathKinds;
    methodIdDefinitionPathKinds.push_back(VFGNode::VFGNodeK::Store);
    methodIdDefinitionPathKinds.push_back(VFGNode::VFGNodeK::ARet);
    auto methodIDDefinitionPath = findBackwardsPath(parameterSVGNode, nullptr, &methodIdDefinitionPathKinds);
    if (methodIDDefinitionPath == nullptr) {
        cout << "WARNING: unable to determine MethodID!" << endl;
        return nullptr;
    }
    auto candidateGetMethodIDNode = methodIDDefinitionPath->at(1);

    auto candidateGetMethodID = candidateGetMethodIDNode->getValue();
    cout << "Candidate getmethodID: " << candidateGetMethodID->toString() << endl;
    auto callGetMethodID = SVFUtil::dyn_cast<SVFCallInst>(candidateGetMethodID);
    return callGetMethodID;
}


const SVFValue* getLoadedValue(SVFIR* pag, const SVFValue* val) {
    NodeID nodeId = pag->getValueNode(val);
    auto gnodepag = pag->getGNode(nodeId);
    for (const auto &loadEdge: gnodepag->getIncomingEdges(SVFStmt::PEDGEK::Load)) {
        cout << loadEdge->toString() << endl;
        if (auto loadStmt = SVFUtil::dyn_cast<LoadStmt>(loadEdge)) {
            auto rhs = loadStmt->getRHSVar();
            auto val = rhs->getValue();
            return val;
        }
    }
    return nullptr;
}

typedef std::function<jobject(const SVFValue*,const char *,const char *,std::vector<const SVFValue*>)> Callback;
jobject processFunction(SVFModule* module, SVFIR* pag, SVFG* svfg, VFG* vfg, const SVFFunction* func, Callback cb) {
    cout << "processing function " << func->getName() << endl;
    cout << "vfg node count: " << vfg->nodeNum << endl;
    // cursed c++ actually works
    // CALCULATE OFFSETS OF JNI FUNCTIONS IN JNI STRUCT
    JNINativeInterface_ j{};
    const long offset_GetObjectClass = (long) (&j.GetObjectClass) - (long) (&j);
    const long offset_GetMethodID = (long) (&j.GetMethodID) - (long) (&j);
    const long offset_CallVoidMethod = (long) (&j.CallVoidMethod) - (long) (&j);
    // offset for GetObjectClass: 248
    // offset for GetMethodID: 264
    // offset for CallVoidMethod: 488

    VFGNode *allocaJNInode = nullptr;
    cout << "checking for JNIStruct parameter " << endl;
    //vector<const SVFVar *> funcArgs = pag->getFunArgsList(func);
    auto vfgNodes = vfg->getVFGNodes(func);
    cout << "vfg nodes for function: " << vfgNodes.size() << endl;
    for (const auto &vfgNode: vfgNodes) {
        if (const AddrVFGNode *addrVfgNode = SVFUtil::dyn_cast<AddrVFGNode>(vfgNode)) {
            auto edge = addrVfgNode->getPAGEdge();
            if (const auto addrStmt = SVFUtil::dyn_cast<AddrStmt>(edge)) {
                auto val = addrStmt->getValue();
                auto llvmVal = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(val);
                if (const auto ai = llvm::dyn_cast<llvm::AllocaInst>(llvmVal)) {
                    if (isAllocaJNINativeInterface(ai)) {
                        allocaJNInode = vfgNode;
                        cout << " found JNIStruct parameter " << endl;
                    }
                }
            }
        }
    }
    Map<const SVFCallInst *, int> jvmCallOffsets;
    if (allocaJNInode) {
        cout << "Looking for CallVoidMethod invocations" << endl;
        for (const auto &bb: func->getBasicBlockList()) {
            for (const auto &inst: bb->getInstructionList()) {
                //cout << inst->toString() << inst->getType()->toString() << endl;
                if (const auto *call = SVFUtil::dyn_cast<SVFCallInst>(inst)) {
                    //auto callNode = icfg->getCallICFGNode(inst);
                    //cout << "found call " << call->toString() << " ICFG NodeID: " << callNode->getId() << endl;
                    const SVFValue *func = call->getCalledOperand();

                    auto pagValueNode = pag->getValueNode(func);
                    auto pagNode = pag->getGNode(pag->getValueNode(func));
                    auto geps = pag->getValueEdges(func);
                    cout << pagNode->toString() << endl;
                    auto callTargetSVFGNode = svfg->getDefSVFGNode(pagNode);
                    std::vector<VFGNode::VFGNodeK> loadOffsetFunctionFromJNI;
                    loadOffsetFunctionFromJNI.push_back(VFGNode::VFGNodeK::Gep);
                    loadOffsetFunctionFromJNI.push_back(VFGNode::VFGNodeK::Load);
                    loadOffsetFunctionFromJNI.push_back(VFGNode::VFGNodeK::Load);
                    loadOffsetFunctionFromJNI.push_back(VFGNode::VFGNodeK::Addr);
                    if (auto path = findBackwardsPath(callTargetSVFGNode, allocaJNInode,
                                                      &loadOffsetFunctionFromJNI)) {
                        //cout << path->size() << endl;
                        auto getElementPointerNode = (*path)[1];
                        auto llvmVal = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(
                                getElementPointerNode->getValue());
                        if (const auto gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(llvmVal)) {
                            int offset = getOffset(gepi);
                            cout << "offset: " << offset << endl;
                            jvmCallOffsets[call] = offset;
                        }
                    } else {
                        cout << "no path found" << endl;
                    }
                }

            }
        }
    } else {
        cout << "no jni struct found. aborting" << endl;
        return nullptr;
    }

    for (const auto &jvmCallAndOffset: jvmCallOffsets) {
        int offset = jvmCallAndOffset.second;
        auto callInst = jvmCallAndOffset.first;
        if (offset == offset_CallVoidMethod) {
            // void (JNICALL *CallVoidMethod)
            //      (JNIEnv *env, jobject obj, jmethodID methodID, ...);
            auto base = callInst->getArgOperand(1);
            auto methodID = callInst->getArgOperand(2);
            auto methodIDParam = svfg->getSVFGNode(svfg->getPAG()->getValueNode(methodID));
            cout << methodIDParam->toString() << endl;
            auto callGetMethodID = getDefinitionIfRetVal(svfg, methodID);
            if (callGetMethodID == nullptr || jvmCallOffsets[callGetMethodID] != offset_GetMethodID) {
                cout << "WARNING: unable to determine MethodID!" << endl;
                continue;
            }


            // jmethodID (JNICALL *GetMethodID)
            //      (JNIEnv *env, jclass clazz, const char *name, const char *sig);
            auto jclass = callGetMethodID->getArgOperand(1);
            auto methodNamePtr = callGetMethodID->getArgOperand(2);
            auto methodSignaturePtr = callGetMethodID->getArgOperand(3);
            auto jclassPagNode = pag->getGNode(pag->getValueNode(jclass));
            //auto jclassPagONode = pag->getGNode(pag->getObjectNode(jclass));
            //auto jclassVFGNode = svfg->getDefSVFGNode(jclassPagNode);
            // auto jclassVFGNode = svfg->getSVFGNode(svfg->getPAG()->getValueNode(jclass));
            auto callGetOrFindClass = getDefinitionIfRetVal(svfg, jclass);
            Constant *c;

            // this could be:
            // 1. an argument
            // 2. a call to FindClass()
            // 3. a call to GetObjectClass()
            // this proof of concept only handles GetObjectClass for now
            //auto candidateGetClass = jclassVFGNode->getValue();
            //auto callGetOrFindClass = SVFUtil::dyn_cast<SVFCallInst>(candidateGetClass);
            if (callGetOrFindClass == nullptr) {
                cout << "WARNING: unable to determine jclass" << endl;
                continue;
            }

            if (jvmCallOffsets[callGetOrFindClass] == offset_GetObjectClass) {
                auto jobject = callGetOrFindClass->getArgOperand(1);
                // now check where this jobject came from

                // is it a param?
                if (auto getClassBaseObjectParamNode = getDefinitionIfFormalParameter(svfg, jobject)) {
                    cout << "GetClass base object is param: " << getClassBaseObjectParamNode->toString() << endl;
                    getClassBaseObjectParamNode->getId();

                }
            } else {
                cout << "warning. findclass not supported yet, only getClass.." << endl;
            }
            cout << methodNamePtr->toString() << endl;

            auto methodName = getStringParameterInitializer(methodNamePtr);
            auto methodSignature = getStringParameterInitializer(methodSignaturePtr);

            cout << "Method name: " << methodName << endl << "Method Signature: " << methodSignature << endl;
            /*
            // now match jni callsite base object + args to passed java base object+args, then call callback
            // todo: handle all other cases. this means value returned from function (c or jni), value is base type, value was constructed etc...

            auto callsiteBaseDefinition = svfg->getDefSVFGNode(pag->getGNode(pag->getValueNode(base)));

            // definition of parameters passed to function
            // only if they were passed from java
            auto potentialParameterBase = getDefinitionIfFormalParameter(svfg, base);

            // formal parameter nodes of jni function, used for matching
            // 0st arg is JNI struct
            auto formalParamBaseObject = svfg->getFormalParmVFGNode(pag->getGNode(pag->getValueNode(func->getArg(1))));
            vector<FormalParmVFGNode*> formalParameters;
            formalParameters.push_back(formalParamBaseObject);
            for (int i = 2; i < func->arg_size(); i++){
                auto formalParamArg = svfg->getFormalParmVFGNode(pag->getGNode(pag->getValueNode(func->getArg(i))));
                formalParameters.push_back(formalParamArg);
            }

            // cout << "Callsite base, arg0" << endl;
            // cout << potentialParameterBase->toString() << endl;
            // cout << potentialParameterArg0->toString() << endl;
            // cout << "Argument nodes" << endl;
            // cout << callsiteBaseDefinition->toString() << endl;
            // cout << formalParameters.at(0)->toString() << endl;
            // cout << formalParameters.at(1)->toString() << endl;

            // match passed arguments against formal parameters

            vector<jobject> callsiteAbstractParameters;
            for (int i = 3; i < callInst->arg_size(); i++) {
                const SVFValue* arg = callInst->getArgOperand(i);
                if (auto potentialParameter = getDefinitionIfFormalParameter(svfg, arg)){
                    auto idx = std::find(formalParameters.begin(), formalParameters.end(), potentialParameter);
                    if (idx != formalParameters.end()) {
                        if (idx == formalParameters.begin()) {
                            callsiteAbstractParameters.push_back(abstractBase);
                        } else {
                            callsiteAbstractParameters.push_back(abstractArgs.at(idx - formalParameters.begin() - 1));
                        }
                    } else {
                        callsiteAbstractParameters.push_back(nullptr);
                    }
                } else {
                    cout << "warning: callsite passed parameter was not passed as argument. will be null" << endl;
                    callsiteAbstractParameters.push_back(nullptr);
                }
            }

            jobject baseJobject = nullptr;
            auto idx = std::find(formalParameters.begin(), formalParameters.end(),potentialParameterBase);
            if (idx != formalParameters.end()){
                if (idx == formalParameters.begin()){
                    baseJobject = abstractBase;
                } else {
                    baseJobject = abstractArgs.at(idx - formalParameters.begin() - 1);
                }
            }
            */
            vector<const SVFValue*> callsiteParameterNodes;
            for (int i = 3; i < callInst->arg_size(); i++) {
                const SVFValue* arg = callInst->getArgOperand(i);
                const SVFValue* argiLoaded = getLoadedValue(pag, arg);
                callsiteParameterNodes.push_back(argiLoaded ? argiLoaded : arg);

            }
            cout << "detected jni call." << endl;
            const SVFValue* baseLoaded = getLoadedValue(pag, base);
            jobject callRetVal = cb(baseLoaded ? baseLoaded :  base, methodName, methodSignature, callsiteParameterNodes);
            cout << callRetVal << endl;

        }

    }
    return nullptr;
}


int main(int argc, char **argv) {

    std::vector<std::string> moduleNameVec;
    moduleNameVec.push_back("/home/julius/SVF-Java/libnative.bc");
    if (Options::WriteAnder() == "ir_annotator") {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR *pag = builder.build();
    pag->dump("pag");
    /// Create Andersen's pointer analysis
    Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
    /// Query aliases
    /// aliasQuery(ander,value1,value2);
    /// Print points-to information
    /// printPts(ander, value1);


    /// Call Graph
    PTACallGraph *callgraph = ander->getPTACallGraph();

    /// ICFG
    ICFG *icfg = pag->getICFG();
    icfg->dump("icfg");

    /// Value-Flow Graph (VFG)
    VFG *vfg = new VFG(callgraph);
    vfg->dump("vfg");
    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFG *svfg = svfBuilder.buildFullSVFG(ander);
    svfg->dump("svfg");

    for (const SVFFunction *func: *svfModule) {
        cout << "function " << func->getName() << " argcount: " << func->arg_size() << endl;
        for (u32_t i = 0; i < func->arg_size(); i++) {
            const SVFArgument *arg = func->getArg(i);
            SVFValue *val = (SVFValue *) arg;
            cout << "arg " << i << " pts: " << printPts(ander, val) << endl;
        }
        //func->getExitBB()->getTerminator()->
        if (func->getName() == "print") {
            const SVFValue *value = func->getArg(0);
            /// Collect uses of an LLVM Value
            traverseOnVFG(svfg, value);

        }
//         std::string typeStr;
//         raw_string_ostream typeStream(typeStr);
//         type->print(typeStream);
        jobject base = new _jobject();
        std::vector<jobject> args;
        args.push_back(new _jobject());
        auto funArgs = pag->getFunArgsList(func);
        for (const auto &arg: funArgs) {
            cout << "arg " << arg->toString() << " val " << arg->getValue()->toString() << endl;

            for (const auto &edge: arg->getOutEdges()) {
                if (auto store = SVFUtil::dyn_cast<StoreStmt>(edge)) {
                    cout << "arg stored: " << edge->getValue()->toString() << endl;
                    cout << "to: " << store->getLHSVar()->toString() << endl;
                }
            }
        }
        //auto args = pag->getFunArgsList(func);
        Callback callback = [pag, ander, svfg](const SVFValue* base, const char* methodName, const char* methodSignature, std::vector<const SVFValue*> args) {
            cout << "base: " << base->toString() << endl;
            NodeID nodeId = pag->getValueNode(base);
            const PointsTo &pts = ander->getPts(nodeId);
            std::vector<PAGNode *> pointsToSet;
            for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
                 ii != ie; ii++) {
                PAGNode *target = pag->getGNode(*ii);
                cout << "base pts " << target->toString() << endl;
                pointsToSet.push_back(target);
            }
            auto svfgnode = svfg->getSVFGNode(nodeId);
            for (const auto &item: args) {
                NodeID nodeId = pag->getValueNode(item);
                const PointsTo &pts = ander->getPts(nodeId);
                for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
                     ii != ie; ii++) {
                    PAGNode *target = pag->getGNode(*ii);
                    cout << "base pts " << target->toString() << endl;
                    pointsToSet.push_back(target);
                }
            }

            return nullptr;
        };
        processFunction(svfModule, pag, svfg, vfg, func, callback);

    }

    /// Collect all successor nodes on ICFG
    /// traverseOnICFG(icfg, value);

    // clean up memory
    delete vfg;
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}



void *getCppReferencePointer(JNIEnv *env, jobject obj) {
    jclass cls = env->FindClass("svfjava/CppReference");
    jfieldID addressField = env->GetFieldID(cls, "address", "J");
    void *ref = (void *) env->GetLongField(obj, addressField);
    return ref;
}


/*
 * Class:     svfjava_SVFModule
 * Method:    processFunction
 * Signature: (Lsvfjava/SVFFunction;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFModule_processFunction
        (JNIEnv * env, jobject svfModule, jobject svfFunction, jobject base, jobjectArray args) {
    SVFModule *module = static_cast<SVFModule *>(getCppReferencePointer(env, svfModule));
    jclass svfModuleClass = env->GetObjectClass(svfModule);
    SVFFunction *func = static_cast<SVFFunction *>(getCppReferencePointer(env, svfFunction));
    jfieldID pagField = env->GetFieldID(svfModuleClass, "pag", "Lsvfjava/SVFIR;");
    SVFIR *pag = static_cast<SVFIR *>(getCppReferencePointer(env, env->GetObjectField(svfModule, pagField)));
    jfieldID vfgField = env->GetFieldID(svfModuleClass, "vfg", "Lsvfjava/VFG;");
    VFG *vfg = static_cast<VFG *>(getCppReferencePointer(env, env->GetObjectField(svfModule, vfgField)));
    jfieldID svfgField = env->GetFieldID(svfModuleClass, "svfg", "Lsvfjava/SVFG;");
    SVFG *svfg = static_cast<SVFG *>(getCppReferencePointer(env, env->GetObjectField(svfModule, svfgField)));

    jobject listener = env->GetObjectField(svfModule, env->GetFieldID(svfModuleClass, "listener", "Lsvfjava/SVFAnalysisListener;"));
    jmethodID listenerCallback = env->GetMethodID(env->GetObjectClass(listener), "nativeToJavaCallDetected", "(Lsvfjava/SVFValue;Ljava/lang/String;Ljava/lang/String;[Lsvfjava/SVFValue;)Ljava/lang/Object;");
    assert(listenerCallback);
    Callback callback = [env, listener, listenerCallback](const SVFValue* base, const char* methodName, const char* methodSignature, std::vector<const SVFValue*> args) {
        cout << "calling java svf listener. detected method call to " << methodName << " signature " << methodSignature << endl;
        // Convert char* to Java strings
        jstring jMethodName = env->NewStringUTF(methodName);
        jstring jMethodSignature = env->NewStringUTF(methodSignature);

        // Convert std::vector to jobjectArray
        jobjectArray jArgs = env->NewObjectArray(args.size(), env->FindClass("java/lang/Object"), nullptr);
        for (int i = 0; i < args.size(); ++i) {
            jobject jargval = createCppRefObject(env, "svfjava/SVFValue", args[i]);
            env->SetObjectArrayElement(jArgs, i, jargval);
        }
        jobject jbaseVal = createCppRefObject(env, "svfjava/SVFValue", base);
        // Call the Java callback method
        jobject retVal = env->CallObjectMethod(listener, listenerCallback, jbaseVal, jMethodName, jMethodSignature, jArgs);

        // Release local references
        env->DeleteLocalRef(jMethodName);
        env->DeleteLocalRef(jMethodSignature);
        env->DeleteLocalRef(jArgs);

        return retVal;
    };

    std::vector<jobject> argsV;
    assert(module);
    assert(func);
    assert(pag);
    assert(vfg);
    assert(svfg);
    assert(svfModuleClass);
    assert(pagField);
    assert(vfgField);
    assert(svfgField);
    return processFunction(module, pag, svfg, vfg, func, callback);

}
/*
 * Class:     svfjava_SVFJava
 * Method:    runmain
 * Signature: ([Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_svfjava_SVFJava_runmain
        (JNIEnv *env, jobject jthis, jobjectArray args) {
    // thank god for chatGPT

    printf("calling main\n");
    // Get the number of arguments in the jobjectArray
    jsize argc = env->GetArrayLength(args);

    // Create a C array of strings to hold the arguments
    char *argv[argc];

    // Populate the argv array with argument values from jobjectArray
    for (jsize i = 0; i < argc; i++) {
        jstring arg = static_cast<jstring>(env->GetObjectArrayElement(args, i));
        const char *argStr = env->GetStringUTFChars(arg, 0);
        argv[i] = strdup(argStr); // Allocate memory for each argument
        env->ReleaseStringUTFChars(arg, argStr);
        env->DeleteLocalRef(arg); // Release the local reference
    }


    // Call the main function with argc and argv
    main(argc, argv);

    // Clean up allocated memory
    for (jsize i = 0; i < argc; i++) {
        free(argv[i]);
    }
}
/*
 * Class:     svfjava_SVFModule
 * Method:    createSVFModule
 * Signature: (Ljava/lang/String;)Lsvfjava/SVFModule;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFModule_createSVFModule(JNIEnv *env, jclass cls, jstring moduleName) {
    const char *moduleNameStr = env->GetStringUTFChars(moduleName, NULL);
    std::vector<std::string> moduleNameVec;
    moduleNameVec.push_back(moduleNameStr);

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    jclass svfModuleClass = env->FindClass("svfjava/SVFModule");
    jmethodID constructor = env->GetMethodID(svfModuleClass, "<init>", "(J)V");
    jobject svfModuleObject = env->NewObject(svfModuleClass, constructor, (jlong) svfModule);

    env->ReleaseStringUTFChars(moduleName, moduleNameStr);

    return svfModuleObject;
}

/*
 * Class:     svfjava_SVFIRBuilder
 * Method:    create
 * Signature: (Lsvfjava/SVFModule;)Lsvfjava/SVFIRBuilder;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIRBuilder_create(JNIEnv *env, jclass cls, jobject module) {
    SVFModule *svfModule = (SVFModule *) getCppReferencePointer(env, module);
    SVFIRBuilder *builder = new SVFIRBuilder(svfModule);

    jclass builderClass = env->FindClass("svfjava/SVFIRBuilder");
    jmethodID constructor = env->GetMethodID(builderClass, "<init>", "(J)V");
    jobject builderObject = env->NewObject(builderClass, constructor, (jlong) builder);

    return builderObject;
}

/*
 * Class:     svfjava_SVFIRBuilder
 * Method:    build
 * Signature: ()Lsvfjava/SVFIR;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIRBuilder_build(JNIEnv *env, jobject obj) {
    SVFIRBuilder *builder = (SVFIRBuilder *) getCppReferencePointer(env, obj);

    SVFIR *ir = builder->build();

    jclass irClass = env->FindClass("svfjava/SVFIR");
    jmethodID constructor = env->GetMethodID(irClass, "<init>", "(J)V");
    jobject irObject = env->NewObject(irClass, constructor, (jlong) ir);

    return irObject;
}

/*
 * Class:     svfjava_Andersen
 * Method:    create
 * Signature: (Lsvfjava/SVFIR;)Lsvfjava/Andersen;
 */
JNIEXPORT jobject JNICALL Java_svfjava_Andersen_create(JNIEnv *env, jclass cls, jobject irObj) {
    SVFIR *ir = (SVFIR *) getCppReferencePointer(env, irObj);

    Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(ir);

    jclass anderClass = env->FindClass("svfjava/Andersen");
    jmethodID constructor = env->GetMethodID(anderClass, "<init>", "(J)V");
    jobject anderObject = env->NewObject(anderClass, constructor, (jlong) ander);

    return anderObject;
}

/*
 * Class:     svfjava_Andersen
 * Method:    getPTACallGraph
 * Signature: ()Lsvfjava/PTACallGraph;
 */
JNIEXPORT jobject JNICALL Java_svfjava_Andersen_getPTACallGraph(JNIEnv *env, jobject obj) {
    Andersen *ander = (Andersen *) getCppReferencePointer(env, obj);

    PTACallGraph *callgraph = ander->getPTACallGraph();

    jclass callgraphClass = env->FindClass("svfjava/CppReference");
    jmethodID constructor = env->GetMethodID(callgraphClass, "<init>", "(J)V");
    jobject callgraphObject = env->NewObject(callgraphClass, constructor, (jlong) callgraph);

    return callgraphObject;
}
/*
 * Class:     svfjava_VFG
 * Method:    create
 * Signature: (Lsvfjava/PTACallGraph;)Lsvfjava/VFG;
 */
JNIEXPORT jobject JNICALL Java_svfjava_VFG_create
        (JNIEnv *env, jclass cls, jobject cg) {
    PTACallGraph *callgraph = (PTACallGraph *) getCppReferencePointer(env, cg);
    VFG *vfg = new VFG(callgraph);
    jclass vfgClass = env->FindClass("svfjava/VFG");
    jmethodID constructor = env->GetMethodID(vfgClass, "<init>", "(J)V");
    jobject o = env->NewObject(vfgClass, constructor, (jlong) vfg);
    return o;
}
/*
 * Class:     svfjava_SVFGBuilder
 * Method:    create
 * Signature: ()Lsvfjava/SVFGBuilder;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFGBuilder_create(JNIEnv *env, jclass cls) {

    SVFGBuilder *b = new SVFGBuilder();
    jmethodID constructor = env->GetMethodID(cls, "<init>", "(J)V");
    jobject o = env->NewObject(cls, constructor, (jlong) b);

    return o;
}
/*
 * Class:     svfjava_SVFGBuilder
 * Method:    buildFullSVFG
 * Signature: (Lsvfjava/Andersen;)Lsvfjava/SVFG;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFGBuilder_buildFullSVFG(JNIEnv *env, jobject jthis, jobject obj) {
    SVFGBuilder *builder = (SVFGBuilder *) getCppReferencePointer(env, jthis);
    Andersen *ander = (Andersen *) getCppReferencePointer(env, obj);

    SVFG *svfg = builder->buildFullSVFG(ander);

    jclass svfgClass = env->FindClass("svfjava/SVFG");
    jmethodID constructor = env->GetMethodID(svfgClass, "<init>", "(J)V");
    jobject svfgObject = env->NewObject(svfgClass, constructor, (jlong) svfg);

    return svfgObject;
}
/*
 * Class:     svfjava_SVFG
 * Method:    dump
 * Signature: (Ljava/lang/String)V;
 */
JNIEXPORT void JNICALL Java_svfjava_SVFG_dump(JNIEnv *env, jobject jthis, jstring filename) {
    SVFG *graph = (SVFG *) getCppReferencePointer(env, jthis);

    const char *argStr = env->GetStringUTFChars(filename, 0);
    std::string s(argStr);
    graph->dump(s);
    env->ReleaseStringUTFChars(filename, argStr);
    env->DeleteLocalRef(filename);
}
/*
 * Class:     svfjava_SVFModule
 * Method:    getFunctions
 * Signature: ()[Lsvfjava/SVFFunction;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFModule_getFunctions(JNIEnv *env, jobject obj) {
    SVFModule *svfModule = (SVFModule *) getCppReferencePointer(env, obj);

    std::vector<const SVFFunction *> functions = svfModule->getFunctionSet();

    jclass svfFunctionClass = env->FindClass("svfjava/SVFFunction");
    jmethodID constructor = env->GetMethodID(svfFunctionClass, "<init>", "(JLjava/lang/String;)V");

    jobjectArray functionArray = env->NewObjectArray(functions.size(), svfFunctionClass, nullptr);

    for (size_t i = 0; i < functions.size(); ++i) {
    jstring jName = env->NewStringUTF(functions[i]->getName().c_str());
    cout << functions[i]->getName();
    jobject svfFunctionObject = env->NewObject(svfFunctionClass, constructor, (jlong) functions[i], (jstring) jName);
    env->DeleteLocalRef(jName);
    env->SetObjectArrayElement(functionArray, i, svfFunctionObject);
    }

    return functionArray;
}
/*
 * Class:     svfjava_SVFFunction
 * Method:    getArgumentsNative
 * Signature: ()[Lsvfjava/SVFArgument;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFFunction_getArgumentsNative(JNIEnv *env, jobject obj) {
    SVFFunction *svfFunction = (SVFFunction *) getCppReferencePointer(env, obj);

    jclass svfArgumentClass = env->FindClass("svfjava/SVFArgument");
    jmethodID constructor = env->GetMethodID(svfArgumentClass, "<init>", "(J)V");

    jobjectArray argumentArray = env->NewObjectArray(svfFunction->arg_size(), svfArgumentClass, nullptr);

    for (size_t i = 0; i < svfFunction->arg_size(); ++i) {
        const SVFArgument *arg = svfFunction->getArg(i);
        jobject svfArgumentObject = env->NewObject(svfArgumentClass, constructor, (jlong) arg);

        env->SetObjectArrayElement(argumentArray, i, svfArgumentObject);
    }

    return argumentArray;
}
/*
 * Class:     svfjava_SVFIR
 * Method:    getArgumentValues
 * Signature: (Lsvfjava/SVFFunction;)[Lsvfjava/SVFValue;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFIR_getArgumentValues
        (JNIEnv * env, jobject jthis, jobject jfunc){

    SVFIR* pag = (SVFIR *) getCppReferencePointer(env, jthis);
    SVFFunction *svfFunction = (SVFFunction *) getCppReferencePointer(env, jfunc);

    jclass svfValueClass = env->FindClass("svfjava/SVFValue");

    auto funArgs = pag->getFunArgsList(svfFunction);

    jobjectArray argumentArray = env->NewObjectArray(funArgs.size(), svfValueClass, nullptr);
    for (int i = 0; i < funArgs.size(); i++) {
        const auto &arg = funArgs.at(i);
        cout << "arg " << arg->toString() << " val " << arg->getValue()->toString() << endl;
        for (const auto &edge: arg->getOutEdges()) {
            if (auto store = SVFUtil::dyn_cast<StoreStmt>(edge)) {
                cout << "arg stored: " << edge->getValue()->toString() << endl;
                cout << "to: " << store->getLHSVar()->toString() << endl;
                auto val = store->getLHSVar()->getValue();
                jobject svfArgumentObject = createCppRefObject(env, "svfjava/SVFValue", val);
                env->SetObjectArrayElement(argumentArray, i, svfArgumentObject);
                break;
            }
        }

    }
    cout << "done" << endl;
    return argumentArray;
}

/*
 * Class:     svfjava_PointsTo
 * Method:    toStringNative
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_svfjava_PointsTo_toStringNative
        (JNIEnv *env, jobject jthis) {
    PointsTo *pt = (PointsTo *) getCppReferencePointer(env, jthis);
    return nullptr;
}
/*
 * Class:     svfjava_VFGNode
 * Method:    toStringNative
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_svfjava_VFGNode_toStringNative
        (JNIEnv *env, jobject jthis) {
    VFGNode* t = (VFGNode *) getCppReferencePointer(env, jthis);
    jstring str = env->NewStringUTF(t->toString().c_str());
    return str;
}
/*
 * Class:     svfjava_SVFValue
 * Method:    toStringNative
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_svfjava_SVFValue_toStringNative
        (JNIEnv *env, jobject jthis) {
    SVFValue *v = (SVFValue *) getCppReferencePointer(env, jthis);
    std::string s = v->toString();
    const char *c = s.c_str();
    jstring moduleNameStr = env->NewStringUTF(c);
    return moduleNameStr;
}


/*
 * Class:     svfjava_SVFIR
 * Method:    getGNode
 * Signature: (Lsvfjava/PointsTo;)Lsvfjava/SVFVar;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIR_getGNode
        (JNIEnv *env, jobject jthis, jobject jpt) {
    // SVFIR* ir = (SVFIR*)getCppReferencePointer(env, jthis);
    // PointsTo* pt = (PointsTo*)getCppReferencePointer(env, jpt);
    // NodeID nodeID = ir->getValueNode(svfval);
    // PAGNode* node = ir->getGNode(nodeID);
    // return createCppRefObject(env, "svfjava/SVFVar", node);
}
/*
 * Class:     svfjava_SVFIR
 * Method:    getValueNode
 * Signature: (Lsvfjava/SVFValue;)Lsvfjava/NodeID;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIR_getValueNode
        (JNIEnv *env, jobject jthis, jobject val) {
    SVFIR *ir = (SVFIR *) getCppReferencePointer(env, jthis);
    SVFValue *svfVal = (SVFValue *) getCppReferencePointer(env, val);
    NodeID nodeId = ir->getValueNode(svfVal);

}

/*
 * Class:     svfjava_Andersen
 * Method:    getPTS
 * Signature: (Lsvfjava/SVFIR;Lsvfjava/SVFValue;)[Lsvfjava/SVFVar;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_Andersen_getPTS(JNIEnv *env, jobject jthis, jobject jir, jobject jval) {
    Andersen *andersen = (Andersen *) getCppReferencePointer(env, jthis);
    SVFIR *ir = (SVFIR *) getCppReferencePointer(env, jir);
    SVFValue *svfVal = (SVFValue *) getCppReferencePointer(env, jval);
    NodeID nodeId = ir->getValueNode(svfVal);
    const PointsTo &pts = andersen->getPts(nodeId);
    std::vector<PAGNode *> pointsToSet;
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
         ii != ie; ii++) {
        PAGNode *target = ir->getGNode(*ii);
        pointsToSet.push_back(target);
    }

    jclass pointsToClass = env->FindClass("svfjava/SVFVar");

    jobjectArray resultArray = env->NewObjectArray(pointsToSet.size(), pointsToClass, nullptr);

    for (size_t i = 0; i < pointsToSet.size(); ++i) {
        jobject target = createCppRefObject(env, "svfjava/SVFVar", pointsToSet[i]);
        env->SetObjectArrayElement(resultArray, i, target);
        env->DeleteLocalRef(target);
    }

    // Return the jobjectArray
    return resultArray;
}

/*
 * Class:     svfjava_SVFVar
 * Method:    getValue
 * Signature: ()Lsvfjava/SVFValue;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFVar_getValue
        (JNIEnv *env, jobject jthis) {
    SVFVar *v = (SVFVar *) getCppReferencePointer(env, jthis);
    const SVFValue *val = v->getValue();
    return createCppRefObject(env, "svfjava/SVFValue", val);
}
/*
 * Class:     svfjava_SVFG
 * Method:    getFormalParameter
 * Signature: (Lsvfjava/SVFValue;)Lsvfjava/VFGNode;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFG_getFormalParameter
        (JNIEnv * env, jobject jthis, jobject jval){
    SVFValue* val = (SVFValue*) getCppReferencePointer(env, jval);
    SVFG* svfg = (SVFG*) getCppReferencePointer(env, jthis);

    if (VFGNode* node = (VFGNode*)getDefinitionIfFormalParameter(svfg, val)) {
        jobject jnode = createCppRefObject(env, "svfjava/VFGNode", node);
        return jnode;
    }
    return nullptr;

}

/*
 * Class:     svfjava_SVFG
 * Method:    getFunctionFormalParameter
 * Signature: (Lsvfjava/SVFFunction;I)Lsvfjava/VFGNode;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFG_getFunctionFormalParameter
        (JNIEnv * env, jobject jthis, jobject jfunc, jint index){

    SVFFunction* func = (SVFFunction*) getCppReferencePointer(env, jfunc);
    SVFG* svfg = (SVFG*) getCppReferencePointer(env, jthis);
    SVFIR* pag = svfg->getPAG();
    if (VFGNode* node = (VFGNode*)svfg->getFormalParmVFGNode(pag->getGNode(pag->getValueNode(func->getArg(index))))) {
        jobject jnode = createCppRefObject(env, "svfjava/VFGNode", node);
        return jnode;
    }
    return nullptr;
}
