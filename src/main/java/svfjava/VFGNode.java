package svfjava;
public class VFGNode extends CppReference {
    private VFGNode(long address) {
        super(address);
    }
    public native String toStringNative();
    public String toString() {
        return toStringNative();
    }
}
