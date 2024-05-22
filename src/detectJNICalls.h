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

// if classname is not known statically, this will be null
// call base is first argument.
    const char* className;
    const llvm::CallBase* callSite;

    JNIInvocation(const char* methodName, const char* methodSignature, const char* className,
                  const llvm::CallBase* callSite) : methodName(methodName), methodSignature(methodSignature),
                                                    className(className), callSite(callSite) {}
};

struct JNIAllocation {
    const char* className;
    const llvm::CallBase* allocSite;

    // id is received from java analysis
    long id;
    JNIAllocation(const char* className, const llvm::CallBase *allocSite) : className(className),
                                                                             allocSite(allocSite) {}
};
struct JNIGetField {
    const llvm::CallBase* callSite;
    const char* fieldName;
    const char* className;
    JNIGetField(const char* fieldName, const char* className, const llvm::CallBase *callSite) :
        fieldName(fieldName), className(className), callSite(callSite) {}
};
struct JNISetField {
    const llvm::CallBase* callSite;
    const char* fieldName;
    const char* className;
    JNISetField(const char* fieldName, const char* className, const llvm::CallBase *callSite) :
            fieldName(fieldName), className(className), callSite(callSite) {}
};
struct JNIGetArrayElement {
    const llvm::CallBase* callSite;
    JNIGetArrayElement(const llvm::CallBase *callSite) : callSite(callSite) {}
};
struct JNISetArrayElement {
    const llvm::CallBase* callSite;
    JNISetArrayElement(const llvm::CallBase *callSite) : callSite(callSite) {}
};
class DetectNICalls {
    llvm::StringRef getPassName() const
    {
        return "Transform JNI Calls";
    }


    void handleJniCall(const llvm::Module* module, JNICallOffset offset, const llvm::CallBase* cb, const llvm::Function& B);
    void handleGetOrSetField(const llvm::Module *pModule, JNICallOffset offset, const llvm::CallBase *pBase, const llvm::Function& B);
    void handleNewObject(const llvm::Module *module, JNICallOffset offset, const llvm::CallBase *cb, const llvm::Function& B);
public:
    void processFunction(const llvm::Function &F);
    std::map<const llvm::CallBase*, JNIInvocation*> detectedJNIInvocations;
    std::map<const llvm::CallBase*, JNIAllocation*> detectedJNIAllocations;
    std::map<const llvm::CallBase*, JNIGetField*> detectedJNIFieldGets;
    std::map<const llvm::CallBase*, JNISetField*> detectedJNIFieldSets;

    std::map<const llvm::CallBase*, JNIGetArrayElement*> detectedJNIArrayElementGets;
    std::map<const llvm::CallBase*, JNISetArrayElement*> detectedJNIArrayElementSets;

};
#endif //SVF_JAVA_DETECTJNICALLS_H
