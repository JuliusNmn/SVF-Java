package svfjava;
import java.util.Set;
import java.util.HashSet;
public class SVFModule extends CppReference {
    //private long svfg;
    private long extendedPAG;
    private SVFAnalysisListener listener;
    private SVFModule(long address) {
        super(address);
    }

    public static native SVFModule createSVFModule(String moduleName, SVFAnalysisListener listener);
    public native String[] getFunctions();
    public native int getFunctionArgCount(String functionName);
    public native long[] processFunction(String function, long[] basePTS, long[][] argsPTSs);


}
