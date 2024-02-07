package svfjava;

public class SVFModule extends CppReference {
    // Constructor is private to control object creation through native methods
    private SVFModule(long address) {
        super(address);
    }

    // Declaration of a native method to create an SVFModule object
    public static native SVFModule createSVFModule(String moduleName);
}
