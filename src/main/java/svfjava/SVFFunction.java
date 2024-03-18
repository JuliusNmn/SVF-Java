package svfjava;

public class SVFFunction extends CppReference {
    private SVFFunction(long address) {
        super(address);
    }

    private native SVFArgument[] getArgumentsNative();
    public SVFArgument[] getArguments() {
        return getArgumentsNative();
    }

}