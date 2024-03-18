package svfjava;

public class SVFModule extends CppReference {
    public SVFAnalysisListener listener;
    public SVFG svfg;
    public VFG vfg;
    public SVFIR pag;
    private SVFModule(long address) {
        super(address);
        SVFIRBuilder b = SVFIRBuilder.create(this);
        pag = b.build();
        Andersen ander = Andersen.create(pag);
        PTACallGraph cg = ander.getPTACallGraph();
        vfg = VFG.create(cg);
        SVFGBuilder svfgBuilder = SVFGBuilder.create();
        svfg = svfgBuilder.buildFullSVFG(ander);
    }

    public static native SVFModule createSVFModule(String moduleName);
    public native SVFFunction[] getFunctions();

    public native Object processFunction(SVFFunction function, Object base, Object[] args);
}
