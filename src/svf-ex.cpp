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
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "detectJNICalls.h"
#include "CustomAndersen.h"
#include <iostream>
#include "svf-ex.h"


using namespace llvm;
using namespace std;
using namespace SVF;

// processMethod transitively!!

// this class extends the PAG with special dummy alloc nodes
// dummy nodes can be added to the initial points to sets of other nodes (such as native function arguments & return values)
// Andersen's analysis is performed on demand if initial PTS are changed, new nodes will be propagated.

// dummy nodes are added  for each java alloc site returned from the java analysis.
// a mapping between java allocsite id and SVF NodeID is kept
// dummy nodes are also created during preprocessing:
// 1. at each jni callsite (CallObjectMethod etc...)
//    - if a PTS requested from java contains one of these nodes, the java analysis will be queried
//    - for each java alloc site returned from the analysis, a dummy node is created & added to PTS of callsite
// 2. at each jni alloc site (NewObject)
//    - the java analysis will be queried for an alloc site id to be used.

void ExtendedPAG::addPTS(NodeID node, set<long> customNodes) {
        set<NodeID>* existingSet = additionalPTS[node];
        if (!existingSet) {
            existingSet = new set<NodeID>();
            additionalPTS[node] = existingSet;
            refreshAndersen = true;
        }
        for (const long customNode: customNodes) {
            NodeID pagNode = createOrAddDummyNode(customNode);
            if (existingSet->insert(pagNode).second) {
                refreshAndersen = true;
            }
        }
    }


    NodeID ExtendedPAG::createOrAddDummyNode(long customId) {
        if (NodeID id = javaAllocNodeToSVFDummyNode[customId]){
            return id;
        }
        NodeID newId = pag->addDummyObjNode(SVFType::getSVFPtrType());
        javaAllocNodeToSVFDummyNode[customId] = newId;
        pagDummyNodeToJavaAllocNode[newId] = customId;
        return newId;
    }
    CustomAndersen* customAndersen;

    void ExtendedPAG::updateAndersen() {
        outs() << "running andersen's analysis with " << additionalPTS.size() << " additional Points-to edges" << ENDL;
        customAndersen = new CustomAndersen(pag, &additionalPTS);
        customAndersen->disablePrintStat();
        customAndersen->analyze();
        refreshAndersen = false;
    }

    // get PTS for return node / callsite param (+ base) node
    set<long>* ExtendedPAG::getPTS(NodeID node, const JNIInvocation* requestCallsite) {
        auto pts = customAndersen->getPts(node);
        set<long>* javaAllocPTS = new set<long>();
        for (const NodeID allocNode: pts) {
            if (JNIInvocation* jniCallsite = jniCallsiteDummyNodes[allocNode]) {
                // PTS includes return value of Java function. request it
                // problem:
                // bufObj = ...
                // bufObj = CallObjectMethod(env, bufObj)
                if (jniCallsite != requestCallsite) {
                    set<long> *jniCallsitePTS = getReturnPTSForJNICallsite(jniCallsite);
                    javaAllocPTS->insert(jniCallsitePTS->begin(), jniCallsitePTS->end());
                    delete jniCallsitePTS;
                }
            } else if (long javaAllocNode = pagDummyNodeToJavaAllocNode[allocNode]){
                javaAllocPTS->insert(javaAllocNode);
            } else if (JNIGetField* jniGetField = jniGetFieldDummyNodes[allocNode]) {
                // PTS includes return value of GetField. request it
                set<long>* fieldPTS = getPTSForField(jniGetField);
                javaAllocPTS->insert(fieldPTS->begin(), fieldPTS->end());
                delete fieldPTS;
            } else if (JNIGetArrayElement* jniGetArrElement = jniGetArrElementDummyNodes[allocNode]) {
                // PTS includes return value of GetArrayElement. request it
                set<long>* fieldPTS = getPTSForArray(jniGetArrElement);
                javaAllocPTS->insert(fieldPTS->begin(), fieldPTS->end());
                delete fieldPTS;
            }
        }
        return javaAllocPTS;
    }

