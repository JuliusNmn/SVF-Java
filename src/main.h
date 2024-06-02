//
// Created by julius on 5/15/24.
//

#ifndef SVF_JAVA_MAIN_H
#define SVF_JAVA_MAIN_H

#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "detectJNICalls.h"

#include <iostream>
#define ENDL "\n"
// this callback is called for all JNI method invocations.
// call base and argument PTS are passed
typedef std::function<std::set<long>*(std::set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,std::vector<std::set<long>*> argumentsPTS)> CB_SetArgGetReturnPTS;
// this will be used to request an allocsite id for a Native-side JNI allocation
typedef std::function<long(const char* classname, const char* context)> CB_GenerateNativeAllocSiteId;
// called at GetObjectField etc. invoke site
typedef std::function<std::set<long>*(std::set<long>* callBasePTS, const char * className, const char * fieldName)> CB_GetFieldPTS;
// called at SetObjectField etc, invoke site
typedef std::function<void(std::set<long>* callBasePTS, const char * className, const char * fieldName, std::set<long>* argPTS)> CB_SetFieldPTS;
// called at GetObjectArrayElement etc. invoke site
typedef std::function<std::set<long>*(std::set<long>* arrayPTS)> CB_GetArrayElementPTS;


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


    SVFIR* pag;
    DetectNICalls* detectedJniCalls;
    // these PTS are updated when processNativeFunction is called
    //std::map<const VFGNode*, set<long>*> jniFunctionArgPointsToSets;
    std::map<NodeID, std::set<NodeID>*> additionalPTS;
    // for each detected JNI interaction callsite, a dummy node is created
    // if a requested pts contains one of these nodes,
    std::map<NodeID, std::pair<SVFCallInst*, JNICallOffset>*> jniCallsiteDummyNodes;

    // map from custom nodeID to dummyObjNode nodeID.
    std::map<long, NodeID> javaAllocNodeToSVFDummyNode;
    std::map<NodeID, long> pagDummyNodeToJavaAllocNode;

    std::set<NodeID> registeredJavaAllocSites;

    CustomAndersen* customAndersen = nullptr;
    bool refreshAndersen = true;
    void updateAndersen();
    void addPTS(NodeID node, std::set<long> customNodes);
    long getTotalSizeOfAdditionalPTS();

    std::set<long>* getReturnPTSForJNICallsite(const llvm::CallBase* callsite, DominatorTree* tree) ;
    std::set<long>* getPTSForField(const llvm::CallBase* callsite, DominatorTree* tree) ;
    void reportSetField(const llvm::CallBase* callsite, DominatorTree* tree) ;
    std::set<long>* getPTSForArray(const llvm::CallBase* callsite);
    const SVFCallInst* retrieveGetFieldID(const llvm::CallBase* getOrSetField, DominatorTree* tree);
    const char* getClassName(const SVFValue* paramClass, const llvm::CallBase* user, DominatorTree* tree);
    void handleJNIAllocSite(const llvm::CallBase* inst, DominatorTree* tree);
    void solve(const SVFFunction* function);
    NodeID createOrAddDummyNode(long customId);


    // get PTS for return node / callsite param (+ base) node
    // if we are requesting the pts of a base / arg at a jni callsite, pass the callsite.
    // native code often loads / stores to addresses, llvm's SSA is not guaranteed and cycles are possible
    // e.g. obj = CallObjectMethod(env, obj).
    // pts of obj will include this callsite
    std::set<long>* getPTS(NodeID node);

public:
    ExtendedPAG(SVFModule* module, SVFIR* pag);

    // main entry point for native analysis.
    // called from java analysis when PTS for native function call args (+ base) are known
    // gets PTS for return value, requests PTS from java analysis on demand
    // also reports argument points to sets for all transitively called jni callsites (CallMethod + SetField)
    std::set<long>* processNativeFunction(const SVFFunction* function, std::set<long> callBasePTS, std::vector<std::set<long>> argumentsPTS);
    // this reports callsite arguments of all jni callsites and field sets to the java analysis
    // transitively (for the passed function plus any called functions)!
    void runDetector(const SVFFunction* function, std::set<const SVFFunction*>* processed) ;


    CB_SetArgGetReturnPTS callback_ReportArgPTSGetReturnPTS;
    CB_GenerateNativeAllocSiteId callback_GenerateNativeAllocSiteId;
    CB_GetFieldPTS callback_GetFieldPTS;
    CB_SetFieldPTS callback_SetFieldPTS;
    CB_GetArrayElementPTS  callback_GetArrayElementPTS;
    SVFModule* module;
};

// this custom implementation adds additional points-to-sets to a PAG before Andersen is performed.
class CustomAndersen : public SVF::AndersenWaveDiff {
    // extend initial PTS of nodes before Andersen is computed.
    // nodes in PTS must exist in PAG, use addDummyNode to add them.
    // note that this persistently modifies pag, so keep track of added NodeIDs in outside instance.
    std::map<NodeID, std::set<NodeID>*>* additionalPTS;
public:
    CustomAndersen(SVFIR* _pag, std::map<NodeID, std::set<NodeID>*>* additionalPTS, PTATY type = AndersenWaveDiff_WPA, bool alias_check = true): AndersenWaveDiff(_pag, type, alias_check), additionalPTS(additionalPTS) {}
    virtual void initialize() override;
    void updatePTS(std::map<NodeID, std::set<NodeID>*>* additionalPTS);
};




#endif //SVF_JAVA_MAIN_H
