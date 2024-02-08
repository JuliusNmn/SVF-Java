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


using namespace llvm;
using namespace std;
using namespace SVF;

/*
Creates a new java instance of a specified CppReference class, 
initialized with the address of the supplied pointer
*/
jobject createCppRefObject(JNIEnv* env, char* classname, const void* pointer) {
    jclass svfValueClass = env->FindClass(classname);
    jmethodID valueConstructor = env->GetMethodID(svfValueClass, "<init>", "(J)V");
    return  env->NewObject(svfValueClass, valueConstructor, (jlong) pointer);
}
static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

/*!
 * An example to query alias results of two LLVM values
 */
SVF::AliasResult aliasQuery(PointerAnalysis* pta, Value* v1, Value* v2)
{
    SVFValue* val1 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v1);
    SVFValue* val2 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v2);

    return pta->alias(val1,val2);
}

/*!
 * An example to print points-to set of an LLVM value
 */
std::string printPts(PointerAnalysis* pta, SVFValue* svfval)
{

    std::string str;
    raw_string_ostream rawstr(str);

    NodeID pNodeId = pta->getPAG()->getValueNode(svfval);
    const PointsTo& pts = pta->getPts(pNodeId);
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
            ii != ie; ii++) 
    {
        rawstr << " " << *ii << " ";
        PAGNode* targetObj = pta->getPAG()->getGNode(*ii);
        if(targetObj->hasValue())
        {
            rawstr << "(" << targetObj->getValue()->toString() << ")\t ";
        }
    }

    return rawstr.str();

}


/*!
 * An example to query/collect all successor nodes from a ICFGNode (iNode) along control-flow graph (ICFG)
 */
