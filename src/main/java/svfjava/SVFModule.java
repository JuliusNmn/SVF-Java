package svfjava;

public class SVFModule extends CppReference {
    public SVFAnalysisListener listener;
    public SVFG staticValueFlowGraph;
    public VFG valueFlowGraph;
    public SVFIR programAssignmentGraph;
    public Andersen andersen;
    public PTACallGraph callGraph;
    private SVFModule(long address) {
        super(address);
        SVFIRBuilder b = SVFIRBuilder.create(this);
        programAssignmentGraph = b.build();
        andersen = Andersen.create(programAssignmentGraph);
        callGraph = andersen.getPTACallGraph();
        valueFlowGraph = VFG.create(callGraph);
        SVFGBuilder svfgBuilder = SVFGBuilder.create();
        staticValueFlowGraph = svfgBuilder.buildFullSVFG(andersen);
    }

    public static SVFModule createSVFModuleJava(String moduleName){
       return SVFModule.createSVFModule(moduleName);
    }

    public static native SVFModule createSVFModule(String moduleName);
    public native SVFFunction[] getFunctions();

    public native Object processFunction(SVFFunction function, Object base, Object[] args);
}
