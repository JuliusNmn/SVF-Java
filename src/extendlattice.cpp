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
    auto pts = customAndersen->getPts(node);
    set<long>* javaAllocPTS = new set<long>();
    for (const NodeID allocNode: pts) {
        if (long id = pagDummyNodeToJavaAllocNode[allocNode]){
            javaAllocPTS->insert(id);
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
