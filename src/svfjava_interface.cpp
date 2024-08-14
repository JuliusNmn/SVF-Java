//
// Created by julius on 5/15/24.
//

#include "jni.h"
#include "jniheaders/svfjava_SVFJava.h"
#include "jniheaders/svfjava_SVFModule.h"

#include <iostream>
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "detectJNICalls.h"
#include "main.h"
#include <iostream>

using namespace llvm;
using namespace std;
using namespace SVF;
void *getCppReferencePointer(JNIEnv *env, jobject obj) {
    jclass cls = env->FindClass("svfjava/CppReference");
    jfieldID addressField = env->GetFieldID(cls, "address", "J");
    void *ref = (void *) env->GetLongField(obj, addressField);
    return ref;
}


// Function to convert jlongArray to std::set<long long>
std::set<long> jlongArrayToSet(JNIEnv *env, jlongArray arr) {
    std::set<long> resultSet;

    // Get array length
    jsize len = env->GetArrayLength(arr);

    // Get array elements
    jlong *elements = env->GetLongArrayElements(arr, nullptr);

    // Add elements to the set
    for (int i = 0; i < len; ++i) {
        resultSet.insert(elements[i]);
        assert(elements[i] != 0);
    }

    // Release array elements
    env->ReleaseLongArrayElements(arr, elements, JNI_ABORT);

    return resultSet;
}

/*
 * Class:     svfjava_SVFModule
 * Method:    processFunction
 * Signature: (Ljava/lang/String;[J[J)[J
 */
