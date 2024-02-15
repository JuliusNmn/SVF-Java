package svfjava;

public class SVFValue extends CppReference {
    protected SVFValue(long address) {
        super(address);
    }
    native String toStringNative();
    @Override
    public String toString() {
        return toStringNative();
    }
}