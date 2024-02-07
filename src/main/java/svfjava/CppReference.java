package svfjava;
public class CppReference {
    @SuppressWarnings("unused")
    private long address; // Stores the native C++ pointer address, only used from JNI

    public CppReference(long address) {
        this.address = address;
    }
}