JNIEXPORT jlongArray JNICALL Java_svfjava_SVFModule_processFunction
        (JNIEnv * env, jobject svfModule, jstring functionName, jlongArray basePTS, jobjectArray argsPTSs, jobject listenerArg) {
    // get data structures (SVFModule, SVFFunction, SVFG, ExtendedPAG)
    SVFModule *module = static_cast<SVFModule *>(getCppReferencePointer(env, svfModule));
    assert(module);
    const char* functionNameStr = env->GetStringUTFChars(functionName, NULL);
    const SVFFunction* function = module->getSVFFunction(functionNameStr);
    if (function) {
        cout << " processFunction called:  " << function->toString() << ENDL;

    } else {
        cout << "function not found: " << functionNameStr << ENDL;
        return nullptr;
    }

    jclass svfModuleClass = env->GetObjectClass(svfModule);
    //jfieldID svfgField = env->GetFieldID(svfModuleClass, "svfg", "J");
    //assert(svfgField);
    //SVFG *svfg = static_cast<SVFG *>((void*)env->GetLongField(svfModule, svfgField));
    //assert(svfg);
    jfieldID ExtendedPAGField = env->GetFieldID(svfModuleClass, "extendedPAG", "J");
    assert(ExtendedPAGField);
    ExtendedPAG* e = static_cast<ExtendedPAG *>((void*)env->GetLongField(svfModule, ExtendedPAGField));
    assert(e);

    //cout << "svfg: " << (long)svfg << ENDL;
    cout << "extended svfg: " << (long)e << ENDL;


    jclass listenerClass = env->GetObjectClass(listenerArg);
    jmethodID listenerCallback1 = env->GetMethodID(listenerClass, "nativeToJavaCallDetected", "([JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[[J)[J");
    assert(listenerCallback1);
    jmethodID listenerCallback2 = env->GetMethodID(listenerClass, "jniNewObject", "(Ljava/lang/String;Ljava/lang/String;)J");
    assert(listenerCallback2);
    jmethodID listenerCallback3 = env->GetMethodID(listenerClass, "getField", "([JLjava/lang/String;Ljava/lang/String;)[J");
    assert(listenerCallback3);
    jmethodID listenerCallback4 = env->GetMethodID(listenerClass, "setField", "([JLjava/lang/String;Ljava/lang/String;[J)V");
    assert(listenerCallback4);
    jmethodID listenerCallback5 = env->GetMethodID(listenerClass, "getArrayElement", "([J)[J");
    assert(listenerCallback5);
    jmethodID listenerCallback6 = env->GetMethodID(listenerClass, "getNativeFunctionArgument", "(Ljava/lang/String;I)[J");
    assert(listenerCallback6);
    //jobject listener = env->NewGlobalRef(listenerArg);
    jobject listener = listenerArg;

    CB_SetArgGetReturnPTS cb1 = [env, listener, listenerCallback1](set<long>* callBasePTS,const char * className,const char * methodName, const char* methodSignature,vector<set<long>*> argumentsPTS) {
        cout << "calling java svf listener. detected method call to " << methodName << " signature " << methodSignature << ENDL;
        // Convert char* to Java strings
        jstring jMethodName = env->NewStringUTF(methodName);
        jstring jMethodSignature = env->NewStringUTF(methodSignature);
        // Convert callBasePTS set to a Java long array
        jlongArray basePTSArray = env->NewLongArray(callBasePTS->size());
        jlong* basePTSArrayElements = env->GetLongArrayElements(basePTSArray, nullptr);
        int i = 0;
        for (long value : *callBasePTS) {
            basePTSArrayElements[i++] = value;
        }
        env->ReleaseLongArrayElements(basePTSArray, basePTSArrayElements, 0);

        // Convert argumentsPTS vector of sets to a Java long 2D array
        jobjectArray argumentsPTSArray = env->NewObjectArray(argumentsPTS.size(), env->FindClass("[J"), nullptr);
        for (size_t i = 0; i < argumentsPTS.size(); ++i) {
            std::set<long>* argumentSet = argumentsPTS[i];
            jlongArray argumentArray = env->NewLongArray(argumentSet->size());
            jlong* argumentArrayElements = env->GetLongArrayElements(argumentArray, nullptr);
            size_t j = 0;
            for (long value : *argumentSet) {
                argumentArrayElements[j++] = value;
            }
            env->ReleaseLongArrayElements(argumentArray, argumentArrayElements, 0);
            env->SetObjectArrayElement(argumentsPTSArray, i, argumentArray);
        }
        jstring jClassName = nullptr;
        if (className) {
            jClassName = env->NewStringUTF(className);
        }
        // Call the Java method
        jlongArray resultArray = (jlongArray)env->CallObjectMethod(listener, listenerCallback1, basePTSArray, jClassName, env->NewStringUTF(methodName), env->NewStringUTF(methodSignature), argumentsPTSArray);
        // Convert the returned long array to a C++ set
        std::set<long>* result = new std::set<long>();
        if (resultArray != nullptr) {
            jsize length = env->GetArrayLength(resultArray);
            jlong* resultElements = env->GetLongArrayElements(resultArray, nullptr);
            for (int i = 0; i < length; ++i) {
                result->insert(resultElements[i]);
            }
            env->ReleaseLongArrayElements(resultArray, resultElements, 0);
        }
        // Release local references
        env->DeleteLocalRef(jMethodName);
        env->DeleteLocalRef(jMethodSignature);
        return result;
    };
    CB_GenerateNativeAllocSiteId  cb2 = [env, listener, listenerCallback2](const char* className, const char* context) {
        jstring jClassName = env->NewStringUTF(className);
        jstring jContext = env->NewStringUTF(context);
        cout << "getting new instanceid from java for " << className << ENDL;
        jlong newInstanceId = env->CallLongMethod(listener, listenerCallback2, jClassName, jContext);
        env->DeleteLocalRef(jClassName);
        env->DeleteLocalRef(jContext);
        return newInstanceId;
    };
    CB_GetFieldPTS cb3 = [env, listener, listenerCallback3](set<long>* basePTS, const char * className, const char * fieldName) {
        //cout << "GetField " << fieldName << ENDL;
        // Convert char* to Java strings
        jstring jClassName = nullptr;
        if (className) {
            jClassName = env->NewStringUTF(className);
        }
        jstring jFieldName = env->NewStringUTF(fieldName);
        // Convert callBasePTS set to a Java long array
        jlongArray basePTSArray = env->NewLongArray(basePTS->size());
        jlong* basePTSArrayElements = env->GetLongArrayElements(basePTSArray, nullptr);
        int i = 0;
        cout << "GetField base pts ";

        for (long value : *basePTS) {
            basePTSArrayElements[i++] = value;
            cout << value << " ";
        }
        cout << ENDL;
        env->ReleaseLongArrayElements(basePTSArray, basePTSArrayElements, 0);
        cout << "requesting pts for field " << ENDL;
        // Call the Java method
        jlongArray resultArray = (jlongArray)env->CallObjectMethod(listener, listenerCallback3, basePTSArray, jClassName, jFieldName);
        // Convert the returned long array to a C++ set
        std::set<long>* result = new std::set<long>();

        if (resultArray != nullptr) {

            jsize length = env->GetArrayLength(resultArray);
            cout << "returned pts size " << length << " elements: ";
            jlong* resultElements = env->GetLongArrayElements(resultArray, nullptr);
            for (int i = 0; i < length; ++i) {
                result->insert(resultElements[i]);
                cout << resultElements[i];
            }
            cout << ENDL;
            env->ReleaseLongArrayElements(resultArray, resultElements, 0);
        }
        // Release local references
        env->DeleteLocalRef(jClassName);
        env->DeleteLocalRef(jFieldName);
        return result;
    };
    CB_SetFieldPTS cb4 = [env, listener, listenerCallback4] (set<long>* basePTS, const char * className, const char * fieldName, set<long>* argPTS) {
        //cout << "SetField " << className << " " << fieldName << ENDL;
        // Convert char* to Java strings
        jstring jClassName = nullptr;
        if (className) {
            jClassName = env->NewStringUTF(className);
        }
        jstring jFieldName = env->NewStringUTF(fieldName);
        // Convert callBasePTS set to a Java long array
        jlongArray basePTSArray = env->NewLongArray(basePTS->size());
        jlong* basePTSArrayElements = env->GetLongArrayElements(basePTSArray, nullptr);
        int i = 0;
        for (long value : *basePTS) {
            basePTSArrayElements[i++] = value;
        }
        env->ReleaseLongArrayElements(basePTSArray, basePTSArrayElements, 0);

        jlongArray argumentArray = env->NewLongArray(argPTS->size());
        jlong* argumentArrayElements = env->GetLongArrayElements(argumentArray, nullptr);
        size_t j = 0;
        for (long value : *argPTS) {
            argumentArrayElements[j++] = value;
        }
        env->ReleaseLongArrayElements(argumentArray, argumentArrayElements, 0);

        // Call the Java method
        env->CallVoidMethod(listener, listenerCallback4, basePTSArray, jClassName, jFieldName, argumentArray);

        // Release local references
        env->DeleteLocalRef(jClassName);
        env->DeleteLocalRef(jFieldName);
        env->DeleteLocalRef(argumentArray);
    };
    CB_GetArrayElementPTS cb5 = [env, listener, listenerCallback5] (set<long>* arrayPTS) {
        cout << "GetArrayElement " << arrayPTS << ENDL;

        jlongArray basePTSArray = env->NewLongArray(arrayPTS->size());
        jlong* basePTSArrayElements = env->GetLongArrayElements(basePTSArray, nullptr);
        int i = 0;
        cout << "GetField base pts ";

        for (long value : *arrayPTS) {
            basePTSArrayElements[i++] = value;
            cout << value << " ";
        }
        cout << ENDL;
        env->ReleaseLongArrayElements(basePTSArray, basePTSArrayElements, 0);
        cout << "requesting pts for field " << ENDL;
        // Call the Java method
        jlongArray resultArray = (jlongArray)env->CallObjectMethod(listener, listenerCallback5, basePTSArray);
        // Convert the returned long array to a C++ set
        std::set<long>* result = new std::set<long>();

        if (resultArray != nullptr) {

            jsize length = env->GetArrayLength(resultArray);
            cout << "returned pts size " << length << " elements: ";
            jlong* resultElements = env->GetLongArrayElements(resultArray, nullptr);
            for (int i = 0; i < length; ++i) {
                result->insert(resultElements[i]);
                cout << resultElements[i];
            }
            cout << ENDL;
            env->ReleaseLongArrayElements(resultArray, resultElements, 0);
        }

        return result;
    };
    CB_GetNativeFunctionArg cb6 = [env, listener, listenerCallback6](const char* name, int index) {
        jstring jMethodName = env->NewStringUTF(name);

        jlongArray resultArray = (jlongArray)env->CallObjectMethod(listener, listenerCallback6, jMethodName, index);

        std::set<long>* result = new std::set<long>();

        if (resultArray != nullptr) {

            jsize length = env->GetArrayLength(resultArray);
            cout << "returned pts size " << length << " elements: ";
            jlong* resultElements = env->GetLongArrayElements(resultArray, nullptr);
            for (int i = 0; i < length; ++i) {
                result->insert(resultElements[i]);
                cout << resultElements[i];
            }
            cout << ENDL;
            env->ReleaseLongArrayElements(resultArray, resultElements, 0);
        }
        // Release local references
        env->DeleteLocalRef(jMethodName);
        return result;
    };
    e->callback_ReportArgPTSGetReturnPTS = cb1;
    e->callback_GenerateNativeAllocSiteId = cb2;
    e->callback_GetFieldPTS = cb3;
    e->callback_SetFieldPTS = cb4;
    e->callback_GetArrayElementPTS = cb5;
    e->callback_GetNativeFunctionArg = cb6;
    //auto pag = svfg->getPAG();
    //assert(pag);
    //auto args = svfg->getPAG()->getFunArgsList(function);
    set<long> basePTSSet = jlongArrayToSet(env, basePTS);
    vector<set<long>> argsPTSsVector;
    // Get array length
    jsize argsCount = env->GetArrayLength(argsPTSs);
    cout << "passed arg count: " << argsCount << ENDL;
    cout << "svffunction arg count: " << function->arg_size() << ENDL;
    // Iterate over args
    for (int i = 0; i < argsCount; ++i) {
        cout << i << ENDL;
        // Get the arg PTS (which is a jlongArray)
        jlongArray rowArray = (jlongArray) env->GetObjectArrayElement(argsPTSs, i);
        set<long> rowSet = jlongArrayToSet(env, rowArray);
        // Add the PTS to the args PTS vector
        argsPTSsVector.push_back(rowSet);
    }

    // public Set<JavaAllocSite> nativeToJavaCallDetected(Set<JavaAllocSite> base, String methodName, String methodSignature, Set<JavaAllocSite>[] args);
    // dot -Grankdir=LR -Tpdf pag.dot -o pag.pdf
    // dot -Grankdir=LR -Tpdf pag2.dot -o pag2.pdf
    // dot -Grankdir=LR -Tpdf svfg.dot -o svfg.pdf
    // dot -Grankdir=LR -Tpdf svfg2.dot -o svfg2.pdf
    std::vector<jobject> argsV;
    cout << "process function" << ENDL;
    set<long>* returnPTS = e->processNativeFunction(function, basePTSSet, argsPTSsVector);
    jlongArray returnPTSArray = env->NewLongArray(returnPTS->size());
    jlong* returnPTSArrayElements = env->GetLongArrayElements(returnPTSArray, nullptr);
    int i = 0;
    cout << "reporting return PTS for function " << function->getName() << ENDL;
    cout << "PTS size: " << returnPTS->size() << " elements: ";
    for (long value : *returnPTS) {
        returnPTSArrayElements[i++] = value;
        cout << value;
    }
    cout << ENDL;
    delete returnPTS;
    env->ReleaseLongArrayElements(returnPTSArray, returnPTSArrayElements, 0);
    return returnPTSArray;
}

