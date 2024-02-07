package svfjava;
public class Andersen extends CppReference {
    private Andersen(long address) {
        super(address);
    }

    public static native Andersen create(SVFIR ir);
    public native PTACallGraph getPTACallGraph();
}