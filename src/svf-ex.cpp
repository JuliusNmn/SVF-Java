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
#include "jniheaders/svfjava_SVFJava.h"
#include "jniheaders/svfjava_SVFModule.h"
#include "detectJNICalls.h"
#include "CustomAndersen.h"
#include <iostream>

using namespace llvm;
using namespace std;
using namespace SVF;


// this callback is called for all JNI method invocations.
// call base and argument PTS are passed
typedef std::function<set<long>*(set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS)> ProcessJavaMethodInvocation;
// this will be used to request an allocsite id for a Native-side JNI allocation
typedef std::function<long(const char* classname, const char* context)> GetJNIAllocSiteId;

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
class ExtendedPAG {

private:

    ProcessJavaMethodInvocation callback = nullptr;
    GetJNIAllocSiteId getAllocSiteId = nullptr;
    SVFIR* pag;
    DetectNICalls* detectedJniCalls;
    // these PTS are updated when getNativeFunctionPTS is called
    //std::map<const VFGNode*, set<long>*> jniFunctionArgPointsToSets;
    map<NodeID, set<NodeID>*> additionalPTS;
    // for each detected JNI interaction callsite, a dummy node is created
    // if a requested pts contains one of these nodes,
    map<NodeID, JNIInvocation*> jniCallsiteDummyNodes;
    bool refreshAndersen = true;


    void addPTS(NodeID node, set<long> customNodes) {
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

    // map from custom nodeID to dummyObjNode nodeID.
    map<long, NodeID> javaAllocNodeToSVFDummyNode;
    map<NodeID, long> pagDummyNodeToJavaAllocNode;

    NodeID createOrAddDummyNode(long customId) {
        if (NodeID id = javaAllocNodeToSVFDummyNode[customId]){
            return id;
        }
        NodeID newId = pag->addDummyObjNode(SVFType::getSVFPtrType());
        javaAllocNodeToSVFDummyNode[customId] = newId;
        pagDummyNodeToJavaAllocNode[newId] = customId;
        return newId;
    }
    CustomAndersen* customAndersen;

    void updateAndersen() {
        if (refreshAndersen) {
            customAndersen = new CustomAndersen(pag, &additionalPTS);
            customAndersen->analyze();
            refreshAndersen = false;
        }
    }

    // get PTS for return node / callsite param (+ base) node
    set<long>* getPTS(NodeID node) {
        updateAndersen();
        auto pts = customAndersen->getPts(node);
        set<long>* javaAllocPTS = new set<long>();
        for (const NodeID allocNode: pts) {
            if (JNIInvocation* jniCallsite = jniCallsiteDummyNodes[allocNode]) {
                // PTS includes return value of JNI function. request it
                set<long>* jniCallsitePTS = getReturnPTSForJNICallsite(jniCallsite);
                javaAllocPTS->insert(jniCallsitePTS->begin(), jniCallsitePTS->end());
                delete jniCallsitePTS;
            } else if (long javaAllocNode = pagDummyNodeToJavaAllocNode[allocNode]){
                javaAllocPTS->insert(javaAllocNode);
            }
        }
        return javaAllocPTS;
    }

public:
    ExtendedPAG(SVFModule* module, SVFIR* pag, ProcessJavaMethodInvocation javaCallback, GetJNIAllocSiteId getAllocSiteId) : pag(pag),  callback(javaCallback), getAllocSiteId(getAllocSiteId) {
        const llvm::Module* lm = LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule();
        detectedJniCalls = new DetectNICalls();

        for (const auto &svfFunction: *module) {
            const llvm::Function* llvmFunction = llvm::dyn_cast<llvm::Function>(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfFunction));
            detectedJniCalls->processFunction(*llvmFunction);
        }
        // for each detected jni callsite, add a dummynode and register it
        cout << "detected " << detectedJniCalls->detectedJNIInvocations.size() << " JNI invoke sites" << endl;
        for (const auto &item: detectedJniCalls->detectedJNIInvocations) {
            cout << "JNI invoke site " << item.second->className << " " << item.second->methodName << endl;
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            NodeID callsiteNode = pag->getValueNode(inst);
            NodeID dummyNode = pag->addDummyObjNode(inst->getFunction()->getReturnType());
            jniCallsiteDummyNodes[dummyNode] = item.second;
            set<NodeID>* s = new set<NodeID>();
            s->insert(dummyNode);
            additionalPTS[callsiteNode] = s;
        }
        cout << " detected " << detectedJniCalls->detectedJNIAllocations.size() << " JNI alloc sites" << endl;
        for (const auto &item: detectedJniCalls->detectedJNIAllocations) {
            cout << "JNI alloc site " << item.second->className << endl;
            auto inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(item.first);
            NodeID callsiteNode = pag->getValueNode(inst);
            long id = getAllocSiteId(item.second->className, nullptr);
            NodeID allocSiteDummyNode = createOrAddDummyNode(id);
            set<NodeID>* s = new set<NodeID>();
            s->insert(allocSiteDummyNode);
            additionalPTS[callsiteNode] = s;
            pagDummyNodeToJavaAllocNode[callsiteNode] = id;
        }

    }