/*
 * Class:     svfjava_SVFModule
 * Method:    createSVFModule
 * Signature: (Ljava/lang/String;Lsvfjava/SVFAnalysisListener;)Lsvfjava/SVFModule;
 */
JNIEXPORT jobject JNICALL Java_svfjava_SVFModule_createSVFModule(JNIEnv *env, jclass cls, jstring moduleName) {
    cout << "SVF-Java built at " << __DATE__ << " " << __TIME__ << ENDL;
    cout << "LLVM Version: " << LLVM_VERSION_STRING << ENDL;
    static map<string, ExtendedPAG*> cachedModules;

    const char *moduleNameStr = env->GetStringUTFChars(moduleName, NULL);
    std::vector<std::string> moduleNameVec;
    moduleNameVec.push_back(moduleNameStr);
    ExtAPI::getExtAPI()->setExtBcPath(SVF_INSTALL_EXTAPI_FILE);
    ExtendedPAG *ex = cachedModules[moduleNameStr];
    SVFModule *svfModule;
    if (!ex) {
        svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
        SVFIRBuilder* builder = new SVFIRBuilder(svfModule);
        SVFIR *pag = builder->build();
        ex = new ExtendedPAG(svfModule, pag);

        cachedModules[moduleNameStr] = ex;

    } else {
        svfModule = ex->module;
        cout << "using cached module " << moduleNameStr << ENDL;
    }
    assert(ex);
    assert(svfModule);
    jclass svfModuleClass = env->FindClass("svfjava/SVFModule");
    jmethodID constructor = env->GetMethodID(svfModuleClass, "<init>", "(J)V");
    jobject svfModuleObject = env->NewObject(svfModuleClass, constructor, (jlong) svfModule);

    //
    //Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
    //SVFGBuilder* svfBuilder = new SVFGBuilder();
    //SVFG *svfg = svfBuilder->buildFullSVFG(ander);
    //assert(svfg->getPAG());

    //env->SetLongField(svfModuleObject, env->GetFieldID(svfModuleClass, "svfg", "J"), (long)svfg);
    env->SetLongField(svfModuleObject, env->GetFieldID(svfModuleClass, "extendedPAG", "J"), (long)ex);
    env->ReleaseStringUTFChars(moduleName, moduleNameStr);

    return svfModuleObject;
}

