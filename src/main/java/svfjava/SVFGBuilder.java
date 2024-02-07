package svfjava;


public class SVFGBuilder extends CppReference {
    private SVFGBuilder(long address) {
        super(address);
    }

    public static native SVFGBuilder create();
    public native SVFG buildFullSVFG(Andersen andersen);
}