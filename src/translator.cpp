//
// Created by julius on 5/31/24.
//

#include "svf-ex.h"
using namespace std;

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
