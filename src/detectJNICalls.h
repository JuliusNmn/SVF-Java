//
// Created by julius on 4/10/24.
//

#ifndef SVF_JAVA_DETECTJNICALLS_H
#define SVF_JAVA_DETECTJNICALLS_H

#include "jni.h"
#include <iostream>
#include <SVF-LLVM/BasicTypes.h>
#include "SVFIR/SVFValue.h"

typedef unsigned long JNICallOffset;

const char* getStringParameterInitializer(const llvm::Value* arg);
class DetectNICalls {
    llvm::StringRef getPassName() const
    {
        return "Transform JNI Calls";
    }

public:
    void processFunction(const llvm::Function &F);
    std::map<const llvm::CallBase*, JNICallOffset> detectedJNICalls;

};
#endif //SVF_JAVA_DETECTJNICALLS_H
