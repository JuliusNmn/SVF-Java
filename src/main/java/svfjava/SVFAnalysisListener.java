package svfjava;
import java.util.Set;
import java.util.HashSet;
public interface SVFAnalysisListener {
    // argument arrays are guaranteed to be "sets" (unique)
    // return value should be unique
    public long[] nativeToJavaCallDetected(long[] basePTS, String className, String methodName, String methodSignature, long[][] argsPTSs);

    // called when NewObject was detected.
    // context is always null, may be used in future version
    public long jniNewObject(String className, String context);

    // called at GetObjectField etc. invoke site
    public long[] getField(long[] basePTS, String className, String fieldName);

    // called at SetObjectField etc, invoke site
    public void setField(long[] basePTS, String className, String fieldName, long[] argPTS);

    // called at GetArrayElement. index insensitive
    public long[] getArrayElement(long[] basePTS);

    // called at SetArrayElement. index insensitive
    public void setArrayElement(long[] basePTS, long[] argPTS);


}
