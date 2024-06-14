package org.geevm.tests.classfile;

public class Fields {

    private char charField = 'c';
    private byte byteField = 1;
    private short shortField = 2;
    private int intField = 3;
    private long longField = 4L;
    private float floatField = 3.14f;
    private double doubleField = 2.718;
    private String stringField = "hello";

    private int privField = 1;
    public int publicField = 1;
    protected int protectedField = 1;
    int internalField = 1;
    private static int privateStaticField = 1;
    public static int publicStaticField = 1;

    private static final int privateStaticFinalField = 1;
    public final int publicFinalField = 1;

    public volatile int publicVolatileField = 1;
    public transient int publicTransientField = 1;

}
