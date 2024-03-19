package svfjava;

public class SVFIR extends CppReference {
    private SVFIR(long address) {
        super(address);
    }
    public native SVFVar getGNode(PointsTo pointsTo);
    public native NodeID getValueNode(SVFValue val);

   /**
     these values are taken from the PAG
     */
    public native SVFValue[] getArgumentValues(SVFFunction func);
}