/*
 * Class:     svfjava_SVFModule
 * Method:    getFunctions
 * Signature: ()[Ljava/lnag/String;
 */
JNIEXPORT jobjectArray JNICALL Java_svfjava_SVFModule_getFunctions
        (JNIEnv * env, jobject jThis){
    SVFModule *svfModule = (SVFModule *) getCppReferencePointer(env, jThis);

    std::vector<const SVFFunction *> functions = svfModule->getFunctionSet();

    jclass strClass = env->FindClass("java/lang/String");

    jobjectArray functionArray = env->NewObjectArray(functions.size(), strClass, nullptr);

    for (size_t i = 0; i < functions.size(); ++i) {
        jstring jName = env->NewStringUTF(functions[i]->getName().c_str());

        env->SetObjectArrayElement(functionArray, i, jName);
    }

    return functionArray;
}
/*
 * Class:     svfjava_SVFModule
 * Method:    getFunctionArgCount
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_svfjava_SVFModule_getFunctionArgCount(JNIEnv* env, jobject jThis, jstring jFunctionName) {
    SVFModule *svfModule = (SVFModule *) getCppReferencePointer(env, jThis);
    const char *funcNameStr = env->GetStringUTFChars(jFunctionName, NULL);
    auto function = svfModule->getSVFFunction(funcNameStr);
    env->ReleaseStringUTFChars(jFunctionName, funcNameStr);
    return function->arg_size();
}