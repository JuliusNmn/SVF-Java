package svfjava;

public class SVFJava {
    static {
        System.out.println("Loading " + System.mapLibraryName("svf-lib"));
        System.loadLibrary("svf-lib");
    }
    public static void main(String[] args) {
        new SVFJava().runmain(args);
    }

    private native void runmain(String[] args);
}