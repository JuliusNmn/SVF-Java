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
#include <iostream>
#include "main.h"


using namespace llvm;
using namespace std;
using namespace SVF;

int main(int argc, char **argv) {

    std::vector<std::string> moduleNameVec;
    if (argc != 2) {
        cout << "usage: " << argv[0] << "path_to_module(.bc|.ll)" << endl;
        return 1;
    }
    moduleNameVec.push_back(argv[1]);
    if (Options::WriteAnder() == "ir_annotator") {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }
    cout << "EXTAPI path: " << SVF_INSTALL_EXTAPI_FILE << endl;
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
    // cout << endl;
    CB_SetArgGetReturnPTS cb1 = [](set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS) {
        //  make function return this
        cout << "function called " << methodName << endl;
        return callBasePTS;
    };
    CB_GenerateNativeAllocSiteId  cb2 = [](const char* className, const char* context) {
        return 101;
    };
    CB_GetFieldPTS cb3 = [](set<long>* callBasePTS, const char * className, const char * fieldName) {
        cout << "GetField " << fieldName << endl;
        return callBasePTS;
    };
    CB_SetFieldPTS cb4 = [] (set<long>* callBasePTS, const char * className, const char * fieldName, set<long>* argPTS) {
        cout << "SetField " << argPTS << endl;
    };
    CB_GetArrayElementPTS cb5 = [] (set<long>* arrayPTS) {
        cout << "GetArrayElement " << arrayPTS << endl;
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


    for (auto function : svfModule->getFunctionSet()){
        

        //auto function = svfModule->getSVFFunction("Java_org_libjpegturbo_turbojpeg_TJTransformer_transform");
        //auto function = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_intraprocedural_unidirectional_NativeIdentityFunction_identity");
        //auto function = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_interprocedural_unidirectional_CAccessJava_ReadJavaFieldFromNative_getMyfield");

        if (function->getName().find("Java_") == 0) {
            cout << function->getName() << endl;
            vector<set<long>> argumentsPTS1;

            for (int i = 0; i < function->arg_size() - 2; i++) {
                set<long> arg1PTS1;
                arg1PTS1.insert(1000 + i);
                argumentsPTS1.push_back(arg1PTS1);
            }
            auto pts = e->processNativeFunction(function, callbasePTS, argumentsPTS1);

            cout << pts->size() << endl;
        }
     }
    /*
    //auto f1 = svfModule->getSVFFunction("llvm_controlflow_interprocedural_interleaved_CallJavaFunctionFromNativeAndReturn_callMyJavaFunctionFromNative");
    auto f1 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_interprocedural_interleaved_RegisterCallback_registerCallbacksAndCall");
    //auto f1 = svfModule->getSVFFunction("Java_org_libjpegturbo_turbojpeg_TJTransformer_transform");
    cout << f1->toString() << endl;
    vector<set<long>> argumentsPTS1;
    for (int i = 0; i < f1->arg_size() - 2; i++) {
        set<long> arg1PTS1;
        //arg1PTS1.insert(1000 + i);
        argumentsPTS1.push_back(arg1PTS1);
    }

    auto pts1 = e->processNativeFunction(f1, callbasePTS, argumentsPTS1);

    cout << pts1->size() << endl;
    auto f2 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_intraprocedural_bidirectional_WriteNativeGlobalVariable_getNativeGlobal");
    vector<set<long>> argumentsPTS2;
    auto pts2 = e->processNativeFunction(f2, callbasePTS, argumentsPTS2);
    cout << pts2->size() << endl;


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

        cout << pts2->size() << endl;

    }*/

    //auto nativeIdentity = svfModule->getSVFFunction("nativeIdentityFunction");


    // clean up memory
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}

