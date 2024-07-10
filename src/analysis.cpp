//
// Created by julius on 6/7/24.
//
#include "main.h"
using namespace std;
using namespace SVF;



// main entry point for native detector.
// called from java analysis when PTS for native function call args (+ base) are known
// gets PTS for return value, requests PTS from java analysis on demand
// also reports argument points to sets for all transitively called jni callsites (CallMethod + SetField)

set<long>* ExtendedPAG::processNativeFunction(const SVFFunction* function, set<long> callBasePTS, vector<set<long>> argumentsPTS) {

    // (env, this, arg0, arg1, ...)
    auto name =  function->getName();
    auto args = pag->getFunArgsList(function);
    auto baseNode = pag->getValueNode(args[1]->getValue());
    if (args.size() != argumentsPTS.size() + 2) {
        cout << "native function arg count (" << args.size() << ") != size of arguments PTS (" << argumentsPTS.size() << ") + 2 " << endl;
        assert(args.size() == argumentsPTS.size() + 2 && "invalid arg count");
    }
    for (const auto &allocsite: *callBasePTS) {
        assert(allocsite != 0);
    }
    addPTS(baseNode, callBasePTS);


    for (int i = 0; i < argumentsPTS.size(); i++) {
        auto argNode = pag->getValueNode(args[i + 2]->getValue());
        auto argPTS = argumentsPTS[i];
        for (const auto &allocsite: argPTS) {
            assert(allocsite != 0);
        }
        addPTS(argNode, argPTS);
    }
    solve(function);


    NodeID retNode = pag->getReturnNode(function);
    set<long>* javaPTS = getPTS(retNode);

    return javaPTS;
}