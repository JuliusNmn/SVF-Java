package svfjava;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

public class SVFJava {
    static {
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
        String moduleName = "/home/julius/SVF-Java/libnative.bc";
        if (args.length > 0) {
            moduleName = args[0];
        }
        SVFModule module = SVFModule.createSVFModule(moduleName);
        module.listener = new SVFAnalysisListener() {
              public Object nativeToJavaCallDetected(SVFValue base, String methodName, String methodSignature, SVFValue[] args){
                SVFVar[] basePTS = module.andersen.getPTS(module.pag, base);
                System.out.println("points to set for base: " + basePTS.length);
                for (SVFVar v : basePTS) {
                   SVFValue val = v.getValue();
                   System.out.println("points to " + val);
               }
                return null;
              }
          };
        for (SVFFunction f : module.getFunctions()){
                    SVFValue[] funArgs = module.pag.getArgumentValues(f);
                         System.out.println("funArgs: " + funArgs.length);

                    for (SVFValue arg : funArgs) {
                         System.out.println("argument " + arg);

                    }
                   Object r = module.processFunction(f, null, null);
        }
        //svfg.dump("svfg");
        //  dot -Grankdir=LR -Tpdf svfg.dot -o svfg.pdf
        //new SVFJava().runmain(args);
    }

    private native void runmain(String[] args);
}