package svfjava;
import java.util.Set;
import java.util.HashSet;
public interface SVFAnalysisListener {
    // argument arrays are guaranteed to be "sets" (unique)
    // return value should be unique
    public long[] nativeToJavaCallDetected(long[] basePTS, String methodName, String methodSignature, long[][] argsPTSs);

    // called when NewObject was detected.
    // context is always null, may be used in future version
    public long jniNewObject(String className, String context);
}
