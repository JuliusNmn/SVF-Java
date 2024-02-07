package svfjava;

public class SVFModule extends CppReference {
    
    private SVFModule(long address) {
        super(address);
    }

    public static native SVFModule createSVFModule(String moduleName);
    public native SVFFunction[] getFunctions();
}
