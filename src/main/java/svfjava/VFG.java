package svfjava;
public class VFG extends CppReference {
    private VFG(long address) {
        super(address);
    }

    public static native VFG create(PTACallGraph callGraph);
}
