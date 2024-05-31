//
// Created by julius on 5/31/24.
//
#include "svf-ex.cpp"
void ExtendedPAG::solve(const SVFFunction* function) {
    long lastPTSSize = getTotalSizeOfAdditionalPTS();

    while (true) {
        updateAndersen();

        set<const SVFFunction*> processed;
        runDetector(function, &processed);
        long newAdditionalPTSCount = getTotalSizeOfAdditionalPTS();
        if (newAdditionalPTSCount == lastPTSSize) {
            break;
        }
        cout << "pts grew from " << lastPTSSize << " to " << newAdditionalPTSCount << ". performing another iteration. " << endl;
        lastPTSSize = newAdditionalPTSCount;
    }
}