ExtendedPAG::ExtendedPAG(SVFModule* module, SVFIR* pag): pag(pag) {
        this->module = module;
        const llvm::Module* lm = LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule();
        detectedJniCalls = new DetectNICalls();
        updateAndersen();
        for (const auto &svfFunction: *module) {
            const llvm::Function* llvmFunction = llvm::dyn_cast<llvm::Function>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfFunction));
            detectedJniCalls->processFunction(*llvmFunction);
        }
        // for each detected jni callsite, add a dummynode and register it
        outs() << "detected " << detectedJniCalls->detectedJNIInvocations.size() << " JNI invoke sites" << ENDL;
        for (const auto &item: detectedJniCalls->detectedJNIInvocations) {
            outs() << "JNI invoke site " << item.second->className << " " << item.second->methodName << ENDL;
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            NodeID callsiteNode = pag->getValueNode(inst);
            NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
            jniCallsiteDummyNodes[dummyNode] = item.second;
            set<NodeID>* s = new set<NodeID>();
            s->insert(dummyNode);
            additionalPTS[callsiteNode] = s;
        }
        outs() << " detected " << detectedJniCalls->detectedJNIAllocations.size() << " JNI alloc sites" << ENDL;
        for (const auto &item: detectedJniCalls->detectedJNIAllocations) {
            outs() << "JNI alloc site " << item.second->className << ENDL;

        }

        outs() << "detected " << detectedJniCalls->detectedJNIFieldGets.size() << " JNI GetFields" << ENDL;
        for (const auto &item: detectedJniCalls->detectedJNIFieldGets) {
            outs() << "JNI GetField  site " << item.second->className << " " << item.second->fieldName << ENDL;
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            NodeID callsiteNode = pag->getValueNode(inst);
            NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
            jniGetFieldDummyNodes[dummyNode] = item.second;
            set<NodeID>* s = new set<NodeID>();
            s->insert(dummyNode);
            additionalPTS[callsiteNode] = s;
        }

        outs() << "detected " << detectedJniCalls->detectedJNIFieldSets.size() << " JNI SetFields" << ENDL;

        outs() << "detected " << detectedJniCalls->detectedJNIArrayElementGets.size() << " JNI GetArrayElements" << ENDL;
        for (const auto &item: detectedJniCalls->detectedJNIArrayElementGets) {
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            NodeID callsiteNode = pag->getValueNode(inst);
            NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
            jniGetArrElementDummyNodes[dummyNode] = item.second;
            set<NodeID>* s = new set<NodeID>();
            s->insert(dummyNode);
            additionalPTS[callsiteNode] = s;
        }

    }


    // main entry point for native detector.
    // called from java analysis when PTS for native function call args (+ base) are known
    // gets PTS for return value, requests PTS from java analysis on demand
    // also reports argument points to sets for all transitively called jni callsites (CallMethod + SetField)


    set<long>* ExtendedPAG::processNativeFunction(const SVFFunction* function, set<long> callBasePTS, vector<set<long>> argumentsPTS) {

        // todo: add allocsites as they are discovered, using Points-to-analysis for class names etc
        if (pagDummyNodeToJavaAllocNode.empty()) {
            for (const auto &item: detectedJniCalls->detectedJNIAllocations) {
                auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
                NodeID callsiteNode = pag->getValueNode(inst);
                long id = callback_GenerateNativeAllocSiteId(item.second->className, nullptr);
                NodeID allocSiteDummyNode = createOrAddDummyNode(id);
                set<NodeID> *s = new set<NodeID>();
                s->insert(allocSiteDummyNode);
                additionalPTS[callsiteNode] = s;
                pagDummyNodeToJavaAllocNode[callsiteNode] = id;
                outs() << "JNI alloc site " << item.second->className << " id " << id << " nodeId " << allocSiteDummyNode << ENDL;
            }
        }


        // (env, this, arg0, arg1, ...)
        auto name =  function->getName();
        outs() << "ExtendedPAG::processNativeFunction " << name << ENDL;
        auto args = pag->getFunArgsList(function);
        auto baseNode = pag->getValueNode(args[1]->getValue());
        if (args.size() != argumentsPTS.size() + 2) {
            outs() << "native function arg count (" << args.size() << ") != size of arguments PTS (" << argumentsPTS.size() << ") + 2 " << ENDL;

            assert(args.size() == argumentsPTS.size() + 2 && "invalid arg count");
        }
        addPTS(baseNode, callBasePTS);


        for (int i = 0; i < argumentsPTS.size(); i++) {
            auto argNode = pag->getValueNode(args[i + 2]->getValue());
            auto argPTS = argumentsPTS[i];
            addPTS(argNode, argPTS);
        }

        // only run andersen once per query.
        updateAndersen();

        NodeID retNode = pag->getReturnNode(function);
        set<long>* javaPTS = getPTS(retNode);
        customAndersen->getPTACallGraph()->dump("cg");
        set<const SVFFunction*> processed;
        reportArgAndFieldWritePTS(function, &processed);

        return javaPTS;
    }

    // this reports callsite arguments of all jni callsites and field sets to the java analysis
    // transitively (for the passed function plus any called functions)!
    void ExtendedPAG::reportArgAndFieldWritePTS(const SVFFunction* function, set<const SVFFunction*>* processed) {
        auto name =  function->getName();
        if (name == "JNICustomFilter"){
            outs() << "process " << name << ENDL;
        }
        const llvm::Value* f_llvm = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(function);

        for (const auto &item: detectedJniCalls->detectedJNIInvocations) {
            if (item.first->getFunction() == f_llvm) {
                getReturnPTSForJNICallsite(item.second);
            }
        }
        for (const auto &item: detectedJniCalls->detectedJNIFieldSets) {
            if (item.first->getFunction() == f_llvm) {
                reportSetField(item.second);
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
                    reportArgAndFieldWritePTS(calledFunction, processed);
                }


        }
    }

    set<long>* ExtendedPAG::getReturnPTSForJNICallsite(const JNIInvocation* jniInv) {
        SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(jniInv->callSite);
        const SVFCallInst* call = dyn_cast<SVFCallInst>(inst);
        // this is a jni call. now get base + args PTS and perform callback.
        auto base = call->getArgOperand(1);
        auto baseNode = pag->getValueNode(base);

        set<long>* basePTS = getPTS(baseNode, jniInv);

        std::vector<std::set<long>*> argumentsPTS;
        for (int i = 3; i < call->getNumArgOperands(); i++){
            auto arg = call->getArgOperand(i);
            auto argNode = pag->getValueNode(arg);
            set<long>* argPTS = getPTS(argNode);
            argumentsPTS.push_back(argPTS);
        }
        outs() << "requesting return PTS for function " << jniInv->methodName << ENDL;
        return callback_ReportArgPTSGetReturnPTS(basePTS, jniInv->className, jniInv->methodName, jniInv->methodSignature, argumentsPTS);

    }
    set<long>* ExtendedPAG::getPTSForField(const JNIGetField* getField) {
        SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getField->callSite);
        const SVFCallInst* call = dyn_cast<SVFCallInst>(inst);
        // this is a jni call. now get base + args PTS and perform callback.
        auto base = call->getArgOperand(1);

        auto baseNode = pag->getValueNode(base);

        set<long>* basePTS = getPTS(baseNode);


        outs() << "requesting return PTS for field " << getField->fieldName << ENDL;
        return callback_GetFieldPTS(basePTS, getField->className, getField->fieldName);

    }
