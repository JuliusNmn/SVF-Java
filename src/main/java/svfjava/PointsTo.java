package svfjava;
public class PointsTo extends CppReference {
    private PointsTo(long address) {
        super(address);
    }
    native String toStringNative();
    @Override
    public String toString() {
        return toStringNative();
    }
}