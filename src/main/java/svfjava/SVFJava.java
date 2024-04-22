package svfjava;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.HashSet;

public class SVFJava {
    public static void init() {
        String libName = System.mapLibraryName("svf-lib");
        System.out.println("Loading " + libName);
        //System.loadLibrary("svf-lib");
        //URL u = SVFJava.class.getClassLoader().getResource(libName);
        //System.out.println("at " + u);

        String tmpLib = null;
        try {
            tmpLib = extractLibrary(libName);
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.load(tmpLib);
    }


    private static String extractLibrary(String name) throws IOException {
        File lib = File.createTempFile(name, null);
        lib.deleteOnExit(); // The file is deleted when the JVM exits
        byte[] buffer = new byte[1024];
        int read;

        InputStream in = SVFJava.class.getResourceAsStream("/" + name);
        if (in == null) {
            throw new FileNotFoundException(name);
        }

        OutputStream out = new FileOutputStream(lib);
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);

        }

        in.close();
        out.close();

        return lib.getAbsolutePath();
    }

    public static void main(String[] args) {
        SVFJava.init();
        if (args.length != 1) {
            System.err.println("Usage: SVFJava path_to_module(.bc|.ll)");
            return;
        }
        String moduleName = args[0];

        SVFModule module = SVFModule.createSVFModule(moduleName, new SVFAnalysisListener() {
            public long[] nativeToJavaCallDetected(long[] basePTS, String className, String methodName, String methodSignature, long[][] argsPTSs) {
                System.out.println("got pts request for function " + methodName);
                // dummy implementation
                // return PTS for call base as return val of function.
                return basePTS;
                //return new long[]{};
            }

            public long jniNewObject(String className, String context) {
                return 1919;
            }
        });
        for (String f : module.getFunctions()) {
            System.out.println(f);
        }
        long[] resultPTS = module.processFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CallJavaFunctionFromNativeAndReturn_callMyJavaFunctionFromNativeAndReturn", new long[]{666}, new long[][]{new long[]{9000}});
        System.out.println("got pts! " + resultPTS.length);
    }

    private native void runmain(String[] args);
}