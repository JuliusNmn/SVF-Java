//
// Created by julius on 4/22/24.
//

#include "CustomAndersen.h"
using namespace SVF;
#include <iostream>

void CustomAndersen::initialize() {
    Andersen::initialize();
    PointerAnalysis::initialize();
    stat = new AndersenStat(this);
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