
package svfjava;
public class SVFIRBuilder extends CppReference {
    private SVFIRBuilder(long address) {
        super(address);
    }

    public static native SVFIRBuilder create(SVFModule module);
    public native SVFIR build();
}