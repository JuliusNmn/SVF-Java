package svfjava;
public class SVFG extends CppReference {
    private SVFG(long address) {
        super(address);
    }

    public native void dump(String string);
    /**
    if the passed value was loaded from a function parameter, return the formal parameter SVFValue
     */
    public native VFGNode getFormalParameter(SVFValue val);

    public native VFGNode getFunctionFormalParameter(SVFFunction func, int index);
}