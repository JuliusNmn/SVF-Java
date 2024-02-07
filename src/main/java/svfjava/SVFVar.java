package svfjava;
public class SVFVar extends CppReference {
    private SVFVar(long address) {
        super(address);
    }
    public native SVFValue getValue();
}