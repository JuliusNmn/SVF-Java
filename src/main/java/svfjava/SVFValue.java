package svfjava;

public class SVFValue extends SVFArgument {
    private SVFValue(long address) {
        super(address);
    }
    native String toStringNative();
    @Override
    public String toString() {
        return toStringNative();
    }
}