set<long>* ExtendedPAG::getPTSForArray(const JNIGetArrayElement* getField) {
    SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(getField->callSite);
    const SVFCallInst* call = dyn_cast<SVFCallInst>(inst);
    // this is a jni call. now get base + args PTS and perform callback.
    auto base = call->getArgOperand(1);

    auto baseNode = pag->getValueNode(base);

    set<long>* basePTS = getPTS(baseNode);


    outs() << "requesting return PTS for array. pts size " << basePTS->size() << ENDL;
    return callback_GetArrayElementPTS(basePTS);

}
    void ExtendedPAG::reportSetField(const JNISetField* setField) {
        SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(setField->callSite);
        const SVFCallInst* call = dyn_cast<SVFCallInst>(inst);
        // this is a jni call. now get base + args PTS and perform callback.
        auto base = call->getArgOperand(1);
        auto baseNode = pag->getValueNode(base);
        set<long>* basePTS = getPTS(baseNode);

        auto arg = call->getArgOperand(3);
        auto argNode = pag->getValueNode(arg);
        set<long>* argPTS = getPTS(argNode);

        outs() << "reporting SetField for " << setField->fieldName << ENDL;
        callback_SetFieldPTS(basePTS, setField->className, setField->fieldName, argPTS);

    }





int main(int argc, char **argv) {

    std::vector<std::string> moduleNameVec;
    if (argc != 2) {
        outs() << "usage: " << argv[0] << "path_to_module(.bc|.ll)" << ENDL;
        return 1;
    }
    moduleNameVec.push_back(argv[1]);
    if (Options::WriteAnder() == "ir_annotator") {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }
    outs() << "EXTAPI path: " << SVF_INSTALL_EXTAPI_FILE << ENDL;
    if (strlen(SVF_INSTALL_EXTAPI_FILE))
        ExtAPI::getExtAPI()->setExtBcPath(SVF_INSTALL_EXTAPI_FILE);

    SVFModule *svfModule = LLVMModuleSet::buildSVFModule(moduleNameVec);

    auto lm = LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule();


    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR *pag = builder.build();

    pag->dump("pag");
    /// Create Andersen's pointer analysis
    //Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    Andersen* ander = new AndersenWaveDiff(pag, SVF::Andersen::AndersenWaveDiff_WPA, false);
    ander->disablePrintStat();
    ander->analyze();

    ander->getConstraintGraph()->dump("cg");
    /// Query aliases
    /// aliasQuery(ander,value1,value2);
    /// Print points-to information
    /// printPts(ander, value1);

    for (auto &function: svfModule->getFunctionSet()) {

        NodeID returnNode = pag->getReturnNode(function);
        auto pts = ander->getPts(returnNode);
        outs() << "return value pts of function " << function->toString() << ENDL << " size: " << pts.count() << ENDL;
        for (const auto &nodeId: pts) {
            auto node = pag->getGNode(nodeId);
            outs() << node->toString() << ENDL;
        }

    }

    /// Call Graph
    // PTACallGraph *callgraph = ander->getPTACallGraph();
    /// ICFG
    // ICFG *icfg = pag->getICFG();
    // icfg->dump("icfg");

    /// Value-Flow Graph (VFG)
    // VFG *vfg = new VFG(callgraph);
    // vfg->dump("vfg");
    /// Sparse value-flow graph (SVFG)
    // SVFGBuilder svfBuilder;
    // SVFG *svfg = svfBuilder.buildFullSVFG(ander);
    // svfg->dump("svfg");
    // outs() << ENDL;
    CB_SetArgGetReturnPTS cb1 = [](set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS) {
        //  make function return this
        outs() << "function called " << methodName << ENDL;
        return callBasePTS;
    };
    CB_GenerateNativeAllocSiteId  cb2 = [](const char* className, const char* context) {
        return 101;
    };
    CB_GetFieldPTS cb3 = [](set<long>* callBasePTS, const char * className, const char * fieldName) {
        outs() << "GetField " << fieldName << ENDL;
        return callBasePTS;
    };
    CB_SetFieldPTS cb4 = [] (set<long>* callBasePTS, const char * className, const char * fieldName, set<long>* argPTS) {
        outs() << "SetField " << argPTS << ENDL;
    };
    CB_GetArrayElementPTS cb5 = [] (set<long>* arrayPTS) {
        outs() << "GetArrayElement " << arrayPTS << ENDL;
        return arrayPTS;
    };
    ExtendedPAG* e = new ExtendedPAG(svfModule, pag);

    e->callback_ReportArgPTSGetReturnPTS = cb1;
    e->callback_GenerateNativeAllocSiteId = cb2;
    e->callback_GetFieldPTS = cb3;
    e->callback_SetFieldPTS = cb4;
    e->callback_GetArrayElementPTS = cb5;
    int id = 0;
    auto func = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CallJavaFunctionFromNativeAndReturn_callMyJavaFunctionFromNativeAndReturn");

    set<long> callbasePTS;

    callbasePTS.insert(666);
    for (const auto &xxx: svfModule->getFunctionSet()) {
        //auto function = svfModule->getSVFFunction("Java_org_libjpegturbo_turbojpeg_TJTransformer_transform");
        auto function = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_intraprocedural_unidirectional_NativeIdentityFunction_identity");
        //auto function = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_intraprocedural_bidirectional_CallJavaFunctionFromNative_callMyJavaFunctionFromNative");

        if (function->getName().find("Java_") == 0) {
            outs() << function->getName() << ENDL;
            vector<set<long>> argumentsPTS1;

            for (int i = 0; i < function->arg_size() - 2; i++) {
                set<long> arg1PTS1;
                arg1PTS1.insert(1000 + i);
                argumentsPTS1.push_back(arg1PTS1);
            }
            auto pts = e->processNativeFunction(function, callbasePTS, argumentsPTS1);

            outs() << pts->size() << ENDL;
        }

     }
    if (func) {
        vector<set<long>> argumentsPTS1;
        set<long> arg1PTS1;
        arg1PTS1.insert(909);
        argumentsPTS1.push_back(arg1PTS1);
        auto pts = e->processNativeFunction(func, callbasePTS, argumentsPTS1);

        outs() << pts->size() << ENDL;
    }


    //auto func2 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CreateJavaInstanceFromNative_createInstanceAndCallMyFunctionFromNative");
    //auto func2 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_unidirectional_WriteJavaFieldFromNative_setMyfield");
    auto func2 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_unidirectional_WriteJavaFieldFromNative_setMyfield");
    if (func2) {
        set<long> callBasePTS;
        callBasePTS.insert(50);
        vector<set<long>> argumentsPTS2;
        set<long> arg1PTS;
        arg1PTS.insert(2);
        argumentsPTS2.push_back(arg1PTS);
        auto pts2 = e->processNativeFunction(func2, callbasePTS, argumentsPTS2);

        outs() << pts2->size() << ENDL;
    }

    //auto nativeIdentity = svfModule->getSVFFunction("nativeIdentityFunction");


    // clean up memory
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}

