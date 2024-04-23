//
// Created by julius on 4/22/24.
//

#include "CustomAndersen.h"
using namespace SVF;
#include <iostream>

void CustomAndersen::initialize() {
    Andersen::initialize();
    /// Build SVFIR
    PointerAnalysis::initialize();
    /// Create statistic class
    stat = new AndersenStat(this);

    /*
    auto nativeIdentity = pag->getModule()->getSVFFunction("nativeIdentityFunction");
    auto arg0 = pag->getFunArgsList(nativeIdentity)[0];
    auto arg0pagNode = pag->getValueNode(arg0->getValue());
    //NodeID addrNode = pag->addDummyValNode();
    NodeID objNode = pag->addDummyObjNode(arg0->getType());
    //svfMod->setPagFromTXT("asdasd");
    //auto addrEdge = consCG->addAddrCGEdge(objNode, addrNode);
    // processAddr(AddrEdge(src, dst)) -> addPts(dst,src)

    //auto cEdge = consCG->addCopyCGEdge(addrNode, arg0pagNode);

    setGraph(consCG);

    //addPts(addrNode, objNode);
    //addPts(arg0pagNode, objNode);
    */

    /// Build Constraint Graph
    consCG = new ConstraintGraph(pag);
    setGraph(consCG);

    // register addidional PTS
    // normally, PTS is only added if an AddrEdge exists, eg:
    // NodeID addrNode = pag->addDummyValNode();
    // consCG->addAddrCGEdge(objNode, addrNode);
    // this didn't update PTS for some reason. addPts() turned out to work just fine
    for (const auto &additional: *additionalPTS) {
        for (const auto &nodeId: *additional.second) {
            addPts(additional.first, nodeId);
        }
    }
    if (Options::ConsCGDotGraph())
        consCG->dump("consCG_initial");

    setDetectPWC(true);
    /*
    auto nodeStack = SVF::Andersen::SCCDetect();
    std::cout << "SCCDetect: ";
    while (!nodeStack.empty()) {
        NodeID nodeId = nodeStack.top();
        nodeStack.pop();
        std::cout << nodeId << " ";
    }
    std::cout << std::endl;
    // SVFStmt::Addr edge,  addAddrCGEdge(edge->getRHSVarID(),edge->getLHSVarID());
    for (ConstraintGraph::const_iterator nodeIt = consCG->begin(), nodeEit = consCG->end(); nodeIt != nodeEit; nodeIt++)
    {
        ConstraintNode * cgNode = nodeIt->second;
        if (cgNode->getId() == addrNode) {
            for (ConstraintNode::const_iterator it = cgNode->incomingAddrsBegin(), eit = cgNode->incomingAddrsEnd();
                 it != eit; ++it) {
                std::cout << (*it)->getSrcID() << std::endl;
                addPts(addrNode, objNode);
            }
        }
    }
     */
}