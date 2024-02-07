package svfjava;


public class SVFGBuilder extends CppReference {
    private SVFGBuilder(long address) {
        super(address);
    }

    public static native SVFIRBuilder create();
    public native SVFIR build();
}