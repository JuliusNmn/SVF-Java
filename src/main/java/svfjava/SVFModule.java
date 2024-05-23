package svfjava;
import java.util.Set;
import java.util.Map;
public class SVFModule extends CppReference {
    //private long svfg;
    private long extendedPAG;
    private SVFModule(long address) {
        super(address);
    }

    public static native SVFModule createSVFModule(String moduleName );
    public native String[] getFunctions();
    public native int getFunctionArgCount(String functionName);
    public native long[] processFunction(String function, long[] basePTS, long[][] argsPTSs, SVFAnalysisListener listener);
    //public native Map<Long, String> getNativeAllocSites();

}
