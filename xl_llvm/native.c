#include "jni.h"

void* globb;


jobject llvm_stateaccess_interprocedural_unidirectional_CAccessJava_ReadJavaFieldFromNative_getMyfield(JNIEnv* env, jobject jThis) {
    jclass cls = (*env)->FindClass(env, "Lorg/opalj/fpcf/fixtures/xl/llvm/stateaccess/interprocedural/unidirectional/CAccessJava/ReadJavaFieldFromNative;");
    jfieldID fieldID = (*env)->GetFieldID(env, cls, "myfield", "Ljava/lang/Object;");
    return (*env)->GetObjectField(env, jThis, fieldID);
}
// org/opalj/fpcf/fixtures/xl/llvm/stateaccess/interprocedural/unidirectional/CAccessJava/ReadJavaFieldFromNative
JNIEXPORT jobject JNICALL Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_interprocedural_unidirectional_CAccessJava_ReadJavaFieldFromNative_getMyfield(JNIEnv* env, jobject jThis){
    printf("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_interprocedural_unidirectional_CAccessJava_ReadJavaFieldFromNative_getMyfield");
    return llvm_stateaccess_interprocedural_unidirectional_CAccessJava_ReadJavaFieldFromNative_getMyfield(env, jThis);
}

