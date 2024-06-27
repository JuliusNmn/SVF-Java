package svfjava;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.HashSet;
import java.util.Arrays;

public class SVFJava {
    private static boolean loaded = false;
    public static void init() {
        if (loaded)
            return;
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
        loaded = true;
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

    public static class DummyListener implements SVFAnalysisListener {
        long counter = 1;
        public long[] nativeToJavaCallDetected(long[] basePTS, String className, String methodName, String methodSignature, long[][] argsPTSs) {
                    System.out.println("got pts request for function " + methodName + " arg count " + argsPTSs.length);
                    System.out.println("base pts: " + Arrays.toString(basePTS));
                    for (int i = 0; i < argsPTSs.length; i++) {
                        System.out.println("arg " + i + " pts: " + Arrays.toString(argsPTSs[i]));
                    }
                    // dummy implementation
                    // return PTS for call base as return val of function.
                    //return basePTS;
                    long as = counter++;
                    //System.out.println("returning dummy alloc site " + as);
                    return new long[]{};
                }

                public long jniNewObject(String className, String context) {
                    long as = counter++;
                    System.out.println("returning dummy alloc site " + as);
                    return as;
                }
                // called at GetObjectField etc. invoke site
                public long[] getField(long[] basePTS, String className, String fieldName) {
                    System.out.println("GetField " + fieldName);
                    long as = counter++;
                    //System.out.println("returning dummy alloc site " + as);
                    return new long[]{};
                }

                // called at SetObjectField etc, invoke site
                public void setField(long[] basePTS, String className, String fieldName, long[] argPTS){
                    System.out.println("SetField " + fieldName + " pts size: " + argPTS.length);
                    if (argPTS != null){
                        for (long l : argPTS){
                            System.out.println("alloc site " + l);
                        }
                    }
                }

                // called at GetArrayElement. index insensitive
                public long[] getArrayElement(long[] basePTS){
                    System.out.println("GetArrayElement " + basePTS.length);
                    if (basePTS != null){
                        for (long l : basePTS){
                            System.out.println("array alloc site " + l);
                        }
                    }
                    long as = counter++;
                    System.out.println("returning dummy alloc site " + as);
                    return new long[]{as};
                }

                // called at SetArrayElement. index insensitive
                public void setArrayElement(long[] basePTS, long[] argPTS){
                }
    }

    public static void main(String[] args) {
        SVFJava.init();
        if (args.length != 1) {
            System.err.println("Usage: SVFJava path_to_module(.bc|.ll)");
            return;
        }
        String moduleName = args[0];


        System.out.println("Loading Module " + moduleName);
        SVFModule module1 = SVFModule.createSVFModule(moduleName);
        System.out.println("Loaded module.");
        for (String f : module1.getFunctions()) {

            if (f.startsWith("Java_")) {
                System.out.println(f);
                SVFModule module2 = SVFModule.createSVFModule(moduleName);
                int argc = module2.getFunctionArgCount(f);
                //System.out.println(module2.getNativeAllocSites().size());
                long[][] argsPTS = new long[argc-2][];
                for (int i = 0; i < argc - 2; i++){
                    argsPTS[i] = new long[]{1000L + i};
                    long[] a = argsPTS[i];
                    System.out.println(a[0]);
                }

                long[] resultPTS = module2.processFunction(f, new long[]{666}, argsPTS, new DummyListener());
                System.out.println("got pts! " + resultPTS.length);
                for (long i : resultPTS){
                    System.out.println(i);
                }
            }
        }
        /*long[] resultPTS = module1.processFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_controlflow_bidirectional_CallJavaFunctionFromNativeAndReturn_callMyJavaFunctionFromNativeAndReturn", new long[]{666}, new long[][]{new long[]{9000}});
        System.out.println("got pts " + resultPTS.length);

        SVFModule module2 = SVFModule.createSVFModule(moduleName, new DummyListener());
        resultPTS = module2.processFunction("Java_org_opalj_fpcf_fixtures_xl_llvm_stateaccess_unidirectional_WriteJavaFieldFromNative_setMyfield", new long[]{666}, new long[][]{new long[]{432}});
        System.out.println("got pts! " + resultPTS.length);
        */
    }

}