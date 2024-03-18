package svfjava;

public interface SVFAnalysisListener {
    public Object nativeToJavaCallDetected(SVFValue base, String methodName, String methodSignature, SVFValue[] args);
}