void traverseOnICFG(ICFG* icfg, const Instruction* inst)
{
    SVFInstruction* svfinst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(inst);

    ICFGNode* iNode = icfg->getICFGNode(svfinst);
    FIFOWorkList<const ICFGNode*> worklist;
    Set<const ICFGNode*> visited;
    worklist.push(iNode);

    /// Traverse along VFG
    while (!worklist.empty())
    {
        const ICFGNode* iNode = worklist.pop();
        for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
                    iNode->OutEdgeEnd(); it != eit; ++it)
        {
            ICFGEdge* edge = *it;
            ICFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end())
            {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}

/*!
 * An example to query/collect all the uses of a definition of a value along value-flow graph (VFG)
 */
void traverseOnVFG(const SVFG* vfg, const SVFValue* svfval)
{
    SVFIR* pag = SVFIR::getPAG();
    // SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);

    PAGNode* pNode = pag->getGNode(pag->getValueNode(svfval));
    const VFGNode* vNode = vfg->getDefSVFGNode(pNode);
    FIFOWorkList<const VFGNode*> worklist;
    Set<const VFGNode*> visited;
    worklist.push(vNode);

    /// Traverse along VFG
    while (!worklist.empty())
    {
        const VFGNode* vNode = worklist.pop();
        for (VFGNode::const_iterator it = vNode->OutEdgeBegin(), eit =
                    vNode->OutEdgeEnd(); it != eit; ++it)
        {
            VFGEdge* edge = *it;
            VFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end())
            {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }

    /// Collect all LLVM Values
    for(Set<const VFGNode*>::const_iterator it = visited.begin(), eit = visited.end(); it!=eit; ++it)
    {
        const VFGNode* node = *it;
        /// can only query VFGNode involving top-level pointers (starting with % or @ in LLVM IR)
        /// PAGNode* pNode = vfg->getLHSTopLevPtr(node);
        /// Value* val = pNode->getValue();
    }
}



int main(int argc, char ** argv)
{   
    std::vector<std::string> moduleNameVec;
    moduleNameVec.push_back("/home/julius/SVF-Java/example.ll");
    if (Options::WriteAnder() == "ir_annotator")
    {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR* pag = builder.build();

    /// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
    /// Query aliases
    /// aliasQuery(ander,value1,value2);
    /// Print points-to information
    /// printPts(ander, value1);

   
    /// Call Graph
    PTACallGraph* callgraph = ander->getPTACallGraph();

    /// ICFG
    ICFG* icfg = pag->getICFG();
    
    /// Value-Flow Graph (VFG)
    VFG* vfg = new VFG(callgraph); 

    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFG* svfg = svfBuilder.buildFullSVFG(ander);

    
     for (const SVFFunction* func : *svfModule) {
        cout << "function " <<  func->getName() << " argcount: " << func->arg_size() << endl;
        for (u32_t i = 0; i < func->arg_size(); i++) {
           const  SVFArgument* arg = func->getArg(i);
           SVFValue* val = (SVFValue*)arg;
           cout << "arg " << i << " pts: " << printPts(ander, val) << endl;
        }
           if (func->getName() == "print") {
            const SVFValue* value = func->getArg(0);
            /// Collect uses of an LLVM Value
            traverseOnVFG(svfg, value);

        }
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
void* getCppReferencePointer(JNIEnv* env, jobject obj){
    jclass cls = env->FindClass("svfjava/CppReference");
    jfieldID addressField = env->GetFieldID(cls, "address", "J");
    void* ref = (void*)env->GetLongField(obj, addressField);
    return ref;
}
/*
 * Class:     svfjava_SVFJava
 * Method:    runmain
 * Signature: ([Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_svfjava_SVFJava_runmain
(JNIEnv * env, jobject jthis, jobjectArray args) {
    // thank god for chatGPT
    
    printf("calling main\n");
    // Get the number of arguments in the jobjectArray
    jsize argc = env->GetArrayLength(args);
    
    // Create a C array of strings to hold the arguments
    char* argv[argc];
    
    // Populate the argv array with argument values from jobjectArray
    for (jsize i = 0; i < argc; i++) {
        jstring arg = static_cast<jstring>(env->GetObjectArrayElement(args, i));
        const char* argStr = env->GetStringUTFChars(arg, 0);
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

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    jclass svfModuleClass = env->FindClass("svfjava/SVFModule");
    jmethodID constructor = env->GetMethodID(svfModuleClass, "<init>", "(J)V");
    jobject svfModuleObject = env->NewObject(svfModuleClass, constructor, (jlong)svfModule);

    env->ReleaseStringUTFChars(moduleName, moduleNameStr);

    return svfModuleObject;
}

/*
 * Class:     svfjava_SVFIRBuilder
 * Method:    create
 * Signature: (Lsvfjava/SVFModule;)Lsvfjava/SVFIRBuilder;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIRBuilder_create(JNIEnv *env, jclass cls, jobject module) {
    SVFModule* svfModule = (SVFModule*)getCppReferencePointer(env, module);
    SVFIRBuilder* builder = new SVFIRBuilder(svfModule);

    jclass builderClass = env->FindClass("svfjava/SVFIRBuilder");
    jmethodID constructor = env->GetMethodID(builderClass, "<init>", "(J)V");
    jobject builderObject = env->NewObject(builderClass, constructor, (jlong)builder);

    return builderObject;
}

/*
 * Class:     svfjava_SVFIRBuilder
 * Method:    build
 * Signature: ()Lsvfjava/SVFIR;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIRBuilder_build(JNIEnv *env, jobject obj) {
    SVFIRBuilder* builder = (SVFIRBuilder*)getCppReferencePointer(env, obj);

    SVFIR* ir = builder->build();

    jclass irClass = env->FindClass("svfjava/SVFIR");
    jmethodID constructor = env->GetMethodID(irClass, "<init>", "(J)V");
    jobject irObject = env->NewObject(irClass, constructor, (jlong)ir);

    return irObject;
}

/*
 * Class:     svfjava_Andersen
 * Method:    create
 * Signature: (Lsvfjava/SVFIR;)Lsvfjava/Andersen;
 */
JNIEXPORT jobject JNICALL Java_svfjava_Andersen_create(JNIEnv *env, jclass cls, jobject irObj) {
    SVFIR* ir = (SVFIR*)getCppReferencePointer(env, irObj);

    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(ir);

    jclass anderClass = env->FindClass("svfjava/Andersen");
    jmethodID constructor = env->GetMethodID(anderClass, "<init>", "(J)V");
    jobject anderObject = env->NewObject(anderClass, constructor, (jlong)ander);

    return anderObject;
}

/*
 * Class:     svfjava_Andersen
 * Method:    getPTACallGraph
 * Signature: ()Lsvfjava/PTACallGraph;
 */
JNIEXPORT jobject JNICALL Java_svfjava_Andersen_getPTACallGraph(JNIEnv *env, jobject obj) {
    Andersen* ander = (Andersen*)getCppReferencePointer(env, obj);

    PTACallGraph* callgraph = ander->getPTACallGraph();

    jclass callgraphClass = env->FindClass("svfjava/CppReference");
    jmethodID constructor = env->GetMethodID(callgraphClass, "<init>", "(J)V");
    jobject callgraphObject = env->NewObject(callgraphClass, constructor, (jlong)callgraph);

    return callgraphObject;
}
/*
 * Class:     svfjava_VFG
 * Method:    create
 * Signature: (Lsvfjava/PTACallGraph;)Lsvfjava/VFG;
 */
JNIEXPORT jobject JNICALL Java_svfjava_VFG_create
  (JNIEnv * env, jclass cls, jobject cg){
    PTACallGraph* callgraph = (PTACallGraph*)getCppReferencePointer(env, cg);
    VFG* vfg = new VFG(callgraph);
    jclass vfgClass = env->FindClass("svfjava/VFG");
    jmethodID constructor = env->GetMethodID(vfgClass, "<init>", "(J)V");
    jobject o = env->NewObject(vfgClass, constructor, (jlong)callgraph);
    return o;
  }
/*
 * Class:     svfjava_SVFGBuilder
 * Method:    create
 * Signature: ()Lsvfjava/SVFGBuilder;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFGBuilder_create(JNIEnv *env, jclass cls) {

    SVFGBuilder* b = new SVFGBuilder();
    jmethodID constructor = env->GetMethodID(cls, "<init>", "(J)V");
    jobject o = env->NewObject(cls, constructor, (jlong)b);

    return o;
}
/*
 * Class:     svfjava_SVFGBuilder
 * Method:    buildFullSVFG
 * Signature: (Lsvfjava/Andersen;)Lsvfjava/SVFG;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFGBuilder_buildFullSVFG(JNIEnv *env, jobject jthis, jobject obj) {
    SVFGBuilder* builder = (SVFGBuilder*)getCppReferencePointer(env, jthis);
    Andersen* ander = (Andersen*)getCppReferencePointer(env, obj);

    SVFG* svfg = builder->buildFullSVFG(ander);

    jclass svfgClass = env->FindClass("svfjava/SVFG");
    jmethodID constructor = env->GetMethodID(svfgClass, "<init>", "(J)V");
    jobject svfgObject = env->NewObject(svfgClass, constructor, (jlong)svfg);

    return svfgObject;
}
/*
 * Class:     svfjava_SVFModule
 * Method:    getFunctions
 * Signature: ()[Lsvfjava/SVFFunction;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFModule_getFunctions(JNIEnv *env, jobject obj) {
    SVFModule* svfModule = (SVFModule*)getCppReferencePointer(env, obj);

    std::vector<const SVFFunction*> functions = svfModule->getFunctionSet();

    jclass svfFunctionClass = env->FindClass("svfjava/SVFFunction");
    jmethodID constructor = env->GetMethodID(svfFunctionClass, "<init>", "(J)V");

    jobjectArray functionArray = env->NewObjectArray(functions.size(), svfFunctionClass, nullptr);

    for (size_t i = 0; i < functions.size(); ++i) {
        jobject svfFunctionObject = env->NewObject(svfFunctionClass, constructor, (jlong) functions[i]);
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
    SVFFunction* svfFunction = (SVFFunction*)getCppReferencePointer(env, obj);

    jclass svfArgumentClass = env->FindClass("svfjava/SVFArgument");
    jmethodID constructor = env->GetMethodID(svfArgumentClass, "<init>", "(J)V");

    jobjectArray argumentArray = env->NewObjectArray(svfFunction->arg_size(), svfArgumentClass, nullptr);

    for (size_t i = 0; i < svfFunction->arg_size(); ++i) {
        const SVFArgument* arg = svfFunction->getArg(i);
        jobject svfArgumentObject = env->NewObject(svfArgumentClass, constructor, (jlong) arg);

        // Check if the argument can be cast to SVFValue
        const SVFValue* svfValue = dynamic_cast<const SVFValue*>(arg);
        if (svfValue != nullptr) {
            // It can be cast to SVFValue, create SVFValue object instead
            svfArgumentObject = createCppRefObject(env, "svfjava/SVFValue", svfValue);
        }

        env->SetObjectArrayElement(argumentArray, i, svfArgumentObject);
    }

    return argumentArray;
}

/*
 * Class:     svfjava_PointsTo
 * Method:    toStringNative
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_svfjava_PointsTo_toStringNative
  (JNIEnv* env, jobject jthis){
    PointsTo* pt = (PointsTo*)getCppReferencePointer(env, jthis);
    return nullptr;
}

/*
 * Class:     svfjava_SVFValue
 * Method:    toStringNative
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_svfjava_SVFValue_toStringNative
  (JNIEnv* env, jobject jthis){
    SVFValue* v = (SVFValue*)getCppReferencePointer(env, jthis);
    std::string s = v->toString();
    const char* c = s.c_str();
    jstring moduleNameStr = env->NewStringUTF(c);
    return moduleNameStr;
  }


/*
 * Class:     svfjava_SVFIR
 * Method:    getGNode
 * Signature: (Lsvfjava/PointsTo;)Lsvfjava/SVFVar;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFIR_getGNode
  (JNIEnv* env, jobject jthis, jobject jpt){
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
  (JNIEnv* env, jobject jthis, jobject val){
    SVFIR* ir = (SVFIR*)getCppReferencePointer(env, jthis);
    SVFValue* svfVal = (SVFValue*)getCppReferencePointer(env, val);
    NodeID nodeId = ir->getValueNode(svfVal);

}

/*
 * Class:     svfjava_Andersen
 * Method:    getPTS
 * Signature: (Lsvfjava/SVFIR;Lsvfjava/SVFValue;)[Lsvfjava/SVFVar;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_Andersen_getPTS(JNIEnv * env, jobject jthis, jobject jir, jobject jval){
    Andersen* andersen = (Andersen*)getCppReferencePointer(env, jthis);
    SVFIR* ir = (SVFIR*)getCppReferencePointer(env, jir);
    SVFValue* svfVal = (SVFValue*)getCppReferencePointer(env, jval);
    NodeID nodeId = ir->getValueNode(svfVal);
    const PointsTo& pts = andersen->getPts(nodeId);
    std::vector<PAGNode*> pointsToSet;
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
         ii != ie; ii++) {
        PAGNode* target = ir->getGNode(*ii);
        pointsToSet.push_back(target);
    }

    jclass pointsToClass = env->FindClass("svfjava/PointsTo");
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
  (JNIEnv* env, jobject jthis){
    SVFVar* v = (SVFVar*)getCppReferencePointer(env, jthis);
    const SVFValue* val = v->getValue();
    return createCppRefObject(env, "svfjava/SVFValue", val);
  }
