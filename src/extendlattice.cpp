//
// Created by julius on 5/31/24.
//

#include "main.h"
using namespace std;
using namespace SVF;
#include <iostream>

void CustomAndersen::initialize() {
    Andersen::initialize();
    PointerAnalysis::initialize();
    if (stat)
        delete stat;
    stat = new AndersenStat(this);
    if (consCG)
        delete consCG;
    consCG = new ConstraintGraph(pag);
    setGraph(consCG);

    // register addidional PTS
    // normally, PTS is only added if an AddrEdge exists, eg:
    // NodeID addrNode = pag->addDummyValNode();
    // consCG->addAddrCGEdge(objNode, addrNode);
    // this didn't update PTS for some reason. addPts() turned out to suffice for pts propagation
    for (const auto &additional: *additionalPTS) {
        for (const auto &nodeId: *additional.second) {
            addPts(additional.first, nodeId);
        }
    }

    setDetectPWC(true);
}

void CustomAndersen::updatePTS(std::map<NodeID, std::set<NodeID> *> *additionalPTS) {

    for (const auto &additional: *additionalPTS) {
        for (const auto &nodeId: *additional.second) {
            addPts(additional.first, nodeId);
            pushIntoWorklist(nodeId);
            //pushIntoWorklist(additional.first);
        }
    }
}
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
    assert(customId != 0);
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
    cout << "running andersen's analysis with " << additionalPTS.size() << " additional Points-to edges" << endl;
    if (customAndersen) {
        delete customAndersen;
        //customAndersen->updatePTS(&additionalPTS);
    }
    customAndersen = new CustomAndersen(pag, &additionalPTS);
    customAndersen->disablePrintStat();

    customAndersen->analyze();
    refreshAndersen = false;
    //customAndersen->dumpAllPts();
    //pag->dump("pag.dot");
}

// get PTS for return node / callsite param (+ base) node
set<long>* ExtendedPAG::getPTS(NodeID node) {
    JNINativeInterface_ j{};
    const JNICallOffset offset_NewGlobalRef = (unsigned long) (&j.NewGlobalRef) - (unsigned long) (&j);

    auto pts = customAndersen->getPts(node);
    set<long>* javaAllocPTS = new set<long>();
    for (const NodeID allocNode: pts) {
        if (long id = pagDummyNodeToJavaAllocNode[allocNode]){
            javaAllocPTS->insert(id);

        }
        if (std::pair<const char *, int>* funcArg = pagDummyNodeToJavaArgumentNode[allocNode]){
            if (strcmp(funcArg->first,currentFunction->getName().c_str()) != 0){
                // only request argument pts for functions other than the one currently being processed
                set<long>* otherFunctionArgPts = callback_GetNativeFunctionArg(funcArg->first, funcArg->second);
                javaAllocPTS->insert(otherFunctionArgPts->begin(), otherFunctionArgPts->end());
            }
        }

        if (auto getGlobal = jniCallsiteDummyNodes[allocNode]){
            if (getGlobal->second == offset_NewGlobalRef) {
                const SVFCallInst* call = SVFUtil::dyn_cast<SVFCallInst>(getGlobal->first);
                const SVFValue* obj = call->getArgOperand(1);
                auto argNode = pag->getValueNode(obj);
                auto pts2 = customAndersen->getPts(argNode);
                for (const NodeID allocNode2: pts2) {
                    if (std::pair<const char *, int>* funcArg = pagDummyNodeToJavaArgumentNode[allocNode2]){
                        if (strcmp(funcArg->first,currentFunction->getName().c_str()) != 0){
                            // only request argument pts for functions other than the one currently being processed
                            set<long>* otherFunctionArgPts = callback_GetNativeFunctionArg(funcArg->first, funcArg->second);
                            javaAllocPTS->insert(otherFunctionArgPts->begin(), otherFunctionArgPts->end());
                        }
                    }

                    if (long id = pagDummyNodeToJavaAllocNode[allocNode2]){
                        javaAllocPTS->insert(id);
                    }

                }
            }
        }

        auto pts2 = customAndersen->getPts(allocNode);
        for (const NodeID allocNode2: pts2) {
            if (long id = pagDummyNodeToJavaAllocNode[allocNode2]){
                javaAllocPTS->insert(id);
            }

        }
    }
    return javaAllocPTS;
}
