package svfjava;
import java.util.Set;
import java.util.HashSet;
public class SVFModule extends CppReference {
    public SVFAnalysisListener listener;
    private long svfg;
    private long extendedSVFG;
    private SVFModule(long address) {
        super(address);
    }

    public static SVFModule createSVFModuleJava(String moduleName){
       return SVFModule.createSVFModule(moduleName);
    }

    public static native SVFModule createSVFModule(String moduleName);
    public native String[] getFunctions();

    public native long[] processFunction(String function, long[] basePTS, long[][] argsPTSs);


}
