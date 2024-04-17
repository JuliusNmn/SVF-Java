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

struct JNIInvocation {
    const char* methodName;
    const char*  methodSignature;

//todo: this may not be known statically
// class is often obtained with GetClass of base object
    const char* className;
    const llvm::CallBase* callSite;

    JNIInvocation(const char* methodName, const char* methodSignature, const char* className,
                  const llvm::CallBase* callSite) : methodName(methodName), methodSignature(methodSignature),
                                                    className(className), callSite(callSite) {}
};

struct JNIAllocation {
    std::string className;
    std::string constructorSignature;
    llvm::CallBase* allocSite;
};

class DetectNICalls {
    llvm::StringRef getPassName() const
    {
        return "Transform JNI Calls";
    }


    void handleJniCall(const llvm::Module* module, JNICallOffset offset, const llvm::CallBase* cb);
public:
    void processFunction(const llvm::Function &F);
    std::map<const llvm::CallBase*, JNIInvocation*> detectedJNIInvocations;
    std::map<const llvm::CallBase*, JNIAllocation*> detectedJNIAllocations;
};
#endif //SVF_JAVA_DETECTJNICALLS_H
