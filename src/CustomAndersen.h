//
// Created by julius on 4/22/24.
//

#ifndef SVF_JAVA_CUSTOMANDERSEN_H
#define SVF_JAVA_CUSTOMANDERSEN_H
#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
using namespace SVF;
class CustomAndersen : public SVF::AndersenWaveDiff {
    // extend initial PTS of nodes before Andersen is computed.
    // nodes in PTS must exist in PAG, use addDummyNode to add them.
    // note that this persistently modifies pag, so keep track of added NodeIDs in outside instance.
    std::map<NodeID, std::set<NodeID>*>* additionalPTS;
public:
    CustomAndersen(SVFIR* _pag, std::map<NodeID, std::set<NodeID>*>* additionalPTS, PTATY type = AndersenWaveDiff_WPA, bool alias_check = true): AndersenWaveDiff(_pag, type, alias_check), additionalPTS(additionalPTS) {}
    virtual void initialize() override;
};


#endif //SVF_JAVA_CUSTOMANDERSEN_H