    // called from java analysis when PTS for native function call args (+ base) are known
    set<long>* getNativeFunctionPTS(const SVFFunction* function, set<long> callBasePTS, vector<set<long>> argumentsPTS) {
        // (env, this, arg0, arg1, ...)
        auto args = pag->getFunArgsList(function);
        auto baseNode = pag->getValueNode(args[1]->getValue());
        assert(args.size() == argumentsPTS.size() + 2 && "native function arg count must == size of arguments PTS + 2 ");
        addPTS(baseNode, callBasePTS);


        for (int i = 0; i < argumentsPTS.size(); i++) {
            auto argNode = pag->getValueNode(args[i + 2]->getValue());
            auto argPTS = argumentsPTS[i];
            addPTS(argNode, argPTS);
        }

        NodeID retNode = pag->getReturnNode(function);
        set<long>* javaPTS = getPTS(retNode);

        return javaPTS;
    }
private:


    set<long>* getReturnPTSForJNICallsite(const JNIInvocation* jniInv) {
        SVFInstruction* inst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(jniInv->callSite);
        const SVFCallInst* call = dyn_cast<SVFCallInst>(inst);
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
        cout << "requesting return PTS for function " << jniInv->methodName << endl;
        return callback(basePTS, jniInv->className, jniInv->methodName, jniInv->methodSignature, argumentsPTS);

    }

};



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

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    auto lm = LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule();


    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR *pag = builder.build();

    pag->dump("pag");
    /// Create Andersen's pointer analysis
    //Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    Andersen* ander = new AndersenWaveDiff(pag, SVF::Andersen::AndersenWaveDiff_WPA, false);
    ander->analyze();
    ander->getConstraintGraph()->dump("cg");
    /// Query aliases
    /// aliasQuery(ander,value1,value2);
    /// Print points-to information
    /// printPts(ander, value1);

    for (const auto &function: svfModule->getFunctionSet()) {
        NodeID returnNode = pag->getReturnNode(function);
        auto pts = ander->getPts(returnNode);
        cout << "return value pts of function " << function->toString() << endl << " size: " << pts.count() << endl;
        for (const auto &nodeId: pts) {
            auto node = pag->getGNode(nodeId);
            cout << node->toString() << endl;
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
    // cout << endl;
    ProcessJavaMethodInvocation cb1 = [](set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS) {
        //  make function return this
        return callBasePTS;
    };
    GetJNIAllocSiteId  cb2 = [](const char* className, const char* context) {
        return 101;
    };
    ExtendedPAG* e = new ExtendedPAG(svfModule, pag, cb1, cb2);


    int id = 0;
    auto func = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CallJavaFunctionFromNativeAndReturn_callMyJavaFunctionFromNativeAndReturn");
    set<long> callbasePTS;
    callbasePTS.insert(666);
    vector<set<long>> argumentsPTS1;
    set<long> arg1PTS1;
    arg1PTS1.insert(909);
    argumentsPTS1.push_back(arg1PTS1);
    auto pts = e->getNativeFunctionPTS(func, callbasePTS, argumentsPTS1);

    cout << pts->size() << endl;


    auto func2 = svfModule->getSVFFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CreateJavaInstanceFromNative_createInstanceAndCallMyFunctionFromNative");
    set<long> callBasePTS;
    callBasePTS.insert(50);
    vector<set<long>> argumentsPTS2;
    set<long> arg1PTS;
    arg1PTS.insert(2);
    argumentsPTS2.push_back(arg1PTS);
    auto pts2 = e->getNativeFunctionPTS(func2, callbasePTS, argumentsPTS2);

    cout << pts2->size() << endl;


    //auto nativeIdentity = svfModule->getSVFFunction("nativeIdentityFunction");


    // clean up memory
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


// Function to convert jlongArray to std::set<long long>
std::set<long> jlongArrayToSet(JNIEnv *env, jlongArray arr) {
    std::set<long> resultSet;

    // Get array length
    jsize len = env->GetArrayLength(arr);

    // Get array elements
    jlong *elements = env->GetLongArrayElements(arr, nullptr);

    // Add elements to the set
    for (int i = 0; i < len; ++i) {
        resultSet.insert(elements[i]);
    }

    // Release array elements
    env->ReleaseLongArrayElements(arr, elements, JNI_ABORT);

    return resultSet;
}

/*
 * Class:     svfjava_SVFModule
 * Method:    processFunction
 * Signature: (Ljava/lang/String;[J[J)[J
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFModule_processFunction
        (JNIEnv * env, jobject svfModule, jstring functionName, jlongArray basePTS, jobjectArray argsPTSs) {
    // get data structures (SVFModule, SVFFunction, SVFG, ExtendedPAG)
    SVFModule *module = static_cast<SVFModule *>(getCppReferencePointer(env, svfModule));
    assert(module);
    const char* functionNameStr = env->GetStringUTFChars(functionName, NULL);
    const SVFFunction* function = module->getSVFFunction(functionNameStr);
    if (function) {
        cout << " found function " << function->toString() << endl;

    } else {
        cout << "function not found: " << functionNameStr << endl;
        return nullptr;
    }

    jclass svfModuleClass = env->GetObjectClass(svfModule);
    jfieldID svfgField = env->GetFieldID(svfModuleClass, "svfg", "J");
    assert(svfgField);
    SVFG *svfg = static_cast<SVFG *>((void*)env->GetLongField(svfModule, svfgField));
    assert(svfg);
    jfieldID ExtendedPAGField = env->GetFieldID(svfModuleClass, "extendedPAG", "J");
    assert(ExtendedPAGField);
    ExtendedPAG* e = static_cast<ExtendedPAG *>((void*)env->GetLongField(svfModule, ExtendedPAGField));
    assert(e);

    cout << "svfg: " << (long)svfg << endl;
    cout << "extended svfg: " << (long)e << endl;

    auto pag = svfg->getPAG();
    assert(pag);
    auto args = svfg->getPAG()->getFunArgsList(function);
    set<long> basePTSSet = jlongArrayToSet(env, basePTS);
    vector<set<long>> argsPTSsVector;
    // Get array length
    jsize argsCount = env->GetArrayLength(argsPTSs);

    // Iterate over args
    for (int i = 0; i < argsCount; ++i) {
        // Get the arg PTS (which is a jlongArray)
        jlongArray rowArray = (jlongArray) env->GetObjectArrayElement(argsPTSs, i);
        set<long> rowSet = jlongArrayToSet(env, rowArray);
        // Add the PTS to the args PTS vector
        argsPTSsVector.push_back(rowSet);
    }

    // public Set<JavaAllocSite> nativeToJavaCallDetected(Set<JavaAllocSite> base, String methodName, String methodSignature, Set<JavaAllocSite>[] args);
    // dot -Grankdir=LR -Tpdf pag.dot -o pag.pdf
    // dot -Grankdir=LR -Tpdf pag2.dot -o pag2.pdf
    // dot -Grankdir=LR -Tpdf svfg.dot -o svfg.pdf
    // dot -Grankdir=LR -Tpdf svfg2.dot -o svfg2.pdf
    std::vector<jobject> argsV;

    set<long>* returnPTS = e->getNativeFunctionPTS(function, basePTSSet, argsPTSsVector);
    jlongArray returnPTSArray = env->NewLongArray(returnPTS->size());
    jlong* returnPTSArrayElements = env->GetLongArrayElements(returnPTSArray, nullptr);
    int i = 0;
    for (long value : *returnPTS) {
        returnPTSArrayElements[i++] = value;
    }
    delete returnPTS;
    env->ReleaseLongArrayElements(returnPTSArray, returnPTSArrayElements, 0);
    return returnPTSArray;
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
 * Signature: (Ljava/lang/String;Lsvfjava/SVFAnalysisListener;)Lsvfjava/SVFModule;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFModule_createSVFModule(JNIEnv *env, jclass cls, jstring moduleName, jobject listenerArg) {
    const char *moduleNameStr = env->GetStringUTFChars(moduleName, NULL);
    std::vector<std::string> moduleNameVec;
    moduleNameVec.push_back(moduleNameStr);

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    jclass svfModuleClass = env->FindClass("svfjava/SVFModule");
    jmethodID constructor = env->GetMethodID(svfModuleClass, "<init>", "(J)V");
    jobject svfModuleObject = env->NewObject(svfModuleClass, constructor, (jlong) svfModule);

    SVFIRBuilder* builder = new SVFIRBuilder(svfModule);
    SVFIR *pag = builder->build();
    Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
    SVFGBuilder* svfBuilder = new SVFGBuilder();
    SVFG *svfg = svfBuilder->buildFullSVFG(ander);
    assert(svfg->getPAG());


    jmethodID listenerCallback1 = env->GetMethodID(env->GetObjectClass(listenerArg), "nativeToJavaCallDetected", "([JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[[J)[J");
    assert(listenerCallback1);

    jmethodID listenerCallback2 = env->GetMethodID(env->GetObjectClass(listenerArg), "jniNewObject", "(Ljava/lang/String;Ljava/lang/String;)J");
    assert(listenerCallback2);
    jobject listener = env->NewGlobalRef(listenerArg);


    ProcessJavaMethodInvocation cb1 = [env, listener, listenerCallback1](set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS) {
        cout << "calling java svf listener. detected method call to " << methodName << " signature " << methodSignature << endl;
        // Convert char* to Java strings
        jstring jMethodName = env->NewStringUTF(methodName);
        jstring jMethodSignature = env->NewStringUTF(methodSignature);
        // Convert callBasePTS set to a Java long array
        jlongArray basePTSArray = env->NewLongArray(callBasePTS->size());
        jlong* basePTSArrayElements = env->GetLongArrayElements(basePTSArray, nullptr);
        int i = 0;
        for (long value : *callBasePTS) {
            basePTSArrayElements[i++] = value;
        }
        env->ReleaseLongArrayElements(basePTSArray, basePTSArrayElements, 0);

        // Convert argumentsPTS vector of sets to a Java long 2D array
        jobjectArray argumentsPTSArray = env->NewObjectArray(argumentsPTS.size(), env->FindClass("[J"), nullptr);
        for (size_t i = 0; i < argumentsPTS.size(); ++i) {
            std::set<long>* argumentSet = argumentsPTS[i];
            jlongArray argumentArray = env->NewLongArray(argumentSet->size());
            jlong* argumentArrayElements = env->GetLongArrayElements(argumentArray, nullptr);
            size_t j = 0;
            for (long value : *argumentSet) {
                argumentArrayElements[j++] = value;
            }
            env->ReleaseLongArrayElements(argumentArray, argumentArrayElements, 0);
            env->SetObjectArrayElement(argumentsPTSArray, i, argumentArray);
        }
        jstring jClassName = nullptr;
        if (className) {
            jClassName = env->NewStringUTF(className);
        }
        // Call the Java method
        jlongArray resultArray = (jlongArray)env->CallObjectMethod(listener, listenerCallback1, basePTSArray, jClassName, env->NewStringUTF(methodName), env->NewStringUTF(methodSignature), argumentsPTSArray);
        // Convert the returned long array to a C++ set
        std::set<long>* result = new std::set<long>();
        if (resultArray != nullptr) {
            jsize length = env->GetArrayLength(resultArray);
            jlong* resultElements = env->GetLongArrayElements(resultArray, nullptr);
            for (int i = 0; i < length; ++i) {
                result->insert(resultElements[i]);
            }
            env->ReleaseLongArrayElements(resultArray, resultElements, 0);
        }
        // Release local references
        env->DeleteLocalRef(jMethodName);
        env->DeleteLocalRef(jMethodSignature);
        return result;
    };
    GetJNIAllocSiteId  cb2 = [env, listener, listenerCallback2](const char* className, const char* context) {
        jstring jClassName = env->NewStringUTF(className);
        jstring jContext = env->NewStringUTF(context);
        cout << "getting new instanceid from java for " << className << endl;
        jlong newInstanceId = env->CallLongMethod(listener, listenerCallback2, jClassName, jContext);
        env->DeleteLocalRef(jClassName);
        env->DeleteLocalRef(jContext);
        return newInstanceId;
    };

    ExtendedPAG* e = new ExtendedPAG(svfModule, pag, cb1, cb2);
    env->SetLongField(svfModuleObject, env->GetFieldID(svfModuleClass, "svfg", "J"), (long)svfg);
    env->SetLongField(svfModuleObject, env->GetFieldID(svfModuleClass, "extendedPAG", "J"), (long)e);
    env->ReleaseStringUTFChars(moduleName, moduleNameStr);

    return svfModuleObject;
}

/*
 * Class:     svfjava_SVFModule
 * Method:    getFunctions
 * Signature: ()[Ljava/lnag/String;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFModule_getFunctions
        (JNIEnv * env, jobject jThis){
    SVFModule *svfModule = (SVFModule *) getCppReferencePointer(env, jThis);

    std::vector<const SVFFunction *> functions = svfModule->getFunctionSet();

    jclass strClass = env->FindClass("java/lang/String");

    jobjectArray functionArray = env->NewObjectArray(functions.size(), strClass, nullptr);

    for (size_t i = 0; i < functions.size(); ++i) {
        jstring jName = env->NewStringUTF(functions[i]->getName().c_str());

        env->SetObjectArrayElement(functionArray, i, jName);
    }

    return functionArray;
}
