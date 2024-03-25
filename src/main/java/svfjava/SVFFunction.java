package svfjava;

public class SVFFunction extends CppReference {

    private SVFFunction(long address, String name) {
        super(address);
        this.name = name;
    }

    private native SVFArgument[] getArgumentsNative();
    public SVFArgument[] getArguments() {
        return getArgumentsNative();
    }

    private String name;
    public String getName() {return name;}
}