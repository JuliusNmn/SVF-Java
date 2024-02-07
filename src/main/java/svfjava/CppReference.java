package svfjava;
public class CppReference {
    private long address; // Stores the native C++ pointer address

    public CppReference(long address) {
        this.address = address;
    }
}